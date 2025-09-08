
#include "supply_equipment_control.h"
#include <cmath>                            // fmod,  (may need to include <math.h>
#include <random>
#include <algorithm>                        // min, max

//===============================================================================================
//===============================================================================================
//                                 ES100 Control Strategy
//===============================================================================================
//===============================================================================================


ES100_control_strategy::ES100_control_strategy(L2_control_strategies_enum L2_CS_enum_, manage_L2_control_strategy_parameters* params_)
{
    this->L2_CS_enum = L2_CS_enum_;
    this->params = params_;
    
    this->cur_P3kW_setpoint = 0;
    this->charge_start_unix_time = 0;
}


void ES100_control_strategy::update_parameters_for_CE(double target_P3kW_, const CE_status& charge_status, const pev_charge_profile& charge_profile)
{
    this->target_P3kW = target_P3kW_;
    this->cur_P3kW_setpoint = 0;
    
    //--------------------------
    //   Get Needed Information
    //--------------------------
    
    double arrival_unix_time = charge_status.arrival_unix_time;
    double departure_unix_time = charge_status.departure_unix_time;

    pev_charge_profile_result Z = charge_profile.find_result_given_startSOC_and_endSOC(this->target_P3kW, charge_status.now_soc, charge_status.departure_SOC);
    double min_time_to_charge_sec = 3600*Z.total_charge_time_hrs;

    //--------------------
   
    std::string randomization_method;
    double beginning_of_TofU_rate_period__time_from_midnight_sec, end_of_TofU_rate_period__time_from_midnight_sec;
    double M1_delay_period_sec;
    double M4_delay_period_sec;
    double w;
    
    if(this->L2_CS_enum == L2_control_strategies_enum::ES100_A)
    {
        const ES100_L2_parameters& X = this->params->get_ES100_A();
        
        randomization_method = X.randomization_method;
        beginning_of_TofU_rate_period__time_from_midnight_sec = 3600*X.beginning_of_TofU_rate_period__time_from_midnight_hrs;
        end_of_TofU_rate_period__time_from_midnight_sec = 3600*X.end_of_TofU_rate_period__time_from_midnight_hrs;
        M1_delay_period_sec = 3600*X.M1_delay_period_hrs;
        M4_delay_period_sec = M1_delay_period_sec; // <----- TODO: For now we are just making the M4 delay the same as the M1 delay.
        w = this->params->ES100A_getUniformRandomNumber_0to1();
        
        if(randomization_method != "M1" && randomization_method != "M2" && randomization_method != "M3")
        {
            std::cout << "CALDERA ERROR A0.  Look in ES100_control_strategy::update_parameters_for_CE." << std::endl;
            return;
        }
    }
    else if(this->L2_CS_enum == L2_control_strategies_enum::ES100_B)
    {
        const ES100_L2_parameters& X = this->params->get_ES100_B();
        
        randomization_method = X.randomization_method;
        beginning_of_TofU_rate_period__time_from_midnight_sec = 3600*X.beginning_of_TofU_rate_period__time_from_midnight_hrs;
        end_of_TofU_rate_period__time_from_midnight_sec = 3600*X.end_of_TofU_rate_period__time_from_midnight_hrs;
        M1_delay_period_sec = 3600*X.M1_delay_period_hrs;
        M4_delay_period_sec = M1_delay_period_sec; // <----- TODO: For now we are just making the M4 delay the same as the M1 delay.
        w = this->params->ES100B_getUniformRandomNumber_0to1();
        
        if(randomization_method != "M1" && randomization_method != "M2" && randomization_method != "M3")
        {
            std::cout << "CALDERA ERROR A1.  Look in ES100_control_strategy::update_parameters_for_CE." << std::endl;
            return;
        }
    }
    else
    {
        std::cout << "CALDERA ERROR A3.  Look in ES100_control_strategy::update_parameters_for_CE." << std::endl;
        return;
    }

    //-----------------------------------------------
    //  Calculate midnight_nearest_arrival_unix_time
    //-----------------------------------------------
    
    double time_ahead_of_midnight_sec = fmod(arrival_unix_time, 24.0*3600.0);
    double midnight_nearest_arrival_unix_time = arrival_unix_time - time_ahead_of_midnight_sec;

    // midnight_nearest_arrival_unix_time moves a day ahead when it crosses 12, doesn't work with intra day TOU period
    //if(time_ahead_of_midnight_sec > 12.0*3600.0)
    //       midnight_nearest_arrival_unix_time += 24.0*3600.0;
    
    double beginning_of_TofU_rate_period_unix_time = midnight_nearest_arrival_unix_time + beginning_of_TofU_rate_period__time_from_midnight_sec;
    double end_of_TofU_rate_period_unix_time = midnight_nearest_arrival_unix_time + end_of_TofU_rate_period__time_from_midnight_sec;
    
    // Fixes TOU rate period when end TOU is smaller than start TOU. i.e intra day TOU. e.g. (8, -6) aka (8, 18)
    if (end_of_TofU_rate_period_unix_time < beginning_of_TofU_rate_period_unix_time)
    {
        end_of_TofU_rate_period_unix_time += 24.0 * 3600;
    }
  
    //-----------------------------------------
    //  Calculate this->charge_start_unix_time
    //-----------------------------------------
    
    const bool TofU_start_in_park = (arrival_unix_time <= beginning_of_TofU_rate_period_unix_time) && (beginning_of_TofU_rate_period_unix_time <= departure_unix_time);
    const bool TofU_end_in_park = (arrival_unix_time <= end_of_TofU_rate_period_unix_time) && (end_of_TofU_rate_period_unix_time <= departure_unix_time);
    const bool park_start_in_TofU = (beginning_of_TofU_rate_period_unix_time <= arrival_unix_time) && (arrival_unix_time <= end_of_TofU_rate_period_unix_time);
    const bool park_in_current_TofU = TofU_start_in_park || TofU_end_in_park || park_start_in_TofU;
    const bool park_in_next_TofU = 3600*24 + beginning_of_TofU_rate_period_unix_time < departure_unix_time;

    if(!park_in_current_TofU && !park_in_next_TofU)
    {
        // This case is for cases where the charge event is completely disjoint
        // from the TofU period (a.k.a. no overlap at all between the charge event and the TofU).
        
        if( randomization_method == "M1" || randomization_method == "M2" )
        {
            this->charge_start_unix_time = arrival_unix_time;
        }
        else
        {
            if( min_time_to_charge_sec > departure_unix_time - arrival_unix_time )
            {
                this->charge_start_unix_time = arrival_unix_time;
            }
            else
            {
                this->charge_start_unix_time = w*arrival_unix_time + (1-w)*(departure_unix_time - min_time_to_charge_sec);
            }
        }
    }
    else
    {
        // This case is for when there's at least some overlap
        // between the charge event and the TofU period.
        
        //------------------------------------------------------------------
        //  Pick Between Current and Next TofU Window (if Park is in both)
        //------------------------------------------------------------------
        
        // If the charge event is only overlapping with the next TofU,
        // then we shift 'beginning_of_TofU_rate_period_unix_time' and 'end_of_TofU_rate_period_unix_time'
        // to refer to the next-day TofU period instead of today's TofU period.
        if(!park_in_current_TofU && park_in_next_TofU)
        {
            beginning_of_TofU_rate_period_unix_time += 24*3600;
            end_of_TofU_rate_period_unix_time += 24*3600;
        }
        
        //---------------------------------------------------------------
        //        Calculate randomly_adjusted_start_TofU_unix_time
        //---------------------------------------------------------------
        
        // We randomly adjsut the start of the TofU period
        // for this specific charge event so that each charge event
        // has a slightly different start of the TofU (so they don't
        // all start at the same time).
        const double randomly_adjusted_start_TofU_unix_time = [&] () {
            double randomly_adjusted_start_TofU_unix_time;
            if(randomization_method == "M1")
            {
                // The start of the TofU is adjusted just a little bit (for the ASAP case)
                randomly_adjusted_start_TofU_unix_time = w*beginning_of_TofU_rate_period_unix_time + (1-w)*(beginning_of_TofU_rate_period_unix_time + M1_delay_period_sec);
            }
            else if (randomization_method == "M2" || randomization_method == "M3")
            {
                // The start of the TofU is adjusted randomly anywhere between the very beginning to as-late-as-possible.
                if( min_time_to_charge_sec > end_of_TofU_rate_period_unix_time - beginning_of_TofU_rate_period_unix_time )
                {
                    randomly_adjusted_start_TofU_unix_time = beginning_of_TofU_rate_period_unix_time;
                }
                else
                {
                    randomly_adjusted_start_TofU_unix_time = w*beginning_of_TofU_rate_period_unix_time + (1-w)*(end_of_TofU_rate_period_unix_time - min_time_to_charge_sec);
                }
            }
            else if( randomization_method == "M4" )
            {
                // The start of TofU is adjusted to as-late-as-possible (with a small buffer).
                const double min_time_to_charge_plus_random_buffer = min_time_to_charge_sec + w*M4_delay_period_sec;
                randomly_adjusted_start_TofU_unix_time = end_of_TofU_rate_period_unix_time - min_time_to_charge_plus_random_buffer;
            }    
            else
            {
                // Throw an error.
                throw std::runtime_error("Error: This else-block shouldn't happen.");
            }
            
            return randomly_adjusted_start_TofU_unix_time;
        }();
        
        
        
        //--------------------------------------------
        //    Ensure Charge will Fit Inside Park
        //--------------------------------------------
        
        if(randomly_adjusted_start_TofU_unix_time < arrival_unix_time)
        {
            // If we arrive after the TofU period started, then just start charging as soon as we arrive.
            this->charge_start_unix_time = arrival_unix_time;
        }
        else if(departure_unix_time - min_time_to_charge_sec < randomly_adjusted_start_TofU_unix_time)
        {
            // If the last-chance-to-start-charging falls before the beginning of the TofU, then just start as late as possible
            // (so we overlap with the TofU as much as possible).
            this->charge_start_unix_time = departure_unix_time - min_time_to_charge_sec;
        }
        else
        {
            // In this case, the charge-event-duration is shorter than the TofU duration, so we just
            // start at the begining of the TofU (as soon as possible, after the randomly-adjusted start of the TofU)
            this->charge_start_unix_time = randomly_adjusted_start_TofU_unix_time;
        }
    }
}


