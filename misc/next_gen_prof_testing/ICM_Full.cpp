#include <fstream>
#include <string>
#include <chrono>

#include "factory_charging_transitions.h"
#include "datatypes_global.h"
#include "ICM_interface.h"

std::vector<SE_group_configuration> create_SE_group_config(int num_stations_to_create, const std::vector<std::string>& EVSEs)
{

    std::vector<SE_group_configuration> all_SE_group_config{};

    int SE_id_counter = 1;
    for (int i = 1; i <= num_stations_to_create; i++)
    {
        int SE_group_id = i;
        grid_node_id_type grid_node_id = "node_" + std::to_string(SE_group_id);

        std::vector<SE_configuration> SE_configs_under_station{};
        for (const std::string& evse : EVSEs)
        {
            SE_id_type SE_id = SE_id_counter;
            EVSE_type evse_type = evse;
            double latitude = 0.0;                  //dummy
            double longitude = 0.0;                 //dummy
            std::string location_type = "O";        //dummy

            SE_configuration SE_config{ SE_group_id, SE_id, evse_type, latitude, longitude, grid_node_id, location_type };
            SE_configs_under_station.push_back(SE_config);

            SE_id_counter++;
        }
        SE_group_configuration Station_config(SE_group_id, SE_configs_under_station);
        all_SE_group_config.push_back(Station_config);
    }

    return all_SE_group_config;
}

