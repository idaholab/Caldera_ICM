
#ifndef inl_supply_equipment_control_H
#define inl_supply_equipment_control_H

#include "datatypes_global.h"                       // ES500_aggregator_charging_needs
#include "supply_equipment_load.h"                  // supply_equipment_load
#include "helper.h"                                 // get_base_load_forecast, get_value_from_normal_distribution, get_real_value_from_uniform_distribution, get_int_value_from_uniform_distribution
#include "charge_profile_library.h"                 // pev_charge_profile
#include "datatypes_module.h"                       // CE_status

//==========================================
//   VS_get_percX_from_volt_percX_curve
//==========================================

class VS_get_percX_from_volt_percX_curve
{
private:
    std::vector<double> volt_percX_curve_puV;
    std::vector<double> volt_percX_curve_percX;
public:
    VS_get_percX_from_volt_percX_curve() {};
    VS_get_percX_from_volt_percX_curve(std::vector<double>& volt_percX_curve_puV_, std::vector<double>& volt_percX_curve_percX_);
    double get_percX(double puV);
};


//==========================================
//  manage_L2_control_strategy_parameters
//==========================================

class manage_L2_control_strategy_parameters
{
private:
    L2_control_strategy_parameters parameters;
    
    get_real_value_from_uniform_distribution  ES100A_uniformRandomNumber_0to1;
    get_real_value_from_uniform_distribution  ES100B_uniformRandomNumber_0to1;
    get_real_value_from_uniform_distribution  ES110_uniformRandomNumber_0to1;
    
    get_value_from_normal_distribution  ES500_normalRandomError_offToOnLeadTime_sec;
    get_value_from_normal_distribution  ES500_normalRandomError_defaultLeadTime_sec;

    get_int_value_from_uniform_distribution VS100_get_LPF_window_size;
    get_int_value_from_uniform_distribution VS200A_get_LPF_window_size;
    get_int_value_from_uniform_distribution VS200B_get_LPF_window_size;
    get_int_value_from_uniform_distribution VS200C_get_LPF_window_size;
    get_int_value_from_uniform_distribution VS300_get_LPF_window_size;

    VS_get_percX_from_volt_percX_curve VS100_get_percP;
    VS_get_percX_from_volt_percX_curve VS200A_get_percQ;
    VS_get_percX_from_volt_percX_curve VS200B_get_percQ;
    VS_get_percX_from_volt_percX_curve VS200C_get_percQ;

public:
    manage_L2_control_strategy_parameters() {};
    manage_L2_control_strategy_parameters(const L2_control_strategy_parameters& parameters_);
    
    const ES100_L2_parameters& get_ES100_A();
    const ES100_L2_parameters& get_ES100_B();
    const ES110_L2_parameters& get_ES110();
    const ES200_L2_parameters& get_ES200();
    const ES300_L2_parameters& get_ES300();
    const ES500_L2_parameters& get_ES500();
    
    const VS100_L2_parameters& get_VS100();
    const VS200_L2_parameters& get_VS200_A();
    const VS200_L2_parameters& get_VS200_B();
    const VS200_L2_parameters& get_VS200_C();
    const VS300_L2_parameters& get_VS300();
    
    double ES100A_getUniformRandomNumber_0to1();
    double ES100B_getUniformRandomNumber_0to1();
    double ES110_getUniformRandomNumber_0to1();
    
    double ES500_getNormalRandomError_offToOnLeadTime_sec();
    double ES500_getNormalRandomError_defaultLeadTime_sec();
    
    double VS100_get_percP_from_volt_delta_kW_curve(double puV);
    double VS200A_get_percQ_from_volt_var_curve(double puV);
    double VS200B_get_percQ_from_volt_var_curve(double puV);
    double VS200C_get_percQ_from_volt_var_curve(double puV);
    
    LPF_parameters VS100_get_LPF_parameters();
    LPF_parameters VS200A_get_LPF_parameters();
    LPF_parameters VS200B_get_LPF_parameters();
    LPF_parameters VS200C_get_LPF_parameters();
    LPF_parameters VS300_get_LPF_parameters();
    
    int get_LPF_max_window_size();
};


//=========================================
//        ES100 Control Strategy
//=========================================

class ES100_control_strategy
{
private:
    L2_control_strategies_enum L2_CS_enum;
    manage_L2_control_strategy_parameters* params;
    double target_P3kW, cur_P3kW_setpoint, charge_start_unix_time;
    
public:
    ES100_control_strategy(){};
    ES100_control_strategy(L2_control_strategies_enum L2_CS_enum_, manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double target_P3kW_, const CE_status& charge_status, pev_charge_profile* charge_profile);
    double get_P3kW_setpoint(double prev_unix_time, double now_unix_time);
};


//=========================================
//        ES110 Control Strategy
//=========================================

class ES110_control_strategy
{
private:
    manage_L2_control_strategy_parameters* params;
    double target_P3kW, cur_P3kW_setpoint, charge_start_unix_time;
    
public:
    ES110_control_strategy(){};
    ES110_control_strategy(manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double target_P3kW_, const CE_status& charge_status, pev_charge_profile* charge_profile);
    double get_P3kW_setpoint(double prev_unix_time, double now_unix_time);
};


//=========================================
//        ES200 Control Strategy
//=========================================

class ES200_control_strategy
{
private:
    manage_L2_control_strategy_parameters* params;
    double target_P3kW, cur_P3kW_setpoint;
    
public:
    ES200_control_strategy(){};
    ES200_control_strategy(manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double target_P3kW_);
    double get_P3kW_setpoint(double prev_unix_time, double now_unix_time);
};


//=========================================
//        ES300 Control Strategy
//=========================================

