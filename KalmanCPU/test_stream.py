from rp_stream_threaded import SocketClientThread, ClientCommand, ClientReply
import numpy as np
import time

#%%
timestart=time.time()  #time for frame rate
dataframe=0  #counter for number of data packets
dataframeN=100
LOST=0

QUEUE_DEPTH=1000  
SERVER_ADDR = '169.254.248.16', 8900

client = SocketClientThread(QUEUE_DEPTH)
client.start()
client.cmd_q.put(ClientCommand(ClientCommand.CONNECT, SERVER_ADDR))

#%%
data_tot = []
time_u = []
i = 0
start = time.time()
while time.time()<start+10:
    if client.reply_q.qsize():
        a=client.reply_q.get()
        if a.type==0:  #ERROR
            print(a.data)
            print("ERROR: qsize ",client.reply_q.qsize())
            time.sleep(1)
            print('0')#client.cmd_q.put(ClientCommand(ClientCommand.CONNECT, SERVER_ADDR))
        if a.type==1:   #DATA
            dataframe+=1
            t=a.data['params']['timestamp']
            time_u.append(t)
            datat=a.data['bytes_data1']
            data_tot.append(datat)
        if a.type==2:  #MESSAGE
            print("MESSAGE: qsize ",client.reply_q.qsize())
            print(a.data)


#%% 
import matplotlib.pyplot as plt 
#%% 
data = np.ndarray.flatten(np.array(data_tot))
#%%
plt.figure()
plt.plot(data, '-x')
plt.show()
#%% 
amp = np.fft.rfft(data)
fre = np.fft.rfftfreq(len(data), 1/250e3)

plt.figure()
plt.plot(fre, np.abs(amp))
