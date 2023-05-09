#include "EV_charge_model_factory.h"

#include "helper.h"                             //poly_segment

#include <vector>


//#############################################################################
//                         EV Charge Model Factory
//#############################################################################

EV_charge_model_factory::EV_charge_model_factory(const EV_EVSE_inventory& inventory, 
    const EV_ramping_map& custom_EV_ramping, const EV_EVSE_ramping_map& custom_EV_EVSE_ramping, 
    const bool& model_stochastic_battery_degregation) 
    : inventory(inventory), 
    charging_transitions_obj(charging_transitions_factory(inventory, custom_EV_ramping, custom_EV_EVSE_ramping)), 
    puVrms_vs_P2_obj(puVrms_vs_P2_factory(inventory)),
    SOC_vs_P2_obj(SOC_vs_P2_factory(inventory)), P2_vs_battery_eff_obj(P2_vs_battery_efficiency_factory(inventory)), 
    model_stochastic_battery_degregation(model_stochastic_battery_degregation) {}


vehicle_charge_model* EV_charge_model_factory::alloc_get_EV_charge_model(const charge_event_data& event, const EVSE_type& EVSE, const double& SE_P2_limit_kW)
{
    const EV_type& EV = event.EV;
    double battery_size_kWh = this->inventory.get_EV_inventory().at(EV).get_usable_battery_size_kWh();
    double battery_size_with_degredation_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_with_stochastic_degradation_kWh();
    battery_chemistry chemistry = this->inventory.get_EV_inventory().at(EV).get_chemistry();

    double soc_of_full_battery = 99.8;
    double recalc_exponent_threashold = 0.00000001;  // Should be very small 
    double max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments = 0.5;

    bool will_never_discharge = true;
    bool are_battery_losses = true;

    //---------------------------------

    //precomputed
    const integrate_X_through_time& get_next_P2 = this->charging_transitions_obj.get_charging_transitions(EV, EVSE);

    //---------------------------------

    //dynamic
    const poly_function_of_x puVrms_vs_P2_curve = this->puVrms_vs_P2_obj.get_puVrms_vs_P2(EVSE, SE_P2_limit_kW);

    //precomputed
    const SOC_vs_P2& SOC_vs_P2_curve = this->SOC_vs_P2_obj.get_SOC_vs_P2_curves(EV, EVSE);

    //---------------------------------

    double zero_slope_threashold_bat_eff_vs_P2;

    //precomputed
    const P2_vs_battery_efficiency& P2_vs_battery_eff_charging = this->P2_vs_battery_eff_obj.get_P2_vs_battery_eff(EV, charging);
    //precomputed
    const P2_vs_battery_efficiency& P2_vs_battery_eff_discharging = this->P2_vs_battery_eff_obj.get_P2_vs_battery_eff(EV, discharging);

    zero_slope_threashold_bat_eff_vs_P2 = (P2_vs_battery_eff_charging.zero_slope_threashold < P2_vs_battery_eff_discharging.zero_slope_threashold) ? P2_vs_battery_eff_charging.zero_slope_threashold : P2_vs_battery_eff_discharging.zero_slope_threashold;

    //---------------------------------

    double final_bat_size_kWh;

    if (this->model_stochastic_battery_degregation)
        final_bat_size_kWh = battery_size_with_degredation_kWh;
    else
        final_bat_size_kWh = battery_size_kWh;

    //---------------------------------

    algorithm_P2_vs_soc_inputs charging_inputs{ final_bat_size_kWh, recalc_exponent_threashold, SOC_vs_P2_curve.zero_slope_threashold,
        P2_vs_battery_eff_charging.curve };
    algorithm_P2_vs_soc_inputs discharging_inputs{ final_bat_size_kWh, recalc_exponent_threashold, SOC_vs_P2_curve.zero_slope_threashold,
        P2_vs_battery_eff_discharging.curve };

    calculate_E1_energy_limit_inputs E1_limits_charging_inputs{ charging, are_battery_losses, charging_inputs, puVrms_vs_P2_curve,
        max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, SOC_vs_P2_curve.curve };
    calculate_E1_energy_limit_inputs E1_limits_discharging_inputs{ discharging, are_battery_losses, discharging_inputs, puVrms_vs_P2_curve,
        max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, SOC_vs_P2_curve.curve };

    battery_inputs bat_input{ E1_limits_charging_inputs, E1_limits_discharging_inputs, get_next_P2, final_bat_size_kWh, event.arrival_SOC,
        zero_slope_threashold_bat_eff_vs_P2, will_never_discharge, P2_vs_battery_eff_charging.curve, P2_vs_battery_eff_discharging.curve };

    //---------------------------------

    return new vehicle_charge_model(event, bat_input, soc_of_full_battery);
}


void EV_charge_model_factory::write_charge_profile(const std::string& output_path) const
{
    this->SOC_vs_P2_obj.write_charge_profile(output_path);
}