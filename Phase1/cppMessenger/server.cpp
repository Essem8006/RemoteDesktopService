// C++ program to show the example of server application in socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main()
{
    // creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);// AF_INET : IPv4 protocol, SOCK_STREAM : TCP

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);// htons(): Converts port to network byte order.
    serverAddress.sin_addr.s_addr = INADDR_ANY;// INADDR_ANY: Accept connections on any IP.

    // binding socket.
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    // listening to the assigned socket
    listen(serverSocket, 5);// WHY THE 5!!!!!!!!!!!!!!!!!

    // accepting connection request
    int clientSocket = accept(serverSocket, nullptr, nullptr);

    // recieving data
    char buffer[1024] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Message from client: " << buffer << endl;

    // closing the socket.
    close(serverSocket);

    return 0;
}