
#include "supply_equipment_load.h"
#include "SE_EV_factory.h"  		// factory_EV_charge_model, factory_ac_to_dc_converter

#include <cmath>
#include <algorithm>                // sort


//#############################################################################
//                            Charge  Event  Handler
//#############################################################################


charge_event_handler::charge_event_handler(charge_event_queuing_inputs& CE_queuing_inputs_)
{
    this->CE_queuing_inputs = CE_queuing_inputs_;
}


inline double overlap(double start_A, double end_A, double start_B, double end_B)
{
    return std::max(0.0, std::min(end_A, end_B) - std::max(start_A, start_B));
}


void charge_event_handler::add_charge_event(charge_event_data& CE)
{
    double max_allowed_overlap_time_sec = this->CE_queuing_inputs.max_allowed_overlap_time_sec;
    queuing_mode_enum queuing_mode = this->CE_queuing_inputs.queuing_mode;
    
    if(queuing_mode == overlapLimited_mostRecentlyQueuedHasPriority)
    {
        std::set<charge_event_data>::iterator it_lb = this->charge_events.upper_bound(CE);
        if(it_lb != this->charge_events.begin())
            it_lb--;

        std::vector<std::set<charge_event_data>::iterator> its_to_remove;
        double overlap_time_sec;

        for(std::set<charge_event_data>::iterator it = it_lb; it != this->charge_events.end(); it++)
        {
            overlap_time_sec = overlap(it->arrival_unix_time, it->departure_unix_time, CE.arrival_unix_time, CE.departure_unix_time);
            
            if(overlap_time_sec > max_allowed_overlap_time_sec)  
                its_to_remove.push_back(it);
            
            else if(it->arrival_unix_time > CE.departure_unix_time)
                break;
        }

        for(std::set<charge_event_data>::iterator it : its_to_remove)
        {
            this->charge_events.erase(it);
            std::string str = "Warning : Charge Event removed due to overlap.  charge_event_id: " + std::to_string(it->charge_event_id);
            std::cout << str << std::endl;
        }
    }
    else if(queuing_mode == overlapAllowed_earlierArrivalTimeHasPriority)
    {
        while(true)
        {
            if(this->charge_events.count(CE) == 0)
                break;
            else
                CE.arrival_unix_time += 0.001;
        }
    }

    this->charge_events.insert(CE);
}


bool charge_event_handler::charge_event_is_available(double now_unix_time)
{    
    



    // *******************************************************************
    // NOTE: on Apr 20, 2023, we found a bug where the code crashed
    //       in this section because it couldn't dereference the iterator
    //       in the line that says 'this->charge_events.erase(it);'.
    // This was *ONLY IN DEBUG, ONLY ON WINDOWS*.  It worked on Windows in Release mode
    //       and it worked on Linux.  So it's a strange bug.
    // We replaced this block with an equivalent (although perhaps slightly slower)
    //       block of code below.
    // *******************************************************************

    //----------------------------------------------------------------
    // Delete charge_events if there is less than 60 seconds to charge
    //----------------------------------------------------------------
    
    // std::vector<std::set<charge_event_data>::iterator> its_to_remove;
    // 
    // for(std::set<charge_event_data>::iterator it = this->charge_events.begin(); it != this->charge_events.end(); it++)
    // {
    //     if(it->departure_unix_time - now_unix_time < 60)
    //     {
    //         its_to_remove.push_back(it);
    //     }
    //     else
    //     {
    //         break;
    //     }
    // }
    // 
    // std::cout << "flag_ppp_04" << std::endl;
    // std::cout << "its_to_remove.size(): " << its_to_remove.size() << std::endl;
    // std::cout << "this->charge_events.size(): " << this->charge_events.size() << std::endl;
    // 
    // int count = 0;
    // for(std::set<charge_event_data>::iterator it : its_to_remove)
	// {
    //     std::cout << "    in the 'its_to_remove' loop."  << count++ << std::endl;
    //     this->charge_events.erase(it);
	// 	std::string str = "Warning : Charge Event removed since charge time < 60 sec.  charge_event_id: " + std::to_string(it->charge_event_id);
	// 	std::cout << str << std::endl;
	// }
    // 
    // std::cout << "flag_ppp_05" << std::endl;
    
    
    
    
    //----------------------------------------------------------------
    // Delete charge_events if there is less than 60 seconds to charge
    //----------------------------------------------------------------
    std::vector<charge_event_data> its_to_remove;
    for( const charge_event_data& item : this->charge_events )
    {
        if(item.departure_unix_time - now_unix_time < 60)
        {
            its_to_remove.push_back(item);
        }
        else
        {
            break;
        }
    }
    for( const charge_event_data& item : its_to_remove)
	{
        this->charge_events.erase(item);
		std::string str = "Warning : Charge Event removed since charge time < 60 sec.  charge_event_id: " + std::to_string(item.charge_event_id);
		std::cout << str << std::endl;
	}
    
    
    
    
    
    //------------------------------
    
    if(this->charge_events.size() == 0)
        return false;
    
    std::set<charge_event_data>::iterator it = this->charge_events.begin();
    
    if(it->arrival_unix_time - now_unix_time <= 0)
        return true;
    else
        return false;
}


