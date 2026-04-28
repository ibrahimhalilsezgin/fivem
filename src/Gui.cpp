#include "Gui.h"
#include <Windows.h>
#include <string>
#include <vector>
#include "LuaEngine.h"
#include "Core/Stealth.hpp"
#include "Core/ScreenGuard.hpp"

extern LE g_le;

namespace Scr {
    static HWND _hW = nullptr;
    static HWND _hE = nullptr;
    static HWND _hB = nullptr;
    static HANDLE _hT = nullptr;
    static char _gcn[12] = {0};

    LRESULT CALLBACK _GP(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_COMMAND:
                if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
                    int len = GetWindowTextLengthA(_hE);
                    if (len > 0) {
                        std::vector<char> buf(len + 1);
                        GetWindowTextA(_hE, buf.data(), len + 1);
                        g_le.Exec(buf.data());
                        // Zero out buffer
                        SecureZeroMemory(buf.data(), buf.size());
                    }
                }
                break;
            case WM_CLOSE:
                DestroyWindow(hwnd);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    DWORD WINAPI _GT(LPVOID) {
        // Spoof thread identity
        Stealth::SpoofThread();

        HINSTANCE hI = GetModuleHandle(nullptr);
        Stealth::GenClassName(_gcn, sizeof(_gcn));

        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = _GP;
        wc.hInstance = hI;
        wc.lpszClassName = _gcn;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassA(&wc);

        // Random window title
        char wt[12];
        Stealth::GenClassName(wt, sizeof(wt));

        _hW = CreateWindowExA(0, _gcn, wt, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 450, 350,
                              nullptr, nullptr, hI, nullptr);

        if (!_hW) return 0;

        _hE = CreateWindowExA(0, "EDIT", "",
                              WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                              10, 10, 410, 240, _hW, nullptr, hI, nullptr);

        _hB = CreateWindowExA(0, "BUTTON", "OK",
                              WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              10, 260, 150, 35, _hW, (HMENU)1, hI, nullptr);

        ShowWindow(_hW, SW_SHOW);
        UpdateWindow(_hW);

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UnregisterClassA(_gcn, hI);
        _hW = nullptr;
        return 0;
    }

    void Open() {
        _hT = CreateThread(nullptr, 0, _GT, nullptr, 0, nullptr);
    }

    void Close() {
        if (_hW) SendMessage(_hW, WM_CLOSE, 0, 0);
        if (_hT) {
            WaitForSingleObject(_hT, 1000);
            CloseHandle(_hT);
            _hT = nullptr;
        }
    }
}
