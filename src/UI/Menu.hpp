#pragma once
#include <windows.h>

namespace UI {
    class Menu {
    public:
        static void Initialize();
        static void ToggleVisibility();
        static void HandleInput(WPARAM wp);
        static void Render(HDC hdc);

        static bool IsOpen;
        static HWND hMenu;
    };
}
