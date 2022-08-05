
//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								               DirectXFC Project
//--------------------------------------------------------------------------------------------------------------------
// ###################################################################################################################
//====================================================================================================================

#include "SE_EV_factory.h"

#include "supply_equipment_load.h"
#include "supply_equipment_control.h"
#include "battery_integrate_X_in_time.h"	// integrate_X_through_time
#include "helper.h"					        // rand_val, line_segment
#include "charge_profile_library.h"
#include "charge_profile_downsample_fragments.h"


#include <cmath>
#include <vector>
#include <algorithm>						// sort
#include <iostream>							// cout

//#############################################################################
//     Get Battery Efficiency vs P2  (Should be the same in every factory)
//#############################################################################

void get_bat_eff_vs_P2(bool is_charging_not_discharging, battery_chemistry bat_chem, double battery_size_kWh, double& zero_slope_threashold_bat_eff_vs_P2, line_segment& bat_eff_vs_P2)
{
	// When are_losses = false
	// 	- The bat_eff_vs_P2 line segmnet should have the following values (c=0, d=1).

	if(bat_chem == LTO)
	{
		if(is_charging_not_discharging)
			bat_eff_vs_P2 = { 0, 6*battery_size_kWh, -0.0078354/battery_size_kWh, 0.987448};  // x_LB, x_UB, a, b
		else
			bat_eff_vs_P2 = {-6*battery_size_kWh, 0, -0.0102411/battery_size_kWh, 1.0109224};  // x_LB, x_UB, a, b
	}
	else if(bat_chem == LMO)
	{
		if(is_charging_not_discharging)
			bat_eff_vs_P2 = { 0, 4*battery_size_kWh, -0.0079286/battery_size_kWh, 0.9936637};  // x_LB, x_UB, a, b
		else
			bat_eff_vs_P2 = {-4*battery_size_kWh, 0, -0.0092091/battery_size_kWh, 1.005674};  // x_LB, x_UB, a, b		
	}
	else if(bat_chem == NMC)
	{
		if(is_charging_not_discharging)
			bat_eff_vs_P2 = {0, 4*battery_size_kWh, -0.0053897/battery_size_kWh, 0.9908405};  // x_LB, x_UB, a, b
		else
			bat_eff_vs_P2 = {-4*battery_size_kWh, 0, -0.0062339/battery_size_kWh, 1.0088727};  // x_LB, x_UB, a, b
	}

	//-----------------------------------
	
		// If the slope is smaller than 0.000001 that the 'safe' method will be used.
		// Very little rational to using 0.000001 it will allow the complex method using a 1000 kWh battery pack
	if(std::abs(bat_eff_vs_P2.a) < 0.000001)
		zero_slope_threashold_bat_eff_vs_P2 = 0.000001;
	else
		zero_slope_threashold_bat_eff_vs_P2 = 0.9 * std::abs(bat_eff_vs_P2.a);
}


//#############################################################################
//                         EV Charge Model Factory
//#############################################################################


factory_EV_charge_model::factory_EV_charge_model()
{
	this->model_stochastic_battery_degregation = false;
}


void factory_EV_charge_model::set_bool_model_stochastic_battery_degregation(bool model_stochastic_battery_degregation_)
{
	this->model_stochastic_battery_degregation = model_stochastic_battery_degregation_;
}


void factory_EV_charge_model::initialize_custome_parameters(std::map<vehicle_enum, pev_charge_ramping> ramping_by_pevType_only_, std::map< std::tuple<vehicle_enum, supply_equipment_enum>, pev_charge_ramping> ramping_by_pevType_seType_)
{
    this->ramping_by_pevType_only = ramping_by_pevType_only_;
    this->ramping_by_pevType_seType = ramping_by_pevType_seType_;
}


