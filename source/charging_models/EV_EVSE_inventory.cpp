#include "EV_EVSE_inventory.h"

EV_EVSE_inventory::EV_EVSE_inventory(const EV_inventory& EV_inv,
                                     const EVSE_inventory& EVSE_inv)
        : EV_inv(EV_inv), EVSE_inv(EVSE_inv) {}

const EV_inventory& EV_EVSE_inventory::get_EV_inventory() const 
{
    return this->EV_inv;
}

const EVSE_inventory& EV_EVSE_inventory::get_EVSE_inventory() const 
{
    return this->EVSE_inv;
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
