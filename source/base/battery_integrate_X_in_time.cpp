
#include "battery_integrate_X_in_time.h"

#include <cmath>        // abs, exp, sqrt
#include <fstream>        // Stream to files
#include <stdexcept>    // invalid_argument
#include <assert.h>


//#############################################################################
//                    transition_of_X_through_time
//#############################################################################

// Input Data Rules
//        1. If segment_slope_X_per_sec == 0 then criteria_type must be transition_criteria_type::time_delay_sec
//        2. If criteria_type in(transition_criteria_type::delta_X, transition_criteria_type::from_final_X) then segment_slope_X_per_sec != 0


std::ostream& operator << (std::ostream& os, const transition_state& trans_state)
{
    std::string val = "";
    
    if(trans_state == transition_state::off)
        val = "off";
    else if(trans_state == transition_state::on_steady_state)
        val = "on_steady_state";
    else if(trans_state == transition_state::pos_to_off)
        val = "pos_to_off";
    else if(trans_state == transition_state::neg_to_off)
        val = "neg_to_off";
    else if(trans_state == transition_state::off_to_pos)
        val = "off_to_pos";
    else if(trans_state == transition_state::off_to_neg)
        val = "off_to_neg";
    else if(trans_state == transition_state::moving_toward_pos_inf)
        val = "moving_toward_pos_inf";
    else if(trans_state == transition_state::moving_toward_neg_inf)
        val = "moving_toward_neg_inf";
    else if(trans_state == transition_state::pos_moving_toward_pos_inf)
        val = "pos_moving_toward_pos_inf";
    else if(trans_state == transition_state::pos_moving_toward_neg_inf)
        val = "pos_moving_toward_neg_inf";
    else if(trans_state == transition_state::neg_moving_toward_pos_inf)
        val = "neg_moving_toward_pos_inf";
    else if(trans_state == transition_state::neg_moving_toward_neg_inf)
        val = "neg_moving_toward_neg_inf";
    else
    {
        std::cout << "Error: This shouldn't happen." << std::endl;
        assert(false);
    }
   
    os << val;
    return os;
};


transition_of_X_through_time::transition_of_X_through_time( const transition_state trans_state_,
                                                            const double X_deadband_,
                                                            const std::vector<transition_goto_next_segment_criteria> &goto_next_segment_criteria_ )
{
    if( trans_state_ == transition_state::off || trans_state_ == transition_state::on_steady_state )
    {
        std::string error_msg = "PROBLEM: transition_of_X_through_time object defined for trans_state off or on_steady_state.  Look in class transition_of_X_through_time.";
        std::cout << error_msg << std::endl;
        throw(std::invalid_argument(error_msg));
    }
    
    if( goto_next_segment_criteria_.size() < 3 )
    {
        std::string error_msg = "PROBLEM: transition_of_X_through_time object has less than three segments.  This is not allowed.";
        std::cout << error_msg << std::endl;
        throw(std::invalid_argument(error_msg));
    }
    
    this->trans_state = trans_state_;
    this->X_deadband = X_deadband_;
    this->goto_next_segment_criteria = goto_next_segment_criteria_;
    
    this->transition_moving_toward_pos_inf = false; // pos_to_off, off_to_neg, moving_toward_neg_inf, pos_moving_toward_neg_inf, neg_moving_toward_neg_inf
    
    if( trans_state_ == transition_state::neg_to_off )
    {
        this->transition_moving_toward_pos_inf = true;
    }
    else if( trans_state_ == transition_state::off_to_pos )
    {
        this->transition_moving_toward_pos_inf = true;
    }
    else if( trans_state_ == transition_state::moving_toward_pos_inf )
    {
        this->transition_moving_toward_pos_inf = true;
    }
    else if( trans_state_ == transition_state::pos_moving_toward_pos_inf )
    {
        this->transition_moving_toward_pos_inf = true;
    }
    else if( trans_state_ == transition_state::neg_moving_toward_pos_inf )
    {
        this->transition_moving_toward_pos_inf = true;
    }
    else
    {
        std::cout << "Error: This shouldn't happen." << std::endl;
        assert(false);
    }
}


bool transition_of_X_through_time::transition_is_moving_toward_pos_inf()
{
    return this->transition_moving_toward_pos_inf;
}


void transition_of_X_through_time::init_transition( const double start_of_transition_unix_time,
                                                    const double X_at_beginning_of_transition_,
                                                    const double target_ref_X_,
                                                    const transition_interruption_state trans_interruption_state,
                                                    const bool transition_just_crossed_zero )
{
    this->start_of_segment_unix_time = start_of_transition_unix_time;
    this->end_of_last_interval_unix_time = start_of_transition_unix_time;
    this->start_of_segment_X_val = X_at_beginning_of_transition_;
    this->target_ref_X = target_ref_X_;
    
    if( transition_just_crossed_zero )
    {
        this->segment_index = (int)this->goto_next_segment_criteria.size() - 1;
    }
    else if(trans_interruption_state == transition_interruption_state::new_transition_in_opposite_direction)
    {
        this->segment_index = 1;
    }
    else if(trans_interruption_state == transition_interruption_state::new_transition_in_same_direction)
    {
        this->segment_index = 2;
    }
    else
    {
        this->segment_index = 0;
    }
}

