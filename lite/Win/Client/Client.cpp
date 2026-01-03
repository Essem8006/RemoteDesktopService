// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include "resource.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string.h>
#include <tchar.h>
#include <intsafe.h>
#include <vector>
#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")


using namespace std;

string servAddr;

string WideToUTF8(const wchar_t* wstr)
{
    int size = WideCharToMultiByte(
        CP_UTF8, 0,
        wstr, -1,
        nullptr, 0,
        nullptr, nullptr
    );

    std::string result(size - 1, '\0');

    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr, -1,
        result.data(), size,
        nullptr, nullptr
    );

    return result;
}


INT_PTR CALLBACK InputDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t buffer[256];
            GetDlgItemTextW(hDlg, IDC_EDIT, buffer, 256);
            servAddr = WideToUTF8(buffer);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}


struct Frame {
    int height;
    int width;
    vector<BYTE> data;
};

int mouseX = 8888;
int mouseY = 3097;

CRITICAL_SECTION frameLock;
bool frameAvailable = false;
Frame latestFrame{};

void DisplayCapturedImage(HDC hdc, const vector<BYTE>& pixelData, int width, int height) {
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // negative for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // 32-bit (RGBA)

    HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, pixelData.data(), &bmi, DIB_RGB_COLORS);

    // display
    HDC memDC = CreateCompatibleDC(hdc);
    SelectObject(memDC, hBitmap);
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // clean up
    DeleteObject(hBitmap);
    DeleteDC(memDC);
}

DWORD WINAPI recieveFrames(LPVOID lpParam) {
    SOCKET sock = *(SOCKET*)lpParam;

    while (true) {
        // get header
        int buffer;
        recv(sock, (char*)&buffer, sizeof(buffer), 0);
        int height = ntohl(buffer);
        mouseX = height;

        buffer = 0;
        recv(sock, (char*)&buffer, sizeof(buffer), 0);
        int width = ntohl(buffer);        // convert back to host byte order
        mouseY = width;

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


// static PCSTR

static TCHAR szWindowClass[] = _T("DesktopApp");

static TCHAR szTitle[] = _T("Remote desktop client");

HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR     lpCmdLine, _In_ int       nCmdShow) {
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(200, 200, 200));
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Remote desktop client"),
            NULL);

        return 1;
    }

    hInst = hInstance;

    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindowEx failed!"),
            _T("Word"),
            NULL);

        return 1;
    }

    // init winSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        MessageBox(NULL, L"WSAStartup failed", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        MessageBox(NULL, L"Socket failed", L"Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    // get IP address input
    INT_PTR IP_input_result = DialogBox(
        hInstance,
        MAKEINTRESOURCE(101),// id for IDD_INPUTBOX
        hWnd,
        InputDlgProc
    );

    if (IP_input_result != 1)
    {
        MessageBox(NULL, L"Invalid or null IP address, closing program.", L"Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    // connect to server
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, servAddr.c_str(), &server.sin_addr);

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        MessageBox(NULL, L"Connection failed", L"Error", MB_OK | MB_ICONERROR);
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    MessageBox(NULL, L"Connection success", L"Success", MB_OK | MB_ICONINFORMATION);

    InitializeCriticalSection(&frameLock);

    HANDLE hThread = CreateThread( NULL, 0, recieveFrames, &sock, 0, NULL);

    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd, 1, 20, NULL); // 20ms = 50fps

    SetTimer(hWnd, 2, 1000, NULL); // actual fps counter

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    WSACleanup();
    return (int)msg.wParam;
}

RECT clientRect;
int frames = 0;
int fps = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR ptrText[50];
    vector<BYTE> pixelData;
    int screenWidth, screenHeight;
    swprintf_s(ptrText, _T("Pointer at: %d, %d. FPS: %d         "), mouseX, mouseY, fps);// blank space a bad fix that works

    switch (message)
    {
    case WM_TIMER:
    {
        switch (wParam)
        {
        case 1:
            EnterCriticalSection(&frameLock);
            if (frameAvailable) RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW);
            LeaveCriticalSection(&frameLock);
            break;

        case 2:
            fps = frames;
            frames = 0;
            break;
        }
    }
    break;

    case WM_MOUSEMOVE:
    {
        mouseX = (short)(lParam & 0xFFFF);
        mouseY = (short)((lParam >> 16) & 0xFFFF);
    }
    break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);

        EnterCriticalSection(&frameLock);
        DisplayCapturedImage(hdc, latestFrame.data, latestFrame.width, latestFrame.height);
        LeaveCriticalSection(&frameLock);

        GetClientRect(hWnd, &clientRect);
        SetBkColor(hdc, RGB(200, 200, 200));
        TextOut(hdc,
            5, clientRect.bottom - 20, // text height is 15
            ptrText, _tcslen(ptrText));

        EndPaint(hWnd, &ps);
        frames++;
        break;
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        KillTimer(hWnd, 2);
        PostQuitMessage(0);
        DeleteCriticalSection(&frameLock);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

