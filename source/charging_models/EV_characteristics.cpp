#include "EV_characteristics.h"
#include "helper.h"

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

EV_characteristics::EV_characteristics(const EV_type& type, 
                                       const battery_chemistry& chemistry,
                                       const double usable_battery_size_kWh, 
                                       const double range_miles, 
                                       const double efficiency_Wh_per_mile,
                                       const double AC_charge_rate_kW, 
                                       const bool DCFC_capable, 
                                       const double max_c_rate,
                                       const double pack_voltage_at_peak_power_V) 
    : charge_profile_peak_power_W_per_Wh{ this->compute_charge_profile_peak_power_W_per_Wh() },
    type{ type },
    chemistry{ chemistry },
    usable_battery_size_kWh{ usable_battery_size_kWh },
    range_miles{ range_miles },
    efficiency_Wh_per_mile{ efficiency_Wh_per_mile },
    AC_charge_rate_kW{ AC_charge_rate_kW },
    DCFC_capable{ DCFC_capable },
    max_c_rate{ max_c_rate },
    pack_voltage_at_peak_power_V{ pack_voltage_at_peak_power_V },

    battery_size_kWh{ this->compute_battery_size_kWh() },
    peak_power_kW{ this->compute_peak_power() },
    battery_size_with_stochastic_degradation_kWh{ this->compute_battery_size_with_stochastic_degradation_kWh() },
    battery_size_Ah_1C{ this->compute_battery_size_Ah_1C() }
{ }

const peak_power_per_crate& EV_characteristics::get_charge_profile_peak_power_W_per_Wh() const { return this->charge_profile_peak_power_W_per_Wh; }

const EV_type& EV_characteristics::get_type() const                         { return this->type; }
const battery_chemistry& EV_characteristics::get_chemistry() const          { return this->chemistry; }
const bool& EV_characteristics::get_DCFC_capable() const                    { return this->DCFC_capable; }
const double& EV_characteristics::get_max_c_rate() const                    { return this->max_c_rate; }
const double& EV_characteristics::get_usable_battery_size_kWh() const       { return this->usable_battery_size_kWh; }
const double& EV_characteristics::get_range_miles() const                   { return this->range_miles; }
const double& EV_characteristics::get_efficiency_Wh_per_mile() const        { return this->efficiency_Wh_per_mile; }
const double& EV_characteristics::get_AC_charge_rate_kW() const             { return this->AC_charge_rate_kW; }
const double& EV_characteristics::get_pack_voltage_at_peak_power_V() const  { return this->pack_voltage_at_peak_power_V; }

const double& EV_characteristics::get_battery_size_kWh() const              { return this->battery_size_kWh; }
const double EV_characteristics::get_peak_power_kW() const                  { return this->peak_power_kW; }
const double& EV_characteristics::get_battery_size_with_stochastic_degradation_kWh() const { return this->battery_size_with_stochastic_degradation_kWh; }
const double& EV_characteristics::get_battery_size_Ah_1C() const            { return this->battery_size_Ah_1C; }

//Charge Profile Peak Power(W / Wh)
// c - rate	    LMO	    NMC	    LTO
//  0	        0	    0	    0
//  1	        1.27	1.25	1.13
//  2	        2.42	2.42	2.23
//  3	        3.63	3.75	3.36
//  4	        3.63	3.75	4.52
//  5	        3.63	3.75	5.63
//  6	        3.63	3.75	5.63

peak_power_per_crate EV_characteristics::compute_charge_profile_peak_power_W_per_Wh()
{
    peak_power_per_crate data;
    
    std::vector<crate> crate_vec = { 0, 1, 2, 3, 4, 5, 6 };
    std::vector<power> LMO_power_vec = { 0, 1.27, 2.42, 3.63, 3.63, 3.63, 3.63 };
    std::vector<power> NMC_power_vec = { 0, 1.25, 2.42, 3.75, 3.75, 3.75, 3.75 };
    std::vector<power> LTO_power_vec = { 0, 1.13, 2.23, 3.36, 4.52, 5.63, 5.63 };

    data[LMO] = [&]() {
        std::map<crate, power> m;
        for (int i = 0; i < crate_vec.size(); i++)
        {
            m.emplace(crate_vec[i], LMO_power_vec[i]);
        }
        return m;
    }();

    data[NMC] = [&]() {
        std::map<crate, power> m;
        for (int i = 0; i < crate_vec.size(); i++)
        {
            m.emplace(crate_vec[i], NMC_power_vec[i]);
        }
        return m;
    }();

    data[LTO] = [&]() {
        std::map<crate, power> m;
        for (int i = 0; i < crate_vec.size(); i++)
        {
            m.emplace(crate_vec[i], LTO_power_vec[i]);
        }
        return m;
    }();

    return std::move(data);
}

