
#ifndef inl_ICM_interface_H
#define inl_ICM_interface_H

#include "datatypes_global.h"                       // grid_node_id_type, SE_id_type, station_configuration, station_charge_event_data, station_status
#include "datatypes_global_SE_EV_definitions.h"     // get_supply_equipment_enum, get_vehicle_enum, supply_equipment_is_L1, supply_equipment_is_L2, pev_is_compatible_with_supply_equipment
#include "supply_equipment_group.h"               // supply_equipment_group
#include "supply_equipment.h"                       // supply_equipment
#include "supply_equipment_control.h"               // manage_L2_control_strategy_parameters
#include "charge_profile_library.h"                 // pev_charge_profile_library
#include "helper.h"                                 // get_base_load_forecast

#include <vector>
#include <map>
#include <tuple>

#include <pybind11/pybind11.h>
namespace py = pybind11;


class factory_EV_charge_model;
class factory_ac_to_dc_converter;


class interface_to_SE_groups
{
private:
    // unordered_maps may not be used here if they are iterated through.  To maintain repeatable
    // stochastic behavior.

    std::vector<supply_equipment_group> SE_group_objs;
    std::map<int, supply_equipment_group*> SE_group_Id_to_ptr;
    std::map<SE_id_type, supply_equipment*> SEid_to_SE_ptr;
    std::vector<supply_equipment*> SE_ptr_vector;    
    std::map<grid_node_id_type, std::vector<supply_equipment*> > gridNodeId_to_SE_ptrs;
    
    // Pointers to the following should be in every supply_equipment_load object.
    factory_EV_charge_model* EV_model_factory;
    factory_ac_to_dc_converter* ac_to_dc_converter_factory;
    pev_charge_profile_library charge_profile_library;
    get_base_load_forecast baseLD_forecaster;
    manage_L2_control_strategy_parameters manage_L2_control;
    
public:
    interface_to_SE_groups() {};    
    ~interface_to_SE_groups();
    void initialize(bool create_charge_profile_library, std::map<vehicle_enum, pev_charge_ramping> ramping_by_pevType_only, std::vector<pev_charge_ramping_workaround> ramping_by_pevType_seType);
    void initialize_infrastructure(charge_event_queuing_inputs CE_queuing_inputs, std::vector<SE_group_configuration> infrastructure_topology);
    void initialize_baseLD_forecaster(double data_start_unix_time, int data_timestep_sec, std::vector<double>& actual_load_akW, std::vector<double>& forecast_load_akW, double adjustment_interval_hrs);
    void initialize_L2_control_strategy_parameters(L2_control_strategy_parameters L2_parameters);
    void set_ensure_pev_charge_needs_met_for_ext_control_strategy(bool ensure_pev_charge_needs_met);
    
    void stop_active_charge_events(std::vector<SE_id_type> SE_ids);
    void add_charge_events(std::vector<charge_event_data> charge_events);
    void add_charge_events_by_SE_group(std::vector<SE_group_charge_event_data> SE_group_charge_events);
    void set_PQ_setpoints(double now_unix_time, std::vector<SE_setpoint> SE_setpoints);
    std::vector<completed_CE> get_completed_CE();

    std::vector<CE_FICE> get_FICE_by_extCS(std::string external_control_strategy, FICE_inputs inputs);
    std::vector<CE_FICE_in_SE_group> get_FICE_by_SE_groups(std::vector<int> SE_group_ids, FICE_inputs inputs);
    std::vector<CE_FICE> get_FICE_by_SEids(std::vector<int> SEids, FICE_inputs inputs);

    std::vector<active_CE> get_all_active_CEs();
    std::map<std::string, std::vector<active_CE> > get_active_CEs_by_extCS(std::vector<std::string> external_control_strategies);
    std::map<int, std::vector<active_CE> > get_active_CEs_by_SE_groups(std::vector<int> SE_group_ids);
    std::vector<active_CE> get_active_CEs_by_SEids(std::vector<SE_id_type> SEids);

    std::vector<double> get_SE_charge_profile_forecast_akW(SE_id_type SE_id, double setpoint_P3kW, double time_step_mins);
    std::vector<double> get_SE_group_charge_profile_forecast_akW(int SE_group, double setpoint_P3kW, double time_step_mins);
    
    std::map<grid_node_id_type, std::pair<double, double> > get_charging_power(double prev_unix_time, double now_unix_time, std::map<grid_node_id_type, double> pu_Vrms);
    SE_power get_SE_power(SE_id_type SE_id, double prev_unix_time, double now_unix_time, double pu_Vrms);
    
    //---------------------------------------------
    
    ES500_aggregator_charging_needs ES500_get_charging_needs(double unix_time_now, double unix_time_begining_of_next_agg_step);                
    void ES500_set_energy_setpoints(ES500_aggregator_e_step_setpoints pev_energy_setpoints);  
    
    // Not Implemented
    serialized_protobuf_obj ES500_get_charging_needs_protobuf(double unix_time_now, double unix_time_begining_of_next_agg_step);                
    void ES500_set_energy_setpoints_protobuf(py::bytes pev_energy_setpoints);
};


#endif

