import socket

HOST = "127.0.0.1"  # The server's hostname or IP address
PORT = 54321  # The port used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:# same as before
    s.connect((HOST, PORT))
    s.sendall(b"Hello, world")
    data = s.recv(1024)
    print(f"Received {data!r}")
    s.sendall(b"Second test?")
    data = s.recv(1024) #1024 used above is the maximum amount of data to be received at once in bytes
    # automatically sends an empty byte object when socket closes

print(f"Received {data!r}")