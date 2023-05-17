import serial
import time
from send_data_to_backend import upload_positions

ser = serial.Serial("/dev/ttyUSB0", 115200, timeout = 1)


ser.setDTR(False)
time.sleep(1)
ser.flushInput()
ser.setDTR(True)

def get_position():
	print("waiting for positions")
	while True:
		if ser.in_waiting > 0:
			line = ser.readline().decode('utf-8').rstrip() #rstrip() removes white spaces
			payload = {"position_horizontal": line.split(',')[0], "position_vertical": line.split(',')[1]}
			print(payload)

	
get_position()
