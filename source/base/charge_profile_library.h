
#ifndef inl_pev_charge_profile_H
#define inl_pev_charge_profile_H

#include "datatypes_global.h"
#include "datatypes_module.h"
#include "helper.h"

#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>      // int64_t
#include <utility>      // pair

#include "EV_characteristics.h"
#include "EVSE_characteristics.h"
#include "EV_EVSE_inventory.h"

void search_vector_of_doubles( const double search_value,
                               const std::vector<double>& search_vector,
                               int& LB_index,
                               int& UB_index );

pev_charge_profile_result get_default_charge_profile_result();


class pev_charge_profile_aux
{
private:
    EV_type pev_type;
    EVSE_type SE_type;
    double setpoint_P3kW;
    
    std::vector<pev_charge_fragment> charge_fragments;
    std::vector<double> soc_search; 
    std::vector<double> charge_time_search;
    
    pev_charge_fragment get_last_chargeFragment() const;
    
    pev_charge_fragment get_chargeFragment( const bool search_value_is_soc_not_timeHrs,
                                            const double search_value ) const;
    
    pev_charge_fragment calculate_charge_fragment_weighted_average( const double w,
                                                                    const pev_charge_fragment& LB,
                                                                    const pev_charge_fragment& UB ) const;
    
    pev_charge_profile_result  get_pev_charge_profile_result( const pev_charge_fragment& start,
                                                              const pev_charge_fragment& end ) const;
    
    void get_chargeProfile_from_cumulativeChargeProfile( std::vector<pev_charge_profile_result>& charge_profile,
                                                         const std::vector<pev_charge_profile_result>& cumulative_charge_profile ) const;

    void find_chargeProfile( const double startSOC,
                             const bool values_are_soc_not_timeHrs,
                             const std::vector<double>& values,
                             std::vector<pev_charge_profile_result>& charge_profile,
                             std::vector<pev_charge_profile_result>& cumulative_charge_profile ) const;
    
public:
    pev_charge_profile_aux( const EV_type pev_type_,
                            const EVSE_type SE_type_,
                            const double setpoint_P3kW_,
                            const std::vector<pev_charge_fragment>& charge_fragments_ );
    
    double get_setpoint_P3kW() const;
    
    pev_charge_profile_result find_result_given_startSOC_and_endSOC( const double startSOC, const double endSOC ) const;

    pev_charge_profile_result find_result_given_startSOC_and_chargeTime( const double startSOC, const double charge_time_hrs ) const;
    
    void find_chargeProfile_given_startSOC_and_endSOCs( const double startSOC,
                                                        const std::vector<double>& endSOC,
                                                        std::vector<pev_charge_profile_result>& charge_profile ) const;

    void find_chargeProfile_given_startSOC_and_chargeTimes( const double startSOC,
                                                            const std::vector<double>& charge_time_hrs,
                                                            std::vector<pev_charge_profile_result>& charge_profile ) const;
    
    bool operator<(const pev_charge_profile_aux& x) const
    {
        return this->setpoint_P3kW < x.setpoint_P3kW;
    }

    bool operator<(pev_charge_profile_aux& x) const
    {
        return this->setpoint_P3kW < x.setpoint_P3kW;
    }
};


class pev_charge_profile
{
private:

    EV_type pev_type; 
    EVSE_type SE_type;
    charge_event_P3kW_limits CE_P3kW_limits;
    
    std::vector<pev_charge_profile_aux> charge_profiles;
    std::vector<double> setpoint_P3kW_search;
    
    void update_UB_LB_profiles(double setpoint_P3kW);
    
public:
    pev_charge_profile() {};
    pev_charge_profile( const EV_type pev_type_,
                        const EVSE_type SE_type_,
                        const charge_event_P3kW_limits& CE_P3kW_limits_,
                        const std::vector<pev_charge_profile_aux>& charge_profiles_ );
    
    charge_event_P3kW_limits get_charge_event_P3kW_limits() const;
    std::vector<double> get_P3kW_setpoints_of_charge_profiles() const;
    
    pev_charge_profile_result find_result_given_startSOC_and_endSOC( 
        const double setpoint_P3kW,
        const double startSOC,
        const double endSOC 
    ) const;

    pev_charge_profile_result find_result_given_startSOC_and_chargeTime( 
        const double setpoint_P3kW,
        const double startSOC,
        const double charge_time_hrs 
    ) const;
    
