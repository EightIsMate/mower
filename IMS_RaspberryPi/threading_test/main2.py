import threading
from app_communication2 import main as app_communication
#from get_mower_position2 import main as get_mower_position 
from lidar_mower_communication2 import main as lidar_mower_communication
#from camera2 import main as camera
from serial_read_handler import main as serial_read_handler

#create threads

app_thread = threading.Thread(target = app_communication)
#mower_thread = threading.Thread(target = get_mower_position)
lidar_thread = threading.Thread(target = lidar_mower_communication)
#camera_thread = threading.Thread(target= camera)
serial_read_handler_thread = threading.Thread(target = serial_read_handler)

#start threads

try:
    app_thread.start()
    #mower_thread.start()
    lidar_thread.start()
    #camera_thread.start()
    serial_read_handler_thread.start()
except KeyboardInterrupt:
    print('Stoping.')  
    app_thread.join()
    #mower_thread.join()
    lidar_thread.join()
    #camera_thread.join()
    serial_read_handler_thread.join()
