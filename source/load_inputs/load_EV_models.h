#ifndef LOAD_EV_MODELS_H
#define LOAD_EV_MODELS_H

#include "EV_EVSE_inventory.h"
#include "EV_characteristics.h"

#include <string>

class load_EV_models
{
private:
	const EV_inventory EV_inv;

	battery_chemistry string_to_battery_chemistry(const std::string& str);
	bool string_to_DCFC_capable(const std::string& str);

	EV_inventory load(std::string inputs_dir);

public:

	load_EV_models(std::string inputs_dir) :
		EV_inv(this->load(inputs_dir)) {}

	const EV_inventory& get_EV_inventory() const { return this->EV_inv; }
};

#endif // LOAD_EV_MODELS_H