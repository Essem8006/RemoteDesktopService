import sys
import socket

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <target host>")
    sys.exit(1)

HOST = sys.argv[1]
PORT = 54323 # target port to send to

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#SOCK_DGRAM is UDP

send_buffer = b""
recv_buffer = b""
running = True

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

def pil_encode(obj):
    img_byte_arr = io.BytesIO()
    obj.save(img_byte_arr, format="PNG")
    img_byte_arr = img_byte_arr.getvalue()
    return img_byte_arr

def pil_decode(byte_array):
    print(len(byte_array))
    return Image.open(io.BytesIO(byte_array))

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
        print(11)
        data = recv_buffer[:content_len]
        recv_buffer = recv_buffer[content_len:]
        if jsonheader["content-type"] == "PIL image object":
            return [pil_decode(data), recv_buffer]
        else:
            print('Big error')
    print(len(recv_buffer))
    return [None, recv_buffer]

try:
    line = sys.stdin.readline()
    line = line.strip()
    message = create_message(line.encode("utf-8"), "text", "utf-8")
    send_buffer += message
    while running:
        if len(send_buffer) > 0:
            len_sent = sock.sendto(send_buffer, (HOST, PORT))
            send_buffer = send_buffer[len_sent:]
        else:
            break
    jsonheader_len = None
    jsonheader = None
    content = None
    for i in range(1000):
        recv_data, addr = sock.recvfrom(1024)
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
            break
    print(type(content))
    len(content.size)
    content.show()
        
    print('done')
except KeyboardInterrupt:
    print("Caught keyboard interrupt, exiting...")
finally:
    running = False
    sock.close()