void factory_EV_charge_model::get_integrate_X_through_time_obj(vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type, integrate_X_through_time& return_val)
{
	// Each transition_of_X_through_time must have at least 3 segments.

	// struct transition_goto_next_segment_criteria
	// {
    	//	transition_criteria_type criteria_type;
    	//	double criteria_value;
    	//	bool inturupt_this_transition_if_target_X_deviation_limit_exceeded;
    	//	double target_X_deviation_limit_to_interupt_this_transition;
    	//	double segment_slope_X_per_sec;
 	// };

	double X_deadband, target_deadband, off_deadband;
	bool pos_and_neg_transitions_are_unique = false;
	
	std::vector<transition_goto_next_segment_criteria> goto_next_seg;
	
	X_deadband = 0.1;
	target_deadband = 0.01;
	off_deadband = 0.0001;
	
	goto_next_seg.clear();
	goto_next_seg.push_back({time_delay_sec, 0.1, false, 0, 0});
	goto_next_seg.push_back({time_delay_sec, 0.1, false, 0, 0});
	goto_next_seg.push_back({from_final_X, 0, false, 0, 10});
	transition_of_X_through_time default_to_pos_inf(moving_toward_pos_inf, X_deadband, goto_next_seg);
	
	goto_next_seg.clear();
	goto_next_seg.push_back({time_delay_sec, 0.1, false, 0, 0});
	goto_next_seg.push_back({time_delay_sec, 0.1, false, 0, 0});
	goto_next_seg.push_back({from_final_X, 0, false, 0, -10});
	transition_of_X_through_time default_to_neg_inf(moving_toward_neg_inf, X_deadband, goto_next_seg);
	
	//================================
	//  All Default V2G Transitions
	//================================
	
	//----------------------
	//     off_to_neg
	//----------------------
	transition_of_X_through_time off_to_neg_obj;
	off_to_neg_obj = default_to_neg_inf;
	
	//----------------------
	//     neg_to_off
	//----------------------
	transition_of_X_through_time neg_to_off_obj;
	neg_to_off_obj = default_to_pos_inf;
		
	//---------------------------
	// pos_moving_toward_pos_inf
	//---------------------------	
	transition_of_X_through_time pos_moving_toward_pos_inf_obj;
	pos_moving_toward_pos_inf_obj = default_to_pos_inf;

	//---------------------------
	// pos_moving_toward_neg_inf
	//---------------------------
	transition_of_X_through_time pos_moving_toward_neg_inf_obj;
	pos_moving_toward_neg_inf_obj = default_to_neg_inf;

	//---------------------------
	// neg_moving_toward_pos_inf
	//---------------------------
	transition_of_X_through_time neg_moving_toward_pos_inf_obj;
	neg_moving_toward_pos_inf_obj = default_to_pos_inf;

	//---------------------------
	// neg_moving_toward_neg_inf
	//---------------------------
	transition_of_X_through_time neg_moving_toward_neg_inf_obj;
	neg_moving_toward_neg_inf_obj = default_to_neg_inf;
	
	//----------------------------------
	
	if(supply_equipment_is_L1(supply_equipment_type))
	{
		X_deadband = 0.01;
		target_deadband = 0.01;
		off_deadband = 0.0001;
	
		//----------------------
		//     off_to_pos
		//----------------------
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 4.95, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.05, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, 0.5});
		transition_of_X_through_time off_to_pos_obj(off_to_pos, X_deadband, goto_next_seg);
        
		//----------------------
		//     pos_to_off
		//----------------------		
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 0.095, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.005, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, -50});
		transition_of_X_through_time pos_to_off_obj(pos_to_off, X_deadband, goto_next_seg);
		
		//-----------------------
		// moving_toward_pos_inf
		//-----------------------
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 0.12, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.03, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, 0.5});
		transition_of_X_through_time moving_toward_pos_inf_obj(moving_toward_pos_inf, X_deadband, goto_next_seg);
		
		//-----------------------
		// moving_toward_neg_inf
		//-----------------------
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 0.09, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.01, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, -0.5});
		transition_of_X_through_time moving_toward_neg_inf_obj(moving_toward_neg_inf, X_deadband, goto_next_seg);
		
		//-----------------------
		
		integrate_X_through_time integrate_obj(target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
				                           	   pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
				        				       pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj);
		return_val = integrate_obj;
	}
	else if(supply_equipment_is_L2(supply_equipment_type))
	{
		X_deadband = 0.01;
		target_deadband = 0.01;
		off_deadband = 0.0001;
	
		//----------------------
		//     off_to_pos
		//----------------------		
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 4.95, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.05, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, 2});
		transition_of_X_through_time off_to_pos_obj(off_to_pos, X_deadband, goto_next_seg);
		
		//----------------------
		//     pos_to_off
		//----------------------
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 0.095, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.005, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, -100});
		transition_of_X_through_time pos_to_off_obj(pos_to_off, X_deadband, goto_next_seg);
		
		//-----------------------
		// moving_toward_pos_inf
		//-----------------------
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 0.12, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.03, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, 2});
		transition_of_X_through_time moving_toward_pos_inf_obj(moving_toward_pos_inf, X_deadband, goto_next_seg);
		
		//-----------------------
		// moving_toward_neg_inf
		//-----------------------
		goto_next_seg.clear();
		goto_next_seg.push_back({time_delay_sec, 0.09, false, 0, 0});
		goto_next_seg.push_back({time_delay_sec, 0.01, false, 0, 0});
		goto_next_seg.push_back({from_final_X, 0, false, 0, -3});
		transition_of_X_through_time moving_toward_neg_inf_obj(moving_toward_neg_inf, X_deadband, goto_next_seg);
		
		//-----------------------
		
		integrate_X_through_time integrate_obj(target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
				                           	   pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
				        				       pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj);
		return_val = integrate_obj;
	}
	else
	{
        if(vehicle_type == phev20 || vehicle_type == phev50 || vehicle_type == phev_SUV)
            std::cout << "WARNING: PHEV can not fast charge.  EV_enum :" << vehicle_type << " and SE_enum:" << supply_equipment_type << std::endl;
		
		X_deadband = 0.01;
		target_deadband = 0.01;
		off_deadband = 0.0001;
	
        //----------------------------
        
        bool use_custom_ramping_values = false;
        pev_charge_ramping custom_charge_ramping;

        std::tuple<vehicle_enum, supply_equipment_enum> pev_se_pair(vehicle_type, supply_equipment_type);
        
        if(this->ramping_by_pevType_seType.count(pev_se_pair) > 0)
        {
            use_custom_ramping_values = true;
            custom_charge_ramping = this->ramping_by_pevType_seType[pev_se_pair];
        }
        else if(this->ramping_by_pevType_only.count(vehicle_type) > 0)
        {
            use_custom_ramping_values = true;
            custom_charge_ramping = this->ramping_by_pevType_only[vehicle_type];
        }
        
        //----------------------------
        
        if(use_custom_ramping_values)
        {  
            double delay_sec, ramping_kW_per_sec;
            
            //----------------------
            //     pos_to_off
            //----------------------
            delay_sec = custom_charge_ramping.on_to_off_delay_sec;
            ramping_kW_per_sec = custom_charge_ramping.on_to_off_kW_per_sec;
            
            goto_next_seg.clear();
            goto_next_seg.push_back({time_delay_sec, 0.9*delay_sec, false, 0, 0});
            goto_next_seg.push_back({time_delay_sec, 0.1*delay_sec, false, 0, 0});
            goto_next_seg.push_back({from_final_X, 0, false, 0, ramping_kW_per_sec});
            transition_of_X_through_time pos_to_off_obj(pos_to_off, X_deadband, goto_next_seg);
            
            //----------------------
            //     off_to_pos
            //----------------------
            delay_sec = custom_charge_ramping.off_to_on_delay_sec;
            ramping_kW_per_sec = custom_charge_ramping.off_to_on_kW_per_sec;
            
            goto_next_seg.clear();
            goto_next_seg.push_back({time_delay_sec, 0.9*delay_sec, false, 0, 0});
            goto_next_seg.push_back({time_delay_sec, 0.1*delay_sec, false, 0, 0});
            goto_next_seg.push_back({from_final_X, 0, false, 0, ramping_kW_per_sec});
            transition_of_X_through_time off_to_pos_obj(off_to_pos, X_deadband, goto_next_seg);
        
            //-----------------------
            // moving_toward_pos_inf
            //-----------------------
            delay_sec = custom_charge_ramping.ramp_up_delay_sec;
            ramping_kW_per_sec = custom_charge_ramping.ramp_up_kW_per_sec;
            
            goto_next_seg.clear();
            goto_next_seg.push_back({time_delay_sec, 0.9*delay_sec, false, 0, 0});
            goto_next_seg.push_back({time_delay_sec, 0.1*delay_sec, false, 0, 0});
            goto_next_seg.push_back({from_final_X, 0, false, 0, ramping_kW_per_sec});
            transition_of_X_through_time moving_toward_pos_inf_obj(moving_toward_pos_inf, X_deadband, goto_next_seg);
        
            //-----------------------
            // moving_toward_neg_inf
            //-----------------------
            delay_sec = custom_charge_ramping.ramp_down_delay_sec;
            ramping_kW_per_sec = custom_charge_ramping.ramp_down_kW_per_sec;
            
            goto_next_seg.clear();
            goto_next_seg.push_back({time_delay_sec, 0.9*delay_sec, false, 0, 0});
            goto_next_seg.push_back({time_delay_sec, 0.1*delay_sec, false, 0, 0});
            goto_next_seg.push_back({from_final_X, 0, false, 0, ramping_kW_per_sec});
            transition_of_X_through_time moving_toward_neg_inf_obj(moving_toward_neg_inf, X_deadband, goto_next_seg);
    
            //-----------------------
            
            integrate_X_through_time integrate_obj(target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
                                   pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
                                   pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj);
            return_val = integrate_obj;
        }
        else
        {
            //----------------------
            //     pos_to_off
            //----------------------
            goto_next_seg.clear();
            
            // Default Values
            if(supply_equipment_type == dcfc_50)
            {
                goto_next_seg.push_back({time_delay_sec, 0.040, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec, 0.010, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, -3700});
            }
            else if(supply_equipment_type == xfc_150 || supply_equipment_type == xfc_350)
            {
                goto_next_seg.push_back({time_delay_sec, 0.040, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec, 0.010, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, -140000});
            }
            else
                std::cout << "WARNING: Incomplete definition for get_integrate_X_through_time_obj in the factory_EV_charge_model for EV_enum :" << vehicle_type << " and SE_enum:" << supply_equipment_type << std::endl;
            
            transition_of_X_through_time pos_to_off_obj(pos_to_off, X_deadband, goto_next_seg);
            
            //----------------------------------------------------------------
            //     off_to_pos,  moving_toward_pos_inf,  moving_toward_neg_inf
            //----------------------------------------------------------------
        
            if(vehicle_type == bev150_ld1_50kW || vehicle_type == bev250_ld1_75kW)
            {
                //----------------------
                //     off_to_pos
                //----------------------		
                goto_next_seg.clear();
                goto_next_seg.push_back({time_delay_sec, 14.9, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec,  0.1, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, 10});
                transition_of_X_through_time off_to_pos_obj(off_to_pos, X_deadband, goto_next_seg);
            
                //-----------------------
                // moving_toward_pos_inf
                //-----------------------
                goto_next_seg.clear();
                goto_next_seg.push_back({time_delay_sec, 0.09, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec, 0.01, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, 10});
                transition_of_X_through_time moving_toward_pos_inf_obj(moving_toward_pos_inf, X_deadband, goto_next_seg);
            
                //-----------------------
                // moving_toward_neg_inf
                //-----------------------
                goto_next_seg.clear();
                goto_next_seg.push_back({time_delay_sec, 0.09, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec, 0.01, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, -10});
                transition_of_X_through_time moving_toward_neg_inf_obj(moving_toward_neg_inf, X_deadband, goto_next_seg);
                
                //--------------------------------
                
                integrate_X_through_time integrate_obj(target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
                                                   pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
                                                   pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj);
                return_val = integrate_obj;
            }
            else
            {
                //----------------------
                //     off_to_pos
                //----------------------		
                goto_next_seg.clear();
                goto_next_seg.push_back({time_delay_sec, 14.9, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec,  0.1, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, 25});
                transition_of_X_through_time off_to_pos_obj(off_to_pos, X_deadband, goto_next_seg);
            
                //-----------------------
                // moving_toward_pos_inf
                //-----------------------
                goto_next_seg.clear();
                goto_next_seg.push_back({time_delay_sec, 0.09, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec, 0.01, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, 25});
                transition_of_X_through_time moving_toward_pos_inf_obj(moving_toward_pos_inf, X_deadband, goto_next_seg);
            
                //-----------------------
                // moving_toward_neg_inf
                //-----------------------
                goto_next_seg.clear();
                goto_next_seg.push_back({time_delay_sec, 0.09, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec, 0.01, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, -25});
                transition_of_X_through_time moving_toward_neg_inf_obj(moving_toward_neg_inf, X_deadband, goto_next_seg);
                
                //--------------------------------
                
                integrate_X_through_time integrate_obj(target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
                                                   pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
                                                   pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj);
                return_val = integrate_obj;
            }
        }
    }
}


