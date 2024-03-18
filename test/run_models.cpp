#include <fstream>
#include <string>
#include <chrono>

#include "factory_charging_transitions.h"
#include "datatypes_global.h"
#include "ICM_interface.h"


void create_infrastruture_from_EVSE_inputs(const std::vector<std::string>& EVs, const std::vector<std::string>& EVSEs, std::vector<SE_configuration>& infrastructure_topology)
{
    int SE_group_id = 1;
    SE_id_type SE_id = 1;
    EVSE_type evse_type = "";
    double lattitude = 0.0;
    double longitude = 0.0;
    grid_node_id_type grid_node_id = "";
    std::string location_type = "O";

    for (const std::string& EV : EVs)
    {
        for (const EVSE_type& EVSE : EVSEs)
        {
            evse_type = EVSE;
            grid_node_id = EVSE;
            SE_configuration SE{ SE_group_id, SE_id, evse_type, lattitude, longitude, grid_node_id,
                location_type };
            infrastructure_topology.push_back(SE);

            SE_id += 1;
        }
    }
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

    std::vector<SE_configuration> SE_config;
    create_infrastruture_from_EVSE_inputs(EVs, EVSEs, SE_config);
    SE_group_configuration Station_config(1, SE_config);
    std::cout << "SE_group_configuration size : " << Station_config.SEs.size() << "\n";

    std::vector<SE_group_configuration> infrastructure_topology {Station_config};

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
    const std::string input_path = "./inputs/CalderaCast";

    // select models to run
    std::vector<std::string> EVs = { "bev150_150kW", "bev250_350kW" };
    std::vector<std::string> EVSEs = {"dcfc_50", "xfc_150", "xfc_150T", "xfc_200", "xfc_250", "xfc_350"};

    //EVs = { "bev250_350kW" };
    //EVSEs = { "dcfc_50", "xfc_150", "xfc_150T", "xfc_200", "xfc_250", "xfc_350" };

    //EVs = { "bev250_350kW" };
    //EVSEs = {"xfc_350"};


    const interface_to_SE_groups_inputs inputs = create_ICM_inputs(input_path, EVs, EVSEs);

    auto start = std::chrono::high_resolution_clock::now();
    
    std::cout << "Starting ICM initialization" << std::endl;
    // Create ICM object
    interface_to_SE_groups* ICM = new interface_to_SE_groups{input_path, inputs };
    std::cout << "Finished ICM initialization" << std::endl;
    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << "ICM initialization took "
        << std::chrono::duration_cast<std::chrono::seconds>(finish - start).count()
        << " seconds\n";


    // add charge events
    std::vector<SE_group_charge_event_data> SE_group_charge_events{};
    create_charge_events(EVs, EVSEs, SE_group_charge_events);
    ICM->add_charge_events_by_SE_group(SE_group_charge_events);

    // setup simulation time constraints
    double start_unix_time = 0.0 * 3600;
    double end_unix_time = 4.0 * 3600;
    double time_step_sec = 1;

    double prev_unix_time = start_unix_time;
    double cur_unix_time = start_unix_time + time_step_sec;
    double sim_unix_time_hrs = 0.0;

    // build the files header
    std::string header = "simulation_time_hrs";

    std::string car_type = "";
    int counter = 0;
    for (const std::string& EV : EVs)
    {
        if (counter == 0) car_type = "small_car";
        else if (counter == 1) car_type = "large_car";
        for (std::string& evse : EVSEs)
        {
            header += ", " + car_type +"_on_" + evse;
        }
        counter += 1;
    }
    header += "\n";

    // Strings to write data into 
    std::string P1_kW_data = "";
    std::string P2_kW_data = "";
    std::string P3_kW_data = "";
    std::string Q3_kVAR_data = "";
    std::string SOC_data = "";

    while (cur_unix_time < end_unix_time)
    {
        sim_unix_time_hrs = cur_unix_time / 3600.0;

        // add simulation_unix_time_hrs
        P1_kW_data += std::to_string(sim_unix_time_hrs);
        P2_kW_data += std::to_string(sim_unix_time_hrs);
        P3_kW_data += std::to_string(sim_unix_time_hrs);
        Q3_kVAR_data += std::to_string(sim_unix_time_hrs);
        SOC_data += std::to_string(sim_unix_time_hrs);

        for (int se_id = 1; se_id <= EVSEs.size()*EVs.size(); se_id++)
        {
            SE_power pq_power = ICM->get_SE_power(se_id, prev_unix_time, cur_unix_time, 1.0);

            // add data
            P1_kW_data += ", " + std::to_string(pq_power.P1_kW);
            P2_kW_data += ", " + std::to_string(pq_power.P2_kW);
            P3_kW_data += ", " + std::to_string(pq_power.P3_kW);
            Q3_kVAR_data += ", " + std::to_string(pq_power.Q3_kVAR);
            SOC_data += ", " + std::to_string(pq_power.soc);
        }

        // new line
        P1_kW_data += "\n";
        P2_kW_data += "\n";
        P3_kW_data += "\n";
        Q3_kVAR_data += "\n";
        SOC_data += "\n";

        prev_unix_time = cur_unix_time;
        cur_unix_time += time_step_sec;
    }
	
    // initialize file_handle objects;
    std::ofstream P1_kW_out, P2_kW_out, P3_kW_out, Q3_kVAR_out, SOC_out;

    // open the files
    P1_kW_out.open("./P1_kW.csv");
    P2_kW_out.open("./P2_kW.csv");
    P3_kW_out.open("./P3_kW.csv");
    Q3_kVAR_out.open("./Q3_kVAR.csv");
    SOC_out.open("./SOC.csv");
    
    // write the data
    P1_kW_out << header << P1_kW_data;
    P2_kW_out << header << P2_kW_data;
    P3_kW_out << header << P3_kW_data;
    Q3_kVAR_out << header << Q3_kVAR_data;
    SOC_out << header << SOC_data;

    delete ICM;
	return 0;
}