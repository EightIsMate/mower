from math import cos, sin, pi, floor
from adafruit_rplidar import RPLidar
from camera import take_pic
import serial
import time
PORT_NAME = '/dev/ttyUSB0'

ser = serial.Serial("/dev/ttyUSB1", 115200, timeout = 1)

ser.setDTR(False)
time.sleep(1)
ser.flushInput()
ser.setDTR(True)
time.sleep(2)

lidar = RPLidar(None, PORT_NAME, timeout = 3)

averageAngle = 0

def process_data(data):
	for i in range(len(data)):
		if i < 45 or i >= 315: 
		    global averageAngle
		    if data[i] < 300: #Object closer than 30 cm
			    data[i] = 0
			    #take_pic()
			    ser.write(b'L') # notify mower that an object is detected
		    print('%d: Got %d mm measurments' % (i, data[i]))
	
scan_data = [0]*360

try:
    #print(lidar.info)
    for scan in lidar.iter_scans():
        for (_, angle, distance) in scan:
           # print("angle: ", angle)
            #print("distance: ", distance)
            #print(f'Angle: {angle} Distance: {distance}')
            scan_data[min([359, floor(angle)])] = distance
        process_data(scan_data)

except KeyboardInterrupt:
    print('Stoping.')  
    
    
lidar.stop()
lidar.stop_motor()
lidar.disconnect()
