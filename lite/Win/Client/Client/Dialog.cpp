#include "resource.h"
#include "Dialog.h"

std::string servAddr;
char* passwordEntered = nullptr;

INT_PTR CALLBACK InputDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static InputDialogMode mode;
    switch (msg) {
    case WM_INITDIALOG: {
        mode = (InputDialogMode)lParam;
        // set caption and text
        if (mode == InputDialogMode::IP) {
            SetWindowText(hDlg, L"Please enter the server IP");
            SetDlgItemText(hDlg, IDC_LABEL, L"IP:");
        }
        else {
            SetWindowText(hDlg, L"Please enter the server password");
            SetDlgItemText(hDlg, IDC_LABEL, L"Password:");
        }
        return TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            // general conversion
            wchar_t buffer[64];
            GetDlgItemTextW(hDlg, IDC_EDIT, buffer, 64);
            int size = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);

            if (mode == InputDialogMode::IP) {
                // convert to string
                std::string result(size - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, buffer, -1, result.data(), size, nullptr, nullptr);
                servAddr = result;
            }
            else {
                // convert to char
                passwordEntered = new char[size];
                WideCharToMultiByte(CP_UTF8, 0, buffer, -1, passwordEntered, size, nullptr, nullptr);
            }
            EndDialog(hDlg, IDOK);
            return TRUE;

        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }
    break;
    }
    return FALSE;
}