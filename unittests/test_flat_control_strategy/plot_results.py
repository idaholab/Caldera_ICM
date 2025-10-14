import pandas as pd
import sys
import json
import numpy
import matplotlib.pyplot as plt
from datetime import datetime
import warnings
import time
import os
import math

## For now these are hard-coded to match what I put in 'test_ToU_ALAP_control_strategy.cpp'.
ToU_start_hrs = 11.0
ToU_end_hrs = 16.0

df = pd.read_csv(
    "inout/icm/outputs/FLAT_charging_profile.csv",
    keep_default_na = False,
    index_col = False
)

SE_df = pd.read_csv(
    "inout/icm/inputs/SE_ICM.csv",
    keep_default_na = False,
    index_col = False
)

CE_df = pd.read_csv(
    "inout/icm/inputs/CE_ICM.csv",
    keep_default_na = False,
    index_col = False
)

seid_to_nodeidname_map = dict()
nodeidname_to_seid_map = dict()
for i in range(0,len(SE_df)):
    row = SE_df.iloc[[i]]
    SE_id = int(row["SE_id"].item())
    node_id_name = str(row["node_id"].item())
    seid_to_nodeidname_map[SE_id] = node_id_name
    nodeidname_to_seid_map[node_id_name] = SE_id

nodeidname__to__dwellstarthrs_dwellendhrs_pair__map = dict()
for i in range(0,len(CE_df)):
    row = CE_df.iloc[[i]]
    SE_id = int(row["SE_id"].item())
    dwell_start_hrs = float(row["start_time"].item())
    dwell_end_hrs = float(row["end_time_prk"].item())
    nodeidname__to__dwellstarthrs_dwellendhrs_pair__map[ seid_to_nodeidname_map[SE_id] ] = (dwell_start_hrs,dwell_end_hrs)


time_hrs = df["simulation_time_hrs"].to_numpy()

fig_size = (11,6)
n_tests = 10

for charge_event_i in range(1,n_tests+1):
    
    node_id_name = "home"+str(charge_event_i)

    energy_kW = df[node_id_name].to_numpy()
    
    dwell_start_hrs = nodeidname__to__dwellstarthrs_dwellendhrs_pair__map[ node_id_name ][0]
    dwell_end_hrs = nodeidname__to__dwellstarthrs_dwellendhrs_pair__map[ node_id_name ][1]

    # Create a figure and two subplots stacked vertically
    fig, (ax1) = plt.subplots( 1, 1, figsize=fig_size, sharex=False )

    # The Time-of-use bar.
    bar_x_center = (ToU_start_hrs + ToU_end_hrs) / 2
    bar_width = ToU_end_hrs - ToU_start_hrs
    bar_height = energy_kW.max()*1.7
    plt.bar(bar_x_center, bar_height, width=bar_width, align='center', color="green", alpha=0.8)
    
    # The dwell period bar.
    bar_x_center = (dwell_start_hrs + dwell_end_hrs) / 2
    bar_width = dwell_end_hrs - dwell_start_hrs
    bar_height = energy_kW.max()*1.45
    plt.bar(bar_x_center, bar_height, width=bar_width, align='center', color="yellow", alpha=0.6)

    # The power profile.
    ax1.plot(time_hrs,energy_kW)
    ax1.set_title("Charge event: "+node_id_name)
    ax1.set_xlabel("Time (hrs)")
    ax1.set_ylabel("Power (kW)")
    ax1.set_xlim((8.0,19.0))
    
    ax1.legend(["Power Profile (kW)","Time-of-Use","Dwell Period"],loc="upper right")
    
    fig.savefig("figs/fig_"+node_id_name+".png")


    

#plt.show()
