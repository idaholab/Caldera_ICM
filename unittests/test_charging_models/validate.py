# -*- coding: utf-8 -*-
"""
Created on Sat May 20 22:27:35 2023

@author: CEBOM
"""

import glob
import pandas as pd
import numpy as np
#import matplotlib.pyplot as plt

files = glob.glob("*.csv")

for file in files:
    original_file = "original_dxfc_models/" + file
    
    original_df = pd.read_csv(original_file)
    new_df = pd.read_csv(file)
    
    tolerance = 1
    bool_array = np.isclose(original_df, new_df, rtol=tolerance, atol=tolerance)
    count_false = np.count_nonzero(bool_array == False)
    count_true = np.count_nonzero(bool_array == True)
    
    error_rate = (count_false/(count_true+count_false))*100
    
    #equal = np.allclose(original_df, new_df, atol=tolerance)
    
    # Check if the DataFrames are equal
    if error_rate < 0.5:
        print("{} The DataFrames are equal within the tolerance.".format(file.split(".")[0]))
    else:
        print("{} The DataFrames are not equal within the tolerance.".format(file.split(".")[0]))
        exit(1)
        
exit(0)

    
#    fig = plt.figure(figsize=(8, 6))
#    plt.plot(original_df["simulation_time_hrs"], original_df[" soc_t1"], label = "original")
#    plt.plot(new_df["simulation_time_hrs"], new_df[" soc_t1"], label = "new")
    
#    plt.plot(original_df["simulation_time_hrs"], original_df[" P2_kW"], label = "original")
#    plt.plot(new_df["simulation_time_hrs"], new_df[" P2_kW"], label = "new")
    
#    plt.legend()
#    plt.show()

#fig.savefig("test")