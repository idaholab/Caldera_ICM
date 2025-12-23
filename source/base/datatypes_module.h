
#ifndef inl_datatypes_module_H
#define inl_datatypes_module_H

#include <string>
#include <iostream>
#include <fstream>

#include "datatypes_global.h"  

//===================================================================
//        ac_power_metrics   (Used in ac_to_dc_converter)
//===================================================================

struct ac_power_metrics
{
    double time_step_duration_hrs;
    double P1_kW; 
    double P2_kW;
    double P3_kW; 
    double Q3_kVAR;
    
    ac_power_metrics() {};
    ac_power_metrics(double time_step_duration_hrs_, double P1_kW_, double P2_kW_, double P3_kW_, double Q3_kVAR_);
    void add_to_self(const ac_power_metrics& rhs);
    static std::string get_file_header();
};
std::ostream& operator<<(std::ostream& out, ac_power_metrics& x);

//===========================================
//         CE_status & SE_status 
//===========================================

struct CE_status
{
    int charge_event_id;
    vehicle_id_type vehicle_id;
    EV_type vehicle_type;
    stop_charging_criteria stop_charge;
    double arrival_unix_time; 
    double departure_unix_time;
    double arrival_SOC;
    double departure_SOC;
    double now_unix_time;
    double now_soc;
    double now_charge_energy_E3kWh;
    double now_dcPkW;
    double now_acPkW;  
    double now_acQkVAR;
    
    CE_status() {};
};


struct SE_status
{
    double now_unix_time;

    SE_configuration SE_config;
    SE_charging_status SE_charging_status_val;
    
    bool pev_is_connected_to_SE;
    CE_status current_charge;
    std::vector<CE_status> completed_charges;
    
    SE_status() {};
    SE_status(
        const double now_unix_time,
        const SE_configuration& SE_config,
        const SE_charging_status& SE_charging_status_val,
        const bool pev_is_connected_to_SE
    )
        :
        now_unix_time{ now_unix_time }, 
        SE_config{ SE_config }, 
        SE_charging_status_val{ SE_charging_status_val },
        pev_is_connected_to_SE{ pev_is_connected_to_SE },
        current_charge{ CE_status{} }, 
        completed_charges{ std::vector<CE_status>{} }
    {

    }
};


#endif

