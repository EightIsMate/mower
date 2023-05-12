from serial_communication_controller2 import SerialCommunicationThread
import camera2
import get_mower_position2
import time

def main():
    ser_thread = SerialCommunicationThread()
    ser_thread.start()

    while True:
        data = ser_thread.read()

        if data is not None:
            print("serial read handler: ", data)
            if data == "P":
                print("Received Picture Command")
                camera2.main()
            elif is_float_tuple(data):
                print("Received Position data")
                get_mower_position2.main(data)

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