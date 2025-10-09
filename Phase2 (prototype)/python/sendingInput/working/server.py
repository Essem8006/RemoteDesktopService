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
from serverEncoding import *

send_buffer = b""
recv_buffer = b""

# main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
import pyautogui
try:
    while True:
        if len(send_buffer) > 0:
            len_sent = conn.send(send_buffer[:1024])
            if len_sent < 0:
                print('Error sending')
                break
            send_buffer = send_buffer[len_sent:]
        else:
            send_buffer += create_message(pil_encode(pyautogui.screenshot()), "PIL image object", "PNG")

except KeyboardInterrupt:
    print("Caught keyboard interrupt, exiting...")
finally:
    lsock.close()