transition_integral_of_X transition_of_X_through_time::get_integral( const double target_X_original_parameter,
                                                                     const double integrate_to_unix_time,
                                                                     const bool transition_will_cross_zero )
{
    bool will_reach_target_before_now_time;
    bool target_X_deviation_limit_exceeded;
    bool current_X_passed_target_cuz_target_X_moved;
    bool transition_complete_cuz_target_X_moved;
    bool from_final_complete_cuz_target_X_moved;
    bool in_this_segment_will_reach_target;
    bool in_this_segment_will_reach_now_unix_time;
    
    double now_X_val;
    double start_of_interval_X_val;
    double end_of_interval_X_val;
    double end_of_segment_X_val;
    double start_of_interval_unix_time;
    double end_of_interval_unix_time_aux;
    double end_of_segment_unix_time;
    double criteria_value;
    double interval_area_Xsec;
    double interval_duration_sec;
    double segment_slope_;

    // Set the const parameter to a mutable copy.
    double target_X = target_X_original_parameter;
    
    start_of_interval_unix_time = this->end_of_last_interval_unix_time;
    
    segment_slope_ = this->goto_next_segment_criteria[this->segment_index].segment_slope_X_per_sec;
    start_of_interval_X_val = this->start_of_segment_X_val + segment_slope_*(start_of_interval_unix_time - this->start_of_segment_unix_time);
    
    //---------------------------
    //   Transition Interupted
    //---------------------------
        // There are two ways the transition is interruped and both are caused by
        // changes in the target_X value.  Two examples below will demonstrate each
        // case.
        //        Example 1.
        //            - The transition is moving to positive infinity (sudden increase in target_X).
        //            - The target_X increases a lot and on the segment where the increase occured (inturupt_this_transition_if_target_X_deviation_limit_exceeded == True)
        //            - If (inturupt_this_transition_if_target_X_deviation_limit_exceeded == False) then
        //                - Do not interrupt the transition
        //                - Set target_ref_X == target_X
        //        Example 2.
        //            - The transition is moving to positive infinity (sudden decrease in target_X).
        //            - The target_X is less than start_of_interval_X_val
    
        // X_deadband is used to determine when to transition from Steady State to a transition
        // The X_deadband is necessary here because when current_X_passed_target_cuz_target_X_moved = true
        // the code calling this class will need to create a new transition to move toward the target but if 
        // the current value is within the deadband then the next state should be Steady State.

    target_X_deviation_limit_to_interupt_this_transition = this->goto_next_segment_criteria[this->segment_index].target_X_deviation_limit_to_interupt_this_transition;
    
    target_X_deviation_limit_exceeded = this->transition_moving_toward_pos_inf ? target_X >= this->target_ref_X + target_X_deviation_limit_to_interupt_this_transition : target_X <= this->target_ref_X - target_X_deviation_limit_to_interupt_this_transition; 
    current_X_passed_target_cuz_target_X_moved = this->transition_moving_toward_pos_inf ? start_of_interval_X_val >= target_X + this->X_deadband : start_of_interval_X_val <= target_X - this->X_deadband;
    
    if(current_X_passed_target_cuz_target_X_moved || (target_X_deviation_limit_exceeded && this->goto_next_segment_criteria[this->segment_index].inturupt_this_transition_if_target_X_deviation_limit_exceeded))
    {
        interval_area_Xsec = 0;
        interval_duration_sec = 0;
        transition_status tmp_trans_status = current_X_passed_target_cuz_target_X_moved ? transition_status::transition_interupted_opposite_direction : transition_status::transition_interupted_same_direction;
        
        transition_integral_of_X return_val = {interval_area_Xsec, interval_duration_sec, start_of_interval_unix_time, start_of_interval_X_val, tmp_trans_status};
        return return_val;
    }
    
    if(!this->goto_next_segment_criteria[this->segment_index].inturupt_this_transition_if_target_X_deviation_limit_exceeded)
    {
        this->target_ref_X = target_X;
    }
    
    //--------------------------------------------
    //   Transition Complete due to moving Target
    //--------------------------------------------
    transition_complete_cuz_target_X_moved = std::abs(start_of_interval_X_val - target_X) <= this->X_deadband;
    
    if(transition_complete_cuz_target_X_moved)
    {
        interval_area_Xsec = 0;
        interval_duration_sec = 0;
        transition_integral_of_X return_val = {interval_area_Xsec, interval_duration_sec, start_of_interval_unix_time, start_of_interval_X_val, transition_status::transition_reached_target_not_nowTime};
        return return_val;
    }
    
    //--------------------------------------------
    //       Transition Might Cross Zero
    //--------------------------------------------
        
    if(transition_will_cross_zero)
    {
        target_X = 0;
    }
    
    //--------------------------------------------
    //          Calculate Integral
    //--------------------------------------------
    
    transition_criteria_type criteria_type;
    transition_status trans_status_val;
       
    interval_area_Xsec = 0;
    interval_duration_sec = 0;
    
    while(true)
    {
        start_of_interval_unix_time = this->end_of_last_interval_unix_time;
        segment_slope_ = this->goto_next_segment_criteria[this->segment_index].segment_slope_X_per_sec;
        start_of_interval_X_val = this->start_of_segment_X_val + segment_slope_*(start_of_interval_unix_time - this->start_of_segment_unix_time);
        
        //-------------------------------------------------------------
        // Calculate end_of_segment_unix_time and end_of_segment_X_val
        //-------------------------------------------------------------
        
        criteria_type = this->goto_next_segment_criteria[this->segment_index].criteria_type;
        criteria_value = this->goto_next_segment_criteria[this->segment_index].criteria_value;
        from_final_complete_cuz_target_X_moved = false;
        
        if(criteria_type == transition_criteria_type::time_delay_sec)
        {
            end_of_segment_unix_time = this->start_of_segment_unix_time + criteria_value;
            end_of_segment_X_val = this->start_of_segment_X_val + segment_slope_*criteria_value;
        }    
        else if(criteria_type == transition_criteria_type::delta_X)
        {
            end_of_segment_unix_time = this->start_of_segment_unix_time + std::abs(criteria_value/segment_slope_);
            end_of_segment_X_val = this->transition_moving_toward_pos_inf ? this->start_of_segment_X_val + criteria_value : this->start_of_segment_X_val - criteria_value;
        }
        else if(criteria_type == transition_criteria_type::from_final_X)
        {
            // (from_final_complete_cuz_target_X_moved = true)  WILL NEVER HAPPEN DURING THE LAST SEGMENT because either
            // current_X_passed_target_cuz_target_X_moved = true or transition_complete_cuz_target_X_moved = true
            // and execution will never arrive here.
            // This is important because when from_final_complete_cuz_target_X_moved = true the code in 'Loop and process next segment' is
            // always executed.  So if from_final_complete_cuz_target_X_moved = true on the last segment then an index out of bounds / seg fault will occur.
        
            end_of_segment_X_val = this->transition_moving_toward_pos_inf ? target_X - criteria_value : target_X + criteria_value;            
            from_final_complete_cuz_target_X_moved = this->transition_moving_toward_pos_inf ? start_of_interval_X_val > end_of_segment_X_val + 0.000001 : start_of_interval_X_val < end_of_segment_X_val - 0.000001;
            
            if(from_final_complete_cuz_target_X_moved)
            {
                end_of_segment_unix_time = start_of_interval_unix_time;
                end_of_segment_X_val = start_of_interval_X_val;
            }
            else
            {
                end_of_segment_unix_time = this->start_of_segment_unix_time + std::abs((std::abs(this->start_of_segment_X_val - target_X) - criteria_value)/segment_slope_);
            }
        }
        
        //========================================================================        
                                                                                            // 0.000001 is needed because of (criteria_type = transition_criteria_type::from_final_X, criteria_value = 0)
        in_this_segment_will_reach_target = this->transition_moving_toward_pos_inf ? (end_of_segment_X_val > target_X - 0.000001) : (end_of_segment_X_val < target_X + 0.000001);
        in_this_segment_will_reach_now_unix_time = (integrate_to_unix_time < end_of_segment_unix_time + 0.00001);

        if(!from_final_complete_cuz_target_X_moved && (in_this_segment_will_reach_target || in_this_segment_will_reach_now_unix_time))
        {
            //---------------------------------
            //   Will return in this Segment
            //---------------------------------
            
            now_X_val = this->start_of_segment_X_val + segment_slope_*(integrate_to_unix_time - this->start_of_segment_unix_time);
            will_reach_target_before_now_time = this->transition_moving_toward_pos_inf ? target_X <= now_X_val : target_X >= now_X_val;
            
            if(will_reach_target_before_now_time)
            {
                trans_status_val = transition_will_cross_zero ? transition_status::transition_crossing_zero_now : transition_status::transition_reached_target_not_nowTime;
                end_of_interval_X_val = target_X;
                
                if(0.00001 < std::abs(segment_slope_))
                {
                    end_of_interval_unix_time_aux = this->start_of_segment_unix_time + std::abs((this->start_of_segment_X_val - target_X)/segment_slope_);
                }
                else
                {
                    end_of_interval_unix_time_aux =    integrate_to_unix_time < end_of_segment_unix_time ? integrate_to_unix_time : end_of_segment_unix_time;
                }
            }
            else
            {
                trans_status_val = transition_status::transition_reached_nowTime_not_target;
                end_of_interval_X_val = now_X_val;
                end_of_interval_unix_time_aux = integrate_to_unix_time;
            }
            
            interval_duration_sec += end_of_interval_unix_time_aux - start_of_interval_unix_time;
            interval_area_Xsec += 0.5*(start_of_interval_X_val + end_of_interval_X_val) * (end_of_interval_unix_time_aux - start_of_interval_unix_time);
            this->end_of_last_interval_unix_time = end_of_interval_unix_time_aux;
            
            if(std::abs(end_of_segment_unix_time - end_of_interval_unix_time_aux) < 0.00001)
            {
                this->start_of_segment_unix_time = end_of_segment_unix_time;
                this->start_of_segment_X_val = end_of_segment_X_val;
                this->segment_index += 1;
            }
            
            transition_integral_of_X return_val = {interval_area_Xsec, interval_duration_sec, end_of_interval_unix_time_aux, end_of_interval_X_val, trans_status_val};
            return return_val;
        }
        else
        {
            //---------------------------------
            // Loop and process next segment
            //---------------------------------
            
            interval_duration_sec += end_of_segment_unix_time - start_of_interval_unix_time;
            interval_area_Xsec += 0.5*(start_of_interval_X_val + end_of_segment_X_val) * (end_of_segment_unix_time - start_of_interval_unix_time);
            
            this->end_of_last_interval_unix_time = end_of_segment_unix_time;
            this->start_of_segment_unix_time = end_of_segment_unix_time;
            this->start_of_segment_X_val = end_of_segment_X_val;
            this->segment_index += 1;
            
            if(this->goto_next_segment_criteria.size() <= this->segment_index)
            {
                std::string error_msg = "Caldera PEV transitions must be terminated by a segment where (criteria_type = transition_criteria_type::from_final_X, criteria_value = 0).  Look in class transition_of_X_through_time.";
                std::cout << error_msg << std::endl;
                throw(std::invalid_argument(error_msg));
            }
        }
    }
}