double ES100_control_strategy::get_P3kW_setpoint(double prev_unix_time, double now_unix_time)
{
    if(now_unix_time >= this->charge_start_unix_time)
        this->cur_P3kW_setpoint = this->target_P3kW;
    
    return this->cur_P3kW_setpoint;
}


//===============================================================================================
//===============================================================================================
//                                 ES110 Control Strategy
//===============================================================================================
//===============================================================================================


ES110_control_strategy::ES110_control_strategy(manage_L2_control_strategy_parameters* params_)
{
    this->params = params_;
    this->cur_P3kW_setpoint = 0;
}


void ES110_control_strategy::update_parameters_for_CE(double target_P3kW_, const CE_status& charge_status, const pev_charge_profile& charge_profile)
{    
    this->target_P3kW = target_P3kW_;
    this->cur_P3kW_setpoint = 0;
    
    //--------------------
    
    double arrival_unix_time = charge_status.arrival_unix_time;
    double departure_unix_time = charge_status.departure_unix_time;
    
    pev_charge_profile_result Z = charge_profile.find_result_given_startSOC_and_endSOC(this->target_P3kW, charge_status.now_soc, charge_status.departure_SOC);
    double min_time_to_charge_sec = 3600*Z.total_charge_time_hrs;
    
    double w = this->params->ES110_getUniformRandomNumber_0to1();
    
    if(min_time_to_charge_sec > departure_unix_time - arrival_unix_time)
        this->charge_start_unix_time = arrival_unix_time;
    else
        this->charge_start_unix_time = w*arrival_unix_time + (1-w)*(departure_unix_time - min_time_to_charge_sec);
}


double ES110_control_strategy::get_P3kW_setpoint(double prev_unix_time, double now_unix_time)
{
    if(now_unix_time >= this->charge_start_unix_time)
        this->cur_P3kW_setpoint = this->target_P3kW;
    
    return this->cur_P3kW_setpoint;
}








//===============================================================================================
//===============================================================================================
//                                 ES200 Control Strategy        ["FLAT" strategy]
//===============================================================================================
//===============================================================================================
// Constructor. For the FLAT control strategy,
// there probably doesn't need to be any paramters.
ES200_control_strategy::ES200_control_strategy( manage_L2_control_strategy_parameters* params_ )
{
    this->params = params_;
    this->cur_P3kW_setpoint = 0;
}

// This is called whenever a new CE is loaded.
// 'this->target_P3kW' is set to the P3 value that'll make the profile flat.
void ES200_control_strategy::update_parameters_for_CE( const double max_P3kW,
                                                       const CE_status& charge_status,
                                                       const pev_charge_profile& charge_profile )
{
    // Here, we calculate the target_P3kW for the FLAT control.
    // (1) we compute the time to charge assuming we charged at full-speed.
    // (2) We determine our dwell time
    // (3) We set the power level based on the ratio between dwell_time and min_time_to_charge.
    //           e.g. if min_time_to_charge is 1 hour, and max_P3kW is 10 kW, but our dwell time
    //                is 2 hours, then we charge at  max_P3kW*((1 hr)/(2 hr)) = 10.0*(1/2) = 5 kW.
    
    // Compute the minimum time it takes to charge based on the max power.
    const pev_charge_profile_result Z = charge_profile.find_result_given_startSOC_and_endSOC( max_P3kW, charge_status.arrival_SOC, charge_status.departure_SOC);
    const double min_time_to_charge_sec = 3600 * Z.total_charge_time_hrs;
    
    // Find the dwell time
    const double dwell_time_sec = charge_status.departure_unix_time - charge_status.arrival_unix_time;
    
    // Use ratio between min-time and dwell-time to compute the flat power level.
    const double flat_P3kW = fmin( max_P3kW, max_P3kW*(min_time_to_charge_sec/dwell_time_sec) );
    
    this->target_P3kW = flat_P3kW;
    this->cur_P3kW_setpoint = 0;
    //std::cout << "flat_P3kW: " << flat_P3kW << std::endl;
}

// To Steven: I think no need to change below as we have set the right target_P3kW at the start of the charge event.
double ES200_control_strategy::get_P3kW_setpoint( double prev_unix_time,
                                                  double now_unix_time )
{    
    this->cur_P3kW_setpoint = this->target_P3kW;
    return this->cur_P3kW_setpoint;
}











//===============================================================================================
//===============================================================================================
//                                 ES300 Control Strategy
//===============================================================================================
//===============================================================================================


ES300_control_strategy::ES300_control_strategy(manage_L2_control_strategy_parameters* params_)
{
    this->params = params_;
    this->cur_P3kW_setpoint = 0;
}


void ES300_control_strategy::update_parameters_for_CE(double target_P3kW_)
{
    this->target_P3kW = target_P3kW_;
    this->cur_P3kW_setpoint = 0;
}


double ES300_control_strategy::get_P3kW_setpoint(double prev_unix_time, double now_unix_time)
{    
    this->cur_P3kW_setpoint = this->target_P3kW;
    return this->cur_P3kW_setpoint;
}

//===============================================================================================
//===============================================================================================
//                                 ES400 Control Strategy
//===============================================================================================
//===============================================================================================


