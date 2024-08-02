
#include "datatypes_global.h"
#include <cmath>
#include <stdexcept>	// invalid_argument

//==================================================================
//                timeseries
//==================================================================


timeseries::timeseries(const double data_starttime_sec, const double data_timestep_sec, const std::vector<double>& data)
    :
    data_starttime_sec{ data_starttime_sec },
    data_timestep_sec{ data_timestep_sec },
    data{ data },
    data_endtime_sec{ this->data_starttime_sec + this->data.size() * this->data_timestep_sec }
{

}


double timeseries::get_val_from_time(double time_sec) const
{
    //std::cout << "timeseries time_sec : " << time_sec << std::endl;
    //std::cout << "timeseries this->data_starttime_sec : " << this->data_starttime_sec << std::endl;
    //std::cout << "timeseries this->data_timestep_sec : " << this->data_timestep_sec << std::endl;

    time_sec = fmod(time_sec, this->data_endtime_sec - this->data_starttime_sec);
    //std::cout << "timeseries time_sec after rounding: " << time_sec << std::endl;

    const int index = int( (time_sec - this->data_starttime_sec) / this->data_timestep_sec );
    //std::cout << "timeseries index : " << index << std::endl;
    return this->data.at(index);
}

double timeseries::get_val_from_time_with_default( const double time_sec, const double out_of_range_default ) const
{
    const int index = int( (time_sec - this->data_starttime_sec) / this->data_timestep_sec );
    if( index >= this->data.size() || index < 0 )
    {
        return out_of_range_default;
    }
    else
    {
        return this->data.at(index);
    }
}

double timeseries::get_val_from_index(int index) const
{
    return this->data.at(index);
}

double timeseries::get_time_from_index_sec(int index) const
{
    return this->data_starttime_sec + index * this->data_timestep_sec;
}

int timeseries::get_index_from_time(double time_sec) const
{
    return int(time_sec - this->data_starttime_sec) / int(this->data_timestep_sec);
}





//==================================================================
//                time_series_v2
//==================================================================

template <typename ARR_DATA_TYPE>
time_series_v2<ARR_DATA_TYPE>::time_series_v2(
                                      const double data_starttime_sec,
                                      const double data_timestep_sec,
                                      const std::vector<ARR_DATA_TYPE>& data )
                                      :
                                      data_starttime_sec{ data_starttime_sec },
                                      data_timestep_sec{ data_timestep_sec },
                                      data{ data },
                                      data_endtime_sec{ this->data_starttime_sec + this->data.size() * this->data_timestep_sec }
{
}
template time_series_v2<int>::time_series_v2( const double data_starttime_sec,
                                                const double data_timestep_sec,
                                                const std::vector<int>& data );
template time_series_v2<float>::time_series_v2( const double data_starttime_sec,
                                                const double data_timestep_sec,
                                                const std::vector<float>& data );
template time_series_v2<double>::time_series_v2( const double data_starttime_sec,
                                                 const double data_timestep_sec,
                                                 const std::vector<double>& data );


template <typename ARR_DATA_TYPE>
ARR_DATA_TYPE time_series_v2<ARR_DATA_TYPE>::get_val_from_time_with_error_check( const double time_sec ) const
{
    const int index = int( (time_sec - this->data_starttime_sec) / this->data_timestep_sec );
    if( index >= this->data.size() || index < 0 )
    {
        std::string error_msg = "Error: 'time_sec' is out of scope of the array. [1]";
		std::cout << error_msg << std::endl;
		throw(std::invalid_argument(error_msg));
    }
    return this->data.at(index);
}
template int time_series_v2<int>::get_val_from_time_with_error_check( const double time_sec ) const;
template float time_series_v2<float>::get_val_from_time_with_error_check( const double time_sec ) const;
template double time_series_v2<double>::get_val_from_time_with_error_check( const double time_sec ) const;


