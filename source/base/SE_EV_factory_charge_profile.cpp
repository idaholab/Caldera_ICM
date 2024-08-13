

#include "SE_EV_factory_charge_profile.h"

#include "supply_equipment_load.h"
#include "supply_equipment_control.h"
#include "battery_integrate_X_in_time.h"    // integrate_X_through_time
#include "helper.h"                            // rand_val, line_segment
#include "charge_profile_library.h"
#include "charge_profile_downsample_fragments.h"

#include "factory_supply_equipment_model.h"
#include "factory_ac_to_dc_converter.h"
#include "factory_EV_charge_model.h"

#include <cmath>
#include <vector>
#include <algorithm>                        // sort
#include <iostream>                            // cout



//#############################################################################
//                      Charge Profile Library Factory
//#############################################################################

void factory_charge_profile_library::create_charge_fragments_vector( const int charge_event_Id,
                                                                     const double time_step_sec,
                                                                     const double target_acP3_kW,
                                                                     const pev_SE_pair pev_SE,
                                                                     double& max_P3kW,
                                                                     std::vector<pev_charge_fragment>& charge_fragments )
{
    //std::cout << "time_step_sec: " << time_step_sec << "  target_acP3_kW: " << target_acP3_kW << " EV_type: " << pev_SE.EV_type << " SE_type: " << pev_SE.SE_type << std::endl;
    
    charge_fragments.clear();
    max_P3kW = 0;
    
    if(time_step_sec > 60)
    {
        std::cout << "Error:  Varibale time_step_sec must be less than 20 in factory_charge_profile_library::create_charge_fragments_vector." << std::endl;
        return;
    }
    
    //------------------------
    //   Create Factories
    //------------------------
    factory_supply_equipment_model SE_factory{ inventory };
    factory_ac_to_dc_converter ac_to_dc_converter_factory{ inventory };
    pev_charge_profile_library* charge_profile_library = NULL;
    
    EV_ramping_map EV_ramping;
    EV_EVSE_ramping_map EV_EVSE_ramping;
    bool model_stochastic_battery_degregation = false;

    factory_EV_charge_model PEV_charge_factory{ inventory, EV_ramping, EV_EVSE_ramping, model_stochastic_battery_degregation };
    //PEV_charge_factory.set_bool_model_stochastic_battery_degregation(false);

    //------------------------
    //   Create SE Object
    //------------------------
    get_base_load_forecast* baseLD_forecaster = NULL;
    manage_L2_control_strategy_parameters* manage_L2_control = NULL;
    supply_equipment SE_obj;
    
    bool building_charge_profile_library = true;
    SE_configuration SE_config(1, 1, pev_SE.se_type, 12.2, 9.2, "bus_A", "U");  // (station_id, SE_id, SE_enum, lat, long, grid_node_id, location_type)
    SE_factory.get_supply_equipment_model(building_charge_profile_library, SE_config, baseLD_forecaster, manage_L2_control, SE_obj);
    SE_obj.set_pointers_in_SE_Load(&PEV_charge_factory, &ac_to_dc_converter_factory, charge_profile_library);
    
    //------------------------
    //  Create Charge Event
    //------------------------
    control_strategy_enums control_enums;
    control_enums.inverter_model_supports_Qsetpoint = false;
    control_enums.ES_control_strategy = L2_control_strategies_enum::NA;
    control_enums.VS_control_strategy = L2_control_strategies_enum::NA;
    control_enums.ext_control_strategy = "NA";

    stop_charging_criteria stop_charge;

    // (charge_event_id, station_id, SE_id, vehicle_id, vehicle_enum, arrival_unix_time, departure_unix_time, arrival_SOC, departure_SOC, stop_charging_criteria, CE_control_strategy_parameters)
    const double arrival_unix_time = 20*time_step_sec;
    const double departure_unix_time =  1000*3600;
    
    charge_event_data charge_event(charge_event_Id, 1, 1, 1, pev_SE.ev_type, arrival_unix_time, departure_unix_time, 0, 100, stop_charge, control_enums);
    SE_obj.add_charge_event(charge_event);
    
    //------------------------
    //  Create Charge Event
    //------------------------
    ac_power_metrics ac_power;
    pev_charge_fragment fragment;
    fragment.E1_kWh = 0;
    fragment.E2_kWh = 0;
    fragment.E3_kWh = 0;
    fragment.cumQ3_kVARh = 0;
    
    const double pu_Vrms = 1.0;
    double now_unix_time = arrival_unix_time - 3*time_step_sec;
    bool SE_targets_have_been_initialized = false;
    double soc;
    
    while(true)
    {
        SE_obj.get_next(now_unix_time-time_step_sec, now_unix_time, pu_Vrms, soc, ac_power);
        SE_status status_obj = SE_obj.get_SE_status();
        
        if(!SE_targets_have_been_initialized)
        {
            SE_targets_have_been_initialized = true;
            SE_obj.set_target_acP3_kW(target_acP3_kW);
            SE_obj.set_target_acQ3_kVAR(0);
        }
        
        if( ( now_unix_time > arrival_unix_time + 5*time_step_sec )
            &&
            ( status_obj.SE_charging_status_val == SE_charging_status::ev_charge_complete || status_obj.SE_charging_status_val == SE_charging_status::no_ev_plugged_in ) )
        {
            break;
        }
        
        if(status_obj.SE_charging_status_val == SE_charging_status::ev_charging || status_obj.SE_charging_status_val == SE_charging_status::ev_plugged_in_not_charging)
        {
            if( ac_power.P3_kW > max_P3kW )
            {
                max_P3kW = ac_power.P3_kW;
            }

            fragment.soc = status_obj.current_charge.now_soc;
            fragment.E1_kWh += ac_power.P1_kW * ac_power.time_step_duration_hrs;
            fragment.E2_kWh += ac_power.P2_kW * ac_power.time_step_duration_hrs;
            fragment.E3_kWh += ac_power.P3_kW * ac_power.time_step_duration_hrs;
            fragment.cumQ3_kVARh += ac_power.Q3_kVAR * ac_power.time_step_duration_hrs;
            fragment.time_since_charge_began_hrs = (now_unix_time - arrival_unix_time)/3600;
            charge_fragments.push_back(fragment);
        }
        
        now_unix_time += time_step_sec;
    }
}


