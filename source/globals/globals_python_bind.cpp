#include "datatypes_global.h"
#include "EVSE_characteristics.h"
#include "EV_characteristics.h"
#include "EV_EVSE_inventory.h"
#include "inputs.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// delete when done adding pickling functionality
#include <typeinfo>
#include <iostream>

namespace py = pybind11;

PYBIND11_MODULE(Caldera_globals, m)
{
    m.def("get_LPF_window_enum", &get_LPF_window_enum);
    m.def("L2_control_strategy_supports_Vrms_using_QkVAR", &L2_control_strategy_supports_Vrms_using_QkVAR);
    m.def("get_L2_control_strategies_enum", &get_L2_control_strategies_enum);
    m.def("is_L2_ES_control_strategy", &is_L2_ES_control_strategy);
    m.def("is_L2_VS_control_strategy", &is_L2_VS_control_strategy);

    //--------------------------------------------
    //       timeseries
    //--------------------------------------------
    py::class_<timeseries>(m, "timeseries")
        .def(py::init<const double, const double, const std::vector<double>& >())
        .def_readwrite("data_starttime_sec", &timeseries::data_starttime_sec)
        .def_readwrite("data_timestep_sec", &timeseries::data_timestep_sec)
        .def_readwrite("data", &timeseries::data)
        .def_readwrite("data_endtime_sec", &timeseries::data_endtime_sec)
        .def("get_val_from_time", &timeseries::get_val_from_time)
        .def("get_val_from_index", &timeseries::get_val_from_index)
        .def("get_time_from_index_sec", &timeseries::get_time_from_index_sec)
        .def(py::pickle(
            [](const timeseries& obj)
            {  // __getstate__
                return py::make_tuple(obj.data_starttime_sec, obj.data_timestep_sec, obj.data);
            },
            [](py::tuple t)
            {  // __setstate__
                
                double data_starttime_sec = t[0].cast<double>();
                double data_timestep_sec = t[1].cast<double>();
                std::vector<double> data = {};
                for(auto x : t[2])
                    data.push_back(x.cast<double>());

                return timeseries{ data_starttime_sec, data_timestep_sec, data };
            }
    ));

    //--------------------------------------------
    //       interface_to_SE_groups_inputs
    //--------------------------------------------

    py::class_<interface_to_SE_groups_inputs>(m, "interface_to_SE_groups_inputs")
        .def(py::init< bool, EV_ramping_map, std::vector<pev_charge_ramping_workaround>, charge_event_queuing_inputs, std::vector<SE_group_configuration>, double, int, std::vector<double>, std::vector<double>, double,L2_control_strategy_parameters, bool >());

    //---------------------------------
    //       Charge Event Data
    //---------------------------------    
    py::enum_<stop_charging_decision_metric>(m, "stop_charging_decision_metric")
        .value("stop_charging_using_target_soc", stop_charging_decision_metric::stop_charging_using_target_soc)
        .value("stop_charging_using_depart_time", stop_charging_decision_metric::stop_charging_using_depart_time)
        .value("stop_charging_using_whatever_happens_first", stop_charging_decision_metric::stop_charging_using_whatever_happens_first);

    py::enum_<stop_charging_mode>(m, "stop_charging_mode")
        .value("target_charging", stop_charging_mode::target_charging)
        .value("block_charging", stop_charging_mode::block_charging);

    py::class_<stop_charging_criteria>(m, "stop_charging_criteria")
        .def(py::init<>())
        .def(py::init<stop_charging_decision_metric, stop_charging_mode, stop_charging_mode, double, double>())
        .def_readwrite("decision_metric", &stop_charging_criteria::decision_metric)
        .def_readwrite("soc_mode", &stop_charging_criteria::soc_mode)
        .def_readwrite("depart_time_mode", &stop_charging_criteria::depart_time_mode)
        .def_readwrite("soc_block_charging_max_undershoot_percent", &stop_charging_criteria::soc_block_charging_max_undershoot_percent)
        .def_readwrite("depart_time_block_charging_max_undershoot_percent", &stop_charging_criteria::depart_time_block_charging_max_undershoot_percent)
        .def(py::pickle(
            [](const stop_charging_criteria& obj)
            {  // __getstate__
                return py::make_tuple(obj.decision_metric, obj.soc_mode, obj.depart_time_mode, obj.soc_block_charging_max_undershoot_percent, obj.depart_time_block_charging_max_undershoot_percent);
            },
            [](py::tuple t)
            {  // __setstate__
                stop_charging_criteria obj;
                obj.decision_metric = t[0].cast<stop_charging_decision_metric>();
                obj.soc_mode = t[1].cast<stop_charging_mode>();
                obj.depart_time_mode = t[2].cast<stop_charging_mode>();
                obj.soc_block_charging_max_undershoot_percent = t[3].cast<double>();
                obj.depart_time_block_charging_max_undershoot_percent = t[4].cast<double>();
                return obj;
            }
    ));

    py::class_<charge_event_data>(m, "charge_event_data")
        .def(py::init<>())
        .def(py::init<int, int, SupplyEquipmentId, vehicle_id_type, EV_type, double, double, double, double, stop_charging_criteria, control_strategy_enums>())
        .def_readwrite("charge_event_id", &charge_event_data::charge_event_id)
        .def_readwrite("SE_group_id", &charge_event_data::SE_group_id)
        .def_readwrite("SE_id", &charge_event_data::SE_id)
        .def_readwrite("vehicle_id", &charge_event_data::vehicle_id)
        .def_readwrite("vehicle_type", &charge_event_data::vehicle_type)
        .def_readwrite("arrival_unix_time", &charge_event_data::arrival_unix_time)
        .def_readwrite("departure_unix_time", &charge_event_data::departure_unix_time)
        .def_readwrite("arrival_SOC", &charge_event_data::arrival_SOC)
        .def_readwrite("departure_SOC", &charge_event_data::departure_SOC)
        .def_readwrite("stop_charge", &charge_event_data::stop_charge)
        .def_readwrite("control_enums", &charge_event_data::control_enums)
        .def_static("get_file_header", &charge_event_data::get_file_header)
        .def(py::pickle(
            [](const charge_event_data& obj)
            {  // __getstate__
                return py::make_tuple(obj.charge_event_id, obj.SE_group_id, obj.SE_id, obj.vehicle_id, obj.vehicle_type, obj.arrival_unix_time, obj.departure_unix_time, obj.arrival_SOC, obj.departure_SOC, obj.stop_charge, obj.control_enums);
            },
            [](py::tuple t)
            {  // __setstate__
                charge_event_data obj;
                obj.charge_event_id = t[0].cast<int>();
                obj.SE_group_id = t[1].cast<int>();
                obj.SE_id = t[2].cast<SupplyEquipmentId>();
                obj.vehicle_id = t[3].cast<vehicle_id_type>();
                obj.vehicle_type = t[4].cast<EV_type>();
                obj.arrival_unix_time = t[5].cast<double>();
                obj.departure_unix_time = t[6].cast<double>();
                obj.arrival_SOC = t[7].cast<double>();
                obj.departure_SOC = t[8].cast<double>();
                obj.stop_charge = t[9].cast<stop_charging_criteria>();
                obj.control_enums = t[10].cast<control_strategy_enums>();
                return obj;
            }
    ));

    py::class_<SE_group_charge_event_data>(m, "SE_group_charge_event_data")
        .def(py::init<>())
        .def(py::init<int, std::vector<charge_event_data>>())
        .def_readwrite("SE_group_id", &SE_group_charge_event_data::SE_group_id)
        .def_readwrite("charge_events", &SE_group_charge_event_data::charge_events)
        .def(py::pickle(
            [](const SE_group_charge_event_data& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_group_id, obj.charge_events);
            },
            [](py::tuple t)
            {  // __setstate__
                SE_group_charge_event_data obj;
                obj.SE_group_id = t[0].cast<int>();

                for (auto x : t[1])
                    obj.charge_events.push_back(x.cast<charge_event_data>());

                return obj;
            }
    ));

    //=======================================

    py::enum_<queuing_mode_enum>(m, "queuing_mode_enum")
        .value("overlapAllowed_earlierArrivalTimeHasPriority", queuing_mode_enum::overlapAllowed_earlierArrivalTimeHasPriority)
        .value("overlapLimited_mostRecentlyQueuedHasPriority", queuing_mode_enum::overlapLimited_mostRecentlyQueuedHasPriority);

    py::class_<charge_event_queuing_inputs>(m, "charge_event_queuing_inputs")
        .def(py::init<>())
        .def_readwrite("max_allowed_overlap_time_sec", &charge_event_queuing_inputs::max_allowed_overlap_time_sec)
        .def_readwrite("queuing_mode", &charge_event_queuing_inputs::queuing_mode)
        .def(py::pickle(
            [](const charge_event_queuing_inputs& obj)
            {  // __getstate__
                return py::make_tuple(obj.max_allowed_overlap_time_sec, obj.queuing_mode);
            },
            [](py::tuple t)
            {  // __setstate__
                charge_event_queuing_inputs obj;
                obj.max_allowed_overlap_time_sec = t[0].cast<double>();
                obj.queuing_mode = t[1].cast<queuing_mode_enum>();

                return obj;
            }
    ));

    //---------------------------------
    //       SE Configuration
    //---------------------------------
    py::class_<SE_configuration>(m, "SE_configuration")
        .def(py::init<>())
        .def(py::init<int, SupplyEquipmentId, EVSE_type, double, double, grid_node_id_type, std::string>())
        .def_readwrite("SE_group_id", &SE_configuration::SE_group_id)
        .def_readwrite("SE_id", &SE_configuration::SE_id)
        .def_readwrite("supply_equipment_type", &SE_configuration::supply_equipment_type)
        .def_readwrite("lattitude", &SE_configuration::lattitude)
        .def_readwrite("longitude", &SE_configuration::longitude)
        .def_readwrite("grid_node_id", &SE_configuration::grid_node_id)
        .def_readwrite("location_type", &SE_configuration::location_type)
        .def(py::pickle(
            [](const SE_configuration& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_group_id, obj.SE_id, obj.supply_equipment_type, obj.lattitude, obj.longitude, obj.grid_node_id, obj.location_type);
            },
            [](py::tuple t)
            {  // __setstate__
                SE_configuration obj;
                obj.SE_group_id = t[0].cast<int>();
                obj.SE_id = t[1].cast<SupplyEquipmentId>();
                obj.supply_equipment_type = t[2].cast<EVSE_type>();
                obj.lattitude = t[3].cast<double>();
                obj.longitude = t[4].cast<double>();
                obj.grid_node_id = t[5].cast<grid_node_id_type>();
                obj.location_type = t[6].cast<std::string>();
                return obj;
            }
    ));

    py::class_<SE_group_configuration>(m, "SE_group_configuration")
        .def(py::init<>())
        .def(py::init<int, std::vector<SE_configuration> >())
        .def_readwrite("SE_group_id", &SE_group_configuration::SE_group_id)
        .def_readwrite("SEs", &SE_group_configuration::SEs)
        .def(py::pickle(
            [](const SE_group_configuration& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_group_id, obj.SEs);
            },
            [](py::tuple t)
            {  // __setstate__
                SE_group_configuration obj;
                obj.SE_group_id = t[0].cast<int>();
                for (auto x : t[1])
                    obj.SEs.push_back(x.cast<SE_configuration>());

                return obj;
            }
    ));

    //---------------------------------
    //       Status of CE
    //---------------------------------

    py::enum_<SE_charging_status>(m, "SE_charging_status")
        .value("no_ev_plugged_in", SE_charging_status::no_ev_plugged_in)
        .value("ev_plugged_in_not_charging", SE_charging_status::ev_plugged_in_not_charging)
        .value("ev_charging", SE_charging_status::ev_charging)
        .value("ev_charge_complete", SE_charging_status::ev_charge_complete);

    py::class_<FICE_inputs>(m, "FICE_inputs")
        .def(py::init<>())
        .def_readwrite("interval_start_unixtime", &FICE_inputs::interval_start_unixtime)
        .def_readwrite("interval_duration_sec", &FICE_inputs::interval_duration_sec)
        .def_readwrite("acPkW_setpoint", &FICE_inputs::acPkW_setpoint)
        .def(py::pickle(
            [](const FICE_inputs& obj)
            {  // __getstate__
                return py::make_tuple(obj.interval_start_unixtime, obj.interval_duration_sec, obj.acPkW_setpoint);
            },
            [](py::tuple t)
            {  // __setstate__
                FICE_inputs obj;
                obj.interval_start_unixtime = t[0].cast<double>();
                obj.interval_duration_sec = t[1].cast<double>();
                obj.acPkW_setpoint = t[2].cast<double>();
                return obj;
            }
    ));

    py::class_<CE_FICE>(m, "CE_FICE")
        .def(py::init<>())
        .def_readwrite("SE_id", &CE_FICE::SE_id)
        .def_readwrite("charge_event_id", &CE_FICE::charge_event_id)
        .def_readwrite("charge_energy_ackWh", &CE_FICE::charge_energy_ackWh)
        .def_readwrite("interval_duration_hrs", &CE_FICE::interval_duration_hrs)
        .def(py::pickle(
            [](const CE_FICE& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_id, obj.charge_event_id, obj.charge_energy_ackWh, obj.interval_duration_hrs);
            },
            [](py::tuple t)
            {  // __setstate__
                CE_FICE obj;
                obj.SE_id = t[0].cast<SupplyEquipmentId>();
                obj.charge_event_id = t[1].cast<int>();
                obj.charge_energy_ackWh = t[2].cast<double>();
                obj.interval_duration_hrs = t[3].cast<double>();
                return obj;
            }
    ));

    py::class_<CE_FICE_in_SE_group>(m, "CE_FICE_in_SE_group")
        .def(py::init<>())
        .def_readwrite("SE_group_id", &CE_FICE_in_SE_group::SE_group_id)
        .def_readwrite("SE_FICE_vals", &CE_FICE_in_SE_group::SE_FICE_vals)
        .def(py::pickle(
            [](const CE_FICE_in_SE_group& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_group_id, obj.SE_FICE_vals);
            },
            [](py::tuple t)
            {  // __setstate__
                CE_FICE_in_SE_group obj;
                obj.SE_group_id = t[0].cast<int>();
                for (auto x : t[1])
                    obj.SE_FICE_vals.push_back(x.cast<CE_FICE>());

                return obj;
            }
    ));

    py::class_<active_CE>(m, "active_CE")
        .def(py::init<>())
        .def_readwrite("SE_id", &active_CE::SE_id)
        .def_readwrite("supply_equipment_type", &active_CE::supply_equipment_type)
        .def_readwrite("charge_event_id", &active_CE::charge_event_id)
        .def_readwrite("vehicle_id", &active_CE::vehicle_id)
        .def_readwrite("vehicle_type", &active_CE::vehicle_type)
        .def_readwrite("arrival_unix_time", &active_CE::arrival_unix_time)
        .def_readwrite("departure_unix_time", &active_CE::departure_unix_time)
        .def_readwrite("arrival_SOC", &active_CE::arrival_SOC)
        .def_readwrite("departure_SOC", &active_CE::departure_SOC)

        .def_readwrite("now_unix_time", &active_CE::now_unix_time)
        .def_readwrite("now_soc", &active_CE::now_soc)
        .def_readwrite("min_remaining_charge_time_hrs", &active_CE::min_remaining_charge_time_hrs)
        .def_readwrite("min_time_to_complete_entire_charge_hrs", &active_CE::min_time_to_complete_entire_charge_hrs)
        .def_readwrite("now_charge_energy_ackWh", &active_CE::now_charge_energy_ackWh)
        .def_readwrite("energy_of_complete_charge_ackWh", &active_CE::energy_of_complete_charge_ackWh)
        .def_readwrite("now_dcPkW", &active_CE::now_dcPkW)
        .def_readwrite("now_acPkW", &active_CE::now_acPkW)
        .def_readwrite("now_acQkVAR", &active_CE::now_acQkVAR)
        
        
        .def(py::pickle(
            [](const active_CE& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_id, obj.supply_equipment_type, obj.charge_event_id, obj.vehicle_id, obj.vehicle_type, 
                obj.arrival_unix_time, obj.departure_unix_time, obj.arrival_SOC, obj.departure_SOC, obj.now_unix_time,
                obj.now_soc, obj.now_charge_energy_ackWh, obj.energy_of_complete_charge_ackWh, obj.now_dcPkW, obj.now_acPkW, 
                obj.now_acQkVAR, obj.min_remaining_charge_time_hrs, obj.min_time_to_complete_entire_charge_hrs
                );
            },
            [](py::tuple t)
            {  // __setstate_
                active_CE obj;
                obj.SE_id = t[0].cast<SupplyEquipmentId>();
                obj.supply_equipment_type = t[1].cast<EVSE_type>();
                obj.charge_event_id = t[2].cast<int>();
                obj.vehicle_id = t[3].cast<vehicle_id_type>();
                obj.vehicle_type = t[4].cast<EV_type>();
                obj.arrival_unix_time = t[5].cast<double>();
                obj.departure_unix_time = t[6].cast<double>();
                obj.arrival_SOC = t[7].cast<double>();
                obj.departure_SOC = t[8].cast<double>();
                
                obj.now_unix_time = t[9].cast<double>();
                obj.now_soc = t[10].cast<double>();
                obj.now_charge_energy_ackWh = t[11].cast<double>();
                obj.energy_of_complete_charge_ackWh = t[12].cast<double>();
                obj.now_dcPkW = t[13].cast<double>();
                obj.now_acPkW = t[14].cast<double>();
                obj.now_acQkVAR = t[15].cast<double>();
                
                obj.min_remaining_charge_time_hrs = t[16].cast<double>();                
                obj.min_time_to_complete_entire_charge_hrs = t[17].cast<double>();

                return obj;
            }
    ));

    py::class_<SE_setpoint>(m, "SE_setpoint")
        .def(py::init<>())
        .def_readwrite("SE_id", &SE_setpoint::SE_id)
        .def_readwrite("PkW", &SE_setpoint::PkW)
        .def_readwrite("QkVAR", &SE_setpoint::QkVAR)
        .def(py::pickle(
            [](const SE_setpoint& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_id, obj.PkW, obj.QkVAR);
            },
            [](py::tuple t)
            {  // __setstate__
                SE_setpoint obj;
                obj.SE_id = t[0].cast<SupplyEquipmentId>();
                obj.PkW = t[1].cast<double>();
                obj.QkVAR = t[2].cast<double>();

                return obj;
            }
    ));

    py::class_<completed_CE>(m, "completed_CE")
        .def(py::init<>())
        .def_readwrite("SE_id", &completed_CE::SE_id)
        .def_readwrite("charge_event_id", &completed_CE::charge_event_id)
        .def_readwrite("final_soc", &completed_CE::final_soc)
        .def(py::pickle(
            [](const completed_CE& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_id, obj.charge_event_id, obj.final_soc);
            },
            [](py::tuple t)
            {  // __setstate__
                completed_CE obj;
                obj.SE_id = t[0].cast<SupplyEquipmentId>();
                obj.charge_event_id = t[1].cast<int>();
                obj.final_soc = t[2].cast<double>();

                return obj;
            }
    ));

    //---------------------------------
    //       Miscellaneous
    //---------------------------------  
    py::enum_<ac_to_dc_converter_enum>(m, "ac_to_dc_converter_enum")
        .value("pf", ac_to_dc_converter_enum::pf)
        .value("Q_setpoint", ac_to_dc_converter_enum::Q_setpoint);

    py::class_<SE_power>(m, "SE_power")
        .def(py::init<>())
        .def_readwrite("time_step_duration_hrs", &SE_power::time_step_duration_hrs)
        .def_readwrite("P1_kW", &SE_power::P1_kW)
        .def_readwrite("P2_kW", &SE_power::P2_kW)
        .def_readwrite("P3_kW", &SE_power::P3_kW)
        .def_readwrite("Q3_kVAR", &SE_power::Q3_kVAR)
        .def_readwrite("soc", &SE_power::soc)
        .def_readwrite("SE_status_val", &SE_power::SE_status_val)
        .def(py::pickle(
            [](const SE_power& obj)
            {  // __getstate__
                return py::make_tuple(obj.time_step_duration_hrs, obj.P1_kW, obj.P2_kW, obj.P3_kW, obj.Q3_kVAR, obj.soc, obj.SE_status_val);
            },
            [](py::tuple t)
            {  // __setstate__
                SE_power obj;
                obj.time_step_duration_hrs = t[0].cast<double>();
                obj.P1_kW = t[1].cast<double>();
                obj.P2_kW = t[2].cast<double>();
                obj.P3_kW = t[3].cast<double>();
                obj.Q3_kVAR = t[4].cast<double>();
                obj.soc = t[5].cast<double>();
                obj.SE_status_val = t[6].cast<SE_charging_status>();

                return obj;
            }
    ));

    py::class_<pev_batterySize_info>(m, "pev_batterySize_info")
        .def(py::init<>())
        .def_readwrite("vehicle_type", &pev_batterySize_info::vehicle_type)
        .def_readwrite("battery_size_kWh", &pev_batterySize_info::battery_size_kWh)
        .def_readwrite("battery_size_with_stochastic_degredation_kWh", &pev_batterySize_info::battery_size_with_stochastic_degredation_kWh)

        .def(py::pickle(
            [](const pev_batterySize_info& obj)
            {  // __getstate__
                return py::make_tuple(obj.vehicle_type, obj.battery_size_kWh, obj.battery_size_with_stochastic_degredation_kWh);
            },
            [](py::tuple t)
            {  // __setstate__
                pev_batterySize_info obj;
                obj.vehicle_type = t[0].cast<EV_type>();
                obj.battery_size_kWh = t[1].cast<double>();
                obj.battery_size_with_stochastic_degredation_kWh = t[2].cast<double>();

                return obj;
            }
    ));

    //---------------------------------
    //       PEV Charge Profile 
    //---------------------------------
    py::class_<pev_charge_profile_result>(m, "pev_charge_profile_result")
        .def(py::init<>())
        .def_readwrite("soc_increase", &pev_charge_profile_result::soc_increase)
        .def_readwrite("E1_kWh", &pev_charge_profile_result::E1_kWh)
        .def_readwrite("E2_kWh", &pev_charge_profile_result::E2_kWh)
        .def_readwrite("E3_kWh", &pev_charge_profile_result::E3_kWh)
        .def_readwrite("cumQ3_kVARh", &pev_charge_profile_result::cumQ3_kVARh)
        .def_readwrite("total_charge_time_hrs", &pev_charge_profile_result::total_charge_time_hrs)
        .def_readwrite("incremental_chage_time_hrs", &pev_charge_profile_result::incremental_chage_time_hrs)
        .def(py::pickle(
            [](const pev_charge_profile_result& obj)
            {  // __getstate__
                return py::make_tuple(obj.soc_increase, obj.E1_kWh, obj.E2_kWh, obj.E3_kWh, obj.cumQ3_kVARh, obj.total_charge_time_hrs, obj.incremental_chage_time_hrs);
            },
            [](py::tuple t)
            {  // __setstate__
                pev_charge_profile_result obj;
                obj.soc_increase = t[0].cast<double>();
                obj.E1_kWh = t[1].cast<double>();
                obj.E2_kWh = t[2].cast<double>();
                obj.E3_kWh = t[3].cast<double>();
                obj.cumQ3_kVARh = t[4].cast<double>();
                obj.total_charge_time_hrs = t[5].cast<double>();
                obj.incremental_chage_time_hrs = t[6].cast<double>();
                return obj;
            }
    ));

    py::class_<pev_charge_fragment_removal_criteria>(m, "pev_charge_fragment_removal_criteria")
        .def(py::init<>())
        .def_readwrite("max_percent_of_fragments_that_can_be_removed", &pev_charge_fragment_removal_criteria::max_percent_of_fragments_that_can_be_removed)
        .def_readwrite("kW_change_threshold", &pev_charge_fragment_removal_criteria::kW_change_threshold)
        .def_readwrite("threshold_to_determine_not_removable_fragments_on_flat_peak_kW", &pev_charge_fragment_removal_criteria::threshold_to_determine_not_removable_fragments_on_flat_peak_kW)
        .def_readwrite("perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow", &pev_charge_fragment_removal_criteria::perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow)
        .def(py::pickle(
            [](const pev_charge_fragment_removal_criteria& obj)
            {  // __getstate__
                return py::make_tuple(obj.max_percent_of_fragments_that_can_be_removed, obj.kW_change_threshold, obj.threshold_to_determine_not_removable_fragments_on_flat_peak_kW, obj.perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow);
            },
            [](py::tuple t)
            {  // __setstate__
                pev_charge_fragment_removal_criteria obj;
                obj.max_percent_of_fragments_that_can_be_removed = t[0].cast<double>();
                obj.kW_change_threshold = t[1].cast<double>();
                obj.threshold_to_determine_not_removable_fragments_on_flat_peak_kW = t[2].cast<double>();
                obj.perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow = t[3].cast<double>();
                return obj;
            }
    ));

    py::class_<pev_charge_fragment>(m, "pev_charge_fragment")
        .def(py::init<>())
        .def(py::init<double, double, double, double, double, double>())
        .def_readwrite("soc", &pev_charge_fragment::soc)
        .def_readwrite("E1_kWh", &pev_charge_fragment::E1_kWh)
        .def_readwrite("E2_kWh", &pev_charge_fragment::E2_kWh)
        .def_readwrite("E3_kWh", &pev_charge_fragment::E3_kWh)
        .def_readwrite("cumQ3_kVARh", &pev_charge_fragment::cumQ3_kVARh)
        .def_readwrite("time_since_charge_began_hrs", &pev_charge_fragment::time_since_charge_began_hrs)
        .def(py::pickle(
            [](const pev_charge_fragment& obj)
            {  // __getstate__
                return py::make_tuple(obj.soc, obj.E1_kWh, obj.E2_kWh, obj.E3_kWh, obj.cumQ3_kVARh, obj.time_since_charge_began_hrs);
            },
            [](py::tuple t)
            {  // __setstate__
                pev_charge_fragment obj;
                obj.soc = t[0].cast<double>();
                obj.E1_kWh = t[1].cast<double>();
                obj.E2_kWh = t[2].cast<double>();
                obj.E3_kWh = t[3].cast<double>();
                obj.cumQ3_kVARh = t[4].cast<double>();
                obj.time_since_charge_began_hrs = t[5].cast<double>();
                return obj;
            }
    ));

    py::class_<pev_charge_fragment_variation>(m, "pev_charge_fragment_variation")
        .def(py::init<>())
        .def(py::init<int, double, double, double>())
        .def_readwrite("is_removable", &pev_charge_fragment_variation::is_removable)
        .def_readwrite("original_charge_fragment_index", &pev_charge_fragment_variation::original_charge_fragment_index)
        .def_readwrite("time_since_charge_began_hrs", &pev_charge_fragment_variation::time_since_charge_began_hrs)
        .def_readwrite("soc", &pev_charge_fragment_variation::soc)
        .def_readwrite("P3_kW", &pev_charge_fragment_variation::P3_kW)
        .def_readwrite("variation_rank", &pev_charge_fragment_variation::variation_rank)
        .def(py::pickle(
            [](const pev_charge_fragment_variation& obj)
            {  // __getstate__
                return py::make_tuple(obj.is_removable, obj.original_charge_fragment_index, obj.time_since_charge_began_hrs, obj.soc, obj.P3_kW, obj.variation_rank);
            },
            [](py::tuple t)
            {  // __setstate__
                pev_charge_fragment_variation obj;
                obj.is_removable = t[0].cast<bool>();
                obj.original_charge_fragment_index = t[1].cast<int>();
                obj.time_since_charge_began_hrs = t[2].cast<double>();
                obj.soc = t[3].cast<double>();
                obj.P3_kW = t[4].cast<double>();
                obj.variation_rank = t[5].cast<int64_t>();
                return obj;
            }
    ));

    py::class_<charge_profile_validation_data>(m, "charge_profile_validation_data")
        .def(py::init<>())
        .def_readwrite("time_step_sec", &charge_profile_validation_data::time_step_sec)
        .def_readwrite("target_acP3_kW", &charge_profile_validation_data::target_acP3_kW)
        .def_readwrite("fragment_removal_criteria", &charge_profile_validation_data::fragment_removal_criteria)
        .def_readwrite("removed_fragments", &charge_profile_validation_data::removed_fragments)
        .def_readwrite("retained_fragments", &charge_profile_validation_data::retained_fragments)
        .def_readwrite("downsampled_charge_fragments", &charge_profile_validation_data::downsampled_charge_fragments)
        .def_readwrite("original_charge_fragments", &charge_profile_validation_data::original_charge_fragments)
        .def(py::pickle(
            [](const charge_profile_validation_data& obj)
            {  // __getstate__
                return py::make_tuple(obj.time_step_sec, obj.target_acP3_kW, obj.fragment_removal_criteria, obj.removed_fragments, obj.retained_fragments, obj.downsampled_charge_fragments, obj.original_charge_fragments);
            },
            [](py::tuple t)
            {  // __setstate__
                charge_profile_validation_data obj;
                obj.time_step_sec = t[0].cast<double>();
                obj.target_acP3_kW = t[1].cast<double>();
                obj.fragment_removal_criteria = t[2].cast<pev_charge_fragment_removal_criteria>();

                for (auto x : t[3])
                    obj.removed_fragments.push_back(x.cast<pev_charge_fragment_variation>());

                for (auto x : t[4])
                    obj.retained_fragments.push_back(x.cast<pev_charge_fragment_variation>());

                for (auto x : t[5])
                    obj.downsampled_charge_fragments.push_back(x.cast<pev_charge_fragment>());

                for (auto x : t[6])
                    obj.original_charge_fragments.push_back(x.cast<pev_charge_fragment>());

                return obj;
            }
    ));

    py::class_<all_charge_profile_data>(m, "all_charge_profile_data")
        .def(py::init<>())
        .def_readwrite("timestep_sec", &all_charge_profile_data::timestep_sec)
        .def_readwrite("P1_kW", &all_charge_profile_data::P1_kW)
        .def_readwrite("P2_kW", &all_charge_profile_data::P2_kW)
        .def_readwrite("P3_kW", &all_charge_profile_data::P3_kW)
        .def_readwrite("Q3_kVAR", &all_charge_profile_data::Q3_kVAR)
        .def_readwrite("soc", &all_charge_profile_data::soc);

    py::class_<charge_event_P3kW_limits>(m, "charge_event_P3kW_limits")
        .def(py::init<>())
        .def_readwrite("min_P3kW", &charge_event_P3kW_limits::min_P3kW)
        .def_readwrite("max_P3kW", &charge_event_P3kW_limits::max_P3kW)
        .def(py::pickle(
            [](const charge_event_P3kW_limits& obj)
            {  // __getstate__
                return py::make_tuple(obj.min_P3kW, obj.max_P3kW);
            },
            [](py::tuple t)
            {  // __setstate__
                charge_event_P3kW_limits obj;
                obj.min_P3kW = t[0].cast<double>();
                obj.max_P3kW = t[1].cast<double>();
                return obj;
            }
    ));

    //---------------------------------
    //  Low Pass Filter Parameters
    //--------------------------------- 
    py::enum_<LPF_window_enum>(m, "LPF_window_enum")
        .value("Hanning", LPF_window_enum::Hanning)
        .value("Blackman", LPF_window_enum::Blackman)
        .value("Rectangular", LPF_window_enum::Rectangular);

    py::class_<LPF_parameters>(m, "LPF_parameters")
        .def(py::init<>())
        .def_readwrite("window_size", &LPF_parameters::window_size)
        .def_readwrite("window_type", &LPF_parameters::window_type)
        .def(py::pickle(
            [](const LPF_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.window_size, obj.window_type);
            },
            [](py::tuple t)
            {  // __setstate__
                LPF_parameters obj;
                obj.window_size = t[0].cast<int>();
                obj.window_type = t[1].cast<LPF_window_enum>();
                return obj;
            }
    ));

    py::class_<LPF_parameters_randomize_window_size>(m, "LPF_parameters_randomize_window_size")
        .def(py::init<>())
        .def_readwrite("is_active", &LPF_parameters_randomize_window_size::is_active)
        .def_readwrite("seed", &LPF_parameters_randomize_window_size::seed)
        .def_readwrite("window_size_LB", &LPF_parameters_randomize_window_size::window_size_LB)
        .def_readwrite("window_size_UB", &LPF_parameters_randomize_window_size::window_size_UB)
        .def_readwrite("window_type", &LPF_parameters_randomize_window_size::window_type)
        .def(py::pickle(
            [](const LPF_parameters_randomize_window_size& obj)
            {  // __getstate__
                return py::make_tuple(obj.is_active, obj.seed, obj.window_size_LB, obj.window_size_UB, obj.window_type);
            },
            [](py::tuple t)
            {  // __setstate__
                LPF_parameters_randomize_window_size obj;
                obj.is_active = t[0].cast<bool>();
                obj.seed = t[1].cast<int>();
                obj.window_size_LB = t[2].cast<int>();
                obj.window_size_UB = t[3].cast<int>();
                obj.window_type = t[4].cast<LPF_window_enum>();
                return obj;
            }
    ));

    //---------------------------------
    //   Control Strategy Parameters
    //---------------------------------
    py::enum_<L2_control_strategies_enum>(m, "L2_control_strategies_enum")
        .value("NA", L2_control_strategies_enum::NA)
        .value("ES100_A", L2_control_strategies_enum::ES100_A)
        .value("ES100_B", L2_control_strategies_enum::ES100_B)
        .value("ES110", L2_control_strategies_enum::ES110)
        .value("ES200", L2_control_strategies_enum::ES200)
        .value("ES300", L2_control_strategies_enum::ES300)
        .value("ES500", L2_control_strategies_enum::ES500)
        .value("VS100", L2_control_strategies_enum::VS100)
        .value("VS200_A", L2_control_strategies_enum::VS200_A)
        .value("VS200_B", L2_control_strategies_enum::VS200_B)
        .value("VS200_C", L2_control_strategies_enum::VS200_C)
        .value("VS300", L2_control_strategies_enum::VS300);

    py::class_<ES100_L2_parameters>(m, "ES100_L2_parameters")
        .def(py::init<>())
        .def_readwrite("beginning_of_TofU_rate_period__time_from_midnight_hrs", &ES100_L2_parameters::beginning_of_TofU_rate_period__time_from_midnight_hrs)
        .def_readwrite("end_of_TofU_rate_period__time_from_midnight_hrs", &ES100_L2_parameters::end_of_TofU_rate_period__time_from_midnight_hrs)
        .def_readwrite("randomization_method", &ES100_L2_parameters::randomization_method)
        .def_readwrite("M1_delay_period_hrs", &ES100_L2_parameters::M1_delay_period_hrs)
        .def_readwrite("random_seed", &ES100_L2_parameters::random_seed)
        .def(py::pickle(
            [](const ES100_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.beginning_of_TofU_rate_period__time_from_midnight_hrs, obj.end_of_TofU_rate_period__time_from_midnight_hrs, obj.randomization_method, obj.M1_delay_period_hrs, obj.random_seed);
            },
            [](py::tuple t)
            {  // __setstate__
                ES100_L2_parameters obj;
                obj.beginning_of_TofU_rate_period__time_from_midnight_hrs = t[0].cast<double>();
                obj.end_of_TofU_rate_period__time_from_midnight_hrs = t[1].cast<double>();
                obj.randomization_method = t[2].cast<std::string>();
                obj.M1_delay_period_hrs = t[3].cast<double>();
                obj.random_seed = t[4].cast<int>();
                return obj;
            }
    ));

    py::class_<ES110_L2_parameters>(m, "ES110_L2_parameters")
        .def(py::init<>())
        .def_readwrite("random_seed", &ES110_L2_parameters::random_seed)
        .def(py::pickle(
            [](const ES110_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.random_seed);
            },
            [](py::tuple t)
            {  // __setstate__
                ES110_L2_parameters obj;
                obj.random_seed = t[0].cast<int>();
                return obj;
            }
    ));

    py::class_<ES200_L2_parameters>(m, "ES200_L2_parameters")
        .def(py::init<>())
        .def_readwrite("weight_factor_to_calculate_valley_fill_target", &ES200_L2_parameters::weight_factor_to_calculate_valley_fill_target)
        .def(py::pickle(
            [](const ES200_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.weight_factor_to_calculate_valley_fill_target);
            },
            [](py::tuple t)
            {  // __setstate__
                ES200_L2_parameters obj;
                obj.weight_factor_to_calculate_valley_fill_target = t[0].cast<double>();
                return obj;
            }
    ));

    py::class_<ES300_L2_parameters>(m, "ES300_L2_parameters")
        .def(py::init<>())
        .def_readwrite("weight_factor_to_calculate_valley_fill_target", &ES300_L2_parameters::weight_factor_to_calculate_valley_fill_target)
        .def(py::pickle(
            [](const ES300_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.weight_factor_to_calculate_valley_fill_target);
            },
            [](py::tuple t)
            {  // __setstate__
                ES300_L2_parameters obj;
                obj.weight_factor_to_calculate_valley_fill_target = t[0].cast<double>();
                return obj;
            }
    ));

    py::class_<normal_random_error>(m, "normal_random_error")
        .def(py::init<>())
        .def_readwrite("seed", &normal_random_error::seed)
        .def_readwrite("stdev", &normal_random_error::stdev)
        .def_readwrite("stdev_bounds", &normal_random_error::stdev_bounds)
        .def(py::pickle(
            [](const normal_random_error& obj)
            {  // __getstate__
                return py::make_tuple(obj.seed, obj.stdev, obj.stdev_bounds);
            },
            [](py::tuple t)
            {  // __setstate__
                normal_random_error obj;
                obj.seed = t[0].cast<int>();
                obj.stdev = t[1].cast<double>();
                obj.stdev_bounds = t[2].cast<double>();
                return obj;
            }
    ));

    py::class_<ES500_L2_parameters>(m, "ES500_L2_parameters")
        .def(py::init<>())
        .def_readwrite("aggregator_timestep_mins", &ES500_L2_parameters::aggregator_timestep_mins)
        .def_readwrite("off_to_on_lead_time_sec", &ES500_L2_parameters::off_to_on_lead_time_sec)
        .def_readwrite("default_lead_time_sec", &ES500_L2_parameters::default_lead_time_sec)
        .def(py::pickle(
            [](const ES500_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.aggregator_timestep_mins, obj.off_to_on_lead_time_sec, obj.default_lead_time_sec);
            },
            [](py::tuple t)
            {  // __setstate__
                ES500_L2_parameters obj;
                obj.aggregator_timestep_mins = t[0].cast<double>();
                obj.off_to_on_lead_time_sec = t[1].cast<normal_random_error>();
                obj.default_lead_time_sec = t[2].cast<normal_random_error>();
                return obj;
            }
    ));

    py::class_<VS100_L2_parameters>(m, "VS100_L2_parameters")
        .def(py::init<>())
        .def_readwrite("target_P3_reference__percent_of_maxP3", &VS100_L2_parameters::target_P3_reference__percent_of_maxP3)
        .def_readwrite("max_delta_kW_per_min", &VS100_L2_parameters::max_delta_kW_per_min)
        .def_readwrite("volt_delta_kW_curve_puV", &VS100_L2_parameters::volt_delta_kW_curve_puV)
        .def_readwrite("volt_delta_kW_percP", &VS100_L2_parameters::volt_delta_kW_percP)
        .def_readwrite("voltage_LPF", &VS100_L2_parameters::voltage_LPF)
        .def(py::pickle(
            [](const VS100_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.target_P3_reference__percent_of_maxP3, obj.max_delta_kW_per_min, obj.volt_delta_kW_curve_puV, obj.volt_delta_kW_percP, obj.voltage_LPF);
            },
            [](py::tuple t)
            {  // __setstate__
                VS100_L2_parameters obj;
                obj.target_P3_reference__percent_of_maxP3 = t[0].cast<double>();
                obj.max_delta_kW_per_min = t[1].cast<double>();

                for (auto x : t[2])
                    obj.volt_delta_kW_curve_puV.push_back(x.cast<double>());

                for (auto x : t[3])
                    obj.volt_delta_kW_percP.push_back(x.cast<double>());

                obj.voltage_LPF = t[4].cast<LPF_parameters_randomize_window_size>();

                return obj;
            }
    ));

    py::class_<VS200_L2_parameters>(m, "VS200_L2_parameters")
        .def(py::init<>())
        .def_readwrite("target_P3_reference__percent_of_maxP3", &VS200_L2_parameters::target_P3_reference__percent_of_maxP3)
        .def_readwrite("max_delta_kVAR_per_min", &VS200_L2_parameters::max_delta_kVAR_per_min)
        .def_readwrite("volt_var_curve_puV", &VS200_L2_parameters::volt_var_curve_puV)
        .def_readwrite("volt_var_curve_percQ", &VS200_L2_parameters::volt_var_curve_percQ)
        .def_readwrite("voltage_LPF", &VS200_L2_parameters::voltage_LPF)
        .def(py::pickle(
            [](const VS200_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.target_P3_reference__percent_of_maxP3, obj.max_delta_kVAR_per_min, obj.volt_var_curve_puV, obj.volt_var_curve_percQ, obj.voltage_LPF);
            },
            [](py::tuple t)
            {  // __setstate__
                VS200_L2_parameters obj;
                obj.target_P3_reference__percent_of_maxP3 = t[0].cast<double>();
                obj.max_delta_kVAR_per_min = t[1].cast<double>();

                for (auto x : t[2])
                    obj.volt_var_curve_puV.push_back(x.cast<double>());

                for (auto x : t[3])
                    obj.volt_var_curve_percQ.push_back(x.cast<double>());

                obj.voltage_LPF = t[4].cast<LPF_parameters_randomize_window_size>();

                return obj;
            }
    ));

    py::class_<VS300_L2_parameters>(m, "VS300_L2_parameters")
        .def(py::init<>())
        .def_readwrite("target_P3_reference__percent_of_maxP3", &VS300_L2_parameters::target_P3_reference__percent_of_maxP3)
        .def_readwrite("max_QkVAR_as_percent_of_SkVA", &VS300_L2_parameters::max_QkVAR_as_percent_of_SkVA)
        .def_readwrite("gamma", &VS300_L2_parameters::gamma)
        .def_readwrite("voltage_LPF", &VS300_L2_parameters::voltage_LPF)
        .def(py::pickle(
            [](const VS300_L2_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.target_P3_reference__percent_of_maxP3, obj.max_QkVAR_as_percent_of_SkVA, obj.gamma, obj.voltage_LPF);
            },
            [](py::tuple t)
            {  // __setstate__
                VS300_L2_parameters obj;
                obj.target_P3_reference__percent_of_maxP3 = t[0].cast<double>();
                obj.max_QkVAR_as_percent_of_SkVA = t[1].cast<double>();
                obj.gamma = t[2].cast<double>();
                obj.voltage_LPF = t[3].cast<LPF_parameters_randomize_window_size>();
                return obj;
            }
    ));

    py::class_<control_strategy_enums>(m, "control_strategy_enums")
        .def(py::init<>())
        .def_readwrite("inverter_model_supports_Qsetpoint", &control_strategy_enums::inverter_model_supports_Qsetpoint)
        .def_readwrite("ES_control_strategy", &control_strategy_enums::ES_control_strategy)
        .def_readwrite("VS_control_strategy", &control_strategy_enums::VS_control_strategy)
        .def_readwrite("ext_control_strategy", &control_strategy_enums::ext_control_strategy)
        .def(py::pickle(
            [](const control_strategy_enums& obj)
            {  // __getstate__
                return py::make_tuple(obj.inverter_model_supports_Qsetpoint, obj.ES_control_strategy, obj.VS_control_strategy, obj.ext_control_strategy);
            },
            [](py::tuple t)
            {  // __setstate__
                control_strategy_enums obj;
                obj.inverter_model_supports_Qsetpoint = t[0].cast<bool>();
                obj.ES_control_strategy = t[1].cast<L2_control_strategies_enum>();
                obj.VS_control_strategy = t[2].cast<L2_control_strategies_enum>();
                obj.ext_control_strategy = t[3].cast<std::string>();
                return obj;
            }
    ));

    py::class_<L2_control_strategy_parameters>(m, "L2_control_strategy_parameters")
        .def(py::init<>())

        .def_readwrite("ES100_A", &L2_control_strategy_parameters::ES100_A)
        .def_readwrite("ES100_B", &L2_control_strategy_parameters::ES100_B)
        .def_readwrite("ES110", &L2_control_strategy_parameters::ES110)
        .def_readwrite("ES200", &L2_control_strategy_parameters::ES200)
        .def_readwrite("ES300", &L2_control_strategy_parameters::ES300)
        .def_readwrite("ES500", &L2_control_strategy_parameters::ES500)

        .def_readwrite("VS100", &L2_control_strategy_parameters::VS100)
        .def_readwrite("VS200_A", &L2_control_strategy_parameters::VS200_A)
        .def_readwrite("VS200_B", &L2_control_strategy_parameters::VS200_B)
        .def_readwrite("VS200_C", &L2_control_strategy_parameters::VS200_C)
        .def_readwrite("VS300", &L2_control_strategy_parameters::VS300)

        .def(py::pickle(
            [](const L2_control_strategy_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.ES100_A, obj.ES100_B, obj.ES110, obj.ES200, obj.ES300, obj.ES500,
                obj.VS100, obj.VS200_A, obj.VS200_B, obj.VS200_C, obj.VS300);
            },
            [](py::tuple t)
            {  // __setstate__
                L2_control_strategy_parameters obj;

                obj.ES100_A = t[0].cast<ES100_L2_parameters>();
                obj.ES100_B = t[1].cast<ES100_L2_parameters>();
                obj.ES110 = t[2].cast<ES110_L2_parameters>();
                obj.ES200 = t[3].cast<ES200_L2_parameters>();
                obj.ES300 = t[4].cast<ES300_L2_parameters>();
                obj.ES500 = t[5].cast<ES500_L2_parameters>();

                obj.VS100 = t[6].cast<VS100_L2_parameters>();
                obj.VS200_A = t[7].cast<VS200_L2_parameters>();
                obj.VS200_B = t[8].cast<VS200_L2_parameters>();
                obj.VS200_C = t[9].cast<VS200_L2_parameters>();
                obj.VS300 = t[10].cast<VS300_L2_parameters>();

                return obj;
            }
    ));

    //---------------------------------
    //     ES500 Aggregator Structures
    //---------------------------------
    py::class_<ES500_aggregator_pev_charge_needs>(m, "ES500_aggregator_pev_charge_needs")
        .def(py::init<>())
        .def_readwrite("SE_id", &ES500_aggregator_pev_charge_needs::SE_id)
        .def_readwrite("departure_unix_time", &ES500_aggregator_pev_charge_needs::departure_unix_time)
        .def_readwrite("e3_charge_remain_kWh", &ES500_aggregator_pev_charge_needs::e3_charge_remain_kWh)
        .def_readwrite("e3_step_max_kWh", &ES500_aggregator_pev_charge_needs::e3_step_max_kWh)
        .def_readwrite("e3_step_target_kWh", &ES500_aggregator_pev_charge_needs::e3_step_target_kWh)
        .def_readwrite("min_remaining_charge_time_hrs", &ES500_aggregator_pev_charge_needs::min_remaining_charge_time_hrs)
        .def_readwrite("min_time_to_complete_entire_charge_hrs", &ES500_aggregator_pev_charge_needs::min_time_to_complete_entire_charge_hrs)
        .def_readwrite("remaining_park_time_hrs", &ES500_aggregator_pev_charge_needs::remaining_park_time_hrs)
        .def_readwrite("total_park_time_hrs", &ES500_aggregator_pev_charge_needs::total_park_time_hrs)
        .def(py::pickle(
            [](const ES500_aggregator_pev_charge_needs& obj)
            {  // __getstate__
                return py::make_tuple(obj.SE_id, obj.departure_unix_time, obj.e3_charge_remain_kWh, obj.e3_step_max_kWh, obj.e3_step_target_kWh, obj.min_remaining_charge_time_hrs, obj.min_time_to_complete_entire_charge_hrs, obj.remaining_park_time_hrs, obj.total_park_time_hrs);
            },
            [](py::tuple t)
            {  // __setstate__    
                ES500_aggregator_pev_charge_needs obj;
                obj.SE_id = t[0].cast<SupplyEquipmentId>();
                obj.departure_unix_time = t[1].cast<double>();
                obj.e3_charge_remain_kWh = t[2].cast<double>();
                obj.e3_step_max_kWh = t[3].cast<double>();
                obj.e3_step_target_kWh = t[4].cast<double>();
                obj.min_remaining_charge_time_hrs = t[5].cast<double>();
                obj.min_time_to_complete_entire_charge_hrs = t[6].cast<double>();
                obj.remaining_park_time_hrs = t[7].cast<double>();
                obj.total_park_time_hrs = t[8].cast<double>();

                return obj;
            }
    ));

    py::class_<ES500_aggregator_charging_needs>(m, "ES500_aggregator_charging_needs")
        .def(py::init<>())
        .def("is_empty", &ES500_aggregator_charging_needs::is_empty)
        .def_readwrite("next_aggregator_timestep_start_time", &ES500_aggregator_charging_needs::next_aggregator_timestep_start_time)
        .def_readwrite("pev_charge_needs", &ES500_aggregator_charging_needs::pev_charge_needs)
        .def(py::pickle(
            [](const ES500_aggregator_charging_needs& obj)
            {  // __getstate__
                return py::make_tuple(obj.next_aggregator_timestep_start_time, obj.pev_charge_needs);
            },
            [](py::tuple t)
            {  // __setstate__    
                ES500_aggregator_charging_needs obj;
                obj.next_aggregator_timestep_start_time = t[0].cast<double>();

                for (auto x : t[1])
                    obj.pev_charge_needs.push_back(x.cast<ES500_aggregator_pev_charge_needs>());

                return obj;
            }
    ));

    py::class_<ES500_aggregator_e_step_setpoints>(m, "ES500_aggregator_e_step_setpoints")
        .def(py::init<>())
        .def(py::init<double, std::vector<SupplyEquipmentId>, std::vector<double>, std::vector<double> >())
        .def("is_empty", &ES500_aggregator_e_step_setpoints::is_empty)
        .def_readwrite("next_aggregator_timestep_start_time", &ES500_aggregator_e_step_setpoints::next_aggregator_timestep_start_time)
        .def_readwrite("SE_id", &ES500_aggregator_e_step_setpoints::SE_id)
        .def_readwrite("e3_step_kWh", &ES500_aggregator_e_step_setpoints::e3_step_kWh)
        .def_readwrite("charge_progression", &ES500_aggregator_e_step_setpoints::charge_progression)
        .def(py::pickle(
            [](const ES500_aggregator_e_step_setpoints& obj)
            {  // __getstate__
                return py::make_tuple(obj.next_aggregator_timestep_start_time, obj.SE_id, obj.e3_step_kWh, obj.charge_progression);
            },
            [](py::tuple t)
            {  // __setstate__
                ES500_aggregator_e_step_setpoints obj;
                obj.next_aggregator_timestep_start_time = t[0].cast<double>();

                for (auto x : t[1])
                    obj.SE_id.push_back(x.cast<SupplyEquipmentId>());

                for (auto x : t[2])
                    obj.e3_step_kWh.push_back(x.cast<double>());

                for (auto x : t[3])
                    obj.charge_progression.push_back(x.cast<double>());

                return obj;
            }
    ));

    py::class_<ES500_aggregator_charging_forecast>(m, "ES500_aggregator_charging_forecast")
        .def(py::init<>())
        .def_readwrite("arrival_unix_time", &ES500_aggregator_charging_forecast::arrival_unix_time)
        .def_readwrite("departure_unix_time", &ES500_aggregator_charging_forecast::departure_unix_time)
        .def_readwrite("e3_charge_remain_kWh", &ES500_aggregator_charging_forecast::e3_charge_remain_kWh)
        .def_readwrite("e3_step_max_kWh", &ES500_aggregator_charging_forecast::e3_step_max_kWh)
        .def(py::pickle(
            [](const ES500_aggregator_charging_forecast& obj)
            {  // __getstate__
                return py::make_tuple(obj.arrival_unix_time, obj.departure_unix_time, obj.e3_charge_remain_kWh, obj.e3_step_max_kWh);
            },
            [](py::tuple t)
            {  // __setstate__   
                ES500_aggregator_charging_forecast obj;

                for (auto x : t[0])
                    obj.arrival_unix_time.push_back(x.cast<double>());

                for (auto x : t[1])
                    obj.departure_unix_time.push_back(x.cast<double>());

                for (auto x : t[2])
                    obj.e3_charge_remain_kWh.push_back(x.cast<double>());

                for (auto x : t[3])
                    obj.e3_step_max_kWh.push_back(x.cast<double>());

                return obj;
            }
    ));

    py::class_<ES500_aggregator_obj_fun_constraints>(m, "ES500_aggregator_obj_fun_constraints")
        .def(py::init<>())
        .def_readwrite("E_cumEnergy_ALAP_kWh", &ES500_aggregator_obj_fun_constraints::E_cumEnergy_ALAP_kWh)
        .def_readwrite("E_cumEnergy_ASAP_kWh", &ES500_aggregator_obj_fun_constraints::E_cumEnergy_ASAP_kWh)
        .def_readwrite("E_energy_ALAP_kWh", &ES500_aggregator_obj_fun_constraints::E_energy_ALAP_kWh)
        .def_readwrite("E_energy_ASAP_kWh", &ES500_aggregator_obj_fun_constraints::E_energy_ASAP_kWh)
        .def_readwrite("E_step_ALAP", &ES500_aggregator_obj_fun_constraints::E_step_ALAP)
        .def_readwrite("canSolve_aka_pev_charging_in_prediction_window", &ES500_aggregator_obj_fun_constraints::canSolve_aka_pev_charging_in_prediction_window)
        .def(py::pickle(
            [](const ES500_aggregator_obj_fun_constraints& obj)
            {  // __getstate__
                return py::make_tuple(obj.E_cumEnergy_ALAP_kWh, obj.E_cumEnergy_ASAP_kWh, obj.E_energy_ALAP_kWh, obj.E_energy_ASAP_kWh, obj.E_step_ALAP, obj.canSolve_aka_pev_charging_in_prediction_window);
            },
            [](py::tuple t)
            {  // __setstate__    
                ES500_aggregator_obj_fun_constraints obj;

                for (auto x : t[0])
                    obj.E_cumEnergy_ALAP_kWh.push_back(x.cast<double>());

                for (auto x : t[1])
                    obj.E_cumEnergy_ASAP_kWh.push_back(x.cast<double>());

                for (auto x : t[2])
                    obj.E_energy_ALAP_kWh.push_back(x.cast<double>());

                for (auto x : t[3])
                    obj.E_energy_ASAP_kWh.push_back(x.cast<double>());

                for (auto x : t[4])
                    obj.E_step_ALAP.push_back(x.cast<double>());

                obj.canSolve_aka_pev_charging_in_prediction_window = t[5].cast<bool>();

                return obj;
            }
    ));

    py::class_<ES500_charge_cycling_control_boundary_point>(m, "ES500_charge_cycling_control_boundary_point")
        .def(py::init<>())
        .def(py::init<double, double>())
        .def_readwrite("cycling_magnitude", &ES500_charge_cycling_control_boundary_point::cycling_magnitude)
        .def_readwrite("cycling_vs_ramping", &ES500_charge_cycling_control_boundary_point::cycling_vs_ramping)
        .def(py::pickle(
            [](const ES500_charge_cycling_control_boundary_point& obj)
            {  // __getstate__
                return py::make_tuple(obj.cycling_magnitude, obj.cycling_vs_ramping);
            },
            [](py::tuple t)
            {  // __setstate__
                ES500_charge_cycling_control_boundary_point obj;
                obj.cycling_magnitude = t[0].cast<double>();
                obj.cycling_vs_ramping = t[1].cast<double>();
                return obj;
            }
    ));

    py::class_<ES500_stop_charge_cycling_decision_parameters>(m, "ES500_stop_charge_cycling_decision_parameters")
        .def(py::init<>())
        .def_readwrite("next_aggregator_timestep_start_time", &ES500_stop_charge_cycling_decision_parameters::next_aggregator_timestep_start_time)
        .def_readwrite("iteration", &ES500_stop_charge_cycling_decision_parameters::iteration)
        .def_readwrite("is_last_iteration", &ES500_stop_charge_cycling_decision_parameters::is_last_iteration)
        .def_readwrite("off_to_on_nrg_kWh", &ES500_stop_charge_cycling_decision_parameters::off_to_on_nrg_kWh)
        .def_readwrite("on_to_off_nrg_kWh", &ES500_stop_charge_cycling_decision_parameters::on_to_off_nrg_kWh)
        .def_readwrite("total_on_nrg_kWh", &ES500_stop_charge_cycling_decision_parameters::total_on_nrg_kWh)
        .def_readwrite("cycling_vs_ramping", &ES500_stop_charge_cycling_decision_parameters::cycling_vs_ramping)
        .def_readwrite("cycling_magnitude", &ES500_stop_charge_cycling_decision_parameters::cycling_magnitude)
        .def_readwrite("delta_energy_kWh", &ES500_stop_charge_cycling_decision_parameters::delta_energy_kWh)
        .def(py::pickle(
            [](const ES500_stop_charge_cycling_decision_parameters& obj)
            {  // __getstate__
                return py::make_tuple(obj.next_aggregator_timestep_start_time, obj.iteration, obj.is_last_iteration, obj.off_to_on_nrg_kWh, obj.on_to_off_nrg_kWh, obj.total_on_nrg_kWh, obj.cycling_vs_ramping, obj.cycling_magnitude, obj.delta_energy_kWh);
            },
            [](py::tuple t)
            {  // __setstate__
                ES500_stop_charge_cycling_decision_parameters obj;
                obj.next_aggregator_timestep_start_time = t[0].cast<double>();
                obj.iteration = t[1].cast<int>();
                obj.is_last_iteration = t[2].cast<bool>();
                obj.off_to_on_nrg_kWh = t[3].cast<double>();
                obj.on_to_off_nrg_kWh = t[4].cast<double>();
                obj.total_on_nrg_kWh = t[5].cast<double>();
                obj.cycling_vs_ramping = t[6].cast<double>();
                obj.cycling_magnitude = t[7].cast<double>();
                obj.delta_energy_kWh = t[8].cast<double>();
                return obj;
            }
    ));

    //---------------------------------
    //     PEV Ramping Parameters
    //---------------------------------

    py::class_<pev_charge_ramping>(m, "pev_charge_ramping")
        .def(py::init<>())
        .def(py::init<double, double, double, double, double, double, double, double>())
        .def_readwrite("off_to_on_delay_sec", &pev_charge_ramping::off_to_on_delay_sec)
        .def_readwrite("off_to_on_kW_per_sec", &pev_charge_ramping::off_to_on_kW_per_sec)
        .def_readwrite("on_to_off_delay_sec", &pev_charge_ramping::on_to_off_delay_sec)
        .def_readwrite("on_to_off_kW_per_sec", &pev_charge_ramping::on_to_off_kW_per_sec)
        .def_readwrite("ramp_up_delay_sec", &pev_charge_ramping::ramp_up_delay_sec)
        .def_readwrite("ramp_up_kW_per_sec", &pev_charge_ramping::ramp_up_kW_per_sec)
        .def_readwrite("ramp_down_delay_sec", &pev_charge_ramping::ramp_down_delay_sec)
        .def_readwrite("ramp_down_kW_per_sec", &pev_charge_ramping::ramp_down_kW_per_sec)
        .def(py::pickle(
            [](const pev_charge_ramping& obj)
            {  // __getstate__
                return py::make_tuple(obj.off_to_on_delay_sec, obj.off_to_on_kW_per_sec, obj.on_to_off_delay_sec, obj.on_to_off_kW_per_sec, obj.ramp_up_delay_sec, obj.ramp_up_kW_per_sec, obj.ramp_down_delay_sec, obj.ramp_down_kW_per_sec);
            },
            [](py::tuple t)
            {  // __setstate__
                pev_charge_ramping obj;
                obj.off_to_on_delay_sec = t[0].cast<double>();
                obj.off_to_on_kW_per_sec = t[1].cast<double>();
                obj.on_to_off_delay_sec = t[2].cast<bool>();
                obj.on_to_off_kW_per_sec = t[3].cast<double>();
                obj.ramp_up_delay_sec = t[4].cast<double>();
                obj.ramp_up_kW_per_sec = t[5].cast<double>();
                obj.ramp_down_delay_sec = t[6].cast<double>();
                obj.ramp_down_kW_per_sec = t[7].cast<double>();
                return obj;
            }
    ));

    py::class_<pev_charge_ramping_workaround>(m, "pev_charge_ramping_workaround")
        .def(py::init<>())
        .def_readwrite("pev_charge_ramping_obj", &pev_charge_ramping_workaround::pev_charge_ramping_obj)
        .def_readwrite("pev_type", &pev_charge_ramping_workaround::pev_type)
        .def_readwrite("SE_type", &pev_charge_ramping_workaround::SE_type)
        .def(py::pickle(
            [](const pev_charge_ramping_workaround& obj)
            {  // __getstate__
                return py::make_tuple(obj.pev_charge_ramping_obj, obj.pev_type, obj.SE_type);
            },
            [](py::tuple t)
            {  // __setstate__
                pev_charge_ramping_workaround obj;
                obj.pev_charge_ramping_obj = t[0].cast<pev_charge_ramping>();
                obj.pev_type = t[1].cast<EV_type>();
                obj.SE_type = t[2].cast<EVSE_type>();
                return obj;
            }
    ));
}
