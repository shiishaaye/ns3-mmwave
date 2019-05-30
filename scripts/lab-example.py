import sem
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

ns_path = '../ns3-mmwave'
campaign_dir = './lab-example'
script = 'mmwave-lab-example'

campaign = sem.CampaignManager.new(ns_path, script, campaign_dir, overwrite=True, check_repo=False)

print (campaign)

runs = list (range (1,10,1))
distances = list (range (10,1000,100))
param_combination = {"RngRun" : runs, "distance" : distances}

campaign.run_missing_simulations (sem.list_param_combinations(param_combination))

avg_sinr = np.zeros (len(distances))
avg_s = np.zeros (len(distances))

for dist in distances :
    avg_sinr_run = np.zeros (len(runs))
    avg_s_run = np.zeros (len(runs))

    for run in runs :
        params = {'RngRun': run, 'distance' : dist}
        available_files = campaign.db.get_result_files (campaign.db.get_results (params=params)[0])

        sinr_data = pd.read_csv (available_files['RxPacketTrace.txt'], delimiter = "\t", usecols = [0, 12], names = ['mode', 'sinr'], skiprows=1)
        sinr_data = sinr_data [sinr_data['mode'] == 'DL']
        avg_sinr_run[runs.index (run)] = sinr_data.mean (axis=0)['sinr']

        s_data = pd.read_csv (available_files['DlRlcStats.txt'], delimiter = " ", usecols = [0, 5], names = ['mode', 'size'], skiprows=1)
        s_data = s_data [s_data['mode'] == 'Rx']
        avg_s_run[runs.index (run)] = s_data.sum (axis=0, skipna = True)['size']*8/0.5

    avg_sinr[distances.index (dist)] = np.average (avg_sinr_run)
    avg_s[distances.index (dist)] = np.average (avg_s_run)


plt.figure (1)
plt.subplot (211)
plt.plot (distances, avg_sinr)
plt.ylabel ('Average SINR [dB]')
plt.subplot (212)
plt.plot (distances, avg_s)
plt.ylabel ('Average S [bit/s]')
plt.xlabel ('Distance [m]')

plt.show ()
