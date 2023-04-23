#ifndef EV_CHARACTERISTICS_H
#define EV_CHARACTERISTICS_H

#include <string>
#include <iostream>

typedef std::string EV_type;

enum battery_chemistry
{
    LTO = 0,
    LMO = 1,
    NMC = 2
};

std::ostream& operator<<(std::ostream& os, const battery_chemistry& chemistry);

struct EV_characteristics
{
private:
    const EV_type type;
    const battery_chemistry chemistry;
    const double max_c_rate;
    const double battery_size_kWh;
    const double range_miles;
    const double efficiency_Wh_per_mile;
    const double AC_charge_rate_kW;

public:
    EV_characteristics(const EV_type& type, const battery_chemistry& chemistry, const double& max_c_rate, 
        const double& battery_size_kWh, const double& range_miles, const double& efficiency_Wh_per_mile, 
        const double& AC_charge_rate_kW)
        : type(type), chemistry(chemistry), max_c_rate(max_c_rate), battery_size_kWh(battery_size_kWh), 
        range_miles(range_miles), efficiency_Wh_per_mile(efficiency_Wh_per_mile), 
        AC_charge_rate_kW(AC_charge_rate_kW) {}

    EV_type             get_type()                      const { return this->type; }
    battery_chemistry   get_chemistry()                 const { return this->chemistry; }
    double              get_max_c_rate()                const { return this->max_c_rate; }
    double              get_battery_size_kWh()          const { return this->battery_size_kWh; }
    double              get_range_miles()               const { return this->range_miles; }
    double              get_efficiency_Wh_per_mile()    const { return this->efficiency_Wh_per_mile; }
    double              get_AC_charge_rate_kW()         const { return this->AC_charge_rate_kW; }
};

std::ostream& operator<<(std::ostream& os, const EV_characteristics& ev);

#endif // EV_CHARACTERISTICS_H
