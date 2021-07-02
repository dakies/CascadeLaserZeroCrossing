# %% LeCroy testing grounds

import numpy as np
import matplotlib.pyplot as plt
import sys
import os
from rp_stream_threaded import SocketClientThread, ClientCommand, ClientReply
import time

from threading import Thread


# Define the Thread class
# Todo: Who wrote this class???

class Acquisition(Thread):
    def __init__(self, client):
        Thread.__init__(self)
        self.running = True
        self.data_ch1 = []
        self.data_ch2 = []
        self.params = []
        self.client = client
        self.dataframe = 0

    def run(self):
        while self.running is True:
            if self.client.reply_q.qsize():
                a = self.client.reply_q.get()
                if a.type == 0:  # ERROR
                    print(a.data)
                    print("ERROR: qsize ", self.client.reply_q.qsize())
                    time.sleep(1)
                    print('0')  # client.cmd_q.put(ClientCommand(ClientCommand.CONNECT, SERVER_ADDR))
                if a.type == 1:  # DATA
                    self.dataframe += 1
                    # t=a.data['params']['timestamp']
                    # time_u.append(t)
                    params_temp = a.data['params']
                    data1_temp = a.data['bytes_data1']
                    data2_temp = a.data['bytes_data2']
                    self.data_ch1.append(data1_temp)
                    self.data_ch2.append(data2_temp)
                    self.params.append(params_temp)
                if a.type == 2:  # MESSAGE
                    print("MESSAGE: qsize ", client.reply_q.qsize())
                    print(a.data)

    def stop(self):
        self.running = False
        return self.data_ch1, self.data_ch2, self.params


# %% Try establishing a connection with RP

dataframe = 0  # counter for number of data packets
dataframeN = 100
LOST = 0

QUEUE_DEPTH = 1000
SERVER_ADDR = '169.254.248.16', 8900

client = SocketClientThread(QUEUE_DEPTH)
client.start()
client.cmd_q.put(ClientCommand(ClientCommand.CONNECT, SERVER_ADDR))

# %% Initialize the Thread
launch_acq = Acquisition(client)

## %% Make Scan
for ___ in range(1):  # No of scans
    time.sleep(0.2)
    launch_acq.start()
    time.sleep(2)
    hene, mct, params = launch_acq.stop()
    hene = np.ndarray.flatten(np.array(hene))
    mct = np.ndarray.flatten(np.array(mct))
    print(len(hene))

# %%
# %%
plt.figure()
plt.plot(np.flipud(hene), lw=.4)
plt.plot(np.flipud(mct), lw=.4, alpha=0.5)
plt.show()