void factory_EV_charge_model::get_P2_vs_puVrms(bool is_charging_not_discharging, vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type, double SE_P2_limit_atNominalV_kW, poly_function_of_x& P2_vs_puVrms)
{	// The points on the P2_vs_pu_Vrms plot are all multiplied by SE_P2_limit_atNominalV_kW
	// The P2_vs_pu_Vrms plot must pass through the point (1, 1)
	//		At nominal voltage Vrms = 1 the final curve is 1*SE_P2_limit_atNominalV_kW or the limit at nominal voltage
		
	std::vector<point_P2_vs_puVrms> points;
	
	if(supply_equipment_is_L1(supply_equipment_type))
	{
		points.push_back({0, 0});
		points.push_back({0.69, 0});
		points.push_back({0.7, 0.7});
		points.push_back({2, 2});	
	}
	else if(supply_equipment_is_L2(supply_equipment_type))
	{
		points.push_back({0, 0});
		points.push_back({0.34, 0});
		points.push_back({0.35, 0.373});
		points.push_back({0.94, 1});
		points.push_back({2, 1});
	}
	else
	{
		points.push_back({0, 0});
		points.push_back({0.79, 0});
		points.push_back({0.80, 1});
		points.push_back({1.20, 1});
		points.push_back({1.21, 0});
		points.push_back({2.00, 0});
	}
    
	//-------------------------	
	
	std::vector<point_P2_vs_puVrms> points_scaled;
	
	for(point_P2_vs_puVrms x : points)
		points_scaled.push_back({x.puVrms, x.P2_val * SE_P2_limit_atNominalV_kW});	
	
	//----------------------------------------------------
	// Convert Final Points to P2_vs_puVrms poly_function
	//----------------------------------------------------
	
	std::sort(points_scaled.begin(), points_scaled.end());
	
    double a, b;
	std::vector<poly_segment> segments;
	
	for(int i=1; i<points_scaled.size(); i++)
	{
		a = (points_scaled[i-1].P2_val - points_scaled[i].P2_val) / (points_scaled[i-1].puVrms - points_scaled[i].puVrms);
		b = points_scaled[i].P2_val - a*points_scaled[i].puVrms;
	
		segments.push_back({points_scaled[i-1].puVrms, points_scaled[i].puVrms, first, a, b, 0, 0, 0});
	}	// x_LB, x_UB, degree, a, b, c, d, e
	
	//--------

	double x_tolerance = 0.0001;
	bool take_abs_of_x = false;
    bool if_x_is_out_of_bounds_print_warning_message = true;
	poly_function_of_x  return_val(x_tolerance, take_abs_of_x, if_x_is_out_of_bounds_print_warning_message, segments, "P2_vs_puVrms");
	
	P2_vs_puVrms = return_val;	
}


