
#include "Aux_interface.h"
#include "datatypes_global.h"
#include "helper.h"              // get_value_from_normal_distribution, get_real_value_from_uniform_distribution, get_int_value_from_uniform_distribution, get_bernoulli_success

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;


PYBIND11_MODULE(Caldera_ICM_Aux, m)
{
//    m.def("get_pevType_batterySize_map", &get_pevType_batterySize_map);
    
    py::class_<CP_interface>(m, "CP_interface")
        .def(py::init<const std::string&>())
        .def(py::init<const std::string&, bool>())
        .def("get_size_of_CP_library_MB", &CP_interface::get_size_of_CP_library_MB)
        .def("get_CP_validation_data", &CP_interface::get_CP_validation_data)
        .def("get_charge_event_P3kW_limits", &CP_interface::get_charge_event_P3kW_limits)
        .def("get_P3kW_setpoints_of_charge_profiles", &CP_interface::get_P3kW_setpoints_of_charge_profiles)
        .def("find_result_given_startSOC_and_endSOC", &CP_interface::find_result_given_startSOC_and_endSOC)
        .def("find_result_given_startSOC_and_chargeTime", &CP_interface::find_result_given_startSOC_and_chargeTime)
        .def("find_chargeProfile_given_startSOC_and_endSOCs", &CP_interface::find_chargeProfile_given_startSOC_and_endSOCs)
        .def("find_chargeProfile_given_startSOC_and_chargeTimes", &CP_interface::find_chargeProfile_given_startSOC_and_chargeTimes)
        .def("USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile", &CP_interface::USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile);
    
    py::class_<CP_interface_v2>(m, "CP_interface_v2")
        .def(py::init<const std::string&>())
        .def(py::init<const std::string&, double, double, double >())
        .def("get_P3kW_charge_profile", &CP_interface_v2::get_P3kW_charge_profile)
        .def("get_timestep_of_prev_call_sec", &CP_interface_v2::get_timestep_of_prev_call_sec)
        .def("get_all_charge_profile_data", &CP_interface_v2::get_all_charge_profile_data)
        .def("create_charge_profile_from_model", &CP_interface_v2::create_charge_profile_from_model);

    py::class_<get_baseLD_forecast>(m, "get_baseLD_forecast")
        .def(py::init<>())
        .def(py::init<double, int, std::vector<double>, std::vector<double>, double>())
        .def("get_forecast_akW", &get_baseLD_forecast::get_forecast_akW);
    
    py::class_<get_value_from_normal_distribution>(m, "get_value_from_normal_distribution")
        .def(py::init<>())
        .def(py::init<int, double, double, double>())
        .def("get_value", &get_value_from_normal_distribution::get_value);
    
    py::class_<get_real_value_from_uniform_distribution>(m, "get_real_value_from_uniform_distribution")
        .def(py::init<>())
        .def(py::init<int, double, double>())
        .def("get_value", &get_real_value_from_uniform_distribution::get_value);
    
    py::class_<get_int_value_from_uniform_distribution>(m, "get_int_value_from_uniform_distribution")
        .def(py::init<>())
        .def(py::init<int, int, int>())
        .def("get_value", &get_int_value_from_uniform_distribution::get_value);
    
    py::class_<get_bernoulli_success>(m, "get_bernoulli_success")
        .def(py::init<>())
        .def(py::init<int, double>())
        .def("is_success", &get_bernoulli_success::is_success);
}


