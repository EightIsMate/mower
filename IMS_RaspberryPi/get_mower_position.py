from send_data_to_backend import upload_positions

POS_URL = "https://ims8.herokuapp.com/positions/mover"
    
def get_position(position):
    try: 
        line = position

        if line is not None and line.isalpha() is False:
            #print("Mower position: ",line)
            payload = {}
            payload = {"position_horizontal": line.split(',')[0], "position_vertical": line.split(',')[1]}
            print(payload)
            upload_positions(payload)
            return payload

    except Exception as error:
        print("An exception occured: ", error)
        #get_position()

