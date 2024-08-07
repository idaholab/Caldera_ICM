#ifndef LOAD_EVSE_INVENTORY_H
#define LOAD_EVSE_INVENTORY_H

#include "EV_EVSE_inventory.h"

class load_EVSE_inventory
{
private:
	const EVSE_inventory EVSE_inv;

	const EVSE_level string_to_EVSE_level(const std::string& str) const;
	const EVSE_phase string_to_EVSE_phase(const std::string& str) const;

	EVSE_inventory load(const std::string& inputs_dir);

public:
	load_EVSE_inventory(const std::string& inputs_dir);
	const EVSE_inventory& get_EVSE_inventory() const;
};

#endif // LOAD_EVSE_INVENTORY_H