class ES300_control_strategy
{
private:
    manage_L2_control_strategy_parameters* params;
    double target_P3kW, cur_P3kW_setpoint;
    
public:
    ES300_control_strategy(){};
    ES300_control_strategy(manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double target_P3kW_);
    double get_P3kW_setpoint(double prev_unix_time, double now_unix_time);
};

//=========================================
//        ES500 Control Strategy
//=========================================

class ES500_control_strategy
{
private:
    manage_L2_control_strategy_parameters* params;
    double target_P3kW, cur_P3kW_setpoint, next_P3kW_setpoint, unix_time_begining_of_next_agg_step;
    bool updated_P3kW_setpoint_available;
    
public:
    ES500_control_strategy(){};
    ES500_control_strategy(manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double target_P3kW_);
    double get_P3kW_setpoint(double prev_unix_time, double now_unix_time);
    
    void get_charging_needs(double unix_time_now, double unix_time_begining_of_next_agg_step_, pev_charge_profile* charge_profile,
                            const CE_status& charge_status, const charge_event_P3kW_limits& P3kW_limits, const SE_configuration& SE_config,
                            ES500_aggregator_pev_charge_needs& pev_charge_needs);
    void set_energy_setpoints(double e3_setpoint_kWh);
};


//=========================================
//        VS100 Control Strategy
//=========================================

class VS100_control_strategy
{
private:
    manage_L2_control_strategy_parameters* params;
    double max_nominal_S3kVA;
    double prev_P3kW_setpoint;

public:
    VS100_control_strategy(){};
    VS100_control_strategy(manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double max_nominal_S3kVA_);
    double get_P3kW_setpoint(double prev_unix_time, double now_unix_time, double pu_Vrms, double pu_Vrms_SS, double target_P3kW);
};


//=========================================
//        VS200 Control Strategy
//=========================================

class VS200_control_strategy
{
private:
    L2_control_strategies_enum L2_CS_enum;
    manage_L2_control_strategy_parameters* params;
    double max_nominal_S3kVA;
    double prev_Q3kVAR_setpoint;

public:
    VS200_control_strategy(){};
    VS200_control_strategy(L2_control_strategies_enum L2_CS_enum_, manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double max_nominal_S3kVA_);
    double get_Q3kVAR_setpoint(double prev_unix_time, double now_unix_time, double pu_Vrms, double pu_Vrms_SS, double target_P3kW);
};


//=========================================
//        VS300 Control Strategy
//=========================================

class VS300_control_strategy
{
private:
    manage_L2_control_strategy_parameters* params;
    double max_nominal_S3kVA;
    double prev_Q3kVAR_setpoint;

public:
    VS300_control_strategy(){};
    VS300_control_strategy(manage_L2_control_strategy_parameters* params_);
    void update_parameters_for_CE(double max_nominal_S3kVA_);
    double get_Q3kVAR_setpoint(double pu_Vrms, double pu_Vrms_SS, double target_P3kW);
};


//=========================================
//           Helper Functions
//=========================================

void enforce_limits_on_Q3kVAR(double max_abs_Q3kVAR, double& Q3kVAR_setpoint);
double get_max_abs_Q3kVAR(double target_P3kW, double max_nominal_S3kVA, double max_QkVAR_as_percent_of_SkVA);
void enforce_ramping(double prev_unix_time, double now_unix_time, double max_delta_per_min, double prev_setpoint, double& setpoint);


//=========================================
//       supply_equipment_control
//=========================================

class supply_equipment_control
{
private:
    SE_configuration SE_config;
    
    ES100_control_strategy ES100A_obj;
    ES100_control_strategy ES100B_obj;
    ES110_control_strategy ES110_obj;
    ES200_control_strategy ES200_obj;
    ES300_control_strategy ES300_obj;
    ES500_control_strategy ES500_obj;
    
    VS100_control_strategy VS100_obj;
    VS200_control_strategy VS200A_obj;
    VS200_control_strategy VS200B_obj;
    VS200_control_strategy VS200C_obj;
    VS300_control_strategy VS300_obj;
    
    control_strategy_enums L2_control_enums;
    charge_event_P3kW_limits P3kW_limits;
    CE_status charge_status;
    
    double prev_pu_Vrms, target_P3kW;    
    bool must_charge_for_remainder_of_park;
    
    pev_charge_profile* charge_profile;
    get_base_load_forecast* baseLD_forecaster;
    manage_L2_control_strategy_parameters* manage_L2_control;
    
    LPF_kernel LPF;
    
    bool building_charge_profile_library;
    
    bool ensure_pev_charge_needs_met_for_ext_control_strategy;
    
public:
    supply_equipment_control() {};
    supply_equipment_control(bool building_charge_profile_library_, const SE_configuration& SE_config_, get_base_load_forecast* baseLD_forecaster_, manage_L2_control_strategy_parameters* manage_L2_control_);
    
    control_strategy_enums get_control_strategy_enums();
    std::string get_external_control_strategy();
    L2_control_strategies_enum  get_L2_ES_control_strategy();
    L2_control_strategies_enum  get_L2_VS_control_strategy();
    void update_parameters_for_CE(supply_equipment_load& SE_load);
    
    void set_ensure_pev_charge_needs_met_for_ext_control_strategy(bool ensure_pev_charge_needs_met);
    
    void execute_control_strategy(double prev_unix_time, double now_unix_time, double pu_Vrms, supply_equipment_load& SE_load);
    
    //------------------------
    //         ES500
    //------------------------
    void ES500_get_charging_needs(double unix_time_now, double unix_time_begining_of_next_agg_step, supply_equipment_load& SE_load,
                                  ES500_aggregator_pev_charge_needs& pev_charge_needs);
    void ES500_set_energy_setpoints(double e3_setpoint_kWh);
};

#endif

