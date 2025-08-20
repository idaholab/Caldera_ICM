
#include "vehicle_charge_model.h"

#include <cmath>

//###########################################
//          vehicle_charge_model
//###########################################

vehicle_charge_model::vehicle_charge_model(const vehicle_charge_model_inputs& inputs)
    : charge_event{ inputs.CE },
    charge_event_id{ inputs.CE.charge_event_id },
    EV{ inputs.EV },
    arrival_unix_time{ inputs.CE.arrival_unix_time },
    depart_unix_time{ inputs.CE.departure_unix_time },
    arrival_soc{ inputs.CE.arrival_SOC },
    requested_depart_soc{ inputs.CE.departure_SOC },
    decision_metric{ inputs.CE.stop_charge.decision_metric },
    soc_mode{ inputs.CE.stop_charge.soc_mode },
    depart_time_mode{ inputs.CE.stop_charge.depart_time_mode },
    soc_block_charging_max_undershoot_percent{ inputs.CE.stop_charge.soc_block_charging_max_undershoot_percent },
    depart_time_block_charging_max_undershoot_percent{ inputs.CE.stop_charge.depart_time_block_charging_max_undershoot_percent },
    bat{ battery{inputs} },
    soc_of_full_battery{ 99.8 },
    charge_has_completed_{ false },
    charge_needs_met_{ false },
    prev_soc_t1{ this->arrival_soc },
    target_P2_kW{ 0.0 }
{

    double soc_of_empty_battery = 100 - this->soc_of_full_battery;
    this->bat.set_soc_of_full_and_empty_battery(this->soc_of_full_battery, soc_of_empty_battery);
    this->bat.set_target_P2_kW(this->target_P2_kW);

    //-------------------------

    this->stop_charging_at_target_soc = (this->soc_mode == stop_charging_mode::target_charging && this->decision_metric != stop_charging_decision_metric::stop_charging_using_depart_time);
    this->depart_soc = (this->requested_depart_soc < this->soc_of_full_battery) ? this->requested_depart_soc : this->soc_of_full_battery;
}


void vehicle_charge_model::set_target_P2_kW(double target_P2_kW_) 
{ 
    this->target_P2_kW = target_P2_kW_; 
}

double vehicle_charge_model::get_target_P2_kW() const
{
    return this->target_P2_kW;
}

bool vehicle_charge_model::charge_has_completed() const
{
    return this->charge_has_completed_;
}


bool vehicle_charge_model::pev_has_arrived_at_SE(double now_unix_time) const
{
    return (this->arrival_unix_time <= now_unix_time);
}


bool vehicle_charge_model::pev_is_connected_to_SE(double now_unix_time) const
{
    return (this->arrival_unix_time <= now_unix_time && now_unix_time <= this->depart_unix_time);
}


void vehicle_charge_model::get_E1_battery_limits(double& max_E1_limit, double& min_E1_limit) const
{
    max_E1_limit = this->bat.max_E1_limit;
    min_E1_limit = this->bat.min_E1_limit;
}


void vehicle_charge_model::get_next( const double prev_unix_time,
                                     const double now_unix_time,
                                     const double pu_Vrms,
                                     bool& charge_has_completed,
                                     battery_state& bat_state )
{
    if(this->arrival_unix_time <= now_unix_time)
    {
        if(this->charge_needs_met_ || this->soc_of_full_battery <= this->prev_soc_t1)
        {
            this->bat.set_target_P2_kW(0);
        }
        else
        {
            this->bat.set_target_P2_kW(this->target_P2_kW);
        }
    }
    else
    {
        this->bat.set_target_P2_kW(0);
    }
    
    //----------------------------
    
    this->bat.get_next(prev_unix_time, now_unix_time, this->depart_soc, pu_Vrms, this->stop_charging_at_target_soc, bat_state);

    if(this->charge_needs_met_ && std::abs(bat_state.P1_kW) < 0.001)
    {
        this->charge_has_completed_ = true;
        charge_has_completed = true;
    }
    else
    {
        this->charge_has_completed_ = false;
        charge_has_completed = false;
    }

    //-------------------------------

    if(!this->charge_needs_met_)
    {
        bool charge_needs_met_soc = false;
        double soc_t1 = bat_state.soc_t1;

        if(this->soc_mode == stop_charging_mode::target_charging)
        {
            charge_needs_met_soc = (this->depart_soc <= soc_t1);

            if(!charge_needs_met_soc)
            {
                charge_needs_met_soc = (bat_state.reached_target_status == energy_target_reached_status::can_reach_energy_target_this_timestep && (bat_state.E1_energy_to_target_soc_kWh < bat_state.P1_kW * bat_state.time_step_duration_hrs));
            }
        }
        else if(this->soc_mode == stop_charging_mode::block_charging)
        {        
            double boundary_soc = soc_t1 + (soc_t1 - this->prev_soc_t1)*this->soc_block_charging_max_undershoot_percent/100.0;
            charge_needs_met_soc = (this->depart_soc <= boundary_soc);
        }

        //-------------------------------

        bool charge_needs_met_depart_time = false;
        double boundary_time;

        if(this->depart_time_mode == stop_charging_mode::target_charging)
        {
            charge_needs_met_depart_time = (this->depart_unix_time <= now_unix_time);
        }
        else if(this->depart_time_mode == stop_charging_mode::block_charging)
        {        
            boundary_time = now_unix_time + (now_unix_time - prev_unix_time)*this->depart_time_block_charging_max_undershoot_percent/100.0;
            charge_needs_met_depart_time = (this->depart_unix_time <= boundary_time);
        }

        //-------------------------------

        if(this->decision_metric == stop_charging_decision_metric::stop_charging_using_depart_time)
        {
            this->charge_needs_met_ = charge_needs_met_depart_time;
        }
        else
        {
            if(this->soc_of_full_battery <= soc_t1)
            {
                this->charge_needs_met_ = true;
            }
            else if(this->decision_metric == stop_charging_decision_metric::stop_charging_using_target_soc)
            {
                this->charge_needs_met_ = charge_needs_met_soc;
            }
            else if(this->decision_metric == stop_charging_decision_metric::stop_charging_using_whatever_happens_first)
            {
                this->charge_needs_met_ = charge_needs_met_soc || charge_needs_met_depart_time;
            }
            else
            {
                this->charge_needs_met_ = false;
            }
        }
    }

    this->prev_soc_t1 = bat_state.soc_t1;
    
    // this->bat.print_debug_info = false;
    // if(this->charge_event_id == 1214)
    // {
    //   if(99.7 < bat_state.soc_t1 && bat_state.soc_t1 < 99.9)
    //   {
    //     this->bat.print_debug_info = true;
    //     std::cout << "soc_full_bat: " << this->soc_of_full_battery
    //               << "  soc_t1: " << bat_state.soc_t1
    //               << "  charge_needs_met: " << charge_needs_met_
    //               << "  bat_P1kW: " << bat_state.P1_kW
    //               << "  bat_target_P2kW: " << this->bat.get_target_P2_kW()
    //               << std::endl;
    //     std::cout << std::endl;
    //   }
    // }
}