double factory_charge_profile_library::get_max_P3kW( const double time_step_sec,
                                                     const pev_SE_pair pev_SE )
{
    std::vector<pev_charge_fragment> charge_fragments;
    double max_P3kW;
    double target_acP3_kW = 1000000;
    int charge_event_Id = 1000;
    
    create_charge_fragments_vector(charge_event_Id, time_step_sec, target_acP3_kW, pev_SE, max_P3kW, charge_fragments);
    return max_P3kW;
}


double factory_charge_profile_library::get_min_P3kW( const double max_P3kW,
                                                     const pev_SE_pair pev_SE )
{
    double min_P3kW;

    if(this->inventory.get_EVSE_inventory().at(pev_SE.se_type).get_level() == EVSE_level::L1)
    {
        min_P3kW = 0.8*max_P3kW;    
    }
    else if(this->inventory.get_EVSE_inventory().at(pev_SE.se_type).get_level() == EVSE_level::L2)
    {
        min_P3kW = 0.5*max_P3kW;
    }
    else
    {
        min_P3kW = 1;
    }

    return min_P3kW;
}


void factory_charge_profile_library::get_charge_profile_aux_parameters( const double max_P3kW, 
                                                                        const pev_SE_pair pev_SE, 
                                                                        std::vector<double>& time_step_sec, 
                                                                        std::vector<double>& target_acP3_kW, 
                                                                        std::vector<pev_charge_fragment_removal_criteria>& fragment_removal_criteria )
{
    time_step_sec.clear();
    target_acP3_kW.clear();
    fragment_removal_criteria.clear();
    
    //----------------------------
    //   Populate target_acP3_kW
    //----------------------------
    std::vector<double> target_acP3_multiplier;
    
    if(max_P3kW < 5)
    {
        //std::vector<double> X = {1, 0.8, 0.6, 0.4};
        std::vector<double> X = {1, 0.95};
        target_acP3_multiplier = X;
    }
    else if(max_P3kW < 20)
    {
        //std::vector<double> X = {1, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4};
        std::vector<double> X = {1, 0.95};
        target_acP3_multiplier = X;
    }
    else if(max_P3kW < 100)
    {
        //std::vector<double> X = {1, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2};
        std::vector<double> X = {1, 0.95};
        target_acP3_multiplier = X;
    }
    else
    {
        //std::vector<double> X = {1, 0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55, 0.5, 0.45, 0.4, 0.3, 0.2, 0.1};
        std::vector<double> X = {1, 0.95};
        target_acP3_multiplier = X;
    }
    
    for(double X : target_acP3_multiplier)
        target_acP3_kW.push_back(X*max_P3kW);
    
    //------------------------------------------------------
    //   Populate time_step_sec & fragment_removal_criteria
    //------------------------------------------------------
    
    for(double P3kW : target_acP3_kW)
    {
        if(P3kW < 5)
        {
            time_step_sec.push_back(30);    // 20          
        }
        else if(P3kW < 20)
        {
            time_step_sec.push_back(20);    // 10
        }
        else if(P3kW < 100)
        {
            time_step_sec.push_back(10);     // 2
        }
        else
        {
            time_step_sec.push_back(1);     // 1
        }
        
        double perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow = 50.0;
        if(P3kW < 20)
            perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow = -1;
        
        double kW_change_threshold = P3kW/100;
        if(kW_change_threshold > 2)
            kW_change_threshold = 2;
        else if(kW_change_threshold < 0.1)
            kW_change_threshold = 0.1;
        
        pev_charge_fragment_removal_criteria X;
        X.max_percent_of_fragments_that_can_be_removed = 95;
        X.kW_change_threshold = kW_change_threshold;
        X.threshold_to_determine_not_removable_fragments_on_flat_peak_kW = 0.005;
        X.perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow = perc_of_max_starting_point_to_determine_not_removable_fragments_on_low_elbow;
        fragment_removal_criteria.push_back(X);
    }
}


