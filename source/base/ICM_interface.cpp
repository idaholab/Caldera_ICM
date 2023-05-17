
#include "ICM_interface.h"
#include "ac_to_dc_converter.h"		                // ac_to_dc_converter
#include "datatypes_module.h"	                    // ac_power_metrics

#include "SE_EV_factory_charge_profile.h"

#include <iostream>
#include <string>
#include <unordered_set>


interface_to_SE_groups::interface_to_SE_groups(const interface_to_SE_groups_inputs& inputs)
    : inventory{ inputs.inventory }, 
    charge_profile_library{ load_charge_profile_library(inputs) }
{
    EV_EVSE_ramping_map ramping_by_pevType_seType_map;
    for (const pev_charge_ramping_workaround& X : inputs.ramping_by_pevType_seType)
    {
        ramping_by_pevType_seType_map[std::make_pair(X.pev_type, X.SE_type)] = X.pev_charge_ramping_obj;
    }

    //--------------------

    this->EV_model_factory = new factory_EV_charge_model(this->inventory, inputs.ramping_by_pevType_only, ramping_by_pevType_seType_map, false);
   
    this->ac_to_dc_converter_factory = new factory_ac_to_dc_converter(this->inventory);

    this->baseLD_forecaster = get_base_load_forecast{ inputs.data_start_unix_time, inputs.data_timestep_sec, inputs.actual_load_akW, inputs.forecast_load_akW, inputs.adjustment_interval_hrs };

    this->manage_L2_control = manage_L2_control_strategy_parameters{ inputs.L2_parameters };

    //==========================================
    //          Initialize infrastructure
    //==========================================

    factory_supply_equipment_model SE_factory(this->inventory, inputs.CE_queuing_inputs);
    factory_EV_charge_model* EV_model = this->EV_model_factory;
    factory_ac_to_dc_converter* ac_to_dc_converter = this->ac_to_dc_converter_factory;
    pev_charge_profile_library* charge_profile_library_ = &this->charge_profile_library;
    get_base_load_forecast* baseLD_forecaster_ = &this->baseLD_forecaster;
    manage_L2_control_strategy_parameters* manage_L2_control_ = &this->manage_L2_control;

    for (const SE_group_configuration& SE_group_conf : inputs.infrastructure_topology)
    {
        supply_equipment_group Y(SE_group_conf, SE_factory, EV_model, ac_to_dc_converter, charge_profile_library_, baseLD_forecaster_, manage_L2_control_);
        this->SE_group_objs.push_back(Y);
    }

    //---------------------------------

    for (supply_equipment_group& SE_group : this->SE_group_objs)
    {
        SE_group_configuration SE_group_conf = SE_group.get_SE_group_configuration();
        this->SE_group_Id_to_ptr[SE_group_conf.SE_group_id] = &SE_group;

        //------------------------

        std::vector<supply_equipment*> SE_ptrs = SE_group.get_pointers_to_all_SE_objects();

        for (supply_equipment* SE_ptr : SE_ptrs)
        {
            this->SE_ptr_vector.push_back(SE_ptr);

            SE_configuration SE_conf = SE_ptr->get_SE_configuration();
            this->SEid_to_SE_ptr[SE_conf.SE_id] = SE_ptr;

            //-------------------

            if (this->gridNodeId_to_SE_ptrs.count(SE_conf.grid_node_id) == 0)
            {
                std::vector<supply_equipment*> X;
                this->gridNodeId_to_SE_ptrs[SE_conf.grid_node_id] = X;
            }

            this->gridNodeId_to_SE_ptrs[SE_conf.grid_node_id].push_back(SE_ptr);
        }
    }

    //=========================================================================
    //         set_ensure_pev_charge_needs_met_for_ext_control_strategy
    //=========================================================================

    for (supply_equipment* SE_ptr : this->SE_ptr_vector)
        SE_ptr->set_ensure_pev_charge_needs_met_for_ext_control_strategy(inputs.ensure_pev_charge_needs_met);

}

pev_charge_profile_library interface_to_SE_groups::load_charge_profile_library(const interface_to_SE_groups_inputs& inputs)
{
    factory_charge_profile_library X{ inventory };
    return X.get_charge_profile_library(false, inputs.create_charge_profile_library);
}