template <typename ARR_DATA_TYPE>
ARR_DATA_TYPE time_series_v2<ARR_DATA_TYPE>::get_val_from_time_with_cycling( const double time_sec ) const
{    
    const int index = int( (time_sec - this->data_starttime_sec) / this->data_timestep_sec );
    if( index >= this->data.size() || index < 0 )
    {
        const double time_sec_cyc = fmod( time_sec, this->data_endtime_sec - this->data_starttime_sec );
        return this->get_val_from_time_with_cycling( time_sec_cyc );
    }
    return this->data.at(index);
}
template int time_series_v2<int>::get_val_from_time_with_cycling( const double time_sec ) const;
template float time_series_v2<float>::get_val_from_time_with_cycling( const double time_sec ) const;
template double time_series_v2<double>::get_val_from_time_with_cycling( const double time_sec ) const;


template <typename ARR_DATA_TYPE>
ARR_DATA_TYPE time_series_v2<ARR_DATA_TYPE>::get_val_from_time_with_default( const double time_sec, const ARR_DATA_TYPE out_of_range_default ) const
{
    const int index = int( (time_sec - this->data_starttime_sec) / this->data_timestep_sec );
    if( index >= this->data.size() || index < 0 )
    {
        return out_of_range_default;
    }
    else
    {
        return this->data.at(index);
    }
}
template int time_series_v2<int>::get_val_from_time_with_default( const double time_sec, const int out_of_range_default ) const;
template float time_series_v2<float>::get_val_from_time_with_default( const double time_sec, const float out_of_range_default ) const;
template double time_series_v2<double>::get_val_from_time_with_default( const double time_sec, const double out_of_range_default ) const;


template <typename ARR_DATA_TYPE>
ARR_DATA_TYPE time_series_v2<ARR_DATA_TYPE>::get_val_from_index( const int index ) const
{
    if( index >= this->data.size() || index < 0 )
    {
        std::string error_msg = "Error: 'time_sec' is out of scope of the array. [2]";
		std::cout << error_msg << std::endl;
		throw(std::invalid_argument(error_msg));
    }
    return this->data.at(index);
}
template int time_series_v2<int>::get_val_from_index( const int index ) const;
template float time_series_v2<float>::get_val_from_index( const int index ) const;
template double time_series_v2<double>::get_val_from_index( const int index ) const;


template <typename ARR_DATA_TYPE>
double time_series_v2<ARR_DATA_TYPE>::get_time_sec_from_index( const int index ) const
{
    return this->data_starttime_sec + index * this->data_timestep_sec;
}
template double time_series_v2<int>::get_time_sec_from_index( const int index ) const;
template double time_series_v2<float>::get_time_sec_from_index( const int index ) const;
template double time_series_v2<double>::get_time_sec_from_index( const int index ) const;


template <typename ARR_DATA_TYPE>
int time_series_v2<ARR_DATA_TYPE>::get_index_from_time( const double time_sec ) const
{
    const int index = int( (time_sec - this->data_starttime_sec) / this->data_timestep_sec );
    return index;
}
template int time_series_v2<int>::get_index_from_time( const double time_sec ) const;
template int time_series_v2<float>::get_index_from_time( const double time_sec ) const;
template int time_series_v2<double>::get_index_from_time( const double time_sec ) const;







//==================================================================
//                Low Pass Filter Parameters
//==================================================================

std::pair<bool, LPF_window_enum> get_LPF_window_enum(const std::string str_val)
{
    bool conversion_successfull = true;
    LPF_window_enum enum_val;
    
	std::string tmp_str = "";
	
	// ignores whitespace
	for (int i = 0; i < str_val.length(); i++)
		if (!std::isspace(str_val[i]))
			tmp_str += str_val[i];

	     if	(tmp_str == "Hanning") 	    enum_val = Hanning;
	else if	(tmp_str == "Blackman") 	enum_val = Blackman;
    else if	(tmp_str == "Rectangular") 	enum_val = Rectangular;
	else
	{
        conversion_successfull = false;
        enum_val = Rectangular;
    }
    
    std::pair<bool, LPF_window_enum> return_val = {conversion_successfull, enum_val};
    
    return return_val;
}