pev_charge_profile_aux factory_charge_profile_library::get_pev_charge_profile_aux( const int charge_event_Id,
                                                                                   const bool save_validation_data,
                                                                                   const double time_step_sec,
                                                                                   const double target_acP3_kW,
                                                                                   const pev_SE_pair pev_SE,
                                                                                   const pev_charge_fragment_removal_criteria fragment_removal_criteria,
                                                                                   double& max_P3kW )
{
    std::vector<pev_charge_fragment> original_charge_fragments, downsampled_charge_fragments;
    
    create_charge_fragments_vector(charge_event_Id, time_step_sec, target_acP3_kW, pev_SE, max_P3kW, original_charge_fragments);

    downsample_charge_fragment_vector downsample_obj(fragment_removal_criteria);
    downsample_obj.downsample(original_charge_fragments, downsampled_charge_fragments);
    
    //-----------------------------
    
    if(save_validation_data)
    {
        charge_profile_validation_data validation_data_struct;
        validation_data_struct.time_step_sec = time_step_sec;
        validation_data_struct.target_acP3_kW = max_P3kW;
        validation_data_struct.fragment_removal_criteria = fragment_removal_criteria;
        validation_data_struct.removed_fragments = downsample_obj.get_removed_fragments();;
        validation_data_struct.retained_fragments = downsample_obj.get_retained_fragments();;
        validation_data_struct.downsampled_charge_fragments = downsampled_charge_fragments;
        validation_data_struct.original_charge_fragments = original_charge_fragments;
        this->validation_data[std::make_pair(pev_SE.ev_type, pev_SE.se_type)].push_back(validation_data_struct);
    }
    
    //-----------------------------

    pev_charge_profile_aux aux_obj(pev_SE.ev_type, pev_SE.se_type, max_P3kW, downsampled_charge_fragments);
    return aux_obj;
}


