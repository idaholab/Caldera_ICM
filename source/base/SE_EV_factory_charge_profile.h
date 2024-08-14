
#ifndef inl_Factory_Charge_Profile_H
#define inl_Factory_Charge_Profile_H


#include "datatypes_global.h"                       // SE_configuration, pev_charge_fragment, pev_charge_fragment_removal_criteria
#include "datatypes_module.h"                       // charge_event_P3kW_limits, SE_status, CE_Status
#include "vehicle_charge_model.h"                    // vehicle_charge_model, charge_event_data
#include "supply_equipment_load.h"                    // supply_equipment_load
#include "supply_equipment_control.h"               // supply_equipment_control
#include "supply_equipment.h"                       // supply_equipment
#include "ac_to_dc_converter.h"                     // ac_to_dc_converter

#include "EV_characteristics.h"
#include "EVSE_characteristics.h"
#include "EV_EVSE_inventory.h"
#include "factory_charging_transitions.h"

#include <tuple>


//#############################################################################
//                      Charge Profile Library Factory
//#############################################################################

class factory_charge_profile_library
{
private:
    
    static void create_charge_fragments_vector( const EV_EVSE_inventory& inventory,
                                                const int charge_event_Id,
                                                const double time_step_sec,
                                                const double target_acP3_kW,
                                                const pev_SE_pair pev_SE,
                                                double& max_P3kW,
                                                std::vector<pev_charge_fragment>& charge_fragments,
                                                const double c_rate_scale_factor = 1.0 );
    
    static double get_max_P3kW( const EV_EVSE_inventory& inventory,
                                const double time_step_sec,
                                const pev_SE_pair pev_SE );
    
    static double get_min_P3kW( const EV_EVSE_inventory& inventory,
                                const double max_P3kW,
                                const pev_SE_pair pev_SE );
    
    static void get_charge_profile_aux_parameters( const double max_P3kW,
                                                   const pev_SE_pair pev_SE,
                                                   std::vector<double>& time_step_sec,
                                                   std::vector<double>& target_acP3_kW,
                                                   std::vector<pev_charge_fragment_removal_criteria>& fragment_removal_criteria );
    
    static pev_charge_profile_aux get_pev_charge_profile_aux( const EV_EVSE_inventory& inventory,
                                                              const int charge_event_Id,
                                                              const bool save_validation_data,
                                                              const double time_step_sec,
                                                              const double target_acP3_kW,
                                                              const pev_SE_pair pev_SE,
                                                              const pev_charge_fragment_removal_criteria fragment_removal_criteria,
                                                              double& max_P3kW,
                                                              std::map< std::pair<EV_type, EVSE_type>, std::vector<charge_profile_validation_data> >& validation_data );
    
public:
    
    static pev_charge_profile_library get_charge_profile_library( const EV_EVSE_inventory& inventory,
                                                                  const bool save_validation_data,
                                                                  const bool create_charge_profile_library,
                                                                  std::map< std::pair<EV_type, EVSE_type>, std::vector<charge_profile_validation_data> >& validation_data );
    
    static std::vector<pev_charge_fragment> USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile( const EV_EVSE_inventory& inventory,
                                                                                                const double time_step_sec,
                                                                                                const double target_acP3_kW,
                                                                                                const EV_type pev_type,
                                                                                                const EVSE_type SE_type );
};


//====================================


class factory_charge_profile_library_v2
{
    
public:
    
    static void create_charge_profile( const EV_EVSE_inventory& inventory,
                                       const double time_step_sec,
                                       const pev_SE_pair pev_SE,
                                       const double start_soc,
                                       const double end_soc,
                                       const double target_acP3_kW,
                                       std::vector<double>& soc,
                                       std::vector<ac_power_metrics>& charge_profile,
                                       const double c_rate_scale_factor = 1.0 );

    static pev_charge_profile_library_v2 get_charge_profile_library( const EV_EVSE_inventory& inventory,
                                                                     const double L1_timestep_sec,
                                                                     const double L2_timestep_sec,
                                                                     const double HPC_timestep_sec );
    
    static all_charge_profile_data build_all_charge_profile_data_for_specific_pev_SE_pair( const EV_EVSE_inventory& inventory,
                                                                                           const double timestep_sec,
                                                                                           pev_SE_pair pev_SE,
                                                                                           const double start_soc,
                                                                                           const double end_soc,
                                                                                           const double target_acP3_kW,
                                                                                           const double c_rate_scale_factor = 1.0 );
};

#endif


