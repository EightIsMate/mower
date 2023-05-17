from picamera import PiCamera
from time import sleep
import os
from send_data_to_backend2 import upload_image_to_api
from serial_communication_controller2 import SerialCommunicationThread



image_path = '/home/group8/IMS/image4.jpg'

#dummy data for obstical position 
payload = {"position_horizontal": "4.5", "position_vertical": "2.5"}

def main(image_position):
    print("In camera file")
    ser_thread = SerialCommunicationThread()
    ser_thread.start()

    def take_pic():
        camera = PiCamera()

        sleep(2)
        camera.capture(image_path)
        print("picture captured")
        print(image_position)
        upload_image_to_api(image_path, image_position) #image_position is mower_position right now
        ser_thread.write("K")
        camera.close()
        
    #pictureCommand = ser_thread.read()

    #if pictureCommand is not None:
    #    print("pictureCommand: ", pictureCommand)
    take_pic()
	