ES400_control_strategy::ES400_control_strategy(
    manage_L2_control_strategy_parameters* params_
)
{
    this->params = params_;
    this->cur_P3kW_setpoint = 0;
    this->next_P3kW_setpoint = -1;
    this->updated_P3kW_setpoint_available = false;
    this->unix_time_begining_of_next_agg_step = -1;
}

void ES400_control_strategy::update_parameters_for_CE(
    double target_P3kW_
)
{
    //this->target_P3kW = target_P3kW_;

    //this->cur_P3kW_setpoint = 0;
    //this->next_P3kW_setpoint = -1;
    //this->updated_P3kW_setpoint_available = false;
    //this->unix_time_begining_of_next_agg_step = -1;


    this->target_P3kW = this->cur_P3kW_setpoint;
}


void ES400_control_strategy::get_charging_needs(
    double unix_time_now, 
    double unix_time_begining_of_next_agg_step
)
{
    //this->unix_time_begining_of_next_agg_step = unix_time_begining_of_next_agg_step;

}

double ES400_control_strategy::get_P3kW_setpoint(
    double prev_unix_time, 
    double now_unix_time
)
{
    /*
    if (!this->updated_P3kW_setpoint_available || this->unix_time_begining_of_next_agg_step < 0)
        return this->cur_P3kW_setpoint;

    //--------------------------

    if (this->unix_time_begining_of_next_agg_step <= now_unix_time)
    {
        this->cur_P3kW_setpoint = this->next_P3kW_setpoint;
        this->next_P3kW_setpoint = -1;
        this->updated_P3kW_setpoint_available = false;
        this->unix_time_begining_of_next_agg_step = -1;
    }

    return this->cur_P3kW_setpoint;
    */
    return this->cur_P3kW_setpoint;
}


void ES400_control_strategy::set_power_setpoints(
    double P3kW_setpoint
)
{
    /*
    this->updated_P3kW_setpoint_available = true;
    this->next_P3kW_setpoint = P3kW_setpoint;
    */
    this->cur_P3kW_setpoint = P3kW_setpoint;
}

//===============================================================================================
//===============================================================================================
//                                 ES500 Control Strategy
//===============================================================================================
//===============================================================================================


ES500_control_strategy::ES500_control_strategy(manage_L2_control_strategy_parameters* params_)
{
    this->params = params_;
    
    this->cur_P3kW_setpoint = 0;
    this->next_P3kW_setpoint = -1;
    this->updated_P3kW_setpoint_available = false;
    this->unix_time_begining_of_next_agg_step = -1;
}


void ES500_control_strategy::update_parameters_for_CE(double target_P3kW_)
{
    this->target_P3kW = target_P3kW_;
    
    this->cur_P3kW_setpoint = 0;
    this->next_P3kW_setpoint = -1;
    this->updated_P3kW_setpoint_available = false;
    this->unix_time_begining_of_next_agg_step = -1;
}


double ES500_control_strategy::get_P3kW_setpoint(double prev_unix_time, double now_unix_time)
{
    if(!this->updated_P3kW_setpoint_available || this->unix_time_begining_of_next_agg_step < 0)
        return this->cur_P3kW_setpoint;
    
    //--------------------------
    
    bool is_off_to_on_transition = (std::abs(this->cur_P3kW_setpoint) < 0.00001) && (0.25 < this->next_P3kW_setpoint);
    double lead_time_sec;
    
    if(is_off_to_on_transition)
        lead_time_sec = this->params->ES500_getNormalRandomError_offToOnLeadTime_sec(); 
    else
        lead_time_sec = this->params->ES500_getNormalRandomError_defaultLeadTime_sec();
    
    //--------------------------
    
    if(this->unix_time_begining_of_next_agg_step <= now_unix_time + lead_time_sec)
    {
        this->cur_P3kW_setpoint = this->next_P3kW_setpoint;
        this->next_P3kW_setpoint = -1;
        this->updated_P3kW_setpoint_available = false;
        this->unix_time_begining_of_next_agg_step = -1;
    }
    
    return this->cur_P3kW_setpoint;
}


void ES500_control_strategy::set_energy_setpoints(double e3_setpoint_kWh)
{
    const ES500_L2_parameters& X = this->params->get_ES500();
    double aggregator_timestep_hrs = X.aggregator_timestep_mins/60.0;
    
    this->updated_P3kW_setpoint_available = true;
    this->next_P3kW_setpoint = e3_setpoint_kWh/aggregator_timestep_hrs;
}


void ES500_control_strategy::get_charging_needs(double unix_time_now, double unix_time_begining_of_next_agg_step_, const pev_charge_profile& charge_profile,
                                                const CE_status& charge_status, const charge_event_P3kW_limits& P3kW_limits, const SE_configuration& SE_config,
                                                ES500_aggregator_pev_charge_needs& pev_charge_needs)
{
    this->unix_time_begining_of_next_agg_step = unix_time_begining_of_next_agg_step_;
    
    //========================================
    
    const ES500_L2_parameters& X = this->params->get_ES500();
    double aggregator_timestep_hrs = X.aggregator_timestep_mins/60.0;
    
    double max_P3kW = P3kW_limits.max_P3kW;
    
    pev_charge_needs.SE_id = SE_config.SE_id;
    pev_charge_needs.departure_unix_time = charge_status.departure_unix_time;
    pev_charge_needs.total_park_time_hrs = (charge_status.departure_unix_time - charge_status.arrival_unix_time)/3600.0;
    pev_charge_needs.remaining_park_time_hrs = (charge_status.departure_unix_time - charge_status.now_unix_time)/3600.0;
    pev_charge_needs.e3_step_max_kWh = max_P3kW * aggregator_timestep_hrs;
    pev_charge_needs.e3_step_target_kWh = this->target_P3kW * aggregator_timestep_hrs;
    
    //========================================
    
    pev_charge_profile_result X1, X2;
    
    //-----------------------------------------
    //  min_time_to_complete_entire_charge_hrs
    //-----------------------------------------
    X1 = charge_profile.find_result_given_startSOC_and_endSOC(P3kW_limits.max_P3kW, charge_status.arrival_SOC, charge_status.departure_SOC);
    pev_charge_needs.min_time_to_complete_entire_charge_hrs = X1.total_charge_time_hrs;
    
    //-------------------------------------------------------
    //  e3_charge_remain_kWh & min_remaining_charge_time_hrs
    //-------------------------------------------------------
    
    // charge_status.now_unix_time is the time of the previous time step
    // This should be what is used here since it is concurrent with all the 
    // other charge_status values.  Basically we are estimating the soc increase 
    // from the last time step to the beginning of the next aggregator time step.
    
    double soc_increase_from_now_until_beginning_of_next_agg_time_step = 0;
    double time_from_now_to_beginning_of_next_agg_time_step_hrs = 0;
    
    if(0.001 < this->cur_P3kW_setpoint)
    {
        time_from_now_to_beginning_of_next_agg_time_step_hrs = (unix_time_begining_of_next_agg_step_ - charge_status.now_unix_time)/3600.0;
        X1 = charge_profile.find_result_given_startSOC_and_chargeTime(this->cur_P3kW_setpoint, charge_status.now_soc, time_from_now_to_beginning_of_next_agg_time_step_hrs);
        soc_increase_from_now_until_beginning_of_next_agg_time_step = X1.soc_increase;
    }
    
    //-----------------------------
    
    double startSOC = charge_status.now_soc + soc_increase_from_now_until_beginning_of_next_agg_time_step;
    if(startSOC >= charge_status.departure_SOC)
    {
        pev_charge_needs.e3_charge_remain_kWh = 0;
        pev_charge_needs.min_remaining_charge_time_hrs = 0;
    }
    else
    {
        X2 = charge_profile.find_result_given_startSOC_and_endSOC(P3kW_limits.max_P3kW, startSOC, charge_status.departure_SOC);
        pev_charge_needs.e3_charge_remain_kWh = X2.E3_kWh;
        pev_charge_needs.min_remaining_charge_time_hrs = X2.total_charge_time_hrs;
    }
/*
std::cout << "Get ES500 Charging Needs.  ES500_control_strategy::get_charging_needs()" << std::endl;
std::cout << "agg_time_hrs: " << unix_time_begining_of_next_agg_step/3600.0 << "  charge_event_id: " << charge_status.charge_event_id << "  cur_P3kW_setpoint: " << this->cur_P3kW_setpoint << std::endl;
std::cout << "remaining_park_time_hrs: " << pev_charge_needs.remaining_park_time_hrs << "  min_remaining_charge_time_hrs: " << pev_charge_needs.min_remaining_charge_time_hrs << std::endl;
std::cout << "e3_charge_remain_kWh: " << pev_charge_needs.e3_charge_remain_kWh << "  e3_step_target_kWh: " << pev_charge_needs.e3_step_target_kWh << "  e3_step_max_kWh: " << pev_charge_needs.e3_step_max_kWh << std::endl;
std::cout << "unix_time_now_hrs: " << unix_time_now/3600.0 << "  charge_status.now_unix_time: " << charge_status.now_unix_time/3600.0 << "  unix_time_begining_of_next_agg_step: " << unix_time_begining_of_next_agg_step_/3600.0 << std::endl;
std::cout << "From_now_until_beginning_of_next_agg_time_step ->  soc_increase: " << soc_increase_from_now_until_beginning_of_next_agg_time_step << "  time_mins: " << 60*time_from_now_to_beginning_of_next_agg_time_step_hrs << std::endl << std::endl;
*/
}


