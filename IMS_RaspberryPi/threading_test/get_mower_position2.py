import serial
import time
from send_data_to_backend2 import upload_positions
from serial_communication_controller2 import SerialCommunicationThread

POS_URL = "https://ims8.herokuapp.com/positions/mover"

def main(position):

    #ser_thread = SerialCommunicationThread()
    #ser_thread.start()
    try: 
        def get_position():
            print("waiting for mower positions")

            #line = ser_thread.read()
            line = position
    
            if line is not None and line.isalpha() is False:
                #print("Mower position: ",line)
                payload = {"position_horizontal": line.split(',')[0], "position_vertical": line.split(',')[1]}
                print(payload)
                payload = {}
        
        get_position()
    except Exception as error:
        print("An exception occured: ", error)
        main()

    #ser_thread.close()

#if __name__ == "__main__":
#    main()