#ifndef INPUTS_H
#define INPUTS_H

#include "factory_charging_transitions.h"
#include "factory_puVrms_vs_P2.h"
#include "factory_SOC_vs_P2.h"
#include "factory_P2_vs_battery_efficiency.h"

struct vehicle_charge_model_inputs
{
    const charge_event_data& CE;
    const EV_type& EV;
    const EVSE_type& EVSE;
    const double SE_P2_limit_kW;
    const double battery_size_kWh;
    const factory_charging_transitions& CT_factory;
    const factory_puVrms_vs_P2& VP_factory;
    const factory_SOC_vs_P2& SOCP_factory;
    const factory_P2_vs_battery_efficiency& PE_factory;

    vehicle_charge_model_inputs(const charge_event_data& CE, 
                                const EV_type& EV, 
                                const EVSE_type& EVSE,
                                const double& SE_P2_limit_kW, 
                                const double& battery_size_kWh, 
                                const factory_charging_transitions& CT_factory,
                                const factory_puVrms_vs_P2& VP_factory,
                                const factory_SOC_vs_P2& SOCP_factory,
                                const factory_P2_vs_battery_efficiency& PE_factory)
        : CE{ CE }, 
        EV{ EV }, 
        EVSE{ EVSE }, 
        SE_P2_limit_kW{ SE_P2_limit_kW }, 
        battery_size_kWh{ battery_size_kWh },
        CT_factory{ CT_factory }, 
        VP_factory{ VP_factory }, 
        SOCP_factory{ SOCP_factory }, 
        PE_factory{ PE_factory }
    {
    }
};

#endif