    void find_chargeProfile_given_startSOC_and_endSOCs( 
        const double setpoint_P3kW,
        const double startSOC,
        const std::vector<double>& endSOC,
        std::vector<pev_charge_profile_result>& charge_profile 
    ) const;
    
    void find_chargeProfile_given_startSOC_and_chargeTimes( 
        const double setpoint_P3kW,
        const double startSOC,
        const std::vector<double>& charge_time_hrs,
        std::vector<pev_charge_profile_result>& charge_profile 
    ) const;
};


class pev_charge_profile_library
{
private:
    // Using map because there is no specialization of std::hash for std::pair in the c++ standard library.
    std::map< std::pair<EV_type, EVSE_type>, pev_charge_profile> charge_profile;
    pev_charge_profile default_profile;
    
    const EV_EVSE_inventory& inventory;

public:
    pev_charge_profile_library( const EV_EVSE_inventory& inventory );
    
    void add_charge_profile_to_library( const EV_type pev_type,
                                        const EVSE_type SE_type,
                                        const pev_charge_profile& charge_profile );

    const pev_charge_profile& get_charge_profile(
        const EV_type pev_type,
        const EVSE_type SE_type
    ) const;

    bool has_charge_profile( const EV_type pev_type, const EVSE_type SE_type) const
    {
        std::pair<EV_type, EVSE_type> key = {pev_type, SE_type};
        return this->charge_profile.find(key) != this->charge_profile.end(); 
    }
};



class pev_charge_profile_library_v2
{
private:
    struct charge_profile_lib_data
    {
        std::vector<ac_power_metrics> PkW_profile;
        std::vector<double> soc;
        double timestep_sec;
    };

    const EV_EVSE_inventory& inventory;
    
    // C-rate levels that will allow for power-moderation algorithms to work
    // (such as, for example, temperature-aware profiles).
    const std::vector<double> c_rate_scale_factor_levels;
    
    // Key:  std::pair<EV_type, EVSE_type>
    // Value:  A map, with
    //     key: Index into the 'c_rate_scale_factor_levels' array.
    //     value:  The 'charge_profile_lib_data'.
    std::map< std::pair<EV_type, EVSE_type>, std::map< int, charge_profile_lib_data > > PkW_profile;
    
public:  
    pev_charge_profile_library_v2( const EV_EVSE_inventory& inventory,
                                   const std::vector<double> c_rate_scale_factor_levels = std::vector<double>({1.0}) );
    
    const std::vector<double>& get_c_rate_scale_factor_levels() { return c_rate_scale_factor_levels; }
    
    // Call the first  one to add at a specific power level, by providing 'c_rate_scale_factor_index'.
    // Call the second one to always assume the highest-index power level.
    void add_charge_PkW_profile_to_library( const EV_type pev_type,
                                            const EVSE_type SE_type,
                                            const int c_rate_scale_factor_index,
                                            const double timestep_sec,
                                            const std::vector<double>& soc,
                                            const std::vector<ac_power_metrics>& profile );
    void add_charge_PkW_profile_to_library( const EV_type pev_type,
                                            const EVSE_type SE_type,
                                            const double timestep_sec,
                                            const std::vector<double>& soc,
                                            const std::vector<ac_power_metrics>& profile );

    
    // Call the first  one to get a specific power level, by providing 'c_rate_scale_factor_index'.
    // Call the second one to always assume the highest-index power level.
    void get_P3kW_charge_profile( const double start_soc,
                                  const double end_soc,
                                  const EV_type pev_type,
                                  const EVSE_type SE_type,
                                  const int c_rate_scale_factor_index,
                                  double& timestep_sec,
                                  std::vector<double>& P3kW_charge_profile ) const;
    void get_P3kW_charge_profile( const double start_soc,
                                  const double end_soc,
                                  const EV_type pev_type,
                                  const EVSE_type SE_type,
                                  double& timestep_sec,
                                  std::vector<double>& P3kW_charge_profile ) const;


    // Call the first  one to get a specific power level, by providing 'c_rate_scale_factor_index'.
    // Call the second one to always assume the highest-index power level.
    void get_all_charge_profile_data( const double start_soc,
                                      const double end_soc,
                                      const EV_type pev_type,
                                      const EVSE_type SE_type,
                                      const int c_rate_scale_factor_index,
                                      all_charge_profile_data& return_val ) const;
    void get_all_charge_profile_data( const double start_soc,
                                      const double end_soc,
                                      const EV_type pev_type,
                                      const EVSE_type SE_type,
                                      all_charge_profile_data& return_val ) const;
};


#endif




