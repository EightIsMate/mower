import serial
import time
import asyncio
import websockets

ser = serial.Serial("/dev/ttyUSB0", 115200, timeout = 1)


ser.setDTR(False)
time.sleep(1)
ser.flushInput()
ser.setDTR(True)
time.sleep(2)

async def handle_client(websocket, path):
    try:
        print(f'Connection from: {websocket.remote_address}')
        async for message in websocket:
            print(f"Received message: {message}")
            ser.write(message.encode())  # Encode is to UTF-8
            ser.flush()
            await websocket.send("Hello from Raspberry Pi 4!")
    except websockets.ConnectionClosedError as e:
        print(f"Connection closed: {e}")
    except Exception as e:
        print(f"Error: {e}")


start_server = websockets.serve(handle_client, '172.20.10.9', 12345) #Elins hotspot
#start_server = websockets.serve(handle_client, '172.20.10.8', 12345) #Kyrollos hotspot


print('WebSocket server listening for connections...')
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()


#while True:
#	print('Telling the mbot to set the LED ring green')
#	ser.write(b'1')
#	
#	while True:
#		ack = ser.read()
#		if ack == b'A':
#			break
#	print('Arduino sent back %s' % ack)
#	
#	time.sleep(2)
#	
#	print('Telling mbot to turn the LED ring off')
#	ser.write(b'0')
#	while True:
#		ack = ser.read()
#		if ack == b'A':
#			break
#	print('Arduino sent back %s' % ack)
#	
#	time.sleep(2)
