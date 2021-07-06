import matplotlib.pyplot as plt
import numpy as np

cpu_times = np.loadtxt('time_log.txt')

fig, axs = plt.subplots(2, 1)
axs[0].plot(cpu_times)
axs[0].set_title('CPU time for single Kalman filter itertion')
axs[0].set_ylabel('CPU time (microseconds)')
axs[0].set_ylabel('Iteration')

axs[1].hist(cpu_times)
axs[1].set_title('CPU time for single Kalman filter itertion')
axs[1].set_ylabel('Number of iterations')
axs[1].set_xlabel('CPU Time')

plt.show()
