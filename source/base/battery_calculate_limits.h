

#ifndef inl_battery_calculate_limits_H
#define inl_battery_calculate_limits_H

#include <vector>
#include <iostream>   	// Stream to consol (may be needed for files too???)

#include "helper.h"  // line_segment


//-----------------------------------------------

enum energy_target_reached_status
{
    can_not_reach_energy_target_this_timestep = 0,
    can_reach_energy_target_this_timestep = 1,
    have_passed_energy_target = 2,
    unknown = 3,
    target_P2_is_zero = 4
};
std::ostream& operator<<(std::ostream& out, energy_target_reached_status& x);


struct E1_energy_limit
{
	double target_soc;
    double max_E1_energy_kWh;
    double max_E1_energy_charge_time_hrs;
    energy_target_reached_status reached_target_status;
    double E1_energy_to_target_soc_kWh;
    double min_time_to_target_soc_hrs;
};
std::ostream& operator<<(std::ostream& out, E1_energy_limit& x);


//#############################################################################
//                 Algroithm soc_t1 from (t1-t0) and soc_t0  
//#############################################################################


class algorithm_P2_vs_soc
{
private:
	algorithm_P2_vs_soc& operator=(const algorithm_P2_vs_soc& obj) = default;
	
protected:
    algorithm_P2_vs_soc(const algorithm_P2_vs_soc& obj) = default;
    
    std::vector<line_segment> *P2_vs_soc;
    int seg_index, ref_seg_index;
    double soc_to_energy, exp_term, prev_exp_val;
	double recalc_exponent_threashold, zero_slope_threashold_P2_vs_soc;
	bool segment_is_flat_P2_vs_soc, P2_vs_soc_segments_changed;

public:
    algorithm_P2_vs_soc(){};
    algorithm_P2_vs_soc(double battery_size_kWh, double recalc_exponent_threashold_, double zero_slope_threashold_P2_vs_soc_);
    virtual algorithm_P2_vs_soc* clone() const = 0;
    
    void find_line_segment_index(double init_soc, bool &line_segment_not_found);    
    double get_soc_UB() const;
    double get_soc_LB() const;
    double get_soc_to_energy() const;
    void get_next_line_segment(bool is_charging_not_discharging, bool &next_line_segment_exists);
    void set_P2_vs_soc(std::vector<line_segment> *P2_vs_soc_);
    virtual double get_soc_t1(double t1_minus_t0_hrs, double soc_t0) = 0;
    virtual double get_time_to_soc_t1_hrs(double soc_t0, double soc_t1) = 0;
};


class algorithm_P2_vs_soc_no_losses:public algorithm_P2_vs_soc
{
private:
	algorithm_P2_vs_soc_no_losses& operator=(const algorithm_P2_vs_soc_no_losses& obj) = default;
	algorithm_P2_vs_soc_no_losses(const algorithm_P2_vs_soc_no_losses& obj);

    double a, b, A;
    
public:
    algorithm_P2_vs_soc_no_losses(){};
    algorithm_P2_vs_soc_no_losses(double battery_size_kWh, double recalc_exponent_threashold_, double zero_slope_threashold_P2_vs_soc_);
    virtual algorithm_P2_vs_soc_no_losses* clone() const override final;
    
    virtual double get_soc_t1(double t1_minus_t0_hrs, double soc_t0) override final;
    virtual double get_time_to_soc_t1_hrs(double soc_t0, double soc_t1) override final;
};


class algorithm_P2_vs_soc_losses:public algorithm_P2_vs_soc
{
private:
	algorithm_P2_vs_soc_losses& operator=(const algorithm_P2_vs_soc_losses& obj) = default;
	algorithm_P2_vs_soc_losses(const algorithm_P2_vs_soc_losses& obj);

    double a, b, c, d, A, B, C, D, z;
    line_segment bat_eff_vs_P2;

public:
    algorithm_P2_vs_soc_losses(){};
    algorithm_P2_vs_soc_losses(double battery_size_kWh, double recalc_exponent_threashold_,  double zero_slope_threashold_P2_vs_soc_, 
                               const line_segment& bat_eff_vs_P2_);
	virtual algorithm_P2_vs_soc_losses* clone() const override final;

