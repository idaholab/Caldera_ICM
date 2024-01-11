
#ifndef inl_supply_equipment_H
#define inl_supply_equipment_H

#include "supply_equipment_load.h"
#include "supply_equipment_control.h"

#include "datatypes_global.h"                       // SE_configuration, SE_charging_status, charge_event_data 
#include "datatypes_module.h"                       // ac_power_metrics, SE_status, CE_Status
#include "charge_profile_library.h"                 // pev_charge_profile_library


class factory_EV_charge_model;
class factory_ac_to_dc_converter;


class supply_equipment
{
private:
    SE_configuration SE_config;
    
    supply_equipment_control SE_control;
    supply_equipment_load SE_Load;
    
public:
    supply_equipment() {};
    supply_equipment(const SE_configuration& SE_config_, supply_equipment_control& SE_control_, supply_equipment_load& SE_Load_);    
    void set_pointers_in_SE_Load(factory_EV_charge_model* PEV_charge_factory, factory_ac_to_dc_converter* ac_to_dc_converter_factory, pev_charge_profile_library* charge_profile_library);
    
    bool is_SE_with_id(SE_id_type SE_id);
    SE_configuration get_SE_configuration();
    SE_status get_SE_status();  // Used in SE_EV_factory.cpp line 1211
    
    void set_ensure_pev_charge_needs_met_for_ext_control_strategy(bool ensure_pev_charge_needs_met);
    
    void get_CE_FICE(FICE_inputs inputs, bool& pev_is_connected_to_SE, CE_FICE& return_val);
    void get_active_CE(bool& pev_is_connected_to_SE, active_CE& active_CE_val);
    std::vector<completed_CE> get_completed_CE();
    control_strategy_enums get_control_strategy_enums();
    
    void get_time_to_complete_active_charge_hrs(double setpoint_P3kW, bool& pev_is_connected_to_SE, double& time_to_complete_charge_hrs);
    void get_active_charge_profile_forecast_allInfo(double setpoint_P3kW, double time_step_mins, bool& pev_is_connected_to_SE, std::vector<pev_charge_profile_result>& charge_profile);
    void get_active_charge_profile_forecast_akW(double setpoint_P3kW, double time_step_mins, bool& pev_is_connected_to_SE, std::vector<double>& charge_profile);
    void add_charge_event( const charge_event_data& charge_event );
	void set_target_acP3_kW(double target_acP3_kW);
    void set_target_acQ3_kVAR(double target_acQ3_kVAR);
    bool pev_is_connected_to_SE(double now_unix_time);
	void get_next(double prev_unix_time, double now_unix_time, double pu_Vrms, double& soc, ac_power_metrics& ac_power);
    void stop_active_CE();
    
    //------------------------------------------
    
    bool current_CE_is_using_control_strategy(double unix_time_of_interest, L2_control_strategies_enum control_enum_);
    bool current_CE_is_using_external_control_strategy(std::string& ext_control);
    void get_external_control_strategy(std::string& return_val);
    
    //------------------------------------------
    
    void ES500_get_charging_needs(double unix_time_now, double unix_time_begining_of_next_agg_step, ES500_aggregator_pev_charge_needs& pev_charge_needs);
    void ES500_set_energy_setpoints(double e3_setpoint_kWh);
};

#endif