interface_to_SE_groups_inputs create_ICM_inputs(const std::string& input_path, const std::vector<std::string>& EVs, const std::vector<std::string>& EVSEs)
{
    // factory_inputs
    bool create_charge_profile_library = true;
    EV_ramping_map ramping_by_pevType_only{};
    std::vector<pev_charge_ramping_workaround> ramping_by_pevType_seType{};

    // infrastructure_inputs
    charge_event_queuing_inputs CE_queuing_inputs{};
    CE_queuing_inputs.max_allowed_overlap_time_sec = 2 * 60;
    CE_queuing_inputs.queuing_mode = queuing_mode_enum::overlapAllowed_earlierArrivalTimeHasPriority;

    // infrastructure_topology
    int num_stations_to_create = 2;
    std::vector<SE_group_configuration> infrastructure_topology = create_SE_group_config(num_stations_to_create, EVSEs);

    // baseLD_forecaster_inputs
    double data_start_unix_time = 0.0;
    int data_timestep_sec = 24*3600;
    std::vector<double> actual_load_akW{0};
    std::vector<double> forecast_load_akW{0};
    double adjustment_interval_hrs = 0;

    // control_strategy_inputs
    L2_control_strategy_parameters L2_parameters{};

    // Set control strategy params even though they are not useful here
    // TODO : Remove the dependency
    ES100_L2_parameters es100_params;
    es100_params.beginning_of_TofU_rate_period__time_from_midnight_hrs = -1;
    es100_params.end_of_TofU_rate_period__time_from_midnight_hrs = 8;
    es100_params.randomization_method = "M1";
    es100_params.M1_delay_period_hrs = 0.25;
    es100_params.random_seed = 100;


    VS100_L2_parameters vs100_params;
    vs100_params.target_P3_reference__percent_of_maxP3 = 90;
    vs100_params.max_delta_kW_per_min = 1000;
    vs100_params.volt_delta_kW_curve_puV = {0.95, 0.99, 1.0, 1.05};
    vs100_params.volt_delta_kW_percP = { -40, -2, 0, 10 };

    LPF_parameters_randomize_window_size lpf;
    lpf.is_active = true;
    lpf.seed = 40;
    lpf.window_size_LB = 2;
    lpf.window_size_UB = 18;
    lpf.window_type = LPF_window_enum::Rectangular;

    vs100_params.voltage_LPF = lpf;

    L2_parameters.ES100_A = es100_params;
    L2_parameters.VS100 = vs100_params;

    bool ensure_pev_charge_needs_met = true;

    return interface_to_SE_groups_inputs{
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
}


void create_charge_events(const std::vector<std::string>& EVs, const std::vector<std::string>& SEs_to_run, std::vector<SE_group_charge_event_data>& SE_group_charge_events)
{
     
    int charge_event_id = 1;    // is updated down below
    int SE_group_id = 1;
    SE_id_type SE_id = 1;   // is updated down below
    vehicle_id_type vehicle_id = 1; // is updated down below
    EV_type vehicle_type = "";
    double arrival_unix_time = 1.0 * 3600;
    double departure_unix_time = 3.0 * 3600;
    double arrival_SOC = 0;
    double departure_SOC = 98.8;
    stop_charging_criteria stop_charge{};
    control_strategy_enums control_enums{};

    std::vector<charge_event_data> charge_events;

    for (const std::string& EV : EVs)
    {
        vehicle_type = EV;
        for (const std::string& evse : SEs_to_run)
        {

            charge_events.emplace_back(charge_event_id, SE_group_id, SE_id, vehicle_id, vehicle_type,
                arrival_unix_time, departure_unix_time, arrival_SOC, departure_SOC,
                stop_charge, control_enums);

            SE_id += 1;
            charge_event_id += 1;
            vehicle_id += 1;
        }
    }
    

    SE_group_charge_event_data SE_group_CEs{ 1, charge_events };
    SE_group_charge_events.push_back(SE_group_CEs);
}


int main()
{
    const std::string input_path = "./inputs";

    // select models to run
    std::vector<std::string> EVs = { "bev150_150kW", "bev250_350kW" };
    std::vector<std::string> EVSEs = { "L1_1440", "L2_3600", "L2_7200", "L2_9600", "L2_11520", "L2_17280", "dcfc_50", "xfc_150", "xfc_350" };

    const interface_to_SE_groups_inputs inputs = create_ICM_inputs(input_path, EVs, EVSEs);

    auto start = std::chrono::high_resolution_clock::now();
    
    std::cout << "Starting ICM initialization" << std::endl;
    
    // Create ICM object
    interface_to_SE_groups ICM{ input_path, inputs };
    
    std::cout << "Finished ICM initialization" << std::endl;
    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << "ICM initialization took "
        << std::chrono::duration_cast<std::chrono::seconds>(finish - start).count()
        << " seconds\n";


    // create all charge events
    SE_group_charge_event_data SE_group_CE_data_1 = [&] () {
        
        int se_group_id = 1;
        charge_event_data charge_event{
                1,                                                      // int charge_event_id;
                se_group_id,                                            // int SE_group_id;
                1,                                                      // SE_id_type SE_id;
                1,                                                      // vehicle_id_type vehicle_id;
                "bev150_150kW",                                         // EV_type vehicle_type;
                1.0 * 3600,                                             // double arrival_unix_time;    // in seconds
                4.0 * 3600,                                             // double departure_unix_time;  // in seconds
                5,                                                      // double arrival_SOC;          // in percent (for 50%, this will be 50.0)
                95,                                                     // double departure_SOC;        // in percent (for 50%, this will be 50.0)
                stop_charging_criteria{},                               // stop_charging_criteria stop_charge;
                control_strategy_enums{}                                // control_strategy_enums control_enums;
        };

        return SE_group_charge_event_data{ se_group_id, std::vector<charge_event_data>{charge_event} };
    }();

    SE_group_charge_event_data SE_group_CE_data_2 = [&]() {

        int se_group_id = 2;
        charge_event_data charge_event{
            2,                                                      // int charge_event_id;
            se_group_id,                                            // int SE_group_id;
            10,                                                     // SE_id_type SE_id;
            2,                                                      // vehicle_id_type vehicle_id;
            "bev250_350kW",                                         // EV_type vehicle_type;
            3.0 * 3600,                                             // double arrival_unix_time;    // in seconds
            6.0 * 3600,                                             // double departure_unix_time;  // in seconds
            5,                                                      // double arrival_SOC;          // in percent (for 50%, this will be 50.0)
            95,                                                     // double departure_SOC;        // in percent (for 50%, this will be 50.0)
            stop_charging_criteria{},                               // stop_charging_criteria stop_charge;
            control_strategy_enums{}                                // control_strategy_enums control_enums;
        };

        return SE_group_charge_event_data{ se_group_id, std::vector<charge_event_data>{charge_event} };
    }();

    // add charge events
    std::vector<SE_group_charge_event_data> SE_group_charge_events{ SE_group_CE_data_1, SE_group_CE_data_2 };

    ICM.add_charge_events_by_SE_group(SE_group_charge_events);

    // setup simulation time constraints
    double start_unix_time = 0.0 * 3600;
    double end_unix_time = 8.0 * 3600;
    double time_step_sec = 1;

    double prev_unix_time = start_unix_time;
    double cur_unix_time = start_unix_time + time_step_sec;

    //double prev_unix_time = start_unix_time - time_step_sec;
    //double cur_unix_time = start_unix_time;

    double sim_unix_time_hrs = 0.0;

    // build the files header
    std::string header = "simulation_time_hrs,node_id_1,node_id_2\n";

    std::map<grid_node_id_type, double> pu_Vrms;
    pu_Vrms["node_1"] = 1.0;
    pu_Vrms["node_2"] = 1.0;

    // Run simulation by moving forward in time
    std::string data_str = "";
    while (cur_unix_time < end_unix_time)
    {
        sim_unix_time_hrs = cur_unix_time / 3600.0;

        std::map<grid_node_id_type, std::pair<double, double> > charging_power_data = ICM.get_charging_power(prev_unix_time, cur_unix_time, pu_Vrms);

        data_str += std::to_string(sim_unix_time_hrs) + ",";
        data_str += std::to_string(charging_power_data.at("node_1").first) + ",";
        data_str += std::to_string(charging_power_data.at("node_2").first) + "\n";

        prev_unix_time = cur_unix_time;
        cur_unix_time += time_step_sec;
    }
	
    // initialize file_handle objects;
    std::ofstream f_out;

    // open the files
    f_out.open("./outputs/ICM_Full_output.csv");
    
    // write the data
    f_out << header << data_str;

	return 0;
}