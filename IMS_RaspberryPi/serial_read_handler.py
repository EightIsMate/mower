from serial_communication_controller import SerialCommunicationThread
import camera
import get_mower_position
import time
position = {}

def main():
    ser_thread = SerialCommunicationThread()
    ser_thread.start()
    global position
    while True:
        data = ser_thread.read()

        if data is not None:
            if data == "P":
                camera.main(position)
                ser_thread.write("K\n")
            elif is_float_tuple(data):
                position = get_mower_position.get_position(data)

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
    try:
        main()
    except Exception:
        main()