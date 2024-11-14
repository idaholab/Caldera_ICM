
#include "supply_equipment_group.h"      
#include "datatypes_module.h"                   // ac_power_metrics
#include "factory_EV_charge_model.h"            // factory_EV_charge_model
#include "factory_supply_equipment_model.h"     // factory_supply_equipment_model


supply_equipment_group::supply_equipment_group( const SE_group_configuration& SE_group_topology_, 
                                                factory_supply_equipment_model& SE_factory, 
                                                const factory_EV_charge_model& PEV_charge_factory,
                                                const factory_ac_to_dc_converter& ac_to_dc_converter_factory, 
                                                const pev_charge_profile_library& charge_profile_library,
                                                const get_base_load_forecast& baseLD_forecaster,
                                                manage_L2_control_strategy_parameters* manage_L2_control )
{
    this->SE_group_topology = SE_group_topology_;    
    this->SE_objs.clear();
    
    for(const SE_configuration& SE_config : SE_group_topology_.SEs)
    {
        bool building_charge_profile_library = false;
        supply_equipment SE_obj = SE_factory.get_supply_equipment_model(
            building_charge_profile_library, 
            SE_config, 
            baseLD_forecaster, 
            manage_L2_control, 
            PEV_charge_factory,
            ac_to_dc_converter_factory,
            charge_profile_library
        );
        this->SE_objs.push_back(SE_obj);
    }
}


std::vector<supply_equipment*> supply_equipment_group::get_pointers_to_all_SE_objects()
{
    std::vector<supply_equipment*> return_val;
    
    for(supply_equipment& SE_obj : this->SE_objs)
    {
        return_val.push_back(&SE_obj);
    }
    
    return return_val;
}


SE_group_configuration supply_equipment_group::get_SE_group_configuration()
{
    return this->SE_group_topology;
}


void supply_equipment_group::get_active_CEs(std::vector<active_CE>& return_val)
{
    return_val.clear();
    
    bool pev_is_connected_to_SE;
    active_CE active_CE_val;
    
    for(supply_equipment& SE_obj : this->SE_objs)
    {
        SE_obj.get_active_CE(pev_is_connected_to_SE, active_CE_val);

        if(pev_is_connected_to_SE)
            return_val.push_back(active_CE_val);
    }
}


void supply_equipment_group::get_CE_FICE(FICE_inputs inputs, std::vector<CE_FICE>& return_val)
{
    return_val.clear();
    
    bool pev_is_connected_to_SE;
    CE_FICE X;
    
    for(supply_equipment& SE_obj : this->SE_objs)
    {
        SE_obj.get_CE_FICE(inputs, pev_is_connected_to_SE, X);
        
        if(pev_is_connected_to_SE)
            return_val.push_back(X);
    }
}


void supply_equipment_group::get_SE_group_charge_profile_forecast_akW(double setpoint_P3kW, double time_step_mins, std::vector<double>& charge_profile)
{
    std::vector<std::vector<double> > tmp_profiles;
    bool pev_is_connected_to_SE;
    
    for(supply_equipment& SE_obj : this->SE_objs)
    {
        std::vector<double> tmp_charge_profile;
        SE_obj.get_active_charge_profile_forecast_akW(setpoint_P3kW, time_step_mins, pev_is_connected_to_SE, tmp_charge_profile);
        
        if(pev_is_connected_to_SE)
            tmp_profiles.push_back(tmp_charge_profile);
    }
    
    //---------------------
    
    int size = 0;
    
    for(std::vector<double>& x : tmp_profiles)
    {
        if(x.size() > size)
            size = x.size();
    }
    
    //---------------------
    
    charge_profile.resize(size, 0);
    
    for(std::vector<double>& x : tmp_profiles)
    {
        for(int i=0; i<x.size(); i++)
            charge_profile[i] += x[i];
    }
}

