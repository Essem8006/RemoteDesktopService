import sys
import socket
import threading

HOST = ''
PORT = 54323

lsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#SOCK_DGRAM is UDP
lsock.settimeout(1.0)

messages = []
lock = threading.Lock() # lock messages and clientAddress to use between threads
clientAddress = None
running = True

lsock.bind((HOST, PORT))
print(f"Bound to {(HOST, PORT)}")


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
                    messages.append(line.encode("utf-8"))
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
                if messages and clientAddress:
                    lsock.sendto(messages.pop(0), clientAddress)
    except KeyboardInterrupt:
        running = False

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