
#include "battery.h"

#include <cmath>

//###########################################
//          battery_state
//###########################################

std::string battery_state::get_file_header()
{
	return "soc_t1, P1_kW, P2_kW, time_step_duration_hrs, reached_target_status, E1_energy_to_target_soc_kWh, min_time_to_target_soc_hrs";
}


std::ostream& operator<<(std::ostream& out, battery_state& x)
{
	out << x.soc_t1 << "," << x.P1_kW << "," << x.P2_kW << "," << x.time_step_duration_hrs << "," << x.reached_target_status << "," << x.E1_energy_to_target_soc_kWh << "," << x.min_time_to_target_soc_hrs;
	return out;
}

//###########################################
//          battery
//###########################################

battery::battery( const vehicle_charge_model_inputs& inputs )
	: get_E1_limits_charging{ calculate_E1_energy_limit{ battery_charge_mode::charging, inputs } },
	get_E1_limits_discharging{ calculate_E1_energy_limit{ battery_charge_mode::discharging, inputs } },
	get_next_P2{ inputs.CT_factory.get_charging_transitions(inputs.EV, inputs.EVSE) },
	battery_size_kWh{ inputs.battery_size_kWh },
	soc{ inputs.CE.arrival_SOC },
	soc_to_energy{ this->battery_size_kWh / 100.0 },
	target_P2_kW{ 0.0 },
	zero_slope_threshold_bat_eff_vs_P2{ this->get_zero_slope_threshold_bat_eff_vs_P2(inputs) },
	will_never_discharge{ true },
	soc_of_full_battery{ 100 },
	soc_of_empty_battery{ 0.0 },
	bat_eff_vs_P2_charging{ inputs.PE_factory.get_P2_vs_battery_eff(inputs.EV, battery_charge_mode::charging).curve },
	bat_eff_vs_P2_discharging{ inputs.PE_factory.get_P2_vs_battery_eff(inputs.EV, battery_charge_mode::discharging).curve },
	max_E1_limit{ 0.0 },
	min_E1_limit{ 0.0 },
	print_debug_info{ false }
{
}

double battery::get_zero_slope_threshold_bat_eff_vs_P2( const vehicle_charge_model_inputs& inputs )
{
	double charging_zst = inputs.PE_factory.get_P2_vs_battery_eff(inputs.EV, battery_charge_mode::charging).zero_slope_threshold;
	double discharging_zst = inputs.PE_factory.get_P2_vs_battery_eff(inputs.EV, battery_charge_mode::discharging).zero_slope_threshold;

	return (charging_zst < discharging_zst) ? charging_zst : discharging_zst;
}

void battery::set_soc_of_full_and_empty_battery( const double soc_of_full_battery_, 
												 const double soc_of_empty_battery_ )
{
    this->soc_of_full_battery = soc_of_full_battery_;
    this->soc_of_empty_battery = soc_of_empty_battery_;
}

void battery::set_P2_kW( const double P2_kW ) 
{
	this->get_next_P2.set_init_state(P2_kW);
}

void battery::set_target_P2_kW( const double target_P2_kW_ ) 
{
	this->target_P2_kW = target_P2_kW_;
}

double battery::get_target_P2_kW() 
{
	return this->target_P2_kW;
}


