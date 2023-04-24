#ifndef EV_EVSE_INVENTORY_H
#define EV_EVSE_INVENTORY_H

#include <unordered_map>
#include "EV_characteristics.h"
#include "EVSE_characteristics.h"

typedef std::unordered_map<EV_type, EV_characteristics> EV_inventory;
typedef std::unordered_map<EVSE_type, EVSE_characteristics> EVSE_inventory;

class EV_EVSE_inventory {
private:
    const EV_inventory EV_inv;
    const EVSE_inventory EVSE_inv;

public:
    EV_EVSE_inventory(const EV_inventory& EV_inv,
                      const EVSE_inventory& EVSE_inv);

    const EV_inventory& get_EV_inventory() const;
    const EVSE_inventory& get_EVSE_inventory() const;
};

std::ostream& operator<<(std::ostream& os, const EV_EVSE_inventory& inventory);
std::ostream& operator<<(std::ostream& os, const EV_inventory& inventory);
std::ostream& operator<<(std::ostream& os, const EVSE_inventory& inventory);

#endif //EV_EVSE_INVENTORY_H
