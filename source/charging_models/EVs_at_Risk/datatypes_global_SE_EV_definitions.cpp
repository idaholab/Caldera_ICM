
//====================================================================================================================
// ###################################################################################################################
//--------------------------------------------------------------------------------------------------------------------
// 								            EVs-At-Risk  Project
//--------------------------------------------------------------------------------------------------------------------
// ###################################################################################################################
//====================================================================================================================


#include "datatypes_global_SE_EV_definitions.h"


//==================================
//	    Supply Equipment Enum
//==================================


bool supply_equipment_is_L1(supply_equipment_enum SE_enum)
{	
    return false;
    /*
	if(SE_enum == L1_1440)
		return true;
	else
		return false;
    */
}


bool supply_equipment_is_L2(supply_equipment_enum SE_enum)
{
	if(SE_enum == L2_7200 || SE_enum == L2_17280)
		return true;
	else
		return false;
}


bool supply_equipment_is_50kW_dcfc(supply_equipment_enum SE_enum)
{	
    return false;
    /*
	if(SE_enum == dcfc_50)
		return true;
	else
		return false;
    */
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
    std::vector<supply_equipment_enum> SE_enum_vec = {xfc_150, xfc_350, xfc_500kW, xfc_1000kW, xfc_2000kW, xfc_3000kW, dwc_100kW};
    std::vector<vehicle_enum> PEV_enum_vec = {ld_50kWh, ld_100kWh, md_200kWh, hd_300kWh, hd_400kWh, hd_600kWh, hd_800kWh, hd_1000kWh};
    
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

    if      (tmp_str == "L2_7200") 	    SE_enum = L2_7200;
    else if (tmp_str == "L2_17280") 	SE_enum = L2_17280;
    else if (tmp_str == "xfc_150") 	    SE_enum = xfc_150;
    else if (tmp_str == "xfc_350") 	    SE_enum = xfc_350;
	else if (tmp_str == "xfc_500kW") 	SE_enum = xfc_500kW;
	else if	(tmp_str == "xfc_1000kW") 	SE_enum = xfc_1000kW;
	else if (tmp_str == "xfc_2000kW") 	SE_enum = xfc_2000kW;
	else if (tmp_str == "xfc_3000kW") 	SE_enum = xfc_3000kW;
    else if (tmp_str == "dwc_100kW") 	SE_enum = dwc_100kW;    
	else
	{
        conversion_successfull = false;
        SE_enum = xfc_500kW;
    }
    
    std::pair<bool, supply_equipment_enum> return_val = {conversion_successfull, SE_enum};
    
    return return_val;
}


std::ostream& operator<<(std::ostream& out, const supply_equipment_enum& x)
{
         if(x == L2_7200) 	    out << "L2_7200";
    else if(x == L2_17280) 	    out << "L2_17280";
    else if(x == xfc_150) 	    out << "xfc_150";
    else if(x == xfc_350) 	    out << "xfc_350";
	else if(x == xfc_500kW) 	out << "xfc_500kW";
	else if(x == xfc_1000kW) 	out << "xfc_1000kW";
	else if(x == xfc_2000kW) 	out << "xfc_2000kW";
	else if(x == xfc_3000kW) 	out << "xfc_3000kW";
    else if(x == dwc_100kW) out << "dwc_100kW";
	
	return out;
}


std::vector<supply_equipment_enum> get_all_SE_enums()
{
    std::vector<supply_equipment_enum> return_val = {L2_7200, L2_17280, xfc_150, xfc_350, xfc_500kW, xfc_1000kW, xfc_2000kW, xfc_3000kW, dwc_100kW};
    
    return return_val;
}


supply_equipment_enum get_default_supply_equipment_enum()
{
    return xfc_500kW;
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
	if 		(tmp_str == "ld_50kWh") pev_enum = ld_50kWh;
	else if (tmp_str == "ld_100kWh") pev_enum = ld_100kWh;
	else if (tmp_str == "md_200kWh") pev_enum = md_200kWh;
	else if (tmp_str == "hd_300kWh") pev_enum = hd_300kWh;
	else if (tmp_str == "hd_400kWh") pev_enum = hd_400kWh;
    else if (tmp_str == "hd_600kWh") pev_enum = hd_600kWh;
	else if (tmp_str == "hd_800kWh") pev_enum = hd_800kWh;
	else if	(tmp_str == "hd_1000kWh") pev_enum = hd_1000kWh;
	else
	{
        conversion_successfull = false;
        pev_enum = ld_50kWh;
    }
    
    std::pair<bool, vehicle_enum> return_val = {conversion_successfull, pev_enum};
    
    return return_val;
}


std::ostream& operator<<(std::ostream& out, const vehicle_enum& x)
{
	     if(x == ld_50kWh) 	    out << "ld_50kWh";
	else if(x == ld_100kWh) 	out << "ld_100kWh";
    else if(x == md_200kWh) 	out << "md_200kWh";
	else if(x == hd_300kWh) 	out << "hd_300kWh";
	else if(x == hd_400kWh) 	out << "hd_400kWh";
	else if(x == hd_600kWh) 	out << "hd_600kWh";
	else if(x == hd_800kWh) 	out << "hd_800kWh";
	else if(x == hd_1000kWh) 	out << "hd_1000kWh";
    
	return out;
}


std::vector<vehicle_enum> get_all_pev_enums()
{
    std::vector<vehicle_enum> return_val = {ld_50kWh, ld_100kWh, md_200kWh, hd_300kWh, hd_400kWh, hd_600kWh, hd_800kWh, hd_1000kWh};
    
    return return_val;
}


vehicle_enum get_default_vehicle_enum()
{
    return ld_50kWh;
}


//==========================================
//	PEV is Compatible with Supply Equipment
//==========================================

bool pev_is_compatible_with_supply_equipment(vehicle_enum EV_type, supply_equipment_enum SE_type)
{
    return true;
    
    /*
	if( (EV_type == phev20 || EV_type == phev50 || EV_type == phev_SUV) && (SE_type == dcfc_50 || SE_type == xfc_150 || SE_type == xfc_350) )
		return false;
 	//else if( (SE_type == dcfc_50) && (EV_type == bev275_ld1_150kW || EV_type == bev200_ld4_150kW || EV_type == bev250_ld2_300kW || EV_type == bev250_400kW || EV_type == bev300_575kW || EV_type == bev300_400kW || EV_type == bev250_350kW || EV_type == bev300_300kW || EV_type == bev150_150kW) )
	//	return false;
	else
		return true;
    */
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


