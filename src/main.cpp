#include "Core/Common.hpp"
#include "Core/Memory.hpp"
#include "Game/Offsets.hpp"
#include "Features/LocalPlayer.hpp"
#include "Features/ESP.hpp"
#include "UI/Menu.hpp"
#include "UI/Overlay.hpp"
#include <stdio.h>

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Initial wait for game load
    Sleep(5000);

    // Initialize UI
    UI::Menu::Initialize();
    UI::Overlay::Initialize();

    MessageBoxA(NULL, "Modular Internal v6 Loaded!\n\nINSERT: Menu\nEND: Unload", "Modular Engine", MB_OK);

    while (true) {
        // Toggle Menu
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            UI::Menu::ToggleVisibility();
        }

        // Unload Cheat
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }

        // Handle UI Messages
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        // Feature Updates
        Features::LocalPlayer::Update();
        
        // UI Updates & Rendering
        UI::Overlay::Update();
        UI::Overlay::Render();

        Sleep(16); // ~60FPS
    }

    // Cleanup
    if (UI::Menu::hMenu) DestroyWindow(UI::Menu::hMenu);
    if (UI::Overlay::hOverlay) DestroyWindow(UI::Overlay::hOverlay);
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
        if (hThread) CloseHandle(hThread);
    }
    return TRUE;
}