double EV_characteristics::compute_battery_size_kWh()
{
    return this->usable_battery_size_kWh / 0.95;
}

double EV_characteristics::compute_peak_power()
{
    const std::map<crate, power>& profile_peak_power_W_per_Wh = this->charge_profile_peak_power_W_per_Wh.at(this->chemistry);

    crate smallest_crate = profile_peak_power_W_per_Wh.begin()->first;
    crate largest_crate = (--profile_peak_power_W_per_Wh.end())->first;

    if ((this->max_c_rate < smallest_crate || this->max_c_rate > largest_crate) && this->max_c_rate != -1)
    {
        ASSERT(false, "invalid crate for model " << this->type << " crate needs to be between " << smallest_crate << " and " << largest_crate << " crate is " << this->max_c_rate);
    }

    if (this->max_c_rate == -1)
        return -1;

    auto it_UB = profile_peak_power_W_per_Wh.upper_bound(this->max_c_rate);
    auto it_LB = std::prev(it_UB);
    if (it_UB == profile_peak_power_W_per_Wh.end())
    {
        --it_UB;
    }

    crate crate_LB = it_LB->first;
    crate crate_UB = it_UB->first;
    double weight = (this->max_c_rate - crate_LB) / (crate_UB - crate_LB);

    power peak_power_LB = it_LB->second;
    power peak_power_UB = it_UB->second;

    power weighted_peak_power_W_per_Wh = peak_power_LB + weight * (peak_power_UB - peak_power_LB);

    return this->battery_size_kWh * weighted_peak_power_W_per_Wh;
}

double EV_characteristics::compute_battery_size_Ah_1C()
{
    double battery_size_Ah_1C = 1000 * this->peak_power_kW / (this->max_c_rate * this->pack_voltage_at_peak_power_V);

    double average_pack_voltage_V = 1000 * this->battery_size_kWh * battery_size_Ah_1C;

    return battery_size_Ah_1C;
}

double EV_characteristics::compute_battery_size_with_stochastic_degradation_kWh()
{
    if (this->chemistry == LTO)
    {
        return rand_val::rand_range(0.95, 1.0) * this->battery_size_kWh;
    }
    else if (this->chemistry == LMO)
    {
        return rand_val::rand_range(0.85, 1.0) * this->battery_size_kWh;
    }
    else if (this->chemistry == NMC)
    {
        return rand_val::rand_range(0.90, 1.0) * this->battery_size_kWh;
    }
    else
    {
        ASSERT(false, "invalid battery chemistry");
        return 0.0;
    }
}

std::ostream& operator<<(std::ostream& os, const EV_characteristics& ev)
{
    os << "Type: " << ev.get_type() << std::endl;
    os << "Battery Chemistry: " << ev.get_chemistry() << std::endl;
    os << "Usable Battery Size (kWh): " << ev.get_usable_battery_size_kWh() << std::endl;
    os << "Range (miles): " << ev.get_range_miles() << std::endl;
    os << "Efficiency (Wh/mile): " << ev.get_efficiency_Wh_per_mile() << std::endl;
    os << "AC Charge Rate (kW): " << ev.get_AC_charge_rate_kW() << std::endl;
    os << "DCFC capable: " << ev.get_DCFC_capable() << std::endl;
    os << "Max C Rate: " << ev.get_max_c_rate() << std::endl;
    os << "Pack Valtage at Peak Power (V): " << ev.get_pack_voltage_at_peak_power_V() << std::endl;
    os << "Battery Size (kWh): " << ev.get_battery_size_kWh() << std::endl;
    os << "Battery Size (Ah): " << ev.get_battery_size_Ah_1C() << std::endl;

    return os;
}
