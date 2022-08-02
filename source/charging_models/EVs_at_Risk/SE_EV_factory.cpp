
//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								              EVs-At-Risk  Project
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
        else if(EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 15.8976});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
        else if(EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 100.1, 0.0, 15.8976});
            zero_slope_threashold_P2_vs_soc = 900000.0;
            // min_non_zero_slope = 1000000
        }
    }
	else
	{
        if(SE_enum == xfc_150 && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 5.617623090661083, 124.64587923089042});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.4433547003052354, 137.16868440195796});
            P2_vs_soc_segments.push_back({10.0, 71.46359084873505, 0.28079783503165895, 148.79425305469374});
            P2_vs_soc_segments.push_back({71.46359084873505, 93.0, -6.38663967643269, 625.2732793834901});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.2527180515284931;
            // min_non_zero_slope = 0.28079783503165895
        }
        else if(SE_enum == xfc_150 && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 5.019868538599169, 122.38229534071229});
            P2_vs_soc_segments.push_back({3.0, 4.0, 3.0600411622216184, 128.26177746984493});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.216147395224588, 135.63735253783307});
            P2_vs_soc_segments.push_back({10.0, 85.0076631195378, 0.2716779788933731, 145.0820467011452});
            P2_vs_soc_segments.push_back({85.0076631195378, 93.0, -13.213633389200002, 1291.4368525421003});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.2445101810040358;
            // min_non_zero_slope = 0.2716779788933731
        }
        else if(SE_enum == xfc_150 && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 4.4642626723420005, 124.99935482557609});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.0677880183209867, 138.58525344166014});
            P2_vs_soc_segments.push_back({10.0, 92.38650810537449, 0.27087912085065996, 146.55434241636343});
            P2_vs_soc_segments.push_back({92.38650810537449, 100.1, -20.877192983499974, 2100.350877297997});
            zero_slope_threashold_P2_vs_soc = 0.24379120876559396;
            // min_non_zero_slope = 0.27087912085065996
        }
        else if(SE_enum == xfc_150 && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 6.696394008513, 187.49903223836412});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.6016820274814798, 207.87788016249021});
            P2_vs_soc_segments.push_back({10.0, 92.38650810537449, 0.4063186812759899, 219.8315136245451});
            P2_vs_soc_segments.push_back({92.38650810537449, 100.1, -31.315789475249957, 3150.526315946996});
            zero_slope_threashold_P2_vs_soc = 0.3656868131483909;
            // min_non_zero_slope = 0.4063186812759899
        }
        else if(SE_enum == xfc_150 && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 6.696394010425876, 187.4990322919245});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.6016820279390087, 207.87788022187195});
            P2_vs_soc_segments.push_back({10.0, 94.42134030035675, 0.4063186813920575, 219.83151368734147});
            P2_vs_soc_segments.push_back({94.42134030035675, 100.1, -41.75438596700008, 4200.701754596009});
            zero_slope_threashold_P2_vs_soc = 0.3656868132528518;
            // min_non_zero_slope = 0.4063186813920575
        }
        else if(SE_enum == xfc_150 && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 6.696394009788252, 187.49903227407106});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.601682027786505, 207.87788020207802});
            P2_vs_soc_segments.push_back({10.0, 96.46928826437762, 0.40631868135336796, 219.83151366640942});
            P2_vs_soc_segments.push_back({96.46928826437762, 100.1, -62.63157895049987, 6301.052631893987});
            zero_slope_threashold_P2_vs_soc = 0.36568681321803115;
            // min_non_zero_slope = 0.40631868135336796
        }
        else if(SE_enum == xfc_150 && EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 6.696394009469443, 187.49903226514434});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.6016820277102406, 207.87788019218115});
            P2_vs_soc_segments.push_back({10.0, 97.49822035038136, 0.4063186813340238, 219.83151365594333});
            P2_vs_soc_segments.push_back({97.49822035038136, 100.1, -83.50877193399954, 8401.403509191954});
            zero_slope_threashold_P2_vs_soc = 0.36568681320062146;
            // min_non_zero_slope = 0.4063186813340238
        }
        else if(SE_enum == xfc_150 && EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 6.771283784305358, 189.59594596054998});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.6195945947193486, 210.20270271889402});
            P2_vs_soc_segments.push_back({10.0, 98.08946120751611, 0.41086278589443354, 222.29002080714318});
            P2_vs_soc_segments.push_back({98.08946120751611, 100.1, -104.3859649175005, 10501.75438649005});
            zero_slope_threashold_P2_vs_soc = 0.3697765073049902;
            // min_non_zero_slope = 0.41086278589443354
        }
        else if(SE_enum == xfc_350 && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_350 && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 12.51729473245665, 283.21124200604});
            P2_vs_soc_segments.push_back({3.0, 10.0, 3.2704105251700035, 310.95189462789995});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.6348216064257896, 337.3077838153421});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -12.76050606840385, 1249.2960117080581});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.5713394457832106;
            // min_non_zero_slope = 0.6348216064257896
        }
        else if(SE_enum == xfc_350 && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 16.525567949819884, 351.65195242759415});
            P2_vs_soc_segments.push_back({3.0, 4.0, 4.548794221567978, 387.58227361234987});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.047197486223866, 389.58866055372636});
            P2_vs_soc_segments.push_back({10.0, 79.37589851129248, 0.8015852284950763, 422.04478313101424});
            P2_vs_soc_segments.push_back({79.37589851129248, 93.0, -26.453720509489646, 2585.459165283537});
            P2_vs_soc_segments.push_back({93.0, 100.1, -16.09022556471428, 1621.654135419428});
            zero_slope_threashold_P2_vs_soc = 0.7214267056455687;
            // min_non_zero_slope = 0.8015852284950763
        }
        else if(SE_enum == xfc_350 && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 24.78835192472982, 527.4779286413913});
            P2_vs_soc_segments.push_back({3.0, 4.0, 6.823191332351967, 581.3734104185248});
            P2_vs_soc_segments.push_back({4.0, 10.0, 6.070796229335799, 584.3829908305895});
            P2_vs_soc_segments.push_back({10.0, 79.37589851129248, 1.2023778427426144, 633.0671746965213});
            P2_vs_soc_segments.push_back({79.37589851129248, 93.0, -39.68058076423447, 3878.1887479253055});
            P2_vs_soc_segments.push_back({93.0, 100.1, -24.13533834707142, 2432.481203129142});
            zero_slope_threashold_P2_vs_soc = 1.082140058468353;
            // min_non_zero_slope = 1.2023778427426144
        }
        else if(SE_enum == xfc_350 && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 22.9111589508695, 536.3200340480914});
            P2_vs_soc_segments.push_back({3.0, 4.0, 11.56755223272807, 570.3508542025157});
            P2_vs_soc_segments.push_back({4.0, 10.0, 5.569543098642392, 594.3428907388584});
            P2_vs_soc_segments.push_back({10.0, 83.77597648720116, 1.1996787877576258, 638.0415338477061});
            P2_vs_soc_segments.push_back({83.77597648720116, 93.0, -52.90744101897933, 5170.918330567078});
            P2_vs_soc_segments.push_back({93.0, 100.1, -32.18045112942856, 3243.308270838856});
            zero_slope_threashold_P2_vs_soc = 1.0797109089818633;
            // min_non_zero_slope = 1.1996787877576258
        }
        else if(SE_enum == xfc_350 && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 19.695276499377215, 551.467741982562});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.710829493489699, 611.405530006112});
            P2_vs_soc_segments.push_back({10.0, 88.59137653488752, 1.1950549451569656, 646.5632754894394});
            P2_vs_soc_segments.push_back({88.59137653488752, 100.1, -62.63157895050001, 6301.052631894});
            zero_slope_threashold_P2_vs_soc = 1.075549450641269;
            // min_non_zero_slope = 1.1950549451569656
        }
        else if(SE_enum == xfc_350 && EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 19.695276498439537, 551.4677419563069});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.710829493265419, 611.4055299770034});
            P2_vs_soc_segments.push_back({10.0, 91.55241881576407, 1.1950549451000696, 646.5632754586568});
            P2_vs_soc_segments.push_back({91.55241881576407, 100.1, -83.50877193400005, 8401.403509192005});
            zero_slope_threashold_P2_vs_soc = 1.0755494505900627;
            // min_non_zero_slope = 1.1950549451000696
        }
        else if(SE_enum == xfc_350 && EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 19.915540542074574, 557.6351351780881});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.763513513880435, 618.2432432908647});
            P2_vs_soc_segments.push_back({10.0, 93.26215801350378, 1.20841995851304, 653.7941788445387});
            P2_vs_soc_segments.push_back({93.26215801350378, 100.1, -104.38596491750005, 10501.754386490007});
            zero_slope_threashold_P2_vs_soc = 1.087577962661736;
            // min_non_zero_slope = 1.20841995851304
        }
        else if(SE_enum == xfc_500kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_500kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 12.51729473245665, 283.21124200604});
            P2_vs_soc_segments.push_back({3.0, 10.0, 3.2704105251700035, 310.95189462789995});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.6348216064257896, 337.3077838153421});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -12.76050606840385, 1249.2960117080581});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.5713394457832106;
            // min_non_zero_slope = 0.6348216064257896
        }
        else if(SE_enum == xfc_500kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 16.525567949819884, 351.65195242759415});
            P2_vs_soc_segments.push_back({3.0, 4.0, 4.548794221567978, 387.58227361234987});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.047197486223866, 389.58866055372636});
            P2_vs_soc_segments.push_back({10.0, 79.37589851129248, 0.8015852284950763, 422.04478313101424});
            P2_vs_soc_segments.push_back({79.37589851129248, 93.0, -26.453720509489646, 2585.459165283537});
            P2_vs_soc_segments.push_back({93.0, 100.1, -16.09022556471428, 1621.654135419428});
            zero_slope_threashold_P2_vs_soc = 0.7214267056455687;
            // min_non_zero_slope = 0.8015852284950763
        }
        else if(SE_enum == xfc_500kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 24.78835192472982, 527.4779286413913});
            P2_vs_soc_segments.push_back({3.0, 4.0, 6.823191332351967, 581.3734104185248});
            P2_vs_soc_segments.push_back({4.0, 10.0, 6.070796229335799, 584.3829908305895});
            P2_vs_soc_segments.push_back({10.0, 79.37589851129248, 1.2023778427426144, 633.0671746965213});
            P2_vs_soc_segments.push_back({79.37589851129248, 93.0, -39.68058076423447, 3878.1887479253055});
            P2_vs_soc_segments.push_back({93.0, 100.1, -24.13533834707142, 2432.481203129142});
            zero_slope_threashold_P2_vs_soc = 1.082140058468353;
            // min_non_zero_slope = 1.2023778427426144
        }
        else if(SE_enum == xfc_500kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 22.9111589508695, 536.3200340480914});
            P2_vs_soc_segments.push_back({3.0, 4.0, 11.56755223272807, 570.3508542025157});
            P2_vs_soc_segments.push_back({4.0, 10.0, 5.569543098642392, 594.3428907388584});
            P2_vs_soc_segments.push_back({10.0, 83.77597648720116, 1.1996787877576258, 638.0415338477061});
            P2_vs_soc_segments.push_back({83.77597648720116, 93.0, -52.90744101897933, 5170.918330567078});
            P2_vs_soc_segments.push_back({93.0, 100.1, -32.18045112942856, 3243.308270838856});
            zero_slope_threashold_P2_vs_soc = 1.0797109089818633;
            // min_non_zero_slope = 1.1996787877576258
        }
        else if(SE_enum == xfc_500kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 19.695276499377215, 551.467741982562});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.710829493489699, 611.405530006112});
            P2_vs_soc_segments.push_back({10.0, 88.59137653488752, 1.1950549451569656, 646.5632754894394});
            P2_vs_soc_segments.push_back({88.59137653488752, 100.1, -62.63157895050001, 6301.052631894});
            zero_slope_threashold_P2_vs_soc = 1.075549450641269;
            // min_non_zero_slope = 1.1950549451569656
        }
        else if(SE_enum == xfc_500kW && EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 19.695276498439537, 551.4677419563069});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.710829493265419, 611.4055299770034});
            P2_vs_soc_segments.push_back({10.0, 91.55241881576407, 1.1950549451000696, 646.5632754586568});
            P2_vs_soc_segments.push_back({91.55241881576407, 100.1, -83.50877193400005, 8401.403509192005});
            zero_slope_threashold_P2_vs_soc = 1.0755494505900627;
            // min_non_zero_slope = 1.1950549451000696
        }
        else if(SE_enum == xfc_500kW && EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 19.915540542074574, 557.6351351780881});
            P2_vs_soc_segments.push_back({4.0, 10.0, 4.763513513880435, 618.2432432908647});
            P2_vs_soc_segments.push_back({10.0, 93.26215801350378, 1.20841995851304, 653.7941788445387});
            P2_vs_soc_segments.push_back({93.26215801350378, 100.1, -104.38596491750005, 10501.754386490007});
            zero_slope_threashold_P2_vs_soc = 1.087577962661736;
            // min_non_zero_slope = 1.20841995851304
        }
        else if(SE_enum == xfc_1000kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_1000kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 12.51729473245665, 283.21124200604});
            P2_vs_soc_segments.push_back({3.0, 10.0, 3.2704105251700035, 310.95189462789995});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.6348216064257896, 337.3077838153421});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -12.76050606840385, 1249.2960117080581});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.5713394457832106;
            // min_non_zero_slope = 0.6348216064257896
        }
        else if(SE_enum == xfc_1000kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 24.143511712364646, 542.7849693228944});
            P2_vs_soc_segments.push_back({3.0, 10.0, 6.273517022205756, 596.3949533933711});
            P2_vs_soc_segments.push_back({10.0, 69.27462598528474, 1.2186444564972314, 646.9436790504564});
            P2_vs_soc_segments.push_back({69.27462598528474, 93.0, -25.54655870573077, 2501.0931175339615});
            P2_vs_soc_segments.push_back({93.0, 100.1, -16.09022556471428, 1621.654135419428});
            zero_slope_threashold_P2_vs_soc = 1.0967800108475083;
            // min_non_zero_slope = 1.2186444564972314
        }
        else if(SE_enum == xfc_1000kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 36.21526756854697, 814.1774539843416});
            P2_vs_soc_segments.push_back({3.0, 10.0, 9.410275533308633, 894.5924300900566});
            P2_vs_soc_segments.push_back({10.0, 69.27462598528474, 1.8279666847458471, 970.4155185756845});
            P2_vs_soc_segments.push_back({69.27462598528474, 93.0, -38.319838058596154, 3751.639676300942});
            P2_vs_soc_segments.push_back({93.0, 100.1, -24.13533834707142, 2432.481203129142});
            zero_slope_threashold_P2_vs_soc = 1.6451700162712624;
            // min_non_zero_slope = 1.8279666847458471
        }
        else if(SE_enum == xfc_1000kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 36.63632021013128, 777.7564016147287});
            P2_vs_soc_segments.push_back({3.0, 10.0, 9.064410874621704, 860.4721296212574});
            P2_vs_soc_segments.push_back({10.0, 76.96478054467563, 1.7725650232628638, 933.3905881348459});
            P2_vs_soc_segments.push_back({76.96478054467563, 93.0, -51.09311741146153, 5002.186235067922});
            P2_vs_soc_segments.push_back({93.0, 100.1, -32.18045112942856, 3243.308270838856});
            zero_slope_threashold_P2_vs_soc = 1.5953085209365774;
            // min_non_zero_slope = 1.7725650232628638
        }
        else if(SE_enum == xfc_1000kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 33.308124827399, 787.0469348700666});
            P2_vs_soc_segments.push_back({3.0, 4.0, 17.609192570054, 834.1437316421017});
            P2_vs_soc_segments.push_back({4.0, 10.0, 8.090720110980348, 872.2176214783962});
            P2_vs_soc_segments.push_back({10.0, 84.08466589019737, 1.757393651151549, 935.5508860766843});
            P2_vs_soc_segments.push_back({84.08466589019737, 93.0, -79.36116152846905, 7756.377495850622});
            P2_vs_soc_segments.push_back({93.0, 100.1, -48.27067669414284, 4864.962406258284});
            zero_slope_threashold_P2_vs_soc = 1.5816542860363942;
            // min_non_zero_slope = 1.757393651151549
        }
        else if(SE_enum == xfc_1000kW && EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 29.553738860176978, 804.7311453623216});
            P2_vs_soc_segments.push_back({3.0, 4.0, 27.09791437555665, 812.0986188161826});
            P2_vs_soc_segments.push_back({4.0, 10.0, 7.088213844737678, 892.1374209394584});
            P2_vs_soc_segments.push_back({10.0, 87.35344251585242, 1.751995540405572, 945.4996039827796});
            P2_vs_soc_segments.push_back({87.35344251585242, 93.0, -105.81488203795847, 10341.836661134137});
            P2_vs_soc_segments.push_back({93.0, 100.1, -64.36090225885712, 6486.616541677712});
            zero_slope_threashold_P2_vs_soc = 1.576795986365015;
            // min_non_zero_slope = 1.751995540405572
        }
        else if(SE_enum == xfc_1000kW && EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 29.18025000224768, 817.0470000629349});
            P2_vs_soc_segments.push_back({4.0, 10.0, 6.979500000537595, 905.8500000697752});
            P2_vs_soc_segments.push_back({10.0, 89.9032220733732, 1.7705769232133064, 957.939230843018});
            P2_vs_soc_segments.push_back({89.9032220733732, 100.1, -104.3859649175, 10501.754386490002});
            zero_slope_threashold_P2_vs_soc = 1.593519230891976;
            // min_non_zero_slope = 1.7705769232133064
        }
        else if(SE_enum == xfc_2000kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_2000kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 12.51729473245665, 283.21124200604});
            P2_vs_soc_segments.push_back({3.0, 10.0, 3.2704105251700035, 310.95189462789995});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.6348216064257896, 337.3077838153421});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -12.76050606840385, 1249.2960117080581});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.5713394457832106;
            // min_non_zero_slope = 0.6348216064257896
        }
        else if(SE_enum == xfc_2000kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 25.059649124059966, 566.98947371256});
            P2_vs_soc_segments.push_back({3.0, 10.0, 6.547368421380007, 622.5263158205998});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 1.2709141274873685, 675.2908587595263});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -25.546558705730774, 2501.0931175339624});
            P2_vs_soc_segments.push_back({93.0, 100.1, -16.09022556471428, 1621.654135419428});
            zero_slope_threashold_P2_vs_soc = 1.1438227147386317;
            // min_non_zero_slope = 1.2709141274873685
        }
        else if(SE_enum == xfc_2000kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 37.589473686089946, 850.4842105688399});
            P2_vs_soc_segments.push_back({3.0, 10.0, 9.82105263207001, 933.7894737308998});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 1.9063711912310528, 1012.9362881392893});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -38.31983805859616, 3751.6396763009434});
            P2_vs_soc_segments.push_back({93.0, 100.1, -24.13533834707142, 2432.481203129142});
            zero_slope_threashold_P2_vs_soc = 1.7157340721079475;
            // min_non_zero_slope = 1.9063711912310528
        }
        else if(SE_enum == xfc_2000kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 50.11929824811993, 1133.97894742512});
            P2_vs_soc_segments.push_back({3.0, 10.0, 13.094736842760014, 1245.0526316411997});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 2.541828254974737, 1350.5817175190525});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -51.09311741146155, 5002.186235067925});
            P2_vs_soc_segments.push_back({93.0, 100.1, -32.18045112942856, 3243.308270838856});
            zero_slope_threashold_P2_vs_soc = 2.2876454294772635;
            // min_non_zero_slope = 2.541828254974737
        }
        else if(SE_enum == xfc_2000kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 72.50049185041229, 1630.2031760919858});
            P2_vs_soc_segments.push_back({3.0, 10.0, 18.841462497128997, 1791.1802641518357});
            P2_vs_soc_segments.push_back({10.0, 69.24422604132654, 3.6599247078455552, 1942.9956420446701});
            P2_vs_soc_segments.push_back({69.24422604132654, 93.0, -76.63967611719227, 7503.279352601881});
            P2_vs_soc_segments.push_back({93.0, 100.1, -48.27067669414284, 4864.962406258284});
            zero_slope_threashold_P2_vs_soc = 3.2939322370609996;
            // min_non_zero_slope = 3.6599247078455552
        }
        else if(SE_enum == xfc_2000kW && EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 73.34259711028085, 1557.361070737169});
            P2_vs_soc_segments.push_back({3.0, 10.0, 18.149733172790338, 1722.9396625496406});
            P2_vs_soc_segments.push_back({10.0, 76.94140334485147, 3.5491213835502085, 1868.9457804420417});
            P2_vs_soc_segments.push_back({76.94140334485147, 93.0, -102.18623482292311, 10004.372470135851});
            P2_vs_soc_segments.push_back({93.0, 100.1, -64.36090225885712, 6486.616541677712});
            zero_slope_threashold_P2_vs_soc = 3.1942092451951876;
            // min_non_zero_slope = 3.5491213835502085
        }
        else if(SE_enum == xfc_2000kW && EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 71.4577773537474, 1574.312573169373});
            P2_vs_soc_segments.push_back({3.0, 4.0, 25.464850039162634, 1712.2913551131273});
            P2_vs_soc_segments.push_back({4.0, 10.0, 17.45464452354518, 1744.332177175597});
            P2_vs_soc_segments.push_back({10.0, 81.30667131540051, 3.5634451397963116, 1883.2441710130859});
            P2_vs_soc_segments.push_back({81.30667131540051, 93.0, -132.2686025474482, 12927.295826417685});
            P2_vs_soc_segments.push_back({93.0, 100.1, -80.45112782357143, 8108.270677097142});
            zero_slope_threashold_P2_vs_soc = 3.2071006258166803;
            // min_non_zero_slope = 3.5634451397963116
        }
        else if(SE_enum == xfc_3000kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 6.2649122810149995, 141.74736842814});
            P2_vs_soc_segments.push_back({3.0, 10.0, 1.6368421053449982, 155.63157895515002});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.31772853187184213, 168.82271468988156});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -6.386639676432694, 625.2732793834906});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.28595567868465793;
            // min_non_zero_slope = 0.31772853187184213
        }
        else if(SE_enum == xfc_3000kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 12.51729473245665, 283.21124200604});
            P2_vs_soc_segments.push_back({3.0, 10.0, 3.2704105251700035, 310.95189462789995});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 0.6348216064257896, 337.3077838153421});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -12.76050606840385, 1249.2960117080581});
            P2_vs_soc_segments.push_back({93.0, 100.1, -8.037067666357142, 810.016240317714});
            zero_slope_threashold_P2_vs_soc = 0.5713394457832106;
            // min_non_zero_slope = 0.6348216064257896
        }
        else if(SE_enum == xfc_3000kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 25.059649124059966, 566.98947371256});
            P2_vs_soc_segments.push_back({3.0, 10.0, 6.547368421380007, 622.5263158205998});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 1.2709141274873685, 675.2908587595263});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -25.546558705730774, 2501.0931175339624});
            P2_vs_soc_segments.push_back({93.0, 100.1, -16.09022556471428, 1621.654135419428});
            zero_slope_threashold_P2_vs_soc = 1.1438227147386317;
            // min_non_zero_slope = 1.2709141274873685
        }
        else if(SE_enum == xfc_3000kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 37.589473686089946, 850.4842105688399});
            P2_vs_soc_segments.push_back({3.0, 10.0, 9.82105263207001, 933.7894737308998});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 1.9063711912310528, 1012.9362881392893});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -38.31983805859616, 3751.6396763009434});
            P2_vs_soc_segments.push_back({93.0, 100.1, -24.13533834707142, 2432.481203129142});
            zero_slope_threashold_P2_vs_soc = 1.7157340721079475;
            // min_non_zero_slope = 1.9063711912310528
        }
        else if(SE_enum == xfc_3000kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 50.11929824811993, 1133.97894742512});
            P2_vs_soc_segments.push_back({3.0, 10.0, 13.094736842760014, 1245.0526316411997});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172734, 2.541828254974737, 1350.5817175190525});
            P2_vs_soc_segments.push_back({68.08256207172734, 93.0, -51.09311741146155, 5002.186235067925});
            P2_vs_soc_segments.push_back({93.0, 100.1, -32.18045112942856, 3243.308270838856});
            zero_slope_threashold_P2_vs_soc = 2.2876454294772635;
            // min_non_zero_slope = 2.541828254974737
        }
        else if(SE_enum == xfc_3000kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 75.17894737217999, 1700.9684211376798});
            P2_vs_soc_segments.push_back({3.0, 10.0, 19.64210526413998, 1867.5789474618});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172732, 3.8127423824621105, 2025.8725762785784});
            P2_vs_soc_segments.push_back({68.08256207172732, 93.0, -76.63967611719228, 7503.279352601882});
            P2_vs_soc_segments.push_back({93.0, 100.1, -48.27067669414284, 4864.962406258284});
            zero_slope_threashold_P2_vs_soc = 3.4314681442158994;
            // min_non_zero_slope = 3.8127423824621105
        }
        else if(SE_enum == xfc_3000kW && EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 100.23859649623999, 2267.9578948502403});
            P2_vs_soc_segments.push_back({3.0, 10.0, 26.18947368551997, 2490.1052632824003});
            P2_vs_soc_segments.push_back({10.0, 68.08256207172732, 5.083656509949474, 2701.1634350381055});
            P2_vs_soc_segments.push_back({68.08256207172732, 93.0, -102.18623482292308, 10004.372470135846});
            P2_vs_soc_segments.push_back({93.0, 100.1, -64.36090225885712, 6486.616541677712});
            zero_slope_threashold_P2_vs_soc = 4.575290858954527;
            // min_non_zero_slope = 5.083656509949474
        }
        else if(SE_enum == xfc_3000kW && EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 108.59649123349998, 2393.6842106460003});
            P2_vs_soc_segments.push_back({3.0, 10.0, 27.744360903642843, 2636.2406016355717});
            P2_vs_soc_segments.push_back({10.0, 72.45155249679897, 5.401662050131588, 2859.667590170684});
            P2_vs_soc_segments.push_back({72.45155249679897, 93.0, -127.73279352865379, 12505.465587669803});
            P2_vs_soc_segments.push_back({93.0, 100.1, -80.45112782357143, 8108.270677097142});
            zero_slope_threashold_P2_vs_soc = 4.861495845118429;
            // min_non_zero_slope = 5.401662050131588
        }
        else if(SE_enum == dwc_100kW && EV_enum == ld_50kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 3.0, 4.131391988613595, 87.91298812597856});
            P2_vs_soc_segments.push_back({3.0, 4.0, 1.137198555109764, 96.89556842649004});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.011799371844463, 97.39716515955125});
            P2_vs_soc_segments.push_back({10.0, 79.37589850730012, 0.20039630716987314, 105.51119580629715});
            P2_vs_soc_segments.push_back({79.37589850730012, 93.0, -6.613430127372406, 646.3647913208837});
            P2_vs_soc_segments.push_back({93.0, 100.1, -4.02255639117857, 405.413533854857});
            zero_slope_threashold_P2_vs_soc = 0.18035667645288583;
            // min_non_zero_slope = 0.20039630716987314
        }
        else if(SE_enum == dwc_100kW && EV_enum == ld_100kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 3.2825460808634848, 91.91129026417764});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7851382483490266, 101.90092159423547});
            P2_vs_soc_segments.push_back({10.0, 88.5795762311163, 0.19917582404926223, 107.76054583723311});
            P2_vs_soc_segments.push_back({88.5795762311163, 100.1, -10.428157891083332, 1049.1252627903334});
            zero_slope_threashold_P2_vs_soc = 0.179258241644336;
            // min_non_zero_slope = 0.19917582404926223
        }
        else if(SE_enum == dwc_100kW && EV_enum == md_200kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 3.282546082604413, 91.9112903129236});
            P2_vs_soc_segments.push_back({4.0, 10.0, 0.7851382487654301, 101.90092164827954});
            P2_vs_soc_segments.push_back({10.0, 94.54144352797196, 0.19917582415489724, 107.76054589438486});
            P2_vs_soc_segments.push_back({94.54144352797196, 100.1, -20.877192983500027, 2100.3508772980026});
            zero_slope_threashold_P2_vs_soc = 0.17925824173940752;
            // min_non_zero_slope = 0.19917582415489724
        }
        else if(SE_enum == dwc_100kW && EV_enum == hd_300kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 4.923819123906619, 137.8669354693854});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.177707373148145, 152.8513824724193});
            P2_vs_soc_segments.push_back({10.0, 94.54144352797196, 0.29876373623234587, 161.64081884157727});
            P2_vs_soc_segments.push_back({94.54144352797196, 100.1, -31.315789475250035, 3150.5263159470037});
            zero_slope_threashold_P2_vs_soc = 0.2688873626091113;
            // min_non_zero_slope = 0.29876373623234587
        }
        else if(SE_enum == dwc_100kW && EV_enum == hd_400kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 4.9238191253131465, 137.866935508768});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.177707373484567, 152.8513825160823});
            P2_vs_soc_segments.push_back({10.0, 96.04657354332714, 0.2987637363176892, 161.6408188877511});
            P2_vs_soc_segments.push_back({96.04657354332714, 100.1, -41.754385966999884, 4200.701754595989});
            zero_slope_threashold_P2_vs_soc = 0.2688873626859203;
            // min_non_zero_slope = 0.2987637363176892
        }
        else if(SE_enum == dwc_100kW && EV_enum == hd_600kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 4.923819124844304, 137.8669354956405});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.1777073733724248, 152.851382501528});
            P2_vs_soc_segments.push_back({10.0, 97.5588492117089, 0.2987637362892414, 161.64081887235986});
            P2_vs_soc_segments.push_back({97.5588492117089, 100.1, -62.63157895049983, 6301.052631893983});
            zero_slope_threashold_P2_vs_soc = 0.26888736266031726;
            // min_non_zero_slope = 0.2987637362892414
        }
        else if(SE_enum == dwc_100kW && EV_enum == hd_800kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 4.923819124609884, 137.86693548907672});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.1777073733163548, 152.85138249425086});
            P2_vs_soc_segments.push_back({10.0, 98.31768258578957, 0.2987637362750174, 161.6408188646642});
            P2_vs_soc_segments.push_back({98.31768258578957, 100.1, -83.50877193400026, 8401.403509192027});
            zero_slope_threashold_P2_vs_soc = 0.26888736264751567;
            // min_non_zero_slope = 0.2987637362750174
        }
        else if(SE_enum == dwc_100kW && EV_enum == hd_1000kWh)
        {
            P2_vs_soc_segments.push_back({-0.1, 4.0, 4.978885135518643, 139.40878379452204});
            P2_vs_soc_segments.push_back({4.0, 10.0, 1.1908783784701087, 154.56081082271618});
            P2_vs_soc_segments.push_back({10.0, 98.75342864712539, 0.30210498962826, 163.44854471113467});
            P2_vs_soc_segments.push_back({98.75342864712539, 100.1, -104.38596491749996, 10501.754386489998});
            zero_slope_threashold_P2_vs_soc = 0.271894490665434;
            // min_non_zero_slope = 0.30210498962826
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
	else if(vehicle_type == hd_800kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 800;	
	}
	else if(vehicle_type == hd_1000kWh)
	{
		bat_chem = NMC;
		battery_size_kWh = 1000;	
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
    
    standby_acP_kW = 0.2;
    standby_acQ_kVAR = -0.03;
    
    if(SE_enum == L2_7200)
	{
		P2_limit_kW = 6.624;
        standby_acP_kW = 0;
        standby_acQ_kVAR = 0;
	}
    else if(SE_enum == L2_17280)
	{
		P2_limit_kW = 15.8976;
        standby_acP_kW = 0;
        standby_acQ_kVAR = 0;
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
    else if(SE_enum == xfc_500kW)
	{
		P2_limit_kW = 500;
	}
	else if(SE_enum == xfc_1000kW)
	{
		P2_limit_kW = 1000;
	}
    else if(SE_enum == xfc_2000kW)
	{
		P2_limit_kW = 2000;
	}
    else if(SE_enum == xfc_3000kW)
	{
		P2_limit_kW = 3000;
	}
    else if(SE_enum == dwc_100kW)
	{
		P2_limit_kW = 100;
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
	else if(SE_type == xfc_500kW)
	{
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-1000, 0, first, (1.1-1)/1000, 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 9.9509951, fourth, -5.32294E-05, 0.00161341, -0.020375254, 0.153430262, 0.1});  
		inv_eff_from_P2_vec.push_back({9.9509951, 49.9549955, fourth, -1.41183E-07, 2.19219E-05, -0.001317196, 0.038812335, 0.40120321});  
		inv_eff_from_P2_vec.push_back({49.9549955, 124.9624962, fourth, -8.95362E-10, 3.96468E-07, -6.90956E-05, 0.005879087, 0.741555738});  
		inv_eff_from_P2_vec.push_back({124.9624962,	500, fourth, -2.58346E-12, 4.05019E-09, -2.42506E-06, 0.000615186, 0.906293666});  
		inv_eff_from_P2_vec.push_back({500, 1000, first, 0, 0.952430816, 0, 0, 0});
        
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-1000, 0, first, (0.9-1)/1000, 0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 9.9509951, fourth, 3.30449E-08, -1.64406E-06, 4.81836E-05, -0.001229586, -0.96});
		inv_pf_from_P3_vec.push_back({9.9509951, 49.9549955, fourth, 1.83122E-09, -3.19941E-07, 2.3503E-05, -0.000997071, -0.960868555});
		inv_pf_from_P3_vec.push_back({49.9549955, 124.9624962, fourth, 4.9509E-11, -2.28852E-08, 4.27128E-06, -0.000415058, -0.967886798});
		inv_pf_from_P3_vec.push_back({124.9624962, 500, fourth, 2.75481E-13, -4.37287E-10, 2.67801E-07, -7.93935E-05, -0.979114283});
		inv_pf_from_P3_vec.push_back({500, 1000, first, 0, -0.989303981, 0, 0, 0});    
	}
	else if(SE_type == xfc_1000kW)
	{
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-1000, 0, first, (1.1-1)/1000, 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 3.300330033, fourth, -3.07232E-05, 0.000587331, -0.007217139, 0.080972819, 0.1});  
		inv_eff_from_P2_vec.push_back({3.300330033, 19.9019902, fourth, -1.99155E-06, 0.000137261, -0.004017833, 0.069501611, 0.115784348});
		inv_eff_from_P2_vec.push_back({19.9019902, 99.909991, fourth, -8.82395E-09, 2.74024E-06, -0.000329299, 0.019406168, 0.40120321}); 
        inv_eff_from_P2_vec.push_back({99.909991, 1000, fourth, -6.01897E-13, 1.66165E-09, -1.68483E-06, 0.000728381, 0.84911447}); 
        inv_eff_from_P2_vec.push_back({1000, 10000, first, 0, 0.952427626, 0, 0, 0}); 
		
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-1000, 0, first, (0.9-1)/1000, 0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 19.9019902, fourth, 2.0653E-09, -2.05508E-07, 1.20459E-05, -0.000614793, -0.96});
		inv_pf_from_P3_vec.push_back({19.9019902, 99.909991, fourth, 1.14451E-10, -3.99927E-08, 5.87574E-06, -0.000498536, -0.960868555});
		inv_pf_from_P3_vec.push_back({99.909991, 249.9249925, fourth, 3.09432E-12, -2.86066E-09, 1.06782E-06, -0.000207529, -0.967886798});
		inv_pf_from_P3_vec.push_back({249.9249925, 1000, fourth, 1.72176E-14, -5.46609E-11, 6.69504E-08, -3.96968E-05, -0.979114283});
        inv_pf_from_P3_vec.push_back({1000, 10000, first, 0, -0.989303981, 0, 0, 0});
	}
    else if(SE_type == xfc_2000kW)
    {
        									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-1000, 0, first, (1.1-1)/1000, 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 6.600660066, fourth, -1.9202E-06, 7.34164E-05, -0.001804285, 0.04048641, 0.1});  
		inv_eff_from_P2_vec.push_back({6.600660066, 39.8039804, fourth, -1.24472E-07, 1.71577E-05, -0.001004458, 0.034750806, 0.115784348});
		inv_eff_from_P2_vec.push_back({39.8039804, 199.819982, fourth, -5.51497E-10, 3.4253E-07, -8.23247E-05, 0.009703084, 0.40120321}); 
        inv_eff_from_P2_vec.push_back({199.819982, 2000, fourth, -3.76186E-14, 2.07707E-10, -4.21206E-07, 0.00036419, 0.84911447}); 
        inv_eff_from_P2_vec.push_back({2000, 10000, first, 0, 0.952427626, 0, 0, 0}); 
		        
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-1000, 0, first, (0.9-1)/1000, 0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 39.8039804, fourth, 1.29082E-10, -2.56885E-08, 3.01148E-06, -0.000307397, -0.96});
		inv_pf_from_P3_vec.push_back({39.8039804, 199.819982, fourth, 7.1532E-12, -4.99908E-09, 1.46893E-06, -0.000249268, -0.960868555});
		inv_pf_from_P3_vec.push_back({199.819982, 499.849985, fourth, 1.93395E-13, -3.57582E-10, 2.66955E-07, -0.000103765, -0.967886798});
		inv_pf_from_P3_vec.push_back({499.849985, 2000, fourth, 1.0761E-15, -6.83261E-12, 1.67376E-08, -1.98484E-05, -0.979114283});
        inv_pf_from_P3_vec.push_back({2000, 10000, first, 0, -0.989303981, 0, 0, 0});
    }
    else if(SE_type == xfc_3000kW)
    {
        inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-1000, 0, first, (1.1-1)/1000, 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 9.900990099, fourth, -3.79299E-07, 2.1753E-05, -0.000801904, 0.02699094, 0.1});  
		inv_eff_from_P2_vec.push_back({9.900990099, 59.7059706, fourth, -2.45871E-08, 5.08376E-06, -0.000446426, 0.023167204, 0.115784348});
		inv_eff_from_P2_vec.push_back({59.7059706, 299.729973, fourth, -1.08938E-10, 1.0149E-07, -3.65888E-05, 0.006468723, 0.40120321}); 
        inv_eff_from_P2_vec.push_back({299.729973, 3000, fourth, -7.43083E-15, 6.15428E-11, -1.87203E-07, 0.000242794, 0.84911447}); 
        inv_eff_from_P2_vec.push_back({3000, 10000, first, 0, 0.952427626, 0, 0, 0}); 
                
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-1000, 0, first, (0.9-1)/1000, 0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 59.7059706, fourth, 2.54976E-11, -7.61141E-09, 1.33843E-06, -0.000204931, -0.96});
		inv_pf_from_P3_vec.push_back({59.7059706, 299.729973, fourth, 1.41298E-12, -1.48121E-09, 6.5286E-07, -0.000166179, -0.960868555});
		inv_pf_from_P3_vec.push_back({299.729973, 749.7749775, fourth, 3.82014E-14, -1.0595E-10, 1.18647E-07, -6.91763E-05, -0.967886798});
		inv_pf_from_P3_vec.push_back({749.7749775, 3000, fourth, 2.12563E-16, -2.02448E-12, 7.43893E-09, -1.32323E-05, -0.979114283});
        inv_pf_from_P3_vec.push_back({3000, 10000, first, 0, -0.989303981, 0, 0, 0});
    }
    else if(SE_type == dwc_100kW)
    {
        inv_eff_from_P2_vec.clear();
		inv_eff_from_P2_vec.push_back({-1000, 0, first, (1.1-1)/1000, 1, 0, 0, 0});  
		inv_eff_from_P2_vec.push_back({0, 1.99019902, fourth, -0.033268387, 0.201676303, -0.509381357, 0.767151308, 0.1});  
		inv_eff_from_P2_vec.push_back({1.99019902, 9.9909991, fourth, -8.82395E-05, 0.002740242, -0.032929897, 0.194061675, 0.40120321});
		inv_eff_from_P2_vec.push_back({9.9909991, 24.99249925, fourth, -5.59602E-07, 4.95584E-05, -0.001727389, 0.029395433, 0.741555738}); 
        inv_eff_from_P2_vec.push_back({24.99249925, 100, fourth, -1.61466E-09, 5.06274E-07, -6.06264E-05, 0.003075932, 0.906293666}); 
        inv_eff_from_P2_vec.push_back({100, 1000, first, 0, 0.952430816, 0, 0, 0}); 
        
									// {x_LB, x_UB, degree, a, b, c, d, e}     degree = {first, second, third, fourth}
		inv_pf_from_P3_vec.clear();
		inv_pf_from_P3_vec.push_back({-1000, 0, first, (0.9-1)/1000, 0.90, 0, 0, 0});						    
		inv_pf_from_P3_vec.push_back({0, 	1.99019902, fourth, 2.0653E-05, -0.000205508, 0.00120459, -0.006147932, -0.96});
		inv_pf_from_P3_vec.push_back({1.99019902, 9.9909991, fourth, 1.14451E-06, -3.99927E-05, 0.000587574, -0.004985356, -0.960868555});
		inv_pf_from_P3_vec.push_back({9.9909991, 24.99249925, fourth, 3.09432E-08, -2.86066E-06, 0.000106782, -0.00207529, -0.967886798});
		inv_pf_from_P3_vec.push_back({24.99249925, 100, fourth, 1.72176E-10, -5.46609E-08, 6.69504E-06, -0.000396968, -0.979114283});
        inv_pf_from_P3_vec.push_back({100, 1000, first, 0, -0.989303981, 0, 0, 0});
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

