//#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <unistd.h>
//#include <string.h>
#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <sys/socket.h>

using namespace std;

#define PORT     8080
#define MAXLINE  1024

Display *display;
int serverSocket, clientSocket;
struct sockaddr_in serverAddress;


int main() {

    display = XOpenDisplay(nullptr);
    if (!display) {
        cerr << "Failed to open X display.\n";
    }

    XWindowAttributes gwa;
    XGetWindowAttributes(display, DefaultRootWindow(display), &gwa);


    XWarpPointer(display, None, DefaultRootWindow(display), None, None, None, None, 10, 10);
    XFlush(display);


    return 0;
}
