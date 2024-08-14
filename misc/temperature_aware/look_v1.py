import numpy
import pandas as pd
import matplotlib.pyplot as plt
import os
import sys
import datetime
import glob

fig = plt.figure(figsize=(18,14))

plots_list = []
for i in range(0,3):
    plots_list.append( plt.subplot2grid((3, 1), (i, 0), rowspan=1, colspan=3) )

legend_list = []

# The index file.
idf = pd.read_csv( "outputs/index_to_c_rate_scale_factor_lookup.csv", keep_default_na = False, index_col = False )
# Loop over each index, and read the corresponding file.
for i in range(0,len(idf)):
    row = idf.iloc[[i]]
    index = int(row["index"].item())
    power_level_scale_factor = float(row["c_rate_scale_factor"].item())
    
    legend_list.append("Scale Factor "+str(power_level_scale_factor))
    
    # Read the corresponding file.
    ddf = pd.read_csv( "outputs/pevtype_ngp_hyundai_ioniq_5_longrange_awd_100__setype_xfc_350__pi_"+str(index).zfill(3)+".csv", keep_default_na = False, index_col = False )
    # Header:  time | hrs,SOC | ,P1 | kW,P2 | kW,P3 | kW,Q3 | kVAR
    
    time_hrs = ddf["time | hrs"].to_numpy()
    soc = ddf["SOC | "].to_numpy()
    p1 = ddf["P1 | kW"].to_numpy()
    p2 = ddf["P2 | kW"].to_numpy()
    p3 = ddf["P3 | kW"].to_numpy()
    
    k = 0
    plots_list[k].plot( soc, p3 )
    plots_list[k].set_xlabel("SOC")
    plots_list[k].set_ylabel("Power (kW)")
    plots_list[k].set_title("Power vs. SOC")
    
    k = 1
    #plots_list[k].plot( time_hrs, p1 )
    #plots_list[k].plot( time_hrs, p2 )
    plots_list[k].plot( time_hrs, p3 )
    plots_list[k].set_xlabel("Time (hrs)")
    plots_list[k].set_ylabel("Power (kW)")
    plots_list[k].set_title("Power vs. Time")
    
    k = 2
    plots_list[k].plot( time_hrs, soc )
    plots_list[k].set_xlabel("Time (hrs)")
    plots_list[k].set_ylabel("SOC")
    plots_list[k].set_title("SOC vs. Time")


# Set the legends.
plots_list[0].legend(legend_list)
plots_list[1].legend(legend_list)
plots_list[2].legend(legend_list)

fig.subplots_adjust(hspace=0.4)

plt.show()