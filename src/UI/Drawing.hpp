#pragma once
#include "../Core/Common.hpp"

namespace UI {
    class Gfx {
    public:
        static void Txt(HDC hdc, int x, int y, const char* t) {
            if (!t) return;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));
            ::TextOutA(hdc, x + 1, y + 1, (LPCSTR)t, (int)strlen(t));
            SetTextColor(hdc, RGB(0, 255, 0));
            ::TextOutA(hdc, x, y, (LPCSTR)t, (int)strlen(t));
        }

        static void Rect(HDC hdc, int x, int y, int w, int h, COLORREF c) {
            HPEN hPen = CreatePen(PS_SOLID, 2, c);
            HPEN hOld = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
            
            ::Rectangle(hdc, x, y, x + w, y + h);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOld);
            DeleteObject(hPen);
        }
    };
}
