
#include "Aux_interface.h"

#include "SE_EV_factory_charge_profile.h"

//#############################################################################
//                        get_pevType_batterySize_map
//#############################################################################

/*
std::vector<pev_batterySize_info> get_pevType_batterySize_map()
{
    std::vector<pev_batterySize_info> return_val;
    
    pev_batterySize_info X;
    battery_chemistry bat_chem; 
    double battery_size_kWh, battery_size_with_stochastic_degredation_kWh;
    
    std::vector<EV_type> vehicle_types = get_all_pev_enums();
    for(EV_type pev_type : vehicle_types)
    {
        bool is_succes = get_pev_battery_info(pev_type, bat_chem, battery_size_kWh, battery_size_with_stochastic_degredation_kWh);
        
        if(is_succes)
        {
            X.vehicle_type = pev_type;
            X.battery_size_kWh = battery_size_kWh;
            X.battery_size_with_stochastic_degredation_kWh = battery_size_with_stochastic_degredation_kWh;
            return_val.push_back(X);
        }
        else
            std::cout << "CALDER INPUT DATA ERROR: In get_pevType_batterySize_map, battery info not available for vehicle_type: " << pev_type << std::endl;
    }
    
    return return_val;
}
*/

//#############################################################################
//                        Get Charge Profile Object
//#############################################################################

CP_interface::CP_interface( const std::string& input_path )
    : loader{ load_EV_EVSE_inventory{ input_path } },
    inventory{ this->loader.get_EV_EVSE_inventory() },
    CP_library{ load_CP_library(inventory, false) }
{
}


CP_interface::CP_interface( const std::string& input_path,
                            const bool save_validation_data )
    : loader{ load_EV_EVSE_inventory{ input_path } },
    inventory{ this->loader.get_EV_EVSE_inventory() },
    CP_library{ load_CP_library(inventory, save_validation_data) }
{
}

pev_charge_profile_library CP_interface::load_CP_library( const EV_EVSE_inventory& inventory,
                                                          const bool save_validation_data ) const
{
    const bool create_charge_profile_library = true;
    factory_charge_profile_library CP_Factory{ inventory };
    return CP_Factory.get_charge_profile_library( save_validation_data, create_charge_profile_library );
}

double CP_interface::get_size_of_CP_library_MB()
{
    return (double)sizeof(this->CP_library)/1000000.0;
}


std::map< std::pair<EV_type, EVSE_type>, std::vector<charge_profile_validation_data> > CP_interface::get_CP_validation_data()
{
    return this->CP_validation_data;
}

    
charge_event_P3kW_limits CP_interface::get_charge_event_P3kW_limits( const EV_type pev_type,
                                                                     const EVSE_type SE_type )
{
    pev_charge_profile* CP_ptr = this->CP_library.get_charge_profile(pev_type, SE_type);
    return CP_ptr->get_charge_event_P3kW_limits();
}


std::vector<double> CP_interface::get_P3kW_setpoints_of_charge_profiles( const EV_type pev_type,
                                                                         const EVSE_type SE_type )
{
    pev_charge_profile* CP_ptr = this->CP_library.get_charge_profile(pev_type, SE_type);
    return CP_ptr->get_P3kW_setpoints_of_charge_profiles();
}


pev_charge_profile_result CP_interface::find_result_given_startSOC_and_endSOC( const EV_type pev_type,
                                                                               const EVSE_type SE_type,
                                                                               const double setpoint_P3kW,
                                                                               const double startSOC,
                                                                               const double endSOC )
{
    pev_charge_profile* CP_ptr = this->CP_library.get_charge_profile(pev_type, SE_type);
    return CP_ptr->find_result_given_startSOC_and_endSOC( setpoint_P3kW, startSOC, endSOC );
}


pev_charge_profile_result CP_interface::find_result_given_startSOC_and_chargeTime( const EV_type pev_type,
                                                                                   const EVSE_type SE_type,
                                                                                   const double setpoint_P3kW,
                                                                                   const double startSOC,
                                                                                   const double charge_time_hrs )
{
    pev_charge_profile* CP_ptr = this->CP_library.get_charge_profile(pev_type, SE_type);
    return CP_ptr->find_result_given_startSOC_and_chargeTime( setpoint_P3kW, startSOC, charge_time_hrs );
}


std::vector<pev_charge_profile_result> CP_interface::find_chargeProfile_given_startSOC_and_endSOCs( const EV_type pev_type,
                                                                                                    const EVSE_type SE_type,
                                                                                                    const double setpoint_P3kW,
                                                                                                    const double startSOC,
                                                                                                    const std::vector<double> endSOC )
{
    pev_charge_profile* CP_ptr = this->CP_library.get_charge_profile( pev_type, SE_type );
    
    std::vector<pev_charge_profile_result> charge_profile;
    CP_ptr->find_chargeProfile_given_startSOC_and_endSOCs( setpoint_P3kW, startSOC, endSOC, charge_profile );
    
    return charge_profile;
}


std::vector<pev_charge_profile_result> CP_interface::find_chargeProfile_given_startSOC_and_chargeTimes( const EV_type pev_type,
                                                                                                        const EVSE_type SE_type,
                                                                                                        const double setpoint_P3kW,
                                                                                                        const double startSOC,
                                                                                                        const std::vector<double> charge_time_hrs )
{
    pev_charge_profile* CP_ptr = this->CP_library.get_charge_profile( pev_type, SE_type );
        
    std::vector<pev_charge_profile_result> charge_profile;
    CP_ptr->find_chargeProfile_given_startSOC_and_chargeTimes( setpoint_P3kW, startSOC, charge_time_hrs, charge_profile );
    
    return charge_profile;
}


