#include "Aux_interface.h"
#include <filesystem>
#include <sstream>

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


// path_to_inputs:             The path to the inputs directory which holds the files containing the EV and EVSE input parameters
// pev_type:                   The EV_type.
// SE_type:                    The EVSE_type.
// start_soc:                  The start SOC.
// end_soc:                    The end SOC.
// power_level_scale_factor:   The scale factor to apply to the max-C-rate when generating the power curve.
all_charge_profile_data build_entire_charge_profile_using_ICM( const std::string path_to_inputs,
                                                               const EV_type pev_type,
                                                               const EVSE_type se_type,
                                                               const double start_soc,
                                                               const double end_soc,
                                                               const double power_level_scale_factor )
{
    if( fabs( power_level_scale_factor - 1.0 ) > 1e-5 )
    {
        std::cout << "Error: UNDER CONSTRUCTION. 'power_level_scale_factor' cannot be anything but 1.0 right now." << std::endl;
        exit(0);
    }
    
    // ######################################################
    // # 	Build Charge Profile Given Start and End SOC
    // ######################################################

    // Initialize the members of the CP_interface_v2 struct
    const double L1_timestep_sec = 1;                      // Timestep for Level 1 charger
    const double L2_timestep_sec = 1;                      // Timestep for Level 2 charger
    const double HPC_timestep_sec = 1;                     // Timestep for High Power charger
    
    CP_interface_v2 ICM_v2 {
        path_to_inputs,
        L1_timestep_sec,
        L2_timestep_sec,
        HPC_timestep_sec
    };

    // Generate the charge profile data
    all_charge_profile_data all_profile_data = ICM_v2.get_all_charge_profile_data( start_soc, end_soc, pev_type, se_type );
    
    return all_profile_data;
}




// Building the curves for Ioniq 5 
// at ten different power levels.
void output_power_profiles( const std::string path_to_inputs,
                            const std::string output_file_path_prefix,
                            const std::string index_to_power_level_file_path,
                            const EV_type pev_type,
                            const EVSE_type se_type,
                            const double start_soc,
                            const double end_soc,
                            const std::vector<double> power_levels )
{
    // The header.
    const std::string header = "time | hrs,SOC | ,P1 | kW,P2 | kW,P3 | kW,Q3 | kVAR\n";
    
    // Loop over each power level.
    for( int i = 0; i < power_levels.size(); i++ )
    {
        // Get the power level
        const double power_level = power_levels.at(i);
        std::cout << "power_level: " << power_level << std::endl;
        
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
        
        // Generate the profile data.
        all_charge_profile_data all_profile_data = build_entire_charge_profile_using_ICM( path_to_inputs,
                                                                                          pev_type,
                                                                                          se_type,
                                                                                          start_soc,
                                                                                          end_soc,
                                                                                          power_level );
        
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
    f_out.open(index_to_power_level_file_path);
    std::cout << "index_to_power_level_file_path: " << index_to_power_level_file_path << std::endl;
    f_out << "index,power_level_scale_factor\n";
    for( int i = 0; i < power_levels.size(); i++ )
    {
        const double power_level = power_levels.at(i);
        f_out << std::setprecision(12) << i << "," << power_level << std::endl;
    }
}


// ----------
// -- MAIN --
// ----------

int main()
{
    const EV_type pev_type = "ngp_hyundai_ioniq_5_longrange_awd_100";
    const EVSE_type se_type = "xfc_350";
    const double start_soc = 10;
    const double end_soc = 100;
    const std::vector<double> power_levels = {1.0};
    const std::string path_to_inputs = "./inputs";
    std::stringstream output_file_path_prefix_ss;
    output_file_path_prefix_ss << "./outputs/" << "pevtype_" << pev_type << "__setype_" << se_type;
    const std::string index_to_power_level_file_path = "./outputs/index_to_power_level_lookup.csv";
    output_power_profiles( path_to_inputs,
                           output_file_path_prefix_ss.str(),
                           index_to_power_level_file_path,
                           pev_type,
                           se_type,
                           start_soc,
                           end_soc,
                           power_levels );
    return 0;
}

