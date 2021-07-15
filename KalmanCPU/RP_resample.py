import numpy as np
import matplotlib.pyplot as plt
from KalmanCPU.rp_stream_threaded import SocketClientThread, ClientCommand, ClientReply
from KalmanCPU.filter import KalmanFilter
import time
from threading import Thread

class zero_cross_detector(Thread):
    def __init__(self, kf:KalmanFilter):
        Thread.__init__(self)
        self.state = []
        self.filt = []
        self.half_phase_prev = 0
        self.running = True
        self.data_ch1 = []
        self.data_ch2 = []
        self.params = []
        self.client = client
        self.dataframe = 0
        self.crossing = []
        self.resampled = []
        self.kf = kf

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
                    params = a.data['params']
                    data1 = a.data['bytes_data1']
                    data2 = a.data['bytes_data2']

                    for i, z in np.ndenumerate(data1):
                        # Filter
                        x, p = self.kf.filter(z)
                        # Check phase
                        if x[0] > np.pi:
                            half_phase = 1
                        else:
                            half_phase = 0
                        # Detect zero crossing on half phase change
                        if half_phase != self.half_phase_prev:
                            self.resampled.append(data2[i])
                            self.crossing.append(1)
                        else:
                            self.crossing.append(0)
                        self.filt.append(x[2]*np.sin(x[0]))
                        self.state.append(x)
                    # New prev half phase
                    self.half_phase_prev = half_phase

                    # Save data
                    self.data_ch1.append(data1)
                    self.data_ch2.append(data2)
                    self.params.append(params)
                if a.type == 2:  # MESSAGE
                    print("MESSAGE: qsize ", client.reply_q.qsize())
                    print(a.data)

    def stop(self):
        self.running = False
        return self.data_ch1, self.data_ch2, self.params, self.crossing, self.filt, self.resampled, self.state


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
kf = KalmanFilter()
launch_acq = zero_cross_detector(kf)


launch_acq.start()
# time.sleep(10)
name = input('Press enter to end and save measurement \n')
hene, mct, params, crossing, filt, resampled, state = launch_acq.stop()
hene = np.ndarray.flatten(np.array(hene))
mct = np.ndarray.flatten(np.array(mct))

print('No of recorded HeNe samples')
print(len(hene))
print('No of resampled QCL samples')
print(len(resampled))
plt.figure()
plt.plot(np.flipud(hene[100:200]), lw=.4, label='HeNe')
# plt.plot(np.flipud(mct), lw=.4, alpha=0.5)
plt.plot(10*np.array(crossing[100:200]), label='Zero Cross')
plt.plot(filt[100:200], label='Filt. HeNe')
plt.legend()

plt.figure()
plt.plot(np.array(state)[:, 1])
plt.title('Freq')
plt.show()

plt.figure()
plt.plot(np.array(state)[:, 2])
plt.title('Amplitude')
plt.show()

# Save
np.save('data/hene', hene)
np.save('data/filt', filt)
np.save('data/cross', crossing)
np.save('data/sampled', resampled)

