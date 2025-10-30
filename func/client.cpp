#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <vector>
#include <unistd.h> // for usleep()
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>   // for inet_pton
#include <thread>

using namespace std;

Display *display;
int screen;
Window window;
GC gc;
unsigned long black, grey, light, white;

bool controlData[7] = {0,0,0,0,0,0,0};
string controlLabel[7] = {"Control pointer", "Disconnect", "Option 3", "Send text", "View directories", "Server blackout", "Open chat"};
int mousePos[2] = {0,0};

unsigned long RGB(int r, int g, int b) {
    return (r<<16) + (g<<8) + b;
}

void init() {
    display = XOpenDisplay(nullptr);
    if (!display) {
        cerr << "Failed to open X display.\n";
    }
    screen = DefaultScreen(display);
    black = BlackPixel(display, screen);
    grey = RGB(150,150,150);
    light = RGB(200,200,200);
    white = WhitePixel(display, screen);

    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 800, 600, 1, black, light);//border_width bordercol and backgroundcol at end
    XSetStandardProperties(display, window, "Word", "Hi", None, NULL, 0, NULL);
    XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask);// tells X server what events we want from it
    gc = XCreateGC(display, window, 0, nullptr);
    XClearWindow(display, window);
    XMapRaised(display, window);
}

void close() {
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    exit(0);
}

void draw() {
    XClearWindow(display, window);
}

int floor(double i) {
    return (int)i;
}

struct Frame {
    int width;
    int height;
    vector<unsigned char> data; // BGR format
};

XImage* convertFrameToImage(Frame frame, Display* display, int screen, int imgWidth, int imgHeight) {
    XImage* image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0,
        (char*)malloc(imgWidth * imgHeight * 4), imgWidth, imgHeight, 32, 0
    );
    //If you mean change the pixel dimensions of the XImage, no, there is no way to do that. If you want to scale the image pixels, you would use XRender on the server side or something like the Cairo library on the client side. But you still will have to scale to a different image (or window or pixmap), since you can't resize an allocated XImage.
    for (int y = 0; y < imgHeight; ++y) {
        for (int x = 0; x < imgWidth; ++x) {
            int idx;
            if (0 == controlData[2]) {
                idx = (y * frame.width + x) * 3;
            } else {
                idx = (floor(y*frame.height/imgHeight) * frame.width + floor(x*frame.width/imgWidth)) * 3;
            }
            unsigned char b = frame.data[idx];
            unsigned char g = frame.data[idx + 1];
            unsigned char r = frame.data[idx + 2];
            unsigned long pixel = (r << 16) | (g << 8) | b;// << x moves bits x spaces to the left, so this creates a 24 bit colour value
            XPutPixel(image, x, y, pixel);
        }
    }

    return image;
}

void button(int count, int x, int y, int w, int h) {
    XSetForeground(display, gc, black);
    const char *text = controlLabel[count].c_str();
    XDrawString(display,window,gc,x+w+3,y+h,text,strlen(text));
    if (1 == controlData[count]) {
        x += w;
        y += h;
        w *= -1;
        h *= -1;
    }
    XDrawLine(display, window, gc, x+w, y, x+w, y+h);
    XDrawLine(display, window, gc, x, y+h, x+w, y+h);
    XSetForeground(display, gc, white);
    XDrawLine(display, window, gc, x, y, x+w-1, y);
    XDrawLine(display, window, gc, x, y, x, y+h-1);
    XSetForeground(display, gc, grey);
    XDrawLine(display, window, gc, x+w-1, y+1, x+w-1, y+h-1);
    XDrawLine(display, window, gc, x+1, y+h-1, x+w-1, y+h-1);
}

void updateControls(int x, int y) {
    for (int i=0;i<7;i++) {
        if (1 == i && controlData[i] == 1) {
            close();
        }
        button(i, x+3, y+3+i*20, 10, 10);
    }
}

void recieve_messages(int clientSocket) {
    // ----------------------- get dimentions --------------------------------------
    int height;
    recv(clientSocket, &height, 4, 0);// 4 bytes is size of int
    //cout << "Height is: " << height << endl;
    int width;
    recv(clientSocket, &width, 4, 0);
    //cout << "Width is: " << width << endl;

    /*if (send(clientSocket, &width, sizeof(int), 0)  < 0) {
        perror("Send failed on validation");
    }*/
    while (true) {
        // ----------------------- reconstruct frame ------------------------------
        Frame frame{};
        frame.width = width;
        frame.height = height;
        frame.data.resize(width * height * 3);

        int len = frame.data.size();
        unsigned char* ptr = frame.data.data();
        while (len > 0) {
            int chunkSize = min(len, 1024);
            int bytesReceived = recv(clientSocket, ptr, chunkSize, 0);
            if (bytesReceived  <= 0) {
                perror("Send failed");
                break;
            }
            ptr += bytesReceived;
            len -= bytesReceived;
        }
        
        // -------------------- display image -----------------------------------
        XWindowAttributes winAttr;
        XGetWindowAttributes(display, window, &winAttr);
        XImage* image = convertFrameToImage(frame, display, screen, winAttr.width-153, winAttr.height-6);
        XPutImage(display, window, gc, image, 0, 0, 150, 3, winAttr.width-153, winAttr.height-6);
        XDestroyImage(image);
    }

    close(clientSocket);
    //exit(0); // end program
}

int main() {
    init();

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
    cout << "Connected to server" << endl;

    thread recieve(recieve_messages, clientSocket);

    XEvent e;
    while (true) {
        XWindowAttributes winAttr;
        XGetWindowAttributes(display, window, &winAttr);
        // event loop
        while (XPending(display)) {
            XNextEvent(display, &e);
            if (e.type == KeyPress && controlData[3]) {
                cout << e.xkey.keycode << endl;
                int key = e.xkey.keycode;
                char fakeheader = 'k';
                send(clientSocket, &fakeheader, 1, 0);
                int bytesSent = send(clientSocket, &key, 4, 0);
                if (bytesSent  != 4) {
                    perror("Send key press failed");
                    break;
                }
            };
            if (e.type == Expose) draw();
            if (e.type == ButtonPress) {
                int x = e.xbutton.x, y = e.xbutton.y;
                if (x >= 3 && x <= 13 && y <= 300 && y >= 3) {
                    int y2 = (int)((y-3)/10+0.5);
                    if (0 == y2%2 ) {
                        controlData[y2/2] = (controlData[y2/2]+1)%2;
                    }
                    
                }
            };
            if (e.type == MotionNotify && 1 == controlData[0]) {
                mousePos[0] = e.xbutton.x;
                mousePos[1] = e.xbutton.y;
                // send 1 byte identier or can I differentiate on server side??
                char fakeheader = 'm';
                send(clientSocket, &fakeheader, sizeof(fakeheader), 0);// sizeof(fakeheader) is 1
                int bytesSent = send(clientSocket, &mousePos, 8, 0);
                if (bytesSent  != 8) {
                    perror("Send mouse position failed");
                    break;
                }
                XClearArea(display, window, 3, winAttr.height - 13, 140, 10, False);// false means doesn't trigger expose event
                string text = "Pointer at: " + to_string(e.xbutton.x) + ", " + to_string(e.xbutton.y);
                XSetForeground(display, gc, black);
                XDrawString(display,window,gc,3,winAttr.height - 3,text.c_str(),strlen(text.c_str()));
            };
        }
        
        updateControls(0, 0);

        usleep(1000); // 1 ms = 1000 FPS
    }

    recieve.join();

    return 0;
}
