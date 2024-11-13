
#include "datatypes_module.h"


//===================================================================
//        ac_power_metrics   (Used in ac_to_dc_converter)
//===================================================================

ac_power_metrics::ac_power_metrics(double time_step_duration_hrs_, double P1_kW_, double P2_kW_, double P3_kW_, double Q3_kVAR_)
{
    this->time_step_duration_hrs = time_step_duration_hrs_;
    this->P1_kW = P1_kW_;
    this->P2_kW = P2_kW_;
	this->P3_kW = P3_kW_;
	this->Q3_kVAR = Q3_kVAR_;
}


void ac_power_metrics::add_to_self(const ac_power_metrics& rhs)
{
	this->time_step_duration_hrs = rhs.time_step_duration_hrs;
	this->P1_kW += rhs.P1_kW;
	this->P2_kW += rhs.P2_kW;
	this->P3_kW += rhs.P3_kW;
	this->Q3_kVAR += rhs.Q3_kVAR;
}


std::string ac_power_metrics::get_file_header()
{
	return "time_step_duration_hrs, P1_kW, P2_kW, P3_kW, Q3_kVAR";
}


std::ostream& operator<<(std::ostream& out, ac_power_metrics& x)
{
	out << x.time_step_duration_hrs << "," << x.P1_kW << "," << x.P2_kW << "," << x.P3_kW << "," << x.Q3_kVAR;
	return out;
}


