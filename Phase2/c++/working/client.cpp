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


  
// Driver code 
int main() { 
    unsigned long a = 64; // 01000000
    unsigned long b = 2; //  00000010
    unsigned long c = 1024; //  100 00000000
    cout << sizeof(unsigned char) << endl;
    cout << sizeof(b) << endl;
    cout << sizeof(c) << endl;
  
    return 0; 
}