#ifndef EV_EVSE_INVENTORY_H
#define EV_EVSE_INVENTORY_H

#include <unordered_map>
#include "EV_characteristics.h"
#include "SE_characteristics.h"

class EV_EVSE_inventory {
private:
    const std::unordered_map<EV_type, EV_characteristics> EV_inventory;
    const std::unordered_map<SE_type, SE_characteristics> EVSE_inventory;

public:
    EV_EVSE_inventory(const std::unordered_map<EV_type, EV_characteristics>& EV_inv,
                      const std::unordered_map<SE_type, SE_characteristics>& EVSE_inv);

    const std::unordered_map<EV_type, EV_characteristics>& get_EV_inventory() const;
    const std::unordered_map<SE_type, SE_characteristics>& get_EVSE_inventory() const;
};

std::ostream& operator<<(std::ostream& os, const EV_EVSE_inventory& inventory);

#endif //EV_EVSE_INVENTORY_H
