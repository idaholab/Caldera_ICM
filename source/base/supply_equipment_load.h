
#ifndef inl_supply_equipment_load_H
#define inl_supply_equipment_load_H

#include "datatypes_global.h"                       // SE_configuration, SE_charging_status, charge_event_data
#include "ac_to_dc_converter.h"                        // ac_to_dc_converter
#include "vehicle_charge_model.h"                    // vehicle_charge_model
#include "datatypes_module.h"                       // ac_power_metrics, SE_status, CE_Status
#include "charge_profile_library.h"                 // pev_charge_profile, pev_charge_profile_library

#include<vector>
#include <set>
#include <iostream>


class factory_EV_charge_model;
class factory_ac_to_dc_converter;


class charge_event_handler
{
private:
    std::set<charge_event_data> charge_events;
    charge_event_queuing_inputs CE_queuing_inputs;
    
public:
    charge_event_handler() {};
    charge_event_handler( const charge_event_queuing_inputs& CE_queuing_inputs_ );
    
    void add_charge_event( const charge_event_data& CE );
    
    void remove_charge_events_that_are_ending_soon( const double now_unix_time, const double time_limit_seconds );
    
    bool charge_event_is_available( const double now_unix_time ) const;
    
    charge_event_data get_next_charge_event( const double now_unix_time );
};


class supply_equipment_load
{
private:
    double P2_limit_kW, standby_acP_kW, standby_acQ_kVAR;
    
    SE_configuration  SE_config;
    SE_status SE_stat;
    control_strategy_enums control_enums;
    
    charge_event_handler event_handler;
    
    // Both pointer are local to this class.
    ac_to_dc_converter* ac_to_dc_converter_obj;
    vehicle_charge_model* ev_charge_model;               // <--- This object is built by the 'factory_EV_charge_model'.
    
    const factory_EV_charge_model& PEV_charge_factory;   // <--- This factory builds a 'vehicle_charge_model' object.
    const factory_ac_to_dc_converter& ac_to_dc_converter_factory;
    
    const pev_charge_profile_library& charge_profile_library;
    
    //----------------------------
    
    void get_CE_forecast_on_interval(double setpoint_P3kW, double nowSOC, double endSOC, double now_unix_time, double end_unix_time, pev_charge_profile_result& return_val);

public:
    supply_equipment_load(
        const double P2_limit_kW,
        const double standby_acP_kW,
        const double standby_acQ_kVAR,
        const SE_configuration& SE_config,
        const charge_event_queuing_inputs& CE_queuing_inputs,
        const factory_EV_charge_model& PEV_charge_factory,
        const factory_ac_to_dc_converter& ac_to_dc_converter_factory,
        const pev_charge_profile_library& charge_profile_library
    );
    ~supply_equipment_load();
    //supply_equipment_load& operator=(const supply_equipment_load& obj);
    //supply_equipment_load(const supply_equipment_load& obj);
    
    void get_CE_stats_at_end_of_charge(double setpoint_P3kW, double nowSOC, double now_unix_time, bool& pev_is_connected_to_SE, pev_charge_profile_result& return_val);
    void get_CE_FICE(FICE_inputs inputs, double nowSOC, double now_unix_time, bool& pev_is_connected_to_SE, CE_FICE& return_val);
    void get_active_CE(bool& pev_is_connected_to_SE, active_CE& active_CE_val);
    void get_time_to_complete_active_charge_hrs(double setpoint_P3kW, bool& pev_is_connected_to_SE, double& time_to_complete_charge_hrs);

    void get_current_CE_status(bool& pev_is_connected_to_SE, SE_charging_status& SE_status_val, CE_status& charge_status);
    SE_status get_SE_status();
    
    void get_active_charge_profile_forecast_allInfo(double setpoint_P3kW, double time_step_mins, bool& pev_is_connected_to_SE, std::vector<pev_charge_profile_result>& charge_profile);
    void get_active_charge_profile_forecast_akW(double setpoint_P3kW, double time_step_mins, bool& pev_is_connected_to_SE, std::vector<double>& charge_profile);
    
    void add_charge_event( const charge_event_data& charge_event );
    std::vector<completed_CE> get_completed_CE();
    void set_target_acP3_kW(double target_acP3_kW_);
    void set_target_acQ3_kVAR(double target_acQ3_kVAR_);
    double get_PEV_SE_combo_max_nominal_S3kVA();
    
    // pev_is_connected should be updated if ev_charge_model_is_not_NULL then PEV is connected
    bool pev_is_connected_to_SE(double now_unix_time);
    bool pev_is_connected_to_SE__ev_charge_model_not_NULL();
    
    bool get_next(double prev_unix_time, double now_unix_time, double pu_Vrms, double& soc, ac_power_metrics& ac_power);
    void stop_active_CE();
    
    control_strategy_enums get_control_strategy_enums();
    const pev_charge_profile& get_pev_charge_profile();
 };


#endif
