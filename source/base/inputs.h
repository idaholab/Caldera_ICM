#ifndef INPUTS_H
#define INPUTS_H

#include "charging_transitions_factory.h"
#include "puVrms_vs_P2_factory.h"
#include "SOC_vs_P2_factory.h"
#include "P2_vs_battery_efficiency_factory.h"

struct vehicle_charge_model_inputs
{
    const charge_event_data& CE;
    const EV_type& EV;
    const EVSE_type& EVSE;
    const double SE_P2_limit_kW;
    const double battery_size_kWh;
    const charging_transitions_factory& CT_factory;
    const puVrms_vs_P2_factory& VP_factory;
    const SOC_vs_P2_factory& SOCP_factory;
    const P2_vs_battery_efficiency_factory& PE_factory;

    vehicle_charge_model_inputs(const charge_event_data& CE, 
                                const EV_type& EV, 
                                const EVSE_type& EVSE,
                                const double& SE_P2_limit_kW, 
                                const double& battery_size_kWh, 
                                const charging_transitions_factory& CT_factory,
                                const puVrms_vs_P2_factory& VP_factory, 
                                const SOC_vs_P2_factory& SOCP_factory, 
                                const P2_vs_battery_efficiency_factory& PE_factory)
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