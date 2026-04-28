#pragma once
#include <windows.h>

namespace UI {
    class Wnd {
    public:
        static void Init();
        static void Toggle();

        static bool bShow;
        static HWND hW;
    };
}
