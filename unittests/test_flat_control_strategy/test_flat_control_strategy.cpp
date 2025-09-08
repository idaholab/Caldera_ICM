#include "cxxopts.hpp"
#include <string>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <cmath>
#include <chrono>

#include "ICM_interface.h"

std::vector<SE_group_configuration> get_SE_group_configuration(const std::string& filename) {
    std::vector<SE_group_configuration> se_groups;
    std::unordered_map<int, std::vector<SE_configuration>> group_map;

    std::ifstream file(filename);
    std::string line, token;

    // Skip the header line
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        SE_configuration se_config;
        std::string se_id_str, se_group_str;

        std::getline(ss, se_id_str, ',');
        se_config.SE_id = std::stoi(se_id_str);

        std::getline(ss, token, ',');
        se_config.supply_equipment_type = token;
        std::getline(ss, token, ',');
        se_config.longitude = std::stod(token);

        std::getline(ss, token, ',');
        se_config.lattitude = std::stod(token);

        std::getline(ss, token, ',');
        se_config.grid_node_id = token;

        std::getline(ss, se_group_str, ',');
        se_config.SE_group_id = std::stoi(se_group_str);

        std::getline(ss, se_config.location_type, ',');

        group_map[se_config.SE_group_id].push_back(se_config);
    }

    for (const auto& pair : group_map) {
        se_groups.emplace_back(pair.first, pair.second);
    }

    return se_groups;
}

L2_control_strategy_parameters get_L2_control_strategy_parameters()
{
    const double beginning_of_TofU_rate_period__time_from_midnight_hrs = -1;
    const double end_of_TofU_rate_period__time_from_midnight_hrs = 5;
    const std::string randomization_method = "M1";
    const double M1_delay_period_hrs = 0.25;
    const int random_seed = 100;
     
    ES100_L2_parameters ES100_A;
    ES100_A.beginning_of_TofU_rate_period__time_from_midnight_hrs = -1;
    ES100_A.end_of_TofU_rate_period__time_from_midnight_hrs = 5;
    ES100_A.randomization_method = "M1";
    ES100_A.M1_delay_period_hrs = 0.25;
    ES100_A.random_seed = 100;

    ES100_L2_parameters ES100_B;
    ES100_B.beginning_of_TofU_rate_period__time_from_midnight_hrs = -1;
    ES100_B.end_of_TofU_rate_period__time_from_midnight_hrs = 5;
    ES100_B.randomization_method = "M2";
    ES100_B.M1_delay_period_hrs = 0.25;
    ES100_B.random_seed = 100;
    
    ES110_L2_parameters ES110;
    ES110.random_seed = 100;

    ES200_L2_parameters ES200;
    ES200.weight_factor_to_calculate_valley_fill_target = 0.0;

    ES300_L2_parameters ES300;
    ES300.weight_factor_to_calculate_valley_fill_target = 0.0;

    ES400_L2_parameters ES400;
    ES400.communication = false;

    normal_random_error random_err;
    random_err.seed = 100;
    random_err.stdev = 200;
    random_err.stdev_bounds = 1.5;

    ES500_L2_parameters ES500;
    ES500.aggregator_timestep_mins = 15;
    ES500.off_to_on_lead_time_sec = random_err;
    ES500.default_lead_time_sec = random_err;
    
    LPF_parameters_randomize_window_size LPF;
    LPF.is_active = true;
    LPF.seed = 100;
    LPF.window_size_LB = 2;
    LPF.window_size_UB = 18;
    LPF.window_type = LPF_window_enum::Rectangular;

    VS100_L2_parameters VS100;
    VS100.target_P3_reference__percent_of_maxP3 = 90;
    VS100.max_delta_kW_per_min = 1000;
    VS100.volt_delta_kW_curve_puV = std::vector<double>{0.95, 0.99, 1.0, 1.03, 1.05};
    VS100.volt_delta_kW_percP = std::vector<double>{-40, -2, 0, 0, 10};
    VS100.voltage_LPF = LPF;

    VS200_L2_parameters VS200_A;
    VS200_A.target_P3_reference__percent_of_maxP3 = 70;
    VS200_A.max_delta_kVAR_per_min = 1000;
    VS200_A.volt_var_curve_puV = std::vector<double>{0.95, 0.975, 0.99, 1, 1.03, 1.05};
    VS200_A.volt_var_curve_percQ = std::vector<double>{-100, -25, -5, 0, 0, 20};
    VS200_A.voltage_LPF = LPF;

    VS200_L2_parameters VS200_B;
    VS200_B.target_P3_reference__percent_of_maxP3 = 70;
    VS200_B.max_delta_kVAR_per_min = 1000;
    VS200_B.volt_var_curve_puV = std::vector<double>{0.95, 0.975, 0.99, 1, 1.03, 1.05};
    VS200_B.volt_var_curve_percQ = std::vector<double>{-100, -25, -5, 0, 0, 20};
    VS200_B.voltage_LPF = LPF;

    VS200_L2_parameters VS200_C;
    VS200_C.target_P3_reference__percent_of_maxP3 = 70;
    VS200_C.max_delta_kVAR_per_min = 1000;
    VS200_C.volt_var_curve_puV = std::vector<double>{0.95, 0.975, 0.99, 1, 1.03, 1.05};
    VS200_C.volt_var_curve_percQ = std::vector<double>{-100, -25, -5, 0, 0, 20};
    VS200_C.voltage_LPF = LPF;

    VS300_L2_parameters VS300;
    VS300.target_P3_reference__percent_of_maxP3 = 90;
    VS300.max_QkVAR_as_percent_of_SkVA = 90;
    VS300.gamma = 1.0;
    VS300.voltage_LPF = LPF;

    L2_control_strategy_parameters params;
    params.ES100_A = ES100_A;
    params.ES100_B = ES100_B;
    params.ES110 = ES110;
    params.ES200 = ES200;
    params.ES300 = ES300;
    params.ES400 = ES400;
    params.ES500 = ES500;
    params.VS100 = VS100;
    params.VS200_A = VS200_A;
    params.VS200_B = VS200_B;
    params.VS200_C = VS200_C;
    params.VS300 = VS300;

    return params;
}


