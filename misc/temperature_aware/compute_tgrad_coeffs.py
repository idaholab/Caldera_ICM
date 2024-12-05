# NOTE: I am trying to reproduce the computation that was originally in a
#       Jupyter notebook called 'power_drop_multifeature.ipynb' located in
#       the ngp_dashboard/src directory.

import os
import itertools
import numpy
np = numpy
import pandas as pd
import matplotlib.pyplot as plt
import plotly.express as px
import plotly.graph_objects as go

from typing import Any, Dict, List, Tuple
from plotly.subplots import make_subplots
from scipy.optimize import curve_fit
from scipy.signal import savgol_filter
from sklearn import linear_model
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error, r2_score

data_dir = "./data_ignore"


#########################################
##           Helper Functions          ##
#########################################


# Retrieves all experiment data from the 'experiments' directory.
# Returns:
#     Tuple[List[pd.DataFrame], List[str]]: A tuple containing:
#         - A list of DataFrames, each representing an experiment's data.
#         - A list of filepaths corresponding to the experiments.
#
def get_all_experiments() -> Tuple[List[pd.DataFrame], List[str]]:
    exps_root = data_dir + "/experiments/"
    experiment_paths = []
    for root, dirs, files in os.walk(exps_root):
        for file in files:
            if file.startswith('Formatted_IONIQ_') and file.endswith('.parquet'):
                filepath = root + file
                data = file.removesuffix('.parquet').split('_')[1:]
                data.append(filepath)
                experiment_paths.append(data)
    experiments_df = pd.DataFrame( experiment_paths, columns=['vehicle_name', 'init_temp', 'init_condition', 'evse_limit', 'init_soc', 'filepath'] )
    experiments_df.sort_values(by=['vehicle_name', 'init_temp',], inplace=True)
    print("len(experiments_df['filepath']): ",len(experiments_df['filepath']) )
    sample_dfs = [pd.read_parquet(experiment) for experiment in experiments_df['filepath']]
    return sample_dfs, experiments_df['filepath']

# # Retrieves all simulation data from the 'simulations' directory.
# # Returns:
# #     Dict[Tuple[str, str], pd.DataFrame]: A dictionary where each key is a tuple of:
# #         - Electric Vehicle (EV) name
# #         - Electric Vehicle Supply Equipment (EVSE) name
# #       and each value is a DataFrame representing the simulation data.
# #
# def get_all_simulations() -> Dict[Tuple[str, str], pd.DataFrame]:
#     sims_root = data_dir + "/simulations/"
#     simulations = {}
#     for root, dirs, files in os.walk(sims_root):
#         for file in files:
#             print("   file: ",file)
#             if file.startswith('pevtype_ngp_hyundai_ioniq_5_longrange_awd') and file.endswith('.parquet'):
#                 print("got it.")
#                 df = pd.read_parquet(root + file)
#                 # add column for the SOC rate of change
#                 df['SOC gradient'] = np.gradient(df['SOC | '].values)
#                 ev_evse = file.removesuffix('.parquet').split('__')
#                 ev_evse[0] = ev_evse[0][8:]
#                 ev_evse[1] = ev_evse[1][7:]
#                 print("ev_evse: ",ev_evse)
#                 simulations[(ev_evse[0], ev_evse[1])] = df
#             else:
#                 print("skipped")
#     print("in the function 'get_all_simulations', we have len(simulations): ",len(simulations))
#     return simulations

# Retrieves experiment data matching the specified criteria.
# Args:
#     vehicle_model (str): Vehicle model name.
#     init_temp (float): Initial temperature.
#     init_condition (str): Initial condition.
#     evse_limit (float): EVSE limit.
#     init_soc (float): Initial SOC.
# Returns:
#     List[pd.DataFrame]: A list of DataFrames, each representing an experiment's data.
#
def get_experiments(vehicle_model, init_temp, init_condition, evse_limit, init_soc) -> List[pd.DataFrame]:
    exps_dir = data_dir + "/experiments/"
    experiment_paths = []
    for root, dirs, files in os.walk(exps_dir):
        for file in files:
            if file.endswith('.parquet'):
                filepath = root + file
                data = file.removesuffix('.parquet').split('_')[1:]
                data.append(filepath)

                experiment_paths.append(data)
    experiments_df = pd.DataFrame(experiment_paths, columns=['vehicle_name', 'init_temp', 'init_condition', 'evse_limit', 'init_soc', 'filepath'])
    experiments_df.sort_values(by=['vehicle_name', 'init_temp',], inplace=True)
    query_dict = get_experiment_dict(experiments_df, vehicle_model, init_temp, init_condition, evse_limit, init_soc)
    selected_experiments = experiments_df.loc[ (experiments_df[list(query_dict.keys())] == list(query_dict.values())).all(axis=1) ]
    array_of_df_filepath_pairs = [ (pd.read_parquet(experiment),experiment) for experiment in selected_experiments['filepath']]
    return array_of_df_filepath_pairs

