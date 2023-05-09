
#ifndef inl_battery_H
#define inl_battery_H

#include <iostream>
#include <string>

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


struct battery_inputs
{
	const calculate_E1_energy_limit_inputs E1_limits_charging_inputs;
	const calculate_E1_energy_limit_inputs E1_limits_discharging_inputs;
	const integrate_X_through_time get_next_P2;
	const double battery_size_kWh;
	const double soc;
	const double zero_slope_threashold_bat_eff_vs_P2; 
	const bool will_never_discharge;
	const line_segment bat_eff_vs_P2_charging;
	const line_segment bat_eff_vs_P2_discharging;

	battery_inputs(const calculate_E1_energy_limit_inputs& E1_limits_charging_inputs,
		const calculate_E1_energy_limit_inputs& E1_limits_discharging_inputs,
		const integrate_X_through_time& get_next_P2, const double& battery_size_kWh, const double& soc,
		const double& zero_slope_threashold_bat_eff_vs_P2, const bool& will_never_discharge,
		const line_segment& bat_eff_vs_P2_charging, const line_segment& bat_eff_vs_P2_discharging)
		: E1_limits_charging_inputs(E1_limits_charging_inputs), E1_limits_discharging_inputs(E1_limits_discharging_inputs),
		get_next_P2(get_next_P2), battery_size_kWh(battery_size_kWh), soc(soc),
		zero_slope_threashold_bat_eff_vs_P2(zero_slope_threashold_bat_eff_vs_P2),
		will_never_discharge(will_never_discharge), bat_eff_vs_P2_charging(bat_eff_vs_P2_charging),
		bat_eff_vs_P2_discharging(bat_eff_vs_P2_discharging) {}
};

class battery
{
private:
	calculate_E1_energy_limit  get_E1_limits_charging;
	calculate_E1_energy_limit  get_E1_limits_discharging;
	integrate_X_through_time   get_next_P2;
	
	double battery_size_kWh, soc, soc_to_energy, target_P2_kW;
	double zero_slope_threashold_bat_eff_vs_P2;
	bool will_never_discharge;
    double soc_of_full_battery, soc_of_empty_battery;

	line_segment bat_eff_vs_P2_charging;
	line_segment bat_eff_vs_P2_discharging;
	
public:
	// Debugging
	double max_E1_limit, min_E1_limit;
    bool print_debug_info;

	battery(const battery_inputs& inputs);

	void set_soc_of_full_and_empty_battery(double soc_of_full_battery_, double soc_of_empty_battery_);
    
    // This should only be used for debugging purposes or maybe by battery_factory
    void set_P2_kW(double P2_kW);
    
    void set_target_P2_kW(double target_P2_kW_);
    double get_target_P2_kW();
	void get_next(double prev_unix_time, double now_unix_time, double target_soc, double pu_Vrms, bool stop_charging_at_target_soc, battery_state& return_val);
};


#endif



