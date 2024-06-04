# -*- coding: utf-8 -*-
import os
import sys
import pandas as pd
import numpy as np

path_to_here = os.path.abspath(os.path.dirname(sys.argv[0]))
path_to_libs = os.path.join(path_to_here, "..", "..", "libs")
path_to_inputs = os.path.join(path_to_here, "inputs")

index = 1
sys.path.insert( index+0, path_to_libs )

from Caldera_ICM_Aux import CP_interface_v2
from Caldera_globals import all_charge_profile_data


######################################################
# 	Build Charge Profile Given Start and End SOC
######################################################

L1_timestep_sec = 1
L2_timestep_sec = 1
HPC_timestep_sec = 1
ramping_by_pevType_only = {}
ramping_by_pevType_seType = {}

ICM = CP_interface_v2(path_to_inputs, L1_timestep_sec, L2_timestep_sec, HPC_timestep_sec, ramping_by_pevType_only, ramping_by_pevType_seType)

pev_type = "bev250_350kW"
SE_type = "xfc_350"
start_soc = 10
end_soc = 90

all_profile_data =  ICM.get_all_charge_profile_data(start_soc, end_soc, pev_type, SE_type)

start_time = 0
end_time = len(all_profile_data.P1_kW) * all_profile_data.timestep_sec

df = pd.DataFrame()
df["time | sec"] = np.arange(start_time, end_time, all_profile_data.timestep_sec)
df["SOC | "] = all_profile_data.soc
df["P1 | kW"] = all_profile_data.P1_kW
df["P2 | kW"] = all_profile_data.P2_kW
df["P3 | kW"] = all_profile_data.P3_kW
df["Q3 | kVAR"] = all_profile_data.Q3_kVAR

df.to_csv("{}_{}_{}_{}.csv".format(pev_type, SE_type, start_soc, end_soc), index = False)

