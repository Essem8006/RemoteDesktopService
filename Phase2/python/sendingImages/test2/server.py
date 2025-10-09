# Sockets
import sys
import socket
HOST = ''
PORT = 54323
lsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
send_buffer = b""
clientAddress = None
running = True
lsock.bind((HOST, PORT))
lsock.listen()
print(f"Listening on {(HOST, PORT)}")
conn, addr = lsock.accept()
print(f"Accepted connection from {addr}")

# Screen capture
import pyautogui
resolution = pyautogui.size() # (1920, 1080)
img = pyautogui.screenshot()

# Message encoding
import json
import struct
import io

# Functions - - - - - - - - - - - - - - - - - - - - - - - - - - -


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

try:
    recv_data = conn.recv(1024)
    print(f"Received '{recv_data!r}'")
    while running:
        if len(send_buffer) > 0:
            if len(send_buffer) > 1400:
                to_send = send_buffer[:1024]
            else:
                to_send = send_buffer
            len_sent = conn.send(to_send)
            send_buffer = send_buffer[len_sent:]
        else:
            break
    print('done')
except KeyboardInterrupt:
    print("Caught keyboard interrupt, exiting...")
finally:
    running = False
    lsock.close()