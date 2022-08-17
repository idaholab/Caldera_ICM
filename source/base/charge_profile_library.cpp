
#include <vector>
#include <unordered_set>

#include <algorithm>    // sort, upper_bound
#include <random>       // Uniform random number generator
#include <list>
#include <iostream>

// ----------------------------------------------------------------------------------------
//  ???  Riddle  ???
//  If <math.h> and <cmath> are both commented out the program will compile and execute
//  But the output is only valid if <math.h> is not commented out. Why?
// ----------------------------------------------------------------------------------------
#include <math.h>
//#include <cmath>      // nan, round


#include "charge_profile_library.h"


void search_vector_of_doubles(double search_value, std::vector<double>& search_vector, int& LB_index, int& UB_index)
{
    std::vector<double>::iterator it_UB;
    it_UB = std::upper_bound(search_vector.begin(), search_vector.end(), search_value);
    
    if(it_UB == search_vector.begin())
    {
        LB_index = 0;
        UB_index = 0;
    }
    else if(it_UB == search_vector.end())
    {
        LB_index = search_vector.size() - 1;
        UB_index = LB_index;
    }
    else
    {
        UB_index = it_UB - search_vector.begin();
        
        if(UB_index == 0)
            LB_index = 0;
        else
            LB_index = UB_index - 1;
    }
}


pev_charge_profile_result get_default_charge_profile_result()
{
    pev_charge_profile_result obj;
    obj.soc_increase = 0;
    obj.E1_kWh = 0;
    obj.E2_kWh = 0;
    obj.E3_kWh = 0;
    obj.cumQ3_kVARh = 0;
    obj.total_charge_time_hrs = 0;
    obj.incremental_chage_time_hrs = NAN;
    
    return obj;
}


//==============================================================================
//                           pev_charge_profile_aux
//==============================================================================

pev_charge_profile_aux::pev_charge_profile_aux(vehicle_enum pev_type_, supply_equipment_enum SE_type_, double setpoint_P3kW_, std::vector<pev_charge_fragment>& charge_fragments_)
{
    this->pev_type = pev_type_;
    this->SE_type = SE_type_;
    this->setpoint_P3kW = setpoint_P3kW_;
    this->charge_fragments = charge_fragments_;
    
    std::sort(this->charge_fragments.begin(), this->charge_fragments.end());
    
    int num_charge_fragments = this->charge_fragments.size();
    
    this->soc_search.resize(num_charge_fragments);
    this->charge_time_search.resize(num_charge_fragments);
    
    for(int i=0; i<num_charge_fragments; i++)
    {
        this->soc_search[i] = this->charge_fragments[i].soc;
        this->charge_time_search[i] = this->charge_fragments[i].time_since_charge_began_hrs;
    }
}


double pev_charge_profile_aux::get_setpoint_P3kW()
{
    return this->setpoint_P3kW;
}


pev_charge_fragment pev_charge_profile_aux::get_last_chargeFragment()
{
    return this->charge_fragments[this->charge_fragments.size() - 1];
}


pev_charge_fragment pev_charge_profile_aux::calculate_charge_fragment_weighted_average(double w, pev_charge_fragment& LB, pev_charge_fragment& UB)
{
    pev_charge_fragment return_val;
    return_val.soc = (1.0-w)*LB.soc + w*UB.soc;
    return_val.E1_kWh = (1.0-w)*LB.E1_kWh + w*UB.E1_kWh;
    return_val.E2_kWh = (1.0-w)*LB.E2_kWh + w*UB.E2_kWh;
    return_val.E3_kWh = (1.0-w)*LB.E3_kWh + w*UB.E3_kWh;
    return_val.cumQ3_kVARh = (1.0-w)*LB.cumQ3_kVARh + w*UB.cumQ3_kVARh;
    return_val.time_since_charge_began_hrs = (1.0-w)*LB.time_since_charge_began_hrs + w*UB.time_since_charge_began_hrs;
    
    return return_val;
}