charge_event_data charge_event_handler::get_next_charge_event(double now_unix_time)
{
    std::set<charge_event_data>::iterator it = this->charge_events.begin();
    charge_event_data x = *it;
    this->charge_events.erase(it);
    
    return x;
}


//#############################################################################
//                           Supply Equipment 
//#############################################################################

supply_equipment_load::supply_equipment_load()
{
	this->ac_to_dc_converter_obj = NULL;
    this->ev_charge_model = NULL;
    
	this->PEV_charge_factory = NULL;
    this->charge_profile_library = NULL;
    this->cur_charge_profile = NULL;
}


supply_equipment_load::supply_equipment_load(double P2_limit_kW_, double standby_acP_kW_, double standby_acQ_kVAR_, SE_configuration& SE_config_, charge_event_queuing_inputs& CE_queuing_inputs)
{
	this->P2_limit_kW = P2_limit_kW_;
	this->standby_acP_kW = standby_acP_kW_;
	this->standby_acQ_kVAR = standby_acQ_kVAR_;
	this->SE_config = SE_config_;
    
    charge_event_handler X(CE_queuing_inputs);
    this->event_handler = X;
	
    this->SE_stat.SE_config = SE_config_;
    this->SE_stat.now_unix_time = -1;
    this->SE_stat.SE_charging_status_val = no_ev_plugged_in;
    this->SE_stat.pev_is_connected_to_SE = false;
    
    this->ac_to_dc_converter_obj = NULL;
    this->ev_charge_model = NULL;
    
	this->PEV_charge_factory = NULL;
    this->charge_profile_library = NULL;
    this->cur_charge_profile = NULL;    
}


supply_equipment_load::~supply_equipment_load()
{	
	if(this->ac_to_dc_converter_obj != NULL)
	{
		delete this->ac_to_dc_converter_obj;
		this->ac_to_dc_converter_obj = NULL;
	}
	
	if(this->ev_charge_model != NULL)
	{
		delete this->ev_charge_model;
		this->ev_charge_model = NULL;
	}
}


void supply_equipment_load::set_pointers(factory_EV_charge_model* PEV_charge_factory_, factory_ac_to_dc_converter* ac_to_dc_converter_factory_, pev_charge_profile_library* charge_profile_library_)
{
    this->PEV_charge_factory = PEV_charge_factory_;
    this->ac_to_dc_converter_factory = ac_to_dc_converter_factory_;
    this->charge_profile_library = charge_profile_library_;
}


/*
supply_equipment_load::supply_equipment_load(const supply_equipment_load& obj)
{
	*this = obj;
}


supply_equipment_load& supply_equipment_load::operator=(const supply_equipment_load& obj)
{	
	this->P2_limit_kW = obj.P2_limit_kW;
	this->standby_acP_kW = obj.standby_acP_kW;
	this->standby_acQ_kVAR = obj.standby_acQ_kVAR;
	this->SE_config = obj.SE_config;
    this->SE_stat = obj.SE_stat;
	this->event_handler = obj.event_handler;
	
	this->PEV_charge_factory = obj.PEV_charge_factory;
	
	
	if(obj.ac_to_dc_converter_obj != NULL)
		this->ac_to_dc_converter_obj = obj.ac_to_dc_converter_obj->clone();
	else
		this->ac_to_dc_converter_obj = NULL;
	
	
	if(obj.ev_charge_model != NULL)
		this->ev_charge_model = new vehicle_charge_model(*obj.ev_charge_model);
	else
		this->ev_charge_model = NULL;
		
	return *this;
}
*/