void battery::get_next( const double prev_unix_time, 
					    const double now_unix_time, 
					    const double target_soc, 
					    const double pu_Vrms, 
					    const bool stop_charging_at_target_soc, 
					    battery_state& return_val )
{
	double time_step_sec, time_step_hrs;
	
	time_step_sec = now_unix_time - prev_unix_time;
	time_step_hrs = time_step_sec/3600;
	
	//-----------------------------------------
	//  	Calculate Upper and Lower Limits 
	//        (E1_UB, E1_LB, E2_UB, E2_LB)
	//-----------------------------------------
	
	double E1_kWh_UB, E1_kWh_LB, E2_kWh_UB, E2_kWh_LB, c, d, eff_tmp, x;
	E1_energy_limit E1_limit_UB, E1_limit_LB;
	
	//------------------
	// Get E1_limit_UB
	//------------------
	if(this->soc < this->soc_of_full_battery)
	{
		this->get_E1_limits_charging.get_E1_limit(time_step_sec, this->soc, target_soc, pu_Vrms, E1_limit_UB);
		E1_kWh_UB = (E1_limit_UB.reached_target_status == energy_target_reached_status::can_reach_energy_target_this_timestep && stop_charging_at_target_soc) ? E1_limit_UB.E1_energy_to_target_soc_kWh : E1_limit_UB.max_E1_energy_kWh;
	}
	else
	{
		E1_kWh_UB = 0;					// {target_soc, max_energy_kWh, max_energy_charge_time_hrs, reached_target_status, energy_to_target_soc_kWh, min_time_to_target_soc_hrs}
		E1_limit_UB = {this->soc_of_full_battery, 0, 0, energy_target_reached_status::unknown, 0, -1};
	}
	
	//------------------
	// Get E1_limit_LB
	//------------------
	
	if(this->soc_of_empty_battery < this->soc)
	{
		if(this->will_never_discharge)	// {target_soc, max_energy_kWh, max_energy_charge_time_hrs, reached_target_status, energy_to_target_soc_kWh, min_time_to_target_soc_hrs}
        {
			E1_limit_LB = {100, 0, 0, energy_target_reached_status::can_not_reach_energy_target_this_timestep, 0, -1};
        }
		else
        {
			this->get_E1_limits_discharging.get_E1_limit(time_step_sec, this->soc, target_soc, pu_Vrms, E1_limit_LB);
        }

		E1_kWh_LB = (E1_limit_LB.reached_target_status == energy_target_reached_status::can_reach_energy_target_this_timestep && stop_charging_at_target_soc) ? E1_limit_LB.E1_energy_to_target_soc_kWh : E1_limit_LB.max_E1_energy_kWh;
	}
	else
	{
		E1_kWh_LB = 0;					// {target_soc, max_energy_kWh, max_energy_charge_time_hrs, reached_target_status, energy_to_target_soc_kWh, min_time_to_target_soc_hrs}
		E1_limit_LB = {this->soc_of_empty_battery, 0, 0, energy_target_reached_status::unknown, 0, -1};
	}
	
	//-----------------------
	
	// Reporing to calling routines for debugging
	this->max_E1_limit = E1_kWh_UB; 
    this->min_E1_limit = E1_kWh_LB; 
	
	//---------------------------------------
	// eff_bat(avgP2) = c*avgP2 + d
	// x = c/(t1-t0)
	// E2 = [-d + sqrt(d^2 + 4*x*E1)]/(2*x)  
	//---------------------------------------
	
	//------------------
	// Get E2_limit_UB
	//------------------
	if(E1_kWh_UB == 0)
	{
		E2_kWh_UB = 0;
	}
	else
	{
		c = this->bat_eff_vs_P2_charging.a;
		d = this->bat_eff_vs_P2_charging.b;

		if(std::abs(c) < this->zero_slope_threshold_bat_eff_vs_P2)
		{
			eff_tmp = c*(E1_kWh_UB/time_step_hrs) + d;
			E2_kWh_UB = E1_kWh_UB/eff_tmp;
		}
		else
		{
			x = c / time_step_hrs;
			E2_kWh_UB = (-d + std::sqrt(d*d + 4*x*E1_kWh_UB))/(2*x);		
		}
	}
	
	//------------------
	// Get E2_limit_LB
	//------------------
	if(this->will_never_discharge || E1_kWh_LB == 0)
	{
		E2_kWh_LB = 0;
	}
	else
	{
		c = this->bat_eff_vs_P2_discharging.a;
		d = this->bat_eff_vs_P2_discharging.b;

		if(std::abs(c) < this->zero_slope_threshold_bat_eff_vs_P2)
		{
			eff_tmp = c*(E1_kWh_LB/time_step_hrs) + d;
			E2_kWh_LB = E1_kWh_LB/eff_tmp;
		}
		else
		{
			x = c / time_step_hrs;
			E2_kWh_LB = (-d + std::sqrt(d*d + 4*x*E1_kWh_LB))/(2*x);		
		}
	}

	//------------------------------
	//      Calculate P2_kW
	//------------------------------

	double target_P2, P2_kW, P2_kW_UB, P2_kW_LB;
	integral_of_X P2_struct;
	
	target_P2 = this->target_P2_kW;
	P2_kW_UB = E2_kWh_UB/time_step_hrs;
	P2_kW_LB = E2_kWh_LB/time_step_hrs;
	
	if(target_P2 > P2_kW_UB)
    {
		target_P2 = P2_kW_UB;
    }
	else if(target_P2 < P2_kW_LB)
    {
		target_P2 = P2_kW_LB;
    }
	
	P2_struct = this->get_next_P2.get_next(target_P2, prev_unix_time, now_unix_time);
	P2_kW = P2_struct.avg_X;


    // this->get_next_P2.print_debug_info = false;
    // if(this->print_debug_info)
    // {
    //     this->get_next_P2.print_debug_info = true;
    // 
    //     std::cout << "E1_kWh_LB: " << E1_kWh_LB << "  E2_kWh_LB: " << E2_kWh_LB << "  P2_kW_LB: " << P2_kW_LB << std::endl;
    //     std::cout << "E1_kWh_UB: " << E1_kWh_UB << "  E2_kWh_UB: " << E2_kWh_UB << "  P2_kW_UB: " << P2_kW_UB << std::endl;
    //     std::cout << "target_P2: " << target_P2 << "  P2_kW: " << P2_kW << std::endl;
    // 
    //     debug_data data;
    //     this->get_next_P2.get_debug_data(data);
    //     std::cout << "trans_state: " << data.trans_state << "  end_of_interval_X: " << data.end_of_interval_X << "  transition_status_size: " << data.trans_status_vec.size() << std::endl;
    // }

	//------------------------------
	//      Calculate P1_kW
	//------------------------------
	double P1_kW;
	
	if(P2_kW >= 0)
	{
		c = this->bat_eff_vs_P2_charging.a;
		d = this->bat_eff_vs_P2_charging.b;
	}
	else
	{
		c = this->bat_eff_vs_P2_discharging.a;
		d = this->bat_eff_vs_P2_discharging.b;
	}
	
	P1_kW = (c*P2_kW + d)*P2_kW;

	//------------------------------
	//        Update soc
	//------------------------------
	this->soc += P1_kW*time_step_hrs / this->soc_to_energy;

	if(this->soc < 0)
    {
        this->soc = 0;
    }
    else if(this->soc > 100)
    {
        this->soc = 100;
    }

	//---------------------------------
	//     Update Return Values
	//---------------------------------
	return_val.soc_t1 = this->soc;
	return_val.P1_kW = P1_kW;
	return_val.P2_kW = P2_kW;
	return_val.time_step_duration_hrs = time_step_hrs;
	return_val.reached_target_status = (this->target_P2_kW == 0) ? energy_target_reached_status::target_P2_is_zero : (this->target_P2_kW >= 0) ? E1_limit_UB.reached_target_status : E1_limit_LB.reached_target_status;
	return_val.E1_energy_to_target_soc_kWh = (this->target_P2_kW == 0) ? 0 :(this->target_P2_kW >= 0) ? E1_limit_UB.E1_energy_to_target_soc_kWh : E1_limit_LB.E1_energy_to_target_soc_kWh;
	return_val.min_time_to_target_soc_hrs = (this->target_P2_kW == 0) ? -1 :(this->target_P2_kW >= 0) ? E1_limit_UB.min_time_to_target_soc_hrs : E1_limit_LB.min_time_to_target_soc_hrs;
}