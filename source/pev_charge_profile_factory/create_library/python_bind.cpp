#include "pev_charge_profile_factory.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(charge_profile_factory, m)
{
	m.doc() = "Small module that generates charge profiles from data in an input file";

	py::enum_<pev_battery_chemistry>(m, "pev_battery_chemistry")
		.value("LTO", LTO)
		.value("LMO", LMO)
		.value("NMC", NMC);

	py::class_<line_segment>(m, "line_segment")
		.def(py::init<>())
		.def_readwrite("x_LB", &line_segment::x_LB)
		.def_readwrite("x_UB", &line_segment::x_UB)
		.def_readwrite("a", &line_segment::a)
		.def_readwrite("b", &line_segment::b);

	py::class_<bat_objfun_constraints>(m, "bat_objfun_constraints")
		.def(py::init<>())
		.def_readwrite("a", &bat_objfun_constraints::a)
		.def_readwrite("b", &bat_objfun_constraints::b);

	py::class_<pev_charge_profile_factory>(m, "pev_charge_profile_factory")
		.def(py::init<>())
		.def("get_dcfc_charge_profile", &pev_charge_profile_factory::get_dcfc_charge_profile)
		.def("get_L1_or_L2_charge_profile", &pev_charge_profile_factory::get_L1_or_L2_charge_profile)
		.def("get_constraints", &pev_charge_profile_factory::get_constraints);
							
}

