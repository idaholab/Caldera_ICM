
#include "ac_to_dc_converter.h"

#include <cmath>    // abs, exp, log, sqrt

//===================================================================
//                        ac_to_dc_converter   
//===================================================================

ac_to_dc_converter::~ac_to_dc_converter() {}

ac_to_dc_converter::ac_to_dc_converter( const bool can_provide_reactive_power_,
                                        const charge_event_P3kW_limits& CE_P3kW_limits_,
                                        const double S3kVA_from_max_nominal_P3kW_multiplier,
                                        const poly_function_of_x& inv_eff_from_P2_ )
{
    this->can_provide_reactive_power = can_provide_reactive_power_;
    this->CE_P3kW_limits = CE_P3kW_limits_;
    this->inv_eff_from_P2 = inv_eff_from_P2_;
    
    this->max_nominal_S3kVA = S3kVA_from_max_nominal_P3kW_multiplier * this->CE_P3kW_limits.max_P3kW;
}


bool ac_to_dc_converter::get_can_provide_reactive_power_control() const
{
    return this->can_provide_reactive_power;
}


double ac_to_dc_converter::get_max_nominal_S3kVA() const
{
    return this->max_nominal_S3kVA;
}
    

double ac_to_dc_converter::get_P3_from_P2( const double P2 )
{
    return P2 / this->inv_eff_from_P2.get_val(P2);
}


double ac_to_dc_converter::get_approximate_P2_from_P3( const double P3 )
{
    double approx_P2 = P3 * this->inv_eff_from_P2.get_val(P3);
    
    // P2_magLB -> The bound closest to zero
    // P2_magUB -> The bound furthest from zero        
    
    double P2_magUB, P2_magLB, approx_P3;
    bool P2_magLB_valid = false;
    bool P2_magUB_valid = false;
    
    // Loop until we are within tolerance.
    while( true )
    {
        approx_P3 = approx_P2 / this->inv_eff_from_P2.get_val(approx_P2);
        
        // If we are within tolerance, then break.
        if(std::abs(P3 - approx_P3) < 0.001)
        {
            break;
        }
        
        if( std::abs(approx_P3) < std::abs(P3) )
        {
            P2_magLB = approx_P2;
            P2_magLB_valid = true;
        }
        else if( std::abs(approx_P3) >= std::abs(P3) )
        {
            P2_magUB = approx_P2;
            P2_magUB_valid = true;
        }
        else
        {
            std::cout << "Error: This shouldn't happen. [1]" << std::endl;
            exit(0);
        }
        
        if( P2_magLB_valid && P2_magUB_valid )
        {
            approx_P2 = 0.5*(P2_magLB + P2_magUB);
        }
        else if( P2_magLB_valid && !P2_magUB_valid )
        {
            approx_P2 = 1.1*P2_magLB;
        }
        else if( !P2_magLB_valid && P2_magUB_valid )
        {
            approx_P2 = 0.9*P2_magUB;
        }
        else
        {
            std::cout << "Error: This shouldn't happen. [2]" << std::endl;
            exit(0);
        }
    }
    
    return approx_P2;
}


void ac_to_dc_converter::set_target_Q3_kVAR( const double target_Q3_kVAR_ ) 
{
    this->target_Q3_kVAR = target_Q3_kVAR_;
}


//===================================================================
//                     ac_to_dc_converter_pf
//===================================================================

// The sign of pf dictates the sign of Q_kVAR independent of sign of P_kW
// if pf < 0 then Q_kVAR < 0
// if pf > 0 then Q_kVAR > 0

ac_to_dc_converter_pf::~ac_to_dc_converter_pf() {}


ac_to_dc_converter_pf::ac_to_dc_converter_pf( const charge_event_P3kW_limits& CE_P3kW_limits_,
                                              const double S3kVA_from_max_nominal_P3kW_multiplier,
                                              const poly_function_of_x& inv_eff_from_P2_,
                                              const poly_function_of_x& inv_pf_from_P3_ )
                         : ac_to_dc_converter(false, CE_P3kW_limits_, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2_)
{
    this->inv_pf_from_P3 = inv_pf_from_P3_;
}


ac_to_dc_converter* ac_to_dc_converter_pf::clone() const
{
    return new ac_to_dc_converter_pf(*this);
}


void ac_to_dc_converter_pf::get_next( const double time_step_duration_hrs,
                                      const double P1_kW,
                                      const double P2_kW,
                                      ac_power_metrics& return_val )
{
    const double P3_kW = this->get_P3_from_P2(P2_kW);    
    
    const double pf = this->inv_pf_from_P3.get_val(P3_kW);
    
    const double Q3_kVAR = [&] () {
        double Q3_kVAR = P3_kW*std::sqrt(-1 + 1/(pf*pf));
        if( pf < 0 )
        {
            Q3_kVAR = -1*std::abs(Q3_kVAR);
        }
        return Q3_kVAR;
    }();
    
    return_val.time_step_duration_hrs = time_step_duration_hrs;
    return_val.P1_kW = P1_kW;
    return_val.P2_kW = P2_kW;
    return_val.P3_kW = P3_kW;
    return_val.Q3_kVAR = Q3_kVAR;
}


//===================================================================
//                 ac_to_dc_converter_Q_setpoint
//===================================================================

ac_to_dc_converter_Q_setpoint::~ac_to_dc_converter_Q_setpoint() {}


ac_to_dc_converter_Q_setpoint::ac_to_dc_converter_Q_setpoint( const charge_event_P3kW_limits& CE_P3kW_limits_,
                                                              const double S3kVA_from_max_nominal_P3kW_multiplier,
                                                              const poly_function_of_x& inv_eff_from_P2_ )
                                         : ac_to_dc_converter(true, CE_P3kW_limits_, S3kVA_from_max_nominal_P3kW_multiplier, inv_eff_from_P2_) {}


ac_to_dc_converter* ac_to_dc_converter_Q_setpoint::clone() const
{
    return new ac_to_dc_converter_Q_setpoint(*this);
}


void ac_to_dc_converter_Q_setpoint::get_next( const double time_step_duration_hrs,
                                              const double P1_kW,
                                              const double P2_kW,
                                              ac_power_metrics& return_val )
{
    // ac_kVA_limit should not be used when creating charge_profile_library.
    // this->target_Q3_kVAR = 0 when creating charge_profile_library.
    
    const double P3_kW = this->get_P3_from_P2(P2_kW);
    
    const double Q3_kVAR = [&] () {
        double Q3_kVAR;
        const double ac_kVA_limit = get_max_nominal_S3kVA();
        if( ac_kVA_limit < P3_kW )
        {
            Q3_kVAR = 0;
        }
        else
        {
            const double Q3_limit = std::sqrt( ac_kVA_limit*ac_kVA_limit - P3_kW*P3_kW );
            if( std::abs(this->target_Q3_kVAR) < Q3_limit )
            {
                Q3_kVAR = this->target_Q3_kVAR;
            }
            else
            {
                Q3_kVAR = (0 <= this->target_Q3_kVAR) ? Q3_limit : -1*Q3_limit;
            }
        }
        return Q3_kVAR;
    }();
    
    return_val.time_step_duration_hrs = time_step_duration_hrs;
    return_val.P1_kW = P1_kW;
    return_val.P2_kW = P2_kW;
    return_val.P3_kW = P3_kW;
    return_val.Q3_kVAR = Q3_kVAR;
}

