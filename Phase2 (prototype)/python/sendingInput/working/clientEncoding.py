import json
import struct
import io
from PIL import Image

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