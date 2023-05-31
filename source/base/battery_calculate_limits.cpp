

#include "battery_calculate_limits.h"

#include <cmath>    	// abs, exp, log
#include <algorithm>	// sort()
#include <stdexcept>	// invalid_argument

std::ostream& operator<<(std::ostream& out, 
                         E1_energy_limit& x)
{
	out << x.target_soc << "," << x.max_E1_energy_kWh << "," << x.max_E1_energy_charge_time_hrs << "," << x.reached_target_status << "," << x.E1_energy_to_target_soc_kWh << "," << x.min_time_to_target_soc_hrs << std::endl;
	return out;
}


std::ostream& operator<<(std::ostream& out, 
                         energy_target_reached_status& x)
{
	if(x == 0)
		out << "can_not_reach_energy_target_this_timestep";
	else if(x == 1)
		out << "can_reach_energy_target_this_timestep";
	else if(x == 2)
		out << "have_passed_energy_target";
	else if(x == 3)
		out << "unknown";
	else if(x == 4)
		out << "target_P2_is_zero";
	
	return out;
}


//#############################################################################
//                 Algroithm soc_t1 from (t1-t0) and soc_t0  
//#############################################################################

//##############################
//        Parent Class
//##############################

double algorithm_P2_vs_soc::get_soc_to_energy() const
{
	return this->soc_to_energy;
}

double algorithm_P2_vs_soc::get_soc_UB() const
{
	return this->P2_vs_soc->at(seg_index).x_UB;
}

double algorithm_P2_vs_soc::get_soc_LB() const
{
	return this->P2_vs_soc->at(seg_index).x_LB;
}

algorithm_P2_vs_soc::algorithm_P2_vs_soc(const vehicle_charge_model_inputs& inputs)
    :P2_vs_soc{ std::make_shared<std::vector<line_segment> >() },
    seg_index{ 0 },
    ref_seg_index{ -1 },
    soc_to_energy{ inputs.battery_size_kWh / 100.0 },
    prev_exp_val{ -1 },
    exp_term{ std::exp(this->prev_exp_val) },
    recalc_exponent_threshold{ 0.00000001 },
    zero_slope_threshold_P2_vs_soc{ inputs.SOCP_factory.get_SOC_vs_P2_curves(inputs.EV, inputs.EVSE).zero_slope_threshold },
    segment_is_flat_P2_vs_soc{ false },
    P2_vs_soc_segments_changed{ false }
{
}

void algorithm_P2_vs_soc::set_P2_vs_soc(std::shared_ptr<std::vector<line_segment> > P2_vs_soc)
{
	this->P2_vs_soc_segments_changed = true;
	this->P2_vs_soc = P2_vs_soc;
}


void algorithm_P2_vs_soc::find_line_segment_index(double init_soc, 
                                                  bool &line_segment_not_found)
{
    this->seg_index = -1;
    for(int i=0; i< this->P2_vs_soc->size(); i++)
    {
        if(init_soc <= this->P2_vs_soc->at(i).x_UB)
        {
            this->seg_index = i;
            break;
        }
    }

    line_segment_not_found = (this->seg_index == -1);
}


void algorithm_P2_vs_soc::get_next_line_segment(bool is_charging_not_discharging, 
                                                bool &next_line_segment_exists)
{
    if(is_charging_not_discharging)
    {
        this->seg_index++;
        next_line_segment_exists = (this->seg_index < this->P2_vs_soc->size());
    }
    else
    {
        this->seg_index--;
        next_line_segment_exists = (this->seg_index >= 0);
    }
}


//##############################
//   Child Class  (No Losses)
//##############################

algorithm_P2_vs_soc_no_losses::algorithm_P2_vs_soc_no_losses(const battery_charge_mode& mode, 
                                                             const vehicle_charge_model_inputs& inputs)
    :algorithm_P2_vs_soc{ inputs },
    a{ 0.0 },
    b{ 0.0 },
    A{ 0.0 }
{
}

