#include "Aux_interface.h"
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <functional>
#include "helper.h"                 // line_segment,SOC_vs_P2 

namespace temperature_aware
{

// ***************************************************************************
// ***************************************************************************
// ******************      temperature_gradient_model       ******************
// ***************************************************************************
// ***************************************************************************

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
    
    virtual double eval( const double voltage_V,
                         const double current_kA,
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
        double c0_int;  // Intercept (constant)
        double c1_pwr;  // Coefficient associated with power.
        double c2_Temp; // Coefficient associated with temperature
        double c3_time; // Coefficient associated with time.
        double c4_soc;  // Coefficient associated with SOC.

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

class temperature_gradient_model_v2 : public temperature_gradient_model
{
    private:
    
        // The temperature gradient model takes six coefficients
        // and requires five inputs to evaluate.
        // These coefficients are usually determined by analyzing charge
        // data and using a OLS Regression model using scikit-learn.
        //
        //  dT/dt = c0 + c1*V + c2*C + c3*T + c4*t + c5*SOC
        //
        double c0_int;   // Intercept (constant)
        double c1_volt;  // Coefficient associated with voltage.
        double c2_curr;  // Coefficient associated with current.
        double c3_Temp;  // Coefficient associated with temperature
        double c4_time;  // Coefficient associated with time.
        double c5_soc;   // Coefficient associated with SOC.

    public:
        
        temperature_gradient_model_v2() : c0_int(0.0), c1_volt(0.0), c2_curr(0.0), c3_Temp(0.0), c4_time(0.0), c5_soc(0.0) {}
        temperature_gradient_model_v2( const double c0_int,
                                       const double c1_volt,
                                       const double c2_curr,
                                       const double c3_Temp,
                                       const double c4_time,
                                       const double c5_soc ) : c0_int(c0_int), c1_volt(c1_volt), c2_curr(c2_curr), c3_Temp(c3_Temp), c4_time(c4_time), c5_soc(c5_soc) {}
        
        double eval( const double voltage_V,
                     const double current_kA,
                     const double temperature_C,
                     const double charge_time_sec,
                     const double soc ) const override
        {
            return c0_int + c1_volt*voltage_V + c2_curr*current_kA + c3_Temp*temperature_C + c4_time*charge_time_sec + c5_soc*soc;
        }
};


// ***************************************************************************
// ***************************************************************************
// ******************       max_charging_power_model        ******************
// ***************************************************************************
// ***************************************************************************
// NOTE: This object establishes an upper-bound max power that is possible
//       at each temperature and at each SOC.
//       It's *NOT* the same thing as a power profile.

class max_charging_power_model
{
    public:
    
    virtual double eval_at_T( const double temperature_C ) const
    {
        std::cout << "Error: The base class version of this function should never get called."
                  << "It needs to be overridden in a child class." << std::endl;
        exit(0);
        return 0.0;
    }
    
    virtual double eval_at_SOC( const double soc ) const
    {
        std::cout << "Error: The base class version of this function should never get called."
                  << "It needs to be overridden in a child class." << std::endl;
        exit(0);
        return 0.0;
    }
};

class max_charging_power_model_v1 : public max_charging_power_model
{
    private:
        
        // Piecewise-linear function, represented by a series of points.
        std::vector< double > temperature_C_pts;
        std::vector< double > max_charging_power_kW_at_each_T_pts;
        
        // Piecewise-linear function, represented by a series of points.
        std::vector< double > SOC_pts;
        std::vector< double > max_charging_power_kW_at_each_SOC_pts;
        
    public:
        
        max_charging_power_model_v1() {}
        
        max_charging_power_model_v1( const std::vector<double>& temperature_C_pts,
                                     const std::vector<double>& max_charging_power_kW_at_each_T_pts,
                                     const std::vector<double>& SOC_pts,
                                     const std::vector<double>& max_charging_power_kW_at_each_SOC_pts ) :
                                         temperature_C_pts(temperature_C_pts),
                                         max_charging_power_kW_at_each_T_pts(max_charging_power_kW_at_each_T_pts),
                                         SOC_pts(SOC_pts),
                                         max_charging_power_kW_at_each_SOC_pts(max_charging_power_kW_at_each_SOC_pts)                                 
        {
            if( temperature_C_pts.size() != max_charging_power_kW_at_each_T_pts.size() )
            {
                std::cout << "Error: The temperature and charging-power vectors are not the same length." << std::endl;
                exit(0);
            }
            if( SOC_pts.size() != max_charging_power_kW_at_each_SOC_pts.size() )
            {
                std::cout << "Error: The soc and charging-power vectors are not the same length." << std::endl;
                exit(0);
            }
        }
        
        double eval_at_T( const double temperature_C ) const override
        {
            bool found_it = false;
            double power_val = -999999;
            for( int i = 0; i < temperature_C_pts.size()-1; i++ )
            {
                if( temperature_C >= temperature_C_pts.at(i) && temperature_C < temperature_C_pts.at(i+1) )
                {
                    found_it = true;
                    const double frac = (temperature_C-temperature_C_pts.at(i)) / (temperature_C_pts.at(i+1) - temperature_C_pts.at(i));
                    power_val = (1-frac)*max_charging_power_kW_at_each_T_pts.at(i) + frac*max_charging_power_kW_at_each_T_pts.at(i+1);
                    break;
                }
            }
            if( !found_it )
            {
                std::cout << "Error: Temperature was out of range of the points." << std::endl;
                exit(0);
            }
            return power_val;
        }
        
        double eval_at_SOC( const double soc ) const override
        {
            bool found_it = false;
            double power_val = -999999;
            for( int i = 0; i < SOC_pts.size()-1; i++ )
            {
                if( soc >= SOC_pts.at(i) && soc < SOC_pts.at(i+1) )
                {
                    found_it = true;
                    const double frac = (soc-SOC_pts.at(i)) / (SOC_pts.at(i+1) - SOC_pts.at(i));
                    power_val = (1-frac)*max_charging_power_kW_at_each_SOC_pts.at(i) + frac*max_charging_power_kW_at_each_SOC_pts.at(i+1);
                    break;
                }
            }
            if( !found_it )
            {
                std::cout << "Error: SOC was out of range of the points." << std::endl;
                std::cout << "soc: " << soc << std::endl;
                for( int j = 0; j < SOC_pts.size(); j++ )
                {
                    std::cout << "     SOC_pts.at(j):  " << SOC_pts.at(j) << std::endl;
                }
                exit(0);
            }
            return power_val;
        }
};



// ***************************************************************************
// ***************************************************************************
// ******************       TemperatureAwareProfiles        ******************
// ***************************************************************************
// ***************************************************************************

class TemperatureAwareProfiles
{
    public:
    
    static int get_max_power_level_index_at_current_SOC_and_temperature(
                                                                 const std::vector< SOC_vs_P2 > power_profiles_sorted_low_to_high,
                                                                 const max_charging_power_model& max_power_model,
                                                                 const double temperature_C,
                                                                 const double soc )
    {
        int max_power_level_index_at_current_SOC_and_temperature = 0;
        const double max_power_kW = std::fmin( max_power_model.eval_at_T( temperature_C ), max_power_model.eval_at_SOC( soc ) );
        for( int k = 1; k < power_profiles_sorted_low_to_high.size(); k++ )
        {
            const double tmp_power_kW = TemperatureAwareProfiles::eval_power_at_SOC( soc, power_profiles_sorted_low_to_high.at(k) );
            if( tmp_power_kW < max_power_kW )
            {
                max_power_level_index_at_current_SOC_and_temperature = k;
            }
        }
        return max_power_level_index_at_current_SOC_and_temperature;
    }
    
    
    static double eval_power_at_SOC( const double soc,
                                     const SOC_vs_P2& profile )
    {
        const double power_kW = profile.eval(soc);
        return power_kW;
    }
    
    
    static SOC_vs_P2 generate_temperature_aware_power_profile( const std::vector< SOC_vs_P2 > power_profiles_sorted_low_to_high,
                                                               const temperature_gradient_model& temperature_grad_model,
                                                               const max_charging_power_model& max_power_model,
                                                               const double time_step_sec,
                                                               const double battery_capacity_kWh,
                                                               const double start_soc,
                                                               const double end_soc,
                                                               const double start_temperature_C,
                                                               const double min_temperature_C,
                                                               const double max_temperature_C,
                                                               const int start_power_level_index,
                                                               const double update_power_level_delay_sec,
                                                               const std::string output_file_name,
                                                               std::function<int(
                                                                             const int current_power_level_index,
                                                                             const int max_power_level_index_at_current_temperature,
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
        
        double time_since_power_level_update_sec = 0.0;
        
        int loops_i = 0;
        while( soc < end_soc )
        {
            const double power_kW = TemperatureAwareProfiles::eval_power_at_SOC( soc, power_profiles_sorted_low_to_high.at(pwr_level_i) );
            const double temperature_grad = temperature_grad_model.eval( power_kW, temperature_C, time_sec, soc );
            
            // // Display our progress.
            // if( loops_i % 10000 == 0 )
            // {
            //     std::cout << "loops_i: " << loops_i
            //               << "   time_sec: " << time_sec
            //               << "   soc: " << soc
            //               << "   temperature_C: " << temperature_C
            //               << "   temperature_grad: " << temperature_grad
            //               << "   power_kW: " << power_kW
            //               << "   output_file_name: " << output_file_name
            //               << std::endl;
            // }
            
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
            
            int old_pwr_level_i = pwr_level_i;
            
            if( time_since_power_level_update_sec >= update_power_level_delay_sec )
            {
                // Calculate which power level index is the max index and
                // still lower than the one indicated in 'max_power_model'.
                const int max_power_level_index = TemperatureAwareProfiles::get_max_power_level_index_at_current_SOC_and_temperature(
                                                                                                        power_profiles_sorted_low_to_high,
                                                                                                        max_power_model,
                                                                                                        temperature_C,
                                                                                                        soc );
                // Update the power level if needed.
                pwr_level_i = update_power_level_index_callback( pwr_level_i,
                                                                 max_power_level_index,
                                                                 temperature_C,
                                                                 temperature_grad,
                                                                 min_temperature_C,
                                                                 max_temperature_C );
                
                time_since_power_level_update_sec = 0.0;
            }
            
            // if( start_temperature_C == 38.0 )
            // {
            //     std::cout << "soc: "        << soc
            //               << "  time(min): " << (time_sec/60.0)
            //               << "  bat.temperature(C): " << temperature_C
            //               << "  old_pwr_level_i: " << old_pwr_level_i
            //               << "  new_pwr_level_i: " << pwr_level_i
            //               << "  max_power_level_index: " << max_power_level_index
            //               << std::endl;
            // }                        
            
            loops_i++;
            time_since_power_level_update_sec += time_step_sec;
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
        
        // ----------------------------------------
        // Save the result in a 'SOC_vs_P2' object
        // ----------------------------------------
        const SOC_vs_P2 result_SOCvsP2 = [&] () {
            std::vector<line_segment> curve;
            double zero_slope_threshold = 1e-8;  // <------------------------------- TODO: Does this value matter???
            for( int i = 0; i < time_sec_vec.size()-1; i++ )
            {
                const double x0 = soc_vec.at(i);
                const double x1 = soc_vec.at(i+1);
                
                const double y0 = power_kW_vec.at(i);
                const double y1 = power_kW_vec.at(i+1);
                
                const double a = (y1-y0)/(x1-x0);
                const double b = y0 - a*x0;
                
                const double x_LB = x0;
                const double x_UB = x1;
                
                const line_segment ls(x_LB, x_UB, a, b);
                
                curve.push_back(ls);
            }
            SOC_vs_P2 result_SOCvsP2( curve, zero_slope_threshold );
            return result_SOCvsP2;
        }();
        
        return result_SOCvsP2;
    }
};

} // end namespace temperature_aware
