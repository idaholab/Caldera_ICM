#include "Aux_interface.h"
#include <filesystem>
#include <sstream>

int build_entire_charge_profile_using_ICM(std::string &pev_type, std::string &SE_type, double &start_soc, double &end_soc)
{

    // Define the path to the inputs directory which holds the files containing the EV and EVSE input parameters
    std::string path_to_inputs = "./inputs";

    // ######################################################
    // # 	Build Charge Profile Given Start and End SOC
    // ######################################################

    // Initialize the members of the CP_interface_v2 struct
    double L1_timestep_sec = 1;                      // Timestep for Level 1 charger
    double L2_timestep_sec = 1;                      // Timestep for Level 2 charger
    double HPC_timestep_sec = 1;                     // Timestep for High Power charger
    EV_ramping_map ramping_by_pevType_only{};        // Custom Ramping
    EV_EVSE_ramping_map ramping_by_pevType_seType{}; // Custom Ramping

    CP_interface_v2 ICM_v2{
        path_to_inputs,
        L1_timestep_sec,
        L2_timestep_sec,
        HPC_timestep_sec,
        ramping_by_pevType_only,
        ramping_by_pevType_seType};



    // Generate the charge profile data
    all_charge_profile_data all_profile_data = ICM_v2.get_all_charge_profile_data(start_soc, end_soc, pev_type, SE_type);

    double start_time_sec = 0;

    // Write output
    // build the files header
    std::string header = "time | hrs,SOC | ,P1 | kW,P2 | kW,P3 | kW,Q3 | kVAR\n";

    std::string data = "";

    // Loop over the time instances and append the state of the charge profile to the data
    for (int i = 0; i < all_profile_data.soc.size(); i++)
    {
        data += std::to_string((start_time_sec + i * all_profile_data.timestep_sec) / 3600.0) + ",";
        data += std::to_string(all_profile_data.soc[i]) + ",";
        data += std::to_string(all_profile_data.P1_kW[i]) + ",";
        data += std::to_string(all_profile_data.P2_kW[i]) + ",";
        data += std::to_string(all_profile_data.P3_kW[i]) + ",";
        data += std::to_string(all_profile_data.Q3_kVAR[i]) + "\n";
    }

    std::ostringstream oss;
    oss << "Simulation for charge curve configuration [ " << pev_type << " : " << SE_type << " ] complete" << std::endl;
    std::cout << oss.str();

    oss.str("");
    oss.clear();
    oss << "./outputs/" << pev_type << " " << SE_type << ".csv";

    std::string filepath = oss.str();
    std::cout << "Data saved to: " << filepath << std::endl;

    // Store the resultant data in an output file
    std::ofstream f_out;
    f_out.open(filepath);
    f_out << header << data;
    f_out.close();

    return 0;
}

int main()
{
    int return_code = 0;

    double start_soc = 10;
    double end_soc = 100;

    std::string supply_equipment[3] =
        {
            "dcfc_50",
            "xfc_150",
            "xfc_350",
        };

    std::string vehicles[4] =
        {
            "ngp_audi_etron_55_quattro",
            "ngp_hyundai_ioniq_5_longrange_awd",
            "ngp_genesis_gv60",
            "ngp_porsche_taycan_plus",
        };

    for (std::string ev : vehicles)
    {
        for (std::string evse : supply_equipment)
        {
            return_code += build_entire_charge_profile_using_ICM(ev, evse, start_soc, end_soc);
        }
    }
}