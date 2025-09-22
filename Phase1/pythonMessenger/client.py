import selectors
import sys
import socket
import types # blocking and multiple connections to 1 server, doesn't like windows

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <target host>")
    sys.exit(1)

sel = selectors.DefaultSelector()

sel.register(sys.stdin, selectors.EVENT_READ)
messages = []

HOST = sys.argv[1]
PORT = 54323 # target port
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))
events = selectors.EVENT_READ | selectors.EVENT_WRITE
data = types.SimpleNamespace(
            outb=b"",
        )
sel.register(sock, events, data=data)

def handle_input(fileobj, mask):
    line = fileobj.readline().strip()
    if line:
        messages.append(bytes(line, "utf8"))

def service_connection(key, mask):
    sock = key.fileobj
    data = key.data
    if mask & selectors.EVENT_READ:
        recv_data = sock.recv(1024)  # Should be ready to read
        if recv_data:
            print(f"Received {recv_data!r}")
    if mask & selectors.EVENT_WRITE:
        if not data.outb and messages:
            data.outb = messages.pop(0)
        if data.outb:
            print(f"Sending {data.outb!r}")
            sent = sock.send(data.outb)  # Should be ready to write
            data.outb = data.outb[sent:]

try:
    while True:
        events = sel.select(timeout=None)
        for key, mask in events:
            if key.fileobj == sys.stdin:
                handle_input(key.fileobj, mask)
            elif key.fileobj == sock:
                service_connection(key, mask)
except KeyboardInterrupt:
    print("Caught keyboard interrupt, exiting")
finally:
    sel.close()