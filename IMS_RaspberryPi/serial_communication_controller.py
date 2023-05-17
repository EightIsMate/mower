import threading
import serial
import pyudev
import time

class SerialCommunicationThread(threading.Thread):
    def __init__(self, baudrate = 115200 , timeout = 1):
        threading.Thread.__init__(self)
        self.port = find_mower_port()
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
     
    def run(self):
        self.ser = serial.Serial(self.port, self.baudrate, timeout = self.timeout)
        self.ser.setDTR(False)
        time.sleep(1)
        self.ser.flushInput()
        self.ser.setDTR(True)
        time.sleep(2)
    
    def write(self, message):
        if self.ser is not None:
            self.ser.write(message.encode())
            self.ser.flush()

    def read(self):
        if self.ser is not None and self.ser.in_waiting > 0:
            line = self.ser.readline().decode('utf-8').rstrip()
            return line 
        return None

    def close(self):
        if self.ser is not None:
            self.ser.close() 

def find_mower_port():
    context = pyudev.Context()
    devices = context.list_devices(subsystem='tty')
    for device in devices:
        try:
            vendor_id = device.get('ID_VENDOR_ID')
            product_id = device.get('ID_MODEL_ID')
            if vendor_id == '1a86' and product_id == '7523':
                #Mower detected
                return device.device_node
        except:
             pass