interface_to_SE_groups::~interface_to_SE_groups()
{
    if(this->EV_model_factory != NULL)
    {
        delete this->EV_model_factory;
        this->EV_model_factory = NULL;
    }
    
    if(this->ac_to_dc_converter_factory != NULL)
    {
        delete this->ac_to_dc_converter_factory;
        this->ac_to_dc_converter_factory = NULL;
    }
}


void interface_to_SE_groups::stop_active_charge_events(std::vector<SE_id_type> SE_ids)
{
    try
    {
        for(SE_id_type x : SE_ids)
            this->SEid_to_SE_ptr[x]->stop_active_CE();
    }
    catch(...)
    {
        // Throw and Error or something ???
        std::cout << "Attempting to stop charge event with SE_id that has no model in Caldera.  See interface_to_SE_groups::stop_active_charge_events." << std::endl;
    }
}


void interface_to_SE_groups::add_charge_events(std::vector<charge_event_data> charge_events)
{
    try
    {
        for(charge_event_data& X : charge_events)
            this->SEid_to_SE_ptr[X.SE_id]->add_charge_event(X);
    }
    catch(...)
    {
        // Throw and Error or something ???
        std::cout << "Attempting to add charge event with SE_id that has no model in Caldera.  See interface_to_SE_groups::add_charge_events." << std::endl;
    }
}


void interface_to_SE_groups::add_charge_events_by_SE_group(std::vector<SE_group_charge_event_data> SE_group_charge_events)
{    
    for(SE_group_charge_event_data& CE_data : SE_group_charge_events)
    {
        if(this->SE_group_Id_to_ptr.count(CE_data.SE_group_id) == 0)
        {
            // Throw and Error or something ???
            std::cout << "Houston we have a problem!  Location: interface_to_SE_groups::add_charge_events_by_SE_group" << std::endl;
        }
        else
        {
            std::vector<charge_event_data>& charge_events = CE_data.charge_events;            
            
            try
            {
                for(charge_event_data& X : charge_events)
                    this->SEid_to_SE_ptr[X.SE_id]->add_charge_event(X);
            }
            catch(...)
            {
                // Throw and Error or something ???
                std::cout << "Attempting to add charge event with SE_id that has no model in Caldera.  See interface_to_SE_groups::add_charge_events." << std::endl;
            }
        }
    }
}


void interface_to_SE_groups::set_PQ_setpoints(double now_unix_time, std::vector<SE_setpoint> SE_setpoints)
{
    control_strategy_enums Z;
    supply_equipment* SE_ptr;
    std::string NA_string = "NA";
    
    for(SE_setpoint& X : SE_setpoints)
    {
        if(this->SEid_to_SE_ptr.count(X.SE_id) == 0)
        {  
            std::cout << "External Control Strategy Error: External control strategy trying to set pev charging on a supply equipment that does not exist.  interface_to_SE_groups::set_PQ_setpoints()" << std::endl;
            // Throw an Error
        }
        else
        {
            SE_ptr = this->SEid_to_SE_ptr[X.SE_id];
            Z = SE_ptr->get_control_strategy_enums();
            
            if(SE_ptr->pev_is_connected_to_SE(now_unix_time) == false)
                continue;
            
            if(Z.ES_control_strategy == NA && Z.VS_control_strategy == NA && Z.ext_control_strategy != NA_string)
            {
                SE_ptr->set_target_acP3_kW(X.PkW);
                
                if(Z.inverter_model_supports_Qsetpoint)
                    SE_ptr->set_target_acQ3_kVAR(X.QkVAR);
            }
            else
            {
                bool pev_is_connected_to_SE;
                active_CE active_CE_val;
                SE_ptr->get_active_CE(pev_is_connected_to_SE, active_CE_val);
                
                std::cout << "Control Strategy Error:  Attempting to set the pev charging power when not allowed for charge_event_id = " << active_CE_val.charge_event_id << ".  Only allowed to set PEV charging power when in CE_*.csv file: (ES_control_strategy==NA) and (VS_control_strategy==NA) and (Ext_strategy != NA)." << std::endl;
                // Throw an Error
            }
        }
    }
}