pev_charge_fragment pev_charge_profile_aux::get_chargeFragment(bool search_value_is_soc_not_timeHrs, double search_value)
{
    int UB_index, LB_index;
    
    if(search_value_is_soc_not_timeHrs)
        search_vector_of_doubles(search_value, this->soc_search, LB_index, UB_index);
    else
        search_vector_of_doubles(search_value, this->charge_time_search, LB_index, UB_index);
    
    pev_charge_fragment &LB = this->charge_fragments[LB_index];
    pev_charge_fragment &UB = this->charge_fragments[UB_index];
    
    //---------------------
    
    double w, LB_val, UB_val;
    if(search_value_is_soc_not_timeHrs)
    {
        LB_val = LB.soc;
        UB_val = UB.soc;
    }
    else
    {
        LB_val = LB.time_since_charge_began_hrs;
        UB_val = UB.time_since_charge_began_hrs;
    }
    
    if(abs(LB_val - UB_val) < 0.000001)
        w = 1.0;
    else
        w = (search_value - LB_val)/(UB_val - LB_val);
    
    
    return calculate_charge_fragment_weighted_average(w, LB, UB);
}


pev_charge_profile_result pev_charge_profile_aux::get_pev_charge_profile_result(pev_charge_fragment& start, pev_charge_fragment& end)
{
    pev_charge_profile_result obj;
    
    if(end.soc < start.soc)
    {
        std::cout << "ERROR: In pev_charge_profile_aux end_soc less than start_soc." << std::endl;
        obj = get_default_charge_profile_result();
    }
    else
    {
        obj.soc_increase = end.soc - start.soc;
        obj.E1_kWh = end.E1_kWh - start.E1_kWh;
        obj.E2_kWh = end.E2_kWh - start.E2_kWh;
        obj.E3_kWh = end.E3_kWh - start.E3_kWh;
        obj.cumQ3_kVARh = end.cumQ3_kVARh - start.cumQ3_kVARh;
        obj.total_charge_time_hrs = end.time_since_charge_began_hrs - start.time_since_charge_began_hrs;
        obj.incremental_chage_time_hrs = NAN;
    }
    
    return obj; 
}


void pev_charge_profile_aux::get_chargeProfile_from_cumulativeChargeProfile(std::vector<pev_charge_profile_result>& charge_profile, std::vector<pev_charge_profile_result>& cumulative_charge_profile)
{
    int num_elements = cumulative_charge_profile.size();
    charge_profile.resize(num_elements);
    
    charge_profile[0].soc_increase = cumulative_charge_profile[0].soc_increase;
    charge_profile[0].E1_kWh = cumulative_charge_profile[0].E1_kWh;
    charge_profile[0].E2_kWh = cumulative_charge_profile[0].E2_kWh;
    charge_profile[0].E3_kWh = cumulative_charge_profile[0].E3_kWh;
    charge_profile[0].cumQ3_kVARh = cumulative_charge_profile[0].cumQ3_kVARh;
    charge_profile[0].total_charge_time_hrs = cumulative_charge_profile[0].total_charge_time_hrs;
    charge_profile[0].incremental_chage_time_hrs = cumulative_charge_profile[0].total_charge_time_hrs;
    
    for(int i=1; i<num_elements; i++)
    {
        charge_profile[i].soc_increase = cumulative_charge_profile[i].soc_increase - cumulative_charge_profile[i-1].soc_increase;
        charge_profile[i].E1_kWh = cumulative_charge_profile[i].E1_kWh - cumulative_charge_profile[i-1].E1_kWh;
        charge_profile[i].E2_kWh = cumulative_charge_profile[i].E2_kWh - cumulative_charge_profile[i-1].E2_kWh;
        charge_profile[i].E3_kWh = cumulative_charge_profile[i].E3_kWh - cumulative_charge_profile[i-1].E3_kWh;
        charge_profile[i].cumQ3_kVARh = cumulative_charge_profile[i].cumQ3_kVARh - cumulative_charge_profile[i-1].cumQ3_kVARh;
        charge_profile[i].total_charge_time_hrs = cumulative_charge_profile[i].total_charge_time_hrs;
        charge_profile[i].incremental_chage_time_hrs = cumulative_charge_profile[i].total_charge_time_hrs - cumulative_charge_profile[i-1].total_charge_time_hrs;
    }
}