double algorithm_P2_vs_soc_no_losses::get_soc_t1(double t1_minus_t0_hrs, 
                                                 double soc_t0)
{
	bool update_vals = (this->ref_seg_index != this->seg_index) || (this->P2_vs_soc_segments_changed);	
	this->P2_vs_soc_segments_changed = false;

    if(update_vals)
    {
        this->ref_seg_index = this->seg_index;
        this->a = this->P2_vs_soc->at(this->seg_index).a;
        this->b = this->P2_vs_soc->at(this->seg_index).b;
        this->A = this->a/this->soc_to_energy;
        this->segment_is_flat_P2_vs_soc = std::abs(this->a) < this->zero_slope_threshold_P2_vs_soc;
        
        this->prev_exp_val = this->A*t1_minus_t0_hrs;
        this->exp_term = std::exp(this->prev_exp_val);
    }
    
    //-------------
    
    double soc_t1;
    
    if(this->segment_is_flat_P2_vs_soc)
    {
    	double P2_soc_t0 = this->a*soc_t0 + this->b;
        soc_t1 = soc_t0 + P2_soc_t0*t1_minus_t0_hrs/this->soc_to_energy;
    }
    else
    {
    	double cur_exp_val = this->A*t1_minus_t0_hrs;
    	
        if(this->recalc_exponent_threshold < std::abs(this->prev_exp_val - cur_exp_val))
        {
        	this->prev_exp_val = cur_exp_val;
            this->exp_term = std::exp(cur_exp_val);
        }
        
        double P2_soc_t1;
        
        P2_soc_t1 = this->exp_term*(this->a*soc_t0 + this->b);
        soc_t1 = (P2_soc_t1-this->b)/this->a;
    }
    
    return soc_t1;
}

double algorithm_P2_vs_soc_no_losses::get_time_to_soc_t1_hrs(double soc_t0, 
                                                             double soc_t1)
{
	bool update_vals = (this->ref_seg_index != this->seg_index) || (this->P2_vs_soc_segments_changed);
	this->P2_vs_soc_segments_changed = false;

    if(update_vals)
    {
        this->ref_seg_index = this->seg_index;
        this->a = this->P2_vs_soc->at(this->seg_index).a;
        this->b = this->P2_vs_soc->at(this->seg_index).b;
        this->A = this->a/this->soc_to_energy;
        this->segment_is_flat_P2_vs_soc = std::abs(this->a) < this->zero_slope_threshold_P2_vs_soc;
    }
    
    //-------------
    
    double tmp_hrs;
    
    if(this->segment_is_flat_P2_vs_soc)
    {
    	double P2_soc_t0 = this->a*soc_t0 + this->b;
        tmp_hrs = (soc_t1 - soc_t0)*this->soc_to_energy/P2_soc_t0;
    }
    else
        tmp_hrs = std::log((this->a*soc_t1 + this->b)/(this->a*soc_t0 + this->b))/this->A;
                          
    return tmp_hrs;
}


//##############################
//   Child Class  (Losses)
//##############################

algorithm_P2_vs_soc_losses::algorithm_P2_vs_soc_losses(const battery_charge_mode& mode, 
                                                       const vehicle_charge_model_inputs& inputs)
    :algorithm_P2_vs_soc{ inputs },
    bat_eff_vs_P2{ inputs.PE_factory.get_P2_vs_battery_eff(inputs.EV, mode).curve },
    a{ 0.0 },
    b{ 0.0 },
    c{ 0.0 },
    d{ 0.0 },
    A{ 0.0 },
    B{ 0.0 },
    C{ 0.0 },
    D{ 0.0 },
    z{ 0.0 }
{
}


