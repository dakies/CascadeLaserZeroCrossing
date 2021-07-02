# %% LeCroy testing grounds

import numpy as np
import matplotlib.pyplot as plt
import sys
import os
import time
from threading import Thread
from rp_stream_threaded import SocketClientThread, ClientCommand, ClientReply


# utilpath = 'Z:\lab\meas\BarbaraSchneider\Pacman_Scripts'
# sys.path.append(utilpath)
# import instr.lecroy as osc

# from instr.np_stage.stageCommands_np import npstage
# import visa


# Define the Thread class

# class MovingStage(Thread):
#     def __init__(self, stage, l):
#         Thread.__init__(self)
#         self.stage = stage
#         self.l = l
#     def run(self):
#         self.stage.stageScan(scanLength=self.l)

class Acquisition(Thread):
    def __init__(self, client):
        Thread.__init__(self)
        self.running = True
        self.data = []
        self.client = client
        self.dataframe = 0

    def run(self):
        while self.running == True:
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
                    data_temp = a.data['bytes_data1']
                    self.data.append(data_temp)
                if a.type == 2:  # MESSAGE
                    print("MESSAGE: qsize ", client.reply_q.qsize())
                    print(a.data)

    def stop(self):
        self.running = False
        return self.data


# %% Try establishing a connection with RP

dataframe = 0  # counter for number of data packets
dataframeN = 100
LOST = 0

QUEUE_DEPTH = 1000
SERVER_ADDR = '169.254.248.17', 8900

client = SocketClientThread(QUEUE_DEPTH)
client.start()
client.cmd_q.put(ClientCommand(ClientCommand.CONNECT, SERVER_ADDR))

# %% Try establishing a connection with SA
#
# address = 'COM4'
# try:
#     stage = npstage(address)
# except:
#     print('See if no other program has stage open')
#     raise
#
# v = 5 # mm/s
# l = 100
# # l = 35 # mm
# # %% Initialize
# if stage.roughpos() != 0:
#     stage.movAbs_wait(0)
#
# # %% Initialize the Thread
#
# launch_sta = MovingStage(stage, l)
launch_acq = Acquisition(client)

# %% Make Scan
for ___ in range(1):  # No of scans
    # if stage.roughpos() != 0:
    #     stage.movAbs_wait(0)
    time.sleep(0.2)
    launch_acq.start()
    # launch_sta.start()
    time.sleep(1)
    # launch_sta.join()
    data = launch_acq.stop()

# %%


# folder = r'Z:\lab\meas\Mathieu\Samples\EV2548\A\ALMG25\Sandbox'
#
# filename = 'test'
# savename = os.path.join(folder, filename)
# savename=savename + "_000"
# while(os.path.exists(str(savename)+'.npy')):
#     print("Filename exists already! Get new name ..")
#     # try:
#     num=int(savename[-3:])+1
#     savename=savename[0:-3]+"%03d" % num
#     # except ValueError:
#     #     savename=savename + "00"
# if np.size(ch) == 4:
#     np.save(savename, {'time': t, 'HeNe': raw[ch[0]], 'MCT': raw[ch[1]], 'X': raw[ch[2]], 'Y': raw[ch[3]]})
# else:
#     np.save(savename, {'time': t, 'HeNe': raw[ch[0]], 'MCT': raw[ch[1]]})

# %%
data = np.ndarray.flatten(np.array(data))
# %%
print(len(data))
# %%
plt.figure()
plt.plot(data, '-o', lw=.4)
plt.show()