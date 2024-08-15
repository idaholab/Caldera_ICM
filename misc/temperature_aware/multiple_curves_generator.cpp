#include "Aux_interface.h"
#include <filesystem>
#include <sstream>
#include "SE_EV_factory_charge_profile.h"
#include <algorithm>

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










// Outputting the curves at several different power levels.
// Note: 'c_rate_scale_factor_levels' and 'all_charge_profile_data_vec' must be the same length.
//
void output_power_profiles( const std::vector<double> c_rate_scale_factor_levels,
                            const std::vector< all_charge_profile_data > all_charge_profile_data_vec,
                            const std::string output_file_path_prefix,
                            const std::string index_to_c_rate_scale_factor_lookup_file_path )
{
    if( all_charge_profile_data_vec.size() != c_rate_scale_factor_levels.size() )
    {
        std::cout << "Error: 'all_charge_profile_data_vec' and 'c_rate_scale_factor_levels' are not the same size." << std::endl;
        exit(0);
    }
    
    // The header.
    const std::string header = "time | hrs,SOC | ,P1 | kW,P2 | kW,P3 | kW,Q3 | kVAR\n";
    
    // Loop over each power level.
    for( int i = 0; i < c_rate_scale_factor_levels.size(); i++ )
    {
        // Get the power level
        const double c_rate_scale_factor = c_rate_scale_factor_levels.at(i);
        std::cout << "c_rate_scale_factor: " << c_rate_scale_factor << std::endl;
        
        // Construct the filename.
        const std::string output_file_name = [&] () {
            // Make the number string at the end of the filename.
            std::stringstream file_number_ss;
            if( i >= 0 && i <= 9 )
            {
                file_number_ss << "00" << i;
            }
            else if( i >= 10 && i <= 99 )
            {
                file_number_ss << "0" << i;
            }
            else if( i >= 100 && i <= 999 )
            {
                file_number_ss << "" << i;
            }
            else
            {
                std::cout << "Error: More than 1000 power levels not supported." << std::endl;
                exit(0);
            }
            // Make the filename.
            return output_file_path_prefix+"__pi_"+file_number_ss.str()+".csv";
        }();
        
        // Open the file, and write the header.
        std::ofstream f_out;
        f_out.open(output_file_name);
        std::cout << "output_file_name: " << output_file_name << std::endl;
        f_out << header;
        
        // Get the the profile data.
        const all_charge_profile_data all_profile_data = all_charge_profile_data_vec.at(i);
        
        // Write the data to the file.
        const double start_time_sec = 0.0;
        for (int i = 0; i < all_profile_data.soc.size(); i++)
        {
            f_out << std::setprecision(12) << ((start_time_sec + i * all_profile_data.timestep_sec) / 3600.0) << ",";
            f_out << std::setprecision(12) << (all_profile_data.soc[i]) << ",";
            f_out << std::setprecision(12) << (all_profile_data.P1_kW[i]) << ",";
            f_out << std::setprecision(12) << (all_profile_data.P2_kW[i]) << ",";
            f_out << std::setprecision(12) << (all_profile_data.P3_kW[i]) << ",";
            f_out << std::setprecision(12) << (all_profile_data.Q3_kVAR[i]) << std::endl;
        }
        
        // Close the file.
        f_out.close();
    }
    
    // Write the index-to-power-level lookup file.
    std::ofstream f_out;
    f_out.open(index_to_c_rate_scale_factor_lookup_file_path);
    std::cout << "index_to_c_rate_scale_factor_lookup_file_path: " << index_to_c_rate_scale_factor_lookup_file_path << std::endl;
    f_out << "index,c_rate_scale_factor\n";
    for( int i = 0; i < c_rate_scale_factor_levels.size(); i++ )
    {
        const double c_rate_scale_factor = c_rate_scale_factor_levels.at(i);
        f_out << std::setprecision(12) << i << "," << c_rate_scale_factor << std::endl;
    }
}


class temperature_gradient_model
{
    public:
    
    virtual double eval( const double power_kW,
                         const double temperature_C,
                         const double charge_time_sec,
                         const double soc ) const
    {
        return 0.0;
    }
};

