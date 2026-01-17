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

#include "Socket.h"
#include "Dialog.h"
#include "Display.h"

#pragma comment(lib, "ws2_32.lib")  // Winsock lib


using namespace std;


int mouseX = 8888;
int mouseY = 3097;


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
            _T("Remote Desktop client"),
            NULL);

        return 1;
    }

    // get IP address input
    INT_PTR IP_input_result = DialogBox(
        hInstance,
        MAKEINTRESOURCE(101), // IDD_INPUTBOX
        hWnd,
        InputDlgProc,
        (LPARAM)InputDialogMode::IP
    );

    if (IP_input_result != 1)
    {
        MessageBox(NULL, L"Invalid or null IP address, closing program.", L"Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    // init winsock
    SOCKET sock = INVALID_SOCKET;
    if (InitSocket(servAddr, 8080, sock) != 0) {
        MessageBox(NULL, L"Connection failed, closing program.", L"Error", MB_OK | MB_ICONERROR);
        CloseSocket(sock);
        return 1;
    }

    MessageBox(NULL, L"Connection success", L"Success", MB_OK | MB_ICONINFORMATION);

    // get password
    INT_PTR password_input_result = DialogBoxParam(
        hInstance,
        MAKEINTRESOURCE(101), // IDD_INPUTBOX
        hWnd,
        InputDlgProc,
        (LPARAM)InputDialogMode::Password
    );

    if (password_input_result != 1)
    {
        MessageBox(NULL, L"Invalid or null password, closing program.", L"Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    // send password
    int status = send(sock, passwordEntered, strlen(passwordEntered)+1, 0);
    

    // init for thread
    InitializeCriticalSection(&frameLock);

    HANDLE hThread = CreateThread(NULL, 0, recieveFrames, &sock,0, NULL);


    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd, 1, 10, NULL); // 10ms = 100fps

    SetTimer(hWnd, 2, 1000, NULL); // fps

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
        case 1:   // Animation timer
            EnterCriticalSection(&frameLock);
            if (frameAvailable) RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW);
            LeaveCriticalSection(&frameLock);
            break;

        case 2:   // FPS timer
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
        GetClientRect(hWnd, &clientRect);

        EnterCriticalSection(&frameLock);
        DisplayCapturedImage(hdc, latestFrame.data, latestFrame.width, latestFrame.height, clientRect.right -200, clientRect.bottom);
        LeaveCriticalSection(&frameLock);

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

