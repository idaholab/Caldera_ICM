#include "EV_characteristics.h"

std::ostream& operator<<(std::ostream& os, const battery_chemistry& chemistry) {
    switch (chemistry) {
    case LTO:
        os << "LTO";
        break;
    case LMO:
        os << "LMO";
        break;
    case NMC:
        os << "NMC";
        break;
    default:
        os << "Invalid battery chemistry";
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const EV_characteristics& ev)
{
    os << "Type: " << ev.get_type() << std::endl;
    os << "Battery Chemistry: " << ev.get_chemistry() << std::endl;
    os << "DCFC capable: " << ev.get_DCFC_capable() << std::endl;
    os << "Max C Rate: " << ev.get_max_c_rate() << std::endl;
    os << "Battery Size (kWh): " << ev.get_battery_size_kWh() << std::endl;
    os << "Range (miles): " << ev.get_range_miles() << std::endl;
    os << "Efficiency (Wh/mile): " << ev.get_efficiency_Wh_per_mile() << std::endl;
    os << "AC Charge Rate (kW): " << ev.get_AC_charge_rate_kW() << std::endl;
    return os;
}