# Creates a dictionary of experiment parameters for filtering.
# Args:
#     experiments_df (pd.DataFrame): DataFrame containing experiment data.
#     vehicle_model (str): Vehicle model name.
#     init_temp (float): Initial temperature.
#     init_condition (str): Initial condition.
#     evse_limit (float): EVSE limit.
#     init_soc (float): Initial SOC.
# Returns:
#     Dict[str, Any]: A dictionary where keys are experiment parameter names and values are the corresponding values.
#
def get_experiment_dict(experiments_df, vehicle_model, init_temp, init_condition, evse_limit, init_soc) -> Dict[str, Any]:
    experiments = zip(experiments_df.columns[:-1], [
        vehicle_model, init_temp, init_condition, evse_limit, init_soc])

    params_dict = {column: val for column,
                   val in experiments if val is not None}
    return params_dict

# Retrieves column names for power, temperature, and SOC from a DataFrame.
# Args:
#     df (pd.DataFrame): Input DataFrame containing battery data.
# Returns:
#     Tuple[str, str, str]: A tuple containing the column names for:
#         - Battery power
#         - Average battery temperature
#         - Battery state of charge (SOC)
#
def get_column_names(df) -> Tuple[str, str, str]:
    power_col = next( (col for col in df.columns if col.startswith("Battery Power")), None )
    temperature_col = next( (col for col in df.columns if col.startswith("Avg Battery Temp")), None )
    soc_col  = next( (col for col in df.columns if col.startswith("Battery Display")), None )
    return power_col, temperature_col, soc_col

# # Calculates and prints charge rates at various levels of degradation.
# # Args:
# #     baseline_c_rate (float): The initial charge rate (100% capacity)
# # Returns:
# #     None
# #
# def calc_charge_rates(baseline_c_rate) -> None:
#     for n in range(0, 10):    
#         calc_c_rate = baseline_c_rate - baseline_c_rate * (0.1 * n)
#         print(f'Calculated Charge {(10 - n) * 10}%: {calc_c_rate}')
#         print()

# Displays a dataframe.
#
def display(df,make_pyplot_table=False):
    if( make_pyplot_table ):
        # Display DataFrame as a table using pyplot
        fig, ax = plt.subplots(figsize=(10, 10))  # Set the size as needed
        ax.axis('tight')
        ax.axis('off')
        ax.table(cellText=df.values, colLabels=df.columns, loc='center')
        plt.show()
    else:
        print(df)

# ###########################################
# ## Storing the experiments as .csv files ##
# ## so I can look at them.                ##
# ###########################################
# experiments_df, filepaths = get_all_experiments()
# for i,df in enumerate(experiments_df):
#     if( filepaths[i][-8:] != ".parquet" ):
#         print("ERROR: Not a parquet file.")
#         exit()
#     filename_without_ext = filepaths[i][0:-8]
#     df.to_csv(filename_without_ext+".csv", index=False)

# #########################################
# ## Load and Preprocess Experiment Data ##
# #########################################
# experiments_df, filepaths = get_all_experiments()
# for df in experiments_df:
#     df['Avg Battery Temp [°C]'] = df['Avg Battery Temp [°C]'].replace('--', None, inplace=False)








######################################################################
## Preprocess and Visualize Experiment Data for IONIQ Vehicle Model ##
######################################################################

combined_data = []
time_col = 'Time Elapsed [hrs]'
for df,filepath in get_experiments('IONIQ', None, None, None, None):
    
    # Copy the original dataframe so we can modify it.
    df = df.copy(deep=True)
    
    # Retrieves column names for power, temperature, and SOC from a DataFrame.
    power_col, temperature_col, soc_col = get_column_names(df)
    
    # Replace -- with None
    df = df.replace('--', None)
    
    # Drop any NA values in the Time column.
    df = df.dropna(subset=['Time [hh:mm:ss.0]'])
    
    if(df[power_col].isna().all()):
        # Skip this dataframe if the power column is all NA.
        continue
    
    # -------------------------------
    # MORE CLEAN-UP of the dataframe.
    # -------------------------------
    # Make the power column all positive values.
    df[power_col] = df[power_col].abs()
    # Convert the time column to type string.
    df['Time [hh:mm:ss.0]'] = df['Time [hh:mm:ss.0]'].astype(str)
    # Create an hrs column
    df['Time Elapsed [hrs]'] = pd.to_timedelta( df['Time [hh:mm:ss.0]']).dt.total_seconds()/3600
    # Create a seconds column
    df['Time Elapsed [s]'] = pd.to_timedelta( df['Time [hh:mm:ss.0]']).dt.total_seconds()
    # Set the dataframe to only contain these five columns.
    df = df[['Time Elapsed [hrs]', 'Time Elapsed [s]', power_col, temperature_col, soc_col]]
    
    # --------------------------------------------------------
    # De-noising the power column and the temperature column.  
    # NOTE:
    #    'savegol_filter' is from scipy.
    #    Parameters for 'savegol_filter' are: savegol_filter(x, window_length, polyorder, deriv=0, delta=1.0, axis=-1, mode='interp', cval=0.0)
    #    Also see "Savitzky-Golay filter" on Wikipedia.
    # --------------------------------------------------------
    window_len = 10*60*6  # 6 minutes. The timestep is 0.1 seconds, so this would be 6 minutes.
    polyorder = 1
    df[power_col] = savgol_filter( df[power_col].abs()/1000, window_len, polyorder )
    df['temp_denoised'] = savgol_filter( df[temperature_col], window_len, polyorder )
    # Compute the gradient of the temperature using the denoised (smoothed) temperature data.
    df['temp_gradient'] = np.gradient( df['temp_denoised'] )
    
    print("filepath: ",filepath)
    
    # # Looking at the smooth vs original temperature data. 
    # fig, ax = plt.subplots(figsize=(20, 8))
    # ax.plot( df['Time Elapsed [s]'], df[temperature_col] )
    # ax.plot( df['Time Elapsed [s]'], df["temp_denoised"] )
    # ax.set_title(filepath)
    # ax.set_xlabel("Time (s)")
    # ax.set_ylabel("Temperature")
    # ax.legend(["Original","Denoised"])
    # plt.show()

    # Column-stacking the data and then adding each row of the
    # data one at a time to the 'combined_data' list.
    combined_data.extend( 
        np.column_stack(
            (
                df[power_col],
                df['temp_denoised'],
                df['Time Elapsed [s]'],
                df['Battery Display SOC [%]'],
                df['temp_gradient']
            )
        )
    )

