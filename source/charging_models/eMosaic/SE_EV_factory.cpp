
//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								              eMosaic  Project
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
/*
        if(vehicle_type == phev20 || vehicle_type == phev50 || vehicle_type == phev_SUV)
            std::cout << "WARNING: PHEV can not fast charge.  EV_enum :" << vehicle_type << " and SE_enum:" << supply_equipment_type << std::endl;
*/
        
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
/*                   
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
*/
                //----------------------
                //     pos_to_off
                //----------------------
                goto_next_seg.clear();
                goto_next_seg.push_back({time_delay_sec, 0.040, false, 0, 0});
                goto_next_seg.push_back({time_delay_sec, 0.010, false, 0, 0});
                goto_next_seg.push_back({from_final_X, 0, false, 0, -140000});                 
                transition_of_X_through_time pos_to_off_obj(pos_to_off, X_deadband, goto_next_seg);

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
//            }
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
    //      - Make sure the delay is small when battery stops charging/discharging
	
	vehicle_enum EV_enum = vehicle_type;
	supply_equipment_enum SE_enum = supply_equipment_type;
	P2_vs_soc_segments.clear();
	
	if(supply_equipment_is_L1(SE_enum) || supply_equipment_is_L2(SE_enum))
	{
        //std::cout << "Error: P2_vs_soc is not defigned in the factory_EV_charge_model for EV_enum:" << EV_enum << " and SE_enum:" << SE_enum << std::endl;

        if(EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 97.55911529426994, 0.0, 15.8976});
            P2_vs_soc_segments.push_back({97.55911529426994, 100.1, -5.219298245875, 525.0877193245});
            zero_slope_threashold_P2_vs_soc = 4.6973684212875;
            // min_non_zero_slope = 5.219298245875
        }
        else if(EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 99.0805541670789, 0.0, 15.8976});
            P2_vs_soc_segments.push_back({99.0805541670789, 100.1, -10.428157891083332, 1049.1252627903334});
            zero_slope_threashold_P2_vs_soc = 9.385342101974999;
            // min_non_zero_slope = 10.428157891083332
        }
        else if(EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 15.8976});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 15.8976});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 15.8976});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 15.8976});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else
		{
			std::cout << "Error: P2_vs_soc is not defigned in the factory_EV_charge_model for EV_enum:" << EV_enum << " and SE_enum:" << SE_enum << std::endl;
		}
    }
	else
	{
        if(SE_enum == xfc_20kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.575622120010188, 44.117419360285275});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.3768663594935312, 48.91244240235191});
            P2_vs_soc_segments.push_back({10.0, 89.0632790877546, 0.09560439561619848, 51.725062041125234});
            P2_vs_soc_segments.push_back({89.0632790877546, 100.1, -5.219298245874994, 525.0877193244995});
            zero_slope_threashold_P2_vs_soc = 0.08604395605457862;
            // min_non_zero_slope = 0.09560439561619848
        }
        else if(SE_enum == xfc_20kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.5756221188144741, 44.11741932680526});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.37686635920753375, 48.91244236523303});
            P2_vs_soc_segments.push_back({10.0, 94.77601010200533, 0.09560439554364591, 51.7250620018719});
            P2_vs_soc_segments.push_back({94.77601010200533, 100.1, -10.428157891083309, 1049.125262790331});
            zero_slope_threashold_P2_vs_soc = 0.08604395598928133;
            // min_non_zero_slope = 0.09560439554364591
        }
        else if(SE_enum == xfc_20kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 1.5756221196501194, 44.11741935020333});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.37686635940740715, 48.91244239117418});
            P2_vs_soc_segments.push_back({10.0, 97.6801414822594, 0.0956043955943506, 51.72506202930474});
            P2_vs_soc_segments.push_back({97.6801414822594, 100.1, -20.877192983500027, 2100.350877298003});
            zero_slope_threashold_P2_vs_soc = 0.08604395603491555;
            // min_non_zero_slope = 0.0956043955943506
        }
        else if(SE_enum == xfc_20kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.3634331794751793, 66.17612902530499});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.5652995391111106, 73.36866358676126});
            P2_vs_soc_segments.push_back({10.0, 97.6801414822594, 0.1434065933915259, 77.58759304395711});
            P2_vs_soc_segments.push_back({97.6801414822594, 100.1, -31.315789475250043, 3150.526315947004});
            zero_slope_threashold_P2_vs_soc = 0.12906593405237332;
            // min_non_zero_slope = 0.1434065933915259
        }
        else if(SE_enum == xfc_20kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.3634331801503095, 66.17612904420865});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.5652995392725921, 73.36866360771953});
            P2_vs_soc_segments.push_back({10.0, 98.40886379832033, 0.143406593432491, 77.58759306612053});
            P2_vs_soc_segments.push_back({98.40886379832033, 100.1, -41.7543859670001, 4200.701754596011});
            zero_slope_threashold_P2_vs_soc = 0.12906593408924188;
            // min_non_zero_slope = 0.143406593432491
        }
        else if(SE_enum == xfc_20kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.3634331799252664, 66.17612903790743});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.5652995392187643, 73.36866360073344});
            P2_vs_soc_segments.push_back({10.0, 99.1392508482728, 0.14340659341883583, 77.58759305873272});
            P2_vs_soc_segments.push_back({99.1392508482728, 100.1, -62.6315789505007, 6301.052631894071});
            zero_slope_threashold_P2_vs_soc = 0.12906593407695224;
            // min_non_zero_slope = 0.14340659341883583
        }
        else if(SE_enum == xfc_50kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 2.3061961358536984, 57.85589134807428});
            P2_vs_soc_segments.push_back({3.0, 4.0, 1.5817920377189691, 60.02910364247847});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.55732603406617, 64.12696765708966});
            P2_vs_soc_segments.push_back({10.0, 85.73286929825494, 0.12776780662918027, 68.42254993145956});
            P2_vs_soc_segments.push_back({85.73286929825494, 93.0, -6.61343012737241, 646.3647913208841});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.11499102596626225;
            // min_non_zero_slope = 0.12776780662918027
        }
        else if(SE_enum == xfc_50kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.1008294917526342, 58.82322576907368});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.5024884789433757, 65.21658982031072});
            P2_vs_soc_segments.push_back({10.0, 92.85646376354687, 0.12747252739152787, 68.9667493358292});
            P2_vs_soc_segments.push_back({92.85646376354687, 100.1, -10.428157891083318, 1049.125262790332});
            zero_slope_threashold_P2_vs_soc = 0.11472527465237509;
            // min_non_zero_slope = 0.12747252739152787
        }
        else if(SE_enum == xfc_50kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 2.100829492866826, 58.8232258002711});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.5024884792098755, 65.21658985489891});
            P2_vs_soc_segments.push_back({10.0, 96.71109148897058, 0.1274725274591341, 68.96674937240633});
            P2_vs_soc_segments.push_back({96.71109148897058, 100.1, -20.87719298350004, 2100.3508772980044});
            zero_slope_threashold_P2_vs_soc = 0.11472527471322069;
            // min_non_zero_slope = 0.1274725274591341
        }
        else if(SE_enum == xfc_50kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 3.151244239300239, 88.23483870040666});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7537327188148133, 97.82488478234836});
            P2_vs_soc_segments.push_back({10.0, 96.71109148897058, 0.19120879118870113, 103.45012405860948});
            P2_vs_soc_segments.push_back({96.71109148897058, 100.1, -31.315789475250064, 3150.5263159470064});
            zero_slope_threashold_P2_vs_soc = 0.172087912069831;
            // min_non_zero_slope = 0.19120879118870113
        }
        else if(SE_enum == xfc_50kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 3.1512442402004135, 88.23483872561154});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7537327190301215, 97.8248848102927});
            P2_vs_soc_segments.push_back({10.0, 97.6801414814277, 0.19120879124332119, 103.4501240881607});
            P2_vs_soc_segments.push_back({97.6801414814277, 100.1, -41.75438596700013, 4200.701754596013});
            zero_slope_threashold_P2_vs_soc = 0.17208791211898908;
            // min_non_zero_slope = 0.19120879124332119
        }
        else if(SE_enum == xfc_50kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 3.1512442399003553, 88.23483871720991});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7537327189583534, 97.82488480097791});
            P2_vs_soc_segments.push_back({10.0, 98.6521408966291, 0.19120879122511444, 103.45012407831031});
            P2_vs_soc_segments.push_back({98.6521408966291, 100.1, -62.63157895050013, 6301.052631894013});
            zero_slope_threashold_P2_vs_soc = 0.172087912102603;
            // min_non_zero_slope = 0.19120879122511444
        }
        else if(SE_enum == xfc_90kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.981494058324488, 107.83924328411507});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.2532032848768888, 119.02411560445788});
            P2_vs_soc_segments.push_back({10.0, 74.82301745936606, 0.24450387431895568, 129.1111097100372});
            P2_vs_soc_segments.push_back({74.82301745936606, 93.0, -6.386639676432694, 625.2732793834905});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.2200534868870601;
            // min_non_zero_slope = 0.24450387431895568
        }
        else if(SE_enum == xfc_90kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.208670382432602, 109.02358567178142});
            P2_vs_soc_segments.push_back({3.0, 4.0, 3.2576382654535037, 111.87668202271871});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.0141592452541937, 120.85059810351595});
            P2_vs_soc_segments.push_back({10.0, 86.43691960693974, 0.23939864534422806, 128.5982041026156});
            P2_vs_soc_segments.push_back({86.43691960693974, 93.0, -13.213633389200002, 1291.4368525421003});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.21545878080980527;
            // min_non_zero_slope = 0.23939864534422806
        }
        else if(SE_enum == xfc_90kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 3.939055299125298, 110.29354837550832});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.9421658985185153, 122.28110597793544});
            P2_vs_soc_segments.push_back({10.0, 93.34245041357688, 0.23901098898587644, 129.31265507326182});
            P2_vs_soc_segments.push_back({93.34245041357688, 100.1, -20.877192983499995, 2100.350877298});
            zero_slope_threashold_P2_vs_soc = 0.2151098900872888;
            // min_non_zero_slope = 0.23901098898587644
        }
        else if(SE_enum == xfc_90kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 5.908582948687947, 165.44032256326247});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.413248847777773, 183.42165896690315});
            P2_vs_soc_segments.push_back({10.0, 93.34245041357688, 0.35851648347881465, 193.96898260989272});
            P2_vs_soc_segments.push_back({93.34245041357688, 100.1, -31.315789475249993, 3150.5263159469996});
            zero_slope_threashold_P2_vs_soc = 0.3226648351309332;
            // min_non_zero_slope = 0.35851648347881465
        }
        else if(SE_enum == xfc_90kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 5.908582950375772, 165.44032261052163});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.4132488481814813, 183.4216590192988});
            P2_vs_soc_segments.push_back({10.0, 95.14264129936262, 0.35851648358122695, 193.96898266530133});
            P2_vs_soc_segments.push_back({95.14264129936262, 100.1, -41.75438596700004, 4200.701754596004});
            zero_slope_threashold_P2_vs_soc = 0.32266483522310424;
            // min_non_zero_slope = 0.35851648358122695
        }
        else if(SE_enum == xfc_90kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 5.9085829498131694, 165.44032259476856});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.4132488480469085, 183.4216590018336});
            P2_vs_soc_segments.push_back({10.0, 96.95307821277244, 0.3585164835470898, 193.96898264683182});
            P2_vs_soc_segments.push_back({96.95307821277244, 100.1, -62.63157895049982, 6301.052631893983});
            zero_slope_threashold_P2_vs_soc = 0.3226648351923808;
            // min_non_zero_slope = 0.3585164835470898
        }
        else if(SE_enum == xfc_150kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_150kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.236665772849031, 142.42035984410856});
            P2_vs_soc_segments.push_back({3.0, 4.0, 2.7636455073737323, 152.83942064053446});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.5191296201801892, 157.8174841893086});
            P2_vs_soc_segments.push_back({10.0, 82.87656185028212, 0.3200969792170903, 169.80781059893962});
            P2_vs_soc_segments.push_back({82.87656185028212, 93.0, -13.213633389199996, 1291.4368525420996});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.28808728129538125;
            // min_non_zero_slope = 0.3200969792170903
        }
        else if(SE_enum == xfc_150kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 5.252073732167062, 147.05806450067774});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.2562211980246936, 163.04147463724723});
            P2_vs_soc_segments.push_back({10.0, 90.95798438810432, 0.3186813186478349, 172.4168734310158});
            P2_vs_soc_segments.push_back({90.95798438810432, 100.1, -20.87719298350002, 2100.350877298002});
            zero_slope_threashold_P2_vs_soc = 0.2868131867830514;
            // min_non_zero_slope = 0.3186813186478349
        }
        else if(SE_enum == xfc_150kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 7.878110598250593, 220.58709675101662});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.8843317970370403, 244.56221195587082});
            P2_vs_soc_segments.push_back({10.0, 90.95798438810432, 0.4780219779717523, 258.6253101465237});
            P2_vs_soc_segments.push_back({90.95798438810432, 100.1, -31.31578947525003, 3150.5263159470032});
            zero_slope_threashold_P2_vs_soc = 0.43021978017457707;
            // min_non_zero_slope = 0.4780219779717523
        }
        else if(SE_enum == xfc_150kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 7.87811060050103, 220.58709681402883});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.8843317975753058, 244.56221202573172});
            P2_vs_soc_segments.push_back({10.0, 93.34245041152576, 0.4780219781083028, 258.6253102204018});
            P2_vs_soc_segments.push_back({93.34245041152576, 100.1, -41.754385967000005, 4200.701754596001});
            zero_slope_threashold_P2_vs_soc = 0.43021978029747254;
            // min_non_zero_slope = 0.4780219781083028
        }
        else if(SE_enum == xfc_150kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 7.878110599750884, 220.58709679302478});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.884331797395882, 244.56221200244477});
            P2_vs_soc_segments.push_back({10.0, 95.74497751202671, 0.4780219780627865, 258.62531019577574});
            P2_vs_soc_segments.push_back({95.74497751202671, 100.1, -62.6315789504999, 6301.05263189399});
            zero_slope_threashold_P2_vs_soc = 0.43021978025650787;
            // min_non_zero_slope = 0.4780219780627865
        }
        else if(SE_enum == xfc_180kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_180kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 9.962567056607753, 215.7149074441158});
            P2_vs_soc_segments.push_back({3.0, 10.0, 2.5067524323892307, 238.08235131677137});
            P2_vs_soc_segments.push_back({10.0, 74.79765954931783, 0.48906314990897837, 258.2592441415739});
            P2_vs_soc_segments.push_back({74.79765954931783, 93.0, -12.760506068403844, 1249.2960117080574});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.44015683491808055;
            // min_non_zero_slope = 0.48906314990897837
        }
        else if(SE_enum == xfc_180kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 8.413586383851928, 218.0648556674365});
            P2_vs_soc_segments.push_back({3.0, 4.0, 6.524765254935051, 223.73131905418714});
            P2_vs_soc_segments.push_back({4.0, 10.0, 2.0273159854486114, 241.7211161321329});
            P2_vs_soc_segments.push_back({10.0, 86.44766494285561, 0.47879189283242996, 257.2063570582947});
            P2_vs_soc_segments.push_back({86.44766494285561, 93.0, -26.4537205094896, 2585.459165283533});
            P2_vs_soc_segments.push_back({93.0, 100.1, -16.09022556471428, 1621.654135419428});
            zero_slope_threashold_P2_vs_soc = 0.430912703549187;
            // min_non_zero_slope = 0.47879189283242996
        }
        else if(SE_enum == xfc_180kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 12.620379575777893, 327.0972835011547});
            P2_vs_soc_segments.push_back({3.0, 4.0, 9.787147882402577, 335.59697858128067});
            P2_vs_soc_segments.push_back({4.0, 10.0, 3.040973978172917, 362.5816741981993});
            P2_vs_soc_segments.push_back({10.0, 86.44766494285561, 0.7181878392486449, 385.80953558744204});
            P2_vs_soc_segments.push_back({86.44766494285561, 93.0, -39.6805807642344, 3878.188747925299});
            P2_vs_soc_segments.push_back({93.0, 100.1, -24.13533834707142, 2432.481203129142});
            zero_slope_threashold_P2_vs_soc = 0.6463690553237804;
            // min_non_zero_slope = 0.7181878392486449
        }
        else if(SE_enum == xfc_180kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 11.817165900751544, 330.88064522104327});
            P2_vs_soc_segments.push_back({4.0, 10.0, 2.8264976963629627, 366.8433180385976});
            P2_vs_soc_segments.push_back({10.0, 89.77246075003511, 0.7170329671624539, 387.93796533060265});
            P2_vs_soc_segments.push_back({89.77246075003511, 100.1, -41.754385966999955, 4200.701754595995});
            zero_slope_threashold_P2_vs_soc = 0.6453296704462085;
            // min_non_zero_slope = 0.7170329671624539
        }
        else if(SE_enum == xfc_180kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 11.817165899626339, 330.8806451895371});
            P2_vs_soc_segments.push_back({4.0, 10.0, 2.826497696093817, 366.8433180036672});
            P2_vs_soc_segments.push_back({10.0, 93.34245041220946, 0.7170329670941796, 387.93796529366364});
            P2_vs_soc_segments.push_back({93.34245041220946, 100.1, -62.631578950499936, 6301.052631893994});
            zero_slope_threashold_P2_vs_soc = 0.6453296703847616;
            // min_non_zero_slope = 0.7170329670941796
        }
        else if(SE_enum == xfc_450kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_450kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 12.51729473245665, 283.21124200604});
            P2_vs_soc_segments.push_back({3.0, 10.0, 3.2704105251700035, 310.95189462789995});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.6348216064257896, 337.3077838153421});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -12.76050606840385, 1249.2960117080581});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.5713394457832106;
            // min_non_zero_slope = 0.6348216064257896
        }
        else if(SE_enum == xfc_450kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 19.92597622893681, 431.35697302123805});
            P2_vs_soc_segments.push_back({3.0, 10.0, 5.01281313820391, 476.0964622934368});
            P2_vs_soc_segments.push_back({10.0, 74.82301746515543, 0.9780154970269997, 516.4444387052058});
            P2_vs_soc_segments.push_back({74.82301746515543, 93.0, -25.546558705730774, 2501.0931175339624});
            P2_vs_soc_segments.push_back({93.0, 100.1, -16.09022556471428, 1621.654135419428});
            zero_slope_threashold_P2_vs_soc = 0.8802139473242998;
            // min_non_zero_slope = 0.9780154970269997
        }
        else if(SE_enum == xfc_450kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 29.888964343405213, 647.035459531857});
            P2_vs_soc_segments.push_back({3.0, 10.0, 7.519219707305864, 714.1446934401552});
            P2_vs_soc_segments.push_back({10.0, 74.82301746515543, 1.4670232455404997, 774.6666580578087});
            P2_vs_soc_segments.push_back({74.82301746515543, 93.0, -38.31983805859616, 3751.6396763009434});
            P2_vs_soc_segments.push_back({93.0, 100.1, -24.13533834707142, 2432.481203129142});
            zero_slope_threashold_P2_vs_soc = 1.3203209209864497;
            // min_non_zero_slope = 1.4670232455404997
        }
        else if(SE_enum == xfc_450kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 28.995145127083383, 636.5103566468297});
            P2_vs_soc_segments.push_back({3.0, 4.0, 10.085573957279466, 693.2390701562415});
            P2_vs_soc_segments.push_back({4.0, 10.0, 7.084454224656581, 705.243549086733});
            P2_vs_soc_segments.push_back({10.0, 81.12808975550497, 1.4417737895737657, 761.6703534375612});
            P2_vs_soc_segments.push_back({81.12808975550497, 93.0, -52.90744101897925, 5170.9183305670695});
            P2_vs_soc_segments.push_back({93.0, 100.1, -32.18045112942856, 3243.308270838856});
            zero_slope_threashold_P2_vs_soc = 1.297596410616389;
            // min_non_zero_slope = 1.4417737895737657
        }
        else if(SE_enum == xfc_450kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 25.24075915850751, 654.1945671167897});
            P2_vs_soc_segments.push_back({3.0, 4.0, 19.57429576311191, 671.1939573029765});
            P2_vs_soc_segments.push_back({4.0, 10.0, 6.0819479580768, 725.163348523117});
            P2_vs_soc_segments.push_back({10.0, 86.44766494081131, 1.4363756787739141, 771.6190713161458});
            P2_vs_soc_segments.push_back({86.44766494081131, 93.0, -79.3611615284689, 7756.377495850607});
            P2_vs_soc_segments.push_back({93.0, 100.1, -48.27067669414284, 4864.962406258284});
            zero_slope_threashold_P2_vs_soc = 1.2927381108965228;
            // min_non_zero_slope = 1.4363756787739141
        }
		else
		{
			std::cout << "Error: P2_vs_soc is not defigned in the factory_EV_charge_model for EV_enum:" << EV_enum << " and SE_enum:" << SE_enum << std::endl;
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
    // Battery Size here is Usable Battery Size
    
    bool is_success = true;

    if(vehicle_type == ld_50kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 50;	
	}
	else if(vehicle_type == ld_100kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 100;	
	}
    else if(vehicle_type == md_200kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 200;	
	}
	else if(vehicle_type == hd_300kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 300;	
	}
	else if(vehicle_type == hd_400kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 400;
	}
	else if(vehicle_type == hd_600kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 600;	
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

factory_supply_equipment_model::factory_supply_equipment_model(charge_event_queuing_inputs& CE_queuing_inputs_)
{
    this->CE_queuing_inputs = CE_queuing_inputs_;
}


void factory_supply_equipment_model::get_supply_equipment_model(bool building_charge_profile_library, SE_configuration& SE_config, get_base_load_forecast* baseLD_forecaster, manage_L2_control_strategy_parameters* manage_L2_control, supply_equipment& return_val)
{
	double P2_limit_kW, standby_acP_kW, standby_acQ_kVAR;
	
	supply_equipment_enum SE_enum = SE_config.supply_equipment_type;
	
	standby_acP_kW = 0;
	standby_acQ_kVAR = 0;
    
/*
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
*/
    
    if(SE_enum == L2_7200W)
	{
		P2_limit_kW = 6.624;
        standby_acP_kW = 0;
        standby_acQ_kVAR = 0;
	}
    else if(SE_enum == L2_17280W)
	{
		P2_limit_kW = 15.8976;
        standby_acP_kW = 0;
        standby_acQ_kVAR = 0;
	}
    else if(SE_enum == xfc_20kW)
	{
		P2_limit_kW = 20;
        standby_acP_kW = 0.100;  
		standby_acQ_kVAR = -0.590;
	}
    else if(SE_enum == xfc_50kW)
	{
		P2_limit_kW = 50;
        standby_acP_kW = 0.100;  
		standby_acQ_kVAR = -0.590;
	}
    else if(SE_enum == xfc_90kW)
	{
		P2_limit_kW = 90;
        standby_acP_kW = 0.100;  
		standby_acQ_kVAR = -0.590;
	}
    else if(SE_enum == xfc_150kW)
	{
		P2_limit_kW = 150;
        standby_acP_kW = 0.170;
		standby_acQ_kVAR = -0.445;
	}
    else if(SE_enum == xfc_180kW)
	{
		P2_limit_kW = 180;
        standby_acP_kW = 0.170;
		standby_acQ_kVAR = -0.445;
	}
    else if(SE_enum == xfc_450kW)
	{
		P2_limit_kW = 450;
        standby_acP_kW = 0.170;
		standby_acQ_kVAR = -0.445;
	}
	else
		std::cout << "WARNING: get_supply_equipment_model is not defigned in the factory_supply_equipment_model for SE_enum:" << SE_enum << std::endl;
    
	//============================
    
	supply_equipment_load load(P2_limit_kW, standby_acP_kW, standby_acQ_kVAR, SE_config, this->CE_queuing_inputs);
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
    else if(SE_type == xfc_20kW)  // Copied data from L2 Charger
    {
        // {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-30, 0, first, (1.1-1)/(-30-0), 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 4, second, -0.005, 0.045, 0.82, 0, 0}); 
		inv_eff_from_P2_vec.push_back({4, 30, first, 0, 0.92, 0, 0, 0});
		
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-30, 0, first, (-1+0.9)/(-30-0), -0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 6, third, -0.00038737, 0.00591216, -0.03029164, -0.9462841, 0});
		inv_pf_from_P3_vec.push_back({6, 30, first, 0, -0.9988681, 0, 0, 0});
    }
    else if(SE_type == xfc_50kW || SE_type == xfc_90kW)
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
    else if(SE_type == xfc_150kW || SE_type == xfc_180kW || SE_type == xfc_450kW)
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