std::vector<completed_CE> interface_to_SE_groups::get_completed_CE()
{
    std::vector<completed_CE> return_val;
    std::vector<completed_CE> tmp;
    
    for(supply_equipment* SE_ptr : this->SE_ptr_vector)
    {
        tmp = SE_ptr->get_completed_CE();
        
        if(0 < tmp.size())
            return_val.insert(return_val.end(), tmp.begin(), tmp.end());
    }
    
    return return_val;
}


std::vector<CE_FICE> interface_to_SE_groups::get_FICE_by_extCS(std::string external_control_strategy, FICE_inputs inputs)
{
    std::vector<CE_FICE> return_val;
    
    bool pev_is_connected_to_SE;
    CE_FICE FICE_val;
    
    for(supply_equipment* SE_ptr : this->SE_ptr_vector)
    {
        if(SE_ptr->current_CE_is_using_external_control_strategy(external_control_strategy))
        {
            SE_ptr->get_CE_FICE(inputs, pev_is_connected_to_SE, FICE_val);
            
            if(pev_is_connected_to_SE)
                return_val.push_back(FICE_val);
        }
    }
    
    return return_val;
}

   
std::vector<CE_FICE_in_SE_group> interface_to_SE_groups::get_FICE_by_SE_groups(std::vector<int> SE_group_ids, FICE_inputs inputs)
{
    std::vector<CE_FICE_in_SE_group> return_val;
    std::vector<CE_FICE> X;
    CE_FICE_in_SE_group Y;
        
    for(int group_id : SE_group_ids)
    {
        if(this->SE_group_Id_to_ptr.count(group_id) == 0)
        {
            std::cout << "CALDERA ERROR:  interface_to_SE_groups::get_FICE_by_SE_groups() -> No findy" << std::endl;
            // Throw an Error
        }
        else
        {
            this->SE_group_Id_to_ptr[group_id]->get_CE_FICE(inputs, X);
            Y.SE_group_id = group_id;
            Y.SE_FICE_vals = X;
            return_val.push_back(Y);
        }
    }
    
    return return_val;
}


std::vector<CE_FICE> interface_to_SE_groups::get_FICE_by_SEids(std::vector<int> SEids, FICE_inputs inputs)
{
    std::vector<CE_FICE> return_val;
    
    bool pev_is_connected_to_SE;
    CE_FICE FICE_val;
    
    for(SE_id_type SE_id : SEids)
    {
        if(this->SEid_to_SE_ptr.count(SE_id) == 0)
        {
            std::cout << "CALDERA ERROR:  interface_to_SE_groups::get_FICE_by_SEids() -> No findy" << std::endl;
            // Throw an Error
        }
        else
        {            
            this->SEid_to_SE_ptr[SE_id]->get_CE_FICE(inputs, pev_is_connected_to_SE, FICE_val);
            
            if(pev_is_connected_to_SE)
                return_val.push_back(FICE_val);
        }
    }
    
    return return_val;
}


std::vector<active_CE> interface_to_SE_groups::get_all_active_CEs()
{
    std::vector<active_CE> return_val;
    
    bool pev_is_connected_to_SE;
    active_CE active_CE_val;
    
    for(supply_equipment* SE_ptr : this->SE_ptr_vector)
    {
        SE_ptr->get_active_CE(pev_is_connected_to_SE, active_CE_val);
        
        if(pev_is_connected_to_SE)
            return_val.push_back(active_CE_val);
    }
    
    return return_val;
}


std::map<std::string, std::vector<active_CE> > interface_to_SE_groups::get_active_CEs_by_extCS(std::vector<std::string> external_control_strategies)
{
    std::unordered_set<std::string> ECS_set(external_control_strategies.begin(), external_control_strategies.end());
    
    std::map<std::string, std::vector<active_CE>> return_val;
    
    for(const std::string& extCS : ECS_set)
    {    
        std::vector<active_CE> z;
        return_val[extCS] = z;
    }
    
    //-----------------------------
    
    bool pev_is_connected_to_SE;
    active_CE active_CE_val;
    std::string ECS_str;
    
    for(supply_equipment* SE_ptr : this->SE_ptr_vector)
    {
        SE_ptr->get_external_control_strategy(ECS_str);
        
        if(ECS_str == "NA")
            continue;
        
        if(ECS_set.count(ECS_str) > 0)
        {
            SE_ptr->get_active_CE(pev_is_connected_to_SE, active_CE_val);
            
            if(pev_is_connected_to_SE)                    
                return_val[ECS_str].push_back(active_CE_val);            
        }
    }
    
    return return_val;
}


