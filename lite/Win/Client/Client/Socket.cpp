#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Socket.h"

int InitSocket(const std::string& ip, uint16_t port, SOCKET& out) {
    // start
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)  return 1;

    // get socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return 1;

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    //connect
    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) return 1;

    out = sock;

    return 0;
};

void CloseSocket(SOCKET sock) {
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup();
};

CRITICAL_SECTION frameLock;
bool frameAvailable = false;
Frame latestFrame{};

DWORD WINAPI recieveFrames(LPVOID lpParam) {
    SOCKET sock = *(SOCKET*)lpParam;

    while (true) {
        // get header
        int buffer;
        recv(sock, (char*)&buffer, sizeof(buffer), 0);
        int height = ntohl(buffer);        // convert back to host byte order

        buffer = 0;
        recv(sock, (char*)&buffer, sizeof(buffer), 0);
        int width = ntohl(buffer);        // convert back to host byte order

        // construct frame
        Frame frame{};
        frame.height = height;
        frame.width = width;
        frame.data.resize(width * height * 4);

        int len = frame.data.size();
        char* ptr = reinterpret_cast<char*>(frame.data.data());
        while (len > 0) {
            int chunkSize = min(len, 1024);
            int bytesRecieved = recv(sock, ptr, chunkSize, MSG_WAITALL);
            if (bytesRecieved <= 0) {
                MessageBox(NULL, L"Recieve failed", L"Error", MB_OK | MB_ICONERROR);
                break;
            }
            ptr += bytesRecieved;
            len -= bytesRecieved;
        }


        EnterCriticalSection(&frameLock);
        latestFrame = frame;
        frameAvailable = true;
        LeaveCriticalSection(&frameLock);
    }

    return 0;
}