void supply_equipment_load::set_target_acP3_kW(double target_acP3_kW_)
{
	if(this->ev_charge_model != NULL)
    {
        double approx_P2_kW = this->ac_to_dc_converter_obj->get_approxamate_P2_from_P3(target_acP3_kW_);
        
        if(this->P2_limit_kW < approx_P2_kW)
    		approx_P2_kW = this->P2_limit_kW;
        
      	this->ev_charge_model->set_target_P2_kW(approx_P2_kW);
    }
}


void supply_equipment_load::set_target_acQ3_kVAR(double target_acQ3_kVAR_)
{
    if(this->ev_charge_model != NULL)
    {
        this->ac_to_dc_converter_obj->set_target_Q3_kVAR(target_acQ3_kVAR_);
    } 
}


double supply_equipment_load::get_PEV_SE_combo_max_nominal_S3kVA()
{
    if(this->ev_charge_model != NULL)
        return this->ac_to_dc_converter_obj->get_max_nominal_S3kVA();
    
    std::cout << "CALDERA ERROR: Calling supply_equipment_load::get_PEV_SE_combo_max_nominal_S3kVA when this->ac_to_dc_converter_obj is NULL" << std::endl;
    return 0.1;
}


void supply_equipment_load::get_active_charge_profile_forecast_akW(double setpoint_P3kW, double time_step_mins, bool& pev_is_connected_to_SE, std::vector<double>& charge_profile)
{
    std::vector<pev_charge_profile_result> charge_profile_aux;
    get_active_charge_profile_forecast_allInfo(setpoint_P3kW, time_step_mins, pev_is_connected_to_SE, charge_profile_aux);
    
    //---------------
    
    charge_profile.clear();
    double time_step_hrs = time_step_mins/60.0;
    
    for(const pev_charge_profile_result& x : charge_profile_aux)
    {
        charge_profile.push_back(x.E3_kWh/time_step_hrs);
    }
}


void supply_equipment_load::get_active_charge_profile_forecast_allInfo(double setpoint_P3kW, double time_step_mins, bool& pev_is_connected_to_SE, std::vector<pev_charge_profile_result>& charge_profile)
{
    charge_profile.clear();
    
    double time_to_complete_charge_hrs;
    get_time_to_complete_active_charge_hrs(setpoint_P3kW, pev_is_connected_to_SE, time_to_complete_charge_hrs);
    
    if(pev_is_connected_to_SE && 0 < time_to_complete_charge_hrs)
    {
        std::vector<double> charge_time_hrs;
        double time_hrs = 0;
        double time_step_hrs = time_step_mins/60.0;
        
        while(true)
        {
            time_hrs += time_step_hrs;
            
            if(time_hrs > time_to_complete_charge_hrs)
            {
                charge_time_hrs.push_back(time_to_complete_charge_hrs);
                break;
            }
            else
                charge_time_hrs.push_back(time_hrs);
        }

        double startSOC = this->SE_stat.current_charge.now_soc;
        this->cur_charge_profile->find_chargeProfile_given_startSOC_and_chargeTimes(setpoint_P3kW, startSOC, charge_time_hrs, charge_profile);
    }
}


