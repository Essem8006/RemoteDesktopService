#pragma once

#include <windows.h>
#include <string>//?
#include <vector>


int InitSocket(const std::string& ip, uint16_t port, SOCKET& out);
void CloseSocket(SOCKET sock);

struct Frame {
    int height;
    int width;
    std::vector<BYTE> data;
};

extern CRITICAL_SECTION frameLock;
extern bool frameAvailable;
extern Frame latestFrame;

DWORD WINAPI recieveFrames(LPVOID lpParam);