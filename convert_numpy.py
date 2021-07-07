import numpy as np
import matplotlib.pyplot as plt
dat = np.load('./Simulation/hene.npy')
dat = dat[:, :10000]
# plt.plot(dat[0, :],dat[1, :])
plt.show()
np.savetxt('hene.txt', dat[1, :])