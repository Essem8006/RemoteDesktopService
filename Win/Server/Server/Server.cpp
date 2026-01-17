#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

using namespace std;

struct Frame {
    int height;
    int width;
    vector<BYTE> data;
};

string setPassword() {
    cout << "\033[33m" << "__________                       __           ________                 __      __                 " << endl; // first bit is colour code
    cout << "\\______   \\ ____   _____   _____/  |_  ____   \\______ \\   ____   _____|  | ___/  |_  ____ ______" << endl;
    cout << " |       _// __ \\ /     \\ /  _ \\   __\\/ __ \\   |    |  \\_/ __ \\ /  ___/  |/ /\\   __\\/  _ \\\\____ \\ " << endl;
    cout << " |    |   \\  ___/|  Y Y  (  <_> )  | \\  ___/   |    `   \\  ___/ \\___ \\|    <  |  | (  <_> )  |_> >" << endl;
    cout << " |____|_  /\\___  >__|_|  /\\____/|__|  \\___  > /_______  /\\___  >____  >__|_ \\ |__|  \\____/|   __/" << endl;
    cout << "        \\/     \\/      \\/                 \\/          \\/     \\/     \\/     \\/             |__ | " << endl << "\033[0m"; // reset to normal colour

    cout << endl << "Code by @essem8006, ASKII art by @patorjk" << endl;
    cout << endl << "To begin, please set a password: ";
    string password;
    cin >> password;
    return password;
}

static Frame screenShot() {
    Frame frame{};
    frame.width = GetSystemMetrics(SM_CXSCREEN);
    frame.height = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(NULL);  // NULL is for whole screen
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, frame.width, frame.height);

    SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, frame.width, frame.height, hScreenDC, 0, 0, SRCCOPY);

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = frame.width;
    bmi.bmiHeader.biHeight = -frame.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // 32-bit (BGRA)

    // set memory
    int imageSize = frame.width * frame.height * 4;  // 4 bytes for BGRA

    frame.data.resize(imageSize);

    GetDIBits(hMemoryDC, hBitmap, 0, frame.height, frame.data.data(), &bmi, DIB_RGB_COLORS);

    // tidy up the toys
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return frame;
}

bool sendAll(SOCKET s, const vector<BYTE>& data) {
    const char* buffer = reinterpret_cast<const char*>(data.data());
    size_t totalSent = 0;
    size_t dataSize = data.size();

    while (totalSent < dataSize)
    {
        int sent = send(
            s,
            buffer + totalSent,
            static_cast<int>(dataSize - totalSent),
            0
        );

        if (sent == SOCKET_ERROR) return false;

        totalSent += sent;
    }
    return true;
}

int main() {
    string password = setPassword();

    int port = 8080;

    WSADATA wsa;
    SOCKET server, client;
    sockaddr_in serverAddr{}, clientAddr{};
    int clientSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET)
    {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "Bind failed";
        closesocket(server);
        return 1;
    }

    listen(server, 1);
    cout << "Server waiting on port " << port << "..." << endl;

    client = accept(server, (sockaddr*)&clientAddr, &clientSize);
    cout << "Client connected." << endl;

    while (true) {
        Frame frame = screenShot();

        int height = htonl(frame.height);        // convert to network byte order
        send(client, (char*)&height, sizeof(height), 0);

        int width = htonl(frame.width);        // convert to network byte order
        send(client, (char*)&width, sizeof(width), 0);

        sendAll(client, frame.data);
    };

    closesocket(client);
    closesocket(server);
    WSACleanup();

    cout << "Done.";
    return 0;
}
