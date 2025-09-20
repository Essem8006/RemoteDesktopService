import socket

#socket()
#.bind()
#.listen()
#.accept()
#.connect()
#.connect_ex()
#.send() only sends one byte so use sendall
#.recv()
#.close()

# maps to c counterparts: system calls

#socket.SOCK_STREAM default protocol is TCP

# This server script just returns all messages it recieves back to the client

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 54321  # Port to listen on (non-privileged ports are > 1023) mav 65535

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s: # handles the closing as well. AF_INET is the Internet address family for IPv4. SOCK_STREAM is the socket type for TCP
    s.bind((HOST, PORT))
    s.listen()#enables a server to accept connections. It makes the server a listening socket
    conn, addr = s.accept() # waits for connection then sets an object as the connection and addr as the address of the client
    with conn:
        print(f"Connected by {addr}")
        while True:
            data = conn.recv(1024)
            if not data:
                break# closes on empty byte
            conn.sendall(data)