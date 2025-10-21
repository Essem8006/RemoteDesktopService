# Socket - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
import socket

HOST = ''
PORT = 54323

lsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
lsock.bind((HOST, PORT))
lsock.listen()
print(f"Listening on {(HOST, PORT)}")
conn, addr = lsock.accept()
print(f"Accepted connection from {addr}")

# Message encoding - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
from transmitter import *
from reciever import *

# Threads - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
import threading
import pyautogui
running = True

def send_images():
    global running
    send_buffer = b""
    try:
        while running:
            if len(send_buffer) > 0:
                len_sent = conn.send(send_buffer[:1024])
                if len_sent < 0:
                    print('Error sending')
                    break
                send_buffer = send_buffer[len_sent:]
            else:
                send_buffer += create_message(pil_encode(pyautogui.screenshot()), "PIL image object", "PNG")
    except KeyboardInterrupt:
        running = False
    finally:
        print('Send closed')

def recieve_input():
    global running
    recv_buffer = b""
    jsonheader_len = None
    jsonheader = None
    content = None
    try:
        while running:
            recv_data = conn.recv(1024)
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
                    #pyautogui.moveTo(content['x'], content['y'])
                    #print(content['x'], content['y'])
                    jsonheader_len = None
                    jsonheader = None
                    content = None
            else:
                print(f"Closing connection to {addr}")
                running = False
    except KeyboardInterrupt:
        running = False
    finally:
        print('Recieve closed')


threads = [
    threading.Thread(target=send_images, daemon=True),
    threading.Thread(target=recieve_input, daemon=True)#daemon closes thread on main program end
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

except KeyboardInterrupt:
    print("\nCaught keyboard interrupt, exiting...")
finally:
    lsock.close()