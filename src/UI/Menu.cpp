#include "Core/Common.hpp"
#include "UI/Menu.hpp"
#include "UI/Drawing.hpp"
#include "Features/LocalPlayer.hpp"
#include "Features/ESP.hpp"
#include "Core/ScreenGuard.hpp"

namespace UI {
    bool Wnd::bShow = false;
    HWND Wnd::hW = NULL;

    static char _wcn[12] = {0};

    LRESULT CALLBACK _WP(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_COMMAND) {
            int cmd = LOWORD(wp);
            if (cmd == 1) Feat::LP::bGod = !Feat::LP::bGod;
            if (cmd == 2) Feat::LP::bArm = !Feat::LP::bArm;
            if (cmd == 3) Feat::Vis::bOn = !Feat::Vis::bOn;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        if (msg == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            SetBkMode(hdc, TRANSPARENT);
            
            char buf[64];
            sprintf_s(buf, "%s M1", Feat::LP::bGod ? "[+]" : "[-]");
            Gfx::Txt(hdc, 20, 140, buf);

            sprintf_s(buf, "%s M2", Feat::LP::bArm ? "[+]" : "[-]");
            Gfx::Txt(hdc, 20, 170, buf);

            sprintf_s(buf, "%s M3", Feat::Vis::bOn ? "[+]" : "[-]");
            Gfx::Txt(hdc, 20, 200, buf);

            EndPaint(hwnd, &ps);
        }
        if (msg == WM_CLOSE) ShowWindow(hwnd, SW_HIDE);
        return DefWindowProcA(hwnd, msg, wp, lp);
    }

    void Wnd::Init() {
        Stealth::GenClassName(_wcn, sizeof(_wcn));

        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = (WNDPROC)_WP;
        wc.lpszClassName = _wcn;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassA(&wc);

        char wtitle[12];
        Stealth::GenClassName(wtitle, sizeof(wtitle));

        hW = CreateWindowExA(WS_EX_TOPMOST, _wcn, wtitle, 
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 
                            100, 100, 300, 310, NULL, NULL, NULL, NULL);
        
        CreateWindowA("BUTTON", "Toggle God Mode", WS_CHILD | WS_VISIBLE, 20, 10, 240, 35, hW, (HMENU)1, NULL, NULL);
        CreateWindowA("BUTTON", "Toggle Armor", WS_CHILD | WS_VISIBLE, 20, 50, 240, 35, hW, (HMENU)2, NULL, NULL);
        CreateWindowA("BUTTON", "Toggle ESP", WS_CHILD | WS_VISIBLE, 20, 90, 240, 35, hW, (HMENU)3, NULL, NULL);
        
        ShowWindow(hW, SW_HIDE);
    }

    void Wnd::Toggle() {
        bShow = !bShow;
        ShowWindow(hW, bShow ? SW_SHOW : SW_HIDE);
    }
}