//==================================================================
//                     PEV Charge Profile 
//==================================================================

pev_charge_fragment_variation::pev_charge_fragment_variation(int original_charge_fragment_index_, double time_since_charge_began_hrs_, double soc_, double P3_kW_)
{
    this->original_charge_fragment_index = original_charge_fragment_index_;
    this->time_since_charge_began_hrs = time_since_charge_began_hrs_;
    this->soc = soc_;
    this->P3_kW = P3_kW_;
    this->variation_rank = -1;
}


pev_charge_fragment::pev_charge_fragment(double soc_, double E1_kWh_, double E2_kWh_, double E3_kWh_, double cumQ3_kVARh_, double time_since_charge_began_hrs_)
{
    this->soc = soc_;
    this->E1_kWh = E1_kWh_;
    this->E2_kWh = E2_kWh_;
    this->E3_kWh = E3_kWh_;
    this->cumQ3_kVARh = cumQ3_kVARh_;
    this->time_since_charge_began_hrs = time_since_charge_began_hrs_;
} 


//==================================================================
//               L2_control_strategy_parameters
//==================================================================


bool L2_control_strategy_supports_Vrms_using_QkVAR(L2_control_strategies_enum  control_strategy)
{
    bool return_val = false;
    
    std::vector<L2_control_strategies_enum> QkVAR_strategies = {VS200_A, VS200_B, VS200_C, VS300};
    
    for(L2_control_strategies_enum x : QkVAR_strategies)
    {
        if(control_strategy == x)
        {
            return_val = true;
            break;
        }
    }
    
    return return_val;
}


std::pair<bool, L2_control_strategies_enum> get_L2_control_strategies_enum(const std::string str_val)
{
	bool conversion_successfull = true;
    L2_control_strategies_enum enum_val;
    
	std::string tmp_str = "";
	
	// ignores whitespace
	for (int i = 0; i < str_val.length(); i++)
		if (!std::isspace(str_val[i]))
			tmp_str += str_val[i];

	if 		(tmp_str == "NA") 	    enum_val = NA;
	else if	(tmp_str == "ES100-A") 	enum_val = ES100_A;
    else if	(tmp_str == "ES100-B") 	enum_val = ES100_B;
    else if	(tmp_str == "ES110") 	enum_val = ES110;
	else if (tmp_str == "ES200") 	enum_val = ES200;
	else if (tmp_str == "ES300") 	enum_val = ES300;
	else if (tmp_str == "ES500") 	enum_val = ES500;
    
	else if (tmp_str == "VS100") 	enum_val = VS100;
	else if (tmp_str == "VS200-A") 	enum_val = VS200_A;
    else if (tmp_str == "VS200-B") 	enum_val = VS200_B;
    else if (tmp_str == "VS200-C") 	enum_val = VS200_C;
    else if (tmp_str == "VS300") 	enum_val = VS300;
	else
	{
        conversion_successfull = false;
        enum_val = NA;
    }
    
    std::pair<bool, L2_control_strategies_enum> return_val = {conversion_successfull, enum_val};
    
    return return_val;
}


bool is_L2_ES_control_strategy(L2_control_strategies_enum control_strategy_enum)
{
    std::vector<L2_control_strategies_enum> ES_strategies = {ES100_A, ES100_B, ES110, ES200, ES300, ES500};
    
    bool return_val = false;
    for(L2_control_strategies_enum x : ES_strategies)
    {
        if(control_strategy_enum == x)
        {
            return_val = true;
            break;
        }
    }
   
    return return_val;
}


bool is_L2_VS_control_strategy(L2_control_strategies_enum control_strategy_enum)
{
    std::vector<L2_control_strategies_enum> VS_strategies = {VS100, VS200_A, VS200_B, VS200_C, VS300};
    
    bool return_val = false;
    for(L2_control_strategies_enum x : VS_strategies)
    {
        if(control_strategy_enum == x)
        {
            return_val = true;
            break;
        }
    }
   
    return return_val;
}


