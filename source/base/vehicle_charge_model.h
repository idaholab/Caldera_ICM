
#ifndef inl_vehicle_charge_model_H
#define inl_vehicle_charge_model_H

#include <iostream>

#include "battery.h"				                // battery
#include "datatypes_global.h"                       // charge_event_data, stop_charging_mode, stop_charging_decision_metric
#include "EVSE_characteristics.h"                   // EV_type

//---------------------------------

class vehicle_charge_model
{
private:
	charge_event_data charge_event;
    EV_type EV; 
    double arrival_unix_time, depart_unix_time, arrival_soc, requested_depart_soc;
    int charge_event_id;
    
    stop_charging_decision_metric decision_metric;
	stop_charging_mode soc_mode;
	stop_charging_mode depart_time_mode;
	double soc_block_charging_max_undershoot_percent;
	double depart_time_block_charging_max_undershoot_percent;
	
	bool stop_charging_at_target_soc;
	double depart_soc;
    
    battery bat;
    
    double target_P2_kW, soc_of_full_battery, prev_soc_t1;
    bool charge_has_completed_, charge_needs_met_;
    
    vehicle_charge_model& operator=(const vehicle_charge_model& obj) = default;
    vehicle_charge_model(const vehicle_charge_model& obj) = default;

public:
    vehicle_charge_model(const charge_event_data& event, const battery_inputs& inputs, double soc_of_full_battery);
    
    void set_target_P2_kW(double target_P2_kW_);
    double get_target_P2_kW();
    bool pev_has_arrived_at_SE(double now_unix_time);
    bool pev_is_connected_to_SE(double now_unix_time);
    bool charge_has_completed();
    
    void get_E1_battery_limits(double& max_E1_limit, double& min_E1_limit);
    
   	void get_next(double prev_unix_time, double now_unix_time, double pu_Vrms,  bool& charge_has_completed, battery_state& bat_state);
};


#endif