pev_charge_profile_library factory_charge_profile_library::get_charge_profile_library( const bool save_validation_data,
                                                                                       const bool create_charge_profile_library )
{
    this->validation_data.clear();

    //-----------------------------------
    
    pev_charge_profile_library return_val{inventory};

    if(create_charge_profile_library)
    {
        const std::vector<pev_SE_pair>& all_pev_SE_pairs = this->inventory.get_all_compatible_pev_SE_combinations();
        
        int charge_event_Id = 1000;
        double get_max_time_step_sec;
        
        for(const pev_SE_pair& pev_SE : all_pev_SE_pairs)
        {
            if(this->inventory.get_EVSE_inventory().at(pev_SE.se_type).get_level() == EVSE_level::L1)
            {
                get_max_time_step_sec = 60;
            }
            else if(this->inventory.get_EVSE_inventory().at(pev_SE.se_type).get_level() == EVSE_level::L2)
            {
                get_max_time_step_sec = 30;
            }
            else
            {
                get_max_time_step_sec = 1;
            }
            
            const double max_target_P3kW = get_max_P3kW(get_max_time_step_sec, pev_SE);
            
            //-----------------------
            
            std::vector<double> time_step_sec;
            std::vector<double> target_acP3_kW;
            std::vector<pev_charge_fragment_removal_criteria> fragment_removal_criteria;
            
            get_charge_profile_aux_parameters( max_target_P3kW, pev_SE, time_step_sec, target_acP3_kW, fragment_removal_criteria );
            
            //-----------------------
            
            std::vector<pev_charge_profile_aux> charge_profiles_aux_vector;
            
            double max_P3kW = 0;
            double max_P3kW_tmp;
            
            for( int i = 0; i < time_step_sec.size(); i++ )
            {
                charge_event_Id += 1;
                charge_profiles_aux_vector.push_back(get_pev_charge_profile_aux(charge_event_Id, save_validation_data, time_step_sec[i], target_acP3_kW[i], pev_SE, fragment_removal_criteria[i], max_P3kW_tmp));
            
                if(max_P3kW_tmp > max_P3kW)
                {
                    max_P3kW = max_P3kW_tmp;
                }
            }
            
            //-----------------------
            
            charge_event_P3kW_limits CE_P3kW_limits;
            CE_P3kW_limits.max_P3kW = max_P3kW;
            CE_P3kW_limits.min_P3kW = get_min_P3kW(max_P3kW, pev_SE);
            
            //-----------------------

            pev_charge_profile charge_profile(pev_SE.ev_type, pev_SE.se_type, CE_P3kW_limits, charge_profiles_aux_vector);
            return_val.add_charge_profile_to_library(pev_SE.ev_type, pev_SE.se_type, charge_profile);
        }
    }

    return return_val;
}


std::map< std::pair<EV_type, EVSE_type>, std::vector<charge_profile_validation_data> > factory_charge_profile_library::get_validation_data()
{
    return this->validation_data;
}


std::vector<pev_charge_fragment> factory_charge_profile_library::USE_FOR_DEBUG_PURPOSES_ONLY_get_raw_charge_profile( const double time_step_sec,
                                                                                                                     const double target_acP3_kW,
                                                                                                                     const EV_type pev_type,
                                                                                                                     const EVSE_type SE_type )
{
    pev_SE_pair pev_SE;
    pev_SE.ev_type = pev_type;
    pev_SE.se_type = SE_type;
    
    double max_P3kW;
    int charge_event_Id = 1000;
    std::vector<pev_charge_fragment> return_val;    
    create_charge_fragments_vector(charge_event_Id, time_step_sec, target_acP3_kW, pev_SE, max_P3kW, return_val);
    
    return return_val;
}




//#############################################################################
//                      Charge Profile Library Factory
//#############################################################################


