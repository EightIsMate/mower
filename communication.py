import asyncio
import websockets

async def handle_client(websocket, path):
    print(f'Connection from: {websocket.remote_address}')

    message = await websocket.recv()
    print(f'Received: {message}')

    await websocket.send('Hello from Raspberry Pi!')

start_server = websockets.serve(handle_client, '172.20.10.9', 12345) #Elins hotspot

print('WebSocket server listening for connections...')
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()