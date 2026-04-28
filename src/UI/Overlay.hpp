#pragma once
#include <windows.h>

namespace UI {
    class Ovl {
    public:
        static void Init();
        static void Sync();
        static void Paint();
        static void Hide();   // Screenshot sırasında gizle
        static void Show();   // Screenshot sonrası göster

        static HWND hO;
        static int sW;
        static int sH;

        // D3D internal rendering modu
        static bool bUseInternal;
    };
}
