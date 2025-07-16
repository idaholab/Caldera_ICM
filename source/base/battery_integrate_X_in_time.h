
#ifndef _inl_battery_integrate_X_in_time_H
#define _inl_battery_integrate_X_in_time_H

#include <vector>
#include <iostream>   // Stream to consol (may be needed for files too???)
#include <string>

//#############################################################################
//                    transition_of_X_through_time
//#############################################################################


enum class transition_criteria_type
{
    time_delay_sec = 0,
    delta_X = 1,
    from_final_X = 2
};


enum class transition_status
{
    transition_reached_target_not_nowTime = 0,
    transition_reached_nowTime_not_target = 1,
    transition_interupted_same_direction = 2,
    transition_interupted_opposite_direction = 3,
    transition_crossing_zero_now = 4
};


enum class transition_interruption_state
{
    not_interupted = 0,
    new_transition_in_same_direction = 1,
    new_transition_in_opposite_direction = 2
};


enum class transition_state
{
    off = 0,
    on_steady_state = 1,
    pos_to_off = 2,
    neg_to_off = 3,
    off_to_pos = 4,
    off_to_neg = 5,
    moving_toward_pos_inf = 6,
    moving_toward_neg_inf = 7,
    pos_moving_toward_pos_inf = 8,
    pos_moving_toward_neg_inf = 9,
    neg_moving_toward_pos_inf = 10,
    neg_moving_toward_neg_inf = 11
};
std::ostream& operator << (std::ostream& os, const transition_state& obj);


struct transition_goto_next_segment_criteria
{
    transition_criteria_type criteria_type;
    double criteria_value;
    bool inturupt_this_transition_if_target_X_deviation_limit_exceeded;
    double target_X_deviation_limit_to_interupt_this_transition;
    double segment_slope_X_per_sec;

    transition_goto_next_segment_criteria(const transition_criteria_type& criteria_type,
                                          const double criteria_value,
                                          const bool inturupt_this_transition_if_target_X_deviation_limit_exceeded,
                                          const double target_X_deviation_limit_to_interupt_this_transition,
                                          const double segment_slope_X_per_sec)
        : criteria_type{ criteria_type },
        criteria_value{ criteria_value },
        inturupt_this_transition_if_target_X_deviation_limit_exceeded{ inturupt_this_transition_if_target_X_deviation_limit_exceeded },
        target_X_deviation_limit_to_interupt_this_transition{ target_X_deviation_limit_to_interupt_this_transition },
        segment_slope_X_per_sec{ segment_slope_X_per_sec }
    {
    }
};


struct transition_integral_of_X
{
    double interval_area_Xsec;
    double interval_duration_sec;
    double end_of_interval_unix_time;
    double end_of_interval_X_val;
    transition_status status_val;
};


class transition_of_X_through_time
{
private:
    double start_of_segment_X_val, target_ref_X, X_deadband;
    double start_of_segment_unix_time, end_of_last_interval_unix_time;
    int segment_index;
    bool transition_moving_toward_pos_inf;
    transition_state trans_state;
    
    std::vector<transition_goto_next_segment_criteria> goto_next_segment_criteria;
    
        // Dealing with inturrupting transitions
    int start_segment_index_when_this_transition_interupts_another_transition;
    double target_X_deviation_limit_to_interupt_this_transition;

public:
    transition_of_X_through_time(){}
    transition_of_X_through_time( const transition_state trans_state_,
                                  const double X_deadband_,
                                  const std::vector<transition_goto_next_segment_criteria> &goto_next_segment_criteria_ );

    bool transition_is_moving_toward_pos_inf();
    void init_transition( const double start_of_transition_unix_time,
                          const double X_at_beginning_of_transition_,
                          const double target_ref_X_,
                          const transition_interruption_state trans_interruption_state,
                          const bool transition_just_crossed_zero );
    transition_integral_of_X get_integral( const double target_X,
                                           const double integrate_to_unix_time,
                                           const bool transition_will_cross_zero );
};


//#############################################################################
//                         integrate_X_through_time
//#############################################################################

struct integral_of_X
{
    double avg_X;
    double area_Xsec;
    double time_sec;
};

struct debug_data
{
    transition_state trans_state;
    double end_of_interval_X;
    std::vector<transition_status> trans_status_vec;
};


class integrate_X_through_time
{

private:
    
    double X;
    double target_ref_X;
    double target_deadband;
    double off_deadband;
    
    bool X_has_been_set;
    bool target_set_while_turning_off;
    bool pos_and_neg_transitions_are_unique;
    
    transition_state trans_state;
    
    std::vector<transition_status> debug_trans_status_vec;
    
    transition_of_X_through_time  trans_obj_pos_to_off;
    transition_of_X_through_time  trans_obj_neg_to_off;
    transition_of_X_through_time  trans_obj_off_to_pos;
    transition_of_X_through_time  trans_obj_off_to_neg;
    transition_of_X_through_time  trans_obj_moving_toward_pos_inf;
    transition_of_X_through_time  trans_obj_moving_toward_neg_inf;
    transition_of_X_through_time  trans_obj_pos_moving_toward_pos_inf;
    transition_of_X_through_time  trans_obj_pos_moving_toward_neg_inf;
    transition_of_X_through_time  trans_obj_neg_moving_toward_pos_inf;
    transition_of_X_through_time  trans_obj_neg_moving_toward_neg_inf;
    transition_of_X_through_time *cur_trans_obj;
    
    bool transition_moving_toward_pos_inf( const transition_state trans_state_ );
    
public:
    bool print_debug_info;

    integrate_X_through_time() {};
    integrate_X_through_time( const double target_deadband_,
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
                              const transition_of_X_through_time &trans_obj_neg_moving_toward_neg_inf_
                       );
    integrate_X_through_time( const integrate_X_through_time &obj );
    
    void get_debug_data( debug_data &data );
    
        // This should only be used by battery_factory::get_pev_battery_control_input
    void set_init_state( const double X_ );
    integral_of_X get_next( const double target_X,
                            const double integrate_from_unix_time,
                            const double integrate_to_unix_time );
};


#endif



