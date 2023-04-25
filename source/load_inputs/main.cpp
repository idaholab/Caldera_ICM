#include "load_EV_models.h"
#include "load_EVSE_models.h"


int main()
{
	load_EVSE_models loader("C:\\Users\\CEBOM\\Documents\\repos\\Caldera_ICM\\inputs");
	EVSE_inventory inv = loader.get_EVSE_inventory();
	std::cout << inv << std::endl;

	return 0;
}