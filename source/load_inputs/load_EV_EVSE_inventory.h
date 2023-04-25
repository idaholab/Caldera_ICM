#ifndef LOAD_EV_EVSE_INVENTORY_H
#define LOAD_EV_EVSE_INVENTORY_H

#include "EV_EVSE_inventory.h"
#include "load_EV_inventory.h"
#include "load_EVSE_inventory.h"

class load_EV_EVSE_inventory
{
private:
	const EV_EVSE_inventory inventory;

public:
	load_EV_EVSE_inventory(std::string inputs_dir);

	const EV_EVSE_inventory& get_EV_EVSE_inventory() const;
};

#endif //LOAD_EV_EVSE_INVENTORY_H