void pev_charge_profile_aux::find_chargeProfile(double startSOC, bool values_are_soc_not_timeHrs, std::vector<double>& values, std::vector<pev_charge_profile_result>& charge_profile, std::vector<pev_charge_profile_result>& cumulative_charge_profile)
{
    cumulative_charge_profile.clear();
    
    //-----------------------
    
    int num_elements = values.size();
    
    pev_charge_fragment start_fragment = get_chargeFragment(true, startSOC);
    pev_charge_fragment end_fragment;
    pev_charge_profile_result charge_info;
    
    double X = 0;
    if(!values_are_soc_not_timeHrs)
        X = start_fragment.time_since_charge_began_hrs;
    
    for(int i=0; i<num_elements; i++)
    {      
        end_fragment = get_chargeFragment(values_are_soc_not_timeHrs, values[i] + X);
        charge_info = get_pev_charge_profile_result(start_fragment, end_fragment);
        
        if(i > 0 && (std::abs(charge_info.total_charge_time_hrs - cumulative_charge_profile[i-1].total_charge_time_hrs) < 0.000001))
            break;
        else
            cumulative_charge_profile.push_back(charge_info);
    }
    
    get_chargeProfile_from_cumulativeChargeProfile(charge_profile, cumulative_charge_profile);
}


pev_charge_profile_result pev_charge_profile_aux::find_result_given_startSOC_and_endSOC(double startSOC, double endSOC)
{
    pev_charge_fragment start_fragment = get_chargeFragment(true, startSOC);
    pev_charge_fragment end_fragment = get_chargeFragment(true, endSOC);
    return get_pev_charge_profile_result(start_fragment, end_fragment);
}


pev_charge_profile_result pev_charge_profile_aux::find_result_given_startSOC_and_chargeTime(double startSOC, double charge_time_hrs)
{
    pev_charge_fragment start_fragment = get_chargeFragment(true, startSOC);
    pev_charge_fragment end_fragment = get_chargeFragment(false, start_fragment.time_since_charge_began_hrs + charge_time_hrs);    
    return get_pev_charge_profile_result(start_fragment, end_fragment);
}


void pev_charge_profile_aux::find_chargeProfile_given_startSOC_and_endSOCs(double startSOC, std::vector<double>& endSOC, std::vector<pev_charge_profile_result>& charge_profile)
{
    std::vector<pev_charge_profile_result> cumulative_charge_profile;
    find_chargeProfile(startSOC, true, endSOC, charge_profile, cumulative_charge_profile);
}


void pev_charge_profile_aux::find_chargeProfile_given_startSOC_and_chargeTimes(double startSOC, std::vector<double>& charge_time_hrs, std::vector<pev_charge_profile_result>& charge_profile)
{
    std::vector<pev_charge_profile_result> cumulative_charge_profile;
    find_chargeProfile(startSOC, false, charge_time_hrs, charge_profile, cumulative_charge_profile);
}


//==============================================================================
//                             pev_charge_profile
//==============================================================================

pev_charge_profile::pev_charge_profile(vehicle_enum pev_type_, supply_equipment_enum SE_type_, charge_event_P3kW_limits& CE_P3kW_limits_, std::vector<pev_charge_profile_aux>& charge_profiles_)
{
    this->pev_type = pev_type_;
    this->SE_type = SE_type_;
    this->CE_P3kW_limits = CE_P3kW_limits_;
    
    this->charge_profiles = charge_profiles_;
    std::sort(this->charge_profiles.begin(), this->charge_profiles.end());
    
    this->UB_profile = NULL;
    this->LB_profile = NULL;
    
    //----------------------
    
    int num_elements = this->charge_profiles.size();
    this->setpoint_P3kW_search.resize(num_elements);
    
    for(int i=0; i<num_elements; i++)
    {
        this->setpoint_P3kW_search[i] = this->charge_profiles[i].get_setpoint_P3kW();
    }
}


