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



struct interface_to_SE_groups_inputs
{
    const EV_EVSE_inventory& inventory;

    // factory_inputs
    bool create_charge_profile_library;
    EV_ramping_map ramping_by_pevType_only;
    std::vector<pev_charge_ramping_workaround> ramping_by_pevType_seType;

    // infrastructure_inputs
    charge_event_queuing_inputs CE_queuing_inputs;
    std::vector<SE_group_configuration> infrastructure_topology;

    // baseLD_forecaster_inputs
    double data_start_unix_time;
    int data_timestep_sec;
    std::vector<double>& actual_load_akW;
    std::vector<double>& forecast_load_akW;
    double adjustment_interval_hrs;

    // control_strategy_inputs
    L2_control_strategy_parameters L2_parameters;
    bool ensure_pev_charge_needs_met;

    interface_to_SE_groups_inputs(const EV_EVSE_inventory& inventory,
                                  bool create_charge_profile_library,
                                  EV_ramping_map ramping_by_pevType_only,
                                  std::vector<pev_charge_ramping_workaround> ramping_by_pevType_seType,
                                  charge_event_queuing_inputs CE_queuing_inputs,
                                  std::vector<SE_group_configuration> infrastructure_topology,
                                  double data_start_unix_time,
                                  int data_timestep_sec,
                                  std::vector<double>& actual_load_akW,
                                  std::vector<double>& forecast_load_akW,
                                  double adjustment_interval_hrs,
                                  L2_control_strategy_parameters L2_parameters,
                                  bool ensure_pev_charge_needs_met)
        : inventory{ inventory },
        create_charge_profile_library{ create_charge_profile_library },
        ramping_by_pevType_only{ ramping_by_pevType_only },
        ramping_by_pevType_seType{ ramping_by_pevType_seType },
        CE_queuing_inputs{ CE_queuing_inputs },
        infrastructure_topology{ infrastructure_topology },
        data_start_unix_time{ data_start_unix_time },
        data_timestep_sec{ data_timestep_sec },
        actual_load_akW{ actual_load_akW },
        forecast_load_akW{ forecast_load_akW },
        adjustment_interval_hrs{ adjustment_interval_hrs },
        L2_parameters{ L2_parameters },
        ensure_pev_charge_needs_met{ ensure_pev_charge_needs_met }
    {
    }
};

#endif