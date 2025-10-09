import sys
import socket
import threading

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <target host>")
    sys.exit(1)

HOST = sys.argv[1]
PORT = 54323 # target port to send to

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#SOCK_DGRAM is UDP
sock.settimeout(1.0) # given we need blocking this lets us skip once every second to see if running is false and shut down cleanly if so

messages = []
lock = threading.Lock() # lock messages to use between threads
running = True

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
    global running
    try:
        while running:
            try:
                recv_data, addr = sock.recvfrom(1024)
                print(f"Received '{recv_data!r}' from {addr}")
            except socket.timeout:
                continue
    except KeyboardInterrupt:
        running = False

def send_messages():
    global running
    try:
        while running:
            with lock:
                if messages:
                    sock.sendto(messages.pop(0), (HOST, PORT))
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
    sock.close()