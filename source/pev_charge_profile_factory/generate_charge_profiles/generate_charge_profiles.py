#!/usr/bin/env python3

import charge_profile_factory as factory


min_zero_slope_threashold_P2_vs_soc = 0.01  # 1 kW change in P2 over 100 soc

plot_soc_vs_P2_soc_step_size = 1

#--------------------------------------------------------------------------

def get_charge_profile(pev_charge_profile):
	P2_vs_soc = []
	soc_vals = []
	i = 0
	soc = 0
	
	while True:
		while True:
			if soc <= pev_charge_profile[i].x_UB:
				break
			else:
				i+=1		
		
		soc_vals.append(soc)
		
		a = pev_charge_profile[i].a
		b = pev_charge_profile[i].b
					
		P2_vs_soc.append(a*soc+b)			
		soc+=plot_soc_vs_P2_soc_step_size
	
		if 100 < soc:
			break
		
	return (P2_vs_soc, soc_vals)
	
	
def print_charge_profile(ouput_file_name, charge_profile_list, soc_vals):
	lines_to_print = []
	
	SE_enum_str = ', '
	EV_enum_str = 'soc, '
	
	for (SE_enum, EV_enum, P2_vs_soc) in charge_profile_list:
		SE_enum_str += "{}, ".format(SE_enum)
		EV_enum_str += "{}, ".format(EV_enum)
	
	lines_to_print.append(SE_enum_str)
	lines_to_print.append(EV_enum_str)
	
	#-----------------------
	
	for i in range(len(soc_vals)):
		str_val = "{}, ".format(soc_vals[i])
		for (SE_enum, EV_enum, P2_vs_soc) in charge_profile_list:
			str_val += "{}, ".format(P2_vs_soc[i])
	
		lines_to_print.append(str_val)
	
	#-----------------------
	
	with open(ouput_file_name, "w") as fout_profiles:		
		for str_val in lines_to_print:
			fout_profiles.write(str_val+"\n")
			

def plot_dcfc_charge_profile(dcfc_charge_profile_list):
	charge_profile_list = []
	
	for (SE_enum, EV_enum, pev_charge_profile, constraints) in dcfc_charge_profile_list:		
		(P2_vs_soc, soc_vals) = get_charge_profile(pev_charge_profile)		
		charge_profile_list.append((SE_enum, EV_enum, P2_vs_soc))
				
	#-------------------------------------------------------
	
	print_charge_profile("./output/dcfc_charge_profiles.csv", charge_profile_list, soc_vals)


def plot_L1_L2_charge_profile(L1_L2_charge_profile_list):
	charge_profile_list = []
	
	for (EV_enum, pev_charge_profile) in L1_L2_charge_profile_list:		
		(P2_vs_soc, soc_vals) = get_charge_profile(pev_charge_profile)		
		charge_profile_list.append(("", EV_enum, P2_vs_soc))
				
	#-------------------------------------------------------
	
	print_charge_profile("./output/L1_L2_charge_profiles.csv", charge_profile_list, soc_vals)


#--------------------------------------------------------------------------

def get_min_non_zero_slope(slopes):

	abs_min_non_zero_slope = 1000000

	for m in slopes:
		if 0 < abs(m) and abs(m) < abs_min_non_zero_slope:
			abs_min_non_zero_slope = abs(m)
	
	return abs_min_non_zero_slope


def write_charge_profile(is_dcfc_not_L1_L2, fout_data_, fout_code_, SE_enum_, EV_enum_, pev_charge_profile_):
	fout_data_.write("EV,{}\n".format(EV_enum_))
	if is_dcfc_not_L1_L2: fout_data_.write("SE,{}\n".format(SE_enum_))	
	fout_data_.write("soc_LB,soc_UB,a,b\n")
	
	if is_dcfc_not_L1_L2:
		fout_code_.write("else if(SE_enum == {} && EV_enum == {})".format(SE_enum_, EV_enum_) + "\n")
	else:
		fout_code_.write("else if(EV_enum == {})".format(EV_enum_) + "\n")
	
	fout_code_.write("{\n")
	
	slopes = []
	
	for i in range(len(pev_charge_profile_)):
		slopes.append(pev_charge_profile_[i].a)
		x_LB = pev_charge_profile_[i].x_LB
		x_UB = pev_charge_profile_[i].x_UB
		
		if(x_LB == 0): x_LB = -0.1
		if(x_UB == 100): x_UB = 100.1
		
		data_str = "{}, {}, {}, {}".format(x_LB, x_UB, pev_charge_profile_[i].a, pev_charge_profile_[i].b)
		
		fout_data_.write(data_str + "\n")
		fout_code_.write("    P2_vs_soc_segments.push_back({" + data_str + "});\n")
		
	#-----------------
	
	fout_data_.write("\n\n")
	
	#-----------------	
	
	min_non_zero_slope = get_min_non_zero_slope(slopes)
	
	if min_non_zero_slope < min_zero_slope_threashold_P2_vs_soc:	
		fout_code_.write("    zero_slope_threashold_P2_vs_soc = {};\n".format(min_zero_slope_threashold_P2_vs_soc))
		fout_code_.write("    // min_non_zero_slope = {}\n".format(min_non_zero_slope))
		fout_code_.write("    // !!!!!!!!!!!!!!!!!!!!   Very Small Slope Present   !!!!!!!!!!!!!!!!!!!!\n")
	else:
		fout_code_.write("    zero_slope_threashold_P2_vs_soc = {};\n".format(0.9 * min_non_zero_slope))
		fout_code_.write("    // min_non_zero_slope = {}\n".format(min_non_zero_slope))
	
	fout_code_.write("}\n")


