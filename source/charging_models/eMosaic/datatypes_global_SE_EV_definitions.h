
#ifndef inl_datatypes_global_SE_EV_DEFINITIONS_H
#define inl_datatypes_global_SE_EV_DEFINITIONS_H

//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								          eMosaic  Project
//--------------------------------------------------------------------------------------------------------------------
// ###################################################################################################################
//====================================================================================================================

#include <string>
#include <iostream>
#include <cctype> 		// isspace
#include <vector>


enum supply_equipment_enum
{
    L2_7200W = 0,
    L2_17280W = 1,
    xfc_20kW = 2,
    xfc_50kW = 3,
	xfc_90kW = 4,
    xfc_150kW = 5,
    xfc_180kW = 6,
    xfc_450kW = 7
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
    hd_600kWh = 5
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

