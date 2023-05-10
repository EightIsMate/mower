import pyudev
import time

from math import cos, sin, pi, floor
from adafruit_rplidar import RPLidar, RPLidarException
#from rplidar import RPLidarException
#from camera import take_pic
import serial
import numpy as np

def detect_serial_devices():
	global lidar_device_port
	global mower_device_port
	global lidar
	global ser
	context = pyudev.Context() 
	devices = context.list_devices(subsystem = 'tty')
	serial_devices = []
	for device in devices:
		try:
			vendor_id = device.get('ID_VENDOR_ID')
			product_id = device.get('ID_MODEL_ID')
			if vendor_id == '10c4' and product_id == 'ea60':
				#LIDAR detected
				#serial_devices.append(device.device_node)
				lidar_device_port = device.device_node
				lidar = RPLidar(None, lidar_device_port, timeout = 3)
				print("LIDAR device: {}".format(lidar_device_port))
			elif vendor_id == '1a86' and product_id == '7523':
				#mower detected
				mower_device_port = device.device_node
				ser = serial.Serial(mower_device_port, 115200, timeout = 1)
				print("Mower device: {}".format(mower_device_port))
				ser.setDTR(False)
				time.sleep(1)
				ser.flushInput()
				ser.setDTR(True)
				time.sleep(2)
				#serial_devices.append(device.device_node)
		except:
			pass
#	return serial_devices

#def connect_serial_devices():

#	global lidar
#	global ser
#	while True:
#		serial_devices = detect_serial_devices()
#		if len(serial_devices) == 2: # change to 1 for only 1 port, else 2
			#Both devices detected
#			lidar_device_port = serial_devices[1]
			#mower_device_port = serial_devices[1]
#			break
#		time.sleep(1)
	#ser = serial.Serial(mower_device_port, 115200, timeout = 1)

	#ser.setDTR(False)
	#time.sleep(1)
	#ser.flushInput()
	#ser.setDTR(True)
	#time.sleep(2)

	#lidar = RPLidar(None, lidar_device_port, timeout = 3)
#	print("LIDAR device: {}".format(lidar_device_port))
	#print("Mower device: {}".format(mower_device_port))

#connect_serial_devices()

detect_serial_devices()

detected_angles = []
averageAngle = 0
previous_averageAngle = 0
object_detected_counter = 0 # count for 10 times to make sure there is an object there


def process_data(data):
	for i in range(len(data)):
		if i < 45 or i >= 315: 
		    global averageLidarAngle
		    if data[i] < 300 and data[i] != 0: #Object closer than 30 cm
			    detected_angles.append(i)
			    #print('%d: Got %d mm measurments' % (i, data[i]))
			    #data[i] = 0
			    #take_pic()
			    #ser.write(b'L') # notify mower that an object is detected
		    #print('%d: Got %d mm measurments' % (i, data[i]))

def compare_angle():
	global averageAngle
	global previous_averageAngle
	global object_detected_counter
	if abs(averageAngle - previous_averageAngle) < 10 or abs(averageAngle - previous_averageAngle) > 350:
		object_detected_counter+=1
	else:
		object_detected_counter = 0

def calculateAverageAngle(angles_with_object):
	global averageAngle
	global previous_averageAngle
	array_detected_angles = np.array(angles_with_object)
	#Define sub-ranges
	sub_range_angle_1 = array_detected_angles[(array_detected_angles >= 0) & (array_detected_angles <= 45)]
	sub_range_angle_2 = array_detected_angles[(array_detected_angles >= 315) & (array_detected_angles <= 360)]
	
	#Define weight of sub-ranges
	weight_sub_range_1 = len(sub_range_angle_1)/len(array_detected_angles)
	weight_sub_range_2 = len(sub_range_angle_2)/len(array_detected_angles)
	
	#Calculate average angle for each sub-range
	avg_sub_range_angle_1 = np.mean(sub_range_angle_1)
	avg_sub_range_angle_2 = np.mean(sub_range_angle_2)


	if len(sub_range_angle_1) > 1 and len(sub_range_angle_2) > 1:
		#Combine average angles using weighted average
		if len(sub_range_angle_1) > len(sub_range_angle_2):#turn right
			averageAngle = avg_sub_range_angle_1 - (weight_sub_range_2 * (360 - avg_sub_range_angle_2))
			if averageAngle < 0: # possibly turn left if adjusted weight make angle < 0
				averageAngle = averageAngle + 360
		else:  # turn left
			averageAngle = avg_sub_range_angle_2 + (weight_sub_range_1 * avg_sub_range_angle_1)
			if averageAngle > 360:
				averageAngle = averageAngle - 360
	elif len(sub_range_angle_1) > 1:
		averageAngle = avg_sub_range_angle_1
	elif len(sub_range_angle_2) > 1:
		averageAngle = avg_sub_range_angle_2
		
	compare_angle()
	print("The counter is : " + str(object_detected_counter))
	previous_averageAngle = averageAngle
	print("Average angle: {:.2f} degrees".format(averageAngle))

def send_average_angle():
	global averageAngle
	global previous_averageAngle
	global object_detected_counter
	averageAngle = floor(averageAngle)
	send_lidar_msg = "L" + f'{averageAngle:03d}' + "\n"
	#send_lidar_msg = send_lidar_msg.encode('utf-8')
	print(send_lidar_msg)
	ser.write(send_lidar_msg.encode('utf-8')) # notify mower that an object is detected
	object_detected_counter = 0
	time.sleep(3) 
scan_data = [0]*360

def collect_data():
	global lidar
	global scan_data
	try:
		#print(lidar.info)
		for scan in lidar.iter_scans():
			for (_, angle, distance) in scan:
			   # print("angle: ", angle)
				#print("distance: ", distance)
				#print(f'Angle: {angle} Distance: {distance}')
				scan_data[min([359, floor(angle)])] = distance
			process_data(scan_data)
			if detected_angles:
				calculateAverageAngle(detected_angles)
				if object_detected_counter >= 10:
					print("Object is detected!")
					send_average_angle()
				detected_angles.clear()
				scan_data = [0] * 360

	except RPLidarException:
		print('Stopping.')  
		lidar.stop()
		lidar.stop_motor()
		lidar.disconnect()
		#connect_serial_devices()
		detect_serial_devices()
		scan_data = [0] * 360
		#lidar = RPLidar(None, lidar_device_port, timeout = 3)
		collect_data()

	except KeyboardInterrupt:
		print('Stoping.')     
		lidar.stop()
		lidar.stop_motor()
		lidar.disconnect()
    	
collect_data()
#lidar.stop_motor()
	



