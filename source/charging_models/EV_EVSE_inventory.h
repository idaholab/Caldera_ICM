#ifndef EV_EVSE_INVENTORY_H
#define EV_EVSE_INVENTORY_H

#include <unordered_map>
#include "EV_characteristics.h"
#include "EVSE_characteristics.h"

typedef std::unordered_map<EV_type, EV_characteristics> EV_inventory;
typedef std::unordered_map<EVSE_type, EVSE_characteristics> EVSE_inventory;

struct pev_SE_pair
{
    EV_type ev_type;
    EVSE_type se_type;

    pev_SE_pair() {}
    pev_SE_pair(const EV_type& ev_type,
                const EVSE_type& se_type)
        : ev_type(ev_type),
        se_type(se_type) { }
};

class EV_EVSE_inventory {
private:
    const EV_inventory EV_inv;
    const EVSE_inventory EVSE_inv;

    const std::vector<EV_type> all_EVs;
    const std::vector<EVSE_type> all_EVSEs;

    const EV_type default_EV;
    const EVSE_type default_EVSE;

    const std::vector<pev_SE_pair> compatible_EV_EVSE_pair;

    const std::vector<EV_type> load_all_EVs() const;
    const std::vector<EVSE_type> load_all_EVSEs() const;
    const EV_type load_default_EV() const;
    const EVSE_type load_default_EVSE() const;
    const std::vector<pev_SE_pair> EV_EVSE_inventory::load_compatible_EV_EVSE_pair() const;

public:
    EV_EVSE_inventory(const EV_inventory& EV_inv,
                      const EVSE_inventory& EVSE_inv);

    const EV_inventory& get_EV_inventory() const;
    const EVSE_inventory& get_EVSE_inventory() const;
    
    const std::vector<EV_type>& get_all_EVs() const;
    const std::vector<EVSE_type>& get_all_EVSEs() const;

    const EV_type& get_default_EV() const;
    const EVSE_type& get_default_EVSE() const;

    const std::vector<pev_SE_pair>& get_all_compatible_pev_SE_combinations() const;
};

std::ostream& operator<<(std::ostream& os, const EV_EVSE_inventory& inventory);
std::ostream& operator<<(std::ostream& os, const EV_inventory& inventory);
std::ostream& operator<<(std::ostream& os, const EVSE_inventory& inventory);



#endif //EV_EVSE_INVENTORY_H
