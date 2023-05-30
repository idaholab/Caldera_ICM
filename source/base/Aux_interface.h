
#ifndef inl_Aux_interface_H
#define inl_Aux_interface_H

#include "datatypes_global.h"                       // SE_id_type,  station_charge_event_data, station_status
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
    
    CP_interface(const std::string& input_path);
    CP_interface(const std::string& input_path, bool save_validation_data);

    pev_charge_profile_library load_CP_library(const EV_EVSE_inventory& inventory, bool save_validation_data);
    
    double get_size_of_CP_library_MB();
    std::map< std::pair<EV_type, EVSE_type>, std::vector<charge_profile_validation_data> > get_CP_validation_data();
    
    charge_event_P3kW_limits get_charge_event_P3kW_limits(EV_type pev_type, EVSE_type SE_type);
    std::vector<double> get_P3kW_setpoints_of_charge_profiles(EV_type pev_type, EVSE_type SE_type);
    
    pev_charge_profile_result find_result_given_startSOC_and_endSOC(EV_type pev_type, EVSE_type SE_type, double setpoint_P3kW, double startSOC, double endSOC);
    pev_charge_profile_result find_result_given_startSOC_and_chargeTime(EV_type pev_type, EVSE_type SE_type, double setpoint_P3kW, double startSOC, double charge_time_hrs);
    
    std::vector<pev_charge_profile_result> find_chargeProfile_given_startSOC_and_endSOCs(EV_type pev_type, EVSE_type SE_type, double setpoint_P3kW, double startSOC, std::vector<double> endSOC);
    std::vector<pev_charge_profile_result> find_chargeProfile_given_startSOC_and_chargeTimes(EV_type pev_type, EVSE_type SE_type, double setpoint_P3kW, double startSOC, std::vector<double> charge_time_hrs);
    
    std::vector<pev_charge_fragment> USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile(double time_step_sec, double target_acP3_kW, EV_type pev_type, EVSE_type SE_type);
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
    CP_interface_v2(const std::string& input_path);
    CP_interface_v2(const std::string& input_path,
                    double L1_timestep_sec, 
                    double L2_timestep_sec, 
                    double HPC_timestep_sec, 
                    EV_ramping_map ramping_by_pevType_only, 
                    EV_EVSE_ramping_map ramping_by_pevType_seType);

    all_charge_profile_data create_charge_profile_from_model(double time_step_sec, 
                                                             EV_type pev_type, 
                                                             EVSE_type SE_type, 
                                                             double start_soc, 
                                                             double end_soc, 
                                                             double target_acP3_kW, 
                                                             EV_ramping_map ramping_by_pevType_only, 
                                                             EV_EVSE_ramping_map ramping_by_pevType_seType);
    
    std::vector<double> get_P3kW_charge_profile(double start_soc, double end_soc, EV_type pev_type, EVSE_type SE_type);
    double get_timestep_of_prev_call_sec();
    
    all_charge_profile_data get_all_charge_profile_data(double start_soc, double end_soc, EV_type pev_type, EVSE_type SE_type);
};


class get_baseLD_forecast
{
private:
    get_base_load_forecast baseLD_forecaster;

public:
    get_baseLD_forecast() {};
    get_baseLD_forecast(double data_start_unix_time, int data_timestep_sec, std::vector<double> actual_load_akW, std::vector<double> forecast_load_akW, double adjustment_interval_hrs);
    std::vector<double> get_forecast_akW(double unix_start_time, int forecast_timestep_mins, double forecast_duration_hrs);
};


#endif


