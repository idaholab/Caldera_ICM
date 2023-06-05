#ifndef EV_CHARACTERISTICS_H
#define EV_CHARACTERISTICS_H

#include <string>
#include <iostream>
#include <unordered_map>
#include <map>

typedef std::string EV_type;

enum battery_chemistry
{
    LTO = 0,
    LMO = 1,
    NMC = 2
};

namespace std {
	template <> struct hash<battery_chemistry> {
		int operator() (const battery_chemistry& t) const { return (int)t; }
	};
}

std::ostream& operator<<(std::ostream& os, const battery_chemistry& chemistry);

typedef double crate;
typedef double power;
typedef std::unordered_map< battery_chemistry, std::map<crate, power> > peak_power_per_crate;

struct EV_characteristics
{
private:

    const peak_power_per_crate charge_profile_peak_power_W_per_Wh;

    const EV_type type;
    const battery_chemistry chemistry;
    const double usable_battery_size_kWh;
    const double range_miles;
    const double efficiency_Wh_per_mile;
    const double AC_charge_rate_kW;
    const bool DCFC_capable;
    const double max_c_rate;
    const double pack_voltage_at_peak_power_V;
    
    const double battery_size_kWh;
    const double battery_size_Ah_1C;
    const double battery_size_with_stochastic_degradation_kWh;

    peak_power_per_crate compute_charge_profile_peak_power_W_per_Wh();
    double compute_battery_size_kWh();
    double compute_battery_size_Ah_1C();
    double compute_battery_size_with_stochastic_degradation_kWh();

public:
	EV_characteristics(const EV_type& type,
					   const battery_chemistry& chemistry,
					   const double usable_battery_size_kWh,
					   const double range_miles,
					   const double efficiency_Wh_per_mile,
					   const double AC_charge_rate_kW,
					   const bool DCFC_capable,
					   const double max_c_rate,
					   const double pack_voltage_at_peak_power_V);

    const peak_power_per_crate& get_charge_profile_peak_power_W_per_Wh() const;

    const EV_type& get_type() const;
    const battery_chemistry& get_chemistry() const;
    const double& get_usable_battery_size_kWh() const;
    const double& get_range_miles() const;
    const double& get_efficiency_Wh_per_mile() const;
    const double& get_AC_charge_rate_kW() const;
    const bool& get_DCFC_capable() const;
    const double& get_max_c_rate() const;
    const double& get_pack_voltage_at_peak_power_V() const;
    
    const double& get_battery_size_kWh() const;
    const double& get_battery_size_Ah_1C() const;
    const double& get_battery_size_with_stochastic_degradation_kWh() const;

};

std::ostream& operator<<(std::ostream& os, const EV_characteristics& ev);

#endif // EV_CHARACTERISTICS_H
