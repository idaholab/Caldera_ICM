#ifndef FACTORY_AC_TO_DC_CONVERTER_H
#define FACTORY_AC_TO_DC_CONVERTER_H

#include "ac_to_dc_converter.h"
#include "EV_characteristics.h"
#include "EVSE_characteristics.h"
#include "EV_EVSE_inventory.h"

//#############################################################################
//                      AC to DC Converter Factory
//#############################################################################

class factory_ac_to_dc_converter
{
private:
    const EV_EVSE_inventory& inventory;

public:
    factory_ac_to_dc_converter(const EV_EVSE_inventory& inventory) 
        : inventory{ inventory }
    {
    }
    ac_to_dc_converter* alloc_get_ac_to_dc_converter(
        ac_to_dc_converter_enum converter_type, 
        EVSE_type EVSE, 
        EV_type EV, 
        charge_event_P3kW_limits& CE_P3kW_limits
    ) const;
};

#endif  // FACTORY_AC_TO_DC_CONVERTER_H