control_strategy_enums::control_strategy_enums()
{
    this->inverter_model_supports_Qsetpoint = false;
    this->ES_control_strategy = NA;
    this->VS_control_strategy = NA;
    this->ext_control_strategy = "NA";
}


//==================================================================
//                    ES500 Aggregator Structures
//==================================================================

bool ES500_aggregator_charging_needs::is_empty()
{
    return (this->pev_charge_needs.size() == 0);
}


bool ES500_aggregator_e_step_setpoints::is_empty()
{
    return (this->SE_id.size() == 0);
}


ES500_aggregator_e_step_setpoints::ES500_aggregator_e_step_setpoints(double next_aggregator_timestep_start_time_, std::vector<SupplyEquipmentId> SE_id_, std::vector<double> e3_step_kWh_, std::vector<double> charge_progression_)
{
    this->next_aggregator_timestep_start_time = next_aggregator_timestep_start_time_;
    this->SE_id = SE_id_;
    this->e3_step_kWh = e3_step_kWh_;
    this->charge_progression = charge_progression_;
}


ES500_charge_cycling_control_boundary_point::ES500_charge_cycling_control_boundary_point(double cycling_magnitude_, double cycling_vs_ramping_)
{
	this->cycling_magnitude = cycling_magnitude_;
	this->cycling_vs_ramping = cycling_vs_ramping_;
}


//==================================================================
//                       Charge Event Data
//==================================================================

std::ostream& operator<<(std::ostream& out, const stop_charging_decision_metric& x)
{
    if(x == stop_charging_using_target_soc) 				    out << "stop_charging_using_target_soc";
	else if(x == stop_charging_using_depart_time)  				out << "stop_charging_using_depart_time";
	else if(x == stop_charging_using_whatever_happens_first)  	out << "stop_charging_using_whatever_happens_first";
		
	return out;
}

std::ostream& operator<<(std::ostream& out, const stop_charging_mode& x)
{
	     if(x == target_charging) 	out << "target_charging";
	else if(x == block_charging) 	out << "block_charging";
	
	return out;
}


std::ostream& operator<<(std::ostream& out, const stop_charging_criteria& x)
{
	out << x.decision_metric << "," << x.soc_mode << "," << x.depart_time_mode << "," << x.soc_block_charging_max_undershoot_percent << "," << x.depart_time_block_charging_max_undershoot_percent;
	return out;
}


std::string stop_charging_criteria::get_file_header()
{
	return "decision_metric,soc_mode,depart_time_mode,soc_block_charging_max_undershoot_percent,depart_time_block_charging_max_undershoot_percent";
}


std::ostream& operator<<(std::ostream& out, const charge_event_data& x)
{
	out << x.charge_event_id << "," << x.SE_group_id << "," << x.SE_id << "," << x.vehicle_id << "," << x.vehicle_type << "," << x.arrival_unix_time << "," << x.departure_unix_time << "," << x.arrival_SOC << "," << x.departure_SOC << "," << x.stop_charge;
	return out;
}


std::string charge_event_data::get_file_header()
{
	return "charge_event_id,SE_group_id,SE_id,vehicle_id,vehicle_type,arrival_unix_time,departure_unix_time,arrival_SOC,departure_SOC," + stop_charging_criteria::get_file_header();
}

    
stop_charging_criteria::stop_charging_criteria()
{
    this->decision_metric = stop_charging_using_whatever_happens_first;
    this->soc_mode = target_charging;
    this->depart_time_mode = block_charging;
    this->soc_block_charging_max_undershoot_percent = 50;
    this->depart_time_block_charging_max_undershoot_percent = 40;
}


