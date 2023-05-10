#include "charging_transitions_factory.h"

#include "battery_integrate_X_in_time.h"		// integrate_X_through_time, transition_goto_next_segment_criteria, transition_of_X_through_time

//###########################################
//          charging_transitions
//###########################################

charging_transitions::charging_transitions(const EVSE_level_charging_transitions& charging_transitions_by_EVSE_level,
										   const EV_charging_transitions& charging_transitions_by_custom_EV,
										   const EV_EVSE_charging_transitions& charging_transitions_by_custom_EV_EVSE)
	: charging_transitions_by_EVSE_level{ charging_transitions_by_EVSE_level },
	charging_transitions_by_custom_EV{ charging_transitions_by_custom_EV },
	charging_transitions_by_custom_EV_EVSE{ charging_transitions_by_custom_EV_EVSE }
{
}


//###########################################
//          charging_transitions_factory
//###########################################

charging_transitions_factory::charging_transitions_factory(const EV_EVSE_inventory& inventory,
														   const EV_ramping_map& custom_EV_ramping,
														   const EV_EVSE_ramping_map& custom_EV_EVSE_ramping)
    : inventory{ inventory },
    custom_EV_ramping{ custom_EV_ramping },
    custom_EV_EVSE_ramping{ custom_EV_EVSE_ramping },
    charging_transitions_obj{ this->load_charging_transitions() }
{
}


