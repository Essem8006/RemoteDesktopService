#include <arpa/inet.h>   // for inet_pton
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main()
{
    // create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // specify server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    // convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        return 1;
    }

    // connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection failed");
        return 1;
    }

    // send message
    cout << "Enter message: ";
    string message;
    getline(cin, message);

    if (send(clientSocket, message.c_str(), message.size(), 0) < 0) {
        perror("Send failed");
        return 1;
    }

    // receive echo
    char buffer[1024] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        perror("Receive failed");
        return 1;
    }
    buffer[bytesReceived] = '\0'; // null-terminate
    cout << "Echo from server: " << buffer << endl;

    // close socket
    close(clientSocket);

    return 0;
}