std::map<int, std::vector<active_CE> > interface_to_SE_groups::get_active_CEs_by_SE_groups(std::vector<int> SE_group_ids)
{
    std::map<int, std::vector<active_CE> > return_val;
    std::vector<active_CE> X;
    
    for(int group_id : SE_group_ids)
    {
        if(this->SE_group_Id_to_ptr.count(group_id) == 0)
        {
            std::cout << "CALDERA ERROR:  interface_to_SE_groups::get_active_CEs_by_SE_groups() -> No findy" << std::endl;
            // Throw an Error
        }
        else
        {
            this->SE_group_Id_to_ptr[group_id]->get_active_CEs(X);
            return_val[group_id] = X;
        }
    }
    
    return return_val;
}


std::vector<active_CE> interface_to_SE_groups::get_active_CEs_by_SEids(std::vector<SE_id_type> SEids)
{
    std::vector<active_CE> return_val;
    
    bool pev_is_connected_to_SE;
    active_CE active_CE_val;
    
    for(SE_id_type SE_id : SEids)
    {
        if(this->SEid_to_SE_ptr.count(SE_id) == 0)
        {
            std::cout << "CALDERA ERROR:  interface_to_SE_groups::get_active_CEs_by_SEids() -> No findy" << std::endl;
            // Throw an Error
        }
        else
        {            
            this->SEid_to_SE_ptr[SE_id]->get_active_CE(pev_is_connected_to_SE, active_CE_val);
            
            if(pev_is_connected_to_SE)
                return_val.push_back(active_CE_val);
        }
    }
    
    return return_val;
}


std::vector<double> interface_to_SE_groups::get_SE_charge_profile_forecast_akW(SE_id_type SE_id, double setpoint_P3kW, double time_step_mins)
{
    std::vector<double> charge_profile;
    
    if(this->SEid_to_SE_ptr.count(SE_id) == 0)
    {
        std::cout << "CALDERA ERROR:  interface_to_SE_groups::get_SE_charge_profile_forecast_akW() -> No findy" << std::endl;
        // Throw an Error
    }
    else
    {
        bool pev_is_connected_to_SE;
        this->SEid_to_SE_ptr[SE_id]->get_active_charge_profile_forecast_akW(setpoint_P3kW, time_step_mins, pev_is_connected_to_SE, charge_profile);
    }
    
    return charge_profile;
}

 
std::vector<double> interface_to_SE_groups::get_SE_group_charge_profile_forecast_akW(int SE_group, double setpoint_P3kW, double time_step_mins)
{
    std::vector<double> charge_profile;
    
    if(this->SE_group_Id_to_ptr.count(SE_group) == 0)
    {
        // Throw and Error or something ???
        std::cout << "Houston we have a problem!  Location: interface_to_SE_groups::get_SE_group_charge_profile_forecast_akW" << std::endl;
    }
    else
        this->SE_group_Id_to_ptr[SE_group]->get_SE_group_charge_profile_forecast_akW(setpoint_P3kW, time_step_mins, charge_profile);

    return charge_profile;
}


