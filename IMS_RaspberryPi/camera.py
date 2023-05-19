from picamera import PiCamera
from time import sleep
import os
from send_data_to_backend import upload_image_to_api
from serial_communication_controller import SerialCommunicationThread

image_path = '/home/group8/IMS/image4.jpg'


def main(image_position):

    def take_pic():
        print("In take_pic")
        camera = PiCamera()

        #Rotating the camera view 180 deg since it is upside down
        camera.hflip = True
        camera.vflip = True
        
        sleep(2)
        camera.capture(image_path)
        camera.close()
        upload_image_to_api(image_path, image_position) #image_position is mower_position right now

        
    take_pic()
	

