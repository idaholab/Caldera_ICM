#include "Aux_interface.h"
#include <filesystem>


int build_entire_charge_profile_using_ICM()
{

	std::string path_to_inputs = "./inputs";

	//######################################################
	//# 	Build Charge Profile Given Start and End SOC
	//######################################################

	double L1_timestep_sec = 1;								// Timestep for Level 1 charger
	double L2_timestep_sec = 1;								// Timestep for Level 2 charger
	double HPC_timestep_sec = 1;							// Timestep for High Power charger
	EV_ramping_map ramping_by_pevType_only{};				// Custom Ramping
	EV_EVSE_ramping_map ramping_by_pevType_seType{};		// Custom Ramping

	CP_interface_v2 ICM_v2{ 
		path_to_inputs, 
		L1_timestep_sec, 
		L2_timestep_sec, 
		HPC_timestep_sec, 
		ramping_by_pevType_only, 
		ramping_by_pevType_seType 
	};

	std::string pev_type = "bev250_350kW";
	std::string SE_type = "xfc_350";
	double start_soc = 10;
	double end_soc = 90;

	all_charge_profile_data all_profile_data = ICM_v2.get_all_charge_profile_data(start_soc, end_soc, pev_type, SE_type);

	double start_time_sec = 0;

	// Write output
	// build the files header
	std::string header = "time | hrs,SOC | ,P1 | kW, P2 | kW, P3 | kW, Q3 | kVAR\n";

	std::string data = "";
	
	for (int i = 0; i < all_profile_data.soc.size(); i++)
	{
		data += std::to_string((start_time_sec + i* all_profile_data.timestep_sec)/3600.0) + ",";
		data += std::to_string(all_profile_data.soc[i]) + ",";
		data += std::to_string(all_profile_data.P1_kW[i]) + ",";
		data += std::to_string(all_profile_data.P2_kW[i]) + ",";
		data += std::to_string(all_profile_data.P3_kW[i]) + ",";
		data += std::to_string(all_profile_data.Q3_kVAR[i]) + "\n";

	}

	std::ofstream f_out;
	f_out.open("./outputs/ICM_Aux_output.csv");
	f_out << header << data;
	f_out.close();

	return 0;
}

int estimate_charging_parameters_using_ICM()
{
	std::string path_to_inputs = "./inputs";
	CP_interface ICM_v1{ path_to_inputs, false };

	double setpoint_P3kW = 1000;
	std::string pev_type = "bev250_350kW";
	std::string SE_type = "xfc_350";


	double startSOC, endSOC, charge_time_hrs;
	std::vector<double> endSOC_vec, charge_time_hrs_vec;
	//----------------------------------------
	// find_result_given_startSOC_and_endSOC
	//----------------------------------------
	
	startSOC = 20;
	endSOC = 85;
	pev_charge_profile_result X0 = ICM_v1.find_result_given_startSOC_and_endSOC(pev_type, SE_type, setpoint_P3kW, startSOC, endSOC);

	std::cout << "-----------------------------------" << "\n";
	std::cout << "find_result_given_startSOC_and_endSOC" << "\n";
	std::cout << "------------------------------------" << "\n";
	std::cout << "startSOC: " << startSOC << " endSOC: " << endSOC << "\n";
	std::cout << "soc_increase: " << X0.soc_increase << " total_time_hrs: " << X0.total_charge_time_hrs << " incr_time_hrs : " << X0.incremental_chage_time_hrs << "\n";
	std::cout << "\n" << "\n";


	//-------------------------------------------
	// find_result_given_startSOC_and_chargeTime
	//-------------------------------------------
	startSOC = 20;
	charge_time_hrs = 15.0 / 60.0;
	pev_charge_profile_result X1 = ICM_v1.find_result_given_startSOC_and_chargeTime(pev_type, SE_type, setpoint_P3kW, startSOC, charge_time_hrs);

	std::cout << "-----------------------------------" << "\n";
	std::cout << "find_result_given_startSOC_and_chargeTime" << "\n";
	std::cout << "------------------------------------" << "\n";
	std::cout << "startSOC: " << startSOC << " charge_time_hrs: " << charge_time_hrs << "\n";
	std::cout << "soc_increase: " << X1.soc_increase << " total_time_hrs: " << X1.total_charge_time_hrs << " incr_time_hrs : " << X1.incremental_chage_time_hrs << "\n";
	std::cout << "\n" << "\n";


	//------------------------------------------------
	// find_chargeProfile_given_startSOC_and_endSOCs
	//------------------------------------------------
	startSOC = 20;
	endSOC_vec = { 30, 40, 50, 60, 70, 80, 90, 100 };
	std::vector<pev_charge_profile_result> X2 = ICM_v1.find_chargeProfile_given_startSOC_and_endSOCs(pev_type, SE_type, setpoint_P3kW, startSOC, endSOC_vec);

	std::cout << "-----------------------------------" << "\n";
	std::cout << "find_chargeProfile_given_startSOC_and_endSOCs" << "\n";
	std::cout << "------------------------------------" << "\n";
	std::cout << "startSOC: " << startSOC << " endSOCs: ";
	for ( double endSOC : endSOC_vec )
	{
		std::cout << endSOC << " ";
	}
	std::cout << "\n";
	for (pev_charge_profile_result x : X2)
	{
		std::cout << "soc_increase: " << x.soc_increase << " total_time_hrs: " << x.total_charge_time_hrs << " incr_time_hrs : " << x.incremental_chage_time_hrs << "\n";
	}
	std::cout << "\n" << "\n";
	

	//------------------------------------------------
	// find_chargeProfile_given_startSOC_and_chargeTimes
	//------------------------------------------------
	startSOC = 20;
	charge_time_hrs_vec = { 5 / 60, 10 / 60, 15 / 60, 20 / 60, 25 / 60, 30 / 60, 35 / 60, 40 / 60, 45 / 60, 50 / 60, 60 / 60 };
	std::vector<pev_charge_profile_result> X3 = ICM_v1.find_chargeProfile_given_startSOC_and_chargeTimes(pev_type, SE_type, setpoint_P3kW, startSOC, charge_time_hrs_vec);
	
	std::cout << "-----------------------------------" << "\n";
	std::cout << "find_chargeProfile_given_startSOC_and_chargeTimes" << "\n";
	std::cout << "------------------------------------" << "\n";
	std::cout << "startSOC: " << startSOC << " charge_time_hrs: ";
	for (double charge_time_hrs : charge_time_hrs_vec)
	{
		std::cout << charge_time_hrs << " ";
	}
	std::cout << "\n";
	for (pev_charge_profile_result x : X2)
	{
		std::cout << "soc_increase: " << x.soc_increase << " total_time_hrs: " << x.total_charge_time_hrs << " incr_time_hrs : " << x.incremental_chage_time_hrs << "\n";
	}
	std::cout << "\n" << "\n";

	return 0;
}

int main()
{
	int return_code = 0;
	return_code += build_entire_charge_profile_using_ICM();
	return_code += estimate_charging_parameters_using_ICM();

	return 0;
}