//===============================================================================================
//===============================================================================================
//                             VS_get_percX_from_volt_percX_curve
//===============================================================================================
//===============================================================================================

VS_get_percX_from_volt_percX_curve::VS_get_percX_from_volt_percX_curve(std::vector<double>& volt_percX_curve_puV_, std::vector<double>& volt_percX_curve_percX_)
{
    this->volt_percX_curve_puV = volt_percX_curve_puV_;
    this->volt_percX_curve_percX = volt_percX_curve_percX_;
}


double VS_get_percX_from_volt_percX_curve::get_percX(double puV)
{
    int vector_size = this->volt_percX_curve_puV.size();
    
    if(puV <= this->volt_percX_curve_puV[0])
        return this->volt_percX_curve_percX[0];
    
    if(puV >= this->volt_percX_curve_puV[vector_size-1])
        return this->volt_percX_curve_percX[vector_size-1];
    
    //---------------------

    double p, return_val = 0;

    for(int i=1; i<vector_size; i++)
    {
        if(puV <= this->volt_percX_curve_puV[i])
        {
            p = (puV - this->volt_percX_curve_puV[i-1]) / (this->volt_percX_curve_puV[i] - this->volt_percX_curve_puV[i-1]);
            return_val = (1-p)*this->volt_percX_curve_percX[i-1] + p*this->volt_percX_curve_percX[i];
            break;
        }
    }
    
    return return_val;
}


//===============================================================================================
//===============================================================================================
//                                 VS100 Control Strategy
//===============================================================================================
//===============================================================================================


VS100_control_strategy::VS100_control_strategy(manage_L2_control_strategy_parameters* params_)
{
    this->params = params_;
}


void VS100_control_strategy::update_parameters_for_CE(double max_nominal_S3kVA_)
{
    this->max_nominal_S3kVA = max_nominal_S3kVA_;
    this->prev_P3kW_setpoint = 0;
}


double VS100_control_strategy::get_P3kW_setpoint(double prev_unix_time, double now_unix_time, double pu_Vrms, double pu_Vrms_SS, double target_P3kW)
{
    double max_delta_kW_per_min, percP;
    
    const VS100_L2_parameters& X = this->params->get_VS100();
    double puV = (X.voltage_LPF.is_active) ? pu_Vrms_SS : pu_Vrms;
    
    max_delta_kW_per_min = X.max_delta_kW_per_min;
    percP = this->params->VS100_get_percP_from_volt_delta_kW_curve(puV);
    
    //---------------
    
    double delta_P3kW = 0.01 * percP * this->max_nominal_S3kVA;
    double P3kW_setpoint = target_P3kW + delta_P3kW;  // Bounds checking is done in calling function
  
    //---------------
    
    enforce_ramping(prev_unix_time, now_unix_time, max_delta_kW_per_min, this->prev_P3kW_setpoint, P3kW_setpoint);
    this->prev_P3kW_setpoint = P3kW_setpoint;

    return P3kW_setpoint;
}


//===============================================================================================
//===============================================================================================
//                                 VS200 Control Strategy
//===============================================================================================
//===============================================================================================



VS200_control_strategy::VS200_control_strategy(L2_control_strategies_enum L2_CS_enum_, manage_L2_control_strategy_parameters* params_)
{
    this->L2_CS_enum = L2_CS_enum_;
    this->params = params_;
}


void VS200_control_strategy::update_parameters_for_CE(double max_nominal_S3kVA_)
{
    this->max_nominal_S3kVA = max_nominal_S3kVA_;
    this->prev_Q3kVAR_setpoint = 0;
}


double VS200_control_strategy::get_Q3kVAR_setpoint(double prev_unix_time, double now_unix_time, double pu_Vrms, double pu_Vrms_SS, double target_P3kW)
{
    double max_delta_kVAR_per_min, percQ, puV;
    
    if(this->L2_CS_enum == L2_control_strategies_enum::VS200_A)
    {
        const VS200_L2_parameters& X = this->params->get_VS200_A();
        puV = (X.voltage_LPF.is_active) ? pu_Vrms_SS : pu_Vrms;
        
        max_delta_kVAR_per_min = X.max_delta_kVAR_per_min;
        percQ = this->params->VS200A_get_percQ_from_volt_var_curve(puV);
    }
    else if(this->L2_CS_enum == L2_control_strategies_enum::VS200_B)
    {
        const VS200_L2_parameters& X = this->params->get_VS200_B();
        puV = (X.voltage_LPF.is_active) ? pu_Vrms_SS : pu_Vrms;
        
        max_delta_kVAR_per_min = X.max_delta_kVAR_per_min;
        percQ = this->params->VS200B_get_percQ_from_volt_var_curve(puV);
    }
    else if(this->L2_CS_enum == L2_control_strategies_enum::VS200_C)
    {
        const VS200_L2_parameters& X = this->params->get_VS200_C();
        puV = (X.voltage_LPF.is_active) ? pu_Vrms_SS : pu_Vrms;
        
        max_delta_kVAR_per_min = X.max_delta_kVAR_per_min;
        percQ = this->params->VS200C_get_percQ_from_volt_var_curve(puV);
    }
    
    //---------------
    
    double Q3kVAR_setpoint = 0.01 * percQ * this->max_nominal_S3kVA;

    double max_QkVAR_as_percent_of_SkVA = 100;
    double max_abs_Q3kVAR = get_max_abs_Q3kVAR(target_P3kW, this->max_nominal_S3kVA, max_QkVAR_as_percent_of_SkVA);
    
    enforce_limits_on_Q3kVAR(max_abs_Q3kVAR, Q3kVAR_setpoint);
  
    //---------------
    
    enforce_ramping(prev_unix_time, now_unix_time, max_delta_kVAR_per_min, this->prev_Q3kVAR_setpoint, Q3kVAR_setpoint);
    this->prev_Q3kVAR_setpoint = Q3kVAR_setpoint;

    return Q3kVAR_setpoint;
}


