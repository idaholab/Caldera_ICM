#include "temperature_aware_profiles.h"


double find_c_rate_input_given_power(const double actual_bat_size_kWh, const double max_power_kW, const std::string& bat_chem)
{

    std::vector<double> crate_vec = { 0, 1, 2, 3, 4, 5, 6 };
    std::vector<double> LMO_power_vec = { 0, 1.27, 2.42, 3.63, 3.63, 3.63, 3.63 };
    std::vector<double> NMC_power_vec = { 0, 1.25, 2.42, 3.75, 3.75, 3.75, 3.75 };
    std::vector<double> LTO_power_vec = { 0, 1.13, 2.23, 3.36, 4.52, 5.63, 5.63 };

    const std::pair<std::vector<double>, std::vector<double>> crate_vs_pu_power = [&]()
    {
        if (bat_chem == "LMO")
        {
            return std::make_pair(crate_vec, LMO_power_vec);
        }
        else if (bat_chem == "NMC")
        {
            return std::make_pair(crate_vec, NMC_power_vec);
        }
        else if (bat_chem == "LTO")
        {
            return std::make_pair(crate_vec, LTO_power_vec);
        }
        else
        {
            std::cout << "Error: battery chemistry not supported (next-gen-profiles)." << std::endl;
            return std::make_pair(std::vector<double>{}, std::vector<double>{});
        }
    }();

    double max_charge_time_hrs = actual_bat_size_kWh / max_power_kW;
    double max_c_rate = 1 / max_charge_time_hrs;


    std::vector<crate> c_rate_arr = crate_vs_pu_power.first;
    std::vector<power> power_arr = crate_vs_pu_power.second;

    double weight_0, weight_1, c_rate;

    for (int i = 1; i < power_arr.size(); i++)
    {
        if ((max_c_rate >= power_arr[i - 1]) && max_c_rate < power_arr[i])
        {
            weight_0 = (power_arr[i] - max_c_rate) / (power_arr[i] - power_arr[i - 1]);
            weight_1 = 1 - weight_0;

            c_rate = (weight_0)*c_rate_arr[i - 1] + (weight_1)*c_rate_arr[i];
            return c_rate;
        }
    }
    std::cout << "Error: max_c_rate is out-of-bounds." << std::endl;
    return 0.0;
}


// ------------------------------------------------------------------------
// Old version that is inefficient, and doesn't support
// doing all the different power levels (a.ka. c_rate_scale_factor levls).
// I'm moving away from doing it this way.
// ------------------------------------------------------------------------
// // path_to_inputs:             The path to the inputs directory which holds the files containing the EV and EVSE input parameters
// // pev_type:                   The EV_type.
// // SE_type:                    The EVSE_type.
// // start_soc:                  The start SOC.
// // end_soc:                    The end SOC.
// // c_rate_scale_factor:        The scale factor to apply to the max-C-rate when generating the power curve.
// all_charge_profile_data build_entire_charge_profile_using_ICM___version_1( const std::string path_to_inputs,
//                                                                            const EV_type pev_type,
//                                                                            const EVSE_type se_type,
//                                                                            const double start_soc,
//                                                                            const double end_soc,
//                                                                            const double c_rate_scale_factor )
// {
//     if( fabs( c_rate_scale_factor - 1.0 ) > 1e-5 )
//     {
//         std::cout << "Error: UNDER CONSTRUCTION. 'c_rate_scale_factor' cannot be anything but 1.0 right now." << std::endl;
//         exit(0);
//     }
// 
//     // ######################################################
//     // # 	Build Charge Profile Given Start and End SOC
//     // ######################################################
// 
//     // Initialize the members of the CP_interface_v2 struct
//     const double L1_timestep_sec = 1;                      // Timestep for Level 1 charger
//     const double L2_timestep_sec = 1;                      // Timestep for Level 2 charger
//     const double HPC_timestep_sec = 1;                     // Timestep for High Power charger
// 
//     CP_interface_v2 ICM_v2 {
//         path_to_inputs,
//         L1_timestep_sec,
//         L2_timestep_sec,
//         HPC_timestep_sec
//     };
// 
//     // Generate the charge profile data
//     all_charge_profile_data all_profile_data = ICM_v2.get_all_charge_profile_data( start_soc, end_soc, pev_type, se_type );
// 
//     return all_profile_data;
// }













// ----------
// -- SIMS --
// ----------