//#############################################################################
//                            integrate_X_through_time
//#############################################################################

integrate_X_through_time::integrate_X_through_time( const double target_deadband_,
                                                    const double off_deadband_,
                                                    const bool pos_and_neg_transitions_are_unique_,
                                                    const transition_of_X_through_time &trans_obj_pos_to_off_,
                                                    const transition_of_X_through_time &trans_obj_neg_to_off_,
                                                     const transition_of_X_through_time &trans_obj_off_to_pos_,
                                                    const transition_of_X_through_time &trans_obj_off_to_neg_,
                                                    const transition_of_X_through_time &trans_obj_moving_toward_pos_inf_,
                                                    const transition_of_X_through_time &trans_obj_moving_toward_neg_inf_,
                                                    const transition_of_X_through_time &trans_obj_pos_moving_toward_pos_inf_,
                                                    const transition_of_X_through_time &trans_obj_pos_moving_toward_neg_inf_,
                                                    const transition_of_X_through_time &trans_obj_neg_moving_toward_pos_inf_,
                                                    const transition_of_X_through_time &trans_obj_neg_moving_toward_neg_inf_ )
{
    this->target_deadband = target_deadband_;
    this->off_deadband = off_deadband_;
    this->pos_and_neg_transitions_are_unique = pos_and_neg_transitions_are_unique_;

    this->trans_obj_pos_to_off = trans_obj_pos_to_off_;
    this->trans_obj_neg_to_off = trans_obj_neg_to_off_;
    this->trans_obj_off_to_pos = trans_obj_off_to_pos_;
    this->trans_obj_off_to_neg = trans_obj_off_to_neg_;
    this->trans_obj_moving_toward_pos_inf = trans_obj_moving_toward_pos_inf_;
    this->trans_obj_moving_toward_neg_inf = trans_obj_moving_toward_neg_inf_;
    this->trans_obj_pos_moving_toward_pos_inf = trans_obj_pos_moving_toward_pos_inf_;
    this->trans_obj_pos_moving_toward_neg_inf = trans_obj_pos_moving_toward_neg_inf_;
    this->trans_obj_neg_moving_toward_pos_inf = trans_obj_neg_moving_toward_pos_inf_;
    this->trans_obj_neg_moving_toward_neg_inf = trans_obj_neg_moving_toward_neg_inf_;
    this->cur_trans_obj = NULL;

    this->X = 0;
    this->target_ref_X = 0;        
    this->trans_state = transition_state::off;
    this->X_has_been_set = false;
    this->target_set_while_turning_off = false;
    
    this->print_debug_info = false;
}