//===============================================================================================
//===============================================================================================
//                                 VS300 Control Strategy
//===============================================================================================
//===============================================================================================


VS300_control_strategy::VS300_control_strategy(manage_L2_control_strategy_parameters* params_)
{
    this->params = params_;
}


void VS300_control_strategy::update_parameters_for_CE(double max_nominal_S3kVA_)
{
    this->max_nominal_S3kVA = max_nominal_S3kVA_;
    this->prev_Q3kVAR_setpoint = 0;
}


double VS300_control_strategy::get_Q3kVAR_setpoint(double pu_Vrms, double pu_Vrms_SS, double target_P3kW)
{
    const VS300_L2_parameters& X = this->params->get_VS300();
    double puV = (X.voltage_LPF.is_active) ? pu_Vrms_SS : pu_Vrms;
    
    double max_abs_Q3kVAR = get_max_abs_Q3kVAR(target_P3kW, this->max_nominal_S3kVA, X.max_QkVAR_as_percent_of_SkVA);
    
    double pu_Vrms_ref = 1.0;
    double Q3kVAR_setpoint = this->prev_Q3kVAR_setpoint - X.gamma*(puV - pu_Vrms_ref);
    
    enforce_limits_on_Q3kVAR(max_abs_Q3kVAR, Q3kVAR_setpoint);
    this->prev_Q3kVAR_setpoint = Q3kVAR_setpoint;
    
    return Q3kVAR_setpoint;
}


//===============================================================================================
//===============================================================================================
//                                   Helper Functions
//===============================================================================================
//===============================================================================================

void enforce_limits_on_Q3kVAR(double max_abs_Q3kVAR, double& Q3kVAR_setpoint)
{
    if(Q3kVAR_setpoint > max_abs_Q3kVAR)
        Q3kVAR_setpoint = max_abs_Q3kVAR;
    else if(Q3kVAR_setpoint < -1*max_abs_Q3kVAR)
        Q3kVAR_setpoint = -1*max_abs_Q3kVAR;
}


double get_max_abs_Q3kVAR(double target_P3kW, double max_nominal_S3kVA, double max_QkVAR_as_percent_of_SkVA)
{
    double max_abs_Q3kVAR_1 = 0.01*max_QkVAR_as_percent_of_SkVA*max_nominal_S3kVA;
    double max_abs_Q3kVAR_2 = max_nominal_S3kVA*max_nominal_S3kVA - target_P3kW*target_P3kW;
    max_abs_Q3kVAR_2 = (max_abs_Q3kVAR_2 > 0) ? std::sqrt(max_abs_Q3kVAR_2) : 0;
    
    return std::min({max_abs_Q3kVAR_1, max_abs_Q3kVAR_2});
}


void enforce_ramping(double prev_unix_time, double now_unix_time, double max_delta_per_min, double prev_setpoint, double& setpoint)
{
    double max_delta = max_delta_per_min*(now_unix_time - prev_unix_time)/60.0;
    
    if(max_delta < std::abs(setpoint - prev_setpoint))
    {
        if(setpoint < prev_setpoint)
            setpoint = prev_setpoint - max_delta;
        else
            setpoint = prev_setpoint + max_delta;
    }
}


//===============================================================================================
//===============================================================================================
//                                 Supply Equipment Control
//===============================================================================================
//===============================================================================================

supply_equipment_control::supply_equipment_control( const bool building_charge_profile_library_,
                                                    const SE_configuration& SE_config_,
                                                    const get_base_load_forecast& baseLD_forecaster_,
                                                    manage_L2_control_strategy_parameters* manage_L2_control_ )
    : SE_config{ SE_config_ }, 
    building_charge_profile_library{ building_charge_profile_library_ }, 
    baseLD_forecaster{ baseLD_forecaster_ }

{
    if(this->building_charge_profile_library)
    {
        return;
    }
    
    //----------------
    
    this->manage_L2_control = manage_L2_control_;
    
    ES100_control_strategy ES100A_tmp(L2_control_strategies_enum::ES100_A, this->manage_L2_control);  this->ES100A_obj = ES100A_tmp;
    ES100_control_strategy ES100B_tmp(L2_control_strategies_enum::ES100_B, this->manage_L2_control);  this->ES100B_obj = ES100B_tmp;
    ES110_control_strategy ES110_tmp(this->manage_L2_control);  this->ES110_obj = ES110_tmp; 
    ES200_control_strategy ES200_tmp(this->manage_L2_control);  this->ES200_obj = ES200_tmp; 
    ES300_control_strategy ES300_tmp(this->manage_L2_control);  this->ES300_obj = ES300_tmp; 
    ES500_control_strategy ES500_tmp(this->manage_L2_control);  this->ES500_obj = ES500_tmp;
    
    VS100_control_strategy VS100_tmp(this->manage_L2_control);  this->VS100_obj = VS100_tmp;
    VS200_control_strategy VS200A_tmp(L2_control_strategies_enum::VS200_A, this->manage_L2_control);  this->VS200A_obj = VS200A_tmp;
    VS200_control_strategy VS200B_tmp(L2_control_strategies_enum::VS200_B, this->manage_L2_control);  this->VS200B_obj = VS200B_tmp;
    VS200_control_strategy VS200C_tmp(L2_control_strategies_enum::VS200_C, this->manage_L2_control);  this->VS200C_obj = VS200C_tmp;
    VS300_control_strategy VS300_tmp(this->manage_L2_control);  this->VS300_obj = VS300_tmp;
    
    //--------------
    
    int max_window_size = this->manage_L2_control->get_LPF_max_window_size();
    double initial_raw_data_value = 1.0;
    this->LPF = LPF_kernel{ max_window_size, initial_raw_data_value };
    
    LPF_parameters LPF_params;
    LPF_params.window_size = 1;
    LPF_params.window_type = LPF_window_enum::Rectangular;
    this->LPF.update_LPF(LPF_params);
    
    //--------------
    
    this->target_P3kW = 20;
    this->prev_pu_Vrms = 1.0;
    
     //--------------
     
    this->ensure_pev_charge_needs_met_for_ext_control_strategy = true;
}


control_strategy_enums supply_equipment_control::get_control_strategy_enums()
{
    return this->L2_control_enums;
}


L2_control_strategies_enum supply_equipment_control::get_L2_ES_control_strategy()
{
    return this->L2_control_enums.ES_control_strategy;
}


L2_control_strategies_enum  supply_equipment_control::get_L2_VS_control_strategy()
{
    return this->L2_control_enums.VS_control_strategy;
}


std::string supply_equipment_control::get_external_control_strategy()
{
    return this->L2_control_enums.ext_control_strategy;
}


void supply_equipment_control::set_ensure_pev_charge_needs_met_for_ext_control_strategy( const bool ensure_pev_charge_needs_met )
{
    this->ensure_pev_charge_needs_met_for_ext_control_strategy = ensure_pev_charge_needs_met;
}


