import numpy as np
import matplotlib.pyplot as plt
import time
from filter import *
import socket

# GPU 10.0.0.10

online = 1

# initial values for X and P
X = np.array([1, 1, 1])
# P = np.array([n_phi_0, n_omega_0, n_A_0, n_od_0])
P = np.array([0, 0, 0, 0])
ts = 0.0001
kf = KalmanFilter(X, P, ts=ts)

if online == 1:
    # Set up connection
    UDP_IP = "10.0.0.11"

    UDP_PORT = 8900

    sock = socket.socket(socket.AF_INET,  # Internet
                         socket.SOCK_DGRAM)  # UDP
    sock.bind((UDP_IP, UDP_PORT))
    while True:
        try:
            # Attempt to receive up to 1024 bytes of data
            data, addr = sock.recvfrom(1024)
            print(data)
        except socket.error:
            print('No data')
        # now = time.time()
        # y, x, cross = kf.filter(data[i])
        # elapsed = time.time() - now  # how long was it running?

        # dur.append(elapsed)
        # res.append(x[2] * kf.binsin(x[0]))
        # time.sleep(ts - elapsed)  # sleep accordingly so the full

else:
    signal = np.load('signal.npy')
    result = np.zeros(len(signal))
    phase = np.zeros(len(signal))
    pred_zc = np.zeros(len(signal))
    cov = np.zeros([4, len(signal)])
    ampl = np.zeros(len(signal))
    # ctrl = np.zeros(len(times))
    ctrl = np.zeros([3, len(signal)])
    freq = np.zeros(len(signal))
    # Iterate through all time stess
    res = list()
    dur = list()
    for i in range(0, len(signal)):
        now = time.time()
        y, x, cross = kf.filter(signal[i])
        elapsed = time.time() - now  # how long was it running?
        dur.append(elapsed)
        res.append(x[2] * kf.binsin(x[0]))
        # time.sleep(ts - elapsed)  # sleep accordingly so the full iteration takes 1 seconds
    plt.plot(res)
    plt.plot(signal)
    plt.show()

    plt.plot(dur)
    plt.show()
