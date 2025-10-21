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
from clientEncoding import *

send_buffer = b""
recv_buffer = b""

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

# main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
try:
    jsonheader_len = None
    jsonheader = None
    content = None
    while True:
        recv_data= sock.recv(1024)
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
            frame = np.array(content)
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            out.write(frame)
            cv2.imshow('Live', frame)
            if cv2.waitKey(1) == ord('q'):
                break
            jsonheader_len = None
            jsonheader = None
            content = None
        
except KeyboardInterrupt:
    print("Caught keyboard interrupt, exiting...")
finally:
    sock.close()