integrate_X_through_time::integrate_X_through_time( const integrate_X_through_time &obj )
{    
    this->X = obj.X;
    this->target_ref_X = obj.target_ref_X;   
    this->target_deadband = obj.target_deadband;
    this->off_deadband = obj.off_deadband;
    this->pos_and_neg_transitions_are_unique = obj.pos_and_neg_transitions_are_unique;
    this->trans_state = obj.trans_state;
    this->X_has_been_set = obj.X_has_been_set;
    this->target_set_while_turning_off = obj.target_set_while_turning_off;
    
    //------------------------------------
    
    this->trans_obj_pos_to_off = obj.trans_obj_pos_to_off;
    this->trans_obj_neg_to_off = obj.trans_obj_neg_to_off;
    this->trans_obj_off_to_pos = obj.trans_obj_off_to_pos;
    this->trans_obj_off_to_neg = obj.trans_obj_off_to_neg;
    this->trans_obj_moving_toward_pos_inf = obj.trans_obj_moving_toward_pos_inf;
    this->trans_obj_moving_toward_neg_inf = obj.trans_obj_moving_toward_neg_inf;
    this->trans_obj_pos_moving_toward_pos_inf = obj.trans_obj_pos_moving_toward_pos_inf;
    this->trans_obj_pos_moving_toward_neg_inf = obj.trans_obj_pos_moving_toward_neg_inf;
    this->trans_obj_neg_moving_toward_pos_inf = obj.trans_obj_neg_moving_toward_pos_inf;
    this->trans_obj_neg_moving_toward_neg_inf = obj.trans_obj_neg_moving_toward_neg_inf;
    
    //------------------------------------
    
    this->cur_trans_obj = NULL;
    
    if(this->trans_state == transition_state::pos_to_off)
        this->cur_trans_obj = &this->trans_obj_pos_to_off;
    else if(this->trans_state == transition_state::neg_to_off)
        this->cur_trans_obj = &this->trans_obj_neg_to_off;
    else if(this->trans_state == transition_state::off_to_pos)
        this->cur_trans_obj = &this->trans_obj_off_to_pos;
    else if(this->trans_state == transition_state::off_to_neg)
        this->cur_trans_obj = &this->trans_obj_off_to_neg;        
    else if(this->trans_state == transition_state::moving_toward_pos_inf)
        this->cur_trans_obj = &this->trans_obj_moving_toward_pos_inf;
    else if(this->trans_state == transition_state::moving_toward_neg_inf)
        this->cur_trans_obj = &this->trans_obj_moving_toward_neg_inf;        
    else if(this->trans_state == transition_state::pos_moving_toward_pos_inf)
        this->cur_trans_obj = &this->trans_obj_pos_moving_toward_pos_inf;
    else if(this->trans_state == transition_state::pos_moving_toward_neg_inf)
        this->cur_trans_obj = &this->trans_obj_pos_moving_toward_neg_inf;
    else if(this->trans_state == transition_state::neg_moving_toward_pos_inf)
        this->cur_trans_obj = &this->trans_obj_neg_moving_toward_pos_inf;
    else if(this->trans_state == transition_state::neg_moving_toward_neg_inf)
        this->cur_trans_obj = &this->trans_obj_neg_moving_toward_neg_inf;
    else
    {
        std::cout << "Error: This shouldn't happen." << std::endl;
        assert(false);
    }
}


