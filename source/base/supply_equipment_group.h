
#ifndef inl_supply_equipment_SE_group_H
#define inl_supply_equipment_SE_group_H


#include "datatypes_global.h"                       // SE_group_configuration, SE_group_charge_event_data, grid_power
#include "supply_equipment.h"                       // supply_equipment
#include "helper.h"                                 // LPF_kernel

#include <vector>


class factory_EV_charge_model;
class factory_supply_equipment_model;

class supply_equipment_group
{
private:
    SE_group_configuration SE_group_topology;
    std::vector<supply_equipment> SE_objs;

public:
    supply_equipment_group(const SE_group_configuration& SE_group_topology_, 
                           factory_supply_equipment_model& SE_factory, 
                           factory_EV_charge_model* PEV_charge_factory, 
                           factory_ac_to_dc_converter* ac_to_dc_converter_factory, 
                           pev_charge_profile_library* charge_profile_library,
                           const get_base_load_forecast& baseLD_forecaster, 
                           manage_L2_control_strategy_parameters* manage_L2_control);
    std::vector<supply_equipment*> get_pointers_to_all_SE_objects();
    SE_group_configuration get_SE_group_configuration();
    void get_active_CEs(std::vector<active_CE>& return_val);
    void get_CE_FICE(FICE_inputs inputs, std::vector<CE_FICE>& return_val);

    void get_SE_group_charge_profile_forecast_akW(double setpoint_P3kW, double time_step_mins, std::vector<double>& charge_profile);
};

#endif


