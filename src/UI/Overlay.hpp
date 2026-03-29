#pragma once
#include <windows.h>

namespace UI {
    class Overlay {
    public:
        static void Initialize();
        static void Update();
        static void Render();

        static HWND hOverlay;
        static int ScreenW;
        static int ScreenH;
    };
}
