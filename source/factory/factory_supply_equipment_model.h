#ifndef FACTORY_SUPPLY_EQUIPMENT_MODEL_H
#define FACTORY_SUPPLY_EQUIPMENT_MODEL_H

#include "datatypes_global.h"
#include "helper.h"
#include "supply_equipment_load.h"
#include "supply_equipment_control.h"
#include "supply_equipment.h"

#include "EV_EVSE_inventory.h"

//#############################################################################
//                Supply Equipment Charge Model Factory
//#############################################################################

class factory_supply_equipment_model
{
private:
    const EV_EVSE_inventory& inventory;

    charge_event_queuing_inputs CE_queuing_inputs;

public:
    factory_supply_equipment_model(const EV_EVSE_inventory& inventory) : inventory{ inventory } {}
    factory_supply_equipment_model(const EV_EVSE_inventory& inventory, const charge_event_queuing_inputs& CE_queuing_inputs_);

    void get_supply_equipment_model(bool building_charge_profile_library, 
                                    const SE_configuration& SE_config, 
                                    get_base_load_forecast* baseLD_forecaster, 
                                    manage_L2_control_strategy_parameters* manage_L2_control, 
                                    supply_equipment& return_val);
};

#endif  // FACTORY_SUPPLY_EQUIPMENT_MODEL_H