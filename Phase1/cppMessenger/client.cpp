#include <arpa/inet.h>   // for inet_pton
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

using namespace std;

void recieve_messages(int clientSocket) {
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // null-terminate WHAT IS THIS
            cout << "\nServer: " << buffer << "\n> " << flush;
        }
        else if (bytesReceived < 0) {
            perror("Receive failed");
            break;
        }
        else{
            cout << "\nServer disconnected.\n";
            break;
        }
    }
    close(clientSocket);
    exit(0); // end program
}
void send_messages(int clientSocket) {
    string message;
    while (true) {
        cout << "> " << flush;
        getline(cin, message);
        if (message.size() == 0) { break; }
        if (send(clientSocket, message.c_str(), message.size(), 0) < 0) {
            perror("Send failed");
            break;
        }
    }
    close(clientSocket);
    exit(0); // end program
}

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
    cout << "Connected to server";

    thread recieve(recieve_messages, clientSocket);
    thread sending(send_messages, clientSocket);
    

    // join threads (keeps program alive)
    sending.join();
    recieve.join();

    return 0;
}