#--------------------------------------------------------------------------


def write_constraints(fout_constraints_, SE_enum_, EV_enum_, constraints_):
	fout_constraints_.write("EV,{}\n".format(EV_enum_))
	fout_constraints_.write("SE,{}\n".format(SE_enum_))	
	fout_constraints_.write("a,b\n")
	
	for i in range(len(constraints_)):		
		data_str = "{}, {}".format(constraints_[i].a, constraints_[i].b)		
		fout_constraints_.write(data_str + "\n")
		
	fout_constraints_.write("\n\n")
	
	
#==============================================================================================


factory_obj = factory.pev_charge_profile_factory()


with open("./dcfc_inputs.csv", "r") as fin, open("./output/dcfc_data.csv", "w") as fout_data, \
     open("./output/dcfc_code.txt", "w") as fout_code, open("./output/dcfc_constraints.csv", "w") as fout_constraints:
    
	dcfc_charge_profile_list = []
    
	fin.readline() #skip the header
	for linebuf in fin.readlines():
		tokens = linebuf.split(',')
	
		if len(tokens) != 7:
			print("Malformed input line. Skipping...")
			continue
		
		EV_enum = tokens[0]
		SE_enum = tokens[5]
		
		# -------------------------------------
		
		bat_chemistry = None
		
		if tokens[1] == "LMO":
			bat_chemistry = factory.pev_battery_chemistry.LMO
		elif tokens[1] == "LTO":
			bat_chemistry = factory.pev_battery_chemistry.LTO
		elif tokens[1] == "NMC":
			bat_chemistry = factory.pev_battery_chemistry.NMC
		else:
			print("Invalid battery chemistry: {}. Skipping...".format(tokens[1]))
			continue
			
		# -------------------------------------
		
		battery_size_kWh      = float(tokens[2])
		bat_size_Ah_1C    = float(tokens[3])
		pev_max_c_rate        = float(tokens[4])
		dcfc_dc_current_limit = float(tokens[6])
		pev_P2_pwr_limit_kW = 10000 # placeholder
		
		pev_charge_profile = factory_obj.get_dcfc_charge_profile(bat_chemistry, battery_size_kWh, bat_size_Ah_1C, pev_max_c_rate, pev_P2_pwr_limit_kW, dcfc_dc_current_limit)
		constraints = factory_obj.get_constraints()
			
		# -------------------------------------
		
		write_charge_profile(True, fout_data, fout_code, SE_enum, EV_enum, pev_charge_profile)
		write_constraints(fout_constraints, SE_enum, EV_enum, constraints)
		dcfc_charge_profile_list.append([SE_enum, EV_enum, pev_charge_profile, constraints])
				
	plot_dcfc_charge_profile(dcfc_charge_profile_list)

#------------------------------------------------------------------------------------------------------------------------------

with open("./L1_L2_inputs.csv", "r") as fin, open("./output/L1_L2_data.csv", "w") as fout_data, open("./output/L1_L2_code.txt", "w") as fout_code:
	L1_L2_charge_profile_list = []
	
	fin.readline() #skip the header
	for linebuf in fin.readlines():
		tokens = linebuf.split(',')
	
		if len(tokens) != 4:
			print("Malformed input line. Skipping...")
			continue
		
		EV_enum = tokens[0]
				
		# -------------------------------------
		
		bat_chemistry = None
		
		if tokens[1] == "LMO":
			bat_chemistry = factory.pev_battery_chemistry.LMO
		elif tokens[1] == "LTO":
			bat_chemistry = factory.pev_battery_chemistry.LTO
		elif tokens[1] == "NMC":
			bat_chemistry = factory.pev_battery_chemistry.NMC
		else:
			print("Invalid battery chemistry: {}. Skipping...".format(tokens[1]))
			continue
			
		# -------------------------------------
		
		battery_size_kWh = float(tokens[2])		
		EV_charge_rate_kW = float(tokens[3])
		
		pev_charge_profile = factory_obj.get_L1_or_L2_charge_profile(bat_chemistry, battery_size_kWh, EV_charge_rate_kW)
		
		# -------------------------------------
		
		write_charge_profile(False, fout_data, fout_code, 0, EV_enum, pev_charge_profile)
		L1_L2_charge_profile_list.append([EV_enum, pev_charge_profile])
		
	plot_L1_L2_charge_profile(L1_L2_charge_profile_list)