int sim_00()
{
    using namespace temperature_aware;
    
    // Build the inventory object.
    const std::string path_to_inputs = "./inputs";
    load_EV_EVSE_inventory inventory_loader( path_to_inputs );
    const EV_EVSE_inventory& inventory = inventory_loader.get_EV_EVSE_inventory();
    const double timestep_sec = 1.0;
    
    // This was hard-coded in 'factory_charge_profile_library_v2::get_charge_profile_library' so I'm doing the same thing here.
    const double target_acP3_kW = 1000000;
    // Hard coded for collecting the power profiles.
    const double power_profiles_time_step_sec = 1.0;
    
    const EV_type pev_type = "ngp_hyundai_ioniq_5_longrange_awd_100";
    const EVSE_type se_type = "xfc_350";
    const double curvegenerate_start_soc = 5;
    const double cruvegenerate_end_soc = 100;
    const std::vector<double> c_rate_scale_factor_levels = [&] () {
        std::vector<double> c_rate_scale_factor_levels;
        const int N = 5;
        for( int i = 0; i < N; i++ )
        {
            c_rate_scale_factor_levels.push_back((i+1)*(1.0/N));
        }
        return c_rate_scale_factor_levels;
    }();
    
    std::stringstream output_file_path_prefix_ss;
    output_file_path_prefix_ss << "./outputs/" << "pevtype_" << pev_type << "__setype_" << se_type;
    const std::string index_to_c_rate_scale_factor_lookup_file_path = "./outputs/index_to_c_rate_scale_factor_lookup.csv";
    
    std::vector< all_charge_profile_data > all_charge_profile_data_vec;
    factory_charge_profile_library_v2::collect_power_profiles( inventory,
                                                               power_profiles_time_step_sec,
                                                               pev_SE_pair(pev_type, se_type),
                                                               curvegenerate_start_soc,
                                                               cruvegenerate_end_soc, 
                                                               target_acP3_kW,
                                                               c_rate_scale_factor_levels,
                                                               all_charge_profile_data_vec );
    output_power_profiles( c_rate_scale_factor_levels,
                           all_charge_profile_data_vec,
                           output_file_path_prefix_ss.str(),
                           index_to_c_rate_scale_factor_lookup_file_path );
    return 0;
    
}


