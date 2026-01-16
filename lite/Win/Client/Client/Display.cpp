#include <windows.h>
#include <vector>
#include "Display.h"

void DisplayCapturedImage(HDC hdc, const std::vector<BYTE>& pixelData, int frameWidth, int frameHeight, int displayWidth, int displayHeight) {
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = frameWidth;
    bmi.bmiHeader.biHeight = -frameHeight;  // Negative height for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // 32-bit (RGBA)
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);

    StretchDIBits(hdc, 200, 0, displayWidth, displayHeight, 0, 0, frameWidth, frameHeight, pixelData.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}