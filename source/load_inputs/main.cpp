#include "load_EV_inventory.h"
#include "load_EVSE_inventory.h"
#include "load_EV_EVSE_inventory.h"


int main()
{
	load_EV_EVSE_inventory loader("C:\\Users\\CEBOM\\Documents\\repos\\Caldera_ICM\\inputs");
	EV_EVSE_inventory inv = loader.get_EV_EVSE_inventory();
	std::cout << inv << std::endl;

	return 0;
}