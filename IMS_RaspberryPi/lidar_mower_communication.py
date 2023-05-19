import pyudev
import time

from math import cos, sin, pi, floor
from adafruit_rplidar import RPLidar, RPLidarException
import numpy as np
from serial_communication_controller import SerialCommunicationThread

ser_thread = None

def detect_serial_devices():
	global lidar_device_port
	global mower_device_port
	global lidar
	global ser
	context = pyudev.Context()
	devices = context.list_devices(subsystem='tty')
	for device in devices:
		try:
			vendor_id = device.get('ID_VENDOR_ID')
			product_id = device.get('ID_MODEL_ID')
			if vendor_id == '10c4' and product_id == 'ea60':
				# LIDAR detected
				lidar_device_port = device.device_node
				lidar = RPLidar(None, lidar_device_port, timeout=3)
				print("LIDAR device: {}".format(lidar_device_port))
		except:
			pass





def process_data(data):
	for i in range(len(data)):
		if i < 45 or i >= 315:
			global averageLidarAngle
			if data[i] < 300 and data[i] != 0: #Object closer than 30 cm
			    detected_angles.append(i)

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

def send_average_angle(ser_thread):
	global averageAngle
	global previous_averageAngle
	global object_detected_counter
	averageAngle = floor(averageAngle)
	send_lidar_msg = "L" + f'{averageAngle:03d}' + "\n"
	#send_lidar_msg = send_lidar_msg.encode('utf-8')
	print(send_lidar_msg)
	ser_thread.write(send_lidar_msg)
	#ser.write(send_lidar_msg.encode('utf-8')) # notify mower that an object is detected
	object_detected_counter = 0
	time.sleep(5) 

def main():
	global ser_thread
	global lidar
	global scan_data
	scan_data = [0] * 360
	detect_serial_devices()

	global detected_angles
	detected_angles = []
	global averageAngle
	averageAngle = 0
	global previous_averageAngle
	previous_averageAngle = 0
	# count for 10 times to make sure there is an object there
	global object_detected_counter 
	object_detected_counter = 0
	try:
		if ser_thread is None:
			ser_thread = SerialCommunicationThread()
			ser_thread.start()

		#print(lidar.info)
		for scan in lidar.iter_scans():
			for (_, angle, distance) in scan:
			   	#print("angle: ", angle)
				#print("distance: ", distance)
				#print(f'Angle: {angle} Distance: {distance}')
				scan_data[min([359, floor(angle)])] = distance
			process_data(scan_data)
			if detected_angles:
				calculateAverageAngle(detected_angles)
				if object_detected_counter >= 10:
					print("Object is detected!")
					send_average_angle(ser_thread)
				detected_angles.clear()
				scan_data = [0] * 360

	except RPLidarException:
		print('Stopping.')  
		lidar.stop()
		lidar.stop_motor()
		lidar.disconnect()
		detect_serial_devices()
		scan_data = [0] * 360
		main()

	except KeyboardInterrupt:
		print('Stoping.')     
		lidar.stop()
		lidar.stop_motor()
		lidar.disconnect()
		
	ser_thread.close()

if __name__ == "__main__":
	main()