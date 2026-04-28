#include "Core/Common.hpp"
#include "Core/Memory.hpp"
#include "Core/Stealth.hpp"
#include "Core/ModuleStomp.hpp"
#include "Core/StackSpoof.hpp"
#include "Core/ScreenGuard.hpp"
#include "Game/Offsets.hpp"
#include "Features/LocalPlayer.hpp"
#include "Features/ESP.hpp"
#include "UI/Menu.hpp"
#include "UI/Overlay.hpp"
#include "Hooks.h"
#include "LuaEngine.h"

// Global instances
LE g_le;

// HMODULE'ü global tut (thread pool callback'ten erişim için)
static HMODULE g_hModule = nullptr;

// ============================================================
//  Ana iş parçacığı
//  Thread Pool üzerinden çalıştırılır → Call stack'te
//  ntdll.dll thread pool fonksiyonları görünür (tamamen meşru)
// ============================================================

static void InternalLoop() {
    Stealth::g_api.Init();

    // Wait for game load
    if (Stealth::g_api.pSleep) {
        Stealth::g_api.pSleep(5000);
    } else {
        Sleep(5000);
    }

    // Tüm hook ve anti-detection katmanlarını başlat
    Hk::Setup();

    // Lua engine
    g_le.Start();

    // UI başlat (D3D internal rendering + screenshot korumalı)
    UI::Wnd::Init();
    UI::Ovl::Init();

    while (true) {
        // Toggle menu
        SHORT ks = 0;
        if (Stealth::g_api.pGetAsyncKeyState) {
            ks = Stealth::g_api.pGetAsyncKeyState(VK_INSERT);
        } else {
            ks = GetAsyncKeyState(VK_INSERT);
        }
        if (ks & 1) {
            UI::Wnd::Toggle();
        }

        // Unload
        SHORT uk = 0;
        if (Stealth::g_api.pGetAsyncKeyState) {
            uk = Stealth::g_api.pGetAsyncKeyState(VK_END);
        } else {
            uk = GetAsyncKeyState(VK_END);
        }
        if (uk & 1) {
            break;
        }

        // Message pump
        MSG msg;
        if (Stealth::g_api.pPeekMessageA) {
            while (Stealth::g_api.pPeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (Stealth::g_api.pTranslateMessage) Stealth::g_api.pTranslateMessage(&msg);
                if (Stealth::g_api.pDispatchMessageA) Stealth::g_api.pDispatchMessageA(&msg);
            }
        } else {
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        // Feature tick
        Feat::LP::Tick();
        
        // Overlay (sadece fallback modda - internal modda D3D handle eder)
        UI::Ovl::Sync();
        UI::Ovl::Paint();

        // ~60 FPS
        if (Stealth::g_api.pSleep) {
            Stealth::g_api.pSleep(16);
        } else {
            Sleep(16);
        }
    }

    // Cleanup
    Hk::Teardown();
    g_le.Stop();

    if (UI::Wnd::hW) {
        if (Stealth::g_api.pDestroyWindow) {
            Stealth::g_api.pDestroyWindow(UI::Wnd::hW);
        } else {
            DestroyWindow(UI::Wnd::hW);
        }
    }
    if (UI::Ovl::hO) {
        if (Stealth::g_api.pDestroyWindow) {
            Stealth::g_api.pDestroyWindow(UI::Ovl::hO);
        } else {
            DestroyWindow(UI::Ovl::hO);
        }
    }

    FreeLibraryAndExitThread(g_hModule, 0);
}

// ============================================================
//  Thread Pool Work Callback
//  Windows Thread Pool üzerinden çalışır.
//  Call stack:
//    ntdll.dll!TppWorkerThread
//    ntdll.dll!TppWorkpExecute
//    kernel32.dll!BaseThreadInitThunk
//    → Tamamen meşru görünür!
// ============================================================

static void CALLBACK _WorkCallback(PTP_CALLBACK_INSTANCE Instance, 
                                    PVOID Context, 
                                    PTP_WORK Work) {
    (void)Instance; (void)Context; (void)Work;
    InternalLoop();
}

// ============================================================
//  İlk giriş noktası (DllMain'den çağrılır)
//  Anti-detection kontrollerini yapar, sonra Thread Pool'a yönlendirir
// ============================================================

DWORD WINAPI _EntryPoint(LPVOID lpParam) {
    g_hModule = (HMODULE)lpParam;

    // [1] Thread identity spoofing
    Stealth::SpoofThread();

    // [2] Anti-debug gate
    if (Stealth::IsBeingDebugged()) {
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }

    // [3] Timing anomaly check (VM/sandbox detection)
    if (Stealth::IsTimingAnomalous()) {
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }

    // [4] PE header silme (dump analizi zorlaştırma)
    Stealth::ErasePEHeader(g_hModule);

    // [5] Ana loop'u Thread Pool üzerinden çalıştır
    //     Bu sayede call stack tamamen ntdll thread pool
    //     fonksiyonlarından oluşur - FiveM için meşru görünür
    PTP_WORK work = CreateThreadpoolWork(_WorkCallback, nullptr, nullptr);
    if (work) {
        SubmitThreadpoolWork(work);
        // Work callback FreeLibraryAndExitThread çağıracağı için
        // burada beklemiyoruz
        CloseThreadpoolWork(work);
    } else {
        // Fallback: doğrudan çalıştır
        InternalLoop();
    }

    return 0;
}

// ============================================================
//  DllMain - Minimal footprint
// ============================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(NULL, 0, _EntryPoint, hModule, 0, NULL);
        if (hThread) CloseHandle(hThread);
    }
    return TRUE;
}
