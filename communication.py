import asyncio
import websockets

async def handle_client(websocket, path):
    try:
        print(f'Connection from: {websocket.remote_address}')
        async for message in websocket:
            print(f"Received message: {message}")
            await websocket.send("Hello from Raspberry Pi 4!")
    except websockets.ConnectionClosedError as e:
        print(f"Connection closed: {e}")
    except Exception as e:
        print(f"Error: {e}")

start_server = websockets.serve(handle_client, '172.20.10.9', 12345) #Elins hotspot

print('WebSocket server listening for connections...')
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()