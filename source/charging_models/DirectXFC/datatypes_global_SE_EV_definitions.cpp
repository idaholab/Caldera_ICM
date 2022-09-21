
//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								          DirectXFC Project
//--------------------------------------------------------------------------------------------------------------------
// ###################################################################################################################
//====================================================================================================================


#include "datatypes_global_SE_EV_definitions.h"


//==================================
//	    Supply Equipment Enum
//==================================


bool supply_equipment_is_L1(supply_equipment_enum SE_enum)
{	
	if(SE_enum == L1_1440)
		return true;
	else
		return false;
}


bool supply_equipment_is_L2(supply_equipment_enum SE_enum)
{
	if(SE_enum == L2_3600 || SE_enum == L2_7200 || SE_enum == L2_9600 || SE_enum == L2_11520 || SE_enum == L2_17280)
		return true;
	else
		return false;
}


bool supply_equipment_is_50kW_dcfc(supply_equipment_enum SE_enum)
{	
	if(SE_enum == dcfc_50)
		return true;
	else
		return false;
}


bool supply_equipment_is_3phase(supply_equipment_enum SE_enum)
{
    if(supply_equipment_is_L1(SE_enum) || supply_equipment_is_L2(SE_enum))
        return false;
    else
        return true;
}


bool is_XFC_charge(vehicle_enum PEV_enum, supply_equipment_enum SE_enum)
{
    std::vector<supply_equipment_enum> SE_enum_vec = {xfc_150, xfc_350};
    std::vector<vehicle_enum> PEV_enum_vec = {bev275_ld1_150kW, bev200_ld4_150kW, bev250_ld2_300kW, bev250_400kW, bev300_575kW, bev300_400kW, bev250_350kW, bev300_300kW, bev150_150kW};
    
    bool SE_is_XFC_capable = false;
    for(supply_equipment_enum x : SE_enum_vec)
    {
        if(x == SE_enum)
        {
            SE_is_XFC_capable = true;
            break;
        }
    }
    
    bool PEV_is_XFC_capable = false;
    for(vehicle_enum x : PEV_enum_vec)
    {
        if(x == PEV_enum)
        {
            PEV_is_XFC_capable = true;
            break;
        }
    }
    
    return SE_is_XFC_capable && PEV_is_XFC_capable;
}


std::pair<bool, supply_equipment_enum> get_supply_equipment_enum(const std::string str_val)
{
	bool conversion_successfull = true;
    supply_equipment_enum SE_enum;
    
	std::string tmp_str = "";
	
	// ignores whitespace
	for (int i = 0; i < str_val.length(); i++)
		if (!std::isspace(str_val[i]))
			tmp_str += str_val[i];

	if 		(tmp_str == "1") 	SE_enum = L1_1440;
	else if	(tmp_str == "2") 	SE_enum = L2_3600;
	else if (tmp_str == "3") 	SE_enum = L2_7200;
	else if (tmp_str == "4") 	SE_enum = L2_9600;
	else if (tmp_str == "5") 	SE_enum = L2_11520;
	else if (tmp_str == "20") 	SE_enum = L2_17280;
	
	else if (tmp_str == "6") 	SE_enum = dcfc_50;
	else if (tmp_str == "7") 	SE_enum = xfc_150;
	else if (tmp_str == "8") 	SE_enum = xfc_350;
	else
	{
        conversion_successfull = false;
        SE_enum = L1_1440;
    }
    
    std::pair<bool, supply_equipment_enum> return_val = {conversion_successfull, SE_enum};
    
    return return_val;
}


std::ostream& operator<<(std::ostream& out, const supply_equipment_enum& x)
{
	     if(x == L1_1440) 	out << "L1_1440";
	else if(x == L2_3600) 	out << "L2_3600";
	else if(x == L2_7200) 	out << "L2_7200";
	else if(x == L2_9600) 	out << "L2_9600";
	else if(x == L2_11520) 	out << "L2_11520";
	else if(x == L2_17280) 	out << "L2_17280";
	
	else if(x == dcfc_50) 	out << "dcfc_50";
	else if(x == xfc_150) 	out << "xfc_150";
	else if(x == xfc_350) 	out << "xfc_350";
	
	return out;
}


std::vector<supply_equipment_enum> get_all_SE_enums()
{
    std::vector<supply_equipment_enum> return_val = {L1_1440, L2_3600, L2_7200, L2_9600, L2_11520, L2_17280, dcfc_50, xfc_150, xfc_350};
    
    return return_val;
}


supply_equipment_enum get_default_supply_equipment_enum()
{
    return L2_9600;
}


//==================================
//	       Vehicle Enum
//==================================


