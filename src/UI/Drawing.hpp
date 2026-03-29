#pragma once
#include "../Core/Common.hpp"

namespace UI {
    class Drawing {
    public:
        static void Text(HDC hdc, int x, int y, const char* text) {
            if (!text) return;
            // High visibility shadow text
            SetBkMode(hdc, TRANSPARENT);
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
}
