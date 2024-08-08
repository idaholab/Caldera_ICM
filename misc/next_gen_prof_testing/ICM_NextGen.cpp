#include "Aux_interface.h"
#include <filesystem>
#include <sstream>

double find_c_rate_input_given_power(const double actual_bat_size_kWh, const double max_power_kW, const std::string &bat_chem)
{

    std::vector<double> crate_vec = {0, 1, 2, 3, 4, 5, 6};
    std::vector<double> LMO_power_vec = {0, 1.27, 2.42, 3.63, 3.63, 3.63, 3.63};
    std::vector<double> NMC_power_vec = {0, 1.25, 2.42, 3.75, 3.75, 3.75, 3.75};
    std::vector<double> LTO_power_vec = {0, 1.13, 2.23, 3.36, 4.52, 5.63, 5.63};

    const std::pair<std::vector<double>, std::vector<double>> crate_vs_pu_power = [&]()
    {
        if (bat_chem == "LMO")
        {
            return std::make_pair(crate_vec, LMO_power_vec);
        }
        else if (bat_chem == "NMC")
        {
            return std::make_pair(crate_vec, NMC_power_vec);
        }
        else if (bat_chem == "LTO")
        {
            return std::make_pair(crate_vec, LTO_power_vec);
        }
        else
        {
            std::cout << "!!!!!!!!!!!!!!!!ERROR : bat_chem npt supported" << std::endl;
            return std::make_pair(std::vector<double>{}, std::vector<double>{});
        }
    }();

    double max_charge_time_hrs = actual_bat_size_kWh / max_power_kW;
    double max_c_rate = 1 / max_charge_time_hrs;

    std::vector<crate> c_rate_arr = crate_vs_pu_power.first;
    std::vector<power> power_arr = crate_vs_pu_power.second;

    double weight_0, weight_1, c_rate;

    for (int i = 1; i < power_arr.size(); i++)
    {
        if ((max_c_rate >= power_arr[i - 1]) && max_c_rate < power_arr[i])
        {
            weight_0 = (power_arr[i] - max_c_rate) / (power_arr[i] - power_arr[i - 1]);
            weight_1 = 1 - weight_0;

            c_rate = (weight_0)*c_rate_arr[i - 1] + (weight_1)*c_rate_arr[i];
            return c_rate;
        }
    }
    std::cout << "Error: max_c_rate is out-of-bounds." << std::endl;
    return 0.0;
}

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
    std::string header = "time | hrs,SOC | ,P1 | kW,P2 | kW,P3 | kW,Q3 | kVAR,temperature | ºC\n";

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

    // std::vector<std::string> supply_equipment =
    //     {
    //         "dcfc_50",
    //         "xfc_150",
    //         "xfc_350",
    //     };

    // std::vector<std::string> vehicles =
    //     {
    //         "ngp_audi_etron_55_quattro",
    //         "ngp_hyundai_ioniq_5_longrange_awd",
    //         "ngp_genesis_gv60",
    //         "ngp_porsche_taycan_plus",
    //     };

    std::vector<std::string> supply_equipment =
        {
            "xfc_350",
        };

    std::vector<std::string> vehicles =
        {
            "ngp_hyundai_ioniq_5_longrange_awd_100",
            "ngp_hyundai_ioniq_5_longrange_awd_90",
            "ngp_hyundai_ioniq_5_longrange_awd_80",
            "ngp_hyundai_ioniq_5_longrange_awd_70",
            "ngp_hyundai_ioniq_5_longrange_awd_60",
            "ngp_hyundai_ioniq_5_longrange_awd_50",
            "ngp_hyundai_ioniq_5_longrange_awd_40",
            "ngp_hyundai_ioniq_5_longrange_awd_30",
            "ngp_hyundai_ioniq_5_longrange_awd_20",
            "ngp_hyundai_ioniq_5_longrange_awd_10"
        };

    for (std::string ev : vehicles)
    {
        for (std::string evse : supply_equipment)
        {
            return_code += build_entire_charge_profile_using_ICM(ev, evse, start_soc, end_soc);
        }
    }

    // An example to find the average c-rate to input in EV_inputs.csv file.
    // // The EV_input file actually takes average c-rate and not max c-rate.
    // double usable_bat_size_kWh = 74.0;
    // double actual_bat_size_kWh = 77.4;
    // double max_power_kW = 233;
    // std::string bat_chem = "NMC";
    // double crate = find_c_rate_input_given_power(actual_bat_size_kWh, max_power_kW, bat_chem);

    // std::cout << "Hyunda IONIQ 5 c_rate : " << crate << std::endl;
}