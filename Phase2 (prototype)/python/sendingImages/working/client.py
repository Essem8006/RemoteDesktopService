import sys
import socket

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <target host>")
    sys.exit(1)

HOST = sys.argv[1]
PORT = 54323 # target port to send to

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)#SOCK_DGRAM is UDP
sock.connect((HOST, PORT))

send_buffer = b""
recv_buffer = b""

# Message encoding
import json
import struct
import io
from PIL import Image

# Functions - - - - - - - - - - - - - - - - - - - - - - - - - - -


def json_encode(obj, encoding):
        return json.dumps(obj, ensure_ascii=False).encode(encoding)

def json_decode(json_bytes, encoding):
    tiow = io.TextIOWrapper(
        io.BytesIO(json_bytes), encoding=encoding, newline=""
    )
    obj = json.load(tiow)
    tiow.close()
    return obj

def pil_decode(byte_array):
    return Image.open(io.BytesIO(byte_array))

def process_protoheader(recv_buffer):
    hdrlen = 2
    if len(recv_buffer) >= hdrlen:
        jsonheader_len = struct.unpack(">H", recv_buffer[:hdrlen])[0]
        recv_buffer = recv_buffer[hdrlen:]
        return [jsonheader_len, recv_buffer]
    return [None, recv_buffer]

def process_jsonheader(hdrlen, recv_buffer):
    if len(recv_buffer) >= hdrlen:
        header = json_decode(recv_buffer[:hdrlen], "utf-8")
        recv_buffer = recv_buffer[hdrlen:]
        for header_item in ("byteorder", "content-length", "content-type", "content-encoding"):
            if header_item not in header:
                raise ValueError(f"Missing required header '{header_item}'.")
        return [header, recv_buffer]
    return [None, recv_buffer]

def process_response(recv_buffer, jsonheader):
    content_len = jsonheader["content-length"]
    if len(recv_buffer) >= content_len:
        data = recv_buffer[:content_len]
        recv_buffer = recv_buffer[content_len:]
        if jsonheader["content-type"] == "PIL image object":
            return [pil_decode(data), recv_buffer]
        else:
            print('Big error')
    return [None, recv_buffer]

import cv2
import numpy as np
resolution = (1920, 1080)
codec = cv2.VideoWriter_fourcc(*"XVID")
filename = "Recording.avi"
fps = 60.0
out = cv2.VideoWriter(filename, codec, fps, resolution)
cv2.namedWindow("Live", cv2.WINDOW_NORMAL)
cv2.resizeWindow("Live", 480, 270)

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