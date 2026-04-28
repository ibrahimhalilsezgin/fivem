#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>
#include "../Core/ModuleStomp.hpp"

// ============================================================
//  D3D11 Internal Renderer V3
//
//  ❌ Eski sorun: VMT pointer version.dll'e işaret ediyordu
//     → d3d11.dll'in pointer'ı version.dll'e = Anomali!
//
//  ✅ Yeni: VMT pointer d3d11.dll'in KENDİ code cave'ine
//     işaret eder. Pointer validasyonu:
//     - VMT[Present] adresi → d3d11.dll .text bölümü ✅
//     - d3d11.dll zaten Present fonksiyonu barındırır ✅
//     - Aynı modül içi yönlendirme → tamamen doğal ✅
//
//  Ek: DeviceContext VMT hook ile CopyResource yakalanır
//  (screenshot koruması için ScreenGuard.hpp'ye bildirir)
// ============================================================

namespace Render {

    using fn_Present = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
    using fn_ResizeBuffers = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    using DrawCallback = void(*)(ID3D11Device*, ID3D11DeviceContext*, IDXGISwapChain*);

    // State
    inline fn_Present        g_origPresent = nullptr;
    inline fn_ResizeBuffers  g_origResize  = nullptr;
    inline DrawCallback      g_drawCb      = nullptr;

    inline ID3D11Device*          g_device    = nullptr;
    inline ID3D11DeviceContext*   g_context   = nullptr;
    inline ID3D11RenderTargetView* g_rtv      = nullptr;
    inline IDXGISwapChain*        g_swapChain = nullptr;

    inline bool g_initialized = false;
    inline uintptr_t* g_vtable = nullptr;

    void OnFrame(ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* sc);
    void CleanupRenderStates();

    // --------------------------------------------------------
    //  Present Hook (Detour)
    // --------------------------------------------------------

    inline HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (!g_initialized) {
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) {
                g_device->GetImmediateContext(&g_context);
                g_swapChain = pSwapChain;

                // ScreenGuard: DeviceContext VMT hook kur (screenshot tespiti)
                // ScreenGuard.hpp include edildiğinde otomatik çalışır
            }