charge_event_P3kW_limits pev_charge_profile::get_charge_event_P3kW_limits() {return this->CE_P3kW_limits;}

std::vector<double> pev_charge_profile::get_P3kW_setpoints_of_charge_profiles() {return this->setpoint_P3kW_search;}


void pev_charge_profile::update_UB_LB_profiles(double setpoint_P3kW)
{
    int LB_index, UB_index;
    
    search_vector_of_doubles(setpoint_P3kW, this->setpoint_P3kW_search, LB_index, UB_index);
    
    this->UB_profile = &this->charge_profiles[UB_index];
    this->LB_profile = &this->charge_profiles[LB_index];
}


pev_charge_profile_result pev_charge_profile::find_result_given_startSOC_and_endSOC(double setpoint_P3kW, double startSOC, double endSOC)
{
    if(setpoint_P3kW <= 0)
    {
        std::cout << "ERROR A1: In pev_charge_profile (setpoint_P3kW <= 0)." << std::endl;
        return get_default_charge_profile_result();
    }
    
    update_UB_LB_profiles(setpoint_P3kW);
    
    return this->UB_profile->find_result_given_startSOC_and_endSOC(startSOC, endSOC);
}


pev_charge_profile_result pev_charge_profile::find_result_given_startSOC_and_chargeTime(double setpoint_P3kW, double startSOC, double charge_time_hrs)
{
    if(setpoint_P3kW <= 0)
    {
        std::cout << "ERROR A2: In pev_charge_profile (setpoint_P3kW <= 0)." << std::endl;
        return get_default_charge_profile_result();
    }
    
    update_UB_LB_profiles(setpoint_P3kW);
    return this->UB_profile->find_result_given_startSOC_and_chargeTime(startSOC, charge_time_hrs);
}


void pev_charge_profile::find_chargeProfile_given_startSOC_and_endSOCs(double setpoint_P3kW, double startSOC, std::vector<double>& endSOC, std::vector<pev_charge_profile_result>& charge_profile)
{    
    if(setpoint_P3kW <= 0)
    {
        std::cout << "ERROR A3: In pev_charge_profile (setpoint_P3kW <= 0)." << std::endl;
        charge_profile.clear();
        return;
    }

    update_UB_LB_profiles(setpoint_P3kW);
    this->UB_profile->find_chargeProfile_given_startSOC_and_endSOCs(startSOC, endSOC, charge_profile);
}


void pev_charge_profile::find_chargeProfile_given_startSOC_and_chargeTimes(double setpoint_P3kW, double startSOC, std::vector<double>& charge_time_hrs, std::vector<pev_charge_profile_result>& charge_profile)
{
    if(setpoint_P3kW <= 0)
    {
        std::cout << "ERROR A4: In pev_charge_profile (setpoint_P3kW <= 0)." << std::endl;
        charge_profile.clear();
        return;
    }
    
    update_UB_LB_profiles(setpoint_P3kW);
    this->UB_profile->find_chargeProfile_given_startSOC_and_chargeTimes(startSOC, charge_time_hrs, charge_profile);
}


//==============================================================================
//                          pev_charge_profile_library
//==============================================================================

