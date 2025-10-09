import selectors
import sys
import socket
import types

sel = selectors.DefaultSelector()

sel.register(sys.stdin, selectors.EVENT_READ)
messages = []

HOST = ''
PORT = 54323
lsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#SOCK_DGRAM is UDP
events = selectors.EVENT_READ | selectors.EVENT_WRITE
data = types.SimpleNamespace( outb=b"" )

lsock.bind((HOST, PORT))
print(f"Bound to {(HOST, PORT)}")

sel.register(lsock, events, data=data)

def handle_input(fileobj, mask):
    line = fileobj.readline().strip()
    if line:
        messages.append(bytes(line, "utf8"))

def service_connection(key, mask):
    sock = key.fileobj # PROBLEM HERE see line 51
    data = key.data
    if mask & selectors.EVENT_READ:
        recv_data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes  # Should be ready to read
        if recv_data:
            print(f"Received '{recv_data!r}' from {addr}")
            global client_address
            client_address = addr
    if mask & selectors.EVENT_WRITE:
        if not data.outb and messages:
            data.outb = messages.pop(0)
        if data.outb:
            #print(f"Sending: '{data.outb!r}' to {client_address}")
            sent = sock.sendto(data.outb, client_address)  # Should be ready to write
            data.outb = data.outb[sent:]

try:
    while True:
        events = sel.select(timeout=None)
        for key, mask in events:
            if key.fileobj == sys.stdin:
                handle_input(key.fileobj, mask)
            elif key.fileobj == lsock:
                service_connection(key, mask)
except KeyboardInterrupt:
    print("Caught keyboard interrupt, exiting")
finally:
    sel.close()