std::unordered_map<int, int> get_SE_id_to_SE_group_id_map(const std::string& SE_file_path) {
    std::ifstream file(SE_file_path);
    std::string line, header, token;
    std::unordered_map<int, int> seMap;

    // Read the header line
    std::getline(file, header);

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        int se_id, se_group;

        // Read SE_id
        std::getline(ss, token, ',');
        se_id = std::stoi(token);

        // Skip other columns until SE_group
        for (int i = 0; i < 4; ++i) {
            std::getline(ss, token, ',');
        }

        // Read SE_group
        std::getline(ss, token, ',');
        se_group = std::stoi(token);

        // Insert into the map
        seMap[se_id] = se_group;
    }
    return seMap;
}



std::vector<charge_event_data> get_charge_events(const std::string& CE_file_path, const std::string& SE_file_path, const std::string& control_strategy)
{
    const std::unordered_map<int, int> SE_id_to_SE_group_id_map =  get_SE_id_to_SE_group_id_map(SE_file_path);

    std::vector<charge_event_data> data;
    std::ifstream file(CE_file_path);
    std::string line;

    // Skip the header
    std::getline(file, line);

    const stop_charging_criteria scc;
    control_strategy_enums control_enums;

    const std::vector<std::string> possible_control_modes = {
        "ASAP",
        "ALAP",
        "FLAT"
    };

    // if( possible_control_modes.find(control_strategy) == possible_control_modes.end() )
    // {
    //     std::cerr << "Control mode not valid. Has to be one of: ";
    //     std::cerr << "{ ";
    //     for( int i = 0; i < possible_control_modes.size(); i++ )
    //     {
    //         const std::string& pcm = possible_control_modes.at(i);
    //         std::cerr << pcm;
    //         if( i < possible_control_modes.size()-1 ) std::cerr << ", ";
    //     }
    //     std::cerr << " }" << std::endl;
    // }

    if (control_strategy == "ASAP")
    {
        control_enums.inverter_model_supports_Qsetpoint = false;
        control_enums.ES_control_strategy = L2_control_strategies_enum::NA;
        control_enums.VS_control_strategy = L2_control_strategies_enum::NA;
        control_enums.ext_control_strategy = "NA";
    }
    else if (control_strategy == "ALAP")
    {
        control_enums.inverter_model_supports_Qsetpoint = false;
        control_enums.ES_control_strategy = L2_control_strategies_enum::NA;
        control_enums.VS_control_strategy = L2_control_strategies_enum::NA;
        control_enums.ext_control_strategy = "ext_0001";  // Waiting for an external strategy message that never comes (so it's as late as possible).
    }
    else if (control_strategy == "FLAT")
    {
        control_enums.inverter_model_supports_Qsetpoint = false;
        control_enums.ES_control_strategy = L2_control_strategies_enum::ES200;  // "ES" stands for "Energy Shifting".
        control_enums.VS_control_strategy = L2_control_strategies_enum::NA;     // "VS" stands for "voltage support"
        control_enums.ext_control_strategy = "NA";
    }
    else
    {
        std::cerr << "Control mode not valid." << std::endl;
    }

    std::vector<std::string> tokens;
    tokens.reserve(13);

    while (std::getline(file, line)) {

        tokens.clear();
        std::istringstream ss(line);
        std::string item;
    
        while (std::getline(ss, item, ',')) {
            tokens.push_back(std::move(item));
        }

        if (tokens.size() == 13) {

            int charge_event_id = std::stoi(tokens[0]);
            int SE_id = std::stoi(tokens[1]);
            int SE_group_id = SE_id_to_SE_group_id_map.at(SE_id);
            int PEV_id = std::stoi(tokens[2]);
            std::string PEV_type = tokens[3];
            double arrival_unix_time = std::stod(tokens[4])*3600;
            double departure_unix_time = std::stod(tokens[6])*3600;
            double arrival_SOC = std::stod(tokens[8]) * 100;
            double departure_SOC = std::stod(tokens[9]) * 100;

            //if (std::max(arrival_unix_time, 24*3600.0) < std::min(departure_unix_time, 48*3600.0))
            {
                data.emplace_back(charge_event_id, SE_group_id, SE_id, PEV_id, PEV_type, arrival_unix_time,
                                  departure_unix_time, arrival_SOC, departure_SOC, scc, control_enums);
            }
        }
        else{
            std::cerr << "CE file has columns not equal to 13." << std::endl;
        }
    }
    std::cout <<  "Total charge events to be added to ICM simulation: " << data.size() << std::endl;
    return data;
}

