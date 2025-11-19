// HelloWindowsDesktop.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <intsafe.h>
#include <vector>

#include <iostream>

using namespace std;

void DisplayCapturedImage(HDC hdc, const vector<BYTE>& pixelData, int width, int height)
{
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // Negative height for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // 32-bit (RGBA)

    // Create a DIB (Device Independent Bitmap) section and select into the DC
    HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, pixelData.data(), &bmi, DIB_RGB_COLORS);

    // Display the bitmap on the screen
    HDC memDC = CreateCompatibleDC(hdc);
    SelectObject(memDC, hBitmap);
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // Clean up
    DeleteObject(hBitmap);
    DeleteDC(memDC);
}

void screenShot(HDC hdc) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create a device context (DC) for the screen
    HDC hScreenDC = GetDC(NULL);  // NULL refers to the entire screen
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC); // DC for in-memory bitmap

    // Create a compatible bitmap for the screen capture
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);

    // Select the bitmap into the memory DC
    SelectObject(hMemoryDC, hBitmap);

    // Perform the screen capture (BitBlt from screen DC to memory DC)
    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

    // Get the pixel data into a DIB (Device Independent Bitmap) structure
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = screenWidth;
    bmi.bmiHeader.biHeight = -screenHeight;  // Negative for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // 32-bit color depth (BGRA)

    // Allocate memory for the pixel data
    int pixelDataSize = screenWidth * screenHeight * 4;  // 4 bytes per pixel (BGRA)
    vector<BYTE> pixelData(pixelDataSize);

    // Get the pixel data from the bitmap into the buffer
    GetDIBits(hMemoryDC, hBitmap, 0, screenHeight, pixelData.data(), &bmi, DIB_RGB_COLORS);

    // Do something with pixelData here (e.g., display, processing, etc.)
    DisplayCapturedImage(hdc, pixelData, screenWidth, screenHeight);

    // Clean up
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
}

// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Word");

// Stored instance handle for use in Win32 API calls such as FindResource
HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH) CreateSolidBrush(RGB(200, 200, 200));
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Word"),
            NULL);

        return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindowEx explained:
    // WS_EX_OVERLAPPEDWINDOW : An optional extended window style.
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, height)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 100,
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

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindowEx
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd, 1, 10, NULL);//Animation loop: 10ms = 100fps target

    SetTimer(hWnd, 2, 1000, NULL);//actual fps counter

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

RECT clientRect;
int mouseX = 0;
int mouseY = 0;
int frames = 0;
int fps = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR ptrText[50];
    swprintf_s(ptrText, _T("Pointer at: %d, %d. FPS: %d         "), mouseX, mouseY, fps);// blank space a bad fix that works

    switch (message)
    {
    case WM_TIMER:
    {
        switch (wParam)
        {
        case 1:   // Animation timer
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW);
            break;

        case 2:   // FPS timer
            // Here you can compute FPS once per second
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
        InvalidateRect(hWnd, NULL, FALSE);// to call paint
    }
    break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        screenShot(hdc);

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
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