void factory_EV_charge_model::get_P2_vs_soc(bool is_charging_not_discharging, vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type, double& zero_slope_threashold_P2_vs_soc, std::vector<line_segment>& P2_vs_soc_segments)
{
	// P2 vs soc must be defigned a little below 0 soc and a little above 100 soc.

	// When a battery is approaching 0 soc or 100 soc there is a chance that energy continues to go into the battery while
	// the soc is not changing (fixed at 0 or 100 soc)
	// 	- This is caused by the fact that when a battery stopps charging/discharging there is a ramp down period.
	// 	- This problem (for small time steps) can be mitigated by the following:
	//		- Make sure the P2_vs_soc curve decreases to zero as soc approaches 0 or 100 soc
	//		- Make sure the ramp rate is large when a battery stops charging/discharging
	
	vehicle_enum EV_enum = vehicle_type;
	supply_equipment_enum SE_enum = supply_equipment_type;
	P2_vs_soc_segments.clear();
	
	if(supply_equipment_is_L1(SE_enum) || supply_equipment_is_L2(SE_enum))
	{
        if(EV_enum == bev250_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 10.58});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev300_575kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 10.58});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev300_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 10.58});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev250_350kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 10.58});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev300_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 10.58});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev150_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 98.72484033603, 0.0, 8.832});
            P2_vs_soc_segments.push_back({98.72484033603, 100.1, -4.697368420791666, 472.57894734216666});
            zero_slope_threashold_P2_vs_soc = 4.2276315787125;
            // min_non_zero_slope = 4.697368420791666
        }
        else if(EV_enum == bev250_ld2_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 99.44670348141902, 0.0, 10.58});
            P2_vs_soc_segments.push_back({99.44670348141902, 100.1, -9.133771930033333, 918.9035087929333});
            zero_slope_threashold_P2_vs_soc = 8.22039473703;
            // min_non_zero_slope = 9.133771930033333
        }
        else if(EV_enum == bev200_ld4_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 8.832});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev275_ld1_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 8.832});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev250_ld1_75kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 6.072});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == bev150_ld1_50kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 99.31240336127273, 0.0, 6.072});
            P2_vs_soc_segments.push_back({99.31240336127273, 100.1, -4.697368420791666, 472.57894734216666});
            zero_slope_threashold_P2_vs_soc = 4.2276315787125;
            // min_non_zero_slope = 4.697368420791666
        }
        else if(EV_enum == phev_SUV)
        {
            P2_vs_soc_segments.push_back({-0.1, 97.60505263157894, 0.0, 8.832});
            P2_vs_soc_segments.push_back({97.60505263157894, 100.1, -2.9440104166666665, 296.18229166666663});
            zero_slope_threashold_P2_vs_soc = 2.649609375;
            // min_non_zero_slope = 2.9440104166666665
        }
        else if(EV_enum == phev50)
        {
            P2_vs_soc_segments.push_back({-0.1, 99.03512583355923, 0.0, 3.016365});
            P2_vs_soc_segments.push_back({99.03512583355923, 100.1, -1.9213541666666665, 193.29791666666665});
            zero_slope_threashold_P2_vs_soc = 1.7292187499999998;
            // min_non_zero_slope = 1.9213541666666665
        }
        else if(EV_enum == phev20)
        {
            P2_vs_soc_segments.push_back({-0.1, 95.7383018487395, 0.0, 3.016365});
            P2_vs_soc_segments.push_back({95.7383018487395, 100.1, -0.6197916666666666, 62.354166666666664});
            zero_slope_threashold_P2_vs_soc = 0.5578124999999999;
            // min_non_zero_slope = 0.6197916666666666
        } 
	}
	else
	{
        if(SE_enum == xfc_350 && EV_enum == bev250_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 4.329714912379645, 322.71842106000804});
            P2_vs_soc_segments.push_back({2.0, 3.0, 3.3686540570945818, 324.6405427705782});
            P2_vs_soc_segments.push_back({3.0, 50.0, 0.5091449118257467, 333.21907020638463});
            P2_vs_soc_segments.push_back({50.0, 60.0, 1.3249342105566044, 292.4296052698418});
            P2_vs_soc_segments.push_back({60.0, 80.0, 0.8413815789665984, 321.4427631652421});
            P2_vs_soc_segments.push_back({80.0, 86.53033106641354, 1.8254111842522565, 242.7203947423895});
            P2_vs_soc_segments.push_back({86.53033106641354, 100.1, -28.973684211188562, 2907.776315855936});
            zero_slope_threashold_P2_vs_soc = 0.45823042064317204;
            // min_non_zero_slope = 0.5091449118257467
        }
        else if(SE_enum == xfc_350 && EV_enum == bev300_575kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 8.930107641345186, 353.0572604682489});
            P2_vs_soc_segments.push_back({2.0, 50.0, 0.5469814091946925, 369.8235129325499});
            P2_vs_soc_segments.push_back({50.0, 60.0, 1.575766146201767, 318.38427608219615});
            P2_vs_soc_segments.push_back({60.0, 80.0, 0.9453830731008894, 356.2072604682488});
            P2_vs_soc_segments.push_back({80.0, 89.35681205228872, 1.7902080376146545, 288.6212633071476});
            P2_vs_soc_segments.push_back({89.35681205228872, 100.1, -40.94999999999999, 4107.749999999998});
            zero_slope_threashold_P2_vs_soc = 0.4922832682752233;
            // min_non_zero_slope = 0.5469814091946925
        }
        else if(SE_enum == xfc_350 && EV_enum == bev300_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 6.0720263129870125, 319.25399985265193});
            P2_vs_soc_segments.push_back({2.0, 3.0, 2.2167993410821256, 326.9644537964617});
            P2_vs_soc_segments.push_back({3.0, 50.0, 0.4924432388544657, 332.13752210314465});
            P2_vs_soc_segments.push_back({50.0, 60.0, 1.3922999993573966, 287.14468407799814});
            P2_vs_soc_segments.push_back({60.0, 80.0, 0.853484210132399, 319.473631431498});
            P2_vs_soc_segments.push_back({80.0, 87.9268896067988, 1.718822367627764, 250.2465788318688});
            P2_vs_soc_segments.push_back({87.9268896067988, 100.1, -32.28496239111427, 3240.093607527127});
            zero_slope_threashold_P2_vs_soc = 0.44319891496901914;
            // min_non_zero_slope = 0.4924432388544657
        }
        else if(SE_enum == xfc_350 && EV_enum == bev250_350kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 11.854583333333334, 256.74});
            P2_vs_soc_segments.push_back({3.0, 10.0, 2.9833928571428543, 283.35357142857146});
            P2_vs_soc_segments.push_back({10.0, 74.77040325345465, 0.5820394736842108, 307.3671052631579});
            P2_vs_soc_segments.push_back({74.77040325345465, 93.0, -15.168269230769234, 1485.0240384615388});
            P2_vs_soc_segments.push_back({93.0, 100.1, -9.553571428571427, 962.8571428571427});
            zero_slope_threashold_P2_vs_soc = 0.5238355263157898;
            // min_non_zero_slope = 0.5820394736842108
        }
        else if(SE_enum == xfc_350 && EV_enum == bev300_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 10.099631574286015, 220.47726305613594});
            P2_vs_soc_segments.push_back({3.0, 10.0, 2.5590451116008492, 243.09902244419146});
            P2_vs_soc_segments.push_back({10.0, 73.77447156057245, 0.4987894734540005, 263.7015788256599});
            P2_vs_soc_segments.push_back({73.77447156057245, 93.0, -12.45394736267308, 1219.2828941740968});
            P2_vs_soc_segments.push_back({93.0, 100.1, -7.843984958785712, 790.5563906125712});
            zero_slope_threashold_P2_vs_soc = 0.44891052610860044;
            // min_non_zero_slope = 0.4987894734540005
        }
        else if(SE_enum == xfc_350 && EV_enum == bev150_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.99957894709066, 110.694315783324});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.2821954886505722, 121.84646615864426});
            P2_vs_soc_segments.push_back({10.0, 71.79222115564788, 0.2495069251938948, 132.17335179321103});
            P2_vs_soc_segments.push_back({71.79222115564788, 93.0, -5.747975708182693, 562.7459513857405});
            P2_vs_soc_segments.push_back({93.0, 100.1, -3.620300751678571, 364.8721804308571});
            zero_slope_threashold_P2_vs_soc = 0.22455623267450533;
            // min_non_zero_slope = 0.2495069251938948
        }
        else if(SE_enum == xfc_350 && EV_enum == bev250_ld2_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 10.013684210755194, 222.96105263667525});
            P2_vs_soc_segments.push_back({3.0, 10.0, 2.5805263158484584, 245.26052632139545});
            P2_vs_soc_segments.push_back({10.0, 70.91530947121583, 0.5018282548591153, 266.0475069312889});
            P2_vs_soc_segments.push_back({70.91530947121583, 93.0, -11.17661943345385, 1094.228238891408});
            P2_vs_soc_segments.push_back({93.0, 100.1, -7.039473684371428, 709.4736842267428});
            zero_slope_threashold_P2_vs_soc = 0.4516454293732038;
            // min_non_zero_slope = 0.5018282548591153
        }
        else if(SE_enum == xfc_350 && EV_enum == bev200_ld4_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.387833333333332, 110.026});
            P2_vs_soc_segments.push_back({3.0, 4.0, 3.0039285714285713, 114.17771428571429});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.0604285714285697, 121.9517142857143});
            P2_vs_soc_segments.push_back({10.0, 85.72163996478638, 0.24300000000000002, 130.12599999999998});
            P2_vs_soc_segments.push_back({85.72163996478638, 93.0, -12.565517241379332, 1228.0931034482778});
            P2_vs_soc_segments.push_back({93.0, 100.1, -7.6428571428571415, 770.2857142857141});
            zero_slope_threashold_P2_vs_soc = 0.21870000000000003;
            // min_non_zero_slope = 0.24300000000000002
        }
        else if(SE_enum == xfc_350 && EV_enum == bev275_ld1_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.645111841936343, 109.2933947328678});
            P2_vs_soc_segments.push_back({3.0, 4.0, 2.4053712405140546, 116.01261653713465});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.1287199247709812, 121.11922180010694});
            P2_vs_soc_segments.push_back({10.0, 83.94626112545511, 0.24423785424213065, 129.96404250539547});
            P2_vs_soc_segments.push_back({83.94626112545511, 93.0, -10.912159709222056, 1066.501905587351});
            P2_vs_soc_segments.push_back({93.0, 100.1, -6.637218044871427, 668.9323308027426});
            zero_slope_threashold_P2_vs_soc = 0.2198140688179176;
            // min_non_zero_slope = 0.24423785424213065
        }
        else if(SE_enum == xfc_350 && EV_enum == bev250_ld1_75kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.964999999973802, 55.0199999992664});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.46999999999373154, 60.999999999186684});
            P2_vs_soc_segments.push_back({10.0, 90.97982885085574, 0.11923076922917951, 64.50769230683221});
            P2_vs_soc_segments.push_back({90.97982885085574, 100.1, -7.828947368316662, 787.6315789368662});
            zero_slope_threashold_P2_vs_soc = 0.10730769230626155;
            // min_non_zero_slope = 0.11923076922917951
        }
        else if(SE_enum == xfc_350 && EV_enum == bev150_ld1_50kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.3186184209793759, 36.9213157874225});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.3153947368245828, 40.934210524041674});
            P2_vs_soc_segments.push_back({10.0, 89.85909047573648, 0.0800101214530449, 43.288056677757055});
            P2_vs_soc_segments.push_back({89.85909047573648, 100.1, -4.697368420791664, 472.5789473421664});
            zero_slope_threashold_P2_vs_soc = 0.0720091093077404;
            // min_non_zero_slope = 0.0800101214530449
        }
        else if(SE_enum == xfc_150 && EV_enum == bev250_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 3.4115495812616254, 115.57568694456087});
            P2_vs_soc_segments.push_back({2.0, 50.0, 0.21829314446229742, 121.96219981815953});
            P2_vs_soc_segments.push_back({50.0, 60.0, 0.419903966224057, 111.88165873007155});
            P2_vs_soc_segments.push_back({60.0, 64.0, 0.3285296719323875, 117.36411638757171});
            P2_vs_soc_segments.push_back({64.0, 80.0, 0.27993597748270843, 120.47411283235117});
            P2_vs_soc_segments.push_back({80.0, 90.0, 0.5659781541197988, 97.59073870138394});
            P2_vs_soc_segments.push_back({90.0, 93.40624470271933, 0.9255714930474067, 65.22733819789923});
            P2_vs_soc_segments.push_back({93.40624470271933, 100.1, -22.20760233968896, 2226.010233969016});
            zero_slope_threashold_P2_vs_soc = 0.19646383001606768;
            // min_non_zero_slope = 0.21829314446229742
        }
        else if(SE_enum == xfc_150 && EV_enum == bev300_575kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 5.7137365980573644, 108.5609953630899});
            P2_vs_soc_segments.push_back({2.0, 50.0, 0.23807235825238984, 119.51232384269984});
            P2_vs_soc_segments.push_back({50.0, 64.0, 0.40812404271838376, 111.00973961940015});
            P2_vs_soc_segments.push_back({64.0, 80.0, 0.2720826951455885, 119.71638586405905});
            P2_vs_soc_segments.push_back({80.0, 90.0, 0.4217281774756626, 107.74474727765312});
            P2_vs_soc_segments.push_back({90.0, 96.37854258608343, 1.4284341495143404, 17.14120979417212});
            P2_vs_soc_segments.push_back({96.37854258608343, 100.1, -40.38750000000007, 4047.300000000006});
            zero_slope_threashold_P2_vs_soc = 0.21426512242715087;
            // min_non_zero_slope = 0.23807235825238984
        }
        else if(SE_enum == xfc_150 && EV_enum == bev300_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 3.9602889016317047, 114.15519782569974});
            P2_vs_soc_segments.push_back({2.0, 50.0, 0.22338493959603753, 121.62900574977107});
            P2_vs_soc_segments.push_back({50.0, 60.0, 0.4179693519528929, 111.8997851319283});
            P2_vs_soc_segments.push_back({60.0, 64.0, 0.34792186951924226, 116.10263407794734});
            P2_vs_soc_segments.push_back({64.0, 80.0, 0.27864623463526195, 120.53627471052208});
            P2_vs_soc_segments.push_back({80.0, 90.0, 0.5331521155660414, 100.17580423605973});
            P2_vs_soc_segments.push_back({90.0, 94.07680939379412, 1.0457918137074111, 54.03823140333645});
            P2_vs_soc_segments.push_back({94.07680939379412, 100.1, -24.74561402366666, 2480.411402363966});
            zero_slope_threashold_P2_vs_soc = 0.20104644563643378;
            // min_non_zero_slope = 0.22338493959603753
        }
        else if(SE_enum == xfc_150 && EV_enum == bev250_350kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 4.093204816419428, 114.60973485974407});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.9790362665227138, 127.06640905933092});
            P2_vs_soc_segments.push_back({10.0, 88.00164569821838, 0.24836435566451834, 134.37312816791288});
            P2_vs_soc_segments.push_back({88.00164569821838, 100.1, -12.395833333333346, 1247.0833333333346});
            zero_slope_threashold_P2_vs_soc = 0.22352792009806652;
            // min_non_zero_slope = 0.24836435566451834
        }
        else if(SE_enum == xfc_150 && EV_enum == bev300_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.47056295290951, 112.38226810561518});
            P2_vs_soc_segments.push_back({3.0, 4.0, 3.0909542725259143, 116.52109414676598});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.0801824141979623, 124.56418158007777});
            P2_vs_soc_segments.push_back({10.0, 85.78074999649847, 0.24809195400623296, 132.88508618199506});
            P2_vs_soc_segments.push_back({85.78074999649847, 93.0, -12.896188741779294, 1260.4113424309744});
            P2_vs_soc_segments.push_back({93.0, 100.1, -7.843984958785712, 790.5563906125712});
            zero_slope_threashold_P2_vs_soc = 0.22328275860560967;
            // min_non_zero_slope = 0.24809195400623296
        }
        else if(SE_enum == xfc_150 && EV_enum == bev150_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.99957894709066, 110.694315783324});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.2821954886505722, 121.84646615864426});
            P2_vs_soc_segments.push_back({10.0, 71.79222115564788, 0.2495069251938948, 132.17335179321103});
            P2_vs_soc_segments.push_back({71.79222115564788, 93.0, -5.747975708182693, 562.7459513857405});
            P2_vs_soc_segments.push_back({93.0, 100.1, -3.620300751678571, 364.8721804308571});
            zero_slope_threashold_P2_vs_soc = 0.22455623267450533;
            // min_non_zero_slope = 0.2495069251938948
        }
        else if(SE_enum == xfc_150 && EV_enum == bev250_ld2_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.6056579736571575, 110.6314475850196});
            P2_vs_soc_segments.push_back({3.0, 4.0, 2.629336759794284, 116.56041122660822});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.117204294753378, 122.60894108677185});
            P2_vs_soc_segments.push_back({10.0, 84.58878917626679, 0.2462678254666393, 131.31830577963925});
            P2_vs_soc_segments.push_back({84.58878917626679, 93.0, -11.573502722587579, 1131.1383847808447});
            P2_vs_soc_segments.push_back({93.0, 100.1, -7.039473684371428, 709.4736842267428});
            zero_slope_threashold_P2_vs_soc = 0.22164104291997538;
            // min_non_zero_slope = 0.2462678254666393
        }
        else if(SE_enum == xfc_150 && EV_enum == bev200_ld4_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.387833333333332, 110.026});
            P2_vs_soc_segments.push_back({3.0, 4.0, 3.0039285714285713, 114.17771428571429});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.0604285714285697, 121.9517142857143});
            P2_vs_soc_segments.push_back({10.0, 85.72163996478638, 0.24300000000000002, 130.12599999999998});
            P2_vs_soc_segments.push_back({85.72163996478638, 93.0, -12.565517241379332, 1228.0931034482778});
            P2_vs_soc_segments.push_back({93.0, 100.1, -7.6428571428571415, 770.2857142857141});
            zero_slope_threashold_P2_vs_soc = 0.21870000000000003;
            // min_non_zero_slope = 0.24300000000000002
        }
        else if(SE_enum == xfc_150 && EV_enum == bev275_ld1_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.645111841936343, 109.2933947328678});
            P2_vs_soc_segments.push_back({3.0, 4.0, 2.4053712405140546, 116.01261653713465});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.1287199247709812, 121.11922180010694});
            P2_vs_soc_segments.push_back({10.0, 83.94626112545511, 0.24423785424213065, 129.96404250539547});
            P2_vs_soc_segments.push_back({83.94626112545511, 93.0, -10.912159709222056, 1066.501905587351});
            P2_vs_soc_segments.push_back({93.0, 100.1, -6.637218044871427, 668.9323308027426});
            zero_slope_threashold_P2_vs_soc = 0.2198140688179176;
            // min_non_zero_slope = 0.24423785424213065
        }
        else if(SE_enum == xfc_150 && EV_enum == bev250_ld1_75kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.964999999973802, 55.0199999992664});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.46999999999373154, 60.999999999186684});
            P2_vs_soc_segments.push_back({10.0, 90.97982885085574, 0.11923076922917951, 64.50769230683221});
            P2_vs_soc_segments.push_back({90.97982885085574, 100.1, -7.828947368316662, 787.6315789368662});
            zero_slope_threashold_P2_vs_soc = 0.10730769230626155;
            // min_non_zero_slope = 0.11923076922917951
        }
        else if(SE_enum == xfc_150 && EV_enum == bev150_ld1_50kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.3186184209793759, 36.9213157874225});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.3153947368245828, 40.934210524041674});
            P2_vs_soc_segments.push_back({10.0, 89.85909047573648, 0.0800101214530449, 43.288056677757055});
            P2_vs_soc_segments.push_back({89.85909047573648, 100.1, -4.697368420791664, 472.5789473421664});
            zero_slope_threashold_P2_vs_soc = 0.0720091093077404;
            // min_non_zero_slope = 0.0800101214530449
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev250_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 3.788484599254138, 80.8617667161077});
            P2_vs_soc_segments.push_back({2.0, 50.0, 0.17117626721323317, 88.09638338018951});
            P2_vs_soc_segments.push_back({50.0, 60.0, 0.3014386748549845, 81.58326299810194});
            P2_vs_soc_segments.push_back({60.0, 64.0, 0.2854513841618164, 82.54250043969203});
            P2_vs_soc_segments.push_back({64.0, 80.0, 0.20095911656999052, 87.95000556556889});
            P2_vs_soc_segments.push_back({80.0, 90.0, 0.3345955326854353, 77.2590922763333});
            P2_vs_soc_segments.push_back({90.0, 95.17766873682544, 0.9598383128649036, 20.987242060181156});
            P2_vs_soc_segments.push_back({95.17766873682544, 100.1, -22.207602339688908, 2226.0102339690106});
            zero_slope_threashold_P2_vs_soc = 0.15405864049190987;
            // min_non_zero_slope = 0.17117626721323317
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev300_575kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 4.201276910336288, 79.82426129638964});
            P2_vs_soc_segments.push_back({2.0, 50.0, 0.17505320459734566, 87.87670870786754});
            P2_vs_soc_segments.push_back({50.0, 64.0, 0.3000912078811646, 81.62480854367658});
            P2_vs_soc_segments.push_back({64.0, 80.0, 0.20006080525410974, 88.02675431180809});
            P2_vs_soc_segments.push_back({80.0, 90.0, 0.31009424814386843, 79.22407888062739});
            P2_vs_soc_segments.push_back({90.0, 97.36748324301774, 1.0503192275840734, 12.603830731008948});
            P2_vs_soc_segments.push_back({97.36748324301774, 100.1, -40.38749999999989, 4047.299999999989});
            zero_slope_threashold_P2_vs_soc = 0.1575478841376111;
            // min_non_zero_slope = 0.17505320459734566
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev300_400kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 2.0, 4.199614678289122, 79.79267888749331});
            P2_vs_soc_segments.push_back({2.0, 50.0, 0.1749839449287132, 87.84194035421413});
            P2_vs_soc_segments.push_back({50.0, 64.0, 0.299972477020652, 81.5925137496172});
            P2_vs_soc_segments.push_back({64.0, 80.0, 0.1999816513471008, 87.99192659272447});
            P2_vs_soc_segments.push_back({80.0, 90.0, 0.3099715595880074, 79.19273393345193});
            P2_vs_soc_segments.push_back({90.0, 96.10440213789735, 1.0499036695722772, 12.598844034867646});
            P2_vs_soc_segments.push_back({96.10440213789735, 100.1, -27.633552618825053, 2769.205261879805});
            zero_slope_threashold_P2_vs_soc = 0.15748555043584186;
            // min_non_zero_slope = 0.1749839449287132
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev250_350kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 3.009709423837818, 84.27186386745888});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7198796077372884, 93.43118313186099});
            P2_vs_soc_segments.push_back({10.0, 91.28940217198085, 0.18262084975332216, 98.80377071170065});
            P2_vs_soc_segments.push_back({91.28940217198085, 100.1, -12.395833333333321, 1247.0833333333321});
            zero_slope_threashold_P2_vs_soc = 0.16435876477798994;
            // min_non_zero_slope = 0.18262084975332216
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev300_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.9995937847830283, 83.9886259739248});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7174600910168047, 93.11716074898969});
            P2_vs_soc_segments.push_back({10.0, 89.33220488812198, 0.18200706073257772, 98.47169105183197});
            P2_vs_soc_segments.push_back({89.33220488812198, 100.1, -10.177631574249991, 1023.9210521589991});
            zero_slope_threashold_P2_vs_soc = 0.16380635465931995;
            // min_non_zero_slope = 0.18200706073257772
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev150_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 3.7605318619325048, 79.81793577856644});
            P2_vs_soc_segments.push_back({3.0, 4.0, 1.0131800786122467, 88.05999112852722});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.9211469136841531, 88.42812378823957});
            P2_vs_soc_segments.push_back({10.0, 79.21407166142347, 0.18203905240493465, 95.81920240103176});
            P2_vs_soc_segments.push_back({79.21407166142347, 93.0, -5.952087114006899, 581.7283121273916});
            P2_vs_soc_segments.push_back({93.0, 100.1, -3.620300751678571, 364.8721804308571});
            zero_slope_threashold_P2_vs_soc = 0.1638351471644412;
            // min_non_zero_slope = 0.18203905240493465
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev250_ld2_300kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.9745412836807366, 83.2871559430606});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7114678897353396, 92.33944951884219});
            P2_vs_soc_segments.push_back({10.0, 88.17172261448238, 0.18048694420454656, 97.64925897415013});
            P2_vs_soc_segments.push_back({88.17172261448238, 100.1, -9.133771930033335, 918.9035087929335});
            zero_slope_threashold_P2_vs_soc = 0.1624382497840919;
            // min_non_zero_slope = 0.18048694420454656
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev200_ld4_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.9833655973345725, 83.53423672536802});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7135785398204808, 92.61338495542438});
            P2_vs_soc_segments.push_back({10.0, 89.10233975523242, 0.18102237916886246, 97.93894656194057});
            P2_vs_soc_segments.push_back({89.10233975523242, 100.1, -9.916666666666659, 997.666666666666});
            zero_slope_threashold_P2_vs_soc = 0.1629201412519762;
            // min_non_zero_slope = 0.18102237916886246
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev275_ld1_150kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 3.0938121720736254, 83.74678600148754});
            P2_vs_soc_segments.push_back({3.0, 4.0, 2.783247248588379, 84.67848077194328});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7424466720727572, 92.84168307800577});
            P2_vs_soc_segments.push_back({10.0, 87.25460192080672, 0.18250827670609035, 98.44106703167243});
            P2_vs_soc_segments.push_back({87.25460192080672, 93.0, -10.912159709222076, 1066.501905587353});
            P2_vs_soc_segments.push_back({93.0, 100.1, -6.637218044871427, 668.9323308027426});
            zero_slope_threashold_P2_vs_soc = 0.1642574490354813;
            // min_non_zero_slope = 0.18250827670609035
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev250_ld1_75kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.5065000003205822, 42.18200000897634});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.3603333334100125, 46.76666667661862});
            P2_vs_soc_segments.push_back({10.0, 93.1997917851699, 0.09141025642970844, 49.45589744642166});
            P2_vs_soc_segments.push_back({93.1997917851699, 100.1, -7.828947368316656, 787.6315789368657});
            zero_slope_threashold_P2_vs_soc = 0.0822692307867376;
            // min_non_zero_slope = 0.09141025642970844
        }
        else if(SE_enum == dcfc_50 && EV_enum == bev150_ld1_50kW)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.3186184209793759, 36.9213157874225});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.3153947368245828, 40.934210524041674});
            P2_vs_soc_segments.push_back({10.0, 89.85909047573648, 0.0800101214530449, 43.288056677757055});
            P2_vs_soc_segments.push_back({89.85909047573648, 100.1, -4.697368420791664, 472.5789473421664});
            zero_slope_threashold_P2_vs_soc = 0.0720091093077404;
            // min_non_zero_slope = 0.0800101214530449
        }
		else
		{
			std::cout << "WARNING: P2_vs_soc is not defigned in the factory_EV_charge_model for EV_enum:" << EV_enum << " and SE_enum:" << SE_enum << std::endl;
		}
	}
}


