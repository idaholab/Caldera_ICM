#ifndef EV_CHARGE_MODEL_FACTORY_H
#define EV_CHARGE_MODEL_FACTORY_H

#include <unordered_map>
#include <map>

#include "EV_EVSE_inventory.h"

#include "charging_transitions_factory.h"
#include "puVrms_vs_P2_factory.h"
#include "SOC_vs_P2_factory.h"
#include "P2_vs_battery_efficiency_factory.h"

#include "vehicle_charge_model.h"

class EV_charge_model_factory
{
private:

    const EV_EVSE_inventory& inventory;

    const charging_transitions_factory charging_transitions_obj;
    const puVrms_vs_P2_factory puVrms_vs_P2_obj;
    const SOC_vs_P2_factory SOC_vs_P2_obj;
    const P2_vs_battery_efficiency_factory P2_vs_battery_eff_obj;

    const bool model_stochastic_battery_degregation;

public:
	EV_charge_model_factory(const EV_EVSE_inventory& inventory,
                            const EV_ramping_map& EV_ramping,
                            const EV_EVSE_ramping_map& EV_EVSE_ramping,
                            const bool& model_stochastic_battery_degregation);
    
    vehicle_charge_model* alloc_get_EV_charge_model(const charge_event_data& event, 
                                                    const EVSE_type& EVSE, 
                                                    const double& SE_P2_limit_kW) const;

    void write_charge_profile(const std::string& output_path) const;
};

#endif