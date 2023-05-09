#include <iostream>
#include "pev_charge_profile_factory.h"

int main()
{
	pev_charge_profile_factory factory_obj;
	std::vector<line_segment> segs = factory_obj.get_dcfc_charge_profile(NMC, 92.1053, 126.808, 2.64, 100000, 125);
	return 0;
}