void factory_EV_charge_model::get_E1_Energy_limit(bool is_charging_not_discharging, bool are_battery_losses, vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type,
                                                  double battery_size_with_degredation_kWh, double SE_P2_limit_atNominalV_kW, double recalc_exponent_threashold,
                                                  double max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, line_segment& bat_eff_vs_P2, 
                                                  calculate_E1_energy_limit& return_val)
{
	poly_function_of_x P2_vs_puVrms;
	double zero_slope_threashold_P2_vs_soc;
	std::vector<line_segment> P2_vs_soc_segments;

	this->get_P2_vs_puVrms(is_charging_not_discharging, vehicle_type, supply_equipment_type, SE_P2_limit_atNominalV_kW, P2_vs_puVrms);  
	this->get_P2_vs_soc(is_charging_not_discharging, vehicle_type, supply_equipment_type, zero_slope_threashold_P2_vs_soc, P2_vs_soc_segments);

	//--------------------------------
	
	algorithm_P2_vs_soc *X;
	calc_E1_energy_limit *Y;

	if(are_battery_losses)
		X = new algorithm_P2_vs_soc_losses(battery_size_with_degredation_kWh, recalc_exponent_threashold,  zero_slope_threashold_P2_vs_soc, bat_eff_vs_P2);
	else
		X = new algorithm_P2_vs_soc_no_losses(battery_size_with_degredation_kWh, recalc_exponent_threashold, zero_slope_threashold_P2_vs_soc);
	
	if(is_charging_not_discharging)
		Y = new calc_E1_energy_limit_charging(X);
	else
		Y = new calc_E1_energy_limit_discharging(X);
	
	//--------------------------------
	
	calculate_E1_energy_limit tmp(is_charging_not_discharging, max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, Y, P2_vs_puVrms, P2_vs_soc_segments);

	return_val = tmp;
}