void supply_equipment_control::update_parameters_for_CE( supply_equipment_load& SE_load )
{
    if(this->building_charge_profile_library)
        return;
    
    //----------------
    
    LPF_parameters LPF_params;
    LPF_params.window_size = 1;
    LPF_params.window_type = LPF_window_enum::Rectangular;
    
    this->must_charge_for_remainder_of_park = false;
    this->L2_control_enums = SE_load.get_control_strategy_enums();
    
    bool pev_is_connected_to_SE;
    SE_charging_status SE_status_val;
    SE_load.get_current_CE_status(pev_is_connected_to_SE, SE_status_val, this->charge_status);

    //----------------
    
    if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::NA && this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::NA)
    {
        // When creating charge_profile_library the charging must be uncontrolled.
        this->P3kW_limits.min_P3kW = 0;
        this->P3kW_limits.max_P3kW = 1;
        this->target_P3kW = 0;
        
        //------------------------------------------------------------------------------
        //  Uncontrolled charging power level should not be set here:
        //  It is set in the following function when the charge event is first loaded. 
        //      supply_equipment_load::get_next
        //------------------------------------------------------------------------------
        
        // this->target_P3kW = 1000000;
        // SE_load.set_target_acP3_kW(this->target_P3kW);
    }
    else
    {
        const pev_charge_profile& charge_profile = SE_load.get_pev_charge_profile();
        this->P3kW_limits = charge_profile.get_charge_event_P3kW_limits();
        
        double max_P3kW = this->P3kW_limits.max_P3kW;
        this->target_P3kW = max_P3kW;

        //----------------------
        //   Voltage Support
        //----------------------
                
        if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS100)
        {
            const VS100_L2_parameters& X = this->manage_L2_control->get_VS100();            
            this->VS100_obj.update_parameters_for_CE(SE_load.get_PEV_SE_combo_max_nominal_S3kVA());
            this->target_P3kW = max_P3kW * 0.01 * X.target_P3_reference__percent_of_maxP3;
            
            if(X.voltage_LPF.is_active)
                LPF_params = this->manage_L2_control->VS100_get_LPF_parameters();
        }
        else if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS200_A)
        {
            const VS200_L2_parameters& X = this->manage_L2_control->get_VS200_A();
            this->VS200A_obj.update_parameters_for_CE(SE_load.get_PEV_SE_combo_max_nominal_S3kVA());
            this->target_P3kW = max_P3kW * 0.01 * X.target_P3_reference__percent_of_maxP3;
            
            if(X.voltage_LPF.is_active)
                LPF_params = this->manage_L2_control->VS200A_get_LPF_parameters();
        }
        else if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS200_B)
        {
            const VS200_L2_parameters& X = this->manage_L2_control->get_VS200_B();
            this->VS200B_obj.update_parameters_for_CE(SE_load.get_PEV_SE_combo_max_nominal_S3kVA());
            this->target_P3kW = max_P3kW * 0.01 * X.target_P3_reference__percent_of_maxP3;
            
            if(X.voltage_LPF.is_active)
                LPF_params = this->manage_L2_control->VS200B_get_LPF_parameters();
        }
        else if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS200_C)
        {
            const VS200_L2_parameters& X = this->manage_L2_control->get_VS200_C();
            this->VS200C_obj.update_parameters_for_CE(SE_load.get_PEV_SE_combo_max_nominal_S3kVA());
            this->target_P3kW = max_P3kW * 0.01 * X.target_P3_reference__percent_of_maxP3;
            
            if(X.voltage_LPF.is_active)
                LPF_params = this->manage_L2_control->VS200C_get_LPF_parameters();
        }
        else if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS300)
        {
            const VS300_L2_parameters& X = this->manage_L2_control->get_VS300();
            this->VS300_obj.update_parameters_for_CE(SE_load.get_PEV_SE_combo_max_nominal_S3kVA());
            this->target_P3kW = max_P3kW * 0.01 * X.target_P3_reference__percent_of_maxP3;
            
            if(X.voltage_LPF.is_active)
                LPF_params = this->manage_L2_control->VS300_get_LPF_parameters();
        }
        
        //----------------------
        //    Energy Shifting
        //----------------------
        
        if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES100_A)
            this->ES100A_obj.update_parameters_for_CE(this->target_P3kW, this->charge_status, charge_profile);
        
        if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES100_B)
            this->ES100B_obj.update_parameters_for_CE(this->target_P3kW, this->charge_status, charge_profile);
        
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES110)
            this->ES110_obj.update_parameters_for_CE(this->target_P3kW, this->charge_status, charge_profile);
        
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES200)
            this->ES200_obj.update_parameters_for_CE( this->target_P3kW, this->charge_status, charge_profile );
        
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES300)
            this->ES300_obj.update_parameters_for_CE(this->target_P3kW);

        else if (this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES400)
            this->ES400_obj.update_parameters_for_CE(this->target_P3kW);

        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES500)
            this->ES500_obj.update_parameters_for_CE(this->target_P3kW);
    }
    
    this->LPF.update_LPF(LPF_params);
}


void supply_equipment_control::execute_control_strategy( const double prev_unix_time,
                                                         const double now_unix_time,    
                                                         const double pu_Vrms,
                                                         supply_equipment_load& SE_load )
{
    if(this->building_charge_profile_library)
    {
        return;
    }
    
    //----------------
    
    // It is important to call add_raw_data_value this every iteration even if 
    // there is currently not a control strategy using the pu_Vrms_SS value.  
    // Calling this function keeps the raw_puV_data up to date.
    this->LPF.add_raw_data_value(pu_Vrms);
    double pu_Vrms_SS = this->LPF.get_filtered_value();
    this->prev_pu_Vrms = pu_Vrms;
    
    //--------------------------------------
    //  Return if Charging is Uncontrolled
    //--------------------------------------
    if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::NA && this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::NA && this->L2_control_enums.ext_control_strategy == "NA")
    {
        return;
    }
    
    //----------------------------------
    // Get Charge Status
    // Return if PEV is not Connected     
    //----------------------------------
    bool pev_is_connected_to_SE;
    SE_charging_status SE_status_val;
    SE_load.get_current_CE_status(pev_is_connected_to_SE, SE_status_val, this->charge_status);
    
    if( !pev_is_connected_to_SE )
    {
        return;
    }

    //-----------------------------
    //  Calculate Charging Bounds
    //-----------------------------
    
    double min_time_to_complete_charge_hrs;
    SE_load.get_time_to_complete_active_charge_hrs(100000, pev_is_connected_to_SE, min_time_to_complete_charge_hrs);

    double charge_priority = 3600*min_time_to_complete_charge_hrs / (this->charge_status.departure_unix_time - this->charge_status.now_unix_time);
    charge_priority = std::min({charge_priority, 1.0});
    
    double P3kW_LB = charge_priority * this->P3kW_limits.max_P3kW;
    
    if(P3kW_LB < this->P3kW_limits.min_P3kW)
    {
        P3kW_LB = this->P3kW_limits.min_P3kW;
    }
    
    if(this->P3kW_limits.max_P3kW < P3kW_LB)
    {
        P3kW_LB = this->P3kW_limits.max_P3kW;
    }
    
    if(charge_priority > 0.97)
    {
        this->must_charge_for_remainder_of_park = true;
    }
    
    //-----------------------------------------------------------------
    //  Ensure Charge Needs met when using External Control Strategy
    //-----------------------------------------------------------------
    if(this->L2_control_enums.ext_control_strategy != "NA")
    {    
        if(this->must_charge_for_remainder_of_park && this->ensure_pev_charge_needs_met_for_ext_control_strategy)
        {
            SE_load.set_target_acP3_kW(100000);
            SE_load.set_target_acQ3_kVAR(0);
        }
    
        return;
    }
    
    //-----------------------------
    //      Energy Shifting
    //-----------------------------
    double P3kW_setpoint = this->target_P3kW;
    bool is_charging = true;
    
    if(this->must_charge_for_remainder_of_park)
    {
        P3kW_setpoint = this->P3kW_limits.max_P3kW;
    }
    else
    {
        if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES100_A)
        {
            P3kW_setpoint = this->ES100A_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time);
        }
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES100_B)
        {
            P3kW_setpoint = this->ES100B_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time);
        }
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES110)
        {
            P3kW_setpoint = this->ES110_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time);
        }
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES200)
        {
            P3kW_setpoint = this->ES200_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time);
        }
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES300)
        {
            P3kW_setpoint = this->ES300_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time);
        }
        else if (this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES400)
        {
            P3kW_LB = 0.0;
            P3kW_setpoint = this->ES400_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time);
        }
        else if(this->L2_control_enums.ES_control_strategy == L2_control_strategies_enum::ES500)
        {
            P3kW_setpoint = this->ES500_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time);
        }
        else
        {
            throw std::invalid_argument("CALDERA ERROR: We should never get to the else block.");
        }
    
        //---------------------
        
        if( std::abs(P3kW_setpoint) < 0.00001 )
        {
            is_charging = false;
        }
        
        if( is_charging && P3kW_setpoint < P3kW_LB )
        {
            P3kW_setpoint = P3kW_LB;
        }
    }
    
    //------------------------------
    //  Voltage Supporting via PkW 
    //------------------------------
        
    if( this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS100 )
    {
        P3kW_setpoint = this->VS100_obj.get_P3kW_setpoint(prev_unix_time, now_unix_time, pu_Vrms, pu_Vrms_SS, P3kW_setpoint);
    }
    
    // Bind P3kW_setpoint
    if(this->must_charge_for_remainder_of_park)
    {
        P3kW_setpoint = this->P3kW_limits.max_P3kW;
    }
    else if( is_charging && P3kW_setpoint < P3kW_LB )
    {
        P3kW_setpoint = P3kW_LB;
    }
    
    //------------------------------
    // Voltage Supporting via QkVAR 
    //------------------------------
    
    double Q3kVAR_setpoint = 0;
    
    if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS200_A)
    {
        Q3kVAR_setpoint = this->VS200A_obj.get_Q3kVAR_setpoint(prev_unix_time, now_unix_time, pu_Vrms, pu_Vrms_SS, P3kW_setpoint);
    }
    
    else if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS200_B)
    {
        Q3kVAR_setpoint = this->VS200B_obj.get_Q3kVAR_setpoint(prev_unix_time, now_unix_time, pu_Vrms, pu_Vrms_SS, P3kW_setpoint);
    }
    
    else if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS200_C)
    {
        Q3kVAR_setpoint = this->VS200C_obj.get_Q3kVAR_setpoint(prev_unix_time, now_unix_time, pu_Vrms, pu_Vrms_SS, P3kW_setpoint);
    }
    
    else if(this->L2_control_enums.VS_control_strategy == L2_control_strategies_enum::VS300)
    {
        Q3kVAR_setpoint = this->VS300_obj.get_Q3kVAR_setpoint(pu_Vrms, pu_Vrms_SS, P3kW_setpoint);
    }
    
    //---------------------
    //   Apply Setpoints
    //---------------------
    SE_load.set_target_acP3_kW(P3kW_setpoint);
    SE_load.set_target_acQ3_kVAR(Q3kVAR_setpoint);
}