const charging_transitions charging_transitions_factory::load_charging_transitions()
{
    EVSE_level_charging_transitions transitions_by_EVSE_level;

    /*
	Each transition_of_X_through_time must have at least 3 segments.

	struct transition_goto_next_segment_criteria
	{
		transition_criteria_type criteria_type;
		double criteria_value;
		bool inturupt_this_transition_if_target_X_deviation_limit_exceeded;
		double target_X_deviation_limit_to_interupt_this_transition;
		double segment_slope_X_per_sec;
	};
    */

    double X_deadband, target_deadband, off_deadband;
    bool pos_and_neg_transitions_are_unique = false;

    std::vector<transition_goto_next_segment_criteria> goto_next_seg{};
    
    X_deadband = 0.1;
    target_deadband = 0.01;
    off_deadband = 0.0001;

    goto_next_seg.clear();
    goto_next_seg.emplace_back( time_delay_sec, 0.1, false, 0, 0 );
    goto_next_seg.emplace_back( time_delay_sec, 0.1, false, 0, 0 );
    goto_next_seg.emplace_back( from_final_X, 0, false, 0, 10 );
    transition_of_X_through_time default_to_pos_inf{ moving_toward_pos_inf, X_deadband, goto_next_seg };

    goto_next_seg.clear();
    goto_next_seg.emplace_back( time_delay_sec, 0.1, false, 0, 0 );
    goto_next_seg.emplace_back( time_delay_sec, 0.1, false, 0, 0 );
    goto_next_seg.emplace_back( from_final_X, 0, false, 0, -10 );
    transition_of_X_through_time default_to_neg_inf{ moving_toward_neg_inf, X_deadband, goto_next_seg };

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

    transitions_by_EVSE_level.emplace(L1, [&]() {

        X_deadband = 0.01;
        target_deadband = 0.01;
        off_deadband = 0.0001;

        //----------------------
        //     off_to_pos
        //----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 4.95, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.05, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, 0.5 );
        transition_of_X_through_time off_to_pos_obj{ off_to_pos, X_deadband, goto_next_seg };

        //----------------------
        //     pos_to_off
        //----------------------		
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.095, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.005, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, -50 );
        transition_of_X_through_time pos_to_off_obj{ pos_to_off, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_pos_inf
        //-----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.12, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.03, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, 0.5 );
        transition_of_X_through_time moving_toward_pos_inf_obj{ moving_toward_pos_inf, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_neg_inf
        //-----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.09, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.01, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, -0.5 );
        transition_of_X_through_time moving_toward_neg_inf_obj{ moving_toward_neg_inf, X_deadband, goto_next_seg };

        //-----------------------

        return integrate_X_through_time{ target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
            pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
            pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj };
    }());
    
    transitions_by_EVSE_level.emplace(L2, [&]() {

        X_deadband = 0.01;
        target_deadband = 0.01;
        off_deadband = 0.0001;

        //----------------------
        //     off_to_pos
        //----------------------		
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 4.95, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.05, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, 2 );
        transition_of_X_through_time off_to_pos_obj{ off_to_pos, X_deadband, goto_next_seg };

        //----------------------
        //     pos_to_off
        //----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.095, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.005, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, -100 );
        transition_of_X_through_time pos_to_off_obj{ pos_to_off, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_pos_inf
        //-----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.12, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.03, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, 2 );
        transition_of_X_through_time moving_toward_pos_inf_obj{ moving_toward_pos_inf, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_neg_inf
        //-----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.09, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.01, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, -3 );
        transition_of_X_through_time moving_toward_neg_inf_obj{ moving_toward_neg_inf, X_deadband, goto_next_seg };

        //-----------------------

        return integrate_X_through_time{ target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
            pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
            pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj };
    }());

    transitions_by_EVSE_level.emplace(DCFC, [&]() {

        X_deadband = 0.01;
        target_deadband = 0.01;
        off_deadband = 0.0001;
       
        //----------------------
        //     pos_to_off
        //----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.040, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.010, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, -140000 );
        transition_of_X_through_time pos_to_off_obj{ pos_to_off, X_deadband, goto_next_seg };

        //----------------------
        //     off_to_pos
        //----------------------		
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 14.9, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec,  0.1, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, 25 );
        transition_of_X_through_time off_to_pos_obj{ off_to_pos, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_pos_inf
        //-----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.09, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.01, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, 25 );
        transition_of_X_through_time moving_toward_pos_inf_obj{ moving_toward_pos_inf, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_neg_inf
        //-----------------------
        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.09, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.01, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, -25 );
        transition_of_X_through_time moving_toward_neg_inf_obj{ moving_toward_neg_inf, X_deadband, goto_next_seg };

        //--------------------------------

        return integrate_X_through_time{ target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
            pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
            pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj };
    }());

    //--------------------------------------------
    //      transition_by_custom_EV
    //--------------------------------------------

    //helper function
    pev_charge_ramping custom_charge_ramping;

    auto load_custom_ramping = [&]() {
        X_deadband = 0.01;
        target_deadband = 0.01;
        off_deadband = 0.0001;

        double delay_sec, ramping_kW_per_sec;

        //----------------------
        //     pos_to_off
        //----------------------
        delay_sec = custom_charge_ramping.on_to_off_delay_sec;
        ramping_kW_per_sec = custom_charge_ramping.on_to_off_kW_per_sec;

        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.9 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.1 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, ramping_kW_per_sec );
        transition_of_X_through_time pos_to_off_obj{ pos_to_off, X_deadband, goto_next_seg };

        //----------------------
        //     off_to_pos
        //----------------------
        delay_sec = custom_charge_ramping.off_to_on_delay_sec;
        ramping_kW_per_sec = custom_charge_ramping.off_to_on_kW_per_sec;

        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.9 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.1 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, ramping_kW_per_sec );
        transition_of_X_through_time off_to_pos_obj{ off_to_pos, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_pos_inf
        //-----------------------
        delay_sec = custom_charge_ramping.ramp_up_delay_sec;
        ramping_kW_per_sec = custom_charge_ramping.ramp_up_kW_per_sec;

        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.9 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.1 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, ramping_kW_per_sec );
        transition_of_X_through_time moving_toward_pos_inf_obj{ moving_toward_pos_inf, X_deadband, goto_next_seg };

        //-----------------------
        // moving_toward_neg_inf
        //-----------------------
        delay_sec = custom_charge_ramping.ramp_down_delay_sec;
        ramping_kW_per_sec = custom_charge_ramping.ramp_down_kW_per_sec;

        goto_next_seg.clear();
        goto_next_seg.emplace_back( time_delay_sec, 0.9 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( time_delay_sec, 0.1 * delay_sec, false, 0, 0 );
        goto_next_seg.emplace_back( from_final_X, 0, false, 0, ramping_kW_per_sec );
        transition_of_X_through_time moving_toward_neg_inf_obj{ moving_toward_neg_inf, X_deadband, goto_next_seg };

        //-----------------------

        return integrate_X_through_time{ target_deadband, off_deadband, pos_and_neg_transitions_are_unique,
            pos_to_off_obj, neg_to_off_obj, off_to_pos_obj, off_to_neg_obj, moving_toward_pos_inf_obj, moving_toward_neg_inf_obj,
            pos_moving_toward_pos_inf_obj, pos_moving_toward_neg_inf_obj, neg_moving_toward_pos_inf_obj, neg_moving_toward_neg_inf_obj };
    };
    
    EV_charging_transitions charging_transitions_by_custom_EV;
    
    for (const std::pair<EV_type, pev_charge_ramping>& custom_ramping : this->custom_EV_ramping)
    {
        const EV_type& EV = custom_ramping.first;
        custom_charge_ramping = custom_ramping.second;

        charging_transitions_by_custom_EV.emplace(EV, load_custom_ramping());
    }

    //--------------------------------------------
    //      transition_by_custom_EV_EVSE
    //--------------------------------------------

    EV_EVSE_charging_transitions charging_transitions_by_custom_EV_EVSE;
    
    for (const std::pair<std::pair<EV_type, EVSE_type>, pev_charge_ramping>& custom_ramping : this->custom_EV_EVSE_ramping)
    {
        const std::pair<EV_type, EVSE_type>& EV_EVSE = custom_ramping.first;
        custom_charge_ramping = custom_ramping.second;

        charging_transitions_by_custom_EV_EVSE.emplace(EV_EVSE, load_custom_ramping());
    }
    
    return charging_transitions{ transitions_by_EVSE_level, charging_transitions_by_custom_EV, charging_transitions_by_custom_EV_EVSE };
}

const integrate_X_through_time& charging_transitions_factory::get_charging_transitions(const EV_type& EV, 
                                                                                       const EVSE_type& EVSE) const
{
    const EVSE_level& level = this->inventory.get_EVSE_inventory().at(EVSE).get_level();
    
    if (level == L1 || level == L2)
    {
        return this->charging_transitions_obj.charging_transitions_by_EVSE_level.at(level);
    }
    else    // DCFC
    {

        if (this->charging_transitions_obj.charging_transitions_by_custom_EV_EVSE.count(std::make_pair( EV, EVSE )) > 0)
        {
            return this->charging_transitions_obj.charging_transitions_by_custom_EV_EVSE.at(std::make_pair(EV, EVSE));
        }
        else if (this->charging_transitions_obj.charging_transitions_by_custom_EV.count(EV) > 0)
        {
            return this->charging_transitions_obj.charging_transitions_by_custom_EV.at(EV);
        }
        else
        {
            return this->charging_transitions_obj.charging_transitions_by_EVSE_level.at(level);
        }
    }
}