class temperature_gradient_model_v1 : public temperature_gradient_model
{
    private:
    
        // The temperature gradient model takes 4 coefficients
        // and requires 3 inputs to evaluate.
        // These coefficients are usually determined by analyzing charge
        // data and using a OLS Regression model using scikit-learn.
        //
        //  dT/dt = c0 + c1 * P + c2 * T + c3 * t
        //
        const double c0_int;  // Intercept (constant)
        const double c1_pwr;  // Coefficient associated with power.
        const double c2_Temp; // Coefficient associated with temperature
        const double c3_time; // Coefficient associated with time.
        const double c4_soc;  // Coefficient associated with SOC.

    public:
        
        temperature_gradient_model_v1() : c0_int(0.0), c1_pwr(0.0), c2_Temp(0.0), c3_time(0.0), c4_soc(0.0) {}
        temperature_gradient_model_v1( const double c0_int,
                                       const double c1_pwr,
                                       const double c2_Temp,
                                       const double c3_time,
                                       const double c4_soc ) : c0_int(c0_int), c1_pwr(c1_pwr), c2_Temp(c2_Temp), c3_time(c3_time), c4_soc(c4_soc) {}
        
        double eval( const double power_kW,
                     const double temperature_C,
                     const double charge_time_sec,
                     const double soc ) const override
        {
            return c0_int + c1_pwr*power_kW + c2_Temp*temperature_C + c3_time*charge_time_sec + c4_soc*soc;
        }
};


class TemperatureAwareProfiles
{
    public:
    
    static double eval_P2_at_SOC( const double soc, const all_charge_profile_data profile )
    {
        double power_kW = 0.0;
        bool found_it = false;
        if( soc < profile.soc.at(0) )
        {
            power_kW = profile.P2_kW.at(0);
            found_it = true;
        }
        else if( soc >= profile.soc.at(profile.soc.size()-1) )
        {
            power_kW = profile.P2_kW.at(profile.soc.size()-1);
            found_it = true;
        }
        else
        {
            for( int i = 0; i < profile.soc.size(); i++ )
            {
                if( soc >= profile.soc.at(i) && soc < profile.soc.at(i+1) )
                {
                    const double soc0 = profile.soc.at(i);
                    const double soc1 = profile.soc.at(i+1);
                    const double frac = (soc - soc0)/(soc1 - soc0);
                    power_kW = (1.0-frac) * profile.P2_kW.at(i) + frac * profile.P2_kW.at(i+1);
                    found_it = true;
                    if( power_kW < 1e-5 )
                    {
                        std::cout << "ERROR: The power is pretty much zero. [1]" << std::endl
                                  << " Are we starting at an SOC that's at the beginning of the curve?" << std::endl
                                  << " We need to start at an SOC above the beginning of the curve." << std::endl;
                        std::cout << "found power_kW: " << power_kW
                                  << "   profile.P2_kW.at(i): " << profile.P2_kW.at(i)
                                  << "   profile.P2_kW.at(i+1): " << profile.P2_kW.at(i+1)
                                  << "   profile.soc.at(i): " << profile.soc.at(i)
                                  << "   profile.soc.at(i+1): " << profile.soc.at(i+1)
                                  << "   soc: " << soc
                                  << std::endl;
                        exit(0);
                    }
                    break;
                }    
            }
        }
        if( !found_it )
        {
            std::cout << "ERROR: Couldn't evaluate the power for some reason." << std::endl;
            exit(0);
        }
        if( power_kW < 1e-5 )
        {
            std::cout << "ERROR: The power is pretty much zero. [2]" << std::endl
                      << " Are we starting at an SOC that's at the beginning of the curve?" << std::endl
                      << " We need to start at an SOC above the beginning of the curve." << std::endl;
            std::cout << "found power_kW: " << power_kW
                      << "   soc: " << soc
                      << std::endl;
            exit(0);
        }
        return power_kW;
    }
    