vehicle_charge_model* factory_EV_charge_model::alloc_get_EV_charge_model(const charge_event_data& event, supply_equipment_enum supply_equipment_type, double SE_P2_limit_kW)
{	
	double battery_size_kWh, recalc_exponent_threashold, max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, soc_of_full_battery;
	double battery_size_with_degredation_kWh;
    bool will_never_discharge, are_battery_losses;
	battery_chemistry bat_chem;
	
	soc_of_full_battery = 99.8;
	recalc_exponent_threashold = 0.00000001;  // Should be very small 
	max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments = 0.5;
	
	will_never_discharge = true;
	are_battery_losses = true;

	get_pev_battery_info(event.vehicle_type, bat_chem, battery_size_kWh, battery_size_with_degredation_kWh);
	
    //---------------------------------

	integrate_X_through_time get_next_P2;
	this->get_integrate_X_through_time_obj(event.vehicle_type, supply_equipment_type, get_next_P2);

	//---------------------------------

	double zero_slope_threashold_bat_eff_vs_P2, zst_A, zst_B;
	line_segment bat_eff_vs_P2_charging;
	line_segment bat_eff_vs_P2_discharging;

	get_bat_eff_vs_P2(true, bat_chem, battery_size_kWh, zst_A, bat_eff_vs_P2_charging);
	get_bat_eff_vs_P2(false, bat_chem, battery_size_kWh, zst_B, bat_eff_vs_P2_discharging);
	
	zero_slope_threashold_bat_eff_vs_P2 = (zst_A < zst_B) ? zst_A : zst_B;
	
	//---------------------------------
	
	double final_bat_size_kWh;
	
	if(this->model_stochastic_battery_degregation)
		final_bat_size_kWh = battery_size_with_degredation_kWh;
	else
		final_bat_size_kWh = battery_size_kWh;
	
	//---------------------------------
	
	calculate_E1_energy_limit get_E1_limits_charging;
	calculate_E1_energy_limit get_E1_limits_discharging;

	this->get_E1_Energy_limit(true, are_battery_losses, event.vehicle_type, supply_equipment_type, final_bat_size_kWh, SE_P2_limit_kW, recalc_exponent_threashold,                              
                              max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, bat_eff_vs_P2_charging, get_E1_limits_charging);

	this->get_E1_Energy_limit(false, are_battery_losses, event.vehicle_type, supply_equipment_type, final_bat_size_kWh, SE_P2_limit_kW, recalc_exponent_threashold,                              
                              max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, bat_eff_vs_P2_discharging, get_E1_limits_discharging);

	//---------------------------------

	battery bat(final_bat_size_kWh, event.arrival_SOC, will_never_discharge, zero_slope_threashold_bat_eff_vs_P2,
			    bat_eff_vs_P2_charging, bat_eff_vs_P2_discharging, get_E1_limits_charging, get_E1_limits_discharging, get_next_P2);
	
	//---------------------------------
	
	return new vehicle_charge_model(event, bat, soc_of_full_battery);
}


