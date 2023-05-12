from picamera import PiCamera
from time import sleep
import serial
import time
import os
from send_data_to_backend import upload_image_to_api

#ser = serial.Serial("/dev/ttyUSB0", 115200, timeout = 1)

#ser.setDTR(False)
#time.sleep(1)
#ser.flushInput()
#ser.setDTR(True)
#time.sleep(2)

image_path = '/home/group8/IMS/image2.jpg'

URL = "https://ims8.herokuapp.com/upload"
POS_URL = "https://ims8.herokuapp.com/positions/mover"


#dummy data for obstical position 
payload = {"position_horizontal": "4.5", "position_vertical": "2.5"}

def take_pic():
	#camera = PiCamera()

	#sleep(5)
	#camera.capture('/home/group8/IMS/image2.jpg')
	#ser.write(b'K\n')
	print("picture captured")
	upload_image_to_api(POS_URL,URL, image_path, payload) 
	
def delete_image(path):
	os.remove(path)

#pictureCommand = ser.read()
#if pictureCommand == b'P':
take_pic()
	
	
