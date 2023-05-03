import serial
import time
import matplotlib.pyplot as plt
import numpy as np


def draw_robot_position(x_pos, y_pos, heading_pos, prev_positions_pos):
    plt.cla()  # Clear the current axes
    # plt.xlim(-5, 5)
    # plt.ylim(-5, 5)

    # Plot the robot's path as a line
    if len(prev_positions_pos) > 1:
        path_x, path_y = zip(*prev_positions_pos)
        plt.plot(path_x, path_y, 'g-')

    plt.plot(x_pos, y_pos, 'bo', markersize=10)

    if heading_pos < 0:
        heading_pos = 360 - (heading_pos -1)

    arrow_length = 1
    dx = arrow_length * np.cos(np.radians(heading_pos))
    dy = arrow_length * np.sin(np.radians(heading_pos))

    plt.arrow(x_pos, y_pos, dx, dy, width=0.1, fc='r', ec='r')

    plt.xlabel("X Position")
    plt.ylabel("Y Position")
    plt.title("Robot Position")
    plt.grid(True)

    ax = plt.gca()  # Get the current axes
    ax.set_aspect('equal', adjustable='box')  # Set an equal aspect ratio


# Connect to the serial port
serial_port = serial.Serial('/dev/tty.usbserial-1410', 115200) 
time.sleep(2)  # Allow some time for the connection to establish

# Set up the plot
plt.ion()  # Turn on interactive mode
fig = plt.figure(figsize=(6, 6))

prev_positions = []  # Store previous positions to draw the path

while True:
    try:
        # Read a line from the serial port
        print("in while")
        line = serial_port.readline().decode(errors='ignore').strip()
        print(line)

        # Extract values and append to the lists
        x = float(line.split(',')[0])
        print(f"x: {x}")
        y = float(line.split(',')[1])
        print(f"y: {y}")
        heading = float(line.split(',')[2])
        print(f"heading: {heading}")

        prev_positions.append((x, y))

        # Update the plot with the new position and heading
        draw_robot_position(x, y, heading, prev_positions)
        plt.pause(0.01)  # Add a short delay to avoid overloading the CPU

    except (serial.SerialException, ValueError) as e:
        print(f"Error: {e}")
        break

serial_port.close()
plt.ioff()
plt.show()