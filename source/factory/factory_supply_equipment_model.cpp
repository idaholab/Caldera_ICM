#include "factory_supply_equipment_model.h"

//#############################################################################
//                Supply Equipment Charge Model Factory
//#############################################################################

factory_supply_equipment_model::factory_supply_equipment_model(const EV_EVSE_inventory& inventory)
    : inventory{ inventory },
    CE_queuing_inputs{ }
{
}

factory_supply_equipment_model::factory_supply_equipment_model(const EV_EVSE_inventory& inventory, 
                                                               const charge_event_queuing_inputs& CE_queuing_inputs_)
    : inventory{ inventory },
    CE_queuing_inputs{ CE_queuing_inputs_ }
{
}


supply_equipment factory_supply_equipment_model::get_supply_equipment_model(
    bool building_charge_profile_library,
    const SE_configuration& SE_config, 
    const get_base_load_forecast& baseLD_forecaster,
    manage_L2_control_strategy_parameters* manage_L2_control
)
{
    EVSE_type EVSE = SE_config.supply_equipment_type;

    double P2_limit_kW = this->inventory.get_EVSE_inventory().at(EVSE).get_power_limit_kW();
    double standby_acP_kW = this->inventory.get_EVSE_inventory().at(EVSE).get_standby_real_power_kW();
    double standby_acQ_kVAR = this->inventory.get_EVSE_inventory().at(EVSE).get_standby_reactive_power_kW();

    //============================

    supply_equipment_load load(P2_limit_kW, standby_acP_kW, standby_acQ_kVAR, SE_config, this->CE_queuing_inputs);
    supply_equipment_control control(building_charge_profile_library, SE_config, baseLD_forecaster, manage_L2_control);
    
    return supply_equipment{ SE_config, control, load };
}
