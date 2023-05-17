import pyudev
import time

def detect_serial_devices():
	context = pyudev.Context()
	devices = context.list_devices(subsystem = 'tty')
	serial_devices = []
	for device in devices:
		try:
			vendor_id = device.get('ID_VENDOR_ID')
			product_id = device.get('ID_MODEL_ID')
			if vendor_id == '10c4' and product_id == 'ea60':
				#LIDAR detected
				serial_devices.append(device.device_node)
			elif vendor_id == '1a86' and product_id == '7523':
				#mower detected
				serial_devices.append(device.device_node)
		except:
			pass
	return serial_devices
	
while True:
	serial_devices = detect_serial_devices()
	if len(serial_devices) == 2:
		#Both devices detected
		lidar_device = serial_devices[0]
		mcu_device = serial_devices[1]
		break
	time.sleep(1)
	
print("LIDAR device: {}".format(lidar_device))
print("Mwer device: {}".format(mcu_device))