void supply_equipment_control::ES500_get_charging_needs( const double unix_time_now,
                                                         const double unix_time_begining_of_next_agg_step,
                                                         supply_equipment_load& SE_load,
                                                         ES500_aggregator_pev_charge_needs& pev_charge_needs)
{
    if( this->building_charge_profile_library )
    {
        return;
    }
    
    //----------------
    
    // This should be called right after supply_equipment::current_CE_using_control_strategy is called (guaranteeing that there is a current charge)!
    // This is done in the interface object
    
    //-----------------------------------------
    
    //  tmp_charge_status should be calculated as below.  Do not used
    //  this->charge_status.  If this->charge status is used the charge_status.now_unix_time
    //  will be two time steps behind unix_time_now.  By retrieving this->charge_status as
    //  below the charge_status.now_unix_time will only be one time steps behind unix_time_now.
    
    bool pev_is_connected_to_SE;
    SE_charging_status SE_status_val;
    CE_status tmp_charge_status;
    SE_load.get_current_CE_status( pev_is_connected_to_SE, SE_status_val, tmp_charge_status );
    
    const pev_charge_profile& charge_profile = SE_load.get_pev_charge_profile();
    this->ES500_obj.get_charging_needs( unix_time_now,
                                        unix_time_begining_of_next_agg_step,
                                        charge_profile,
                                        tmp_charge_status,
                                        this->P3kW_limits,
                                        this->SE_config,
                                        pev_charge_needs );
}


void supply_equipment_control::ES500_set_energy_setpoints( const double e3_setpoint_kWh )
{
    if( this->building_charge_profile_library )
    {
        return;
    }
    
    //----------------
    
    this->ES500_obj.set_energy_setpoints(e3_setpoint_kWh);
}


void supply_equipment_control::ES400_set_power_setpoints(const double p3_kW)
{
    //----------------

    this->ES400_obj.set_power_setpoints(p3_kW);
}

//===============================================================================================
//===============================================================================================
//                            Manage L2 Control Strategy Parameters
//===============================================================================================
//===============================================================================================

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//              VERY IMPORTANT
//    The function below needs to be updated each
//   time a new LPF is added to a control strategy
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int manage_L2_control_strategy_parameters::get_LPF_max_window_size()
{
    int max_window_size = 0;
    
    if(parameters.VS100.voltage_LPF.is_active)
    {
        max_window_size = std::max({max_window_size, parameters.VS100.voltage_LPF.window_size_UB});
    }
    
    if(parameters.VS200_A.voltage_LPF.is_active)
    {
        max_window_size = std::max({max_window_size, parameters.VS200_A.voltage_LPF.window_size_UB});
    }
    
    if(parameters.VS200_B.voltage_LPF.is_active)
    {
        max_window_size = std::max({max_window_size, parameters.VS200_B.voltage_LPF.window_size_UB});
    }
    
    if(parameters.VS200_C.voltage_LPF.is_active)
    {
        max_window_size = std::max({max_window_size, parameters.VS200_C.voltage_LPF.window_size_UB});
    }
    
    if(parameters.VS300.voltage_LPF.is_active)
    {
        max_window_size = std::max({max_window_size, parameters.VS300.voltage_LPF.window_size_UB});
    }
    
    return max_window_size;
}


