#include "Overlay.hpp"
#include "../Features/ESP.hpp"
#include "../Core/Stealth.hpp"
#include "../Core/Crypto.hpp"
#include "../Core/ScreenGuard.hpp"
#include "../Render/D3DRenderer.hpp"

namespace UI {
    HWND Ovl::hO = NULL;
    int Ovl::sW = 1920;
    int Ovl::sH = 1080;
    bool Ovl::bUseInternal = true; // Default: D3D internal rendering

    // --------------------------------------------------------
    //  INTERNAL RENDERING PATH (D3D11 VMT Hook)
    //  Harici pencere oluşturmaz, doğrudan oyunun
    //  render pipeline'ında çizer.
    //  FiveM overlay window tespitini tamamen aşar.
    // --------------------------------------------------------

    void Ovl::Init() {
        // Screenshot korumasını başlat
        ScreenGuard::Initialize();

        if (bUseInternal) {
            // D3D11 internal rendering
            // SwapChain hook'u Hooks.cpp'de module stomping ile kurulur
            // Burada sadece draw callback'i kaydet
            Render::g_drawCb = &Render::OnFrame;
            return;
        }

        // --------------------------------------------------------
        //  FALLBACK: External overlay (sadece D3D bulunamazsa)
        //  Ama screenshot koruması + display affinity ile
        // --------------------------------------------------------

        static char _ocn[12] = {0};
        Stealth::GenClassName(_ocn, sizeof(_ocn));

        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = DefWindowProcA;
        wc.lpszClassName = _ocn;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassA(&wc);

        hO = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            _ocn, "", WS_POPUP,
            0, 0, sW, sH, NULL, NULL, NULL, NULL
        );

        Stealth::g_api.Init();
        if (Stealth::g_api.pSetLayered) {
            Stealth::g_api.pSetLayered(hO, RGB(0, 0, 0), 0, LWA_COLORKEY);
        } else {
            SetLayeredWindowAttributes(hO, RGB(0, 0, 0), 0, LWA_COLORKEY);
        }

        ShowWindow(hO, SW_SHOW);
    }

    void Ovl::Sync() {
        if (bUseInternal) return; // Internal modda sync gerekmez

        XC("grcWindow", gwc);

        Stealth::g_api.Init();
        HWND gw = nullptr;
        if (Stealth::g_api.pFindWindowA) {
            gw = Stealth::g_api.pFindWindowA(gwc, NULL);
        } else {
            gw = FindWindowA(gwc, NULL);
        }

        if (gw) {
            RECT r;
            GetWindowRect(gw, &r);
            SetWindowPos(hO, HWND_TOPMOST, r.left, r.top, 
                        r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE);
            sW = r.right - r.left;
            sH = r.bottom - r.top;
        }
    }

    void Ovl::Paint() {
        if (bUseInternal) return; // D3D Present hook handle eder

        // Screenshot sırasında çizme
        if (!ScreenGuard::CanDraw()) return;

        if (!hO) return;
        HDC hdc = GetDC(hO);
        if (!hdc) return;

        RECT rc;
        GetClientRect(hO, &rc);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, sW, sH);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

        FillRect(memDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

        Feat::Vis::Draw(memDC, sW, sH);

        BitBlt(hdc, 0, 0, sW, sH, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(hO, hdc);
    }

    // Screenshot koruması: overlay'i gizle
    void Ovl::Hide() {
        if (bUseInternal) {
            // D3D modunda: ScreenGuard::CanDraw() false döner
            // Present hook çizim yapmaz
            return;
        }
        if (hO) ShowWindow(hO, SW_HIDE);
    }

    // Screenshot sonrası: overlay'i göster
    void Ovl::Show() {
        if (bUseInternal) return;
        if (hO) ShowWindow(hO, SW_SHOW);
    }
}
