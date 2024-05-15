#include <iostream>

#include "load_EV_EVSE_inventory.h"
#include "factory_charging_transitions.h"
#include "factory_EV_charge_model.h"
#include "factory_SOC_vs_P2.h"

#include <fstream>


int main()
{
	load_EV_EVSE_inventory load_inventory{ "..\\inputs\\DirectXFC" };
	const EV_EVSE_inventory& inventory = load_inventory.get_EV_EVSE_inventory();

	//factory_SOC_vs_P2 soc_vs_p2(inventory);

	//soc_vs_p2.get_SOC_vs_P2_curves("XFC250_300kW", "L2_9600");
	
	std::cout << inventory << std::endl;
	EV_ramping_map custom_EV_ramping;
	EV_EVSE_ramping_map custom_EV_EVSE_ramping;
	bool model_stochastic_battery_degregation = false;

	factory_EV_charge_model charge_model_factory{ inventory, custom_EV_ramping, custom_EV_EVSE_ramping, model_stochastic_battery_degregation };

	//charge_model_factory.write_charge_profile("./");
	
	// Build charge event

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
	EV_type vehicle_type = "bev250_350kW";
	double arrival_unix_time = 1*3600;
	double departure_unix_time = 4*3600;
	double arrival_SOC = 0;
	double departure_SOC = 100;
	stop_charging_criteria stop_charge = scc;
	control_strategy_enums control_enums = cs;

	charge_event_data event(charge_event_id, SE_group_id, SE_id, vehicle_id, vehicle_type, arrival_unix_time, departure_unix_time, arrival_SOC, departure_SOC, scc, cs);

	vehicle_charge_model* DCFC_model = charge_model_factory.alloc_get_EV_charge_model(event, "dcfc_50", 1000000);

	// Run the vehicle charge model

	bool charge_has_completed;
	battery_state bat_state;
	double soc_t1;
	double P1_kW;
	double P2_kW;
	double time_step_duration_hrs;
	double E1_energy_to_target_soc_kWh;
	double min_time_to_target_soc_hrs;

	std::string filename, header, data, seperator, new_line;
	std::ofstream file_handle;

	filename = "./dcfc_vehicle_charge_model.csv";

	file_handle = std::ofstream(filename);
	
	seperator = ", ";
	new_line = "\n";

	header = "simulation_time_hrs, soc_t1, P1_kW, P2_kW, time_step_duration_hrs, E1_energy_to_target_soc_kWh, min_time_to_target_soc_hrs";
	header += new_line;

	data = "";

	double simulation_timestep_sec = 1;
	double simulation_start_unix_time = 1 * 3600 - simulation_timestep_sec;
	double simulation_end_unix_time = 4 * 3600;

	double previous_unix_time = simulation_start_unix_time - simulation_timestep_sec;
	double current_unix_time = simulation_start_unix_time;
	while (current_unix_time <= simulation_end_unix_time)
	{
		if (current_unix_time >= simulation_start_unix_time || current_unix_time < simulation_end_unix_time)
		{
			DCFC_model->set_target_P2_kW(10000);
		}
		else
		{
			DCFC_model->set_target_P2_kW(0);
		}

		DCFC_model->get_next(previous_unix_time, current_unix_time, 1.0, charge_has_completed, bat_state);
		
		soc_t1 = bat_state.soc_t1;
		P1_kW = bat_state.P1_kW;
		P2_kW = bat_state.P2_kW;
		time_step_duration_hrs = bat_state.time_step_duration_hrs;
		E1_energy_to_target_soc_kWh = bat_state.E1_energy_to_target_soc_kWh;
		min_time_to_target_soc_hrs = bat_state.min_time_to_target_soc_hrs;
		
		data += std::to_string(current_unix_time) + seperator;
		data += std::to_string(soc_t1) + seperator;
		data += std::to_string(P1_kW) + seperator;
		data += std::to_string(P2_kW) + seperator;
		data += std::to_string(time_step_duration_hrs) + seperator;
		data += std::to_string(E1_energy_to_target_soc_kWh) + seperator;
		data += std::to_string(min_time_to_target_soc_hrs) + new_line;

		current_unix_time += simulation_timestep_sec;
	}

	file_handle << header;
	file_handle << data;
	file_handle.close();

//	"L2_9600"


	return 0;
}