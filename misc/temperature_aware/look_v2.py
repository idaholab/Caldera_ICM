import numpy
import pandas as pd
import matplotlib.pyplot as plt
import os
import sys
import datetime
import glob

fig = plt.figure(figsize=(18,12.5))

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
    
    #p1 = ddf["P1 | kW"].to_numpy()
    p2 = ddf["P2 | kW"].to_numpy()
    #p3 = ddf["P3 | kW"].to_numpy()
    
    k = 0
    #plots_list[k].plot( soc, p1 )
    plots_list[k].plot( soc, p2 )
    #plots_list[k].plot( soc, p3 )
    plots_list[k].set_xlabel("SOC")
    plots_list[k].set_ylabel("Power (kW)")
    plots_list[k].set_title("SOC vs Power")
    plots_list[k].set_xlim((0,100))
    
    k = 1
    #plots_list[k].plot( time_hrs, p1 )
    plots_list[k].plot( time_hrs, p2 )
    #plots_list[k].plot( time_hrs, p3 )
    plots_list[k].set_xlabel("Time (hrs)")
    plots_list[k].set_ylabel("Power (kW)")
    plots_list[k].set_title("Time vs. Power")
    
    k = 2
    plots_list[k].plot( time_hrs, soc )
    plots_list[k].set_xlabel("Time (hrs)")
    plots_list[k].set_ylabel("SOC")
    plots_list[k].set_title("Time vs. SOC")


# Set the legends.
plots_list[0].legend(legend_list)
plots_list[1].legend(legend_list)
plots_list[2].legend(legend_list)

fig.subplots_adjust(hspace=0.4)

#plt.show()


######
######
######
######
######
######

fig1 = plt.figure(figsize=(18,12.5))

plots_list1 = []
for i in range(0,3):
    plots_list1.append( plt.subplot2grid((3, 1), (i, 0), rowspan=1, colspan=3) )

fig2 = plt.figure(figsize=(18,12.5))

plots_list2 = []
for i in range(0,2):
    plots_list2.append( plt.subplot2grid((2, 1), (i, 0), rowspan=1, colspan=2) )


list_of_results_file_names = [
    "outputs/sim_results_using_callback_v1.csv",
    "outputs/sim_results_using_callback_v2.csv",
    "outputs/sim_results_using_callback_v3.csv",
    "outputs/sim_results_using_callback_v4.csv"
]

legend_strs_list = []
for results_file_name in list_of_results_file_names:
    legend_strs_list.append(results_file_name[-6:-4])

for results_file_name in list_of_results_file_names:

    # Load the results file.
    rdf = pd.read_csv( results_file_name, keep_default_na = False, index_col = False )
    # time_sec,soc,power_kW,temperature_C,temperature_grad_dTdt
    time_sec = rdf["time_sec"].to_numpy()
    time_min = time_sec / 60.0
    time_hrs = time_sec / 3600.0
    soc = rdf["soc"].to_numpy()
    power_kW = rdf["power_kW"].to_numpy()
    temperature_C = rdf["temperature_C"].to_numpy()
    temperature_grad_dTdt = rdf["temperature_grad_dTdt"].to_numpy()

    # The first plot is Time vs Power.
    k = 0
    plots_list1[k].plot( time_min, power_kW )
    plots_list1[k].set_title("Time vs. Power_kW")
    plots_list1[k].set_xlabel("Time (min)")
    plots_list1[k].set_ylabel("Power (kW)")
    plots_list1[k].legend(legend_strs_list)

    # The second plot is Time vs Temperature
    k = 1
    plots_list1[k].plot( time_min, temperature_C )
    plots_list1[k].set_title("Time vs. temperature_C")
    plots_list1[k].set_xlabel("Time (min)")
    plots_list1[k].set_ylabel("Temperature (C)")
    plots_list1[k].legend(legend_strs_list)

    # The third plot is Time vs SOC
    k = 2
    plots_list1[k].plot( time_min, soc )
    plots_list1[k].set_title("Time vs. SOC")
    plots_list1[k].set_xlabel("Time (min)")
    plots_list1[k].set_ylabel("SOC")
    plots_list1[k].set_ylim((0,100))
    plots_list1[k].legend(legend_strs_list)

    fig1.subplots_adjust(hspace=0.4)


    # The first plot is SOC vs Power
    k = 0
    plots_list2[k].plot( soc, power_kW )
    plots_list2[k].set_title("SOC vs. Power_kW")
    plots_list2[k].set_xlabel("SOC")
    plots_list2[k].set_ylabel("Power (kW)")
    plots_list1[k].legend(legend_strs_list)

    # The second plot is Power vs temperature-gradient.
    k = 1
    plots_list2[k].plot( power_kW, temperature_grad_dTdt )
    plots_list2[k].set_title("Power_kW vs. temperature_grad_dTdt")
    plots_list2[k].set_xlabel("Power (kW)")
    plots_list2[k].set_ylabel("Temperature gradient (dT/dt)")
    plots_list1[k].legend(legend_strs_list)

    fig2.subplots_adjust(hspace=0.4)



plt.show()


































#