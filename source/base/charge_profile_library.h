
#ifndef inl_pev_charge_profile_H
#define inl_pev_charge_profile_H

#include "datatypes_global.h"
#include "datatypes_module.h"
#include "datatypes_global_SE_EV_definitions.h"
#include "helper.h"

#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>      // int64_t
#include <utility>      // pair


void search_vector_of_doubles(double search_value, std::vector<double>& search_vector, int& LB_index, int& UB_index);
pev_charge_profile_result get_default_charge_profile_result();


class pev_charge_profile_aux
{
private:
    vehicle_enum pev_type;
    supply_equipment_enum SE_type;
    double setpoint_P3kW;
    
    std::vector<pev_charge_fragment> charge_fragments;
    std::vector<double> soc_search; 
    std::vector<double> charge_time_search;
    
    pev_charge_fragment get_last_chargeFragment();
    pev_charge_fragment get_chargeFragment(bool search_value_is_soc_not_timeHrs, double search_value);
    
    pev_charge_fragment calculate_charge_fragment_weighted_average(double w, pev_charge_fragment& LB, pev_charge_fragment& UB);
    pev_charge_profile_result  get_pev_charge_profile_result(pev_charge_fragment& start, pev_charge_fragment& end);
    void get_chargeProfile_from_cumulativeChargeProfile(std::vector<pev_charge_profile_result>& charge_profile, std::vector<pev_charge_profile_result>& cumulative_charge_profile);
    void find_chargeProfile(double startSOC, bool values_are_soc_not_timeHrs, std::vector<double>& values, std::vector<pev_charge_profile_result>& charge_profile, std::vector<pev_charge_profile_result>& cumulative_charge_profile);
    
public:
    pev_charge_profile_aux(vehicle_enum pev_type_, supply_equipment_enum SE_type_, double setpoint_P3kW_, std::vector<pev_charge_fragment>& charge_fragments_);
    double get_setpoint_P3kW();
    
    pev_charge_profile_result find_result_given_startSOC_and_endSOC(double startSOC, double endSOC);
    pev_charge_profile_result find_result_given_startSOC_and_chargeTime(double startSOC, double charge_time_hrs);
    
    void find_chargeProfile_given_startSOC_and_endSOCs(double startSOC, std::vector<double>& endSOC, std::vector<pev_charge_profile_result>& charge_profile);
    void find_chargeProfile_given_startSOC_and_chargeTimes(double startSOC, std::vector<double>& charge_time_hrs, std::vector<pev_charge_profile_result>& charge_profile);
    
    bool operator<(const pev_charge_profile_aux& x) const
	{
		return this->setpoint_P3kW < x.setpoint_P3kW;
	}
};


class pev_charge_profile
{
private:
    vehicle_enum pev_type; 
    supply_equipment_enum SE_type;
    charge_event_P3kW_limits CE_P3kW_limits;
    
    std::vector<pev_charge_profile_aux> charge_profiles;
    std::vector<double> setpoint_P3kW_search;
    
    pev_charge_profile_aux* UB_profile;
    pev_charge_profile_aux* LB_profile;
    
    void update_UB_LB_profiles(double setpoint_P3kW);
    
public:
    pev_charge_profile() {};
    pev_charge_profile(vehicle_enum pev_type_, supply_equipment_enum SE_type_, charge_event_P3kW_limits& CE_P3kW_limits_, std::vector<pev_charge_profile_aux>& charge_profiles_);
    
    charge_event_P3kW_limits get_charge_event_P3kW_limits();
    std::vector<double> get_P3kW_setpoints_of_charge_profiles();
    
    pev_charge_profile_result find_result_given_startSOC_and_endSOC(double setpoint_P3kW, double startSOC, double endSOC);
    pev_charge_profile_result find_result_given_startSOC_and_chargeTime(double setpoint_P3kW, double startSOC, double charge_time_hrs);
    
    void find_chargeProfile_given_startSOC_and_endSOCs(double setpoint_P3kW, double startSOC, std::vector<double>& endSOC, std::vector<pev_charge_profile_result>& charge_profile);
    void find_chargeProfile_given_startSOC_and_chargeTimes(double setpoint_P3kW, double startSOC, std::vector<double>& charge_time_hrs, std::vector<pev_charge_profile_result>& charge_profile);
};


class pev_charge_profile_library
{
private:
    // Using map because there is no specialization of std::hash for std::pair in the c++ standard library.
    std::map< std::pair<vehicle_enum, supply_equipment_enum>, pev_charge_profile> charge_profile;
    pev_charge_profile default_profile;
    
public:
    pev_charge_profile_library();
    
    void add_charge_profile_to_library(vehicle_enum pev_type, supply_equipment_enum SE_type, pev_charge_profile& charge_profile);
    pev_charge_profile* get_charge_profile(vehicle_enum pev_type, supply_equipment_enum SE_type);
};



class pev_charge_profile_library_v2
{
private:
    struct tmp_charge_profile
    {
        std::vector<ac_power_metrics> PkW_profile;
        std::vector<double> soc;
        double timestep_sec;
    };
    
    std::map<std::pair<vehicle_enum, supply_equipment_enum>, tmp_charge_profile> PkW_profile;
    void find_index_and_weight(double soc, std::vector<double>& soc_vector, double& index, double& weight);
    void find_start_end_indexes_from_start_end_soc(double start_soc, double end_soc, std::vector<double>& soc, double& start_index, double& end_index);
    
public:  
    pev_charge_profile_library_v2() {};
    
    void add_charge_PkW_profile_to_library(vehicle_enum pev_type, supply_equipment_enum SE_type, double timestep_sec, std::vector<double>& soc, std::vector<ac_power_metrics>& profile);
    void get_P3kW_charge_profile(double start_soc, double end_soc, vehicle_enum pev_type, supply_equipment_enum SE_type, double& timestep_sec, std::vector<double>& P3kW_charge_profile);
    void get_all_charge_profile_data(double start_soc, double end_soc, vehicle_enum pev_type, supply_equipment_enum SE_type, all_charge_profile_data& return_val);
};


#endif




