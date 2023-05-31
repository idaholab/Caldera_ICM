
#ifndef inl_datatypes_global_H
#define inl_datatypes_global_H

#include "EV_characteristics.h"
#include "EVSE_characteristics.h"

#include <vector>
#include <string>


using vehicle_id_type = int;
using SE_id_type = int;
using grid_node_id_type = std::string;

using process_id = std::string;
using serialized_protobuf_obj = std::string;


//==================================================================
//                Example Code Snippits
//==================================================================

/*                           Execution Time Measurements 
#include <chrono>

std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
// function call(...)
std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
std::chrono::duration<double> time_diff = end_time - start_time;
double time_sec = time_diff.count();
*/

//==================================================================
//                Low Pass Filter Parameters
//==================================================================

enum LPF_window_enum
{
	Hanning=0,
	Blackman=1,
    Rectangular=2
};


struct LPF_parameters
{
    int window_size;
    LPF_window_enum window_type;
};


struct LPF_parameters_randomize_window_size
{
    bool is_active;
    int seed;
    int window_size_LB;
    int window_size_UB;
    LPF_window_enum window_type;
};


std::pair<bool, LPF_window_enum> get_LPF_window_enum(const std::string str_val);


//==================================================================
//                  L2_control_strategy_parameters
//==================================================================

enum L2_control_strategies_enum
{
    NA = 0,
    ES100_A = 1,
    ES100_B = 2,
    ES110 = 3,
    ES200 = 4,
    ES300 = 5,
    ES500 = 6,
    VS100 = 8,   
    VS200_A = 10,
    VS200_B = 11,
    VS200_C = 12,
    VS300 = 13
};


struct ES100_L2_parameters
{
    double beginning_of_TofU_rate_period__time_from_midnight_hrs;
    double end_of_TofU_rate_period__time_from_midnight_hrs;
    std::string randomization_method;
    double M1_delay_period_hrs;
    int random_seed;
};


struct ES110_L2_parameters
{
    int random_seed;
};


struct ES200_L2_parameters
{
    double weight_factor_to_calculate_valley_fill_target;
};


struct ES300_L2_parameters
{
    double weight_factor_to_calculate_valley_fill_target;
};


struct normal_random_error
{
    int seed;
    double stdev;
    double stdev_bounds;
};


struct ES500_L2_parameters
{
    double aggregator_timestep_mins;    
    normal_random_error  off_to_on_lead_time_sec;
    normal_random_error  default_lead_time_sec;
};


struct VS100_L2_parameters
{
    double target_P3_reference__percent_of_maxP3;
    double max_delta_kW_per_min;
    std::vector<double> volt_delta_kW_curve_puV;
    std::vector<double> volt_delta_kW_percP;
    LPF_parameters_randomize_window_size voltage_LPF;
};


struct VS200_L2_parameters
{
    double target_P3_reference__percent_of_maxP3;
    double max_delta_kVAR_per_min;
    std::vector<double> volt_var_curve_puV;
    std::vector<double> volt_var_curve_percQ;
    LPF_parameters_randomize_window_size voltage_LPF;
};


struct VS300_L2_parameters
{
    double target_P3_reference__percent_of_maxP3;
    double max_QkVAR_as_percent_of_SkVA;
    double gamma;
    LPF_parameters_randomize_window_size voltage_LPF;
};


struct L2_control_strategy_parameters
{
    ES100_L2_parameters ES100_A;
    ES100_L2_parameters ES100_B;
    ES110_L2_parameters ES110;
    ES200_L2_parameters ES200;
    ES300_L2_parameters ES300;
    ES500_L2_parameters ES500;
    
    VS100_L2_parameters VS100;    
    VS200_L2_parameters VS200_A;
    VS200_L2_parameters VS200_B;
    VS200_L2_parameters VS200_C;
    VS300_L2_parameters VS300;
};


struct control_strategy_enums
{
    bool inverter_model_supports_Qsetpoint;
    L2_control_strategies_enum  ES_control_strategy;
    L2_control_strategies_enum  VS_control_strategy;
    std::string ext_control_strategy;
    
    control_strategy_enums();
};


