# Bluetooth-Proximity-Lock
A lock that unlocks/locks from proximity of a bluetooth device typically a phone. Designed for Raspberry Pi 3.

# Instructions -- Save all files in same folder 

1. Install bluetooth library: $ sudo apt-get install libbluetooth-dev

2. Install wiringPi:                     $ sudo apt-get purge wiringpi <br />
                                         $ hash -r
                                         
3. Connect your phone to your Raspberry Pi bluetooth

4. Change Bluetooth Unique ID phone[19] to the unique ID of your bluetooth device.

5. Compile detectBTServer.c:             $ gcc -o detectBTServer detectBTServer.c

6. Compile and execute detectBT.c:       $ gcc -o detectBT detectBT.c -lwiringPi -lbluetooth <br />
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;$ sudo ./detectBT
                                                                                  
                                         
7. Compile and execute detectBTClient.c: $ gcc -o detectBTClient detectBTClient.c <br />
                                         $ sudo ./detectBTClient
