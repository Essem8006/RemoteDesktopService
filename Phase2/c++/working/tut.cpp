#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <vector>
#include <unistd.h> // for usleep()
#include <string.h>

using namespace std;

int floor(double i) {
    return (int)i;
}

int main() {
    for (int x=0;x<50;x++) {
        double d = (x-3)/10;
        int y2 = (int)((x-3)/10+0.5);
        cout << floor(x/10) << endl;
    }
    return 0;
}

