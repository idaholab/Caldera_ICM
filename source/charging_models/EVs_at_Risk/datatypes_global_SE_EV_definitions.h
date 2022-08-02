
#ifndef inl_datatypes_global_SE_EV_DEFINITIONS_H
#define inl_datatypes_global_SE_EV_DEFINITIONS_H

//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								          EVs-At-Risk  Project
//--------------------------------------------------------------------------------------------------------------------
// ###################################################################################################################
//====================================================================================================================

#include <string>
#include <iostream>
#include <cctype> 		// isspace
#include <vector>

enum supply_equipment_enum
{
    L2_7200 = 0,
    L2_17280 = 1,
    xfc_150 = 2,
    xfc_350 = 3,
	xfc_500kW = 4,
    xfc_1000kW = 5,
    xfc_2000kW = 6,
    xfc_3000kW = 7,
    dwc_100kW = 8
};
std::ostream& operator<<(std::ostream& out, const supply_equipment_enum& x);
std::pair<bool, supply_equipment_enum> get_supply_equipment_enum(const std::string str_val);
bool supply_equipment_is_L1(supply_equipment_enum SE_enum);
bool supply_equipment_is_L2(supply_equipment_enum SE_enum);
bool supply_equipment_is_50kW_dcfc(supply_equipment_enum SE_enum);
bool supply_equipment_is_3phase(supply_equipment_enum SE_enum);
std::vector<supply_equipment_enum> get_all_SE_enums();
supply_equipment_enum get_default_supply_equipment_enum();

//---------------------------------------------

enum vehicle_enum
{       
    ld_50kWh = 0,
    ld_100kWh = 1,
    md_200kWh = 2,
    hd_300kWh = 3,
    hd_400kWh = 4,
    hd_600kWh = 5,
    hd_800kWh = 6,
    hd_1000kWh = 7
};
std::ostream& operator<<(std::ostream& out, const vehicle_enum& x);
std::pair<bool, vehicle_enum> get_vehicle_enum(const std::string str_val);
std::vector<vehicle_enum> get_all_pev_enums();
vehicle_enum get_default_vehicle_enum();

bool is_XFC_charge(vehicle_enum PEV_enum, supply_equipment_enum SE_enum);

//---------------------------------------------

struct pev_SE_pair
{
    vehicle_enum EV_type;
    supply_equipment_enum SE_type;
};

bool pev_is_compatible_with_supply_equipment(vehicle_enum EV_type, supply_equipment_enum SE_type);
std::vector<pev_SE_pair> get_all_compatible_pev_SE_combinations();


#endif