void supply_equipment_load::get_CE_forecast_on_interval(double setpoint_P3kW, double nowSOC, double endSOC, double now_unix_time, double end_unix_time, pev_charge_profile_result& return_val)
{
    // Needed due to incomplete implementation of pev_charge_profile.
    if(setpoint_P3kW != 0)
        setpoint_P3kW = 100000;
    
    //======================
    
    stop_charging_criteria stop_charge_val = this->SE_stat.current_charge.stop_charge;
    
    double departure_SOC = this->SE_stat.current_charge.departure_SOC;
    double departure_unix_time = this->SE_stat.current_charge.departure_unix_time;
    
    //======================
    
    pev_charge_profile_result target_soc_val, depart_time_val;
    bool return_val_is_zero = false;
    
    if(stop_charge_val.decision_metric == stop_charging_using_target_soc || stop_charge_val.decision_metric == stop_charging_using_whatever_happens_first)
    {
        if(departure_SOC < endSOC)
            endSOC = departure_SOC;
        
        if(endSOC <= nowSOC)
            return_val_is_zero = true;
        else
            target_soc_val = this->cur_charge_profile->find_result_given_startSOC_and_endSOC(setpoint_P3kW, nowSOC, endSOC);
    }
    
    if(stop_charge_val.decision_metric == stop_charging_using_depart_time || stop_charge_val.decision_metric == stop_charging_using_whatever_happens_first)
    {
        if(departure_unix_time < end_unix_time)
            end_unix_time = departure_unix_time;
        
        if(end_unix_time <= now_unix_time)
            return_val_is_zero = true;
        else
        {
            double charge_time_hrs = (end_unix_time - now_unix_time)/3600.0;
            depart_time_val = this->cur_charge_profile->find_result_given_startSOC_and_chargeTime(setpoint_P3kW, nowSOC, charge_time_hrs);
        }
    }
    
    //======================
    
    if(return_val_is_zero)
    {
        return_val.soc_increase = 0;
        return_val.E1_kWh = 0;
        return_val.E2_kWh = 0;
        return_val.E3_kWh = 0;
        return_val.cumQ3_kVARh = 0;
        return_val.total_charge_time_hrs = 0.00001;  // prevent divide by zero error in calling function (when calculate PkW)
        return_val.incremental_chage_time_hrs = -1;        
    }    
    else if(stop_charge_val.decision_metric == stop_charging_using_target_soc)
        return_val = target_soc_val;
    else if(stop_charge_val.decision_metric == stop_charging_using_depart_time)
        return_val = depart_time_val;
    else if(stop_charge_val.decision_metric == stop_charging_using_whatever_happens_first)
    {
        if(depart_time_val.total_charge_time_hrs < target_soc_val.total_charge_time_hrs)
            return_val = depart_time_val;
        else
            return_val = target_soc_val;
    }
}


void supply_equipment_load::get_CE_stats_at_end_of_charge(double setpoint_P3kW, double nowSOC, double now_unix_time, bool& pev_is_connected_to_SE, pev_charge_profile_result& return_val)
{    
    pev_is_connected_to_SE = this->SE_stat.pev_is_connected_to_SE;
    
    if(pev_is_connected_to_SE)
    {
        double departure_SOC = this->SE_stat.current_charge.departure_SOC;
        double departure_unix_time = this->SE_stat.current_charge.departure_unix_time;
        
        get_CE_forecast_on_interval(setpoint_P3kW, nowSOC, departure_SOC, now_unix_time, departure_unix_time, return_val);
    }
}


void supply_equipment_load::get_CE_FICE(FICE_inputs inputs, double nowSOC, double now_unix_time, bool& pev_is_connected_to_SE, CE_FICE& return_val)
{
    pev_is_connected_to_SE = this->SE_stat.pev_is_connected_to_SE;
    
    if(pev_is_connected_to_SE)
    {
        pev_charge_profile_result A, B;
        double setpoint_P3kW;
        double interval_start_unixtime = inputs.interval_start_unixtime;
        double interval_duration_sec = inputs.interval_duration_sec;
        double departure_SOC = this->SE_stat.current_charge.departure_SOC;
        
        //---------------------------
        //      Get Result A
        //---------------------------
        if(inputs.interval_start_unixtime < now_unix_time)
        {
            A.soc_increase = 0;
            A.E1_kWh = 0;
            A.E2_kWh = 0;
            A.E3_kWh = 0;
            A.cumQ3_kVARh = 0;
            A.total_charge_time_hrs = 0;
            A.incremental_chage_time_hrs = 0;
            interval_start_unixtime = now_unix_time;
            interval_duration_sec -= now_unix_time - interval_start_unixtime;
            if(interval_duration_sec < 0) interval_duration_sec = 0;
        }
        else
        {
            setpoint_P3kW = ev_charge_model->get_target_P2_kW();
            get_CE_forecast_on_interval(setpoint_P3kW, nowSOC, departure_SOC, now_unix_time, interval_start_unixtime, A);
        }
        
        //---------------------------
        //      Get Result B
        //---------------------------
        double interval_start_soc = nowSOC + A.soc_increase;
        double interval_end_unixtime = interval_start_unixtime + inputs.interval_duration_sec;
        get_CE_forecast_on_interval(inputs.acPkW_setpoint, interval_start_soc, departure_SOC, interval_start_unixtime, interval_end_unixtime, B);
        
        //---------------------------
        //      Get Return Val
        //---------------------------
        return_val.SE_id = this->SE_stat.SE_config.SE_id;
        return_val.charge_event_id = this->SE_stat.current_charge.charge_event_id;
        return_val.charge_energy_ackWh = B.E3_kWh;
        return_val.interval_duration_hrs = B.total_charge_time_hrs;
    }
}