int sim_01()
{
    using namespace temperature_aware;
    
    // Build the inventory object.
    const std::string path_to_inputs = "./inputs";
    load_EV_EVSE_inventory inventory_loader( path_to_inputs );
    const EV_EVSE_inventory& inventory = inventory_loader.get_EV_EVSE_inventory();
    const double timestep_sec = 1.0;
    
    // Setting the temperature model variables.
    const temperature_gradient_model_v1 temperature_grad_model_v1(
            // 0.00020231134745501544, //const double c0_int,
            // 1.73613180e-05,         //const double c1_pwr,
            // -4.31376160e-05,        // const double c2_Temp,
            // 2.56712548e-07,         //const double c3_time,
            // 3.22413435e-07          //const double c4_soc
        
            // -0.0006,  // 0.00020231134745501544, //const double c0_int,
            // 1.73613180e-05,         //const double c1_pwr,
            // -4.31376160e-05,        // const double c2_Temp,
            // 2.56712548e-07,         //const double c3_time,
            // 3.22413435e-07          //const double c4_soc
           
            // -0.0017,
            // 1.73613180e-05,
            // 0.0,
            // 0.0,
            // 0.0
            
            // -0.0017,
            // 9.0e-05,
            // 0.0,
            // 0.0,
            // 0.0
            
            -0.008,
            9.0e-05,
            0.0,
            0.0,
            0.0
    );
    
    // This was hard-coded in 'factory_charge_profile_library_v2::get_charge_profile_library' so I'm doing the same thing here.
    const double target_acP3_kW = 1000000;
    // Hard coded for collecting the power profiles.
    const double power_profiles_time_step_sec = 1.0;
    
    // make the 'power_profiles_sorted_low_to_high'.
    const EV_type pev_type = "ngp_hyundai_ioniq_5_longrange_awd_100";
    const EVSE_type se_type = "xfc_350";
    const double curvegenerate_start_soc = 5;
    const double cruvegenerate_end_soc = 100;
    const std::vector<double> c_rate_scale_factor_levels = [&] () {
        std::vector<double> c_rate_scale_factor_levels;
        const int N = 10;
        for( int i = 0; i < N; i++ )
        {
            c_rate_scale_factor_levels.push_back((i+1)*(1.0/N));
        }
        return c_rate_scale_factor_levels;
    }();
    std::cout << "c_rate_scale_factor_levels: " << std::endl;
    for( const auto& item : c_rate_scale_factor_levels )
    {
        std::cout << "    " << item << std::endl;
    }
    std::stringstream output_file_path_prefix_ss;
    output_file_path_prefix_ss << "./outputs/" << "pevtype_" << pev_type << "__setype_" << se_type;
    const std::string index_to_c_rate_scale_factor_lookup_file_path = "./outputs/index_to_c_rate_scale_factor_lookup.csv";
    
    std::vector< all_charge_profile_data > all_charge_profile_data_vec;
    factory_charge_profile_library_v2::collect_power_profiles( inventory,
                                                               power_profiles_time_step_sec,
                                                               pev_SE_pair(pev_type, se_type),
                                                               curvegenerate_start_soc,
                                                               cruvegenerate_end_soc, 
                                                               target_acP3_kW,
                                                               c_rate_scale_factor_levels,
                                                               all_charge_profile_data_vec );
    
    output_power_profiles( c_rate_scale_factor_levels,
                           all_charge_profile_data_vec,
                           output_file_path_prefix_ss.str(),
                           index_to_c_rate_scale_factor_lookup_file_path );
                           
    const double time_step_sec = 0.1;
    const double battery_capacity_kWh = inventory.get_EV_inventory().at(pev_type).get_usable_battery_size_kWh();
    const double sim_start_soc = 10;
    const double start_temperature_C = 35;
    const double min_temperature_C = 10;
    const double max_temperature_C = 40;
    const SE_power::PowerType power_type = SE_power::PowerType::P2;
    const int start_power_level_index = all_charge_profile_data_vec.size()-1;
    const std::string sim_results_output_file_name = "./outputs/sim_results.csv";
    
    // Define the callback (v1).
    auto update_power_level_index_callback_v1 = [&] (
        const int current_power_level_index,
        const int power_profiles_vec_size,
        const double current_temperature_C,
        const double current_temperature_grad,
        const double min_temperature_C,
        const double max_temperature_C
    ) {
        if( current_temperature_C >= max_temperature_C )
        {
            return std::max( current_power_level_index-1, 0 );
        }
        else if( current_temperature_C <= min_temperature_C )
        {
            return std::min( current_power_level_index+1, power_profiles_vec_size-1 );
        }
        else
        {
            return current_power_level_index;
        }
    };
    
    // Define the callback (v2).
    auto update_power_level_index_callback_v2 = [&] (
        const int current_power_level_index,
        const int power_profiles_vec_size,
        const double current_temperature_C,
        const double current_temperature_grad,
        const double min_temperature_C,
        const double max_temperature_C
    ) {
        // Approaching the max temperature threshold:
        const double appoarching_max_temp_threshold = max_temperature_C - (max_temperature_C - min_temperature_C)/10;
        const double upper_gradient_threshold = (max_temperature_C - current_temperature_C)/60.0;
        const double lower_gradient_threshold = (max_temperature_C - current_temperature_C)/600.0;
        if(
            current_temperature_C >= max_temperature_C
            ||
            ( current_temperature_C > appoarching_max_temp_threshold && current_temperature_grad > upper_gradient_threshold )
        )
        {
            // Reduce the power level.
            return std::max( current_power_level_index-1, 0 );
        }
        else if(
            current_temperature_C <= min_temperature_C
            ||
            ( current_temperature_C < appoarching_max_temp_threshold && current_temperature_grad < lower_gradient_threshold )
        )
        {
            // Increase the power level.
            return std::min( current_power_level_index+1, power_profiles_vec_size-1 );
        }
        else
        {
            return current_power_level_index;
        }
    };
    
    // Generate the temperature-aware profile.
    TemperatureAwareProfiles::generate_temperature_aware_power_profile( all_charge_profile_data_vec,
                                                                        temperature_grad_model_v1,
                                                                        power_type,
                                                                        time_step_sec,
                                                                        battery_capacity_kWh,
                                                                        sim_start_soc,
                                                                        start_temperature_C,
                                                                        min_temperature_C,
                                                                        max_temperature_C,
                                                                        start_power_level_index,
                                                                        sim_results_output_file_name,
                                                                        update_power_level_index_callback_v2 );
    return 0;
}





// ----------
// -- MAIN --
// ----------

int main()
{
    int ret_val;
    //ret_val += sim_00();
    ret_val += sim_01();
    return ret_val;
}