    static void generate_temperature_aware_power_profile( const std::vector< all_charge_profile_data > power_profiles_sorted_low_to_high,
                                                          const temperature_gradient_model& temperature_grad_model,
                                                          const double time_step_sec,
                                                          const double battery_capacity_kWh,
                                                          const double start_soc,
                                                          const double start_temperature_C,
                                                          const double min_temperature_C,
                                                          const double max_temperature_C,
                                                          const int start_power_level_index,
                                                          const std::string output_file_name,
                                                          std::function<int(
                                                                        const int current_power_level_index,
                                                                        const int power_profiles_vec_size,
                                                                        const double current_temperature_C,
                                                                        const double current_temperature_grad,
                                                                        const double min_temperature_C,
                                                                        const double max_temperature_C
                                                                    )> update_power_level_index_callback )
    {
        if( min_temperature_C >= max_temperature_C )
        {
            std::cout << "Error: Something is wrong with the min_temperature_C and max_temperature_C." << std::endl;
            exit(0);
        }
        
        double time_sec = 0.0;
        double temperature_C = start_temperature_C;
        double soc = start_soc;
        int pwr_level_i = start_power_level_index;
        
        const double end_soc = 98.8;
        const double time_step_hrs = time_step_sec/3600.0;
        
        std::vector<double> time_sec_vec;
        std::vector<double> soc_vec;
        std::vector<double> power_kW_vec;
        std::vector<double> temperature_C_vec;
        std::vector<double> temperature_gradient_dTdt_vec;
        
        int loops_i = 0;
        while( soc < end_soc )
        {
            const double power_kW = TemperatureAwareProfiles::eval_P2_at_SOC( soc, power_profiles_sorted_low_to_high.at(pwr_level_i) );
            const double temperature_grad = temperature_grad_model.eval( power_kW, temperature_C, time_sec, soc );
            
            // Display our progress.
            std::cout << "loops_i: " << loops_i
                      << "   time_sec: " << time_sec
                      << "   soc: " << soc
                      << "   temperature_C: " << temperature_C
                      << "   temperature_grad: " << temperature_grad
                      << "   power_kW: " << power_kW
                      << std::endl;
            
            // Save the current state into the vectors.
            time_sec_vec.push_back(time_sec);
            soc_vec.push_back(soc);
            power_kW_vec.push_back(power_kW);
            temperature_C_vec.push_back(temperature_C);
            temperature_gradient_dTdt_vec.push_back(temperature_grad);
            
            // Update the SOC, temperature, and time.
            soc += ( power_kW * time_step_hrs / battery_capacity_kWh ) * 100.0;
            temperature_C += temperature_grad * time_step_sec;
            time_sec += time_step_sec;
            
            // Update the power level if needed.
            pwr_level_i = update_power_level_index_callback( pwr_level_i,
                                                             (int)power_profiles_sorted_low_to_high.size(),
                                                             temperature_C,
                                                             temperature_grad,
                                                             min_temperature_C,
                                                             max_temperature_C );
            loops_i++;
        }
        
        // -----------------------------------------------
        // Write the results to disk so we can look at it.
        // -----------------------------------------------
        if( output_file_name != std::string("") )
        {
            std::ofstream fout(output_file_name);
            std::string header = "time_sec,soc,power_kW,temperature_C,temperature_grad_dTdt";
            fout << header << std::endl;
            for( int i = 0; i < time_sec_vec.size(); i++ )
            {
                fout << std::setprecision(12) << time_sec_vec.at(i) << ",";
                fout << std::setprecision(12) << soc_vec.at(i) << ",";
                fout << std::setprecision(12) << power_kW_vec.at(i) << ",";
                fout << std::setprecision(12) << temperature_C_vec.at(i) << ",";
                fout << std::setprecision(12) << temperature_gradient_dTdt_vec.at(i) << std::endl;
            }
            fout.close();
        }
    }
};



// ----------
// -- SIMS --
// ----------

int sim_00()
{
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
    const double battery_capacity_kWh = 74.0;
    const double sim_start_soc = 10;
    const double start_temperature_C = 35;
    const double min_temperature_C = 10;
    const double max_temperature_C = 40;
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



