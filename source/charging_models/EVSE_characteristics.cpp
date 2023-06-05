#include "EVSE_characteristics.h"

std::ostream& operator<<(std::ostream& os, const EVSE_level& level) {
    switch(level) {
        case L1:
            os << "L1";
            break;
        case L2:
            os << "L2";
            break;
        case DCFC:
            os << "DCFC";
            break;
        default:
            os << "Invalid EVSE_level";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const EVSE_phase& phase) {
    switch (phase) {
        case singlephase:
            os << "singlephase";
            break;
        case threephase:
            os << "threephase";
            break;
        default:
            os << "Invalid EVSE_phase";
            break;
    }
    return os;
}

EVSE_characteristics::EVSE_characteristics(const EVSE_type& type,
										   const EVSE_level& level,
										   const EVSE_phase& phase,
										   const double power_limit_kW,
										   const double volatge_limit_V,
										   const double current_limit_A,
										   const double standby_real_power_kW,
										   const double standby_reactive_power_kW)
	: type{ type },
	level{ level },
	phase{ phase },
	power_limit_kW{ power_limit_kW },
	volatge_limit_V{ volatge_limit_V },
	current_limit_A{ current_limit_A },
	standby_real_power_kW{ standby_real_power_kW },
	standby_reactive_power_kW{ standby_reactive_power_kW }
{
}

std::ostream& operator<<(std::ostream& os, const EVSE_characteristics& evse)
{
    os << "Type: " << evse.get_type() << std::endl;
    os << "Level: " << evse.get_level() << std::endl;
    os << "Phase: " << evse.get_phase() << std::endl;
    os << "Power Limit (kW): " << evse.get_power_limit_kW() << std::endl;
    os << "Voltage Limit (V): " << evse.get_volatge_limit_V() << std::endl;
    os << "Current Limit (A): " << evse.get_current_limit_A() << std::endl;
    os << "Standby Real Power (kW): " << evse.get_standby_real_power_kW() << std::endl;
    os << "Standby Reactive Power (kW): " << evse.get_standby_reactive_power_kW() << std::endl;

    return os;
}
