
#ifndef inl_pev_charge_profile_factory_H
#define inl_pev_charge_profile_factory_H

#include <vector>
#include <iostream>

//#############################################################################
//                          linear_regression
//#############################################################################

struct xy_point 
{
    double x;
    double y;
    double w;        
};

struct lin_reg_slope_yinter
{
    double m;
    double b;
};

class linear_regression
{
public:
    static lin_reg_slope_yinter  non_weighted(const std::vector<xy_point> &points);
    static lin_reg_slope_yinter  weighted(const std::vector<xy_point> &points);
};


//#############################################################################
//                        PEV Charge Profile Factory
//#############################################################################

struct bat_objfun_constraints
{
    double a;
    double b;
};


enum pev_battery_chemistry
{
	LTO = 0,
	LMO = 1,
	NMC = 2
};


enum Ppu_vs_soc_point_type
{
    interpolate,
    extend,
    use_directly
};


struct Ppu_vs_soc_point
{
    double soc;
    double P_pu;
    Ppu_vs_soc_point_type point_type;
    
    bool operator<(const Ppu_vs_soc_point& rhs) const
    {
        return soc < rhs.soc;
    }

    bool operator<(Ppu_vs_soc_point& rhs) const
    {
        return soc < rhs.soc;
    }
};


struct line_segment
{	  // y = a*x + b
	double x_LB;
    double x_UB;
	double a;
	double b;
   
   	double y_UB() {return a*x_UB + b;}
   	double y_LB() {return a*x_LB + b;}
        
   	bool operator<(const line_segment& rhs) const
   	{
   		return this->x_LB < rhs.x_LB;
   	}

    bool operator<(line_segment& rhs) const
   	{
   		return this->x_LB < rhs.x_LB;
   	}
};
std::ostream& operator<<(std::ostream& out, line_segment& x);


class Ppu_vs_soc_curve
{
private:
    std::vector<Ppu_vs_soc_point> points;
    double C_rate;

public:
    Ppu_vs_soc_curve() {}
    Ppu_vs_soc_curve(double C_rate_, const std::vector<Ppu_vs_soc_point> &points_);
    
    double get_current_rate_amps(double bat_capacity_Ah_1C);
    const std::vector<Ppu_vs_soc_point>& get_points();
    
    bool operator<(const Ppu_vs_soc_curve& rhs) const
    {
        return C_rate < rhs.C_rate;
    }

    bool operator<(Ppu_vs_soc_curve& rhs) const
    {
        return C_rate < rhs.C_rate;
    }
};


class create_dcPkW_from_soc
{
private:
    std::vector<Ppu_vs_soc_curve> curves;
    bool is_charging_not_discharging;

    void get_charging_dcfc_charge_profile(double bat_energy_kWh, double dc_current_limit, double dc_pwr_limit_kW, double bat_capacity_Ah_1C, std::vector<line_segment> &dcPkW_from_soc, std::vector<bat_objfun_constraints> &constraints);
    void get_discharging_dcfc_charge_profile(double bat_energy_kWh, double dc_current_limit, double dc_pwr_limit_kW, double bat_capacity_Ah_1C, std::vector<line_segment> &dcPkW_from_soc, std::vector<bat_objfun_constraints> &constraints);

public:
    create_dcPkW_from_soc() {};
    create_dcPkW_from_soc(bool is_charging_not_discharging_, const std::vector<Ppu_vs_soc_curve>& curves_);
    void get_dcfc_charge_profile(double bat_energy_kWh, double dc_current_limit, double dc_pwr_limit_kW, double bat_capacity_Ah_1C, std::vector<line_segment> &dcPkW_from_soc, std::vector<bat_objfun_constraints> &constraints);

	void get_L1_or_L2_charge_profile(double bat_energy_kWh, double charge_rate_kW, std::vector<line_segment> &pev_charge_profile);	
};


class pev_charge_profile_factory
{
private:
    create_dcPkW_from_soc LMO_charge;    
    create_dcPkW_from_soc LTO_charge;    
    create_dcPkW_from_soc NMC_charge;
    
    std::vector<bat_objfun_constraints> constraints;
    
public:
	pev_charge_profile_factory();	
	
	std::vector<line_segment> get_dcfc_charge_profile(pev_battery_chemistry bat_chemistry, double battery_size_kWh, double bat_capacity_Ah_1C, double pev_max_c_rate, double pev_P2_pwr_limit_kW, double dcfc_dc_current_limit);
	std::vector<bat_objfun_constraints> get_constraints();
	
	std::vector<line_segment> get_L1_or_L2_charge_profile(pev_battery_chemistry bat_chemistry, double bat_energy_kWh, double pev_charge_rate_kW);
};


#endif

