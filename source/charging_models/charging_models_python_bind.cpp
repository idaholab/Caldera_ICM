#include "datatypes_global.h"
#include "EVSE_characteristics.h"
#include "EV_characteristics.h"
#include "EV_EVSE_inventory.h"
#include "inputs.h"
#include "load_EV_EVSE_inventory.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// delete when done adding pickling functionality
#include <typeinfo>
#include <iostream>

namespace py = pybind11;

PYBIND11_MODULE(Caldera_models, m)
{
	//--------------------------------------------
	//       load_EV_EVSE_inventory
	//--------------------------------------------

	py::class_<load_EV_EVSE_inventory>(m, "load_EV_EVSE_inventory")
		.def(py::init< std::string >())
		.def("get_EV_EVSE_inventory", &load_EV_EVSE_inventory::get_EV_EVSE_inventory);

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
		.def("get_battery_size_with_stochastic_degradation_kWh", &EV_characteristics::get_battery_size_with_stochastic_degradation_kWh)
		.def(py::pickle(
			[](const EV_characteristics& obj)
			{  // __getstate__
				return py::make_tuple(obj.get_type(), obj.get_chemistry(), obj.get_usable_battery_size_kWh(), obj.get_range_miles(), obj.get_efficiency_Wh_per_mile(), obj.get_AC_charge_rate_kW(), obj.get_DCFC_capable(), obj.get_max_c_rate(), obj.get_pack_voltage_at_peak_power_V());
			},
			[](py::tuple t)
			{  // __setstate__
				EV_type type = t[0].cast<EV_type>();
				battery_chemistry chemistry = t[1].cast<battery_chemistry>();
				double usable_battery_size_kWh = t[2].cast<double>();
				double range_miles = t[3].cast<double>();
				double efficiency_Wh_per_mile = t[4].cast<double>();
				double AC_charge_rate_kW = t[5].cast<double>();
				bool DCFC_capable = t[6].cast<bool>();
				double max_c_rate = t[7].cast<double>();
				double pack_voltage_at_peak_power_V = t[8].cast<double>();

				EV_characteristics obj{ type, chemistry, usable_battery_size_kWh, range_miles, efficiency_Wh_per_mile, AC_charge_rate_kW, DCFC_capable, max_c_rate, pack_voltage_at_peak_power_V};
				return obj;
			}
		));
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
		.def("get_standby_reactive_power_kW", &EVSE_characteristics::get_standby_reactive_power_kW)
		.def(py::pickle(
			[](const EVSE_characteristics& obj)
			{  // __getstate__
				return py::make_tuple(obj.get_type(), obj.get_level(), obj.get_phase(), obj.get_power_limit_kW(), obj.get_volatge_limit_V(), obj.get_current_limit_A(), obj.get_standby_real_power_kW(), obj.get_standby_reactive_power_kW());
			},
			[](py::tuple t)
			{  // __setstate__
				EVSE_type type = t[0].cast<EVSE_type>();
				EVSE_level level = t[1].cast<EVSE_level>();
				EVSE_phase phase = t[2].cast<EVSE_phase>();
				double power_limit_kW = t[3].cast<double>();
				double volatge_limit_V = t[4].cast<double>();
				double current_limit_A = t[5].cast<double>();
				double standby_real_power_kW = t[6].cast<double>();
				double standby_reactive_power_kW = t[7].cast<double>();

				EVSE_characteristics obj{ type, level, phase, power_limit_kW, volatge_limit_V, current_limit_A, standby_real_power_kW, standby_reactive_power_kW };
				return obj;
			}
	));

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
		.def("get_all_compatible_pev_SE_combinations", &EV_EVSE_inventory::get_all_compatible_pev_SE_combinations)
		.def("pev_is_compatible_with_supply_equipment", &EV_EVSE_inventory::pev_is_compatible_with_supply_equipment)
		.def(py::pickle(
			[](const EV_EVSE_inventory& obj)
			{  // __getstate__
				return py::make_tuple(obj.get_EV_inventory(), obj.get_EVSE_inventory());
			},
			[](py::tuple t)
			{  // __setstate__

				// EV_inventory
				EV_inventory EV_inv = t[0].cast<EV_inventory>();

				// EVSE_inventory
				EVSE_inventory EVSE_inv = t[1].cast<EVSE_inventory>();
//				for (auto x : t[1].cast<EVSE_inventory>())
//					EVSE_inv.insert(x);

				EV_EVSE_inventory obj{ EV_inv, EVSE_inv };
				return obj;
			}
	));
}
