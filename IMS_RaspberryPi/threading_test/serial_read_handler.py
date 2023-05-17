from serial_communication_controller2 import SerialCommunicationThread
import camera2
import get_mower_position2
import time
position = {}

def main():
    ser_thread = SerialCommunicationThread()
    ser_thread.start()
    global position

    while True:
        data = ser_thread.read()

        if data is not None:
            print("serial read handler: ", data)
            if data == "P":
                #print("Received Picture Command")
                camera2.main(position)
            elif is_float_tuple(data):
                #print("Received Position data")
                position = get_mower_position2.get_position(data)

        time.sleep(1)

    ser_thread.close()

def is_float_tuple(data):
    try:
        coordinates = data.split(',')
        if len(coordinates) != 3:
            return False
        for coordinate in coordinates:
            float(coordinate)
        return True
    except ValueError:
        return False

if __name__ == "__main__":
    main()