void factory_charge_profile_library_v2::create_charge_profile( const EV_EVSE_inventory& inventory, 
                                                               const double time_step_sec,
                                                               const pev_SE_pair pev_SE,
                                                               const double start_soc,
                                                               const double end_soc,
                                                               const double target_acP3_kW,
                                                               std::vector<double>& soc,
                                                               std::vector<ac_power_metrics>& charge_profile )
{
    charge_profile.clear();
    soc.clear();
    
    double max_time_step_sec = 600;
    if(time_step_sec > max_time_step_sec)
    {
        std::cout << "Error:  Varibale time_step_sec must be less than " << max_time_step_sec << " in factory_charge_PkW_library::create_charge_profile." << std::endl;
        return;
    }
    
    //------------------------
    //   Create Factories
    //------------------------
    factory_supply_equipment_model SE_factory{ inventory };
    factory_ac_to_dc_converter ac_to_dc_converter_factory{inventory};
    pev_charge_profile_library* charge_profile_library = NULL; // Is needed in SE_obj
    
    EV_ramping_map EV_ramping;
    EV_EVSE_ramping_map EV_EVSE_ramping;
    bool model_stochastic_battery_degregation = false;

    factory_EV_charge_model PEV_charge_factory{ inventory, EV_ramping, EV_EVSE_ramping, model_stochastic_battery_degregation };

    //------------------------
    //   Create SE Object
    //------------------------
    get_base_load_forecast* baseLD_forecaster = NULL;
    manage_L2_control_strategy_parameters* manage_L2_control = NULL;
    supply_equipment SE_obj;
    
    bool building_charge_profile_library = true;
    SE_configuration SE_config(1, 1, pev_SE.se_type, 12.2, 9.2, "bus_A", "U");  // (station_id, SE_id, SE_enum, lat, long, grid_node_id, location_type)
    SE_factory.get_supply_equipment_model(building_charge_profile_library, SE_config, baseLD_forecaster, manage_L2_control, SE_obj);
    SE_obj.set_pointers_in_SE_Load(&PEV_charge_factory, &ac_to_dc_converter_factory, charge_profile_library);
    
    //------------------------
    //  Create Charge Event
    //------------------------
    control_strategy_enums control_enums;
    control_enums.inverter_model_supports_Qsetpoint = false;
    control_enums.ES_control_strategy = L2_control_strategies_enum::NA;
    control_enums.VS_control_strategy = L2_control_strategies_enum::NA;
    control_enums.ext_control_strategy = "NA";

    stop_charging_criteria stop_charge;

    // (charge_event_id, station_id, SE_id, vehicle_id, vehicle_enum, arrival_unix_time, departure_unix_time, arrival_SOC, departure_SOC, stop_charging_criteria, CE_control_strategy_parameters)
    double arrival_unix_time = 5*max_time_step_sec;
    double departure_unix_time =  arrival_unix_time + 1000*3600;
    
    charge_event_data charge_event(1, 1, 1, 1, pev_SE.ev_type, arrival_unix_time, departure_unix_time, start_soc, end_soc, stop_charge, control_enums);
    SE_obj.add_charge_event(charge_event);
    
    //------------------------
    //  Create Charge Event
    //------------------------
    ac_power_metrics ac_power;
    
    double pu_Vrms = 1.0;
    double now_unix_time = arrival_unix_time - 3*time_step_sec;
    bool SE_targets_have_been_initialized = false;
    SE_status status_obj;
    double soc_val;
    
    while(true)
    {
        SE_obj.get_next(now_unix_time-time_step_sec, now_unix_time, pu_Vrms, soc_val, ac_power);
        status_obj = SE_obj.get_SE_status();
        
        if(!SE_targets_have_been_initialized)
        {
            SE_targets_have_been_initialized = true;
            SE_obj.set_target_acP3_kW(target_acP3_kW);
            SE_obj.set_target_acQ3_kVAR(0);
        }
        
        if((now_unix_time > arrival_unix_time + 5*time_step_sec) && (status_obj.SE_charging_status_val == SE_charging_status::ev_charge_complete || status_obj.SE_charging_status_val == SE_charging_status::no_ev_plugged_in))
            break;
        
        if(status_obj.SE_charging_status_val == SE_charging_status::ev_charging || status_obj.SE_charging_status_val == SE_charging_status::ev_plugged_in_not_charging)
        {
            charge_profile.push_back(ac_power);
            soc.push_back(soc_val);
        }
        
        now_unix_time += time_step_sec;
    }
}


pev_charge_profile_library_v2 factory_charge_profile_library_v2::get_charge_profile_library( const double L1_timestep_sec,
                                                                                             const double L2_timestep_sec,
                                                                                             const double HPC_timestep_sec )
{
    pev_charge_profile_library_v2 return_val{ this->inventory };
    std::vector<pev_SE_pair> all_pev_SE_pairs = this->inventory.get_all_compatible_pev_SE_combinations();
    
    std::vector<ac_power_metrics> charge_profile;
    std::vector<double> soc;
    
    const double target_acP3_kW = 1000000;    
    const double start_soc = 0;
    const double end_soc = 100;
    
    for( pev_SE_pair pev_SE : all_pev_SE_pairs )
    {
        const double time_step_sec = [&] () {
            double time_step_sec;
            if( this->inventory.get_EVSE_inventory().at(pev_SE.se_type).get_level() == EVSE_level::L1 )
            {
                time_step_sec = L1_timestep_sec;
            }
            else if( this->inventory.get_EVSE_inventory().at(pev_SE.se_type).get_level() == EVSE_level::L2 )
            {
                time_step_sec = L2_timestep_sec;
            }
            else
            {
                time_step_sec = HPC_timestep_sec;
            }
            return time_step_sec;
        }();
        
        // "soc" and "charge_profile" are initialized by calling this function.
        factory_charge_profile_library_v2::create_charge_profile( this->inventory, time_step_sec, pev_SE, start_soc, end_soc, target_acP3_kW, soc, charge_profile );
        
        // Put the charge profile in the library.
        return_val.add_charge_PkW_profile_to_library( pev_SE.ev_type, pev_SE.se_type, time_step_sec, soc, charge_profile );
    }

    return return_val;
}