void supply_equipment_load::get_time_to_complete_active_charge_hrs(double setpoint_P3kW, bool& pev_is_connected_to_SE, double& time_to_complete_charge_hrs)
{
    double now_soc = this->SE_stat.current_charge.now_soc;
    double now_unix_time = this->SE_stat.now_unix_time;
    pev_charge_profile_result X;
    
    get_CE_stats_at_end_of_charge(setpoint_P3kW, now_soc, now_unix_time, pev_is_connected_to_SE, X);
    
    if(pev_is_connected_to_SE)
        time_to_complete_charge_hrs = X.total_charge_time_hrs;
    else
        time_to_complete_charge_hrs = 0;
}


void supply_equipment_load::get_active_CE(bool& pev_is_connected_to_SE, active_CE& active_CE_val)
{
    pev_is_connected_to_SE = this->SE_stat.pev_is_connected_to_SE;
    
    if(pev_is_connected_to_SE)
    {
        active_CE_val.SE_id = this->SE_config.SE_id;
        active_CE_val.charge_event_id = this->SE_stat.current_charge.charge_event_id;
        active_CE_val.now_unix_time = this->SE_stat.now_unix_time;
        active_CE_val.now_soc = this->SE_stat.current_charge.now_soc;
        active_CE_val.now_charge_energy_ackWh = this->SE_stat.current_charge.now_charge_energy_E3kWh;
        active_CE_val.now_dcPkW = this->SE_stat.current_charge.now_dcPkW;
        active_CE_val.now_acPkW = this->SE_stat.current_charge.now_acPkW;
        active_CE_val.now_acQkVAR = this->SE_stat.current_charge.now_acQkVAR;
        active_CE_val.vehicle_id = this->SE_stat.current_charge.vehicle_id;
        active_CE_val.vehicle_type = this->SE_stat.current_charge.vehicle_type;
        
        //----------------------------
        
        pev_charge_profile_result X;
        double time_to_complete_charge_hrs;
        double setpoint_P3kW = 10000;
        
        get_time_to_complete_active_charge_hrs(setpoint_P3kW, pev_is_connected_to_SE, time_to_complete_charge_hrs);
        active_CE_val.min_remaining_charge_time_hrs = time_to_complete_charge_hrs;
        
        double arrival_unix_time = this->SE_stat.current_charge.arrival_unix_time;                
        double arrival_SOC = this->SE_stat.current_charge.arrival_SOC;        
        get_CE_stats_at_end_of_charge(setpoint_P3kW, arrival_SOC, arrival_unix_time, pev_is_connected_to_SE, X);
        active_CE_val.min_time_to_complete_entire_charge_hrs = X.total_charge_time_hrs;        
        active_CE_val.energy_of_complete_charge_ackWh = X.E3_kWh;
    }
}


void supply_equipment_load::get_current_CE_status(bool& pev_is_connected_to_SE, SE_charging_status& SE_status_val, CE_status& charge_status)
{
    pev_is_connected_to_SE = this->SE_stat.pev_is_connected_to_SE;
    SE_status_val = this->SE_stat.SE_charging_status_val;
    charge_status = this->SE_stat.current_charge;
}


SE_status supply_equipment_load::get_SE_status()
{    
    return this->SE_stat;
}


std::vector<completed_CE> supply_equipment_load::get_completed_CE()
{
    std::vector<completed_CE> return_val;
    
    std::vector<CE_status> completed_charges = this->SE_stat.completed_charges;
    this->SE_stat.completed_charges.clear();
    
    completed_CE Y;
    Y.SE_id = this->SE_config.SE_id;
    
    for(CE_status& X : completed_charges)
    {
        Y.charge_event_id = X.charge_event_id;
        Y.final_soc = X.now_soc;
        return_val.push_back(Y);
    }
    
    return return_val;
}


void supply_equipment_load::add_charge_event(charge_event_data& charge_event)
{
    this->event_handler.add_charge_event(charge_event);
}


bool supply_equipment_load::pev_is_connected_to_SE__ev_charge_model_not_NULL()
{
    return (this->ev_charge_model != NULL);
}