            ID3D11Texture2D* backBuffer = nullptr;
            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
                g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
                backBuffer->Release();
            }

            g_initialized = true;
        }

        // Render target'ı ayarla ve çizim callback'ini çağır
        if (g_rtv && g_context && g_drawCb) {
            g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
            g_drawCb(g_device, g_context, pSwapChain);
        }

        return g_origPresent(pSwapChain, SyncInterval, Flags);
    }

    // --------------------------------------------------------
    //  Resize Hook
    // --------------------------------------------------------

    inline HRESULT WINAPI HookedResize(IDXGISwapChain* pSwapChain, UINT BufferCount, 
                                        UINT Width, UINT Height, DXGI_FORMAT Format, UINT Flags) {
        if (g_rtv) {
            g_rtv->Release();
            g_rtv = nullptr;
        }
        g_initialized = false;

        return g_origResize(pSwapChain, BufferCount, Width, Height, Format, Flags);
    }

    // --------------------------------------------------------
    //  Dummy SwapChain ile vtable adresi bulma
    // --------------------------------------------------------

    inline bool GetSwapChainVtable(uintptr_t* vtableOut) {
        // Random class name (string tespitinden kaçın)
        char clsName[8];
        DWORD seed = GetTickCount();
        for (int i = 0; i < 7; i++) {
            seed = seed * 1103515245 + 12345;
            clsName[i] = 'a' + ((seed >> 16) % 26);
        }
        clsName[7] = '\0';

        WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 
                           0, 0, GetModuleHandle(nullptr), nullptr, nullptr, 
                           nullptr, nullptr, clsName, nullptr };
        RegisterClassExA(&wc);
        HWND hWnd = CreateWindowA(clsName, "", WS_OVERLAPPEDWINDOW, 0, 0, 2, 2, 
                                  nullptr, nullptr, wc.hInstance, nullptr);

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = 2;
        sd.BufferDesc.Height = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate = { 60, 1 };
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL fl;
        IDXGISwapChain* dsc = nullptr;
        ID3D11Device* dd = nullptr;
        ID3D11DeviceContext* dc = nullptr;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &sd, &dsc, &dd, &fl, &dc
        );

        if (FAILED(hr)) {
            DestroyWindow(hWnd);
            UnregisterClassA(clsName, wc.hInstance);
            return false;
        }

        uintptr_t* pVtable = *(uintptr_t**)dsc;
        memcpy(vtableOut, pVtable, 32 * sizeof(uintptr_t));

        dsc->Release();
        dd->Release();
        dc->Release();
        DestroyWindow(hWnd);
        UnregisterClassA(clsName, wc.hInstance);

        return true;
    }

    // Present = vtable index 8, ResizeBuffers = vtable index 13
    constexpr int PRESENT_INDEX = 8;
    constexpr int RESIZE_INDEX  = 13;

    // --------------------------------------------------------
    //  VMT Hook - d3d11.dll Code Cave Trampoline
    //
    //  VMT[Present] → d3d11.dll code cave → JMP HookedPresent
    //
    //  Pointer validasyonu:
    //  Eski: VMT[8] → version.dll adresi → ❌ Anomali!
    //  Yeni: VMT[8] → d3d11.dll .text adresi → ✅ Doğal!
    //
    //  Anti-cheat d3d11.dll'in vtable pointer'larının
    //  d3d11.dll adres aralığında olmasını bekler.
    //  Code cave tam da d3d11.dll .text bölümünde olduğu
    //  için bu beklentiyi karşılar.
    // --------------------------------------------------------

    inline bool HookPresent(IDXGISwapChain* realSwapChain, DrawCallback drawFunc) {
        g_drawCb = drawFunc;

        g_vtable = *(uintptr_t**)realSwapChain;

        g_origPresent = (fn_Present)g_vtable[PRESENT_INDEX];
        g_origResize  = (fn_ResizeBuffers)g_vtable[RESIZE_INDEX];

        DWORD oldProtect;
        VirtualProtect(&g_vtable[PRESENT_INDEX], sizeof(uintptr_t) * 6, 
                       PAGE_READWRITE, &oldProtect);

        // d3d11.dll code cave'lerini kullan
        if (Stomp::g_cave.valid) {
            // Trampoline'ı d3d11.dll'in .text padding'ine yaz
            void* presentTramp = Stomp::CreateTrampoline((void*)&HookedPresent);
            void* resizeTramp  = Stomp::CreateTrampoline((void*)&HookedResize);

            if (presentTramp) {
                // Doğrulama: trampoline gerçekten d3d11.dll'e mi ait?
                if (Stomp::VerifyTrampoline(presentTramp)) {
                    g_vtable[PRESENT_INDEX] = (uintptr_t)presentTramp;
                }
            }
            if (resizeTramp) {
                if (Stomp::VerifyTrampoline(resizeTramp)) {
                    g_vtable[RESIZE_INDEX] = (uintptr_t)resizeTramp;
                }
            }
        } else {
            // Fallback: doğrudan hook (tespit riski yüksek)
            g_vtable[PRESENT_INDEX] = (uintptr_t)&HookedPresent;
            g_vtable[RESIZE_INDEX]  = (uintptr_t)&HookedResize;
        }

        VirtualProtect(&g_vtable[PRESENT_INDEX], sizeof(uintptr_t) * 6, 
                       oldProtect, &oldProtect);

        return true;
    }

    // --------------------------------------------------------
    //  Cleanup
    // --------------------------------------------------------

    inline void Shutdown() {
        if (g_vtable && g_origPresent) {
            DWORD oldProtect;
            VirtualProtect(&g_vtable[PRESENT_INDEX], sizeof(uintptr_t) * 6, 
                           PAGE_READWRITE, &oldProtect);
            g_vtable[PRESENT_INDEX] = (uintptr_t)g_origPresent;
            g_vtable[RESIZE_INDEX]  = (uintptr_t)g_origResize;
            VirtualProtect(&g_vtable[PRESENT_INDEX], sizeof(uintptr_t) * 6, 
                           oldProtect, &oldProtect);
        }

        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
        if (g_context) { g_context->Release(); g_context = nullptr; }
        if (g_device) { g_device->Release(); g_device = nullptr; }

        g_initialized = false;
    }
}