bool integrate_X_through_time::transition_moving_toward_pos_inf( const transition_state trans_state_ )
{
    bool return_val = false;

    if(trans_state_ == transition_state::pos_to_off)
        return_val = this->trans_obj_pos_to_off.transition_is_moving_toward_pos_inf();
    else if(trans_state_ == transition_state::neg_to_off)
        return_val = this->trans_obj_neg_to_off.transition_is_moving_toward_pos_inf();
    else if(trans_state_ == transition_state::off_to_pos)
        return_val = this->trans_obj_off_to_pos.transition_is_moving_toward_pos_inf();
    else if(trans_state_ == transition_state::off_to_neg)
        return_val = this->trans_obj_off_to_neg.transition_is_moving_toward_pos_inf();        
    else if(trans_state_ == transition_state::moving_toward_pos_inf)
        return_val = this->trans_obj_moving_toward_pos_inf.transition_is_moving_toward_pos_inf();
    else if(trans_state_ == transition_state::moving_toward_neg_inf)
        return_val = this->trans_obj_moving_toward_neg_inf.transition_is_moving_toward_pos_inf();        
    else if(trans_state_ == transition_state::pos_moving_toward_pos_inf)
        return_val = this->trans_obj_pos_moving_toward_pos_inf.transition_is_moving_toward_pos_inf();
    else if(trans_state_ == transition_state::pos_moving_toward_neg_inf)
        return_val = this->trans_obj_pos_moving_toward_neg_inf.transition_is_moving_toward_pos_inf();
    else if(trans_state_ == transition_state::neg_moving_toward_pos_inf)
        return_val = this->trans_obj_neg_moving_toward_pos_inf.transition_is_moving_toward_pos_inf();
    else if(trans_state_ == transition_state::neg_moving_toward_neg_inf)
        return_val = this->trans_obj_neg_moving_toward_neg_inf.transition_is_moving_toward_pos_inf();
    else
    {
        std::cout << "Error: This shouldn't happen." << std::endl;
        assert(false);
    }
        
    return return_val;
}


void integrate_X_through_time::get_debug_data( debug_data &data )
{
    data.trans_state = this->trans_state;
    data.end_of_interval_X = this->X;
    data.trans_status_vec = this->debug_trans_status_vec;
}


    // This should only be used by battery_factory::get_pev_battery_control_input
