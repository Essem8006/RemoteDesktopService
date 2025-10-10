# Socket - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
import sys
import socket

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <target host>")
    sys.exit(1)

#server address
HOST = sys.argv[1]
PORT = 54323

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

# Message encoding - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
from reciever import *
from transmitter import *

# output window - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
import cv2
import numpy as np

resolution = (1920, 1080)
codec = cv2.VideoWriter_fourcc(*"XVID")
filename = "Recording.avi"
fps = 60.0
out = cv2.VideoWriter(filename, codec, fps, resolution)
cv2.namedWindow("Live", cv2.WINDOW_NORMAL)
cv2.resizeWindow("Live", 480, 270)

mouse_x = 0
mouse_y = 0
def mouse_position(event, x, y, flags, param):
    global mouse_x, mouse_y
    if event == cv2.EVENT_MOUSEMOVE:
        mouse_x, mouse_y = x, y  # update coordinates

# Bind the callback function to the window
cv2.setMouseCallback("Live", mouse_position)

# Threads - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
import threading
import pyautogui
running = True
img = pyautogui.screenshot()

def recieve_messages():
    global running
    global img
    recv_buffer = b""
    jsonheader_len = None
    jsonheader = None
    content = None
    try:
        while running:
            recv_data = sock.recv(1024)
            if recv_data:
                recv_buffer += recv_data
                if jsonheader_len is None:
                    temp = process_protoheader(recv_buffer)
                    jsonheader_len = temp[0]
                    recv_buffer = temp[1]
                if jsonheader_len is not None and jsonheader is None:
                    temp = process_jsonheader(jsonheader_len, recv_buffer)
                    jsonheader = temp[0]
                    recv_buffer = temp[1]
                if jsonheader is not None and content is None:
                    temp = process_response(recv_buffer, jsonheader)
                    content = temp[0]
                    recv_buffer = temp[1]
                if content is not None:
                    img = content
                    jsonheader_len = None
                    jsonheader = None
                    content = None
            else:
                print('Lost connection to server')
                running = False
    except KeyboardInterrupt:
        running = False
    finally:
        print('Recieve closed')# DOESNT WORK

def send_messages():
    global running
    global mouse_x, mouse_y
    send_buffer = b""
    mouse_pos = pyautogui.position()
    try:
        while running:
            current_pos = pyautogui.position()
            if current_pos != mouse_pos:
                mouse_pos = current_pos

                #probably not the best way to encode
                json_mouse = {
                    "x": mouse_x,
                    "y": mouse_y
                }
                send_buffer += create_message(json_encode(json_mouse, "utf-8"), "Mouse position", "utf-8")
            if len(send_buffer) > 0:
                len_sent = sock.send(send_buffer[:1024])
                if len_sent < 0:
                    print('Error sending')
                    break
                send_buffer = send_buffer[len_sent:]
                    
    except KeyboardInterrupt:
        running = False
    finally:
        print('Send closed')# DOESNT WORK

threads = [
    threading.Thread(target=recieve_messages, daemon=True),
    threading.Thread(target=send_messages, daemon=True)#daemon closes thread on main program end
]

# main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
for t in threads:
    t.start()

try:
    while running:
        for t in threads:
            if not t.is_alive():
                running = False
                raise KeyboardInterrupt
        frame = np.array(img)
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        out.write(frame)
        cv2.imshow('Live', frame)
        if cv2.waitKey(1) == ord('q'):
            running = False
            raise KeyboardInterrupt
except KeyboardInterrupt:
    print("\nCaught keyboard interrupt, exiting...")
finally:
    sock.close()