std::vector<pev_charge_fragment> CP_interface::USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile( const double time_step_sec,
                                                                                                   const double target_acP3_kW,
                                                                                                   const EV_type pev_type,
                                                                                                   const EVSE_type SE_type ) const
{
    factory_charge_profile_library CP_Factory{this->inventory};
    return CP_Factory.USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile( time_step_sec, target_acP3_kW, pev_type, SE_type );
}

//#############################################################################
//                       Get Charge Profile Object (v2)
//#############################################################################


all_charge_profile_data CP_interface_v2::create_charge_profile_from_model( const double time_step_sec, 
                                                                           const EV_type pev_type, 
                                                                           const EVSE_type SE_type, 
                                                                           const double start_soc, 
                                                                           const double end_soc, 
                                                                           const double target_acP3_kW, 
                                                                           const EV_ramping_map ramping_by_pevType_only, 
                                                                           const EV_EVSE_ramping_map ramping_by_pevType_seType )
{
    all_charge_profile_data return_val;
    return_val.timestep_sec = time_step_sec;
    
    if(end_soc < start_soc)
    {
        std::cout << "ERROR:  start_soc must be less than end_soc in CP_interface_v2::create_charge_profile_from_model." << std::endl;
        return return_val;
    }
    
    //---------------------
    
    pev_SE_pair pev_SE;
    pev_SE.ev_type = pev_type;
    pev_SE.se_type = SE_type;
    
    std::vector<double> soc;
    std::vector<ac_power_metrics> ac_power_vec;
    
    // "soc" and "ac_power_vec" are initialized by calling this function.
    factory_charge_profile_library_v2::create_charge_profile( this->inventory, time_step_sec, pev_SE, start_soc, end_soc, target_acP3_kW, soc, ac_power_vec );

    //---------------------
    
    const int num_elements = soc.size();
    
    for( int i = 0; i < num_elements; i++ )
    {
        return_val.P1_kW.push_back(ac_power_vec.at(i).P1_kW);
        return_val.P2_kW.push_back(ac_power_vec.at(i).P2_kW);
        return_val.P3_kW.push_back(ac_power_vec.at(i).P3_kW);
        return_val.Q3_kVAR.push_back(ac_power_vec.at(i).Q3_kVAR);
        return_val.soc.push_back(soc.at(i));
    }
    
    //-----------------------------
    
    return return_val;
}

CP_interface_v2::CP_interface_v2( const std::string& input_path )
    : loader{ load_EV_EVSE_inventory{ input_path } },
    inventory{ this->loader.get_EV_EVSE_inventory() },
    CP_library_v2{ pev_charge_profile_library_v2 {this->inventory} } 
{};


CP_interface_v2::CP_interface_v2( const std::string& input_path,
                                  const double L1_timestep_sec, 
                                  const double L2_timestep_sec, 
                                  const double HPC_timestep_sec )
    : loader{ load_EV_EVSE_inventory{ input_path } },
    inventory{ this->loader.get_EV_EVSE_inventory() },
    CP_library_v2{
        factory_charge_profile_library_v2{ this->inventory }.get_charge_profile_library( L1_timestep_sec, L2_timestep_sec, HPC_timestep_sec )
    }
{
}

std::vector<double> CP_interface_v2::get_P3kW_charge_profile( const double start_soc,
                                                              const double end_soc,
                                                              const EV_type pev_type,
                                                              const EVSE_type SE_type )
{
    std::vector<double> return_val;
    this->CP_library_v2.get_P3kW_charge_profile( start_soc, end_soc, pev_type, SE_type, this->timestep_sec, return_val );
    
    return return_val;
}


double CP_interface_v2::get_timestep_of_prev_call_sec()
{
    return this->timestep_sec;
}


all_charge_profile_data CP_interface_v2::get_all_charge_profile_data( const double start_soc,
                                                                      const double end_soc,
                                                                      const EV_type pev_type,
                                                                      const EVSE_type SE_type )
{
    all_charge_profile_data return_val;
    this->CP_library_v2.get_all_charge_profile_data( start_soc, end_soc, pev_type, SE_type, return_val );
    
    return return_val;
}


//#############################################################################
//                       Get Base Load Forecast
//#############################################################################


get_baseLD_forecast::get_baseLD_forecast( const double data_start_unix_time,
                                          const int data_timestep_sec,
                                          const std::vector<double> actual_load_akW,
                                          const std::vector<double> forecast_load_akW,
                                          const double adjustment_interval_hrs )
{
    get_base_load_forecast X( data_start_unix_time, data_timestep_sec, actual_load_akW, forecast_load_akW, adjustment_interval_hrs );
    this->baseLD_forecaster = X;
}


std::vector<double> get_baseLD_forecast::get_forecast_akW( const double unix_start_time,
                                                           const int forecast_timestep_mins,
                                                           const double forecast_duration_hrs )
{
    return this->baseLD_forecaster.get_forecast_akW(unix_start_time, forecast_timestep_mins, forecast_duration_hrs);
}

