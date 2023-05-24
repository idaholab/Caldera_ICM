#include "load_EV_EVSE_inventory.h"

load_EV_EVSE_inventory::load_EV_EVSE_inventory(const std::string& inputs_dir) :
    inventory(EV_EVSE_inventory(load_EV_inventory(inputs_dir).get_EV_inventory(),
        load_EVSE_inventory(inputs_dir).get_EVSE_inventory())) {}

const EV_EVSE_inventory& load_EV_EVSE_inventory::get_EV_EVSE_inventory() const 
{ 
    return this->inventory; 
}