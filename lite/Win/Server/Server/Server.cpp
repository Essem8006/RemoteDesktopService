#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

using namespace std;

struct Frame {
    int height;
    int width;
    vector<BYTE> data;
};

static Frame screenShot() {
    Frame frame{};
    frame.width = GetSystemMetrics(SM_CXSCREEN);
    frame.height = GetSystemMetrics(SM_CYSCREEN);

    // Create a device context (DC) for the screen
    HDC hScreenDC = GetDC(NULL);  // NULL refers to the entire screen
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC); // DC for in-memory bitmap

    // Create a compatible bitmap for the screen capture
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, frame.width, frame.height);

    // Select the bitmap into the memory DC
    SelectObject(hMemoryDC, hBitmap);

    // Perform the screen capture (BitBlt from screen DC to memory DC)
    BitBlt(hMemoryDC, 0, 0, frame.width, frame.height, hScreenDC, 0, 0, SRCCOPY);

    // Get the pixel data into a DIB (Device Independent Bitmap) structure
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = frame.width;
    bmi.bmiHeader.biHeight = -frame.height;  // Negative for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // 32-bit color depth (BGRA)

    // Allocate memory for the pixel data
    int imageSize = frame.width * frame.height * 4;  // 4 bytes per pixel (BGRA)

    frame.data.resize(imageSize);

    // Get the pixel data from the bitmap into the buffer
    GetDIBits(hMemoryDC, hBitmap, 0, frame.height, frame.data.data(), &bmi, DIB_RGB_COLORS);

    // Clean up
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
    int port = 8080;

    WSADATA wsa;
    SOCKET server, client;
    sockaddr_in serverAddr{}, clientAddr{};
    int clientSize = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed\n";
        closesocket(server);
        return 1;
    }

    listen(server, 1);
    cout << "Server waiting on port " << port << "...\n";

    client = accept(server, (sockaddr*)&clientAddr, &clientSize);
    cout << "Client connected.\n";

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

    cout << "Done.\n";
    return 0;
}