bool L2_control_strategy_supports_Vrms_using_QkVAR(L2_control_strategies_enum  control_strategy);
std::pair<bool, L2_control_strategies_enum> get_L2_control_strategies_enum(const std::string str_val);
bool is_L2_ES_control_strategy(L2_control_strategies_enum control_strategy_enum);
bool is_L2_VS_control_strategy(L2_control_strategies_enum control_strategy_enum);

//==================================================================
//                   ES500 Aggregator Structures
//==================================================================


struct ES500_aggregator_pev_charge_needs
{
    SE_id_type SE_id;
	double departure_unix_time;
    double e3_charge_remain_kWh;
    double e3_step_max_kWh;
    double e3_step_target_kWh;
    double min_remaining_charge_time_hrs;
    double min_time_to_complete_entire_charge_hrs;
    double remaining_park_time_hrs;
    double total_park_time_hrs;
};


struct ES500_aggregator_charging_needs
{
	double next_aggregator_timestep_start_time;
	std::vector<ES500_aggregator_pev_charge_needs> pev_charge_needs;
    
    bool is_empty();
};


struct ES500_aggregator_e_step_setpoints
{
	double next_aggregator_timestep_start_time;
	std::vector<SE_id_type> SE_id;
	std::vector<double> e3_step_kWh;
	std::vector<double> charge_progression;
    
    ES500_aggregator_e_step_setpoints() {};
    ES500_aggregator_e_step_setpoints(double next_aggregator_timestep_start_time_, std::vector<SE_id_type> SE_id_, std::vector<double> e3_step_kWh_, std::vector<double> charge_progression_);

    bool is_empty();
};


struct ES500_aggregator_charging_forecast
{
	std::vector<double> arrival_unix_time;
	std::vector<double> departure_unix_time;
	std::vector<double> e3_charge_remain_kWh;
    std::vector<double> e3_step_max_kWh;
};


struct ES500_aggregator_obj_fun_constraints
{
	std::vector<double> E_cumEnergy_ALAP_kWh, 
						E_cumEnergy_ASAP_kWh, 
						E_energy_ALAP_kWh, 
						E_energy_ASAP_kWh,
						E_step_ALAP;
    bool canSolve_aka_pev_charging_in_prediction_window;
};


struct ES500_charge_cycling_control_boundary_point
{
	ES500_charge_cycling_control_boundary_point() {};
	ES500_charge_cycling_control_boundary_point(double cycling_magnitude, double cycling_vs_ramping);
	double cycling_magnitude;
	double cycling_vs_ramping;
};


struct ES500_stop_charge_cycling_decision_parameters
{
    double next_aggregator_timestep_start_time;
    int iteration;
    bool is_last_iteration;
    double off_to_on_nrg_kWh;
    double on_to_off_nrg_kWh;
    double total_on_nrg_kWh;
    double cycling_vs_ramping;
    double cycling_magnitude;
    double delta_energy_kWh;
};


//==================================================================
//                       Charge Event Data
//==================================================================

bool ext_strategy_str_is_valid(std::string ext_str);


enum stop_charging_decision_metric
{
	stop_charging_using_target_soc = 0,
	stop_charging_using_depart_time = 1,
	stop_charging_using_whatever_happens_first = 2
};
std::ostream& operator<<(std::ostream& out, const stop_charging_decision_metric& x);


enum stop_charging_mode
{
	target_charging = 0,
	block_charging = 1
};
std::ostream& operator<<(std::ostream& out, const stop_charging_mode& x);


struct stop_charging_criteria
{
	stop_charging_decision_metric decision_metric;
	stop_charging_mode soc_mode;
	stop_charging_mode depart_time_mode;
	double soc_block_charging_max_undershoot_percent;
	double depart_time_block_charging_max_undershoot_percent;
	
    stop_charging_criteria();
    stop_charging_criteria(stop_charging_decision_metric decision_metric_, stop_charging_mode soc_mode_, stop_charging_mode depart_time_mode_, double soc_block_charging_max_undershoot_percent_, double depart_time_block_charging_max_undershoot_percent_);
	static std::string get_file_header();
};
std::ostream& operator<<(std::ostream& out, const stop_charging_criteria& x);