    virtual double get_soc_t1(double t1_minus_t0_hrs, double soc_t0) override final;
    virtual double get_time_to_soc_t1_hrs(double soc_t0, double soc_t1) override final;
};


//#############################################################################
//                         Calculate Energy Limit
//#############################################################################

class calc_E1_energy_limit
{
private:
	calc_E1_energy_limit& operator=(const calc_E1_energy_limit& obj) = default;
	calc_E1_energy_limit(const calc_E1_energy_limit& obj) = default;

protected:
    algorithm_P2_vs_soc *P2_vs_soc_algorithm;
	
public:
    calc_E1_energy_limit(){};
    calc_E1_energy_limit(algorithm_P2_vs_soc *P2_vs_soc_algorithm_);
    virtual calc_E1_energy_limit* clone() const = 0;
    virtual ~calc_E1_energy_limit();
    
    virtual void get_E1_limit(double time_step_sec, double init_soc, double target_soc, bool P2_vs_soc_segments_changed, std::vector<line_segment> *P2_vs_soc_segments, E1_energy_limit& E1_limit) = 0;
};


class calc_E1_energy_limit_charging:public calc_E1_energy_limit
{
private:
    calc_E1_energy_limit_charging& operator=(const calc_E1_energy_limit_charging& obj) = default;
    calc_E1_energy_limit_charging(const calc_E1_energy_limit_charging& obj);
        
public:
    calc_E1_energy_limit_charging(){};
    calc_E1_energy_limit_charging(algorithm_P2_vs_soc *P2_vs_soc_algorithm_);
    virtual calc_E1_energy_limit_charging* clone() const override final;
    virtual ~calc_E1_energy_limit_charging() override final;
    
    virtual void get_E1_limit(double time_step_sec, double init_soc, double target_soc, bool P2_vs_soc_segments_changed, std::vector<line_segment> *P2_vs_soc_segments, E1_energy_limit& E1_limit) override final;
};


class calc_E1_energy_limit_discharging:public calc_E1_energy_limit
{
private:
	calc_E1_energy_limit_discharging& operator=(const calc_E1_energy_limit_discharging& obj) = default;
	calc_E1_energy_limit_discharging(const calc_E1_energy_limit_discharging& obj);

public:
    calc_E1_energy_limit_discharging(){};
    calc_E1_energy_limit_discharging(algorithm_P2_vs_soc *P2_vs_soc_algorithm_);
    virtual calc_E1_energy_limit_discharging* clone() const override final;
    virtual ~calc_E1_energy_limit_discharging() override final;
    
    virtual void get_E1_limit(double time_step_sec, double init_soc, double target_soc, bool P2_vs_soc_segments_changed, std::vector<line_segment> *P2_vs_soc_segments, E1_energy_limit& E1_limit) override final;
};


//#############################################################################
//             Calculate Energy Limit Upper and Lower Bound
//#############################################################################


class calculate_E1_energy_limit
{
private:
	calc_E1_energy_limit  *calc_E1_limit;
	poly_function_of_x  P2_vs_puVrms;
	std::vector<line_segment> orig_P2_vs_soc_segments;
	std::vector<line_segment> cur_P2_vs_soc_segments;
	
	double prev_P2_limit, max_abs_P2_in_P2_vs_soc_segments;
	double max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments;
	bool is_charging_not_discharging, prev_P2_limit_binding;
	
	void apply_P2_limit_to_P2_vs_soc_segments(double P2_limit);
	calculate_E1_energy_limit(const calculate_E1_energy_limit& obj) = default;

public:
	calculate_E1_energy_limit() {};
	calculate_E1_energy_limit(bool is_charging_not_discharging_, double max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments_,
	                          calc_E1_energy_limit *calc_E1_limit_, poly_function_of_x& P2_vs_puVrms_, std::vector<line_segment>& P2_vs_soc_segments_);
	calculate_E1_energy_limit& operator=(const calculate_E1_energy_limit& obj);
	~calculate_E1_energy_limit();

	void get_E1_limit(double time_step_sec, double init_soc, double target_soc, double pu_Vrms, E1_energy_limit& E1_limit);
	
	void log_cur_P2_vs_soc_segments(std::ostream& out);
};


#endif



