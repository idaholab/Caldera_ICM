#ifndef FACTORY_P2_VS_BATTERY_EFFICIENCY_H
#define FACTORY_P2_VS_BATTERY_EFFICIENCY_H

#include <vector>
#include <unordered_map>

#include "factory_SOC_vs_P2.h"
#include "EV_characteristics.h"
#include "EV_EVSE_inventory.h"

#include "helper.h"                    // line_segment

struct P2_vs_battery_efficiency
{
    const line_segment curve;
    const double zero_slope_threshold;
    P2_vs_battery_efficiency( const line_segment& curve, 
                              const double& zero_slope_threshold );
};

typedef std::unordered_map<EV_type, std::unordered_map< battery_charge_mode, P2_vs_battery_efficiency> > P2_vs_battery_efficiency_map;

class factory_P2_vs_battery_efficiency
{
private:
    const EV_EVSE_inventory& inventory;

    const P2_vs_battery_efficiency_map P2_vs_battery_eff;

    const P2_vs_battery_efficiency_map load_P2_vs_battery_eff();

public:
    factory_P2_vs_battery_efficiency(const EV_EVSE_inventory& inventory);

    const P2_vs_battery_efficiency& get_P2_vs_battery_eff( const EV_type& EV, 
                                                           const battery_charge_mode& mode ) const;
};
#endif