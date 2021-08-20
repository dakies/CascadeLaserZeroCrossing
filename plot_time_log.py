import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import PercentFormatter

plt.figure(figsize=(10, 6), dpi=200)
cpu_times = np.loadtxt('time_log_tot.txt')
cpu_times = cpu_times[:5000]
cpu_times_avg = [np.mean(cpu_times)] * len(cpu_times)
fig, axs = plt.subplots(2, 1)
axs[0].plot(cpu_times)
axs[0].set_ylim([0, 15])
axs[0].set_title('CPU time required for fetching data from buffer per iteration')
axs[0].set_ylabel('CPU time (microseconds)')
axs[0].set_xlabel('Iteration')
axs[0].plot(range(len(cpu_times)), cpu_times_avg, color='red', ls='--', lw=2, label="Avergae CPU time")
cpu_times_max = [4] * len(cpu_times)
axs[0].plot(range(len(cpu_times_max)), cpu_times_max, color='lime', ls='--', lw=2, label="Max. CPU time")
axs[0].legend()

axs[1].hist(cpu_times, weights=np.ones(len(cpu_times)) / len(cpu_times), bins=30, range=(0, 30))
plt.gca().yaxis.set_major_formatter(PercentFormatter(1))
# axs[1].bar_label(axs[1].containers[0], label_type='edge')
axs[1].set_title('CPU time required for fetching data from buffer per iteration')
axs[1].set_ylabel('Number of iterations')
axs[1].set_xlabel('CPU Time (microseconds)')

plt.show()
