#include <arpa/inet.h> // for AF_INET
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

using namespace std;
  
#define PORT     8080 
#define MAXLINE 1024 

void recieve_messages(int serverSocket, sockaddr_in &clientAddr) {
    socklen_t len = sizeof(clientAddr);
    char buffer[MAXLINE];
    while (true) {
        int bytesReceived = recvfrom(serverSocket, buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &clientAddr, &len);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0'; // null-terminate WHAT IS THIS
            cout << "Client: " << buffer << endl;
        }
        else if (bytesReceived < 0) {
            perror("Receive failed");
            break;
        }
        else{
            cout << "\nClient disconnected.\n";
            break;
        }
    }
    close(serverSocket);
    exit(0); // end program
}

void send_messages(int serverSocket, sockaddr_in &clientAddr) {
    string message;
    while (true) {
        cout << "> " << flush;
        getline(cin, message);
        if (message.size() == 0) { break; }
        if (sendto(serverSocket, message.c_str(), message.size(), MSG_CONFIRM, (const struct sockaddr *) &clientAddr, sizeof(clientAddr))  < 0) {
            perror("Send failed");
            break;
        }
    }
    close(serverSocket);
    exit(0); // end program
}
  
// Driver code 
int main() { 
    int serverSocket; 
    struct sockaddr_in serverAddress, clientAddress; 
      
    // Creating socket file descriptor 
    if ( (serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&serverAddress, 0, sizeof(serverAddress)); 
    memset(&clientAddress, 0, sizeof(clientAddress)); 
      
    // Filling server information 
    serverAddress.sin_family    = AF_INET; // IPv4 
    serverAddress.sin_addr.s_addr = INADDR_ANY; 
    serverAddress.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(serverSocket, (const struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    cout << "Server running on port " << PORT << endl;
    
    thread r(recieve_messages, serverSocket, std::ref(clientAddress));
    thread s(send_messages, serverSocket, std::ref(clientAddress));

    s.join();
    r.join();

    close(serverSocket);
      
    return 0; 
}
//Maximus1!