struct charge_event_data
{
	int charge_event_id;
    int SE_group_id;
    SE_id_type  SE_id;
   	vehicle_id_type vehicle_id;
    EV_type vehicle_type;
    double arrival_unix_time;
    double departure_unix_time;
    double arrival_SOC;
    double departure_SOC;
    stop_charging_criteria stop_charge;
    
    control_strategy_enums control_enums;

    charge_event_data() {};
    charge_event_data(int charge_event_id_, int SE_group_id_, SE_id_type SE_id_, vehicle_id_type vehicle_id_, EV_type vehicle_type,
                      double arrival_unix_time_, double departure_unix_time_, double arrival_SOC_, double departure_SOC_, 
                      stop_charging_criteria stop_charge_, control_strategy_enums control_enums_);
    static std::string get_file_header();
    
	bool operator<(const charge_event_data& rhs) const
	{
		return this->arrival_unix_time < rhs.arrival_unix_time;
	}

    bool operator<(charge_event_data& rhs) const
	{
		return this->arrival_unix_time < rhs.arrival_unix_time;
	}
};
std::ostream& operator<<(std::ostream& out, const charge_event_data& x);


struct SE_group_charge_event_data
{
    int SE_group_id;
    std::vector<charge_event_data> charge_events;
    
    SE_group_charge_event_data() {};
    SE_group_charge_event_data(int SE_group_id_, std::vector<charge_event_data> charge_events_);
};


//=====================================


enum queuing_mode_enum
{
    overlapAllowed_earlierArrivalTimeHasPriority = 0,
    overlapLimited_mostRecentlyQueuedHasPriority = 1
};


struct charge_event_queuing_inputs
{
    double max_allowed_overlap_time_sec;
    queuing_mode_enum queuing_mode;
};


//==================================================================
//                       SE Group Configuration
//==================================================================

struct SE_configuration
{	
    int SE_group_id;
    SE_id_type  SE_id;
	EVSE_type  supply_equipment_type;
    double lattitude;
    double longitude;
    grid_node_id_type grid_node_id;
    std::string location_type;

    SE_configuration() {};
    SE_configuration(int SE_group_id_, SE_id_type SE_id_, EVSE_type supply_equipment_type_, double lat_, double long_, grid_node_id_type grid_node_id_, std::string location_type_);
};


struct SE_group_configuration
{
    int SE_group_id;    
    std::vector<SE_configuration> SEs;
        
    SE_group_configuration() {};
    SE_group_configuration(int SE_group_id_, std::vector<SE_configuration> SEs_);
};


//==================================================================
//                   Status of Charging 
//==================================================================

enum SE_charging_status
{
	no_ev_plugged_in = 0,
	ev_plugged_in_not_charging = 1,
	ev_charging = 2,
	ev_charge_complete = 3,
    ev_charge_ended_early = 4
};
std::ostream& operator<<(std::ostream& out, const SE_charging_status& x);


struct FICE_inputs // Future Interval Charge Energy
{    
    double interval_start_unixtime;
    double interval_duration_sec;
    double acPkW_setpoint;
    
    FICE_inputs() {};
};


struct CE_FICE
{   
    SE_id_type SE_id;
    int charge_event_id;
    double charge_energy_ackWh;
    double interval_duration_hrs;
    
    CE_FICE() {};
};


struct CE_FICE_in_SE_group
{
    int SE_group_id;
    std::vector<CE_FICE> SE_FICE_vals;
    
    CE_FICE_in_SE_group() {};
};


struct active_CE
{    
    SE_id_type SE_id;
    int charge_event_id;
    double now_unix_time;
    double now_soc;
    double min_remaining_charge_time_hrs;
    double min_time_to_complete_entire_charge_hrs;
    double now_charge_energy_ackWh;
    double energy_of_complete_charge_ackWh;
    double now_dcPkW;
    double now_acPkW;  
    double now_acQkVAR;
    vehicle_id_type vehicle_id;
    EV_type vehicle_type;

    active_CE() {};
};


struct SE_setpoint
{
    SE_id_type SE_id; 
    double PkW;
    double QkVAR = 0;
    
    SE_setpoint() {};
};


struct completed_CE
{
    SE_id_type SE_id;
    int charge_event_id;    
    double final_soc;
    
    completed_CE() {};
};