std::map<grid_node_id_type, std::pair<double, double>> interface_to_SE_groups::get_charging_power(double prev_unix_time, double now_unix_time, std::map<grid_node_id_type, double> pu_Vrms)
{
    std::map<grid_node_id_type, std::pair<double, double> > return_val;
    
    //---------------------------
    
    std::map<grid_node_id_type, std::vector<supply_equipment*> >::iterator it;
    std::pair<double, double> tmp_pwr;
    ac_power_metrics ac_power_tmp;
    double P3_kW, Q3_kVAR, soc;
    
    for(std::pair<grid_node_id_type, double> pu_Vrms_pair : pu_Vrms)
    {
        it = this->gridNodeId_to_SE_ptrs.find(pu_Vrms_pair.first);
        
        if(it == this->gridNodeId_to_SE_ptrs.end())
        {
            // There is no SE on this grid node
        }
        else
        {
            P3_kW = 0;
            Q3_kVAR = 0;
        
            for(supply_equipment* SE_ptr : it->second)
            {
                SE_ptr->get_next(prev_unix_time, now_unix_time, pu_Vrms_pair.second, soc, ac_power_tmp);
                
                P3_kW += ac_power_tmp.P3_kW;
                Q3_kVAR += ac_power_tmp.Q3_kVAR;
            }
            
            tmp_pwr.first = P3_kW;
            tmp_pwr.second = Q3_kVAR;
            
            return_val[pu_Vrms_pair.first] = tmp_pwr;
        }
    }
    
    return return_val;
}


SE_power interface_to_SE_groups::get_SE_power(SE_id_type SE_id, double prev_unix_time, double now_unix_time, double pu_Vrms)
{
    SE_power return_val;
    
    if(this->SEid_to_SE_ptr.count(SE_id) == 0)
    {
        std::cout << "CALDERA ERROR:  interface_to_SE_groups::get_SE_charge_profile_forecast_akW() -> No SE_id = " << SE_id << std::endl;
        // Throw an Error
        
        return_val.time_step_duration_hrs = 0;
        return_val.P1_kW = 0;
        return_val.P2_kW = 0;
        return_val.P3_kW = 0;
        return_val.Q3_kVAR = 0;
        return_val.soc = 0;
        return_val.SE_status_val = no_ev_plugged_in;
    }
    else
    {
        //bool pev_is_connected_to_SE;
        SE_charging_status SE_status_val;
        double soc;
        //CE_status charge_status;
        ac_power_metrics ac_power;
        
        //this->SEid_to_SE_ptr[SE_id]->get_current_CE_status(pev_is_connected_to_SE, SE_status_val, charge_status);
        this->SEid_to_SE_ptr[SE_id]->get_next(prev_unix_time, now_unix_time, pu_Vrms, soc, ac_power);

        return_val.time_step_duration_hrs = ac_power.time_step_duration_hrs;
        return_val.P1_kW = ac_power.P1_kW;
        return_val.P2_kW = ac_power.P2_kW;
        return_val.P3_kW = ac_power.P3_kW;
        return_val.Q3_kVAR = ac_power.Q3_kVAR;
        return_val.soc = soc;
        return_val.SE_status_val = SE_status_val;
    }
    
    return return_val;
}


ES500_aggregator_charging_needs interface_to_SE_groups::ES500_get_charging_needs(double unix_time_now, double unix_time_begining_of_next_agg_step)
{
    ES500_aggregator_charging_needs return_val;
    return_val.next_aggregator_timestep_start_time = unix_time_begining_of_next_agg_step;
    
    ES500_aggregator_pev_charge_needs obj;
    
    for(supply_equipment* SE_ptr : this->SE_ptr_vector)
    {
        if(SE_ptr->current_CE_is_using_control_strategy(unix_time_begining_of_next_agg_step, ES500))
        {
            SE_ptr->ES500_get_charging_needs(unix_time_now, unix_time_begining_of_next_agg_step, obj);

            if(obj.e3_charge_remain_kWh > 0.00001)
                return_val.pev_charge_needs.push_back(obj);
        }
    }
    
    return return_val;
}


void interface_to_SE_groups::ES500_set_energy_setpoints(ES500_aggregator_e_step_setpoints pev_energy_setpoints)
{
    std::vector<SE_id_type>& SE_id = pev_energy_setpoints.SE_id;
    std::vector<double>& e3_step_kWh = pev_energy_setpoints.e3_step_kWh;
    
    for(int i=0; i<SE_id.size(); i++)
    {
        if(this->SEid_to_SE_ptr.count(SE_id[i]) == 0)
        {
            std::cout << "CALDERA ERROR:  interface_to_SE_groups::ES500_set_energy_setpoints() -> No findy" << std::endl;
            
            // Throw an Error
        }
        else
        {
            this->SEid_to_SE_ptr[SE_id[i]]->ES500_set_energy_setpoints(e3_step_kWh[i]);
        }
    }
}

