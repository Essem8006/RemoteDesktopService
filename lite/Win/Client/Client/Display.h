#pragma once
#include <windows.h>
#include <vector>

void DisplayCapturedImage(HDC hdc, const std::vector<BYTE>& pixelData, int frameWidth, int frameHeight, int displayWidth, int displayHeight);