double algorithm_P2_vs_soc_losses::get_soc_t1(double t1_minus_t0_hrs, 
                                              double soc_t0)
{
    bool update_vals = (this->ref_seg_index != this->seg_index) || (this->P2_vs_soc_segments_changed);
    this->P2_vs_soc_segments_changed = false;

    if(update_vals)
    {
        this->ref_seg_index = this->seg_index;
        this->a = this->P2_vs_soc->at(this->seg_index).a;
        this->b = this->P2_vs_soc->at(this->seg_index).b;
        this->c = this->bat_eff_vs_P2.a;
        this->d = this->bat_eff_vs_P2.b;
        
        this->A = this->a/this->soc_to_energy;
        this->B = this->b;
        this->C = this->c*this->a/this->soc_to_energy;
        this->D = this->c*this->b + this->d;
        this->z = this->B*this->C - this->A*this->D;
                
        this->segment_is_flat_P2_vs_soc = std::abs(this->a) < this->zero_slope_threshold_P2_vs_soc;
        
        this->prev_exp_val = this->z*t1_minus_t0_hrs;
        this->exp_term = std::exp(this->prev_exp_val);
    }
    
    //-------------
    
    double soc_t1, e_t0, eff_e_t0;
        
    e_t0 = soc_t0*this->soc_to_energy;
    eff_e_t0 = this->C*e_t0 + this->D;
    
    if(this->segment_is_flat_P2_vs_soc)
    {
    	double P2_soc_t0 = this->a*soc_t0 + this->b;
        soc_t1 = soc_t0 + eff_e_t0*P2_soc_t0*t1_minus_t0_hrs/this->soc_to_energy;
    }
    else
    {
    	double cur_exp_val = this->z*t1_minus_t0_hrs;
    	
        if(this->recalc_exponent_threshold < std::abs(this->prev_exp_val - cur_exp_val))
        {
        	this->prev_exp_val = cur_exp_val;
            this->exp_term = std::exp(cur_exp_val);
        }

        double e_t1, P2_e_t0, X;
        
        P2_e_t0 = this->A*e_t0 + this->B;
        X = this->exp_term * eff_e_t0 / P2_e_t0;
        e_t1 = (X*this->B - this->D)/(this->C - X*this->A);
        soc_t1 = e_t1/this->soc_to_energy;
 	}
    
    //-------------
    
    return soc_t1;
}


double algorithm_P2_vs_soc_losses::get_time_to_soc_t1_hrs(double soc_t0, 
                                                          double soc_t1)
{
	bool update_vals = (this->ref_seg_index != this->seg_index) || (this->P2_vs_soc_segments_changed);
	this->P2_vs_soc_segments_changed = false;

    if(update_vals)
    {
        this->ref_seg_index = this->seg_index;
        this->a = this->P2_vs_soc->at(this->seg_index).a;
        this->b = this->P2_vs_soc->at(this->seg_index).b;
        this->c = this->bat_eff_vs_P2.a;
        this->d = this->bat_eff_vs_P2.b;
        
        this->A = this->a/this->soc_to_energy;
        this->B = this->b;
        this->C = this->c*this->a/this->soc_to_energy;
        this->D = this->c*this->b + this->d;
        this->z = this->B*this->C - this->A*this->D;
                
        this->segment_is_flat_P2_vs_soc = std::abs(this->a) < this->zero_slope_threshold_P2_vs_soc;
    }
    
    //-------------
    
    double tmp_hrs, e_t0, eff_e_t0;
    
    e_t0 = soc_t0*this->soc_to_energy;
    eff_e_t0 = this->C*e_t0 + this->D;
    
    if(this->segment_is_flat_P2_vs_soc)
    {
    	double P2_soc_t0 = this->a*soc_t0 + this->b;
        tmp_hrs = (soc_t1-soc_t0)*this->soc_to_energy/(P2_soc_t0*eff_e_t0);
    }
    else
    {
        double e_t1, P2_e_t0, P2_e_t1, eff_e_t1;
        
        P2_e_t0 = this->A*e_t0 + this->B;
        e_t1 = soc_t1*this->soc_to_energy;
        P2_e_t1 = this->A*e_t1 + this->B;
        eff_e_t1 = this->C*e_t1 + this->D;
        
        tmp_hrs = std::log((eff_e_t1*P2_e_t0)/(eff_e_t0*P2_e_t1))/this->z;
    }
    return tmp_hrs;
}


//#############################################################################
//                         Calculate Energy Limits
//#############################################################################


calc_E1_energy_limit::calc_E1_energy_limit(const battery_charge_mode& mode, 
                                           const bool& are_battery_losses, 
                                           const vehicle_charge_model_inputs& inputs)
{
    if (are_battery_losses)
    {
        this->P2_vs_soc_algorithm = std::make_shared<algorithm_P2_vs_soc_losses>(mode, inputs);
    }
    else
    {
        this->P2_vs_soc_algorithm = std::make_shared<algorithm_P2_vs_soc_no_losses>(mode, inputs);
    }
}

//##############################
//           Charging
//##############################

