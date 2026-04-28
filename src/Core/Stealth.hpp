#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#include <cstdint>
#include "Crypto.hpp"

// ============================================================
//  Dynamic API resolution & Anti-detection utilities
//  Import tablosunda şüpheli API çağrıları görünmez
// ============================================================

namespace Stealth {

    // Dynamic API loader - GetProcAddress wrapper
    template<typename F>
    __forceinline F Api(const char* mod, const char* fn) {
        HMODULE h = GetModuleHandleA(mod);
        if (!h) h = LoadLibraryA(mod);
        if (!h) return nullptr;
        return reinterpret_cast<F>(GetProcAddress(h, fn));
    }

    // Dynamic typed API call helpers
    using fn_GetModuleHandleA   = HMODULE(WINAPI*)(LPCSTR);
    using fn_GetModuleInformation = BOOL(WINAPI*)(HANDLE, HMODULE, LPVOID, DWORD);
    using fn_FindWindowA        = HWND(WINAPI*)(LPCSTR, LPCSTR);
    using fn_CreateWindowExA    = HWND(WINAPI*)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
    using fn_ShowWindow         = BOOL(WINAPI*)(HWND, int);
    using fn_SetWindowPos       = BOOL(WINAPI*)(HWND, HWND, int, int, int, int, UINT);
    using fn_GetAsyncKeyState   = SHORT(WINAPI*)(int);
    using fn_Sleep              = void(WINAPI*)(DWORD);
    using fn_CreateThread       = HANDLE(WINAPI*)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
    using fn_SetLayeredWindowAttributes = BOOL(WINAPI*)(HWND, COLORREF, BYTE, DWORD);
    using fn_GetWindowRect      = BOOL(WINAPI*)(HWND, LPRECT);
    using fn_DestroyWindow      = BOOL(WINAPI*)(HWND);
    using fn_PeekMessageA       = BOOL(WINAPI*)(LPMSG, HWND, UINT, UINT, UINT);
    using fn_TranslateMessage   = BOOL(WINAPI*)(const MSG*);
    using fn_DispatchMessageA   = LRESULT(WINAPI*)(const MSG*);
    using fn_RegisterClassA     = ATOM(WINAPI*)(const WNDCLASSA*);

    // Cached API pointers (lazy init)
    struct ApiCache {
        fn_GetAsyncKeyState  pGetAsyncKeyState  = nullptr;
        fn_FindWindowA       pFindWindowA       = nullptr;
        fn_Sleep             pSleep             = nullptr;
        fn_ShowWindow        pShowWindow        = nullptr;
        fn_SetWindowPos      pSetWindowPos      = nullptr;
        fn_GetWindowRect     pGetWindowRect     = nullptr;
        fn_DestroyWindow     pDestroyWindow     = nullptr;
        fn_PeekMessageA      pPeekMessageA      = nullptr;
        fn_TranslateMessage  pTranslateMessage  = nullptr;
        fn_DispatchMessageA  pDispatchMessageA  = nullptr;
        fn_SetLayeredWindowAttributes pSetLayered = nullptr;

        bool initialized = false;

        void Init() {
            if (initialized) return;
            pGetAsyncKeyState = Api<fn_GetAsyncKeyState>("user32.dll", "GetAsyncKeyState");
            pFindWindowA      = Api<fn_FindWindowA>("user32.dll", "FindWindowA");
            pSleep            = Api<fn_Sleep>("kernel32.dll", "Sleep");
            pShowWindow       = Api<fn_ShowWindow>("user32.dll", "ShowWindow");
            pSetWindowPos     = Api<fn_SetWindowPos>("user32.dll", "SetWindowPos");
            pGetWindowRect    = Api<fn_GetWindowRect>("user32.dll", "GetWindowRect");
            pDestroyWindow    = Api<fn_DestroyWindow>("user32.dll", "DestroyWindow");
            pPeekMessageA     = Api<fn_PeekMessageA>("user32.dll", "PeekMessageA");
            pTranslateMessage = Api<fn_TranslateMessage>("user32.dll", "TranslateMessage");
            pDispatchMessageA = Api<fn_DispatchMessageA>("user32.dll", "DispatchMessageA");
            pSetLayered       = Api<fn_SetLayeredWindowAttributes>("user32.dll", "SetLayeredWindowAttributes");
            initialized = true;
        }
    };

    inline ApiCache g_api;

    // Anti-debug checks
    __forceinline bool IsBeingDebugged() {
        // PEB->BeingDebugged check (manual, no API call)
#ifdef _WIN64
        uint8_t* pPeb = (uint8_t*)__readgsqword(0x60);
#else
        uint8_t* pPeb = (uint8_t*)__readfsdword(0x30);
#endif
        if (pPeb && pPeb[2]) return true;

        // NtGlobalFlag check
        DWORD ntGlobalFlag = 0;
#ifdef _WIN64
        ntGlobalFlag = *(DWORD*)(pPeb + 0xBC);
#else
        ntGlobalFlag = *(DWORD*)(pPeb + 0x68);
#endif
        // FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS
        if (ntGlobalFlag & 0x70) return true;

        return false;
    }

    // Timing-based anti-analysis
    __forceinline bool IsTimingAnomalous() {
        LARGE_INTEGER freq, t1, t2;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&t1);
        
        // Do some minimal work
        volatile int x = 0;
        for (volatile int i = 0; i < 100; i++) x += i;
        
        QueryPerformanceCounter(&t2);
        
        // If this trivial loop took more than 50ms, something is watching
        double elapsed = (double)(t2.QuadPart - t1.QuadPart) / freq.QuadPart;
        return elapsed > 0.05;
    }

    // Generate pseudo-random window class name at runtime
    inline void GenClassName(char* buf, size_t len) {
        DWORD tick = GetTickCount();
        DWORD pid = GetCurrentProcessId();
        DWORD seed = tick ^ (pid << 7);
        
        const char charset[] = "abcdefghijklmnopqrstuvwxyz";
        for (size_t i = 0; i < len - 1; i++) {
            seed = seed * 1103515245 + 12345;
            buf[i] = charset[(seed >> 16) % 26];
        }
        buf[len - 1] = '\0';
    }

    // Erase PE header from memory (makes dump analysis harder)
    inline void ErasePEHeader(HMODULE hModule) {
        DWORD oldProtect = 0;
        VirtualProtect(hModule, 4096, PAGE_READWRITE, &oldProtect);
        SecureZeroMemory(hModule, 4096);
        VirtualProtect(hModule, 4096, oldProtect, &oldProtect);
    }

    // Spoof thread start address (mislead thread enumeration)
    inline void SpoofThread() {
        // Change thread description to something innocent
        typedef HRESULT(WINAPI* fn_SetThreadDescription)(HANDLE, PCWSTR);
        auto pSetDesc = Api<fn_SetThreadDescription>("kernel32.dll", "SetThreadDescription");
        if (pSetDesc) {
            pSetDesc(GetCurrentThread(), L"ntdll.dll!TppWorkerThread");
        }
    }
}