void run_icm_simulation( const std::string& input_path,
                         const std::string& output_path,
                         const std::string& CE_file_path,
                         const std::string& SE_file_path,
                         const interface_to_SE_groups_inputs& icm_inputs,
                         const std::string& control_strategy)
{
    interface_to_SE_groups icm{input_path, icm_inputs};

    const std::vector<charge_event_data> charge_events = get_charge_events(CE_file_path, SE_file_path, control_strategy);
    std::cout << "charge_events.size(): " << charge_events.size() << std::endl;

    icm.add_charge_events(charge_events);

    double start_unix_time_sec = 0.0;
    double end_unix_time_sec = 3*24*3600;
    double timestep_sec = 60;
    
    double prev_unix_time_sec = start_unix_time_sec;
    double now_unix_time_sec = start_unix_time_sec + timestep_sec;
    std::map<grid_node_id_type, double> pu_Vrms;
    pu_Vrms.emplace("home", 1.0);
    pu_Vrms.emplace("work", 1.0);
    pu_Vrms.emplace("destination", 1.0);
    
    std::map<grid_node_id_type, std::pair<double, double> > PQ_vals;

    std::vector<double> simulation_time_vec_hrs, home_power_vec_kW, work_power_vec_kW, destination_power_vec_kW, total_power_vec_kW;
    double home_power_kW, work_power_kW, destination_power_kW, total_power_kW;

    auto start_time = std::chrono::high_resolution_clock::now();

    while (now_unix_time_sec < end_unix_time_sec)
    {
        if (std::fmod(now_unix_time_sec, 24*3600.0) < 0.0001)
        {
            std::cout << "simulation time: " <<  now_unix_time_sec/3600.0 << " In Day: " <<  now_unix_time_sec/(24*3600.0);

            
            auto current_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;
            double time_taken_sec = elapsed.count();

            // Estimate time remaining
            double progress = (now_unix_time_sec - start_unix_time_sec) / (end_unix_time_sec - start_unix_time_sec);
            double estimated_total_time_sec = time_taken_sec / progress;
            double estimated_time_remaining_sec = estimated_total_time_sec - time_taken_sec;

            std::cout << " Time taken: " << time_taken_sec/60.0 << " minutes";
            std::cout << " Est time rem.: " << estimated_time_remaining_sec/60.0 << " minutes" << std::endl;

        }   

        PQ_vals = icm.get_charging_power(prev_unix_time_sec, now_unix_time_sec, pu_Vrms);

        home_power_kW = PQ_vals["home"].first;
        work_power_kW = PQ_vals["work"].first;
        destination_power_kW = PQ_vals["destination"].first;
        total_power_kW = home_power_kW + work_power_kW + destination_power_kW;

        simulation_time_vec_hrs.push_back(now_unix_time_sec/3600.0);
        home_power_vec_kW.push_back(home_power_kW);
        work_power_vec_kW.push_back(work_power_kW);
        destination_power_vec_kW.push_back(destination_power_kW);
        total_power_vec_kW.push_back(total_power_kW);
        
        prev_unix_time_sec = now_unix_time_sec;
        now_unix_time_sec += timestep_sec;
    }

    std::cout << "Writing ICM output" << std::endl;

    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file. Failed at path 'output_path': " << output_path << std::endl;
    }

    // header
    file << "simulation_time_hrs,total_kW,home_kW,work_kW,destination_kW" << std::endl;

    for(size_t i = 0; i < simulation_time_vec_hrs.size(); i++)
    {
        file << std::to_string(simulation_time_vec_hrs[i]) << ",";
        file << std::to_string(total_power_vec_kW[i]) << ",";
        file << std::to_string(home_power_vec_kW[i]) << ",";
        file << std::to_string(work_power_vec_kW[i]) << ",";
        file << std::to_string(destination_power_vec_kW[i]) << std::endl;
    }

    file.close();
}

