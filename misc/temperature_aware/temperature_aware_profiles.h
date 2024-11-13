#include "Aux_interface.h"
#include <filesystem>
#include <sstream>
#include "SE_EV_factory_charge_profile.h"
#include <algorithm>
#include <functional>

namespace temperature_aware
{
    
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
        std::cout << "Error: The base class version of this function should never get called."
                  << "It needs to be overridden in a child class." << std::endl;
        exit(0);
        return 0.0;
    }
};

class temperature_gradient_model_v1 : public temperature_gradient_model
{
    private:
    
        // The temperature gradient model takes five coefficients
        // and requires four inputs to evaluate.
        // These coefficients are usually determined by analyzing charge
        // data and using a OLS Regression model using scikit-learn.
        //
        //  dT/dt = c0 + c1*P + c2*T + c3*t + c4*SOC
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
    
    static double eval_power_at_SOC( const SE_power::PowerType power_type,
                                     const double soc,
                                     const all_charge_profile_data profile )
    {
        // ---- helper function ----
        auto get_power_at_index = [&power_type, &profile] ( const int i )
        {
            if( power_type == SE_power::PowerType::P1 )
            {
                return profile.P1_kW.at(i);
            }
            else if( power_type == SE_power::PowerType::P2 )
            {
                return profile.P2_kW.at(i);
            }
            else if( power_type == SE_power::PowerType::P3 )
            {
                return profile.P3_kW.at(i);
            }
            else
            {
                std::cout << "Error: We don't support any other power type here." << std::endl;
                exit(0);
            }
            return 0.0;
        };
        
        // The profile size.
        const int profile_size = profile.soc.size();
        
        // These are set below.
        double power_kW = 0.0;
        bool found_it = false;
        
        if( soc < profile.soc.at(0) )
        {
            power_kW = get_power_at_index(0);
            found_it = true;
        }
        else if( soc >= profile.soc.at(profile.soc.size()-1) )
        {
            power_kW = get_power_at_index(profile.soc.size()-1);
            found_it = true;
        }
        else
        {
            for( int i = 0; i < profile_size; i++ )
            {
                if( soc >= profile.soc.at(i) && soc < profile.soc.at(i+1) )
                {
                    const double soc0 = profile.soc.at(i);
                    const double soc1 = profile.soc.at(i+1);
                    const double frac = (soc - soc0)/(soc1 - soc0);
                    power_kW = (1.0-frac) * get_power_at_index(i) + frac * get_power_at_index(i+1);
                    found_it = true;
                    if( power_kW < 1e-5 )
                    {
                        std::cout << "ERROR: The power is pretty much zero. [1]" << std::endl
                                  << " Are we starting at an SOC that's at the beginning of the curve?" << std::endl
                                  << " We need to start at an SOC above the beginning of the curve." << std::endl;
                        std::cout << "found: "
                                  << "   power_kW: " << power_kW
                                  << "   get_power_at_index(i): " << get_power_at_index(i)
                                  << "   get_power_at_index(i+1): " << get_power_at_index(i+1)
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
                      << "  Are we starting at an SOC that's at the beginning of the curve?" << std::endl
                      << "  We need to start at an SOC above the beginning of the curve." << std::endl;
            std::cout << "found: "
                      << "   power_kW: " << power_kW
                      << "   soc:      " << soc
                      << std::endl;
            exit(0);
        }
        return power_kW;
    }
    
    static void generate_temperature_aware_power_profile( const std::vector< all_charge_profile_data > power_profiles_sorted_low_to_high,
                                                          const temperature_gradient_model& temperature_grad_model,
                                                          const SE_power::PowerType power_type,
                                                          const double time_step_sec,
                                                          const double battery_capacity_kWh,
                                                          const double start_soc,
                                                          const double end_soc,
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
        
        const double time_step_hrs = time_step_sec/3600.0;
        
        std::vector<double> time_sec_vec;
        std::vector<double> soc_vec;
        std::vector<double> power_kW_vec;
        std::vector<double> temperature_C_vec;
        std::vector<double> temperature_gradient_dTdt_vec;
        
        int loops_i = 0;
        while( soc < end_soc )
        {
            const double power_kW = TemperatureAwareProfiles::eval_power_at_SOC( power_type, soc, power_profiles_sorted_low_to_high.at(pwr_level_i) );
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

} // end namespace temperature_aware
