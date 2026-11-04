#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cmath>

#include "ICM_interface.h"

//
// Tests charge_start_unix_time in an uncontrolled setting (ES, VS and Ext all NA).
// Each charge event is plugged in from 1 to 10 hrs and has a different charge start time.
// Charging must not begin before charge_start_unix_time, but the EV is still plugged in from arrival.
//

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
    ES100_A.beginning_of_TofU_rate_period__time_from_midnight_hrs = 11.0;        // <------------ ToU start time (24-hrs format)
    ES100_A.end_of_TofU_rate_period__time_from_midnight_hrs = 16.0;              // <------------ ToU end time (24-hrs format)
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



std::vector<charge_event_data> get_charge_events(const std::string& CE_file_path, const std::string& SE_file_path)
{
    const std::unordered_map<int, int> SE_id_to_SE_group_id_map =  get_SE_id_to_SE_group_id_map(SE_file_path);

    std::vector<charge_event_data> data;
    std::ifstream file(CE_file_path);
    std::string line;

    // Skip the header
    std::getline(file, line);

    const stop_charging_criteria scc;

    // Uncontrolled charging.
    control_strategy_enums control_enums;
    control_enums.inverter_model_supports_Qsetpoint = false;
    control_enums.ES_control_strategy = L2_control_strategies_enum::NA;
    control_enums.VS_control_strategy = L2_control_strategies_enum::NA;
    control_enums.ext_control_strategy = "NA";

    std::vector<std::string> tokens;
    tokens.reserve(14);

    while (std::getline(file, line)) {

        tokens.clear();
        std::istringstream ss(line);
        std::string item;
    
        while (std::getline(ss, item, ',')) {
            tokens.push_back(std::move(item));
        }

        if (tokens.size() == 14) {

            int charge_event_id = std::stoi(tokens[0]);
            int SE_id = std::stoi(tokens[1]);
            int SE_group_id = SE_id_to_SE_group_id_map.at(SE_id);
            int PEV_id = std::stoi(tokens[2]);
            std::string PEV_type = tokens[3];
            double arrival_unix_time = std::stod(tokens[4])*3600;
            double departure_unix_time = std::stod(tokens[6])*3600;
            double charge_start_unix_time = std::stod(tokens[7])*3600;
            double arrival_SOC = std::stod(tokens[9]) * 100;
            double departure_SOC = std::stod(tokens[10]) * 100;

            data.emplace_back(charge_event_id, SE_group_id, SE_id, PEV_id, PEV_type, arrival_unix_time,
                              departure_unix_time, charge_start_unix_time, arrival_SOC, departure_SOC, scc, control_enums);
        }
        else{
            std::cerr << "CE file has columns not equal to 14." << std::endl;
        }
    }
    std::cout <<  "Total charge events to be added to ICM simulation: " << data.size() << std::endl;
    return data;
}

