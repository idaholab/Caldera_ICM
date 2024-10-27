import numpy as np
numpy = np
import matplotlib.pyplot as plt
import pandas as pd

filenames_all = [
    "pevtype_ngp_audi_etron_55_quattro__setype_dcfc_50.csv",
    "pevtype_ngp_audi_etron_55_quattro__setype_xfc_150.csv",
    "pevtype_ngp_audi_etron_55_quattro__setype_xfc_350.csv",
    "pevtype_ngp_genesis_gv60__setype_dcfc_50.csv",
    "pevtype_ngp_genesis_gv60__setype_xfc_150.csv",
    "pevtype_ngp_genesis_gv60__setype_xfc_350.csv",
    "pevtype_ngp_hyundai_ioniq_5_longrange_awd__setype_dcfc_50.csv",
    "pevtype_ngp_hyundai_ioniq_5_longrange_awd__setype_xfc_150.csv",
    "pevtype_ngp_hyundai_ioniq_5_longrange_awd__setype_xfc_350.csv",
    "pevtype_ngp_porsche_taycan_plus__setype_dcfc_50.csv",
    "pevtype_ngp_porsche_taycan_plus__setype_xfc_150.csv",
    "pevtype_ngp_porsche_taycan_plus__setype_xfc_350.csv"
]
filenames_50 = []
for f in filenames_all:
    if( "dcfc_50" in f ):
        filenames_50.append(f)

filenames_150 = []
for f in filenames_all:
    if( "xfc_150" in f ):
        filenames_150.append(f)

filenames_350 = []
for f in filenames_all:
    if( "xfc_350" in f ):
        filenames_350.append(f)
        
filenames_only_genisis350 = [
    "pevtype_ngp_genesis_gv60__setype_xfc_350.csv"
]

filenames = filenames_350
#filenames = filenames_only_genisis350

df0 = pd.read_csv( "./outputs/"+filenames[0], keep_default_na = False, index_col = False )
time_col_name = list(df0.columns)[0]
value_col_names_list = list(df0.columns)[1:4]
value_col_names_list.reverse()

fig = plt.figure(figsize=(18,13.5))
plots_list = []
n_plots = len(value_col_names_list)+1
for i in range(0,n_plots):
    plots_list.append( plt.subplot2grid((n_plots, 1), (i, 0), rowspan=1, colspan=n_plots) )

for i,plotn in enumerate(plots_list):
    if( i < n_plots-1 ):
        value_name = value_col_names_list[i]
        for filename in filenames:
            df = pd.read_csv( "./outputs/"+filename, keep_default_na = False, index_col = False )
            plotn.plot( df[time_col_name], df[value_name] )
        plotn.set_title(value_name)
        plotn.set_xlabel(time_col_name)
        plotn.set_ylabel(value_name)
    else:
        for filename in filenames:
            df = pd.read_csv( "./outputs/"+filename, keep_default_na = False, index_col = False )
            plotn.plot( df[list(df0.columns)[1]], df[list(df0.columns)[2]] )
        plotn.set_title("SOC vs Power")
        plotn.set_xlabel("SOC")
        plotn.set_ylabel("Power (kW)")

fig.subplots_adjust(hspace=0.5)
plt.legend(filenames)
plt.show()




