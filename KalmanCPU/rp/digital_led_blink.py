
#!/usr/bin/python

import sys
import time
import redpitaya_scpi as scpi

rp_s = scpi.scpi('192.168.1.100', port=8900)

led = 0

print ("Blinking LED["+str(led)+"]")

period = 1 # seconds

while 1:
    time.sleep(period/2.0)
    rp_s.tx_txt('DIG:PIN LED' + str(led) + ',' + str(1))
    time.sleep(period/2.0)
    rp_s.tx_txt('DIG:PIN LED' + str(led) + ',' + str(0))