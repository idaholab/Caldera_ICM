
#ifndef inl_Factory_SE_EV_H
#define inl_Factory_SE_EV_H

//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								                EVs-At-Risk  Project
//--------------------------------------------------------------------------------------------------------------------
// ###################################################################################################################
//====================================================================================================================

#include "datatypes_global.h"                       // SE_configuration, pev_charge_fragment, pev_charge_fragment_removal_criteria
#include "datatypes_global_SE_EV_definitions.h"     // supply_equipment_enum, vehicle_enum, pev_SE_pair
#include "datatypes_module.h"                       // charge_event_P3kW_limits, SE_status, CE_Status
#include "vehicle_charge_model.h"	                // vehicle_charge_model, charge_event_data
#include "supply_equipment_load.h"		            // supply_equipment_load
#include "supply_equipment_control.h"               // supply_equipment_control
#include "supply_equipment.h"                       // supply_equipment
#include "ac_to_dc_converter.h"                     // ac_to_dc_converter


#include <tuple>

class integrate_X_through_time;		                // Included in cpp file: #include battery_integrate_X_in_time.h


enum battery_chemistry
{
	LMO = 0,
	LTO = 1,
	NMC = 2
};


void get_bat_eff_vs_P2(bool is_charging_not_discharging, battery_chemistry bat_chem, double battery_size_kWh, double& zero_slope_threashold_bat_eff_vs_P2, line_segment& bat_eff_vs_P2);
bool get_pev_battery_info(vehicle_enum vehicle_type, battery_chemistry& bat_chem, double& battery_size_kWh, double& battery_size_with_stochastic_degredation_kWh);


//#############################################################################
//                         EV Charge Model Factory
//#############################################################################


class factory_EV_charge_model
{
private:
	struct point_P2_vs_puVrms
	{
		double puVrms;
		double P2_val;
		
		bool operator<(const point_P2_vs_puVrms& rhs)
		{
			return (this->puVrms < rhs.puVrms);
		}
	};
	
	bool model_stochastic_battery_degregation;
    
    std::map<vehicle_enum, pev_charge_ramping> ramping_by_pevType_only;
    std::map< std::tuple<vehicle_enum, supply_equipment_enum>, pev_charge_ramping> ramping_by_pevType_seType;
	
	void get_integrate_X_through_time_obj(vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type, integrate_X_through_time& return_val);
	void get_P2_vs_puVrms(bool is_charging_not_discharging, vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type, double SE_P2_limit_atNominalV_kW, poly_function_of_x& P2_vs_puVrms);  
	void get_P2_vs_soc(bool is_charging_not_discharging, vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type, double& zero_slope_threashold_P2_vs_soc, std::vector<line_segment>& P2_vs_soc_segments);
	void get_E1_Energy_limit(bool is_charging_not_discharging, bool are_battery_losses, vehicle_enum vehicle_type, supply_equipment_enum supply_equipment_type,
                             double battery_size_with_degredation_kWh, double SE_P2_limit_atNominalV_kW, double recalc_exponent_threashold,
                             double max_P2kW_error_before_reappling_P2kW_limit_to_P2_vs_soc_segments, line_segment& bat_eff_vs_P2,
                             calculate_E1_energy_limit& return_val);
public:
	factory_EV_charge_model();
    void initialize_custome_parameters(std::map<vehicle_enum, pev_charge_ramping> ramping_by_pevType_only_, std::map< std::tuple<vehicle_enum, supply_equipment_enum>, pev_charge_ramping> ramping_by_pevType_seType_);
	void set_bool_model_stochastic_battery_degregation(bool model_stochastic_battery_degregation_);
	vehicle_charge_model* alloc_get_EV_charge_model(const charge_event_data& event, supply_equipment_enum supply_equipment_type, double SE_P2_limit_kW);
};


//#############################################################################
//                Supply Equipment Charge Model Factory
//#############################################################################

class factory_supply_equipment_model
{
private:
    charge_event_queuing_inputs CE_queuing_inputs;

public:
	factory_supply_equipment_model() {};
    factory_supply_equipment_model(charge_event_queuing_inputs& CE_queuing_inputs_);
    
	void get_supply_equipment_model(bool building_charge_profile_library, SE_configuration& SE_config, get_base_load_forecast* baseLD_forecaster, manage_L2_control_strategy_parameters* manage_L2_control, supply_equipment& return_val);
};

//#############################################################################
//                      AC to DC Converter Factory
//#############################################################################

class factory_ac_to_dc_converter
{
private:

public:  
    factory_ac_to_dc_converter() {};
    ac_to_dc_converter* alloc_get_ac_to_dc_converter(ac_to_dc_converter_enum converter_type, supply_equipment_enum SE_type, vehicle_enum vehicle_type, charge_event_P3kW_limits& CE_P3kW_limits);
};


//#############################################################################
//                         Read Input Data Files
//#############################################################################

/*
//#include "read_csv_files.h"			// charge_events_file, supply_equipment_info_file

class charge_events_file_factory : public charge_events_file
{
private:
	int num_lines_to_skip_at_beginning_of_file;
	charge_events_file_factory(const charge_events_file_factory& obj) = default;
	charge_events_file_factory& operator=(const charge_events_file_factory& obj) = default;
	
protected:
	virtual void parse_line(const std::string& line, bool& parse_successful, charge_event_data& event_data) override final;
	virtual int number_of_lines_to_skip_at_beginning_of_file() override final;
	virtual int number_of_fields() override final;
	
public:
	charge_events_file_factory() {};
	virtual ~charge_events_file_factory() override final;
};



class supply_equipment_info_file_factory : public supply_equipment_info_file
{
private:
	supply_equipment_info_file_factory(const supply_equipment_info_file_factory& obj) = default;
	supply_equipment_info_file_factory& operator=(const supply_equipment_info_file_factory& obj) = default;
	
protected:
	virtual void parse_line(const std::string& line, bool& parse_successful, supply_equipment_data& SE_data) override final;
	virtual int number_of_lines_to_skip_at_beginning_of_file() override final;
	virtual int number_of_fields() override final;
public:
	supply_equipment_info_file_factory() {};
	virtual ~supply_equipment_info_file_factory();
};
*/

#endif


