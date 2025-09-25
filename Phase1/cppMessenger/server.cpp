#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread> // for the messaging

using namespace std;

int main()
{
    // creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);// SOCK_STREAM : TCP
    if (serverSocket < 0) {
        perror("Socket creation failed");
        return 1;// why this not like bind and listen
    }

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;// AF_INET : IPv4
    serverAddress.sin_port = htons(8080);// htons(): Converts port to network byte order.
    serverAddress.sin_addr.s_addr = INADDR_ANY;// INADDR_ANY: Accept connections on any IP.

    // binding socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // listening
    if (listen(serverSocket, 5) < 0) {// 5 is queue for connecting
        perror("Listen failed");
        return 1;
    }
    cout << "Server listening on port 8080..." << endl;

    // accepting connection request
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        perror("Accept failed");
        return 1;
    } else {
        cout << "Connected to client: " << clientSocket << endl;
    }

    // receiving data
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Message from client: " << buffer << endl;

    // send echo
    send(clientSocket, buffer, strlen(buffer), 0);
    cout << "Echoed message to client: " << buffer << endl;

    // closing sockets
    close(clientSocket);// WHY DO THAT HERE
    close(serverSocket);

    return 0;
}
