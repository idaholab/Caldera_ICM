#ifndef LOAD_EV_INVENTORY_H
#define LOAD_EV_INVENTORY_H

#include "EV_EVSE_inventory.h"
#include "EV_characteristics.h"
#include <string>

class load_EV_inventory
{
private:
	const EV_inventory EV_inv;

	battery_chemistry string_to_battery_chemistry(const std::string& str);
	bool string_to_DCFC_capable(const std::string& str);

	EV_inventory load(std::string inputs_dir);

public:
	load_EV_inventory(std::string inputs_dir);
	const EV_inventory& get_EV_inventory() const;
};

#endif // LOAD_EV_INVENTORY_H