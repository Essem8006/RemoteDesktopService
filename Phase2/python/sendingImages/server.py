# Sockets
import sys
import socket
import threading
HOST = ''
PORT = 54323
lsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#SOCK_DGRAM is UDP
lsock.settimeout(1.0)
send_buffer = b""
lock = threading.Lock() # lock messages and clientAddress to use between threads
clientAddress = None
running = True
lsock.bind((HOST, PORT))
print(f"Bound to {(HOST, PORT)}")

# Screen capture
import pyautogui
resolution = pyautogui.size() # (1920, 1080)
img = pyautogui.screenshot()

# Message encoding
import json
import struct
import io

# Functions - - - - - - - - - - - - - - - - - - - - - - - - - - -
def input_thread():
    global running
    try:
        while running:
            line = sys.stdin.readline()
            if not line:
                break
            line = line.strip()
            if line:
                with lock:
                    message = create_message(line.encode("utf-8"), "text", "utf-8")
                    send_buffer += message
    except KeyboardInterrupt:
        running = False

def recieve_messages():
    global clientAddress
    global running
    try:
        while running:
            try:
                recv_data, addr = lsock.recvfrom(1024)
                clientAddress = addr
                print(f"Received '{recv_data!r}' from {addr}")
            except socket.timeout:
                continue
    except KeyboardInterrupt:
        running = False

def send_messages():
    global clientAddress
    global running
    try:
        while running:
            with lock:
                if len(send_buffer) > 0 and clientAddress:
                    print(lsock.sendto(send_buffer, clientAddress))
    except KeyboardInterrupt:
        running = False

def json_encode(obj, encoding):
        return json.dumps(obj, ensure_ascii=False).encode(encoding)

def pil_encode(obj):
    img_byte_arr = io.BytesIO()
    obj.save(img_byte_arr, format="PNG")
    img_byte_arr = img_byte_arr.getvalue()
    return img_byte_arr

def create_message(content_bytes, content_type, content_encoding):
        jsonheader = {
            "byteorder": sys.byteorder,
            "content-type": content_type,
            "content-encoding": content_encoding,
            "content-length": len(content_bytes),
        }
        jsonheader_bytes = json_encode(jsonheader, "utf-8")
        message_hdr = struct.pack(">H", len(jsonheader_bytes))
        message = message_hdr + jsonheader_bytes + content_bytes
        return message


message = create_message(pil_encode(img), "PIL image object", "PNG")
send_buffer += message

threads = [
    threading.Thread(target=input_thread, daemon=True),
    threading.Thread(target=recieve_messages, daemon=True),
    threading.Thread(target=send_messages, daemon=True),
]

for t in threads:
    t.start()

try:
    while True:
        # main thread idle loop
        for t in threads:
            if not t.is_alive():
                running = False
                raise KeyboardInterrupt
except KeyboardInterrupt:
    print("Caught keyboard interrupt, exiting...")
finally:
    running = False
    lsock.close()