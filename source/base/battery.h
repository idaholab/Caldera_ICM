
#ifndef inl_battery_H
#define inl_battery_H

#include <iostream>
#include <string>

#include "inputs.h"							// vehicle_charge_model_inputs
#include "helper.h"
#include "battery_integrate_X_in_time.h"
#include "battery_calculate_limits.h"

struct battery_state
{
	double soc_t1;
	double P1_kW;
	double P2_kW;
	double time_step_duration_hrs;
	energy_target_reached_status reached_target_status;
	double E1_energy_to_target_soc_kWh;
	double min_time_to_target_soc_hrs;
	
	static std::string get_file_header();
};

std::ostream& operator<<(std::ostream& out, battery_state& x);

class battery
{
private:
	calculate_E1_energy_limit  get_E1_limits_charging;
	calculate_E1_energy_limit  get_E1_limits_discharging;
	integrate_X_through_time   get_next_P2;
	
	double battery_size_kWh, soc, soc_to_energy, target_P2_kW;
	double zero_slope_threshold_bat_eff_vs_P2;
	bool will_never_discharge;
    double soc_of_full_battery, soc_of_empty_battery;

	line_segment bat_eff_vs_P2_charging;
	line_segment bat_eff_vs_P2_discharging;

	double get_zero_slope_threshold_bat_eff_vs_P2(const vehicle_charge_model_inputs& inputs);
	
public:
	// Debugging
	double max_E1_limit, min_E1_limit;
    bool print_debug_info;

	battery(const vehicle_charge_model_inputs& inputs);

	void set_soc_of_full_and_empty_battery(double soc_of_full_battery_, 
										   double soc_of_empty_battery_);
    
    // This should only be used for debugging purposes or maybe by battery_factory
    void set_P2_kW(double P2_kW);
    
    void set_target_P2_kW(double target_P2_kW_);
    double get_target_P2_kW();
	void get_next(double prev_unix_time, 
				  double now_unix_time, 
				  double target_soc, 
				  double pu_Vrms, 
				  bool stop_charging_at_target_soc, 
				  battery_state& return_val);
};


#endif