bool get_pev_battery_info(vehicle_enum vehicle_type, battery_chemistry& bat_chem, double& battery_size_kWh, double& battery_size_with_stochastic_degredation_kWh)
{
    bool is_success = true;
    
    if(vehicle_type == phev20)
	{
		bat_chem = NMC;
		battery_size_kWh = 5;	
	}
	else if(vehicle_type == phev50)
	{
		bat_chem = NMC;
		battery_size_kWh = 16;	
	}
    else if(vehicle_type == phev_SUV)
	{
		bat_chem = NMC;
		battery_size_kWh = 23.75;	
	}
	else if(vehicle_type == bev150_ld1_50kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 45;	
	}
	else if(vehicle_type == bev250_ld1_75kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 75;
	}
	else if(vehicle_type == bev275_ld1_150kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 83;	
	}
	else if(vehicle_type == bev200_ld4_150kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 95;	
	}
	else if(vehicle_type == bev250_ld2_300kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 88;	
	}
    else if(vehicle_type == bev150_150kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 45;	
	}
    else if(vehicle_type == bev300_300kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 97.5;	
	}
    else if(vehicle_type == bev250_350kW)
	{
		bat_chem = NMC;
		battery_size_kWh = 118.8;	
	}
    else if(vehicle_type == bev300_400kW)
	{
		bat_chem = LTO;
		battery_size_kWh = 97.5;	
	}
    else if(vehicle_type == bev300_575kW)
	{
		bat_chem = LTO;
		battery_size_kWh = 142.5;	
	}
    else if(vehicle_type == bev250_400kW)
	{
		bat_chem = LTO;
		battery_size_kWh = 87.5;	
	}
	else
    {
        is_success = false;
		std::cout << "WARNING: get_pev_battery_info is not defigned in the factory_EV_charge_model for EV_enum:" << vehicle_type << std::endl;
	}
    
    //---------------------------
    
    if(bat_chem == LTO)
		battery_size_with_stochastic_degredation_kWh = rand_val::rand_range(0.95, 1.0) * battery_size_kWh;

	else if(bat_chem == LMO)
		battery_size_with_stochastic_degredation_kWh = rand_val::rand_range(0.85, 1.0) * battery_size_kWh;

	else if(bat_chem == NMC)
		battery_size_with_stochastic_degredation_kWh = rand_val::rand_range(0.90, 1.0) * battery_size_kWh;

    //---------------------------
    
    return is_success;
}


//#############################################################################
//                Supply Equipment Charge Model Factory
//#############################################################################

void factory_supply_equipment_model::get_supply_equipment_model(bool building_charge_profile_library, SE_configuration& SE_config, get_base_load_forecast* baseLD_forecaster, manage_L2_control_strategy_parameters* manage_L2_control, supply_equipment& return_val)
{
	double P2_limit_kW, standby_acP_kW, standby_acQ_kVAR;
	
	supply_equipment_enum SE_enum = SE_config.supply_equipment_type;
	
	standby_acP_kW = 0;
	standby_acQ_kVAR = 0;
	
	if(SE_enum == L1_1440)
	{
		P2_limit_kW = 1.289338;
	}
	else if(SE_enum == L2_3600)
	{
		P2_limit_kW = 3.30192;
	}
	else if(SE_enum == L2_7200)
	{
		P2_limit_kW = 6.624;
	}
	else if(SE_enum == L2_9600)
	{
		P2_limit_kW = 8.832;
	}
	else if(SE_enum == L2_11520)
	{
		P2_limit_kW = 10.5984;
	}
	else if(SE_enum == L2_17280)
	{
		P2_limit_kW = 15.8976;
	}
	else if(SE_enum == dcfc_50)
	{
		P2_limit_kW = 50;
		standby_acP_kW = 0.100;  
		standby_acQ_kVAR = -0.590;
	}
	else if(SE_enum == xfc_150)
	{
		P2_limit_kW = 150;
		standby_acP_kW = 0.170;
		standby_acQ_kVAR = -0.445;
	}
	else if(SE_enum == xfc_350)
	{
		P2_limit_kW = 350;
		standby_acP_kW = 0.170;
		standby_acQ_kVAR = -0.445;
	}
	else
		std::cout << "WARNING: get_supply_equipment_model is not defigned in the factory_supply_equipment_model for SE_enum:" << SE_enum << std::endl;
	
	//============================
    
	supply_equipment_load load(P2_limit_kW, standby_acP_kW, standby_acQ_kVAR, SE_config);
    supply_equipment_control control(building_charge_profile_library, SE_config, baseLD_forecaster, manage_L2_control);
    supply_equipment SE(SE_config, control, load);
    
	return_val = SE;
}


//#############################################################################
//                      AC to DC Converter Factory
//#############################################################################