# Building a dataframe from the list of rows, and putting column-names on them.
combined_data = pd.DataFrame(combined_data, columns=['P1 | kW', 
                                                     'temperature | ºC', 
                                                     'time | s', 
                                                     'SOC | ', 
                                                     'temperature gradient | ºC/ds']).dropna()

# Printing out what we have.
print(f'Shape of Input Data {combined_data.shape}')
display(combined_data.sample(10))









######################################################################
##  Fit the data using Ordinary Least Squares (OLS) Regression      ##
##  Train Linear Regression Model and Evaluate Performance          ##
######################################################################
# 
# This code trains a linear regression model to predict the temperature gradient based on power, temperature, time, and SOC.
# 
#  * Splits the data into training and testing sets (80% for training and 20% for testing)
#  * Initializes a linear regression model
#  * Trains the model on the training data
#  * Predicts the temperature gradient for the test data
#  * Prints the model's coefficients:
#      - power_coef:       The change in temperature gradient per unit change in power (kW)
#      - temperature_coef: The change in temperature gradient per unit change in temperature (ºC)
#      - time_coef:        The change in temperature gradient per unit change in time (s)
#      - SOC_coef:         The change in temperature gradient per unit change in SOC
#      - intercept:        The temperature gradient when all regressors are zero
#  * Calculates and prints the Mean Squared Error (MSE) between actual and predicted temperature gradients
#  * Calculates and prints the Coefficient of Determination (R-squared) to measure model accuracy


X_train, X_test, y_train, y_test = train_test_split( combined_data[['P1 | kW', 'temperature | ºC', 'time | s', 'SOC | ']],
                                                     combined_data['temperature gradient | ºC/ds'].tolist(),
                                                     test_size=0.80,
                                                     random_state=47 )

reg = linear_model.LinearRegression()

reg.fit(X_train, y_train)

y_pred = reg.predict(X_test)

intercept = reg.intercept_
fitted_curve_coefs = reg.coef_

print(f'Intercept: {intercept}\n')
print(f'Coefficient(s): {fitted_curve_coefs}')

# Mean Squared Error (MSE)
rmse = np.sqrt(mean_squared_error(y_test, y_pred))
print(f"Mean Squared Error (MSE): {rmse:.2f}")

# Coefficient of Determination (R-squared)
r2 = r2_score(y_test, y_pred)
print(f"R-squared (Accuracy): {r2:.2f}")

print("intercept: ",intercept, "  fitted_curve_coefs: ",fitted_curve_coefs)

# # Helper function to evaluate a test result.
# def eval_function_with_coeffs( intercept, fitted_curve_coefs, x_vec ):
#     ret_val = 0.0
#     ret_val += intercept
#     for i in range(0,len(x_vec)):
#         ret_val += fitted_curve_coefs[i]*x_vec[i]
#     return ret_val
# 
# N = 2000
# my_power_vec = np.arange(0,200,0.1)
# my_temperature_vec = np.ones(2000)*30
# my_time_vec = np.arange(0,3600,1.8)
# my_soc_vec = np.arange(0,100,0.05)
# output_tgrad_vec = numpy.zeros(2000)
# for i in range(0,N):
#     x_vec = [ my_power_vec[i], my_temperature_vec[i], my_time_vec[i], my_soc_vec[i] ]
#     output_tgrad_vec[i] = eval_function_with_coeffs(intercept,fitted_curve_coefs,x_vec)
# 
# plt.figure()
# plt.plot(my_time_vec,output_tgrad_vec)
# plt.legend(["time vs tgrad"])
# plt.show()



































#