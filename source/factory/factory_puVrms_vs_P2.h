#ifndef FACTORY_PUVRMS_VS_P2_H
#define FACTORY_PUVRMS_VS_P2_H

#include <unordered_map>
#include <map>

#include "EV_characteristics.h"			// EV_type
#include "EVSE_characteristics.h"		// EVSE_type
#include "EV_EVSE_inventory.h"			// EV_EVSE_inventory

#include "helper.h"						// poly_segment, poly_function_of_x

typedef double puVrms;
typedef double P2;

class factory_puVrms_vs_P2
{
private:
	const EV_EVSE_inventory& inventory;

	const std::unordered_map<EVSE_level, std::map<puVrms, P2> > puVrms_vs_P2_curves;

	const std::unordered_map<EVSE_level, std::map<puVrms, P2> > load_puVrms_vs_P2_curves();

public:
	factory_puVrms_vs_P2(const EV_EVSE_inventory& inventory);

	const poly_function_of_x get_puVrms_vs_P2(const EVSE_type& EVSE, 
											  const double& SE_P2_limit_atNominalV_kW) const;
};

#endif