calc_E1_energy_limit_charging::calc_E1_energy_limit_charging(const bool& are_battery_losses, 
                                                             const vehicle_charge_model_inputs& inputs)
    : calc_E1_energy_limit{ charging, are_battery_losses, inputs }
{
}

void calc_E1_energy_limit_charging::get_E1_limit(double time_step_sec, 
                                                 double init_soc, 
                                                 double target_soc, 
                                                 bool P2_vs_soc_segments_changed, 
                                                 std::shared_ptr<std::vector<line_segment> > P2_vs_soc, 
                                                 E1_energy_limit& E1_limit)
{
	if(P2_vs_soc_segments_changed)
    	this->P2_vs_soc_algorithm->set_P2_vs_soc(P2_vs_soc);
    
    bool line_segment_not_found;
    this->P2_vs_soc_algorithm->find_line_segment_index(init_soc, line_segment_not_found);
    
    if(line_segment_not_found)
    {
        E1_limit = {100, 0, 0, unknown, 0, -1}; // {target_soc, max_energy_kWh, max_energy_charge_time_hrs, reached_target_status, energy_to_target_soc_kWh, min_time_to_target_soc_hrs}
    	return;
    }
    
    //----------------------------------
    //        Calculate
    //----------------------------------
    
    target_soc = (100 < target_soc) ? 100 : target_soc;
    
    energy_target_reached_status energy_target_status = unknown;
    if(target_soc <= init_soc)
    {
        energy_target_status = have_passed_energy_target;
    	target_soc = 100;
    }
    
    double soc_UB, soc_t0, soc_t1, t1_minus_t0_hrs, tmp_hrs;
    double min_time_to_target_hrs, max_energy_charge_time_hrs;
    bool break_now, is_charging_not_discharging;
    bool next_line_segment_exists;
    is_charging_not_discharging = true;
    
    soc_t0 = init_soc;
    t1_minus_t0_hrs = time_step_sec/3600;
    max_energy_charge_time_hrs = 0;
    break_now = false;
    while(true)
    {
        soc_UB = this->P2_vs_soc_algorithm->get_soc_UB();
        soc_t1 = this->P2_vs_soc_algorithm->get_soc_t1(t1_minus_t0_hrs, soc_t0);                     
        
        if(100 < soc_t1)
        	soc_t1 = 100;
        
        // For the last segment (soc_UB > 100)
        // Since soc_t1 <= 100: if(soc_UB < soc_t1) will never be true for the last segment
        if(soc_UB < soc_t1)   
        	soc_t1 = soc_UB;
        else
            break_now = true;

        if(energy_target_status == unknown && soc_t0 <= target_soc && target_soc <= soc_t1)
        {
            energy_target_status = can_reach_energy_target_this_timestep;
            tmp_hrs = this->P2_vs_soc_algorithm->get_time_to_soc_t1_hrs(soc_t0, target_soc);
            min_time_to_target_hrs = max_energy_charge_time_hrs + tmp_hrs;
        }

        if(break_now)
        {
            max_energy_charge_time_hrs += t1_minus_t0_hrs;
            break;
        }
        else
        {
            tmp_hrs = this->P2_vs_soc_algorithm->get_time_to_soc_t1_hrs(soc_t0, soc_t1);
            max_energy_charge_time_hrs += tmp_hrs;
        
            t1_minus_t0_hrs -= tmp_hrs;
            soc_t0 = soc_t1;
            this->P2_vs_soc_algorithm->get_next_line_segment(is_charging_not_discharging, next_line_segment_exists);
            
            if(!next_line_segment_exists)
                break;
        }
    }
        
    if(energy_target_status == unknown)
        energy_target_status = can_not_reach_energy_target_this_timestep;
    
    //--------------------------------------------
    
    double soc_to_energy = this->P2_vs_soc_algorithm->get_soc_to_energy();
    
    E1_limit.target_soc = target_soc;
    E1_limit.max_E1_energy_charge_time_hrs = max_energy_charge_time_hrs;
    E1_limit.max_E1_energy_kWh = (soc_t1 - init_soc)*soc_to_energy;
    E1_limit.reached_target_status = energy_target_status;
    E1_limit.E1_energy_to_target_soc_kWh = (energy_target_status == can_reach_energy_target_this_timestep) ? (target_soc - init_soc)*soc_to_energy : 0;
    E1_limit.min_time_to_target_soc_hrs = (energy_target_status == can_reach_energy_target_this_timestep) ? min_time_to_target_hrs : -1;
}


