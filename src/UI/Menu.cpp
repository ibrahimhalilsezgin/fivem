#include "Core/Common.hpp"
#include "UI/Menu.hpp"
#include "UI/Drawing.hpp"
#include "Features/LocalPlayer.hpp"
#include "Features/ESP.hpp"

namespace UI {
    bool Menu::IsOpen = false;
    HWND Menu::hMenu = NULL;

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_COMMAND) {
            int cmd = LOWORD(wp);
            if (cmd == 1) Features::LocalPlayer::GodMode = !Features::LocalPlayer::GodMode;
            if (cmd == 2) Features::LocalPlayer::InfArmor = !Features::LocalPlayer::InfArmor;
            if (cmd == 3) Features::ESP::Enabled = !Features::ESP::Enabled;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        if (msg == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            SetBkMode(hdc, TRANSPARENT);
            
            char buf[64];
            sprintf_s(buf, "%s God Mode", Features::LocalPlayer::GodMode ? "[+]" : "[-] ");
            Drawing::Text(hdc, 20, 140, buf);

            sprintf_s(buf, "%s Armor", Features::LocalPlayer::InfArmor ? "[+]" : "[-] ");
            Drawing::Text(hdc, 20, 170, buf);

            sprintf_s(buf, "%s ESP BOX", Features::ESP::Enabled ? "[+]" : "[-] ");
            Drawing::Text(hdc, 20, 200, buf);

            EndPaint(hwnd, &ps);
        }
        if (msg == WM_CLOSE) ShowWindow(hwnd, SW_HIDE);
        return DefWindowProcA(hwnd, msg, wp, lp);
    }

    void Menu::Initialize() {
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = (WNDPROC)WndProc;
        wc.lpszClassName = "ModularMenuClass";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassA(&wc);

        hMenu = CreateWindowExA(WS_EX_TOPMOST, "ModularMenuClass", "Modular Internal v6", 
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 
                                100, 100, 300, 310, NULL, NULL, NULL, NULL);
        
        CreateWindowA("BUTTON", "Toggle God Mode", WS_CHILD | WS_VISIBLE, 20, 10, 240, 35, hMenu, (HMENU)1, NULL, NULL);
        CreateWindowA("BUTTON", "Toggle Armor", WS_CHILD | WS_VISIBLE, 20, 50, 240, 35, hMenu, (HMENU)2, NULL, NULL);
        CreateWindowA("BUTTON", "Toggle ESP", WS_CHILD | WS_VISIBLE, 20, 90, 240, 35, hMenu, (HMENU)3, NULL, NULL);
        
        ShowWindow(hMenu, SW_HIDE);
    }

    void Menu::ToggleVisibility() {
        IsOpen = !IsOpen;
        ShowWindow(hMenu, IsOpen ? SW_SHOW : SW_HIDE);
    }
}
