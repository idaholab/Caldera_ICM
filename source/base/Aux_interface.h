
#ifndef inl_Aux_interface_H
#define inl_Aux_interface_H

#include "datatypes_global.h"                       // SupplyEquipmentId,  station_charge_event_data, station_status
#include "charge_profile_library.h"                 // pev_charge_profile_library
#include "helper.h"                                 // get_base_load_forecast
#include "load_EV_EVSE_inventory.h"

//std::vector<pev_batterySize_info> get_pevType_batterySize_map();

#include "factory_charging_transitions.h"

class CP_interface
{
private:

    const load_EV_EVSE_inventory loader;
    const EV_EVSE_inventory& inventory;
    pev_charge_profile_library CP_library;    
    std::map< std::pair<EV_type, EVSE_type>, std::vector<charge_profile_validation_data> > CP_validation_data;

public:
    
    CP_interface( const std::string& input_path );
    CP_interface( const std::string& input_path,
                  const bool save_validation_data );

    pev_charge_profile_library load_CP_library( const EV_EVSE_inventory& inventory,
                                                const bool save_validation_data ) const;
    
    double get_size_of_CP_library_MB();
    std::map< std::pair<EV_type, EVSE_type>, std::vector<charge_profile_validation_data> > get_CP_validation_data();
    
    charge_event_P3kW_limits get_charge_event_P3kW_limits( const EV_type pev_type,
                                                           const EVSE_type SE_type );

    std::vector<double> get_P3kW_setpoints_of_charge_profiles( const EV_type pev_type,
                                                               const EVSE_type SE_type );
    
    pev_charge_profile_result find_result_given_startSOC_and_endSOC( const EV_type pev_type,
                                                                     const EVSE_type SE_type,
                                                                     const double setpoint_P3kW,
                                                                     const double startSOC,
                                                                     const double endSOC );

    pev_charge_profile_result find_result_given_startSOC_and_chargeTime( const EV_type pev_type,
                                                                         const EVSE_type SE_type,
                                                                         const double setpoint_P3kW,
                                                                         const double startSOC,
                                                                         const double charge_time_hrs );
    
    std::vector<pev_charge_profile_result> find_chargeProfile_given_startSOC_and_endSOCs( const EV_type pev_type,
                                                                                          const EVSE_type SE_type,
                                                                                          const double setpoint_P3kW,
                                                                                          const double startSOC,
                                                                                          const std::vector<double> endSOC );

    std::vector<pev_charge_profile_result> find_chargeProfile_given_startSOC_and_chargeTimes( const EV_type pev_type,
                                                                                              const EVSE_type SE_type,
                                                                                              const double setpoint_P3kW,
                                                                                              const double startSOC,
                                                                                              const std::vector<double> charge_time_hrs );
    
    std::vector<pev_charge_fragment> USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile( const double time_step_sec,
                                                                                         const double target_acP3_kW,
                                                                                         const EV_type pev_type,
                                                                                         const EVSE_type SE_type) const;
};


class CP_interface_v2
{
private:

    const load_EV_EVSE_inventory loader;
    const EV_EVSE_inventory& inventory;
    pev_charge_profile_library_v2 CP_library_v2;
    double timestep_sec;
    
public:
    // "input_path" is the path to the folder that contains 'EV_inputs.csv' and 'EVSE_inputs.csv'.
    CP_interface_v2( const std::string& input_path );
    CP_interface_v2( const std::string& input_path,
                     const double L1_timestep_sec, 
                     const double L2_timestep_sec, 
                     const double HPC_timestep_sec, 
                     const EV_ramping_map ramping_by_pevType_only, 
                     const EV_EVSE_ramping_map ramping_by_pevType_seType );

    all_charge_profile_data create_charge_profile_from_model( const double time_step_sec, 
                                                              const EV_type pev_type, 
                                                              const EVSE_type SE_type, 
                                                              const double start_soc, 
                                                              const double end_soc, 
                                                              const double target_acP3_kW, 
                                                              const EV_ramping_map ramping_by_pevType_only, 
                                                              const EV_EVSE_ramping_map ramping_by_pevType_seType );
    
    std::vector<double> get_P3kW_charge_profile( const double start_soc,
                                                 const double end_soc,
                                                 const EV_type pev_type,
                                                 const EVSE_type SE_type );
    double get_timestep_of_prev_call_sec();
    
    all_charge_profile_data get_all_charge_profile_data( const double start_soc,
                                                         const double end_soc,
                                                         const EV_type pev_type,
                                                         const EVSE_type SE_type );
};


class get_baseLD_forecast
{
private:
    get_base_load_forecast baseLD_forecaster;

public:
    get_baseLD_forecast() {};
    get_baseLD_forecast( const double data_start_unix_time,
                         const int data_timestep_sec,
                         const std::vector<double> actual_load_akW,
                         const std::vector<double> forecast_load_akW,
                         const double adjustment_interval_hrs );
    std::vector<double> get_forecast_akW( const double unix_start_time,
                                          const int forecast_timestep_mins,
                                          const double forecast_duration_hrs );
};


#endif


