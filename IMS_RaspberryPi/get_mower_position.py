from send_data_to_backend import upload_positions
    
def get_position(position):
    try: 
        line = position

        if line is not None and line.isalpha() is False:
            payload = {}
            payload = {"position_horizontal": line.split(',')[0], "position_vertical": line.split(',')[1]}
            print(payload)
            upload_positions(payload)
            return payload

    except Exception as error:
        print("An exception occured: ", error)

