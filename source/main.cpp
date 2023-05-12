#include <iostream>
#include "EV_charge_model_factory.h"
#include "load_EV_EVSE_inventory.h"

int main()
{
	load_EV_EVSE_inventory load_inventory{ "../inputs" };
	const EV_EVSE_inventory& inventory = load_inventory.get_EV_EVSE_inventory();

	std::cout << inventory << std::endl;
	EV_ramping_map custom_EV_ramping;
	EV_EVSE_ramping_map custom_EV_EVSE_ramping;
	bool model_stochastic_battery_degregation = false;

	EV_charge_model_factory charge_model_factory{ inventory, custom_EV_ramping, custom_EV_EVSE_ramping, model_stochastic_battery_degregation };

	charge_model_factory.write_charge_profile("./");

	return 0;
}