#include "EV_EVSE_inventory.h"

EV_EVSE_inventory::EV_EVSE_inventory(const std::unordered_map<EV_type, EV_characteristics>& EV_inv,
                                     const std::unordered_map<SE_type, SE_characteristics>& EVSE_inv)
        : EV_inventory(EV_inv), EVSE_inventory(EVSE_inv) {}

const std::unordered_map<EV_type, EV_characteristics>& EV_EVSE_inventory::get_EV_inventory() const {
    return EV_inventory;
}

const std::unordered_map<SE_type, SE_characteristics>& EV_EVSE_inventory::get_EVSE_inventory() const {
    return EVSE_inventory;
}

std::ostream& operator<<(std::ostream& os, const EV_EVSE_inventory& inventory) {
    os << "Electric Vehicles Inventory: " << std::endl;
    for (const auto& ev : inventory.get_EV_inventory()) {
        os << ev.second << std::endl;
    }
    os << "Electric Vehicle Supply Equipment Inventory: " << std::endl;
    for (const auto& evse : inventory.get_EVSE_inventory()) {
        os << evse.second << std::endl;
    }
    return os;
}
