import threading
import time
from app_communication import main as app_communication
from lidar_mower_communication import main as lidar_mower_communication
from serial_read_handler import main as serial_read_handler
from serial_communication_controller import SerialCommunicationThread

#create threads

app_thread = threading.Thread(target = app_communication)
lidar_thread = threading.Thread(target = lidar_mower_communication)
serial_read_handler_thread = threading.Thread(target = serial_read_handler)
ser_thread = SerialCommunicationThread()

try:

    ser_thread.start()
    #start threads
    app_thread.start()
    lidar_thread.start()
    serial_read_handler_thread.start()
    #For deugging without the app
    time.sleep(2)
    ser_thread.write("A00\n")
except KeyboardInterrupt:
    print('Stoping.')  
    app_thread.join()
    lidar_thread.join()
    serial_read_handler_thread.join()
