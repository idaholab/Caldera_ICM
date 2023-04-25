#ifndef LOAD_EVSE_MODELS_H
#define LOAD_EVSE_MODELS_H

#include "EV_EVSE_inventory.h"

class load_EVSE_models
{
private:
	const EVSE_inventory EVSE_inv;

	EVSE_level string_to_EVSE_level(const std::string& str);
	EVSE_phase string_to_EVSE_phase(const std::string& str);
	
	EVSE_inventory load(std::string inputs_dir);

public:

	load_EVSE_models(std::string inputs_dir) :
		EVSE_inv(this->load(inputs_dir)) {}

	const EVSE_inventory& get_EVSE_inventory() const { return this->EVSE_inv; }
};

#endif // LOAD_EV_MODELS_H