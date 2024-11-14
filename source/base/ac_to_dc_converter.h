
#ifndef inl_ac_to_dc_converter_H
#define inl_ac_to_dc_converter_H

#include "datatypes_module.h"       // ac_power_metrics
#include "helper.h"                 // poly_function_of_x


//===================================================================
//                        ac_to_dc_converter   
//===================================================================

// https://stackoverflow.com/questions/12902751/how-to-clone-object-in-c-or-is-there-another-solution

class ac_to_dc_converter
{
private:
    poly_function_of_x inv_eff_from_P2;
    bool can_provide_reactive_power;
    
protected:
    charge_event_P3kW_limits CE_P3kW_limits;
    double max_nominal_S3kVA;
    double target_Q3_kVAR;

public:
    ac_to_dc_converter( const bool can_provide_reactive_power_,
                        const charge_event_P3kW_limits& CE_P3kW_limits_,
                        const double S3kVA_from_max_nominal_P3kW_multiplier,
                        const poly_function_of_x& inv_eff_from_P2_ );
    virtual ~ac_to_dc_converter();
    virtual ac_to_dc_converter* clone() const = 0;
    
    bool get_can_provide_reactive_power_control() const;
    double get_max_nominal_S3kVA() const;
    double get_P3_from_P2( const double P2 );
    double get_approximate_P2_from_P3( const double P3 );
    void set_target_Q3_kVAR( const double target_Q3_kVAR_ );
    
    virtual void get_next( const double time_step_duration_hrs,
                           const double P1_kW,
                           const double P2_kW,
                           ac_power_metrics& return_val ) = 0;
};


//===================================================================
//                     ac_to_dc_converter_pf
//===================================================================

class ac_to_dc_converter_pf : public ac_to_dc_converter
{
private:
    poly_function_of_x inv_pf_from_P3;

public:
    ac_to_dc_converter_pf( const charge_event_P3kW_limits& CE_P3kW_limits_,
                           const double S3kVA_from_max_nominal_P3kW_multiplier,
                           const poly_function_of_x& inv_eff_from_P2_,
                           const poly_function_of_x& inv_pf_from_P3_ );
    virtual ~ac_to_dc_converter_pf() override final;
    ac_to_dc_converter* clone() const override;
    
    virtual void get_next( const double time_step_duration_hrs,
                           const double P1_kW,
                           const double P2_kW,
                           ac_power_metrics& return_val ) override final;
};

//===================================================================
//                 ac_to_dc_converter_Q_setpoint
//===================================================================

class ac_to_dc_converter_Q_setpoint : public ac_to_dc_converter
{
private:

public:
    ac_to_dc_converter_Q_setpoint( const charge_event_P3kW_limits& CE_P3kW_limits_,
                                   const double S3kVA_from_max_nominal_P3kW_multiplier,
                                   const poly_function_of_x& inv_eff_from_P2_ );
    virtual ~ac_to_dc_converter_Q_setpoint() override final;
    ac_to_dc_converter* clone() const override;
    
    virtual void get_next( const double time_step_duration_hrs,
                           const double P1_kW,
                           const double P2_kW,
                           ac_power_metrics& return_val ) override final;
};

#endif

