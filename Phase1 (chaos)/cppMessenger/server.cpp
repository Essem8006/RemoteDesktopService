#include <arpa/inet.h>
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
            buffer[bytesReceived] = '\0'; // null-terminate for old c strings to make them ok
            cout << "\nClient: " << buffer << "\n> " << flush;
        }
        else if (bytesReceived < 0) {
            perror("Receive failed");
            break;
        }
        else{// bytesrecieved == 0
            cout << "\nClient disconnected.\n";
            break;
        }
    }
    close(clientSocket);
    exit(0); // end program?
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
    }
    cout << "Connected to client: " << clientSocket << endl;

    thread r(recieve_messages, clientSocket);
    thread s(send_messages, clientSocket);

    s.join();
    r.join();

    close(serverSocket);

    return 0;
}