stop_charging_criteria::stop_charging_criteria(stop_charging_decision_metric decision_metric_, stop_charging_mode soc_mode_, stop_charging_mode depart_time_mode_, double soc_block_charging_max_undershoot_percent_, double depart_time_block_charging_max_undershoot_percent_)
{
    this->decision_metric = decision_metric_;
    this->soc_mode = soc_mode_;
    this->depart_time_mode = depart_time_mode_;
    this->soc_block_charging_max_undershoot_percent = soc_block_charging_max_undershoot_percent_;
    this->depart_time_block_charging_max_undershoot_percent = depart_time_block_charging_max_undershoot_percent_;
}


charge_event_data::charge_event_data(int charge_event_id_, int SE_group_id_, SupplyEquipmentId SE_id_, vehicle_id_type vehicle_id_, EV_type vehicle_type,
                                     double arrival_unix_time_, double departure_unix_time_, double arrival_SOC_, double departure_SOC_, 
                                     stop_charging_criteria stop_charge_, control_strategy_enums control_enums_)
{ 
    this->charge_event_id = charge_event_id_;
    this->SE_group_id = SE_group_id_;
    this->SE_id = SE_id_;
    this->vehicle_id = vehicle_id_;
    this->vehicle_type = vehicle_type;
    this->arrival_unix_time = arrival_unix_time_;
    this->departure_unix_time = departure_unix_time_;
    this->arrival_SOC = arrival_SOC_;
    this->departure_SOC = departure_SOC_;
    this->stop_charge = stop_charge_;
    this->control_enums = control_enums_;
}


SE_group_charge_event_data::SE_group_charge_event_data(int SE_group_id_, std::vector<charge_event_data> charge_events_)
{
    this->SE_group_id = SE_group_id_;
    this->charge_events = charge_events_;
}


//==================================================================
//                       SE_group Configuration
//==================================================================

SE_configuration::SE_configuration(int SE_group_id_, SupplyEquipmentId SE_id_, EVSE_type supply_equipment_type_, double lat_, double long_, grid_node_id_type grid_node_id_, std::string location_type_)
{
    this->SE_group_id = SE_group_id_;
    this->SE_id = SE_id_;
    this->supply_equipment_type = supply_equipment_type_;
    this->lattitude = lat_;
    this->longitude = long_;
    this->grid_node_id = grid_node_id_;
    this->location_type = location_type_;
}


SE_group_configuration::SE_group_configuration(int SE_group_id_, std::vector<SE_configuration> SEs_)
{
    this->SE_group_id = SE_group_id_;
    this->SEs = SEs_;
}


//==================================================================
//                    Status of Charging 
//==================================================================

std::ostream& operator<<(std::ostream& out, const SE_charging_status& x)
{
	     if(x == no_ev_plugged_in) 				out << "no_ev_plugged_in";
	else if(x == ev_plugged_in_not_charging) 	out << "ev_plugged_in_not_charging";
	else if(x == ev_charging) 					out << "ev_charging";
	else if(x == ev_charge_complete) 			out << "ev_charge_complete";
    else if(x == ev_charge_ended_early) 		out << "ev_charge_ended_early";
    
	return out;
}

//==================================================================
//                     PEV Ramping Parameters
//==================================================================


pev_charge_ramping::pev_charge_ramping(double off_to_on_delay_sec_, double off_to_on_kW_per_sec_, double on_to_off_delay_sec_, double on_to_off_kW_per_sec_,
                                       double ramp_up_delay_sec_, double ramp_up_kW_per_sec_, double ramp_down_delay_sec_, double ramp_down_kW_per_sec_)
{
   this->off_to_on_delay_sec = off_to_on_delay_sec_;
   this->off_to_on_kW_per_sec = off_to_on_kW_per_sec_;
   this->on_to_off_delay_sec = on_to_off_delay_sec_;
   this->on_to_off_kW_per_sec = on_to_off_kW_per_sec_;
   this->ramp_up_delay_sec = ramp_up_delay_sec_;
   this->ramp_up_kW_per_sec = ramp_up_kW_per_sec_;
   this->ramp_down_delay_sec = ramp_down_delay_sec_;
   this->ramp_down_kW_per_sec = ramp_down_kW_per_sec_;
}