//##############################
//        Discharging
//##############################

calc_E1_energy_limit_discharging::calc_E1_energy_limit_discharging(const bool& are_battery_losses, 
                                                                   const vehicle_charge_model_inputs& inputs)
    :calc_E1_energy_limit{ discharging, are_battery_losses, inputs }
{
}

void calc_E1_energy_limit_discharging::get_E1_limit(double time_step_sec, 
                                                    double init_soc, 
                                                    double target_soc, 
                                                    bool P2_vs_soc_segments_changed, 
                                                    std::shared_ptr<std::vector<line_segment> > P2_vs_soc, 
                                                    E1_energy_limit& E1_limit)
{
	if(P2_vs_soc_segments_changed)
    	this->P2_vs_soc_algorithm->set_P2_vs_soc(P2_vs_soc);
	
	bool line_segment_not_found;
    this->P2_vs_soc_algorithm->find_line_segment_index(init_soc, line_segment_not_found);
	
    if(line_segment_not_found)
    {
        E1_limit = {0, 0, 0, unknown, 0, -1}; // {target_soc, max_energy_kWh, max_energy_charge_time_hrs, reached_target_status, energy_to_target_soc_kWh, min_time_to_target_soc_hrs}
        return;
    }
	
    //----------------------------------
    //        Calculate
    //----------------------------------
    
    target_soc = (target_soc < 0) ? 0 : target_soc;
    
    energy_target_reached_status energy_target_status = unknown;
    if(init_soc <= target_soc)
    {
        energy_target_status = have_passed_energy_target;
    	target_soc = 0;
    }
    
    double soc_LB, soc_t0, soc_t1, t1_minus_t0_hrs, tmp_hrs;
    double min_time_to_target_hrs, max_energy_charge_time_hrs;
    bool break_now, is_charging_not_discharging;
    bool next_line_segment_exists;
    is_charging_not_discharging = false;
    
    soc_t0 = init_soc;
    t1_minus_t0_hrs = time_step_sec/3600;
    max_energy_charge_time_hrs = 0;
    break_now = false;
    while(true)
    {
        soc_LB = this->P2_vs_soc_algorithm->get_soc_LB();
        soc_t1 = this->P2_vs_soc_algorithm->get_soc_t1(t1_minus_t0_hrs, soc_t0);                     
        
        if(soc_t1 < 0)
        	soc_t1 = 0;
        
        // For the first segment (soc_LB < 0)
        // Since soc_t1 >= 0: if(soc_t1 < soc_LB) will never be true for the first segment
        if(soc_t1 < soc_LB)  
            soc_t1 = soc_LB;
        else
            break_now = true;

        if(energy_target_status == unknown && soc_t1 <= target_soc && target_soc <= soc_t0)
        {
            energy_target_status = can_reach_energy_target_this_timestep;
            tmp_hrs = this->P2_vs_soc_algorithm->get_time_to_soc_t1_hrs(soc_t0, target_soc);
            min_time_to_target_hrs = max_energy_charge_time_hrs + tmp_hrs;
        }

        if(break_now)
        {
            max_energy_charge_time_hrs += t1_minus_t0_hrs;
            break;
        }
        else
        {
            tmp_hrs = this->P2_vs_soc_algorithm->get_time_to_soc_t1_hrs(soc_t0, soc_t1);
            max_energy_charge_time_hrs += tmp_hrs;
        
            t1_minus_t0_hrs -= tmp_hrs;
            soc_t0 = soc_t1;
            this->P2_vs_soc_algorithm->get_next_line_segment(is_charging_not_discharging, next_line_segment_exists);
            
            if(!next_line_segment_exists)
                break;
        }
    }
    
    if(energy_target_status == unknown)
        energy_target_status = can_not_reach_energy_target_this_timestep;
    
    //--------------------------------------------
    
    double soc_to_energy = this->P2_vs_soc_algorithm->get_soc_to_energy();
    
    E1_limit.target_soc = target_soc;
    E1_limit.max_E1_energy_charge_time_hrs = max_energy_charge_time_hrs;
    E1_limit.max_E1_energy_kWh = (soc_t1 - init_soc)*soc_to_energy;
    E1_limit.reached_target_status = energy_target_status;
    E1_limit.E1_energy_to_target_soc_kWh = (energy_target_status == can_reach_energy_target_this_timestep) ? (target_soc - init_soc)*soc_to_energy : 0;
    E1_limit.min_time_to_target_soc_hrs = (energy_target_status == can_reach_energy_target_this_timestep) ? min_time_to_target_hrs : -1;
}