//==================================================================
//                       Miscellaneous
//==================================================================

struct SE_power
{
    double time_step_duration_hrs;
    double P1_kW;
    double P2_kW;
    double P3_kW;
    double Q3_kVAR;
    double soc;
    SE_charging_status SE_status_val;
};


enum ac_to_dc_converter_enum
{
	pf=0,
	Q_setpoint=1
};


struct pev_batterySize_info
{
    EV_type vehicle_type;
    double battery_size_kWh;
    double battery_size_with_stochastic_degredation_kWh;
};

//==================================================================
//                     PEV Charge Profile 
//==================================================================

struct pev_charge_profile_result
{
    double soc_increase;
    double E1_kWh;
    double E2_kWh;
    double E3_kWh;
    double cumQ3_kVARh;
    double total_charge_time_hrs;
    double incremental_chage_time_hrs;
    
    pev_charge_profile_result() {};
};


struct pev_charge_fragment_removal_criteria
{
    double max_percent_of_fragments_that_can_be_removed;
    double kW_change_threshold;
    double threshold_to_determine_not_removable_fragments_on_flat_peak_kW;
    double perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow;
    
    pev_charge_fragment_removal_criteria() {};
};


struct pev_charge_fragment
{
    double soc;
    double E1_kWh;
    double E2_kWh;
    double E3_kWh;
    double cumQ3_kVARh;
    double time_since_charge_began_hrs;
    
    pev_charge_fragment() {};
    pev_charge_fragment(double soc_, double E1_kWh_, double E2_kWh_, double E3_kWh_, double cumQ3_kVARh_, double time_since_charge_began_hrs_);
    
    bool operator<(const pev_charge_fragment &rhs) const
    {
        return (this->time_since_charge_began_hrs < rhs.time_since_charge_began_hrs);
    }

    bool operator<(pev_charge_fragment &rhs) const
    {
        return (this->time_since_charge_began_hrs < rhs.time_since_charge_began_hrs);
    }
};


struct pev_charge_fragment_variation
{
    bool is_removable;
    int original_charge_fragment_index;
    double time_since_charge_began_hrs;
    double soc;
    double P3_kW;
    int64_t variation_rank;
    
    pev_charge_fragment_variation() {};
    pev_charge_fragment_variation(int original_charge_fragment_index_, double time_since_charge_began_hrs_, double soc_, double P3_kW_);
};


struct charge_profile_validation_data
{
    double time_step_sec;
    double target_acP3_kW;
    pev_charge_fragment_removal_criteria fragment_removal_criteria;
    std::vector<pev_charge_fragment_variation> removed_fragments;
    std::vector<pev_charge_fragment_variation> retained_fragments;
    std::vector<pev_charge_fragment> downsampled_charge_fragments;
    std::vector<pev_charge_fragment> original_charge_fragments;
    
    charge_profile_validation_data() {};
};


struct charge_event_P3kW_limits
{
    double min_P3kW;
    double max_P3kW;
};


struct all_charge_profile_data
{
    double timestep_sec;
    std::vector<double> P1_kW;
    std::vector<double> P2_kW;
    std::vector<double> P3_kW;
    std::vector<double> Q3_kVAR;
    std::vector<double> soc;
};


//==================================================================
//                     PEV Ramping Parameters
//==================================================================


struct pev_charge_ramping
{
    double off_to_on_delay_sec;
    double off_to_on_kW_per_sec;
    
    double on_to_off_delay_sec;
    double on_to_off_kW_per_sec;
    
    double ramp_up_delay_sec;
    double ramp_up_kW_per_sec;
    
    double ramp_down_delay_sec;
    double ramp_down_kW_per_sec;
    
    pev_charge_ramping() {};
    pev_charge_ramping(double off_to_on_delay_sec_, double off_to_on_kW_per_sec_, double on_to_off_delay_sec_, double on_to_off_kW_per_sec_,
                       double ramp_up_delay_sec_, double ramp_up_kW_per_sec_, double ramp_down_delay_sec_, double ramp_down_kW_per_sec_);
};


struct pev_charge_ramping_workaround
{
    pev_charge_ramping pev_charge_ramping_obj;
    EV_type pev_type;
    EVSE_type SE_type;    
};


#endif

