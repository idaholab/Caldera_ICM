#ifndef PUVRMS_VS_P2_FACTORY_H
#define PUVRMS_VS_P2_FACTORY_H

#include <unordered_map>
#include <map>

#include "EV_characteristics.h"
#include "EVSE_characteristics.h"
#include "EV_EVSE_inventory.h"

#include "helper.h"					// poly_segment, poly_function_of_x

typedef double puVrms;
typedef double P2;

class puVrms_vs_P2_factory
{
private:
	const EV_EVSE_inventory& inventory;

	const std::unordered_map<EVSE_level, std::map<puVrms, P2> > puVrms_vs_P2_curves;

	const std::unordered_map<EVSE_level, std::map<puVrms, P2> > load_puVrms_vs_P2_curves();

public:
	puVrms_vs_P2_factory(const EV_EVSE_inventory& inventory);

	const poly_function_of_x get_puVrms_vs_P2(const EVSE_type& EVSE, const double& SE_P2_limit_atNominalV_kW) const;

};

#endif