//#############################################################################
//             Calculate Energy Limit Upper and Lower Bound
//#############################################################################

calculate_E1_energy_limit::calculate_E1_energy_limit(const battery_charge_mode& mode, 
                                                     const vehicle_charge_model_inputs& inputs) 
    : mode{ mode },
    P2_vs_puVrms{ inputs.VP_factory.get_puVrms_vs_P2(inputs.EVSE, inputs.SE_P2_limit_kW) },
    max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments{ 0.5 },
    orig_P2_vs_soc_segments{ inputs.SOCP_factory.get_SOC_vs_P2_curves(inputs.EV, inputs.EVSE).curve }
{
    bool are_battery_losses = true;
    if (this->mode == charging)
    {
        this->calc_E1_limit = std::make_shared<calc_E1_energy_limit_charging>(are_battery_losses, inputs);
    }
    else
    {
        this->calc_E1_limit = std::make_shared<calc_E1_energy_limit_discharging>(are_battery_losses, inputs);
    }

    double min_soc = 1000;
    double max_soc = -1000;

    for (line_segment& x : this->orig_P2_vs_soc_segments)
    {
        if (x.x_LB < min_soc) min_soc = x.x_LB;
        if (x.x_UB > max_soc) max_soc = x.x_UB;
    }

    ASSERT(min_soc >= -0.01 || max_soc <= 100.1, "PROBLEM: The following rule (for the P2_vs_soc_segments datatype) has been broken: min_soc < 0 and 100 < max_soc.");

    //---------------------------------------------

    std::sort(this->orig_P2_vs_soc_segments.begin(), this->orig_P2_vs_soc_segments.end());

    this->cur_P2_vs_soc_segments = this->orig_P2_vs_soc_segments;

    //---------------------

    this->prev_P2_limit_binding = true;

    if (this->mode == charging)
        this->prev_P2_limit = -1000000000;
    else
        this->prev_P2_limit = 1000000000;

    //---------------------

    this->max_abs_P2_in_P2_vs_soc_segments = this->prev_P2_limit;

    double val_0, val_1, val_tmp;

    for(const line_segment& seg : this->cur_P2_vs_soc_segments)
    {
        val_0 = seg.a * seg.x_LB + seg.b;
        val_1 = seg.a * seg.x_UB + seg.b;

        if (this->mode == charging)
        {	// max(val_0, val_1, this->max_abs_P2_in_P2_vs_soc_segments)
            val_tmp = val_0 < val_1 ? val_1 : val_0;
            this->max_abs_P2_in_P2_vs_soc_segments = val_tmp < this->max_abs_P2_in_P2_vs_soc_segments ? this->max_abs_P2_in_P2_vs_soc_segments : val_tmp;
        }
        else
        {	// min(val_0, val_1, this->max_abs_P2_in_P2_vs_soc_segments)
            val_tmp = val_0 < val_1 ? val_0 : val_1;
            this->max_abs_P2_in_P2_vs_soc_segments = val_tmp < this->max_abs_P2_in_P2_vs_soc_segments ? val_tmp : this->max_abs_P2_in_P2_vs_soc_segments;
        }
    }
}

