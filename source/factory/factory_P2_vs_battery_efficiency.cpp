#include "factory_P2_vs_battery_efficiency.h"
#include "helper.h"

//##################################################
//              P2_vs_battery_efficiency
//##################################################

P2_vs_battery_efficiency::P2_vs_battery_efficiency(const line_segment& curve, 
                                                   const double& zero_slope_threshold) 
    : curve{ curve },
    zero_slope_threshold{ zero_slope_threshold }
{
}


//##################################################
//              factory_P2_vs_battery_efficiency
//##################################################

factory_P2_vs_battery_efficiency::factory_P2_vs_battery_efficiency(const EV_EVSE_inventory& inventory) 
    : inventory{ inventory }, 
    P2_vs_battery_eff{ this->load_P2_vs_battery_eff() }
{
}

const P2_vs_battery_efficiency_map factory_P2_vs_battery_efficiency::load_P2_vs_battery_eff()
{    
    P2_vs_battery_efficiency_map outer_map;   

    EV_type EV;
    battery_chemistry chemistry;
    battery_charge_mode mode;
    double battery_size_kWh;
    double zero_slope_threshold;

    for (const auto& EVs : this->inventory.get_EV_inventory())
    {
        std::unordered_map< battery_charge_mode, P2_vs_battery_efficiency> inner_map;

        EV = EVs.first;
        chemistry = EVs.second.get_chemistry();
        battery_size_kWh = EVs.second.get_battery_size_kWh();

        if (chemistry == LTO)
        {
            mode = charging;
            line_segment curve1{ 0, 6 * battery_size_kWh, -0.0078354 / battery_size_kWh, 0.987448 };
            zero_slope_threshold = (std::abs(curve1.a) < 0.000001) ? 0.000001 : 0.9 * std::abs(curve1.a);              // If the slope is smaller than 0.000001 that the 'safe' method will be used. // Very little rational to using 0.000001 it will allow the complex method using a 1000 kWh battery pack
            inner_map.emplace(mode, P2_vs_battery_efficiency{ curve1, zero_slope_threshold });

            //-----------------------
            mode = discharging;
            line_segment curve2{ -6 * battery_size_kWh, 0, -0.0102411 / battery_size_kWh, 1.0109224 };
            zero_slope_threshold = (std::abs(curve2.a) < 0.000001) ? 0.000001 : 0.9 * std::abs(curve2.a);

            inner_map.emplace(mode, P2_vs_battery_efficiency{ curve2, zero_slope_threshold });
        }
        else if (chemistry == LMO)
        {
            mode = charging;
            line_segment curve1{ 0, 4 * battery_size_kWh, -0.0079286 / battery_size_kWh, 0.9936637 };
            zero_slope_threshold = (std::abs(curve1.a) < 0.000001) ? 0.000001 : 0.9 * std::abs(curve1.a);

            inner_map.emplace(mode, P2_vs_battery_efficiency{ curve1, zero_slope_threshold });

            //-----------------------
            mode = discharging;
            line_segment curve2{ -4 * battery_size_kWh, 0, -0.0092091 / battery_size_kWh, 1.005674 };
            zero_slope_threshold = (std::abs(curve2.a) < 0.000001) ? 0.000001 : 0.9 * std::abs(curve2.a);

            inner_map.emplace(mode, P2_vs_battery_efficiency{ curve2, zero_slope_threshold });
        }
        else if (chemistry == NMC)
        {
            mode = charging;
            line_segment curve1{ 0, 4 * battery_size_kWh, -0.0053897 / battery_size_kWh, 0.9908405 };
            zero_slope_threshold = (std::abs(curve1.a) < 0.000001) ? 0.000001 : 0.9 * std::abs(curve1.a);

            inner_map.emplace(mode, P2_vs_battery_efficiency{ curve1, zero_slope_threshold });

            //----------------------
            mode = discharging;
            line_segment curve2{ -4 * battery_size_kWh, 0, -0.0062339 / battery_size_kWh, 1.0088727 };
            zero_slope_threshold = (std::abs(curve2.a) < 0.000001) ? 0.000001 : 0.9 * std::abs(curve2.a);

            inner_map.emplace(mode, P2_vs_battery_efficiency{ curve2, zero_slope_threshold });
        }
        else
        {
            ASSERT(false, "battery chemistry not valid");
        }
        outer_map.emplace(EV, inner_map);
    }
    return outer_map;
}

const P2_vs_battery_efficiency& factory_P2_vs_battery_efficiency::get_P2_vs_battery_eff(const EV_type& EV, 
                                                                                        const battery_charge_mode& mode) const
{
    return this->P2_vs_battery_eff.at(EV).at(mode);
}