#ifndef P2_VS_BATTERY_EFFICIENCY_H
#define P2_VS_BATTERY_EFFICIENCY_H

#include <vector>
#include <unordered_map>

#include "SOC_vs_P2_factory.h"
#include "EV_characteristics.h"
#include "EV_EVSE_inventory.h"

#include "helper.h"					// line_segment

struct P2_vs_battery_efficiency
{
	const line_segment curve;
	const double zero_slope_threashold;
	P2_vs_battery_efficiency(const line_segment& curve, const double& zero_slope_threashold);
};

typedef std::unordered_map<EV_type, std::unordered_map< battery_charge_mode, P2_vs_battery_efficiency> > P2_vs_battery_efficiency_map;

class P2_vs_battery_efficiency_factory
{
private:
	const EV_EVSE_inventory& inventory;

 	const P2_vs_battery_efficiency_map P2_vs_battery_eff;

	const P2_vs_battery_efficiency_map load_P2_vs_battery_eff();

public:
	P2_vs_battery_efficiency_factory(const EV_EVSE_inventory& inventory);

	const P2_vs_battery_efficiency& get_P2_vs_battery_eff(const EV_type& EV, const battery_charge_mode& mode) const;
};
#endif