void calculate_E1_energy_limit::apply_P2_limit_to_P2_vs_soc_segments(double P2_limit)
{
	double soc_0, soc_1, soc_tmp, P_0, P_1, a, b;
    double x_LB, x_UB, m, c;

    std::vector<line_segment> new_P2_vs_soc_segments;
	for(int i=0; i<this->cur_P2_vs_soc_segments.size(); i++)
    {
        x_LB = this->cur_P2_vs_soc_segments.at(i).x_LB;     // SOC 0
        x_UB = this->cur_P2_vs_soc_segments.at(i).x_UB;     // SOC 1
        a = this->cur_P2_vs_soc_segments.at(i).a;
        b = this->cur_P2_vs_soc_segments.at(i).b;

        soc_0 = x_LB;
        soc_1 = x_UB;
        m = a;
        c = b;

        P_0 = m * soc_0 + c;        // y = mx + c;
        P_1 = m * soc_1 + c;
        
        if(this->mode == charging)
        {
		    if(P2_limit <= P_0 && P2_limit <= P_1)
		    {
		        a = 0;
		        b = P2_limit;
		    }
		    else if(P_0 < P2_limit && P2_limit < P_1)
		    {
		        soc_tmp = (P2_limit - c)/m;
		        x_UB = soc_tmp;
		        
                new_P2_vs_soc_segments.emplace_back(soc_tmp, soc_1, 0, P2_limit);
		    }
		    else if(P2_limit < P_0 && P_1 < P2_limit)
		    {
		        soc_tmp = (P2_limit - c)/m;
		        x_LB = soc_tmp;
		        
                new_P2_vs_soc_segments.emplace_back(soc_0, soc_tmp, 0, P2_limit);
		    }
		}
		else
		{
			if(P_0 <= P2_limit && P_1 <= P2_limit)
		    {
		        a = 0;
		        b = P2_limit;        
		    }
		    else if(P2_limit < P_0 && P_1 < P2_limit)
		    {
		        soc_tmp = (P2_limit - c)/m;
		        x_UB = soc_tmp;
		        
                new_P2_vs_soc_segments.emplace_back(soc_tmp, soc_1, 0, P2_limit);
		    }
		    else if(P_0 < P2_limit && P2_limit < P_1)
		    {
		        soc_tmp = (P2_limit - c)/m;
		        x_LB = soc_tmp;

                new_P2_vs_soc_segments.emplace_back(soc_0, soc_tmp, 0, P2_limit);
		    }
		}
        new_P2_vs_soc_segments.emplace_back(x_LB, x_UB, a, b);
    }
    
    std::sort(new_P2_vs_soc_segments.begin(), new_P2_vs_soc_segments.end());
    this->cur_P2_vs_soc_segments = new_P2_vs_soc_segments;
}


void calculate_E1_energy_limit::get_E1_limit(double time_step_sec, 
                                             double init_soc, 
                                             double target_soc, 
                                             double pu_Vrms, 
                                             E1_energy_limit& E1_limit)
{
	double P2_limit;
	bool P2_limit_binding, P2_vs_soc_segments_changed;
	
	P2_limit = this->P2_vs_puVrms.get_val(pu_Vrms);
	
	if(this->mode == charging)
	{
		P2_limit_binding = (P2_limit < this->max_abs_P2_in_P2_vs_soc_segments);
	}
	else
	{
		P2_limit_binding = (this->max_abs_P2_in_P2_vs_soc_segments < P2_limit);
	}

	//---------------------------
	
	P2_vs_soc_segments_changed = false;
	
	if(this->max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments < std::abs(this->prev_P2_limit - P2_limit))
	{
		this->prev_P2_limit = P2_limit;

		if(P2_limit_binding)
		{
			P2_vs_soc_segments_changed = true;
			this->cur_P2_vs_soc_segments = this->orig_P2_vs_soc_segments;
			this->apply_P2_limit_to_P2_vs_soc_segments(P2_limit);
		}
		else
		{
			if(this->prev_P2_limit_binding)
			{
				P2_vs_soc_segments_changed = true;
				this->cur_P2_vs_soc_segments = this->orig_P2_vs_soc_segments;
			}
		}
		
		this->prev_P2_limit_binding = P2_limit_binding;
	}
	
    std::shared_ptr<std::vector<line_segment>> P2_vs_soc_ptr = std::make_shared<std::vector<line_segment> >(this->cur_P2_vs_soc_segments);
	this->calc_E1_limit->get_E1_limit(time_step_sec, init_soc, target_soc, P2_vs_soc_segments_changed, P2_vs_soc_ptr, E1_limit);
}


void calculate_E1_energy_limit::log_cur_P2_vs_soc_segments(std::ostream& out)
{
	for(line_segment x: this->cur_P2_vs_soc_segments)
		out << x;
		
	out << "Number segments: " << this->cur_P2_vs_soc_segments.size() << std::endl << std::endl;
}