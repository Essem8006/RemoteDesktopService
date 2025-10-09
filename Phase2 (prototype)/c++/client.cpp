#include <arpa/inet.h>   // for inet_pton
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

using namespace std;
  
#define PORT     8080 // server port number
#define MAXLINE 1024 

void recieve_messages(int clientSocket, sockaddr_in serverAddress) {
    socklen_t len;
    len = sizeof(serverAddress);
    char buffer[MAXLINE];
    while (true) {
        int bytesReceived = recvfrom(clientSocket, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serverAddress, &len);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // null-terminate WHAT IS THIS
            cout << "Server: "<<buffer<< endl; 
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

void send_messages(int clientSocket, sockaddr_in serverAddress) {
    string message;
    while (true) {
        cout << "> " << flush;
        getline(cin, message);
        if (message.size() == 0) { break; }
        if (sendto(clientSocket, message.c_str(), message.size(), MSG_CONFIRM, (const struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
            perror("Send failed");
            break;
        }
    }
    close(clientSocket);
    exit(0); // end program
}
  
// Driver code 
int main() { 
    int clientSocket;
  
    // Creating socket file descriptor 
    if ( (clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        return 1;
    } 

      
    // Filling server information 
    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress)); 
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(PORT); 
    serverAddress.sin_addr.s_addr = INADDR_ANY;
          
    thread recieve(recieve_messages, clientSocket, serverAddress);
    thread sending(send_messages, clientSocket, serverAddress);
    
    // join threads (keeps program alive)
    sending.join();
    recieve.join();
  
    return 0; 
}