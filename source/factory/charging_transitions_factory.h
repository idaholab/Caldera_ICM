#ifndef CHARGING_TRANSITIONS_FACTORY_H
#define CHARGING_TRANSITIONS_FACTORY_H

#include <unordered_map>

#include "EV_characteristics.h"
#include "EVSE_characteristics.h"
#include "EV_EVSE_inventory.h"

#include "datatypes_global.h"					// pev_charge_ramping
#include "battery_integrate_X_in_time.h"		// integrate_X_through_time
#include "helper.h"

typedef std::unordered_map<EV_type, pev_charge_ramping> EV_ramping_map;
typedef std::unordered_map<std::pair<EV_type, EVSE_type>, pev_charge_ramping, pair_hash> EV_EVSE_ramping_map;

typedef std::unordered_map<EVSE_level, integrate_X_through_time> EVSE_level_charging_transitions;
typedef std::unordered_map<EV_type, integrate_X_through_time> EV_charging_transitions;
typedef std::unordered_map<std::pair<EV_type, EVSE_type>, integrate_X_through_time, pair_hash> EV_EVSE_charging_transitions;

struct charging_transitions
{
	const EVSE_level_charging_transitions charging_transitions_by_EVSE_level;
	const EV_charging_transitions charging_transitions_by_custom_EV;
	const EV_EVSE_charging_transitions charging_transitions_by_custom_EV_EVSE;

	charging_transitions(const EVSE_level_charging_transitions& charging_transitions_by_EVSE_level,
		const EV_charging_transitions& charging_transitions_by_custom_EV,
		const EV_EVSE_charging_transitions& charging_transitions_by_custom_EV_EVSE);
};


class charging_transitions_factory
{
private:
	
	const EV_EVSE_inventory& inventory;

	const EV_ramping_map custom_EV_ramping;
	const EV_EVSE_ramping_map custom_EV_EVSE_ramping;

	const charging_transitions charging_transitions_obj;

	const charging_transitions load_charging_transitions();


public:
	charging_transitions_factory(const EV_EVSE_inventory& inventory, const EV_ramping_map& custom_EV_ramping, const EV_EVSE_ramping_map& custom_EV_EVSE_ramping);

	const integrate_X_through_time& get_charging_transitions(const EV_type& EV, const EVSE_type& EVSE) const;
};

#endif