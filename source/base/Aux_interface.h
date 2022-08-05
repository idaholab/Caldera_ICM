
#ifndef inl_Aux_interface_H
#define inl_Aux_interface_H

#include "datatypes_global.h"                       // SE_id_type,  station_charge_event_data, station_status
#include "datatypes_global_SE_EV_definitions.h"     // pev_SE_pair, get_supply_equipment_enum, get_vehicle_enum, supply_equipment_is_L1, supply_equipment_is_L2, pev_is_compatible_with_supply_equipment
#include "charge_profile_library.h"                 // pev_charge_profile_library
#include "helper.h"                                 // get_base_load_forecast


std::vector<pev_batterySize_info> get_pevType_batterySize_map();


class CP_interface
{
private:
    pev_charge_profile_library CP_library;    
    std::map< std::pair<vehicle_enum, supply_equipment_enum>, std::vector<charge_profile_validation_data> > CP_validation_data;

public:
    CP_interface();
    CP_interface(bool save_validation_data);
    
    double get_size_of_CP_library_MB();
    std::map< std::pair<vehicle_enum, supply_equipment_enum>, std::vector<charge_profile_validation_data> > get_CP_validation_data();
    
    charge_event_P3kW_limits get_charge_event_P3kW_limits(vehicle_enum pev_type, supply_equipment_enum SE_type);
    std::vector<double> get_P3kW_setpoints_of_charge_profiles(vehicle_enum pev_type, supply_equipment_enum SE_type);
    
    pev_charge_profile_result find_result_given_startSOC_and_endSOC(vehicle_enum pev_type, supply_equipment_enum SE_type, double setpoint_P3kW, double startSOC, double endSOC);
    pev_charge_profile_result find_result_given_startSOC_and_chargeTime(vehicle_enum pev_type, supply_equipment_enum SE_type, double setpoint_P3kW, double startSOC, double charge_time_hrs);
    
    std::vector<pev_charge_profile_result> find_chargeProfile_given_startSOC_and_endSOCs(vehicle_enum pev_type, supply_equipment_enum SE_type, double setpoint_P3kW, double startSOC, std::vector<double> endSOC);
    std::vector<pev_charge_profile_result> find_chargeProfile_given_startSOC_and_chargeTimes(vehicle_enum pev_type, supply_equipment_enum SE_type, double setpoint_P3kW, double startSOC, std::vector<double> charge_time_hrs);
    
    std::vector<pev_charge_fragment> USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile(double time_step_sec, double target_acP3_kW, vehicle_enum pev_type, supply_equipment_enum SE_type);
};


class CP_interface_v2
{
private:
    pev_charge_profile_library_v2 CP_library_v2;
    double timestep_sec;
    
public:
    CP_interface_v2() {};
    CP_interface_v2(double L1_timestep_sec, double L2_timestep_sec, double HPC_timestep_sec, std::map<vehicle_enum, pev_charge_ramping> ramping_by_pevType_only, std::map< std::tuple<vehicle_enum, supply_equipment_enum>, pev_charge_ramping> ramping_by_pevType_seType);

    all_charge_profile_data create_charge_profile_from_model(double time_step_sec, vehicle_enum pev_type, supply_equipment_enum SE_type, double start_soc, double end_soc, double target_acP3_kW, std::map<vehicle_enum, pev_charge_ramping> ramping_by_pevType_only, std::map< std::tuple<vehicle_enum, supply_equipment_enum>, pev_charge_ramping> ramping_by_pevType_seType);
    
    std::vector<double> get_P3kW_charge_profile(double start_soc, double end_soc, vehicle_enum pev_type, supply_equipment_enum SE_type);
    double get_timestep_of_prev_call_sec();
    
    all_charge_profile_data get_all_charge_profile_data(double start_soc, double end_soc, vehicle_enum pev_type, supply_equipment_enum SE_type);
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