pev_charge_profile_library::pev_charge_profile_library()
{
    // Dummy Values for default profile
    vehicle_enum pev_type = get_default_vehicle_enum();
    supply_equipment_enum SE_type = get_default_supply_equipment_enum(); 
    charge_event_P3kW_limits CE_P3kW_limits;
    CE_P3kW_limits.min_P3kW = 0;
    CE_P3kW_limits.max_P3kW = 1;
    
    //---------------------------------------
    
    std::vector<pev_charge_profile_aux> charge_profiles;
    std::vector<pev_charge_fragment> charge_fragments;
    
    double setpoint_P3kW = 0;
    charge_fragments.clear();  // {soc, E1_kWh, E2_kWh, E3_kWh, cumQ3_kVARh, time_since_charge_began_hrs}
    charge_fragments.push_back({0, 0, 0, 0, 0, 0});
    charge_fragments.push_back({0, 0, 0, 0, 0, 1000*3600});
    charge_profiles.push_back(pev_charge_profile_aux(pev_type, SE_type, setpoint_P3kW, charge_fragments));
    
    double charge_time_hrs = 10;
    setpoint_P3kW = CE_P3kW_limits.max_P3kW;
    double E3_kWh = charge_time_hrs*setpoint_P3kW;
    charge_fragments.clear();  // {soc, E1_kWh, E2_kWh, E3_kWh, cumQ3_kVARh, time_since_charge_began_hrs}
    charge_fragments.push_back({0, 0, 0, 0, 0, 0});
    //charge_fragments.push_back({100, 0.96*0.92*E3_kWh, 0.92*E3_kWh, E3_kWh, -0.1*E3_kWh, charge_time_hrs});
    charge_profiles.push_back(pev_charge_profile_aux(pev_type, SE_type, setpoint_P3kW, charge_fragments));
    
    //----------------
    
    pev_charge_profile X(pev_type, SE_type, CE_P3kW_limits, charge_profiles);
    this->default_profile = X;
}


void pev_charge_profile_library::add_charge_profile_to_library(vehicle_enum pev_type, supply_equipment_enum SE_type, pev_charge_profile& charge_profile)
{
    std::pair<vehicle_enum, supply_equipment_enum> key;
    key = std::make_pair(pev_type, SE_type);
    
    if(this->charge_profile.count(key) == 0)
        this->charge_profile[key] = charge_profile;
    else
        std::cout << "ERROR:  Duplicate charge profiles added to pev_charge_profile_library" << std::endl;
}


pev_charge_profile* pev_charge_profile_library::get_charge_profile(vehicle_enum pev_type, supply_equipment_enum SE_type)
{
    std::pair<vehicle_enum, supply_equipment_enum> key;
    key = std::make_pair(pev_type, SE_type);
    
    pev_charge_profile* return_val;
    
    if(this->charge_profile.count(key) == 0)
    {
        std::cout << "ERROR:  Charge profile not in pev_charge_profile_library. (pev_type:" << pev_type << "  SE_type:" << SE_type << ")" << std::endl;
        return_val = &this->default_profile;
    }
    else
        return_val = &this->charge_profile[key];
    
    return return_val;
}


//==============================================================================
//                          pev_charge_profile_library_v2
//==============================================================================

void pev_charge_profile_library_v2::add_charge_PkW_profile_to_library(vehicle_enum pev_type, supply_equipment_enum SE_type, double timestep_sec, std::vector<double>& soc, std::vector<ac_power_metrics>& profile)
{
    std::pair<vehicle_enum, supply_equipment_enum> key;
    key = std::make_pair(pev_type, SE_type);
    
    tmp_charge_profile X;
    X.PkW_profile = profile;
    X.soc = soc;
    X.timestep_sec = timestep_sec;
    
    if(this->PkW_profile.count(key) == 0)
        this->PkW_profile[key] = X;
    else
        std::cout << "ERROR:  Duplicate charge profiles added to pev_charge_profile_library_v2" << std::endl;
}


void pev_charge_profile_library_v2::find_index_and_weight(double soc, std::vector<double>& soc_vector, double& index, double& weight)
{
    int vector_size = soc_vector.size();
    index = -1;
    
    for(int i=0; i<vector_size; i++)
    {
        if(soc < soc_vector[i])
        {
            index = i;
            break;
        }
    }
    
    //-------------------------
    
    if(index == -1)
    {
        index = vector_size-1;
        weight = 1;
    }
    else if(index == 0)
        weight = soc/soc_vector[index];    
    else
        weight = (soc - soc_vector[index-1])/(soc_vector[index] - soc_vector[index-1]);
}


