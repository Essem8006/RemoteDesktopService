#pragma once
#include <windows.h>
#include <string>

enum class InputDialogMode {
    IP,
    Password
};

extern std::string servAddr;
extern char* passwordEntered;

INT_PTR CALLBACK InputDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);