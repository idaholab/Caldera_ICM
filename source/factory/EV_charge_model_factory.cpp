#include "EV_charge_model_factory.h"

#include "helper.h"                             //poly_segment

#include <vector>


//########################################################################
//                         EV Charge Model Factory
//########################################################################


EV_charge_model_factory::EV_charge_model_factory(const EV_EVSE_inventory& inventory,
												 const EV_ramping_map& custom_EV_ramping,
												 const EV_EVSE_ramping_map& custom_EV_EVSE_ramping,
												 const bool& model_stochastic_battery_degregation)
	: inventory{ inventory },
	charging_transitions_obj{ charging_transitions_factory{inventory,custom_EV_ramping, custom_EV_EVSE_ramping} },
	puVrms_vs_P2_obj{ puVrms_vs_P2_factory{inventory} },
	SOC_vs_P2_obj{ SOC_vs_P2_factory{inventory} },
	P2_vs_battery_eff_obj{ P2_vs_battery_efficiency_factory{inventory} },
	model_stochastic_battery_degregation{ model_stochastic_battery_degregation }
{
}


vehicle_charge_model* EV_charge_model_factory::alloc_get_EV_charge_model(const charge_event_data& event,
																		 const EVSE_type& EVSE,
																		 const double& SE_P2_limit_kW) const
{
	const EV_type& EV = event.EV;

	double final_bat_size_kWh;
	if (this->model_stochastic_battery_degregation)
	{
		final_bat_size_kWh = this->inventory.get_EV_inventory().at(EV).get_battery_size_with_stochastic_degradation_kWh();
	}
	else
	{
		final_bat_size_kWh = this->inventory.get_EV_inventory().at(EV).get_usable_battery_size_kWh();
	}
	vehicle_charge_model_inputs inputs{ event, EV, EVSE, SE_P2_limit_kW, final_bat_size_kWh,
		this->charging_transitions_obj, this->puVrms_vs_P2_obj, this->SOC_vs_P2_obj, this->P2_vs_battery_eff_obj };

	return new vehicle_charge_model(inputs);
}


void EV_charge_model_factory::write_charge_profile(const std::string& output_path) const
{
	this->SOC_vs_P2_obj.write_charge_profile(output_path);
}