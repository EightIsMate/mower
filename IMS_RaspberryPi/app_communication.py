import asyncio
import websockets
from serial_communication_controller import SerialCommunicationThread

def main():
    ser_thread = SerialCommunicationThread()
    ser_thread.start()

    async def run_server():
        start_server = websockets.serve(handle_client, '172.20.10.9', 12345) #Elins hotspot
        #start_server = websockets.serve(handle_client, '172.20.10.8', 12345) #Kyrollos hotspot
        print('WebSocket server listening for connections...')
        await start_server


    async def handle_client(websocket, path):
        try:
            print(f'Connection from: {websocket.remote_address}')
            async for message in websocket:
                print(f"Received message: {message}")
                ser_thread.write(message)
                await websocket.send("Hello from Raspberry Pi 4!")
        except websockets.ConnectionClosedError as e:
            print(f"Connection closed: {e}")
        except Exception as e:
            print(f"Error: {e}")



    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.create_task(run_server())
    loop.run_forever()
    ser_thread.close()

if __name__ == "__main__":
    main()