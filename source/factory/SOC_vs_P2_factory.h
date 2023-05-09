#ifndef SOC_VS_P2_FACTORY_H
#define SOC_VS_P2_FACTORY_H

#include <map>
#include <unordered_map>

#include "EV_EVSE_inventory.h"

#include "helper.h"                 // line_segment, 

typedef double c_rate;
typedef double SOC;
typedef double power;

enum point_type
{
    interpolate,
    extend,
    use_directly
};

enum battery_charge_mode
{
    charging = 0,
    discharging = 1
};

struct bat_objfun_constraints
{
    double a;
    double b;
};

struct SOC_vs_P2
{
    const std::vector<line_segment> curve;
    const double zero_slope_threashold;

    SOC_vs_P2(const std::vector<line_segment>& curve, const double& zero_slope_threashold)
        : curve(curve), zero_slope_threashold(zero_slope_threashold) {}
};


class create_dcPkW_from_soc
{
private:
    
    const EV_EVSE_inventory& inventory;

    const std::map<c_rate, std::map<SOC, std::pair<power, point_type> >, std::greater<c_rate> > curves;
    const battery_charge_mode mode;

    const double compute_min_non_zero_slope(const std::vector<line_segment>& charge_profile) const;
    const double compute_zero_slope_threashold_P2_vs_soc(const std::vector<line_segment>& charge_profile) const;

    const SOC_vs_P2 get_charging_dcfc_charge_profile(const EV_type& EV, const EVSE_type& EVSE) const;
    const SOC_vs_P2 get_discharging_dcfc_charge_profile(const EV_type& EV, const EVSE_type& EVSE) const;

public:
    create_dcPkW_from_soc(const EV_EVSE_inventory& inventory, 
        const std::map<c_rate, std::map<SOC, std::pair<power, point_type> >, std::greater<c_rate> >& curves, 
        const battery_charge_mode& mode);

    const SOC_vs_P2 get_dcfc_charge_profile(const battery_charge_mode& mode, const EV_type& EV, const EVSE_type& EVSE) const;
    const SOC_vs_P2 get_L1_or_L2_charge_profile(const EV_type& EV) const;
};

class SOC_vs_P2_factory
{
private:

	const EV_EVSE_inventory& inventory;

    const create_dcPkW_from_soc LMO_charge;
    const create_dcPkW_from_soc NMC_charge;
    const create_dcPkW_from_soc LTO_charge;

	std::vector<bat_objfun_constraints> constraints;

	const std::unordered_map<EV_type, SOC_vs_P2 > L1_L2_curves;
	const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > DCFC_curves;

    const create_dcPkW_from_soc load_LMO_charge();
    const create_dcPkW_from_soc load_NMC_charge();
    const create_dcPkW_from_soc load_LTO_charge();

	const std::unordered_map<EV_type, SOC_vs_P2 > load_L1_L2_curves();
	const std::unordered_map< std::pair<EV_type, EVSE_type>, SOC_vs_P2, pair_hash > load_DCFC_curves();

public:
    SOC_vs_P2_factory(const EV_EVSE_inventory& inventory);

	const SOC_vs_P2& get_SOC_vs_P2_curves(const EV_type& EV, const EVSE_type& EVSE) const;

    void write_charge_profile(const std::string& output_path) const;
};

#endif