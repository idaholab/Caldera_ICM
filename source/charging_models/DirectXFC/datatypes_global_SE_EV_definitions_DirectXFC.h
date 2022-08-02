
#ifndef inl_datatypes_global_SE_EV_DEFINITIONS_H
#define inl_datatypes_global_SE_EV_DEFINITIONS_H

//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								           DirectXFC Project
//--------------------------------------------------------------------------------------------------------------------
// ###################################################################################################################
//====================================================================================================================

#include <string>
#include <iostream>
#include <cctype> 		// isspace
#include <vector>

enum supply_equipment_enum
{
	L1_1440 = 0,
	L2_3600 = 1,
	L2_7200 = 2,
	L2_9600 = 3,
	L2_11520 = 4,
	L2_17280 = 5,	
	dcfc_50 = 6,
	xfc_150 = 7,
	xfc_350 = 8
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
{       // Recharge
	phev20 = 0,
	phev50 = 1,
    phev_SUV = 2,
	bev150_ld1_50kW = 3,
	bev250_ld1_75kW = 4,
	bev275_ld1_150kW = 5,
	bev200_ld4_150kW = 6,
	bev250_ld2_300kW = 7,
    
        // Additional PEVs for DirectXFC
    bev250_400kW = 8,
    bev300_575kW = 9,
    bev300_400kW = 10,
    bev250_350kW = 11,
    bev300_300kW = 12,
    bev150_150kW = 13
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

