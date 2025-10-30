#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <vector>
#include <unistd.h> // for usleep()
#include <string.h>
#include <arpa/inet.h> // for AF_INET
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>

using namespace std;

#define PORT     8080 
#define MAXLINE  1024

Display *display;
int serverSocket, clientSocket; 
struct sockaddr_in serverAddress;

struct Frame {
    int width;
    int height;
    vector<unsigned char> data;
    const char* bytes() const {
        return reinterpret_cast<const char*>(data.data());
    }

    size_t size() const {
        return data.size();
    }
};

Frame captureScreenFrame(Display* display) {
    Frame frame{};
    Window root = DefaultRootWindow(display);

    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);
    int width = gwa.width;
    int height = gwa.height;

    XImage* img = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!img) {
        cerr << "Failed to capture X11 image.";
        return frame;
    }

    frame.width = width;
    frame.height = height;
    frame.data.resize(width * height * 3);// set size of frame data

    // Convert from XImage pixel (BGRX) to BGR
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned long pixel = XGetPixel(img, x, y);
            unsigned char blue  = pixel & 0xFF;
            unsigned char green = (pixel >> 8) & 0xFF;
            unsigned char red   = (pixel >> 16) & 0xFF;
            int idx = (y * width + x) * 3;
            frame.data[idx] = blue;
            frame.data[idx + 1] = green;
            frame.data[idx + 2] = red;
        }
    }

    XDestroyImage(img);
    return frame;
}

void init() {
    // Creating socket file descriptor 
    if ( (serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(0); 
    }

    // Filling server information 
    serverAddress.sin_family    = AF_INET; // IPv4 
    serverAddress.sin_addr.s_addr = INADDR_ANY; 
    serverAddress.sin_port = htons(PORT); 
      
    // binding socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Bind failed");
        exit(0);
    }

    // listening
    if (listen(serverSocket, 5) < 0) {// 5 is queue for connecting
        perror("Listen failed");
        exit(0);
    }
    cout << "Server listening on port 8080..." << endl;

    // accepting connection request
    clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        perror("Accept failed");
        exit(0);
    }
    cout << "Connected to client: " << clientSocket << endl;
}

void recieve_messages() {
    while (true) {
        // ----------------------- reconstruct frame ------------------------------
        int mousePos[2] = {0,0};
        int bytesReceived = recv(clientSocket, &mousePos, 8, 0);
        if (bytesReceived == 8) {
            // -------------------- move mouse -----------------------------------
            cout << mousePos[0] << ", " << mousePos[1] << endl;
        }
        else if (bytesReceived == 0) {
            cout << "\nClient disconnected.\n";
            break;
        }
        else{
            cout << bytesReceived << endl;
            perror("Receive failed");
            break;
        }
    }
}

int main() {
    init();

    display = XOpenDisplay(nullptr);
    if (!display) {
        cerr << "Failed to open X display.\n";
    }

    XWindowAttributes gwa;
    XGetWindowAttributes(display, DefaultRootWindow(display), &gwa);

    if (send(clientSocket,+ &gwa.height, sizeof(int), 0)  < 0) {
        perror("Send failed on height");
    }
    if (send(clientSocket, &gwa.width, sizeof(int), 0)  < 0) {
        perror("Send failed on width");
    }

    thread recieve(recieve_messages);

    while (true) {
        Frame frame = captureScreenFrame(display);
        if (frame.data.empty()) {
            cerr << "Failed to capture frame.\n";
        }

        int len = frame.data.size();
        unsigned char* dataPtr = frame.data.data();
        while (len > 0) {
            int chunkSize = min(len, 1024);
            int bytesSent = send(clientSocket, dataPtr, chunkSize, 0);
            if (bytesSent  <= 0) {
                perror("Send failed");
                break;
            }
            dataPtr += bytesSent;
            len -= bytesSent;
        }
    }

    close(serverSocket);

    recieve.join();
    
    return 0;
}
