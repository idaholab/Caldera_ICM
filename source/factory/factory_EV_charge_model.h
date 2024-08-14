#ifndef FACTORY_EV_CHARGE_MODEL_H
#define FACTORY_EV_CHARGE_MODEL_H

#include <unordered_map>
#include <map>

#include "EV_EVSE_inventory.h"

#include "factory_charging_transitions.h"
#include "factory_puVrms_vs_P2.h"
#include "factory_SOC_vs_P2.h"
#include "factory_P2_vs_battery_efficiency.h"

#include "vehicle_charge_model.h"

class factory_EV_charge_model
{
private:

    const EV_EVSE_inventory& inventory;

    const factory_charging_transitions charging_transitions_obj;
    const factory_puVrms_vs_P2 puVrms_vs_P2_obj;
    const factory_SOC_vs_P2 SOC_vs_P2_obj;
    const factory_P2_vs_battery_efficiency P2_vs_battery_eff_obj;

    const bool model_stochastic_battery_degregation;

public:
    factory_EV_charge_model( const EV_EVSE_inventory& inventory,
                             const EV_ramping_map& EV_ramping,
                             const EV_EVSE_ramping_map& EV_EVSE_ramping,
                             const bool model_stochastic_battery_degregation,
                             const double c_rate_scale_factor = 1.0 );
    
    vehicle_charge_model* alloc_get_EV_charge_model(const charge_event_data& event, 
                                                    const EVSE_type& EVSE, 
                                                    const double SE_P2_limit_kW) const;

    void write_charge_profile(const std::string& output_path) const;
};

#endif