bool supply_equipment_load::pev_is_connected_to_SE(double now_unix_time)
{
    bool return_val = false;
    
    if(this->ev_charge_model != NULL)
    {
        if(this->ev_charge_model->pev_is_connected_to_SE(now_unix_time))
            return_val = true;
    }
    
    return return_val;
}


control_strategy_enums supply_equipment_load::get_control_strategy_enums()
{
    return this->control_enums;
}


pev_charge_profile* supply_equipment_load::get_pev_charge_profile()
{
    return this->cur_charge_profile;
}


bool supply_equipment_load::get_next(double prev_unix_time, double now_unix_time, double pu_Vrms, double& soc, ac_power_metrics& ac_power)
{
    bool is_new_CE__update_control_strategies = false;

	if(this->ev_charge_model == NULL)
    {
        this->SE_stat.pev_is_connected_to_SE = false;

        if(this->event_handler.charge_event_is_available(now_unix_time))
        {
        	charge_event_data charge_event = this->event_handler.get_next_charge_event(now_unix_time);
        	supply_equipment_enum SE_type = this->SE_config.supply_equipment_type;

        	this->ev_charge_model = this->PEV_charge_factory->alloc_get_EV_charge_model(charge_event, SE_type, this->P2_limit_kW);

        		// ev_charge_model == NULL when there is a compatibility issue between the PEV and Supply Equipment
        	if(this->ev_charge_model != NULL)
        	{
                this->SE_stat.current_charge.charge_event_id = charge_event.charge_event_id;
                this->SE_stat.current_charge.vehicle_id = charge_event.vehicle_id;
                this->SE_stat.current_charge.vehicle_type = charge_event.vehicle_type;
                this->SE_stat.current_charge.stop_charge = charge_event.stop_charge;                
                this->SE_stat.current_charge.arrival_unix_time = charge_event.arrival_unix_time;
                this->SE_stat.current_charge.departure_unix_time = charge_event.departure_unix_time;
                this->SE_stat.current_charge.arrival_SOC = charge_event.arrival_SOC;
                this->SE_stat.current_charge.departure_SOC = charge_event.departure_SOC;
                
                this->SE_stat.current_charge.now_soc = charge_event.arrival_SOC;
                this->SE_stat.current_charge.now_unix_time = now_unix_time;
                this->SE_stat.current_charge.now_charge_energy_E3kWh = 0;
                
                //------------------
                
                is_new_CE__update_control_strategies = true;
                this->control_enums = charge_event.control_enums;
                
                //--------------------------------
                //  Update Charge Profile
                //--------------------------------
                supply_equipment_enum SE_type = this->SE_config.supply_equipment_type;
                vehicle_enum pev_type = charge_event.vehicle_type;
                
                if(this->charge_profile_library != NULL)
                    this->cur_charge_profile = this->charge_profile_library->get_charge_profile(pev_type, SE_type);
                else  
                    this->cur_charge_profile = NULL;  // This is what is used when creating charge_profile_library.
                
                //--------------------------------
                //  Create ac_to_dc_converter_obj
                //--------------------------------
                charge_event_P3kW_limits P3kW_limits;
                
                if(this->cur_charge_profile != NULL)
                    P3kW_limits = this->cur_charge_profile->get_charge_event_P3kW_limits();
                else
                {
                    // This is what is used when creating charge_profile_library.
                    P3kW_limits.min_P3kW = 0;
                    P3kW_limits.max_P3kW = 1;
                }

                ac_to_dc_converter_enum converter_type = pf;
                if(this->control_enums.inverter_model_supports_Qsetpoint)
                    converter_type = Q_setpoint;
                
                if(this->ac_to_dc_converter_obj != NULL)
                {
                    delete this->ac_to_dc_converter_obj;
                    this->ac_to_dc_converter_obj = NULL;
                }
                
                this->ac_to_dc_converter_obj = this->ac_to_dc_converter_factory->alloc_get_ac_to_dc_converter(converter_type, SE_type, pev_type, P3kW_limits);                
        	
                //----------------------------------------
                //          Set P3, Q3 Targets 
                //----------------------------------------
                if(this->control_enums.ES_control_strategy == NA && control_enums.ext_control_strategy == "NA")
                    set_target_acP3_kW(1.3*this->P2_limit_kW);      // Uncontrolled Charging - (Nominal Power May increase with Vrms)
                else
                    set_target_acP3_kW(0);
                
                set_target_acQ3_kVAR(0);
            }
        }
    }
    
	bool return_default_ac_power = true;
    SE_charging_status SE_charge_status;
    double Q3_kVAR = this->standby_acQ_kVAR;
    soc = -1;
    
	//if(this->ev_charge_model != NULL && this->ev_charge_model->pev_is_connected_to_SE(now_unix_time))
    if(this->ev_charge_model != NULL && this->ev_charge_model->pev_has_arrived_at_SE(now_unix_time))
	{
        battery_state bat_state;
		bool charge_has_completed;

		this->ev_charge_model->get_next(prev_unix_time, now_unix_time, pu_Vrms, charge_has_completed, bat_state);
        
//std::cout << "Simulation Time: " << now_unix_time/3600 << "    target_P2_kW: " << this->ev_charge_model->get_target_P2_kW() << "    P2_kW: " << bat_state.P2_kW << "    now-prev: " << now_unix_time-prev_unix_time << std::endl;
        
        this->SE_stat.pev_is_connected_to_SE = true;
        this->SE_stat.current_charge.now_soc = bat_state.soc_t1;
        this->SE_stat.current_charge.now_unix_time = now_unix_time;
        this->SE_stat.current_charge.now_dcPkW = bat_state.P2_kW;

        soc = bat_state.soc_t1;
		this->ac_to_dc_converter_obj->get_next(bat_state.time_step_duration_hrs, bat_state.P1_kW, bat_state.P2_kW, ac_power);
        
        this->SE_stat.current_charge.now_acPkW = ac_power.P3_kW;
        this->SE_stat.current_charge.now_acQkVAR = ac_power.Q3_kVAR;
        
        if(this->ac_to_dc_converter_obj->get_can_provide_reactive_power_control())
            Q3_kVAR = ac_power.Q3_kVAR;
        
		//--------------
        
		if(ac_power.P3_kW > this->standby_acP_kW)
        {
        	return_default_ac_power = false;
            this->SE_stat.current_charge.now_charge_energy_E3kWh += ac_power.P3_kW * ac_power.time_step_duration_hrs;
        }
        
		if(charge_has_completed)
		{
//################
//if(this->SE_stat.current_charge.charge_event_id < 1000) 
//    std::cout << "Charge Completed!  now_time_hrs: " << now_unix_time/3600.0 << "  charge_event_id: " << this->SE_stat.current_charge.charge_event_id << "  soc: " << bat_state.soc_t1 << "  SE_type: " << this->SE_config.supply_equipment_type << "  EV_type: " << this->SE_stat.current_charge.vehicle_type << std::endl;
//################

			SE_charge_status = ev_charge_complete;
			
            delete this->ev_charge_model;
            this->ev_charge_model = NULL;
            
            CE_status x = this->SE_stat.current_charge;
            this->SE_stat.completed_charges.push_back(x);
            
            this->SE_stat.pev_is_connected_to_SE = false;
            this->SE_stat.current_charge.now_charge_energy_E3kWh = 0;
        }
        else
        {
        	if(std::abs(bat_state.P1_kW) < 0.0001)
        		SE_charge_status = ev_plugged_in_not_charging;
        	else
        		SE_charge_status = ev_charging;
        }
	}
	else
	{
		SE_charge_status = no_ev_plugged_in;
        this->SE_stat.pev_is_connected_to_SE = false;
	}
    
    this->SE_stat.SE_charging_status_val = SE_charge_status;
    this->SE_stat.now_unix_time = now_unix_time;
    
	//--------------------------------------
	
	if(return_default_ac_power)
	{
		ac_power.time_step_duration_hrs = (now_unix_time - prev_unix_time)/3600;
		ac_power.P1_kW = 0;
		ac_power.P2_kW = 0;
		ac_power.P3_kW = this->standby_acP_kW;
		ac_power.Q3_kVAR = Q3_kVAR;
	}
    
    return is_new_CE__update_control_strategies;
}


void supply_equipment_load::stop_active_CE()
{
    if(this->ev_charge_model != NULL)
	{
        this->SE_stat.SE_charging_status_val = ev_charge_ended_early;
        
        delete this->ev_charge_model;
        this->ev_charge_model = NULL;
        
        CE_status x = this->SE_stat.current_charge;
        this->SE_stat.completed_charges.push_back(x);
        
        this->SE_stat.pev_is_connected_to_SE = false;
        this->SE_stat.current_charge.now_charge_energy_E3kWh = 0;
    }
}