ac_to_dc_converter* factory_ac_to_dc_converter::alloc_get_ac_to_dc_converter(ac_to_dc_converter_enum converter_type, supply_equipment_enum SE_type, 
                                                                             vehicle_enum vehicle_type, charge_event_P3kW_limits& P3kW_limits)
{
    std::vector<poly_segment> inv_eff_from_P2_vec;
    std::vector<poly_segment> inv_pf_from_P3_vec;
    	    
    //=============================================================
	//=============================================================
    
	if(supply_equipment_is_L1(SE_type))
	{
		// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-20, 0, first, (1.1-1)/(-20-0), 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 1.4, second, -0.09399, 0.26151, 0.71348, 0, 0}); 
		inv_eff_from_P2_vec.push_back({1.4, 20, first, 0, 0.895374, 0, 0, 0});
		
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-20, 0, first, (-1+0.9)/(-20-0), -0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 1.3, first, -0.0138, -0.9793, 0, 0, 0});
		inv_pf_from_P3_vec.push_back({1.3, 20, first, 0, -0.99724, 0, 0, 0});
	}
	else if(supply_equipment_is_L2(SE_type))
	{
		// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-20, 0, first, (1.1-1)/(-20-0), 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 4, second, -0.005, 0.045, 0.82, 0, 0}); 
		inv_eff_from_P2_vec.push_back({4, 20, first, 0, 0.92, 0, 0, 0});
		
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-20, 0, first, (-1+0.9)/(-20-0), -0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 6, third, -0.00038737, 0.00591216, -0.03029164, -0.9462841, 0});
		inv_pf_from_P3_vec.push_back({6, 20, first, 0, -0.9988681, 0, 0, 0});
	}
	else if(SE_type == dcfc_50)
	{
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-1000, 0, first, (1.1-1)/1000, 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 10, second, -0.0023331, 0.04205, 0.7284, 0, 0});  
		inv_eff_from_P2_vec.push_back({10, 20, second, -0.00035233, 0.01454, 0.7755, 0, 0});  
		inv_eff_from_P2_vec.push_back({20, 30, second, -0.00015968, 0.01006, 0.7698, 0, 0});  
		inv_eff_from_P2_vec.push_back({30, 40, second, -0.000083167, 0.007314, 0.7697, 0, 0});  
		inv_eff_from_P2_vec.push_back({40, 1000, first, 0, 0.9292, 0, 0, 0}); 
		
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-1000, 0, first, (0.9-1)/1000, 0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 10, second, 0.0037161, -0.1109, -0.06708, 0, 0});
		inv_pf_from_P3_vec.push_back({10, 18.6, first, -0.01474, -0.6568, 0, 0, 0});
		inv_pf_from_P3_vec.push_back({18.6, 28, first, -0.003804, -0.8601, 0, 0, 0});
		inv_pf_from_P3_vec.push_back({28, 42, first, -0.001603, -0.9218, 0, 0, 0});
		inv_pf_from_P3_vec.push_back({42, 1000, first, 0, -0.99, 0, 0, 0});	
	}
	else if(SE_type == xfc_150 || SE_type == xfc_350)
	{
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-1000, 0, first, (1.1-1)/1000, 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 25, second, -0.0007134, 0.03554, 0.4724, 0, 0});  
		inv_eff_from_P2_vec.push_back({25, 130, first, 0.0003331, 0.9067, 0, 0, 0});
		inv_eff_from_P2_vec.push_back({130, 1000, first, 0, 0.95, 0, 0, 0}); 
		
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-1000, 0, first, (0.9-1)/1000, 0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 60, third, -0.0000053284, 0.00071628, -0.03361, -0.3506, 0});
		inv_pf_from_P3_vec.push_back({60, 80, first, -0.001034, -0.8775, 0, 0, 0});
		inv_pf_from_P3_vec.push_back({80, 133, second, 0.0000027985, -0.0008177, -0.9127, 0, 0});
		inv_pf_from_P3_vec.push_back({133, 1000, first, 0, -0.972, 0, 0, 0});	
	}
	else
		std::cout << "WARNING: get_supply_equipment_model is not defigned in the factory_supply_equipment_model for SE_enum:" << SE_type << std::endl;
	
	//------------------------------------
    
    double S3kVA_from_max_nominal_P3kW_multiplier = 1;
	
	double x_tolerance = 0.0001;
	bool take_abs_of_x = false;
    bool if_x_is_out_of_bounds_print_warning_message = false;
	
	poly_function_of_x  inv_eff_from_P2(x_tolerance, take_abs_of_x, if_x_is_out_of_bounds_print_warning_message, inv_eff_from_P2_vec, "inv_eff_from_P2");
	poly_function_of_x  inv_pf_from_P3(x_tolerance, take_abs_of_x, if_x_is_out_of_bounds_print_warning_message, inv_pf_from_P3_vec, "inv_pf_from_P3");

    ac_to_dc_converter* return_val = NULL;
    
    if(converter_type == pf)
        return_val = new ac_to_dc_converter_pf(P3kW_limits, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2, inv_pf_from_P3);
    
    else if(converter_type == Q_setpoint)        
        return_val = new ac_to_dc_converter_Q_setpoint(P3kW_limits, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2);
    
    else
    {
        std::cout << "ERROR:  In factory_ac_to_dc_converter undefigned converter_type." << std::endl;
        return_val = new ac_to_dc_converter_pf(P3kW_limits, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2, inv_pf_from_P3);
    }

	return return_val;
}


//#############################################################################
//                         Read Input Data Files
//#############################################################################
/*
//==================================
//  charge_events_file_factory
//==================================

charge_events_file_factory::~charge_events_file_factory() {}

int charge_events_file_factory::number_of_lines_to_skip_at_beginning_of_file() {return 1;}

int charge_events_file_factory::number_of_fields() {return 10;}


void charge_events_file_factory::parse_line(const std::string& line, bool& parse_successful, charge_event_data& event_data)
{
	parse_successful = true;
	
	const char delim = ',';
	std::vector<std::string> tokens = split(line, delim);
	
	if(tokens.size() != this->number_of_fields())
	{
		parse_successful = false;
		return;
	}	
	
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	try
	{
		event_data.charge_event_id = std::stoi(tokens[0]);	
		event_data.supply_equipment_id = std::stoi(tokens[1]);	
		event_data.vehicle_id = std::stoi(tokens[2]);	
		event_data.arrival_unix_time = 3600*std::stod(tokens[4]);
		event_data.departure_unix_time = 3600*std::stod(tokens[6]);
		event_data.arrival_SOC = 100*std::stod(tokens[8]);
		event_data.departure_SOC = 100*std::stod(tokens[9]);
	}
	catch(const std::invalid_argument& ia)
	{
		parse_successful = false;
		return;
	}
	catch(const std::out_of_range& ia)
	{
		parse_successful = false;
		return;
	}
		
	event_data.vehicle_type = get_vehicle_enum(tokens[3], parse_successful);
	
	if(!parse_successful)
		return;

	//--------------------------------- 
	
	stop_charging_criteria_csv_file_enum csv_file_criteria_enum = first_event; //get_stop_charging_criteria_csv_file_enum(tokens[8], parse_successful);
	
	if(!parse_successful)
		return;
	
	stop_charging_criteria criteria;
	criteria.soc_mode = target_charging;
	criteria.depart_time_mode = block_charging;
	criteria.soc_block_charging_max_undershoot_percent = 50;		// Not used here, only used when criteria.soc_mode = block_charging
	criteria.depart_time_block_charging_max_undershoot_percent = 40;
	
	if(csv_file_criteria_enum == soc)
		criteria.decision_metric = stop_charging_using_target_soc;
		
	else if(csv_file_criteria_enum == depart_time)
		criteria.decision_metric = stop_charging_using_depart_time;
		
	else if(csv_file_criteria_enum == first_event)
		criteria.decision_metric = stop_charging_using_whatever_happens_first;
	
	event_data.stop_charge = criteria;
}


//==================================
//  supply_equipment_info_file_factory
//==================================

supply_equipment_info_file_factory::~supply_equipment_info_file_factory() {}

int supply_equipment_info_file_factory::number_of_lines_to_skip_at_beginning_of_file() {return 1;}

int supply_equipment_info_file_factory::number_of_fields() {return 5;}


void supply_equipment_info_file_factory::parse_line(const std::string& line, bool& parse_successful, supply_equipment_data& SE_data)
{	
	const char delim = ',';
	std::vector<std::string> tokens = split(line, delim);
	
	if(tokens.size() != this->number_of_fields())
	{
		parse_successful = false;
		return;
	}	
	
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	try
	{
		SE_data.supply_equipment_id = std::stoi(tokens[0]);
		SE_data.latitude = std::stod(tokens[3]);
		SE_data.longitude = std::stod(tokens[2]);
		SE_data.node_id = std::stoi(tokens[4]);
	}
	catch(const std::invalid_argument& ia)
	{
		parse_successful = false;
		return;
	}
	catch(const std::out_of_range& ia)
	{
		parse_successful = false;
		return;
	}
		
	SE_data.supply_equipment_type = get_supply_equipment_enum(tokens[1], parse_successful);
}
*/

