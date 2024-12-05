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





#