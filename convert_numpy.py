import numpy as np
dat = np.load('./Simulation/hene.npy')
np.savetxt('hene.txt', dat)