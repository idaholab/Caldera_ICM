
#include "supply_equipment.h"
#include "factory_EV_charge_model.h"            // factory_EV_charge_model
#include "factory_supply_equipment_model.h"     // factory_supply_equipment_model


supply_equipment::supply_equipment( const SE_configuration& SE_config_,
                                    const supply_equipment_control& SE_control_,
                                    const supply_equipment_load& SE_Load_ )
{
    this->SE_config = SE_config_;
    this->SE_control = SE_control_;
    this->SE_Load = SE_Load_;
}


void supply_equipment::set_pointers_in_SE_Load( factory_EV_charge_model* PEV_charge_factory,
                                                factory_ac_to_dc_converter* ac_to_dc_converter_factory,
                                                pev_charge_profile_library* charge_profile_library )
{
    this->SE_Load.set_pointers(PEV_charge_factory, ac_to_dc_converter_factory, charge_profile_library);
}
   

bool supply_equipment::is_SE_with_id( const SE_id_type SE_id )
{
    return (this->SE_config.SE_id == SE_id);
}


SE_configuration supply_equipment::get_SE_configuration()
{
    return this->SE_config;
}


SE_status supply_equipment::get_SE_status()
{
    return this->SE_Load.get_SE_status();
}


void supply_equipment::set_ensure_pev_charge_needs_met_for_ext_control_strategy( const bool ensure_pev_charge_needs_met )
{
    this->SE_control.set_ensure_pev_charge_needs_met_for_ext_control_strategy(ensure_pev_charge_needs_met);
}


void supply_equipment::get_CE_FICE( const FICE_inputs inputs,
                                    bool& pev_is_connected_to_SE,
                                    CE_FICE& return_val )
{
    SE_status X = this->SE_Load.get_SE_status();
    pev_is_connected_to_SE = X.pev_is_connected_to_SE;
    
    if(pev_is_connected_to_SE)
    {
        double nowSOC = X.current_charge.now_soc;
        double now_unix_time = X.now_unix_time;
        
        this->SE_Load.get_CE_FICE(inputs, nowSOC, now_unix_time, pev_is_connected_to_SE, return_val);
    }
}


void supply_equipment::get_active_CE( bool& pev_is_connected_to_SE,
                                      active_CE& active_CE_val )
{
    this->SE_Load.get_active_CE(pev_is_connected_to_SE, active_CE_val);
}


std::vector<completed_CE> supply_equipment::get_completed_CE()
{
    return this->SE_Load.get_completed_CE();
}


control_strategy_enums supply_equipment::get_control_strategy_enums()
{
    return this->SE_control.get_control_strategy_enums();
}


void supply_equipment::get_time_to_complete_active_charge_hrs( const double setpoint_P3kW,
                                                               bool& pev_is_connected_to_SE,
                                                               double& time_to_complete_charge_hrs )
{
    this->SE_Load.get_time_to_complete_active_charge_hrs( setpoint_P3kW, pev_is_connected_to_SE, time_to_complete_charge_hrs );
}


void supply_equipment::get_active_charge_profile_forecast_allInfo( const double setpoint_P3kW,
                                                                   const double time_step_mins,
                                                                   bool& pev_is_connected_to_SE,
                                                                   std::vector<pev_charge_profile_result>& charge_profile )
{
    this->SE_Load.get_active_charge_profile_forecast_allInfo( setpoint_P3kW, time_step_mins, pev_is_connected_to_SE, charge_profile );
}


void supply_equipment::get_active_charge_profile_forecast_akW( const double setpoint_P3kW,
                                                               const double time_step_mins,
                                                               bool& pev_is_connected_to_SE,
                                                               std::vector<double>& charge_profile)
{
    this->SE_Load.get_active_charge_profile_forecast_akW( setpoint_P3kW, time_step_mins, pev_is_connected_to_SE, charge_profile );
}


void supply_equipment::add_charge_event( const charge_event_data& charge_event )
{
    this->SE_Load.add_charge_event(charge_event);
}


void supply_equipment::set_target_acP3_kW( const double target_acP3_kW )
{
    this->SE_Load.set_target_acP3_kW(target_acP3_kW);
}


void supply_equipment::set_target_acQ3_kVAR( const double target_acQ3_kVAR )
{
    this->SE_Load.set_target_acQ3_kVAR(target_acQ3_kVAR);
}


bool supply_equipment::pev_is_connected_to_SE( const double now_unix_time )
{
    return this->SE_Load.pev_is_connected_to_SE(now_unix_time);
}


void supply_equipment::get_next( const double prev_unix_time,
                                 const double now_unix_time,
                                 const double pu_Vrms,
                                 double& soc,
                                 ac_power_metrics& ac_power)
{
    if( prev_unix_time >= now_unix_time )
    {
        std::cout << "CALDERA ERROR:  prev_unix_time not less than now_unix_time in supply_equipment::get_next()" << std::endl;
        //now_unix_time = prev_unix_time + 0.5;
        throw std::invalid_argument("CALDERA ERROR:  prev_unix_time not less than now_unix_time in supply_equipment::get_next()");
    }
    
    // SE_control.execute_control_strategy must preceed SE_Load.get_next
    // to ensure PEV charging needs are met even if control won't allow this.
    this->SE_control.execute_control_strategy( prev_unix_time, now_unix_time, pu_Vrms, this->SE_Load );

    const bool is_new_CE__update_control_strategies = this->SE_Load.get_next( prev_unix_time, now_unix_time, pu_Vrms, soc, ac_power );
    if( is_new_CE__update_control_strategies )
    {
        this->SE_control.update_parameters_for_CE(this->SE_Load);
    }
}


void supply_equipment::stop_active_CE()
{
    this->SE_Load.stop_active_CE();
}


void supply_equipment::get_external_control_strategy( std::string& return_val )
{
    if( this->SE_Load.pev_is_connected_to_SE__ev_charge_model_not_NULL() )
    {
        return_val = this->SE_control.get_external_control_strategy();
    }
    else
    {
        return_val = "NA";
    }
}


bool supply_equipment::current_CE_is_using_external_control_strategy( const std::string& ext_control )
{
    std::string X = this->SE_control.get_external_control_strategy();
    SE_status Y = this->SE_Load.get_SE_status();
    
    return (ext_control == X) && Y.pev_is_connected_to_SE;
}


bool supply_equipment::current_CE_is_using_control_strategy( const double unix_time_of_interest,
                                                             const L2_control_strategies_enum control_enum_ )
{
    bool control_strategy_is_used = (this->SE_control.get_L2_ES_control_strategy() == control_enum_) || (this->SE_control.get_L2_VS_control_strategy() == control_enum_);
    bool pev_is_connected = this->SE_Load.pev_is_connected_to_SE(unix_time_of_interest);
    
    return control_strategy_is_used && pev_is_connected;
}


void supply_equipment::ES500_get_charging_needs( const double unix_time_now,
                                                 const double unix_time_begining_of_next_agg_step,
                                                 ES500_aggregator_pev_charge_needs& pev_charge_needs )
{
    this->SE_control.ES500_get_charging_needs(unix_time_now, unix_time_begining_of_next_agg_step, this->SE_Load, pev_charge_needs);
}


void supply_equipment::ES500_set_energy_setpoints( const double e3_setpoint_kWh )
{
    this->SE_control.ES500_set_energy_setpoints(e3_setpoint_kWh);
}

    