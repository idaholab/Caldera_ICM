
#ifndef inl_ICM_interface_H
#define inl_ICM_interface_H

#include "datatypes_global.h"                       // grid_node_id_type, SupplyEquipmentId, station_configuration, station_charge_event_data, station_status
#include "supply_equipment_group.h"                 // supply_equipment_group
#include "supply_equipment.h"                       // supply_equipment
#include "supply_equipment_control.h"               // manage_L2_control_strategy_parameters
#include "charge_profile_library.h"                 // pev_charge_profile_library
#include "helper.h"                                 // get_base_load_forecast
#include "inputs.h"

#include <vector>
#include <map>
#include <tuple>

#include "factory_EV_charge_model.h"
#include "factory_charging_transitions.h"
#include "factory_ac_to_dc_converter.h"
#include "factory_supply_equipment_model.h"

#include "load_EV_EVSE_inventory.h"

class interface_to_SE_groups
{
private:

    const load_EV_EVSE_inventory loader;
    const EV_EVSE_inventory& inventory;

    std::vector<supply_equipment_group> SE_group_objs;
    std::map<int, supply_equipment_group*> SE_group_Id_to_ptr;
    std::map<SupplyEquipmentId, supply_equipment*> SEid_to_SE_ptr;
    std::vector<supply_equipment*> SE_ptr_vector;
    std::map<grid_node_id_type, std::vector<supply_equipment*> > gridNodeId_to_SE_ptrs;
    
    // References to the following should be in every supply_equipment_load object.
    const factory_EV_charge_model EV_model_factory;
    const factory_ac_to_dc_converter ac_to_dc_converter_factory;
    const pev_charge_profile_library charge_profile_library;
    const get_base_load_forecast baseLD_forecaster;

    // manage_L2_control_strategy_parameters consists of random number generator
    // that keeps track of internal state. 
    // This makes it unparallelizable. The memory needs are also somewhat big.
    // So having individual copy of this in every supply_equiment_control is
    // not a great idea. 1 solution could be using omp_critical block.
    manage_L2_control_strategy_parameters manage_L2_control;
    
    const factory_EV_charge_model load_factory_EV_charge_model(const interface_to_SE_groups_inputs& inputs);

public:
    interface_to_SE_groups( const std::string& input_path,
                            const interface_to_SE_groups_inputs& inputs );

    pev_charge_profile_library load_charge_profile_library(const interface_to_SE_groups_inputs& inputs);
    
    void stop_active_charge_events(std::vector<SupplyEquipmentId> SE_ids);
    void add_charge_events( const std::vector<charge_event_data>& charge_events );
    void add_charge_events_by_SE_group( const std::vector<SE_group_charge_event_data>& SE_group_charge_events );
    void set_PQ_setpoints(double now_unix_time, std::vector<SE_setpoint> SE_setpoints);
    std::vector<completed_CE> get_completed_CE();

    std::vector<CE_FICE> get_FICE_by_extCS(std::string external_control_strategy, FICE_inputs inputs);
    std::vector<CE_FICE_in_SE_group> get_FICE_by_SE_groups(std::vector<int> SE_group_ids, FICE_inputs inputs);
    std::vector<CE_FICE> get_FICE_by_SEids(std::vector<int> SEids, FICE_inputs inputs);

    std::vector<active_CE> get_all_active_CEs();
    std::map<std::string, std::vector<active_CE> > get_active_CEs_by_extCS(std::vector<std::string> external_control_strategies);
    std::map<int, std::vector<active_CE> > get_active_CEs_by_SE_groups(std::vector<int> SE_group_ids);
    std::vector<active_CE> get_active_CEs_by_SEids(std::vector<SupplyEquipmentId> SEids);

    std::vector<double> get_SE_charge_profile_forecast_akW(SupplyEquipmentId SE_id, double setpoint_P3kW, double time_step_mins);
    std::vector<double> get_SE_group_charge_profile_forecast_akW(int SE_group, double setpoint_P3kW, double time_step_mins);
    
    std::map<grid_node_id_type, std::pair<double, double> > get_charging_power(double prev_unix_time, double now_unix_time, std::map<grid_node_id_type, double> pu_Vrms);
    // "pu_Vrms" means "per unit voltage root mean squared"
    SE_power get_SE_power(SupplyEquipmentId SE_id, double prev_unix_time, double now_unix_time, double pu_Vrms);
    
    //---------------------------------------------
    
    ES500_aggregator_charging_needs ES500_get_charging_needs(double unix_time_now, double unix_time_begining_of_next_agg_step);                
    void ES500_set_energy_setpoints(ES500_aggregator_e_step_setpoints pev_energy_setpoints);  

};

#endif