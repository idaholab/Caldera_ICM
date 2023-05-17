
#include "ICM_interface.h"
#include "datatypes_global.h"
#include "datatypes_global_SE_EV_definitions.h"


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;


PYBIND11_MODULE(Caldera_ICM, m)
{
    //=======================================================    
    //======================================================= 
    //    Functions for class:  interface_to_SE_groups 
    //======================================================= 
    //======================================================= 
    
    py::class_<interface_to_SE_groups>(m, "interface_to_SE_groups")
        .def(py::init<>())
        .def("initialize", &interface_to_SE_groups::initialize)
        //.def("initialize_infrastructure", &interface_to_SE_groups::initialize_infrastructure)
        //.def("initialize_baseLD_forecaster", &interface_to_SE_groups::initialize_baseLD_forecaster)
        //.def("initialize_L2_control_strategy_parameters", &interface_to_SE_groups::initialize_L2_control_strategy_parameters)
        .def("add_charge_events", &interface_to_SE_groups::add_charge_events)
        .def("add_charge_events_by_SE_group", &interface_to_SE_groups::add_charge_events_by_SE_group)
        .def("stop_active_charge_events", &interface_to_SE_groups::stop_active_charge_events)
        //.def("set_ensure_pev_charge_needs_met_for_ext_control_strategy", &interface_to_SE_groups::set_ensure_pev_charge_needs_met_for_ext_control_strategy)        
        //.def("get_SE_charge_profile_forecast_akW", &interface_to_SE_groups::get_SE_charge_profile_forecast_akW)
        //.def("get_SE_group_charge_profile_forecast_akW", &interface_to_SE_groups::get_SE_group_charge_profile_forecast_akW)
        .def("get_charging_power", &interface_to_SE_groups::get_charging_power)
        //.def("get_SE_power", &interface_to_SE_groups::get_SE_power)
        .def("set_PQ_setpoints", &interface_to_SE_groups::set_PQ_setpoints)
        //.def("get_completed_CE", &interface_to_SE_groups::get_completed_CE)
        //.def("get_FICE_by_extCS", &interface_to_SE_groups::get_FICE_by_extCS)
        //.def("get_FICE_by_SE_groups", &interface_to_SE_groups::get_FICE_by_SE_groups)
        //.def("get_FICE_by_SEids", &interface_to_SE_groups::get_FICE_by_SEids)
        .def("get_all_active_CEs", &interface_to_SE_groups::get_all_active_CEs)
        .def("get_active_CEs_by_extCS", &interface_to_SE_groups::get_active_CEs_by_extCS)        
        .def("get_active_CEs_by_SE_groups", &interface_to_SE_groups::get_active_CEs_by_SE_groups)
        .def("get_active_CEs_by_SEids", &interface_to_SE_groups::get_active_CEs_by_SEids)
        .def("ES500_get_charging_needs", &interface_to_SE_groups::ES500_get_charging_needs)
        .def("ES500_set_energy_setpoints", &interface_to_SE_groups::ES500_set_energy_setpoints);
}