void pev_charge_profile_library_v2::find_start_end_indexes_from_start_end_soc(double start_soc, double end_soc, std::vector<double>& soc, double& start_index, double& end_index)
{
    double start_weight, end_weight;
    
    find_index_and_weight(start_soc, soc, start_index, start_weight);
    find_index_and_weight(end_soc, soc, end_index, end_weight);
    start_weight = 1 - start_weight;
    
    if(start_weight < 0.5)
        start_index += 1;
    
    if(end_weight < 0.5)
        end_index -= 1;
    
    if(start_index >= end_index)       
        end_index = start_index + 1;
}


void pev_charge_profile_library_v2::get_P3kW_charge_profile(double start_soc, double end_soc, vehicle_enum pev_type, supply_equipment_enum SE_type, double& timestep_sec, std::vector<double>& P3kW_charge_profile)
{
    timestep_sec = 1;
    P3kW_charge_profile.clear();
    
    //-----------------------------
    
    if(end_soc < start_soc)
    {
        std::cout << "ERROR:  start_soc must be less than end_soc in pev_charge_profile_library_v2::get_P3kW_charge_profile." << std::endl;
        return;
    }
    
    //-----------------------------
    
    std::pair<vehicle_enum, supply_equipment_enum> key;
    key = std::make_pair(pev_type, SE_type);
    
    if(this->PkW_profile.count(key) == 0)
        std::cout << "ERROR:  Charge profile not in pev_charge_profile_library_v2. (pev_type:" << pev_type << "  SE_type:" << SE_type << ")" << std::endl;
    else
    {
        tmp_charge_profile& X = this->PkW_profile[key];
        timestep_sec = X.timestep_sec;

        double start_index, end_index;
        find_start_end_indexes_from_start_end_soc(start_soc, end_soc, X.soc, start_index, end_index);
        
        //-------------------------
        
        std::vector<ac_power_metrics>& ac_power_vec = X.PkW_profile;
        
        for(int i=start_index; i<=end_index; i++)
            P3kW_charge_profile.push_back(ac_power_vec[i].P3_kW);
    }
}


void pev_charge_profile_library_v2::get_all_charge_profile_data(double start_soc, double end_soc, vehicle_enum pev_type, supply_equipment_enum SE_type, all_charge_profile_data& return_val)
{
    return_val.timestep_sec = 1;
    return_val.P1_kW.clear();
    return_val.P2_kW.clear();
    return_val.P3_kW.clear();
    return_val.Q3_kVAR.clear();
    return_val.soc.clear();
    
    //-----------------------------
    
    if(end_soc < start_soc)
    {
        std::cout << "ERROR:  start_soc must be less than end_soc in pev_charge_profile_library_v2::get_P3kW_charge_profile." << std::endl;
        return;
    }
    
    //-----------------------------
    
    std::pair<vehicle_enum, supply_equipment_enum> key;
    key = std::make_pair(pev_type, SE_type);
    
    if(this->PkW_profile.count(key) == 0)
        std::cout << "ERROR:  Charge profile not in pev_charge_profile_library_v2. (pev_type:" << pev_type << "  SE_type:" << SE_type << ")" << std::endl;
    else
    {
        tmp_charge_profile& X = this->PkW_profile[key];
        return_val.timestep_sec = X.timestep_sec;

        double start_index, end_index;
        find_start_end_indexes_from_start_end_soc(start_soc, end_soc, X.soc, start_index, end_index);
        
        //-------------------------
        
        std::vector<ac_power_metrics>& ac_power_vec = X.PkW_profile;
        
        for(int i=start_index; i<=end_index; i++)
        {
            return_val.P1_kW.push_back(ac_power_vec[i].P1_kW);
            return_val.P2_kW.push_back(ac_power_vec[i].P2_kW);
            return_val.P3_kW.push_back(ac_power_vec[i].P3_kW);
            return_val.Q3_kVAR.push_back(ac_power_vec[i].Q3_kVAR);
            return_val.soc.push_back(X.soc[i]);
        }
    }
}