int run_icm_simulation(  const std::string& input_path,
                         const std::string& output_path,
                         const std::string& CE_file_path,
                         const std::string& SE_file_path,
                         const interface_to_SE_groups_inputs& icm_inputs)
{
    interface_to_SE_groups icm{input_path, icm_inputs};

    const std::vector<charge_event_data> charge_events = get_charge_events(CE_file_path, SE_file_path);
    icm.add_charge_events(charge_events);

    double start_unix_time_sec = 0.0;
    double end_unix_time_sec = 12*3600;
    double timestep_sec = 60;
    
    double prev_unix_time_sec = start_unix_time_sec;
    double now_unix_time_sec = start_unix_time_sec + timestep_sec;
    std::map<grid_node_id_type, double> pu_Vrms;
    
    const int n_node_ids = 3;
    std::vector< std::string > node_ids_strings_arr;
    for( int i = 1; i <= n_node_ids; i++ )
    {
        std::stringstream node_id_name_ss;
        node_id_name_ss << "home" << i;
        node_ids_strings_arr.push_back( node_id_name_ss.str() );
    }
    
    for( const auto& nodeidstr : node_ids_strings_arr )
    {
        pu_Vrms.emplace(nodeidstr, 1.0);
    }
        
    std::map<grid_node_id_type, std::pair<double, double> > PQ_vals;

    std::vector<double> simulation_time_vec_hrs, total_power_vec_kW;
    double total_power_kW;
    std::map< std::string, std::vector<double> > individual_nodeids_data;
    for( const auto& nodeidstr : node_ids_strings_arr )
    {
        individual_nodeids_data[ nodeidstr ] = std::vector<double>();
    }

    while (now_unix_time_sec < end_unix_time_sec)
    {
        PQ_vals = icm.get_charging_power(prev_unix_time_sec, now_unix_time_sec, pu_Vrms);

        total_power_kW = 0.0;
        for( const auto& nodeidstr : node_ids_strings_arr )
        {
            total_power_kW += PQ_vals.at(nodeidstr).first;
            
            individual_nodeids_data.at( nodeidstr ).push_back( PQ_vals.at(nodeidstr).first );
        }
        total_power_vec_kW.push_back(total_power_kW);
        simulation_time_vec_hrs.push_back(now_unix_time_sec/3600.0);
        
        prev_unix_time_sec = now_unix_time_sec;
        now_unix_time_sec += timestep_sec;
    }

    std::cout << "Writing ICM output" << std::endl;

    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file. Failed at path 'output_path': " << output_path << std::endl;
    }

    // header
    file << "simulation_time_hrs,total_kW,";
    for( int j = 1; j <= n_node_ids; j++ )
    {
        const std::string& nodeidstr = node_ids_strings_arr.at(j-1);
        file << nodeidstr;
        if( j < n_node_ids )
        {
            file << ",";
        }
        else
        {
            file << std::endl;
        }
    }

    for(size_t i = 0; i < simulation_time_vec_hrs.size(); i++)
    {
        file << std::to_string(simulation_time_vec_hrs.at(i)) << ",";
        file << std::to_string(total_power_vec_kW.at(i)) << ",";
        for( int j = 1; j <= n_node_ids; j++ )
        {
            const std::string& nodeidstr = node_ids_strings_arr.at(j-1);
            file << std::to_string(individual_nodeids_data.at(nodeidstr).at(i));
            if( j < n_node_ids )
            {
                file << ",";
            }
            else
            {
                file << std::endl;
            }
        }
    }

    file.close();
    
    
    
    // *********************************************************
    // Check the results and return the appropriate error code.
    // *********************************************************
    
    // A map of tuples, (one for each charge event)
    //     containing:  start_time_hrs, end_time_hrs, expected_max_power_level_kW
    //     home1: charge_start = arrival (1 hr). Baseline, same as before charge_start_unix_time existed.
    //     home2: charge_start = 3 hrs. Same profile as home1 shifted by 2 hrs, since the energy needed is the same.
    //     home3: charge_start = 8 hrs. Not enough time to reach the target SOC, so charging stops at departure (10 hrs).
    //            This matches an EV that simply arrives at 8 hrs and departs at 10 hrs.
    std::map< std::string, std::tuple<double,double,double> > correct_solution_data;
    correct_solution_data[ node_ids_strings_arr.at(0) ] = std::make_tuple( 1.0, 4.73333333333, 11.5 );
    correct_solution_data[ node_ids_strings_arr.at(1) ] = std::make_tuple( 3.0, 6.73333333333, 11.5 );
    correct_solution_data[ node_ids_strings_arr.at(2) ] = std::make_tuple( 8.0, 10.0166666667, 11.5 );
    
    int num_errors = 0;
    
    std::cout << "" << std::endl;
    
    for( const auto& key_value_pair : correct_solution_data )
    {
        const std::string& nodeidstr = key_value_pair.first;
        const std::tuple<double,double,double>& expectedstartHrEndHrPwr_tuple = key_value_pair.second;
        
        const double expected_start_hr = std::get<0>(expectedstartHrEndHrPwr_tuple);
        const double expected_end_hr = std::get<1>(expectedstartHrEndHrPwr_tuple);
        const double expected_power_kW = std::get<2>(expectedstartHrEndHrPwr_tuple);
    
        // Loop over all the hours and find the max power level during the charge event period.
        double found_max_power_level = -9999.0;
        double found_start_time_hrs = -9999.0;
        double found_end_time_hrs = -9999.0;
        for( int i = 0; i < simulation_time_vec_hrs.size(); i++ )
        {
            const double power_level_kW_this_timestep = individual_nodeids_data.at(nodeidstr).at(i);
            
            if( power_level_kW_this_timestep > 1e-8 )
            {
                // Record the start time if we haven't already.
                if( found_start_time_hrs < 0 )
                {
                    found_start_time_hrs = simulation_time_vec_hrs.at(i);
                }
                
                // Update the end time.
                found_end_time_hrs = simulation_time_vec_hrs.at(i);
            }
            
            // Update the max power level seen so far.
            if( power_level_kW_this_timestep > found_max_power_level )
            {
                found_max_power_level = power_level_kW_this_timestep;
            }
        }
    
        std::cout << std::setprecision(12) << "Node id: " << nodeidstr
                  << "   found_start_time_hrs:  " << found_start_time_hrs
                  << "   found_end_time_hrs:    " << found_end_time_hrs 
                  << "   found_max_power_level: " << found_max_power_level << std::endl;
    
        
        if( fabs( expected_start_hr - found_start_time_hrs ) > 1e-5 )
        {
            std::cout << "---" << std::endl;
            std::cout << "expected_start_hr:      " << expected_start_hr << std::endl;
            std::cout << "found_start_time_hrs:   " << found_start_time_hrs << std::endl;
            std::cout << "---" << std::endl;
            num_errors++;
        }
        
        if( fabs( expected_end_hr - found_end_time_hrs ) > 1e-5 )
        {
            std::cout << "---" << std::endl;
            std::cout << "expected_end_hr:      " << expected_end_hr << std::endl;
            std::cout << "found_end_time_hrs:   " << found_end_time_hrs << std::endl;
            std::cout << "---" << std::endl;
            num_errors++;
        }
        
        if( fabs( expected_power_kW - found_max_power_level ) > 1e-5 )
        {
            std::cout << "---" << std::endl;
            std::cout << "expected_power_kW:      " << expected_power_kW << std::endl;
            std::cout << "found_max_power_level:  " << found_max_power_level << std::endl;
            std::cout << "---" << std::endl;
            num_errors++;
        }
    }
    
    std::cout << "" << std::endl;
    
    return num_errors;
}

int main(int argc, char* argv[])
{
    const std::string sim_folder = "inout";
 
    // ICM
    std::cout << "ICM started" << std::endl;
    const std::string output_filename = "charge_start_time_charging_profile.csv";
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

    //
    // Returns error code if the results are wrong.
    //
    const int error_code = run_icm_simulation(input_path, output_path, CE_file_path, SE_file_path, icm_inputs);
    if( error_code != 0 )
    {
        std::cout << "error_code: " << error_code << std::endl;
    }
    else
    {
        std::cout << "Success. Code: " << error_code << std::endl;
    }
    
    return error_code;
}