std::pair<bool, vehicle_enum> get_vehicle_enum(const std::string str_val)
{
	bool conversion_successfull = true;
    vehicle_enum pev_enum;
    
	std::string tmp_str = "";
	
	// ignores whitespace
	for (int i = 0; i < str_val.length(); i++)
		if (!std::isspace(str_val[i]))
			tmp_str += str_val[i];
	
        // Recharge
	if 		(tmp_str == "1") pev_enum = bev250_ld2_300kW;
	else if (tmp_str == "2") pev_enum = bev200_ld4_150kW;
	else if (tmp_str == "3") pev_enum = bev275_ld1_150kW;
	else if (tmp_str == "4") pev_enum = bev250_ld1_75kW;
	else if (tmp_str == "5") pev_enum = bev150_ld1_50kW;
    else if (tmp_str == "6") pev_enum = phev_SUV;
	else if (tmp_str == "7") pev_enum = phev50;
	else if	(tmp_str == "8") pev_enum = phev20;
    
        // DirectXFC
    else if	(tmp_str == "9") pev_enum = bev250_400kW;
    else if	(tmp_str == "10") pev_enum = bev300_575kW;
    else if	(tmp_str == "11") pev_enum = bev300_400kW;
    else if	(tmp_str == "12") pev_enum = bev250_350kW;
    else if	(tmp_str == "13") pev_enum = bev300_300kW;
    else if	(tmp_str == "14") pev_enum = bev150_150kW;
	else
	{
        conversion_successfull = false;
        pev_enum = bev150_ld1_50kW;
    }
    
    std::pair<bool, vehicle_enum> return_val = {conversion_successfull, pev_enum};
    
    return return_val;
}


std::ostream& operator<<(std::ostream& out, const vehicle_enum& x)
{
	     if(x == phev20) 	out << "phev20";
	else if(x == phev50) 	out << "phev50";
    else if(x == phev_SUV) 	out << "phev_SUV";
	else if(x == bev150_ld1_50kW) 	out << "bev150_ld1_50kW";
	else if(x == bev250_ld1_75kW) 	out << "bev250_ld1_75kW";
	else if(x == bev275_ld1_150kW) 	out << "bev275_ld1_150kW";
	else if(x == bev200_ld4_150kW) 	out << "bev200_ld4_150kW";
	else if(x == bev250_ld2_300kW) 	out << "bev250_ld2_300kW";
	
    else if(x == bev250_400kW) 	out << "bev250_400kW";
    else if(x == bev300_575kW) 	out << "bev300_575kW";
    else if(x == bev300_400kW) 	out << "bev300_400kW";
    else if(x == bev250_350kW) 	out << "bev250_350kW";
    else if(x == bev300_300kW) 	out << "bev300_300kW";
    else if(x == bev150_150kW) 	out << "bev150_150kW";
        
	return out;
}


std::vector<vehicle_enum> get_all_pev_enums()
{
    std::vector<vehicle_enum> return_val = {phev20, phev50, phev_SUV, bev150_ld1_50kW, bev250_ld1_75kW, bev275_ld1_150kW, bev200_ld4_150kW, bev250_ld2_300kW, bev250_400kW, bev300_575kW, bev300_400kW, bev250_350kW, bev300_300kW, bev150_150kW};
    
    return return_val;
}


vehicle_enum get_default_vehicle_enum()
{
    return bev150_ld1_50kW;
}

//==========================================
//	PEV is Compatible with Supply Equipment
//==========================================

bool pev_is_compatible_with_supply_equipment(vehicle_enum EV_type, supply_equipment_enum SE_type)
{
	if( (EV_type == phev20 || EV_type == phev50 || EV_type == phev_SUV) && (SE_type == dcfc_50 || SE_type == xfc_150 || SE_type == xfc_350) )
		return false;
 	//else if( (SE_type == dcfc_50) && (EV_type == bev275_ld1_150kW || EV_type == bev200_ld4_150kW || EV_type == bev250_ld2_300kW || EV_type == bev250_400kW || EV_type == bev300_575kW || EV_type == bev300_400kW || EV_type == bev250_350kW || EV_type == bev300_300kW || EV_type == bev150_150kW) )
	//	return false;
	else
		return true;
}


std::vector<pev_SE_pair> get_all_compatible_pev_SE_combinations()
{
    std::vector<vehicle_enum> pev_types = get_all_pev_enums();
    std::vector<supply_equipment_enum> SE_types = get_all_SE_enums();
    
    std::vector<pev_SE_pair> return_val;
    
    for(vehicle_enum pev_type : pev_types)
    {
        for(supply_equipment_enum SE_type : SE_types)
        {
            if(pev_is_compatible_with_supply_equipment(pev_type, SE_type))
                return_val.push_back({pev_type, SE_type});
        }
    }
    
    return return_val;
}


