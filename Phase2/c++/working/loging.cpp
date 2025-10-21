#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <vector>
#include <unistd.h> // for usleep()
#include <string.h>

using namespace std;

Display *display;
int screen;
Window window;
GC gc;
unsigned long black, grey, light, white;

int controlData[7] = {0,0,0,0,0,0,0};
string controlLabel[7] = {"Control pointer", "Disconnect", "Option 3", ""};

void init();
void close();
void draw();
unsigned long RGB(int r, int g, int b) {
    return (r<<16) + (g<<8) + b;
}

int floor(double i) {
    return (int)i;
}

struct Frame {
    int width;
    int height;
    vector<unsigned char> data; // BGR format
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

XImage* convertFrameToImage(Frame frame, Display* display, int screen, int imgWidth, int imgHeight) {
    XImage* image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0,
        (char*)malloc(imgWidth * imgHeight * 4), imgWidth, imgHeight, 32, 0
    );
    //If you mean change the pixel dimensions of the XImage, no, there is no way to do that. If you want to scale the image pixels, you would use XRender on the server side or something like the Cairo library on the client side. But you still will have to scale to a different image (or window or pixmap), since you can't resize an allocated XImage.
    for (int y = 0; y < imgHeight; ++y) {
        for (int x = 0; x < imgWidth; ++x) {
            int idx = (y * frame.width + floor(x*frame.width/imgWidth)) * 3;
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

int main() {
    init();

    XEvent e;
    while (true) {
        XWindowAttributes winAttr;
        XGetWindowAttributes(display, window, &winAttr);
        int winHeight = winAttr.height;
        int winWidth = winAttr.width;
        // event loop
        while (XPending(display)) {
            XNextEvent(display, &e);
            if (e.type == KeyPress) close();
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
            if (e.type == MotionNotify) {
                int x = e.xbutton.x, y = e.xbutton.y;
                XClearArea(display, window, 3, winHeight - 13, 200, 10, False);// false means doesn't trigger expose event
                string text = "Pointer at: " + to_string(x) + ", " + to_string(y);
                XSetForeground(display, gc, black);
                XDrawString(display,window,gc,3,winHeight - 3,text.c_str(),strlen(text.c_str()));
            };
        }
        updateControls(0, 0);

        Frame frame = captureScreenFrame(display);
        if (frame.data.empty()) {
            cerr << "Failed to capture frame.\n";
            break;
        }
        XImage* image = convertFrameToImage(frame, display, screen, winWidth-153, winHeight-6);
        XPutImage(display, window, gc, image, 0, 0, 150, 3, winWidth-153, winHeight-6);
        XDestroyImage(image);

        usleep(10000); // 10 ms = 100 FPS
    }

    return 0;
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