#ifndef EVSE_CHARACTERISTICS_H
#define EVSE_CHARACTERISTICS_H

#include <string>
#include <iostream>

typedef std::string EVSE_type;

enum EVSE_level
{
    L1 = 0, 
    L2 = 1, 
    DCFC = 2
};

std::ostream& operator<<(std::ostream& os, const EVSE_level& level);

enum EVSE_phase
{
    singlephase = 0, 
    threephase = 1
};

std::ostream& operator<<(std::ostream& os, const EVSE_phase& phase);

struct EVSE_characteristics
{
private:
    const EVSE_type type;
    const EVSE_level level;
    const EVSE_phase phase;
    const double power_limit_kW;
    const double volatge_limit_V;
    const double current_limit_A;
    const double standby_real_power_kW;
    const double standby_reactive_power_kW;

public:
    EVSE_characteristics(const EVSE_type& type, const EVSE_level& level, const EVSE_phase& phase,
        const double& power_limit_kW, const double& volatge_limit_V, const double& current_limit_A,
        const double& standby_real_power_kW, const double& standby_reactive_power_kW)
        : type(type), level(level), phase(phase), power_limit_kW(power_limit_kW), volatge_limit_V(volatge_limit_V),
        current_limit_A(current_limit_A), standby_real_power_kW(standby_real_power_kW),
        standby_reactive_power_kW(standby_reactive_power_kW) {}

    EVSE_type           get_type()                      const { return this->type; }
    EVSE_level          get_level()                     const { return this->level; }
    EVSE_phase          get_phase()                     const { return this->phase; }
    double              get_power_limit_kW()            const { return this->power_limit_kW; }
    double              get_volatge_limit_V()           const { return this->volatge_limit_V; }
    double              get_current_limit_A()           const { return this->current_limit_A; }
    double              get_standby_real_power_kW()     const { return this->standby_real_power_kW; }
    double              get_standby_reactive_power_kW() const { return this->standby_reactive_power_kW; }
};

std::ostream& operator<<(std::ostream& os, const EVSE_characteristics& evse);

#endif // EVSE_CHARACTERISTICS_H
