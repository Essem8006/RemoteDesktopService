#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <vector>
#include <unistd.h> // for usleep()

using namespace std;

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

XImage* convertFrameToImage(Frame frame, Display* display, int screen) {
    XImage* image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0,
        (char*)malloc(frame.width * frame.height * 4), frame.width/2, frame.height/2, 32, 0
    );

    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            int idx = (y * frame.width + x) * 3;
            unsigned char b = frame.data[idx];
            unsigned char g = frame.data[idx + 1];
            unsigned char r = frame.data[idx + 2];
            unsigned long pixel = (r << 16) | (g << 8) | b;// << x moves bits x spaces to the left, so this creates a 24 bit colour value
            XPutPixel(image, x, y, pixel);
        }
    }

    return image;
}

int main() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        cerr << "Failed to open X display.\n";
        return 1;
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 800, 600, 1, BlackPixel(display, screen), WhitePixel(display, screen));//border_width bordercol and backgroundcol at end

    XSelectInput(display, window, ExposureMask | KeyPressMask);// tells X server what events we want from it
    XMapWindow(display, window);
    GC gc = XCreateGC(display, window, 0, nullptr);

    bool running = true;
    XEvent e;

    while (running) {
        // Non-blocking event check
        while (XPending(display)) {
            XNextEvent(display, &e);
            if (e.type == KeyPress) running = false;
        }

        Frame frame = captureScreenFrame(display);
        if (frame.data.empty()) {
            cerr << "Failed to capture frame.\n";
            break;
        }

        // Create an XImage from the captured frame
        XImage* image = convertFrameToImage(frame, display, screen);

        // Scale the image to window size if needed (simple XPutImage stretch)
        XWindowAttributes winAttr;
        XGetWindowAttributes(display, window, &winAttr);
        XPutImage(display, window, gc, image, 0, 0, 0, 0, winAttr.width, winAttr.height);

        XDestroyImage(image);
        usleep(10000); // 10 ms = 100 FPS
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
