#include <iostream>

#include "load_EV_EVSE_inventory.h"
#include "factory_charging_transitions.h"
#include "factory_EV_charge_model.h"
#include "factory_SOC_vs_P2.h"

#include <fstream>


int main()
{
	load_EV_EVSE_inventory load_inventory{ "./inputs" };
	const EV_EVSE_inventory& inventory = load_inventory.get_EV_EVSE_inventory();

	EV_ramping_map custom_EV_ramping;
	EV_EVSE_ramping_map custom_EV_EVSE_ramping;
	bool model_stochastic_battery_degregation = false;

	factory_EV_charge_model charge_model_factory{ inventory, custom_EV_ramping, custom_EV_EVSE_ramping, model_stochastic_battery_degregation };

	std::vector<std::pair<EV_type, EVSE_type> > EV_EVSE_pair;

	EV_EVSE_pair.push_back(std::make_pair("L2_7200", "bev150_ld1_50kW"));
	EV_EVSE_pair.push_back(std::make_pair("dcfc_50", "bev150_ld1_50kW"));
	EV_EVSE_pair.push_back(std::make_pair("xfc_350", "bev150_ld1_50kW"));

	EV_EVSE_pair.push_back(std::make_pair("L2_7200", "bev250_ld2_300kW"));
	EV_EVSE_pair.push_back(std::make_pair("dcfc_50", "bev250_ld2_300kW"));
	EV_EVSE_pair.push_back(std::make_pair("xfc_350", "bev250_ld2_300kW"));

	EV_EVSE_pair.push_back(std::make_pair("L2_7200", "bev300_575kW"));
	EV_EVSE_pair.push_back(std::make_pair("dcfc_50", "bev300_575kW"));
	EV_EVSE_pair.push_back(std::make_pair("xfc_350", "bev300_575kW"));

	// Build charge event

	bool charge_has_completed;
	battery_state bat_state;
	double soc_t1;
	double P1_kW;
	double P2_kW;

	control_strategy_enums cs;
	cs.inverter_model_supports_Qsetpoint = false;
	cs.ES_control_strategy = L2_control_strategies_enum::NA;
	cs.VS_control_strategy = L2_control_strategies_enum::NA;
	cs.ext_control_strategy = "";

	stop_charging_criteria scc;

	int charge_event_id = 1;
	int SE_group_id = 1;
	SE_id_type  SE_id = 1;
	vehicle_id_type vehicle_id = 1;
	double arrival_unix_time = 1*3600;
	double departure_unix_time = 4*3600;
	double arrival_SOC = 0;
	double departure_SOC = 100;
	stop_charging_criteria stop_charge = scc;
	control_strategy_enums control_enums = cs;

	std::string filename, header, data, seperator, new_line;
	std::ofstream file_handle;

	for (std::pair<EVSE_type, EV_type> elem : EV_EVSE_pair)
	{
		EVSE_type supply_equipment_type = elem.first;
		EV_type vehicle_type = elem.second;
		
		charge_event_data event(charge_event_id, SE_group_id, SE_id, vehicle_id, vehicle_type, arrival_unix_time, departure_unix_time, arrival_SOC, departure_SOC, scc, cs);
		
		vehicle_charge_model* model = charge_model_factory.alloc_get_EV_charge_model(event, supply_equipment_type, 1000000);
		model->set_target_P2_kW(10000000);

		filename = supply_equipment_type + "_" + vehicle_type + ".csv";

		file_handle = std::ofstream(filename);

		seperator = ", ";
		new_line = "\n";

		header = "simulation_time_hrs, soc_t1, P1_kW, P2_kW";
		header += new_line;

		data = "";

		double simulation_timestep_sec = 60;
		double simulation_start_unix_time = arrival_unix_time - simulation_timestep_sec;
		double simulation_end_unix_time = departure_unix_time;

		double previous_unix_time = simulation_start_unix_time - simulation_timestep_sec;
		double current_unix_time = simulation_start_unix_time;


		while (current_unix_time <= simulation_end_unix_time)
		{
			model->get_next(previous_unix_time, current_unix_time, 1.0, charge_has_completed, bat_state);

			soc_t1 = bat_state.soc_t1;
			P1_kW = bat_state.P1_kW;
			P2_kW = bat_state.P2_kW;

			data += std::to_string(current_unix_time / 3600.0) + seperator;
			data += std::to_string(soc_t1) + seperator;
			data += std::to_string(P1_kW) + seperator;
			data += std::to_string(P2_kW) + new_line;

			previous_unix_time = current_unix_time;
			current_unix_time += simulation_timestep_sec;
		}

		file_handle << header;
		file_handle << data;
		file_handle.close();
	}

	return 0;
}