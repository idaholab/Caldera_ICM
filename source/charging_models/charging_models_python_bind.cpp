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

PYBIND11_MODULE(Caldera_models, m)
{
	//--------------------------------------------
	//          EV_characteristics
	//--------------------------------------------

	py::enum_<battery_chemistry>(m, "battery_chemistry")
		.value("LTO", battery_chemistry::LTO)
		.value("LMO", battery_chemistry::LMO)
		.value("NMC", battery_chemistry::NMC);

	py::class_<EV_characteristics>(m, "EV_characteristics")
		.def(py::init<const EV_type&, const battery_chemistry&, const double&, const double&, const double&, const double&, const bool&, const double&, const double& >())
		.def("get_charge_profile_peak_power_W_per_Wh", &EV_characteristics::get_charge_profile_peak_power_W_per_Wh)
		.def("get_type", &EV_characteristics::get_type)
		.def("get_chemistry", &EV_characteristics::get_chemistry)
		.def("get_usable_battery_size_kWh", &EV_characteristics::get_usable_battery_size_kWh)
		.def("get_range_miles", &EV_characteristics::get_range_miles)
		.def("get_efficiency_Wh_per_mile", &EV_characteristics::get_efficiency_Wh_per_mile)
		.def("get_AC_charge_rate_kW", &EV_characteristics::get_AC_charge_rate_kW)
		.def("get_DCFC_capable", &EV_characteristics::get_DCFC_capable)
		.def("get_max_c_rate", &EV_characteristics::get_max_c_rate)
		.def("get_pack_voltage_at_peak_power_V", &EV_characteristics::get_pack_voltage_at_peak_power_V)
		.def("get_battery_size_kWh", &EV_characteristics::get_battery_size_kWh)
		.def("get_battery_size_Ah_1C", &EV_characteristics::get_battery_size_Ah_1C)
		.def("get_battery_size_with_stochastic_degradation_kWh", &EV_characteristics::get_battery_size_with_stochastic_degradation_kWh);

	//--------------------------------------------
	//          EVSE_characteristics
	//--------------------------------------------

	py::enum_<EVSE_level>(m, "EVSE_level")
		.value("L1", EVSE_level::L1)
		.value("L2", EVSE_level::L2)
		.value("DCFC", EVSE_level::DCFC);

	py::enum_<EVSE_phase>(m, "EVSE_phase")
		.value("singlephase", EVSE_phase::singlephase)
		.value("threephase", EVSE_phase::threephase);

	py::class_<EVSE_characteristics>(m, "EVSE_characteristics")
		.def(py::init<const EVSE_type&, const EVSE_level&, const EVSE_phase&, const double&, const double&, const double&, const double&, const double& >())
		.def("get_type", &EVSE_characteristics::get_type)
		.def("get_level", &EVSE_characteristics::get_level)
		.def("get_phase", &EVSE_characteristics::get_phase)
		.def("get_power_limit_kW", &EVSE_characteristics::get_power_limit_kW)
		.def("get_volatge_limit_V", &EVSE_characteristics::get_volatge_limit_V)
		.def("get_current_limit_A", &EVSE_characteristics::get_current_limit_A)
		.def("get_standby_real_power_kW", &EVSE_characteristics::get_standby_real_power_kW)
		.def("get_standby_reactive_power_kW", &EVSE_characteristics::get_standby_reactive_power_kW);

	//--------------------------------------------
	//          EV_EVSE_inventory
	//--------------------------------------------

	py::class_<pev_SE_pair>(m, "pev_SE_pair")
		.def(py::init<const EV_type&, const EVSE_type& >())
		.def_readwrite("ev_type", &pev_SE_pair::ev_type)
		.def_readwrite("se_type", &pev_SE_pair::se_type);

	py::class_<EV_EVSE_inventory>(m, "EV_EVSE_inventory")
		.def(py::init<const EV_inventory&, const EVSE_inventory& >())
		.def("get_EV_inventory", &EV_EVSE_inventory::get_EV_inventory)
		.def("get_EVSE_inventory", &EV_EVSE_inventory::get_EVSE_inventory)
		.def("get_all_EVs", &EV_EVSE_inventory::get_all_EVs)
		.def("get_all_EVSEs", &EV_EVSE_inventory::get_all_EVSEs)
		.def("get_default_EV", &EV_EVSE_inventory::get_default_EV)
		.def("get_default_EVSE", &EV_EVSE_inventory::get_default_EVSE)
		.def("get_all_compatible_pev_SE_combinations", &EV_EVSE_inventory::get_all_compatible_pev_SE_combinations);

}
