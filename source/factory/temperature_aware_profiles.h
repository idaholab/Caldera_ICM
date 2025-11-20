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
                         const double bat_temperature_C,
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
                         const double bat_temperature_C,
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
                     const double bat_temperature_C,
                     const double charge_time_sec,
                     const double soc ) const override
        {
            return c0_int + c1_pwr*power_kW + c2_Temp*bat_temperature_C + c3_time*charge_time_sec + c4_soc*soc;
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
                     const double bat_temperature_C,
                     const double charge_time_sec,
                     const double soc ) const override
        {
            return c0_int + c1_volt*voltage_V + c2_curr*current_kA + c3_Temp*bat_temperature_C + c4_time*charge_time_sec + c5_soc*soc;
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
    
    virtual double eval_at_T( const double bat_temperature_C ) const
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
        std::vector< double > bat_temperature_C_pts;
        std::vector< double > max_charging_power_kW_at_each_T_pts;
        
        // Piecewise-linear function, represented by a series of points.
        std::vector< double > SOC_pts;
        std::vector< double > max_charging_power_kW_at_each_SOC_pts;
        
    public:
        
        max_charging_power_model_v1() {}
        
        max_charging_power_model_v1( const std::vector<double>& bat_temperature_C_pts,
                                     const std::vector<double>& max_charging_power_kW_at_each_T_pts,
                                     const std::vector<double>& SOC_pts,
                                     const std::vector<double>& max_charging_power_kW_at_each_SOC_pts ) :
                                         bat_temperature_C_pts(bat_temperature_C_pts),
                                         max_charging_power_kW_at_each_T_pts(max_charging_power_kW_at_each_T_pts),
                                         SOC_pts(SOC_pts),
                                         max_charging_power_kW_at_each_SOC_pts(max_charging_power_kW_at_each_SOC_pts)                                 
        {
            if( bat_temperature_C_pts.size() != max_charging_power_kW_at_each_T_pts.size() )
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
        

#define THROW_ERROR_IF_T_OR_SOC_OUT_OF_RANGE_OF_DATA 0
        
        double eval_at_T( const double bat_temperature_C ) const override
        {
            bool found_it = false;
            double power_val = -999999;
            for( int i = 0; i < bat_temperature_C_pts.size()-1; i++ )
            {
                if( bat_temperature_C >= bat_temperature_C_pts.at(i) && bat_temperature_C < bat_temperature_C_pts.at(i+1) )
                {
                    found_it = true;
                    const double frac = (bat_temperature_C-bat_temperature_C_pts.at(i)) / (bat_temperature_C_pts.at(i+1) - bat_temperature_C_pts.at(i));
                    power_val = (1-frac)*max_charging_power_kW_at_each_T_pts.at(i) + frac*max_charging_power_kW_at_each_T_pts.at(i+1);
                    break;
                }
            }
            if( !found_it )
            {
                if( THROW_ERROR_IF_T_OR_SOC_OUT_OF_RANGE_OF_DATA )
                {
                    std::cout << "Error in 'max_charging_power_model_v1::eval_at_T':   Temperature was out of range of the points.  bat_temperature_C: " << bat_temperature_C << std::endl;
                    exit(0);
                }    
                if( bat_temperature_C >= bat_temperature_C_pts.at( bat_temperature_C_pts.size()-1 ) )
                {
                    power_val = max_charging_power_kW_at_each_T_pts.at( max_charging_power_kW_at_each_T_pts.size()-1 );
                }
                else if( bat_temperature_C <= bat_temperature_C_pts.at(0) )
                {
                    power_val = max_charging_power_kW_at_each_T_pts.at(0);
                }
                else
                {
                    std::cout << "Error in 'max_charging_power_model_v1::eval_at_T." << std::endl;
                    exit(0);
                }
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
                if( THROW_ERROR_IF_T_OR_SOC_OUT_OF_RANGE_OF_DATA )
                {
                    std::cout << "Error: SOC was out of range of the points." << std::endl;
                    std::cout << "soc: " << soc << std::endl;
                    for( int j = 0; j < SOC_pts.size(); j++ )
                    {
                        std::cout << "     SOC_pts.at(j):  " << SOC_pts.at(j) << std::endl;
                    }
                    exit(0);
                }
                if( soc >= SOC_pts.at( SOC_pts.size()-1 ) )
                {
                    power_val = max_charging_power_kW_at_each_SOC_pts.at( max_charging_power_kW_at_each_SOC_pts.size()-1 );
                }
                else if( soc <= SOC_pts.at( 0 ) )
                {
                    power_val = max_charging_power_kW_at_each_SOC_pts.at(0);
                }
                else
                {
                    std::cout << "Error in 'max_charging_power_model_v1::eval_at_SOC." << std::endl;
                    exit(0);
                }
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
                                                                 const double bat_temperature_C,
                                                                 const double soc )
    {
        int max_power_level_index_at_current_SOC_and_temperature = 0;
        const double max_power_kW = std::fmin( max_power_model.eval_at_T( bat_temperature_C ), max_power_model.eval_at_SOC( soc ) );
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
                                                               const double start_battery_temperature_C,
                                                               const double soft_lower_bound_battery_temperature_C,  // <--- a.k.a. the temperature at which it's okay to heat up again (it's okay for the battery to be colder than this).
                                                               const double soft_upper_bound_battery_temperature_C,
                                                               const int start_power_level_index,
                                                               const double update_power_level_delay_sec,
                                                               const std::string optional_output_file_name_for_testing,  // <--- Just set to an empty-string if not needed.
                                                               std::function<int(
                                                                             const int current_power_level_index,
                                                                             const int max_power_level_index_at_current_temperature,
                                                                             const double current_bat_temperature_C,
                                                                             const double current_temperature_grad,
                                                                             const double soft_lower_bound_battery_temperature_C, // <--- a.k.a. the temperature at which it's okay to heat up again (it's okay for the battery to be colder than this).
                                                                             const double soft_upper_bound_battery_temperature_C
                                                                         )> update_power_level_index_callback )
    {
        if( soft_lower_bound_battery_temperature_C >= soft_upper_bound_battery_temperature_C )
        {
            std::cout << "Error: Something is wrong with the soft_lower_bound_battery_temperature_C and soft_upper_bound_battery_temperature_C." << std::endl;
            exit(0);
        }
        
        double time_sec = 0.0;
        double bat_temperature_C = start_battery_temperature_C;
        double soc = start_soc;
        int pwr_level_i = start_power_level_index;
        
        const double time_step_hrs = time_step_sec/3600.0;
        
        std::vector<double> time_sec_vec;
        std::vector<double> soc_vec;
        std::vector<double> power_kW_vec;
        std::vector<double> bat_temperature_C_vec;
        std::vector<double> temperature_gradient_dTdt_vec;
        
        double time_since_power_level_update_sec = 0.0;
        
        int loops_i = 0;
        while( soc < end_soc )
        {
            const double power_kW = TemperatureAwareProfiles::eval_power_at_SOC( soc, power_profiles_sorted_low_to_high.at(pwr_level_i) );
            const double temperature_grad = temperature_grad_model.eval( power_kW, bat_temperature_C, time_sec, soc );
            
            // // Display our progress.
            // if( loops_i % 10000 == 0 )
            // {
            //     std::cout << "loops_i: " << loops_i
            //               << "   time_sec: " << time_sec
            //               << "   soc: " << soc
            //               << "   bat_temperature_C: " << bat_temperature_C
            //               << "   temperature_grad: " << temperature_grad
            //               << "   power_kW: " << power_kW
            //               << "   output_file_name: " << output_file_name
            //               << std::endl;
            // }
            
            // Save the current state into the vectors.
            time_sec_vec.push_back(time_sec);
            soc_vec.push_back(soc);
            power_kW_vec.push_back(power_kW);
            bat_temperature_C_vec.push_back(bat_temperature_C);
            temperature_gradient_dTdt_vec.push_back(temperature_grad);
            
            // Update the SOC, temperature, and time.
            soc += ( power_kW * time_step_hrs / battery_capacity_kWh ) * 100.0;
            bat_temperature_C += temperature_grad * time_step_sec;
            time_sec += time_step_sec;
            
            int old_pwr_level_i = pwr_level_i;
            
            if( time_since_power_level_update_sec >= update_power_level_delay_sec )
            {
                // Calculate which power level index is the max index and
                // still lower than the one indicated in 'max_power_model'.
                const int max_power_level_index = TemperatureAwareProfiles::get_max_power_level_index_at_current_SOC_and_temperature(
                                                                                                        power_profiles_sorted_low_to_high,
                                                                                                        max_power_model,
                                                                                                        bat_temperature_C,
                                                                                                        soc );
                // Update the power level if needed.
                pwr_level_i = update_power_level_index_callback( pwr_level_i,
                                                                 max_power_level_index,
                                                                 bat_temperature_C,
                                                                 temperature_grad,
                                                                 soft_lower_bound_battery_temperature_C,
                                                                 soft_upper_bound_battery_temperature_C );
                
                time_since_power_level_update_sec = 0.0;
            }
            
            // if( start_battery_temperature_C == 38.0 )
            // {
            //     std::cout << "soc: "        << soc
            //               << "  time(min): " << (time_sec/60.0)
            //               << "  bat.temperature(C): " << bat_temperature_C
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
        if( optional_output_file_name_for_testing != std::string("") )
        {
            std::ofstream fout(optional_output_file_name_for_testing);
            std::string header = "time_sec,soc,power_kW,temperature_C,temperature_grad_dTdt";
            fout << header << std::endl;
            for( int i = 0; i < time_sec_vec.size(); i++ )
            {
                fout << std::setprecision(12) << time_sec_vec.at(i) << ",";
                fout << std::setprecision(12) << soc_vec.at(i) << ",";
                fout << std::setprecision(12) << power_kW_vec.at(i) << ",";
                fout << std::setprecision(12) << bat_temperature_C_vec.at(i) << ",";
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
            
            //
            // Add an initial line segment that does minimal power until 'start_soc'
            // so the final curve always starts at soc=0.
            //
            if( soc_vec.at(0) > 0.0 )
            {
                const SOC_vs_P2 lowest_curve = power_profiles_sorted_low_to_high.at(0);
                
                const double x0 = 0.0;
                const double y0 = lowest_curve.eval( 0.0 );
                
                const double x1 = soc_vec.at(0);
                const double y1 = power_kW_vec.at(0);
                
                const line_segment ls( std::make_pair(x0,y0), std::make_pair(x1,y1) );

                curve.push_back(ls);
            }
            
            //
            // Loop through each timestep and add that line_segment to the curve.
            //
            for( int i = 0; i < time_sec_vec.size()-1; i++ )
            {
                const double x0 = soc_vec.at(i);
                const double y0 = power_kW_vec.at(i);
                
                const double x1 = soc_vec.at(i+1);
                const double y1 = power_kW_vec.at(i+1);
                
                const line_segment ls( std::make_pair(x0,y0), std::make_pair(x1,y1) );
                
                curve.push_back(ls);
            }
            
            //
            // Add a final line segment to take the curve to 100% SOC (if needed)
            //
            if( soc_vec.at( soc_vec.size()-1 ) < 100.0 )
            {
                const SOC_vs_P2 lowest_curve = power_profiles_sorted_low_to_high.at(0);
                
                const double x0 = soc_vec.at( soc_vec.size()-1 );
                const double y0 = power_kW_vec.at( soc_vec.size()-1 );
                
                const double x1 = 100.0;
                const double y1 = lowest_curve.eval( 100.0 );
                
                const line_segment ls( std::make_pair(x0,y0), std::make_pair(x1,y1) );
                
                curve.push_back(ls);
            }
            
            SOC_vs_P2 result_SOCvsP2( curve, zero_slope_threshold );
            return result_SOCvsP2;
        }();
        
        return result_SOCvsP2;
    }
};






struct temperature_aware_profiles_data_store
{
    public:
    
    std::vector< double > ambient_temperature_C_vec; // in Celsius
    std::vector< double > start_battery_temperature_C_vec; // in Celsius
    std::vector< double > start_soc_vec;   // in 0 to 100 format.
    std::map< std::tuple< double, double, double >, SOC_vs_P2 > ambT_batT_SOC_tuple_to_power_profile_map;
    
    temperature_aware_profiles_data_store() {}
    
    void add( const double ambient_temperature_C,
              const double start_battery_temperature_C,
              const double start_soc,
              const SOC_vs_P2& profile )
    {
        const std::tuple< double, double, double > key = std::make_tuple(ambient_temperature_C,start_battery_temperature_C,start_soc);
        if( ambT_batT_SOC_tuple_to_power_profile_map.find( key ) != ambT_batT_SOC_tuple_to_power_profile_map.end() )
        {
            // Throw an error becasue we should only be adding keys not already in the map.
            std::cout << "ERROR: adding key that is already in the map." << std::endl;
            exit(1);
        }
        ambT_batT_SOC_tuple_to_power_profile_map.emplace( key, profile );
            
        // Insert the ambient temperature, battery temperature, and SOC for key-look-up.
        ambient_temperature_C_vec.push_back(ambient_temperature_C);
        start_battery_temperature_C_vec.push_back(start_battery_temperature_C);
        start_soc_vec.push_back(start_soc);
        
        // Sort the vectors to ensure they remain in ascending order.
        std::sort(ambient_temperature_C_vec.begin(), ambient_temperature_C_vec.end());
        std::sort(start_battery_temperature_C_vec.begin(), start_battery_temperature_C_vec.end());
        std::sort(start_soc_vec.begin(), start_soc_vec.end());    
    }
    
    // Checks to be sure the data store is 'complete', a.k.a. it has a profile
    // for every possible pair in (selected_ambient_temperature_C x start_battery_temperature_C_vec x start_soc_vec).
    bool complete()
    {
        for( const double amb_temperature : ambient_temperature_C_vec )
        {
            for( const double bat_temperature : start_battery_temperature_C_vec )
            {
                for( const double soc : start_soc_vec )
                {
                    const std::tuple< double, double, double > key = std::make_tuple(amb_temperature,bat_temperature,soc);
                    if( ambT_batT_SOC_tuple_to_power_profile_map.find( key ) == ambT_batT_SOC_tuple_to_power_profile_map.end() )
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    const SOC_vs_P2& lookup_profile( const double ambient_temperature_C,
                                     const double start_battery_temperature_C,
                                     const double start_soc ) const
    {
        // Find the profile whose
        // * ambient temperature is nearest to 'ambient_temperature_C'
        // * starting temperature is nearest to 'start_battery_temperature_C'
        // * starting SOC is nearest to 'start_soc'.
        auto nearest_ambient_temperature_iterator = std::min_element(
            this->ambient_temperature_C_vec.begin(),
            this->ambient_temperature_C_vec.end(),
            [ambient_temperature_C] ( const double a, const double b ) {
                return std::abs(a - ambient_temperature_C) < std::abs(b - ambient_temperature_C);
            }
        );
        auto nearest_bat_temperature_iterator = std::min_element(
            this->start_battery_temperature_C_vec.begin(),
            this->start_battery_temperature_C_vec.end(),
            [start_battery_temperature_C] ( const double a, const double b ) {
                return std::abs(a - start_battery_temperature_C) < std::abs(b - start_battery_temperature_C);
            }
        );
        auto nearest_soc_iterator = std::min_element(
            this->start_soc_vec.begin(),
            this->start_soc_vec.end(),
            [start_soc] ( const double a, const double b ) {
                return std::abs(a - start_soc) < std::abs(b - start_soc);
            }
        );
        if(    nearest_ambient_temperature_iterator == this->ambient_temperature_C_vec.end()
            || nearest_bat_temperature_iterator == this->start_battery_temperature_C_vec.end()
            || nearest_soc_iterator == this->start_soc_vec.end() )
        {
            // ERROR
            std::cout << "ERROR finding nearest in 'temperature_aware_profiles_data_store::lookup_profile'" << std::endl;
            exit(1);
        }
        
        // Get the key values
        const double nearest_ambient_temperature_C = *nearest_ambient_temperature_iterator;
        const double nearest_bat_temperature_C = *nearest_bat_temperature_iterator;
        const double nearest_soc = *nearest_soc_iterator;
        
        // Return the profile.
        const std::tuple< double, double, double > key = std::make_tuple(nearest_ambient_temperature_C,nearest_bat_temperature_C,nearest_soc);
        return ambT_batT_SOC_tuple_to_power_profile_map.at( key );
    }
    
    void write_to_file( std::ostream& fout ) const
    {
        fout << "temperature_aware_profiles_data_store,n_ambient_temperature_values,n_start_battery_temperature_values,n_start_soc_values,"
             << this->ambient_temperature_C_vec.size() << ","
             << this->start_battery_temperature_C_vec.size() << ","
             << this->start_soc_vec.size() << std::endl;
        for (int i = 0; i < this->ambient_temperature_C_vec.size(); i++)
        {
            fout << std::setprecision(16) << this->ambient_temperature_C_vec.at(i) << std::endl;
        }
        for (int i = 0; i < this->start_battery_temperature_C_vec.size(); i++)
        {
            fout << std::setprecision(16) << this->start_battery_temperature_C_vec.at(i) << std::endl;
        }
        for (int i = 0; i < this->start_soc_vec.size(); i++)
        {
            fout << std::setprecision(16) << this->start_soc_vec.at(i) << std::endl;
        }
        fout << "temperature_aware_profiles_data_store,n_SOC_vs_P2_objects," << this->ambT_batT_SOC_tuple_to_power_profile_map.size() << std::endl;
        for( const auto& dddtuple_SOCvsP2obj_pair : this->ambT_batT_SOC_tuple_to_power_profile_map )
        {
            fout << "SOC_vs_P2,ambientT,batteryT,SOC," << std::setprecision(16)
                 << std::get<0>(dddtuple_SOCvsP2obj_pair.first) << ","
                 << std::get<1>(dddtuple_SOCvsP2obj_pair.first) << "," 
                 << std::get<2>(dddtuple_SOCvsP2obj_pair.first) << std::endl;
            dddtuple_SOCvsP2obj_pair.second.write_to_file( fout );
        }
    }
    
    
    
    
    
    
    //*****
    //*****
    //*****
    //**********
    //**********
    //**********
    //***************
    //***************
    //***************
    
    
    void load_from_file( std::istream& fin )
    {
        // --- helper function ---
        auto trim = [&] ( const std::string& s ) -> std::string {
            size_t first = s.find_first_not_of(" \t\n\r\f\v");
            if (first == std::string::npos) {
                return "";
            }
            size_t last = s.find_last_not_of(" \t\n\r\f\v");
            return s.substr(first, last - first + 1);
        };
        
        // **************************************************************
        // Read the lines with the number of values for each vector.
        // **************************************************************
        
        const std::tuple<int,int,int> nAmbTempVals__nStBatTempVals__nStSOCVals__tuple = [&] () -> std::tuple<int,int,int> {
            std::string line;
            std::getline(fin, line);
            
            // Tokenize the line.
            std::stringstream ss;
            ss << trim(line);
            std::vector<std::string> tokens;
            std::string temp_str;
            while(getline(ss, temp_str, ','))
            {
                tokens.push_back(trim(temp_str));
            }
                        
            // Check that we have the right number of tokens.
            if( tokens.size() != 7 )
            {
                std::cout << "Error. Not the right number of tokens! [temperature_aware_profiles_data_store::load_from_file]" << std::endl;
                exit(1);
            }

            std::string temperature_aware_profiles_data_store_str;
            std::string n_ambient_temperature_values_str;
            std::string n_start_battery_temperature_values_str;
            std::string n_start_soc_values_str;
            int n_ambient_temperature_values;
            int n_start_battery_temperature_values;
            int n_start_soc_values;

            int collected_tokens_count = 0;
            int k = -1;
            k++; if( k < tokens.size() ) { temperature_aware_profiles_data_store_str = tokens.at(k).c_str(); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_ambient_temperature_values_str = tokens.at(k).c_str(); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_start_battery_temperature_values_str = tokens.at(k).c_str(); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_start_soc_values_str = tokens.at(k).c_str(); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_ambient_temperature_values = std::atoi(tokens.at(k).c_str()); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_start_battery_temperature_values = std::atoi(tokens.at(k).c_str()); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_start_soc_values = std::atoi(tokens.at(k).c_str()); collected_tokens_count++; }
            
            if( collected_tokens_count != 7 )
            {
                std::cout << "Error. Incorrect number of tokens collected. [temperature_aware_profiles_data_store::load_from_file]" << std::endl;
                exit(1);
            }
            
            return std::make_tuple( n_ambient_temperature_values, n_start_battery_temperature_values, n_start_soc_values );
        }();
        const int n_ambient_temperature_values       = std::get<0>( nAmbTempVals__nStBatTempVals__nStSOCVals__tuple );
        const int n_start_battery_temperature_values = std::get<1>( nAmbTempVals__nStBatTempVals__nStSOCVals__tuple );
        const int n_start_soc_values                 = std::get<2>( nAmbTempVals__nStBatTempVals__nStSOCVals__tuple );
        
        
        // ****************************************
        // Read the data for the three vectors.
        // ****************************************
        
        for( int i = 0; i < n_ambient_temperature_values; i++ )
        {
            std::string line;
            std::getline(fin, line);
            const double value = std::stod(line);
            this->ambient_temperature_C_vec.push_back(value);
        }
        for( int i = 0; i < n_start_battery_temperature_values; i++ )
        {
            std::string line;
            std::getline(fin, line);
            const double value = std::stod(line);
            this->start_battery_temperature_C_vec.push_back(value);
        }
        for( int i = 0; i < n_start_soc_values; i++ )
        {
            std::string line;
            std::getline(fin, line);
            const double value = std::stod(line);
            this->start_soc_vec.push_back(value);
        }
        
        
        
        // ****************************************
        // Read the line with 'n_SOC_vs_P2_objects'
        // ****************************************
        
        const int n_SOC_vs_P2_objects = [&] () -> int {
            std::string line;
            std::getline(fin, line);
            
            // Tokenize the line.
            std::stringstream ss;
            ss << trim(line);
            std::vector<std::string> tokens;
            std::string temp_str;
            while(getline(ss, temp_str, ','))
            {
                tokens.push_back(trim(temp_str));
            }
                        
            // Check that we have the right number of tokens.
            if( tokens.size() != 3 )
            {
                std::cout << "Error. Not the right number of tokens! [temperature_aware_profiles_data_store::load_from_file]" << std::endl;
                exit(1);
            }

            std::string temperature_aware_profiles_data_store_str;
            std::string n_SOC_vs_P2_objects_str;
            int n_SOC_vs_P2_objects;

            int collected_tokens_count = 0;
            int k = -1;
            k++; if( k < tokens.size() ) { temperature_aware_profiles_data_store_str = tokens.at(k).c_str(); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_SOC_vs_P2_objects_str = tokens.at(k).c_str(); collected_tokens_count++; }
            k++; if( k < tokens.size() ) { n_SOC_vs_P2_objects = std::atoi(tokens.at(k).c_str()); collected_tokens_count++; }
            
            if( collected_tokens_count != 3 )
            {
                std::cout << "Error. Incorrect number of tokens collected. [temperature_aware_profiles_data_store::load_from_file]" << std::endl;
                exit(1);
            }
            
            return n_SOC_vs_P2_objects;
        }(); 
        
        
        
        // ***************************
        // Load each SOC_vs_P2 object.
        // ***************************
        
        for( int i = 0; i < n_SOC_vs_P2_objects; i++ )
        {
            // Read the line with the 'ambT_batT_SOC' values (three doubles)
            const std::tuple<double,double,double> ambT_batT_SOCval_tuple = [&] () -> std::tuple<double,double,double> {
                std::string line;
                std::getline(fin, line);
                
                // Tokenize the line.
                std::stringstream ss;
                ss << trim(line);
                std::vector<std::string> tokens;
                std::string temp_str;
                while(getline(ss, temp_str, ','))
                {
                    tokens.push_back(trim(temp_str));
                }
                            
                // Check that we have the right number of tokens.
                if( tokens.size() != 7 )
                {
                    std::cout << "Error. Not the right number of tokens! [temperature_aware_profiles_data_store::load_from_file]" << std::endl;
                    exit(1);
                }

                std::string SOC_vs_P2_str;
                std::string ambientT_str;
                std::string batteryT_str;
                std::string SOC_str;
                double ambT;
                double batT;
                double SOCval;

                int collected_tokens_count = 0;
                int k = -1;
                k++; if( k < tokens.size() ) { SOC_vs_P2_str = tokens.at(k).c_str(); collected_tokens_count++; }
                k++; if( k < tokens.size() ) { ambientT_str = tokens.at(k).c_str(); collected_tokens_count++; }
                k++; if( k < tokens.size() ) { batteryT_str = tokens.at(k).c_str(); collected_tokens_count++; }
                k++; if( k < tokens.size() ) { SOC_str = tokens.at(k).c_str(); collected_tokens_count++; }
                k++; if( k < tokens.size() ) { ambT = std::stod(tokens.at(k).c_str()); collected_tokens_count++; }
                k++; if( k < tokens.size() ) { batT = std::stod(tokens.at(k).c_str()); collected_tokens_count++; }
                k++; if( k < tokens.size() ) { SOCval = std::stod(tokens.at(k).c_str()); collected_tokens_count++; }
                
                if( collected_tokens_count != 7 )
                {
                    std::cout << "Error. Incorrect number of tokens collected. [temperature_aware_profiles_data_store::load_from_file]" << std::endl;
                    exit(1);
                }
                
                return std::make_tuple( ambT, batT, SOCval );
            }();
            const double ambT = std::get<0>(ambT_batT_SOCval_tuple);
            const double batT = std::get<1>(ambT_batT_SOCval_tuple);
            const double SOCval = std::get<2>(ambT_batT_SOCval_tuple);
            
            // Then, make a tuple as the key.
            std::tuple<double,double,double> key = std::make_tuple(ambT, batT, SOCval);
            
            // Read in the SOC_vs_P2 object.
            SOC_vs_P2 new_socvsp2obj;
            new_socvsp2obj.load_from_file( fin );
            
            // Save the SOC_vs_P2 into the map.
            this->ambT_batT_SOC_tuple_to_power_profile_map[ key ] = new_socvsp2obj;
        }
    }
    
    //***************
    //***************
    //***************
    //**********
    //**********
    //**********
    //*****
    //*****
    //*****
    
    void output_to_cache_file( const std::string filename ) const
    {
        std::ofstream opfile;
        opfile.open(filename);
        this->write_to_file( opfile );
        opfile.close();
        return;
    }
    
    void load_from_cache_file( const std::string filename )
    {
        std::ifstream ifile;
        ifile.open(filename);
        this->load_from_file( ifile );
        ifile.close();
        return;
    }
};









} // end namespace temperature_aware
