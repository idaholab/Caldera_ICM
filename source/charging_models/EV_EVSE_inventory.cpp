#include "EV_EVSE_inventory.h"

EV_EVSE_inventory::EV_EVSE_inventory(const EV_inventory& EV_inv,
                                     const EVSE_inventory& EVSE_inv)
    : EV_inv{ EV_inv },
    EVSE_inv{ EVSE_inv },
    all_EVs{ load_all_EVs() },
    all_EVSEs{ load_all_EVSEs() },
    default_EV{ load_default_EV() },
    default_EVSE{ load_default_EVSE() },
    compatible_EV_EVSE_pair{ load_compatible_EV_EVSE_pair() }
{
}

const std::vector<EV_type> EV_EVSE_inventory::load_all_EVs() const 
{
    std::vector<EV_type> return_val;
    for (const auto& EVs : this->get_EV_inventory())
    {
        return_val.push_back(EVs.first);
    }
    return return_val;
}

const std::vector<EVSE_type> EV_EVSE_inventory::load_all_EVSEs() const
{
    std::vector<EVSE_type> return_val;
    for (const auto& EVSEs : this->get_EVSE_inventory())
    {
        return_val.push_back(EVSEs.first);
    }
    return return_val;
}

const EV_type EV_EVSE_inventory::load_default_EV() const
{
    return this->get_EV_inventory().begin()->first;
}

const EVSE_type EV_EVSE_inventory::load_default_EVSE() const
{
    return this->get_EVSE_inventory().begin()->first;
}

const std::vector<pev_SE_pair> EV_EVSE_inventory::load_compatible_EV_EVSE_pair() const
{
    std::vector<pev_SE_pair> return_val;
    
    for (const auto& EVs : this->get_EV_inventory())
    {
        for (const auto& EVSEs : this->get_EVSE_inventory())
        {
            if (EVSEs.second.get_level() == DCFC)
            {
                if (EVs.second.get_DCFC_capable() == true)
                {
                    return_val.emplace_back(EVs.first, EVSEs.first);
                }
                else    // Non DCFC capable vehicle
                {
                    continue;   // This combination is not valid
                }
            }
            else   // L1 or L2, almost all vehicles can charge at L1 or L2
            {
                return_val.emplace_back(EVs.first, EVSEs.first);
            }
        }
    }
    return return_val;
}


const EV_inventory& EV_EVSE_inventory::get_EV_inventory() const 
{
    return this->EV_inv;
}

const EVSE_inventory& EV_EVSE_inventory::get_EVSE_inventory() const 
{
    return this->EVSE_inv;
}

const std::vector<EV_type>& EV_EVSE_inventory::get_all_EVs() const
{
    return this->all_EVs;
}

const std::vector<EVSE_type>& EV_EVSE_inventory::get_all_EVSEs() const
{
    return this->all_EVSEs;
}

const EV_type& EV_EVSE_inventory::get_default_EV() const
{
    return this->default_EV;
}

const EVSE_type& EV_EVSE_inventory::get_default_EVSE() const
{
    return this->default_EVSE;
}

const std::vector<pev_SE_pair>& EV_EVSE_inventory::get_all_compatible_pev_SE_combinations() const
{
    return this->compatible_EV_EVSE_pair;
}

const bool EV_EVSE_inventory::pev_is_compatible_with_supply_equipment(const pev_SE_pair& EV_EVSE_combination) const
{
    auto it = std::find(this->compatible_EV_EVSE_pair.begin(), this->compatible_EV_EVSE_pair.end(), EV_EVSE_combination);

    if (it == this->compatible_EV_EVSE_pair.end())
    {
        return false;
    }
    else
    {
        return true;
    }
}


std::ostream& operator<<(std::ostream& os, const EV_EVSE_inventory& inventory) 
{
    os << inventory.get_EV_inventory();
    os << inventory.get_EVSE_inventory();
    return os;
}

std::ostream& operator<<(std::ostream& os, const EV_inventory& inventory) 
{
    os << "Electric Vehicles Inventory: " << std::endl;
    for (const auto& ev : inventory) {
        os << ev.second << std::endl;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const EVSE_inventory& inventory) 
{
    os << "Electric Vehicle Supply Equipment Inventory: " << std::endl;
    for (const auto& evse : inventory) {
        os << evse.second << std::endl;
    }
    return os;
}