void integrate_X_through_time::set_init_state(double X_)
{
    if(!this->X_has_been_set && std::abs(X_) > this->off_deadband)
    {
        this->X_has_been_set = true;
        this->target_set_while_turning_off = false;
        this->X = X_;
        this->target_ref_X = X_;        
        this->trans_state = transition_state::on_steady_state;
    }
}


integral_of_X integrate_X_through_time::get_next( const double target_X_original_parameter,
                                                  const double integrate_from_unix_time,
                                                  const double integrate_to_unix_time )
{
    // Allowed Transitions
    //     off,
    //     on_steady_state,
    //     pos_to_off,
    //     neg_to_off,
    //     off_to_pos,
    //     off_to_neg,
    //     moving_toward_pos_inf,
    //     moving_toward_neg_inf,
    //     pos_moving_toward_pos_inf,
    //     pos_moving_toward_neg_inf,
    //     neg_moving_toward_pos_inf,
    //     neg_moving_toward_neg_inf
    //
    // Target_X is always assumed to change at integrate_from_unix_time
    
    // If (this->pos_and_neg_transitions_are_unique == True)
    //         Due too zero crossings there are limitations to the following transitions:
    //            pos_moving_toward_neg_inf
    //            neg_moving_toward_pos_inf
    //
    //         The limitations are as follows:
    //            1. The criteria value transition_criteria_type::from_final_X can only be used on the last segment
    //            2. As always the criteria_value must be 0 on the last segment
    //            3. The abs(slope) of the segments must be increasing.
    //                - The last segment should have the largest abs(slope)
    // 
    //         Reason for these limitations:
    //             - If the limitations are not followed then it is possible for a transition that crosses zero to 
    //              'slow down' or flatten out significantly as the transition approaches zero and then have a large
    //              ramp rate once it is past zero.  This is not a realistic behavior.
    //
    // Else If(this->pos_and_neg_transitions_are_unique == False)
    //        - Limitations mentioned above do not apply.
    //        - Charging and Discharging will have the same slew rates
    //        - Can have unique slew rates for the following:
    //            - moving_toward_pos_inf
    //            - moving_toward_neg_inf
    
    // Set the const parameter to a mutable copy.
    double target_X = target_X_original_parameter;

    //-------------------------------------------------
    //           Update target_has_changed
    //-------------------------------------------------
    
    bool target_has_changed = false;
    //if(this->target_deadband < std::abs(target_X - this->target_ref_X))
    if(this->target_deadband < std::abs(target_X - this->target_ref_X) || std::abs(this->target_ref_X) <= this->target_deadband)
    {
        target_has_changed = true;
        this->target_ref_X = target_X;
    }
    
    if(this->trans_state == transition_state::off && this->target_set_while_turning_off)
    {
        this->target_set_while_turning_off = false;
        target_has_changed = true;
    }
    
    // if(this->print_debug_info)
    // {
    //     std::cout << "target_deadband: " << this->target_deadband << "  target_ref_X: " << this->target_ref_X << "  target_X: " << target_X << std::endl;
    // }
    
    //-------------------------------------------------------------
    //  Update  transition_state_has_changed  and  new_trans_state
    //-------------------------------------------------------------
    
    bool bool_moving_toward_pos_inf = (this->X < target_X);
    
    transition_state new_trans_state;
    bool transition_state_has_changed = false;
    
    if(this->trans_state == transition_state::pos_to_off || this->trans_state == transition_state::neg_to_off)
    {
        if(this->off_deadband < std::abs(target_X))
            this->target_set_while_turning_off = true;
        else
            this->target_set_while_turning_off = false;
        
        target_X = 0;
    }
    else if(target_has_changed)
    {
        if(this->trans_state == transition_state::off)
        {
            if(this->off_deadband < target_X)
            {
                transition_state_has_changed = true;
                new_trans_state = transition_state::off_to_pos;
            }
            else if(target_X < -this->off_deadband)
            {
                transition_state_has_changed = true;
                new_trans_state = transition_state::off_to_neg;
            }
        }
        else if(std::abs(target_X) < this->off_deadband)
        {
            target_X = 0;
            transition_state_has_changed = true;
            
            if(0 <= this->X)
                new_trans_state = transition_state::pos_to_off;
            else
                new_trans_state = transition_state::neg_to_off;
        }
        else
        {
            if(this->trans_state == transition_state::on_steady_state)
            {
                transition_state_has_changed = true;
                
                if(0 <= this->X)
                {
                    if(bool_moving_toward_pos_inf)
                        new_trans_state = this->pos_and_neg_transitions_are_unique ? transition_state::pos_moving_toward_pos_inf : transition_state::moving_toward_pos_inf;
                    else
                        new_trans_state = this->pos_and_neg_transitions_are_unique ? transition_state::pos_moving_toward_neg_inf : transition_state::moving_toward_neg_inf;
                }
                else
                {
                    if(bool_moving_toward_pos_inf)
                        new_trans_state = this->pos_and_neg_transitions_are_unique ? transition_state::neg_moving_toward_pos_inf : transition_state::moving_toward_pos_inf;
                    else
                        new_trans_state = this->pos_and_neg_transitions_are_unique ? transition_state::neg_moving_toward_neg_inf : transition_state::moving_toward_neg_inf;
                }
            }
            else
            {
                if(this->trans_state == 
                    transition_state::off_to_pos)
                {
                    if(!bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = this->pos_and_neg_transitions_are_unique ? transition_state::pos_moving_toward_neg_inf : transition_state::moving_toward_neg_inf;
                    }
                }
                else if(this->trans_state == transition_state::off_to_neg)
                {
                    if(bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = this->pos_and_neg_transitions_are_unique ? transition_state::neg_moving_toward_pos_inf : transition_state::moving_toward_pos_inf;
                    }
                }
                else if(this->trans_state == transition_state::moving_toward_neg_inf)
                {
                    if(bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = transition_state::moving_toward_pos_inf;
                    }
                }
                else if(this->trans_state == transition_state::moving_toward_pos_inf)
                {
                    if(!bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = transition_state::moving_toward_neg_inf;
                    }
                }
                else if(this->trans_state == transition_state::pos_moving_toward_neg_inf)
                {
                    if(bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = transition_state::pos_moving_toward_pos_inf;
                    }
                }
                else if(this->trans_state == transition_state::neg_moving_toward_pos_inf)
                {
                    if(!bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = transition_state::neg_moving_toward_neg_inf;
                    }
                }
                else if(this->trans_state == transition_state::pos_moving_toward_pos_inf)
                {
                    if(!bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = transition_state::pos_moving_toward_neg_inf;
                    }
                }
                else if(this->trans_state == transition_state::neg_moving_toward_neg_inf)
                {
                    if(bool_moving_toward_pos_inf)
                    {
                        transition_state_has_changed = true;
                        new_trans_state = transition_state::neg_moving_toward_pos_inf;
                    }
                }
            }
        }
    }
    
    //-------------------------------------------------------------
    //              Perform Integral Calculation
    //-------------------------------------------------------------
    
    transition_interruption_state trans_interruption_state;
    
    if(transition_state_has_changed && !(this->trans_state == transition_state::off || this->trans_state == transition_state::on_steady_state))
    {    
        bool previous_trans_moving_toward_pos_inf = this->transition_moving_toward_pos_inf(this->trans_state);
        bool current_trans_moving_toward_pos_inf = this->transition_moving_toward_pos_inf(new_trans_state);
    
        if(previous_trans_moving_toward_pos_inf != current_trans_moving_toward_pos_inf)
            trans_interruption_state = transition_interruption_state::new_transition_in_opposite_direction;
        else
            trans_interruption_state = transition_interruption_state::new_transition_in_same_direction;
    }
    else
        trans_interruption_state = transition_interruption_state::not_interupted;
    
    //---------------------

    if(transition_state_has_changed)
        this->trans_state = new_trans_state;
    
    //---------------------
    
    double t0_unix_time, X0, area_Xsec, duration_sec;
    bool transition_will_cross_zero, transition_just_crossed_zero, exit_loop, transition_to_onSteadyState_or_off;
    transition_integral_of_X integral;
    
    t0_unix_time = integrate_from_unix_time;
    X0 = this->X;
    area_Xsec = 0;
    duration_sec = 0;
    
    this->debug_trans_status_vec.clear();

    while(true)
    {
        //-------------------------------
        //   Has Transition Crossed Zero
        //-------------------------------
        
        transition_will_cross_zero = false;
        transition_just_crossed_zero = false;
        
        if(this->pos_and_neg_transitions_are_unique)
        {
            if(this->trans_state == transition_state::pos_moving_toward_neg_inf)
            {
                if(X0 < this->off_deadband)
                {
                    transition_state_has_changed = true;                
                    trans_interruption_state = transition_interruption_state::not_interupted;
                    transition_just_crossed_zero = true;
                    this->trans_state = transition_state::neg_moving_toward_neg_inf;
                }
                else
                    transition_will_cross_zero = (target_X < 0);
            }
            else if(this->trans_state == transition_state::neg_moving_toward_pos_inf)
            {
                if(-this->off_deadband < X0)
                {
                    transition_state_has_changed = true;
                    trans_interruption_state = transition_interruption_state::not_interupted;
                    transition_just_crossed_zero = true;
                    this->trans_state = transition_state::pos_moving_toward_pos_inf;
                }
                else
                    transition_will_cross_zero = (target_X > 0);
            }
        }
        
        //-------------------------------
        //     Update cur_trans_obj
        //-------------------------------
        
        if(transition_state_has_changed)
        {
            if(this->trans_state == transition_state::off || this->trans_state == transition_state::on_steady_state)
                cur_trans_obj = NULL;
            else
            {
                if(this->trans_state == transition_state::pos_to_off)
                    cur_trans_obj = &this->trans_obj_pos_to_off;
                else if(this->trans_state == transition_state::neg_to_off)
                    cur_trans_obj = &this->trans_obj_neg_to_off;
                else if(this->trans_state == transition_state::off_to_pos)
                    cur_trans_obj = &this->trans_obj_off_to_pos;
                else if(this->trans_state == transition_state::off_to_neg)
                    cur_trans_obj = &this->trans_obj_off_to_neg;
                else if(this->trans_state == transition_state::moving_toward_pos_inf)
                    this->cur_trans_obj = &this->trans_obj_moving_toward_pos_inf;
                else if(this->trans_state == transition_state::moving_toward_neg_inf)
                    this->cur_trans_obj = &this->trans_obj_moving_toward_neg_inf;
                else if(this->trans_state == transition_state::pos_moving_toward_pos_inf)
                    cur_trans_obj = &this->trans_obj_pos_moving_toward_pos_inf;
                else if(this->trans_state == transition_state::pos_moving_toward_neg_inf)
                    cur_trans_obj = &this->trans_obj_pos_moving_toward_neg_inf;
                else if(this->trans_state == transition_state::neg_moving_toward_pos_inf)
                    cur_trans_obj = &this->trans_obj_neg_moving_toward_pos_inf;
                else if(this->trans_state == transition_state::neg_moving_toward_neg_inf)
                    cur_trans_obj = &this->trans_obj_neg_moving_toward_neg_inf;
            
                cur_trans_obj->init_transition(t0_unix_time, X0, this->target_ref_X, trans_interruption_state, transition_just_crossed_zero);
                trans_interruption_state = transition_interruption_state::not_interupted;
            }
        }
           
        //-------------------------------
        //    Calculate Integral
        //-------------------------------
        
        transition_state_has_changed = false;
        exit_loop = true;
        
        if(this->trans_state == transition_state::off)
        {            
            duration_sec += integrate_to_unix_time - t0_unix_time;
            //area_Xsec += 0;
            
            t0_unix_time = integrate_to_unix_time;
            X0 = 0;
        }
        else if(this->trans_state == transition_state::on_steady_state)
        {
            duration_sec += integrate_to_unix_time - t0_unix_time;
            area_Xsec += X0*(integrate_to_unix_time - t0_unix_time);
            
            t0_unix_time = integrate_to_unix_time;
            //X0 = X0;
        }
        else
        {
            transition_to_onSteadyState_or_off = false;
            integral = cur_trans_obj->get_integral(target_X, integrate_to_unix_time, transition_will_cross_zero);
        
            duration_sec += integral.interval_duration_sec;
            area_Xsec += integral.interval_area_Xsec;
            this->debug_trans_status_vec.push_back(integral.status_val);
            
            if(integral.status_val == transition_status::transition_reached_nowTime_not_target)
            {
                // Do Nothing
            }
            else if(integral.status_val == transition_status::transition_reached_target_not_nowTime)
            {
                duration_sec += integrate_to_unix_time - integral.end_of_interval_unix_time;
                area_Xsec += integral.end_of_interval_X_val * (integrate_to_unix_time - integral.end_of_interval_unix_time);
                transition_to_onSteadyState_or_off = true;
            }            
            else if(integral.status_val == transition_status::transition_interupted_opposite_direction || integral.status_val == transition_status::transition_interupted_same_direction)
            {
                trans_interruption_state = (integral.status_val == transition_status::transition_interupted_opposite_direction) ? transition_interruption_state::new_transition_in_opposite_direction : transition_interruption_state::new_transition_in_same_direction;
                transition_state_has_changed = true;
                exit_loop = false;
                
                bool_moving_toward_pos_inf = integral.end_of_interval_X_val < target_X;
                
                if(this->pos_and_neg_transitions_are_unique)
                {
                    if(0 < integral.end_of_interval_X_val)
                        this->trans_state = bool_moving_toward_pos_inf ? transition_state::pos_moving_toward_pos_inf : transition_state::pos_moving_toward_neg_inf;
                    else
                        this->trans_state = bool_moving_toward_pos_inf ? transition_state::neg_moving_toward_pos_inf : transition_state::neg_moving_toward_neg_inf;
                }
                else
                    this->trans_state = bool_moving_toward_pos_inf ? transition_state::moving_toward_pos_inf : transition_state::moving_toward_neg_inf;
            }
            else if(integral.status_val == transition_status::transition_crossing_zero_now)
            {
                exit_loop = false;
            }
            
            t0_unix_time = integral.end_of_interval_unix_time;
            X0 = integral.end_of_interval_X_val;
            
            if(transition_to_onSteadyState_or_off)
            {
                cur_trans_obj = NULL;
                
                if(this->trans_state == transition_state::pos_to_off || this->trans_state == transition_state::neg_to_off)                
                    this->trans_state = transition_state::off;
                else
                    this->trans_state = transition_state::on_steady_state;
            }
        }
        
        if(exit_loop)
            break;
    }
    
    this->X = X0;
    
    integral_of_X return_val;
    return_val.avg_X = area_Xsec/duration_sec;
    return_val.area_Xsec = area_Xsec;         
    return_val.time_sec = duration_sec;
    
    return return_val;
}