manage_L2_control_strategy_parameters::manage_L2_control_strategy_parameters(const L2_control_strategy_parameters& parameters_)
{
    this->parameters = parameters_;
    
    int seed;
    double stdev, stdev_bounds;
    
    //----------------------------------------
    
    ES100_L2_parameters ES100A = this->parameters.ES100_A;
    seed = ES100A.random_seed;
    get_real_value_from_uniform_distribution ES100A_tmp(seed, 0, 1);
    this->ES100A_uniformRandomNumber_0to1 = ES100A_tmp;
    
    //----------------------------------------
    
    ES100_L2_parameters ES100B = this->parameters.ES100_B;
    seed = ES100B.random_seed;
    get_real_value_from_uniform_distribution ES100B_tmp(seed, 0, 1);
    this->ES100B_uniformRandomNumber_0to1 = ES100B_tmp;
      
    //----------------------------------------
    
    ES110_L2_parameters ES110 = this->parameters.ES110;
    seed = ES110.random_seed;
    get_real_value_from_uniform_distribution ES110_tmp(seed, 0, 1);
    this->ES110_uniformRandomNumber_0to1 = ES110_tmp;
    
    //----------------------------------------
    
    ES500_L2_parameters ES500 = this->parameters.ES500;
    
    seed = ES500.off_to_on_lead_time_sec.seed;
    stdev = ES500.off_to_on_lead_time_sec.stdev;
    stdev_bounds = ES500.off_to_on_lead_time_sec.stdev_bounds;
    
    get_value_from_normal_distribution ES500_tmp1(seed, 0, stdev, stdev_bounds);
    this->ES500_normalRandomError_offToOnLeadTime_sec = ES500_tmp1;

    // --------------
    
    seed = ES500.default_lead_time_sec.seed;
    stdev = ES500.default_lead_time_sec.stdev;
    stdev_bounds = ES500.default_lead_time_sec.stdev_bounds;
    
    get_value_from_normal_distribution ES500_tmp2(seed, 0, stdev, stdev_bounds);
    this->ES500_normalRandomError_defaultLeadTime_sec = ES500_tmp2;

    //----------------------------------------
   
    LPF_parameters_randomize_window_size LPF_params;
    
    LPF_params = this->parameters.VS100.voltage_LPF;
    get_int_value_from_uniform_distribution VS100_LPF(LPF_params.seed, LPF_params.window_size_LB, LPF_params.window_size_UB);
    this->VS100_get_LPF_window_size = VS100_LPF;
    
    LPF_params = this->parameters.VS200_A.voltage_LPF;
    get_int_value_from_uniform_distribution VS200A_LPF(LPF_params.seed, LPF_params.window_size_LB, LPF_params.window_size_UB);
    this->VS200A_get_LPF_window_size = VS200A_LPF;
    
    LPF_params = this->parameters.VS200_B.voltage_LPF;
    get_int_value_from_uniform_distribution VS200B_LPF(LPF_params.seed, LPF_params.window_size_LB, LPF_params.window_size_UB);
    this->VS200B_get_LPF_window_size = VS200B_LPF;
    
    LPF_params = this->parameters.VS200_C.voltage_LPF;
    get_int_value_from_uniform_distribution VS200C_LPF(LPF_params.seed, LPF_params.window_size_LB, LPF_params.window_size_UB);
    this->VS200C_get_LPF_window_size = VS200C_LPF;
    
    LPF_params = this->parameters.VS300.voltage_LPF;
    get_int_value_from_uniform_distribution VS300_LPF(LPF_params.seed, LPF_params.window_size_LB, LPF_params.window_size_UB);
    this->VS300_get_LPF_window_size = VS300_LPF;

    //----------------------------------------
 
    VS100_L2_parameters VS100 = this->parameters.VS100;
    VS_get_percX_from_volt_percX_curve VS100_obj(VS100.volt_delta_kW_curve_puV, VS100.volt_delta_kW_percP);
    this->VS100_get_percP = VS100_obj;
    
    //----------------------------------------
    
    VS200_L2_parameters VS200;
    
    VS200 = this->parameters.VS200_A;
    VS_get_percX_from_volt_percX_curve VS200A_obj(VS200.volt_var_curve_puV, VS200.volt_var_curve_percQ);
    this->VS200A_get_percQ = VS200A_obj;
    
    VS200 = this->parameters.VS200_B;
    VS_get_percX_from_volt_percX_curve VS200B_obj(VS200.volt_var_curve_puV, VS200.volt_var_curve_percQ);
    this->VS200B_get_percQ = VS200B_obj;

    VS200 = this->parameters.VS200_C;
    VS_get_percX_from_volt_percX_curve VS200C_obj(VS200.volt_var_curve_puV, VS200.volt_var_curve_percQ);
    this->VS200C_get_percQ = VS200C_obj;
}


const ES100_L2_parameters& manage_L2_control_strategy_parameters::get_ES100_A() { return this->parameters.ES100_A; }
const ES100_L2_parameters& manage_L2_control_strategy_parameters::get_ES100_B() { return this->parameters.ES100_B; }
const ES110_L2_parameters& manage_L2_control_strategy_parameters::get_ES110()   { return this->parameters.ES110; }
const ES200_L2_parameters& manage_L2_control_strategy_parameters::get_ES200()   { return this->parameters.ES200; }
const ES300_L2_parameters& manage_L2_control_strategy_parameters::get_ES300()   { return this->parameters.ES300; }
const ES400_L2_parameters& manage_L2_control_strategy_parameters::get_ES400()   { return this->parameters.ES400; }
const ES500_L2_parameters& manage_L2_control_strategy_parameters::get_ES500()   { return this->parameters.ES500; }

const VS100_L2_parameters& manage_L2_control_strategy_parameters::get_VS100()   { return this->parameters.VS100; }
const VS200_L2_parameters& manage_L2_control_strategy_parameters::get_VS200_A() { return this->parameters.VS200_A; }
const VS200_L2_parameters& manage_L2_control_strategy_parameters::get_VS200_B() { return this->parameters.VS200_B; }
const VS200_L2_parameters& manage_L2_control_strategy_parameters::get_VS200_C() { return this->parameters.VS200_C; }
const VS300_L2_parameters& manage_L2_control_strategy_parameters::get_VS300()   { return this->parameters.VS300; }
   

double manage_L2_control_strategy_parameters::ES100A_getUniformRandomNumber_0to1()
{
    return this->ES100A_uniformRandomNumber_0to1.get_value();
}    


double manage_L2_control_strategy_parameters::ES100B_getUniformRandomNumber_0to1()
{
    return this->ES100B_uniformRandomNumber_0to1.get_value();
}  


double manage_L2_control_strategy_parameters::ES110_getUniformRandomNumber_0to1()
{
    return this->ES110_uniformRandomNumber_0to1.get_value();
}


double manage_L2_control_strategy_parameters::ES500_getNormalRandomError_offToOnLeadTime_sec()
{
    return this->ES500_normalRandomError_offToOnLeadTime_sec.get_value();
}


double manage_L2_control_strategy_parameters::ES500_getNormalRandomError_defaultLeadTime_sec()
{
    return this->ES500_normalRandomError_defaultLeadTime_sec.get_value();
}


double manage_L2_control_strategy_parameters::VS100_get_percP_from_volt_delta_kW_curve(double puV)
{
    return this->VS100_get_percP.get_percX(puV);
}


double manage_L2_control_strategy_parameters::VS200A_get_percQ_from_volt_var_curve(double puV)
{
    return this->VS200A_get_percQ.get_percX(puV);
}


double manage_L2_control_strategy_parameters::VS200B_get_percQ_from_volt_var_curve(double puV)
{
    return this->VS200B_get_percQ.get_percX(puV);
}


double manage_L2_control_strategy_parameters::VS200C_get_percQ_from_volt_var_curve(double puV)
{
    return this->VS200C_get_percQ.get_percX(puV);
}


LPF_parameters manage_L2_control_strategy_parameters::VS100_get_LPF_parameters()
{
    LPF_parameters LPF_params;
    LPF_params.window_size = this->VS100_get_LPF_window_size.get_value();
    LPF_params.window_type = this->parameters.VS100.voltage_LPF.window_type;
    
    return LPF_params;
}


LPF_parameters manage_L2_control_strategy_parameters::VS200A_get_LPF_parameters()
{
    LPF_parameters LPF_params;
    LPF_params.window_size = this->VS200A_get_LPF_window_size.get_value();
    LPF_params.window_type = this->parameters.VS200_A.voltage_LPF.window_type;

    return LPF_params;
}


LPF_parameters manage_L2_control_strategy_parameters::VS200B_get_LPF_parameters()
{
    LPF_parameters LPF_params;
    LPF_params.window_size = this->VS200B_get_LPF_window_size.get_value();
    LPF_params.window_type = this->parameters.VS200_B.voltage_LPF.window_type;
    
    return LPF_params;
}


LPF_parameters manage_L2_control_strategy_parameters::VS200C_get_LPF_parameters()
{
    LPF_parameters LPF_params;
    LPF_params.window_size = this->VS200C_get_LPF_window_size.get_value();
    LPF_params.window_type = this->parameters.VS200_C.voltage_LPF.window_type;
    
    return LPF_params;
}


LPF_parameters manage_L2_control_strategy_parameters::VS300_get_LPF_parameters()
{
    LPF_parameters LPF_params;
    LPF_params.window_size = this->VS300_get_LPF_window_size.get_value();
    LPF_params.window_type = this->parameters.VS300.voltage_LPF.window_type;
    
    return LPF_params;
}
