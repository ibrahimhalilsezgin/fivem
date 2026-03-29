#include "Overlay.hpp"
#include "../Features/ESP.hpp"

namespace UI {
    HWND Overlay::hOverlay = NULL;
    int Overlay::ScreenW = 1920;
    int Overlay::ScreenH = 1080;

    LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        return DefWindowProcA(hwnd, msg, wp, lp);
    }

    void Overlay::Initialize() {
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = (WNDPROC)OverlayWndProc;
        wc.lpszClassName = "ModularOverlayClass";
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassA(&wc);

        hOverlay = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
                                    "ModularOverlayClass", "", WS_POPUP,
                                    0, 0, ScreenW, ScreenH, NULL, NULL, NULL, NULL);

        SetLayeredWindowAttributes(hOverlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
        ShowWindow(hOverlay, SW_SHOW);
    }

    void Overlay::Update() {
        HWND gameWnd = FindWindowA("grcWindow", NULL);
        if (gameWnd) {
            RECT r;
            GetWindowRect(gameWnd, &r);
            SetWindowPos(hOverlay, HWND_TOPMOST, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE);
            ScreenW = r.right - r.left;
            ScreenH = r.bottom - r.top;
        }
    }

    class Drawing {
    public:
        static void Text(HDC hdc, int x, int y, const char* text) {
            if (!text) return;
            // Draw text with shadow for better visibility
            SetTextColor(hdc, RGB(0, 0, 0));
            ::TextOutA(hdc, x + 1, y + 1, (LPCSTR)text, (int)strlen(text));
            SetTextColor(hdc, RGB(0, 255, 0));
            ::TextOutA(hdc, x, y, (LPCSTR)text, (int)strlen(text));
        }

        static void Box(HDC hdc, int x, int y, int w, int h, COLORREF color) {
            HPEN hPen = CreatePen(PS_SOLID, 2, color);
            HPEN hOld = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
            
            ::Rectangle(hdc, x, y, x + w, y + h);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOld);
            DeleteObject(hPen);
        }
    };

    void Overlay::Render() {
        HDC hdc = GetDC(hOverlay);
        if (!hdc) return;

        RECT rc;
        GetClientRect(hOverlay, &rc);

        // --- Double Buffering (Memory DC) ---
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ScreenW, ScreenH);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

        // Clear Memory DC
        FillRect(memDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

        // Let ESP module draw to Memory DC
        Features::ESP::Render(memDC, ScreenW, ScreenH);

        // Blit to screen in ONE pass (No flickering)
        BitBlt(hdc, 0, 0, ScreenW, ScreenH, memDC, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(memDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(hOverlay, hdc);
    }
}