int main(int argc, char* argv[])
{
    std::string sim_folder, control_mode;
    try {
        cxxopts::Options options("run_icm", "Run Caldera ICM to generate aggregate EV charging profiles");

        options.add_options()
            ("i,io-folder", "Input Output Folder", cxxopts::value<std::string>())
            ("c,control-mode", "Control mode", cxxopts::value<std::string>())
            ("h,help", "Print usage");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        // Check if mandatory option is provided
        if (!result.count("io-folder")) {
            std::cerr << "Error: --io-folder is required.\n";
            std::cerr << options.help() << std::endl;
            return 1;
        }
        
        // Check if mandatory option is provided
        if (!result.count("control-mode")) {
            std::cerr << "Error: --control-mode is required.\n";
            std::cerr << options.help() << std::endl;
            return 1;
        }

        sim_folder = result["io-folder"].as<std::string>();
        std::cout << "Using folder: " << sim_folder << std::endl;

        control_mode = result["control-mode"].as<std::string>();
        std::cout << "Using control mode: " << control_mode << std::endl;

    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    }
 
    // ICM
    std::cout << "ICM started" << std::endl;
    const std::string output_filename = control_mode + "_charging_profile.csv";
    const std::string input_path = (std::filesystem::path(sim_folder) / "icm" / "inputs").string();
    const std::string SE_file_path = (std::filesystem::path(sim_folder) / "icm" / "inputs" / "SE_ICM.csv").string();
    const std::string CE_file_path = (std::filesystem::path(sim_folder) / "icm" / "inputs" / "CE_ICM.csv").string();
    const std::string output_path = (std::filesystem::path(sim_folder) / "icm" / "outputs" / output_filename).string();
    
    const bool create_charge_profile_library = true;
    const std::vector<pev_charge_ramping_workaround> ramping_by_pevType_seType{};
    const EV_ramping_map ramping_by_pevType_only{};
    
    charge_event_queuing_inputs CE_queuing_inputs{};
    CE_queuing_inputs.max_allowed_overlap_time_sec = 0.0;
    CE_queuing_inputs.queuing_mode = queuing_mode_enum::overlapLimited_mostRecentlyQueuedHasPriority;

    const std::vector<SE_group_configuration> infrastructure_topology = get_SE_group_configuration(SE_file_path);
    const double data_start_unix_time = 0;
    const int data_timestep_sec = 1;
    const int steps =  int(int( (367*24*3600) - (data_start_unix_time) )/ int(data_timestep_sec));
    const std::vector<double> actual_load_akW(steps, 0.0);
    const std::vector<double> forecast_load_akW(steps, 0.0);
    const double adjustment_interval_hrs{};
    const L2_control_strategy_parameters L2_parameters = get_L2_control_strategy_parameters();
    const bool ensure_pev_charge_needs_met = true;

    interface_to_SE_groups_inputs icm_inputs{
        create_charge_profile_library,
        ramping_by_pevType_only,
        ramping_by_pevType_seType,
        CE_queuing_inputs,
        infrastructure_topology,
        data_start_unix_time,
        data_timestep_sec,
        actual_load_akW,
        forecast_load_akW,
        adjustment_interval_hrs,
        L2_parameters,
        ensure_pev_charge_needs_met
    };

    run_icm_simulation(input_path, output_path, CE_file_path, SE_file_path, icm_inputs, control_mode);

    return 0;
}

