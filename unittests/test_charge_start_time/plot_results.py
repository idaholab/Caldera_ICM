import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv(
    "inout/icm/outputs/charge_start_time_charging_profile.csv",
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
for i in range(0,len(SE_df)):
    row = SE_df.iloc[[i]]
    SE_id = int(row["SE_id"].item())
    node_id_name = str(row["node_id"].item())
    seid_to_nodeidname_map[SE_id] = node_id_name

# node_id_name -> (dwell_start_hrs, dwell_end_hrs, charge_start_hrs)
nodeidname__to__dwell_and_charge_start__map = dict()
for i in range(0,len(CE_df)):
    row = CE_df.iloc[[i]]
    SE_id = int(row["SE_id"].item())
    dwell_start_hrs = float(row["start_time"].item())
    dwell_end_hrs = float(row["end_time_prk"].item())
    charge_start_hrs = float(row["charge_start_time"].item())
    nodeidname__to__dwell_and_charge_start__map[ seid_to_nodeidname_map[SE_id] ] = (dwell_start_hrs, dwell_end_hrs, charge_start_hrs)


time_hrs = df["simulation_time_hrs"].to_numpy()

node_id_names = list(nodeidname__to__dwell_and_charge_start__map.keys())
max_power_kW = max(df[node_id_name].max() for node_id_name in node_id_names)

# One subplot per charge event, sharing the time axis so the shift in start time is easy to compare.
fig, axs = plt.subplots( len(node_id_names), 1, figsize=(11,3*len(node_id_names)), sharex=True )

for ax, node_id_name in zip(axs, node_id_names):

    energy_kW = df[node_id_name].to_numpy()
    dwell_start_hrs, dwell_end_hrs, charge_start_hrs = nodeidname__to__dwell_and_charge_start__map[ node_id_name ]

    # The dwell period bar (EV is plugged in).
    ax.axvspan(dwell_start_hrs, dwell_end_hrs, color="yellow", alpha=0.4)

    # The charge start time.
    ax.axvline(charge_start_hrs, color="red", linestyle="--")

    # The power profile.
    ax.plot(time_hrs, energy_kW)
    ax.set_title("Charge event: "+node_id_name+"   (arrival "+str(dwell_start_hrs)+" hrs, charge start "+str(charge_start_hrs)+" hrs, departure "+str(dwell_end_hrs)+" hrs)")
    ax.set_ylabel("Power (kW)")
    ax.set_ylim((0.0, max_power_kW*1.2))

    ax.legend(["Dwell Period","Charge Start","Power Profile (kW)"], loc="upper right")

axs[-1].set_xlabel("Time (hrs)")
axs[-1].set_xlim((0.0, 12.0))

fig.tight_layout()
fig.savefig("figs/fig_charge_start_time.png")

#plt.show()
