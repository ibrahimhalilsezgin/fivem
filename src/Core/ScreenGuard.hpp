#pragma once
#include <windows.h>
#include <cstdint>
#include <d3d11.h>
// ============================================================
//  Screen Guard V3 - D3D-Aware Screenshot Protection
//
//  ❌ WDA_EXCLUDEFROMCAPTURE KULLANILMAZ!
//     → Bu bayrak kendi başına tespit vektörüdür.
//     → Hiçbir yasal uygulama (Discord, NVIDIA, OBS) kullanmaz.
//     → Anti-cheat bu bayrağı gören pencereyi anında mimler.
//
//  ✅ Strateji: D3D DeviceContext'in CopyResource / 
//     CopySubresourceRegion fonksiyonlarını VMT hook ile yakala.
//     FiveM screenshot almak için backbuffer'ı CPU'ya kopyalar.
//     Bu kopyalama anında çizimi DURAKLAT.
//
//  Akış:
//  1. Present hook → ESP çiz → ekranda görünür
//  2. FiveM screenshot ister → CopyResource çağrılır
//  3. Bizim CopyResource hook'umuz → g_isCapturing = true
//  4. Bir sonraki Present frame'inde → CanDraw() = false → çizim yok
//  5. CopyResource orijinali çalışır → temiz backbuffer kopyalanır
//  6. g_isCapturing = false → çizim tekrar başlar
//
//  Böylece screenshot'ta sadece normal oyun ekranı görünür.
// ============================================================

namespace ScreenGuard {

    // --------------------------------------------------------
    //  State
    // --------------------------------------------------------

    inline volatile bool g_isCapturing = false;

    // Son N frame'de CopyResource çağrıldı mı?
    // Screenshot genelde 1-2 frame sürer, güvenlik için 3 frame bekle
    inline volatile int g_captureCountdown = 0;
    constexpr int CAPTURE_COOLDOWN_FRAMES = 3;

    // --------------------------------------------------------
    //  ID3D11DeviceContext VMT Hook
    //  CopyResource = vtable index 47
    //  CopySubresourceRegion = vtable index 46
    //  Map = vtable index 14
    //
    //  FiveM backbuffer'ı okumak için bu fonksiyonları kullanır.
    //  Bunları hook'layarak çizim duraklatma sinyali veririz.
    // --------------------------------------------------------

    // Function pointer types
    using fn_CopyResource = void(STDMETHODCALLTYPE*)(
        ID3D11DeviceContext*, ID3D11Resource*, ID3D11Resource*);
    using fn_CopySubresourceRegion = void(STDMETHODCALLTYPE*)(
        ID3D11DeviceContext*, ID3D11Resource*, UINT, UINT, UINT, UINT,
        ID3D11Resource*, UINT, const D3D11_BOX*);

    // vtable indices
    constexpr int CTX_COPYRESOURCE_INDEX = 47;
    constexpr int CTX_COPYSUBRESOURCE_INDEX = 46;

    inline fn_CopyResource g_origCopyResource = nullptr;
    inline fn_CopySubresourceRegion g_origCopySubresource = nullptr;

    inline uintptr_t* g_ctxVtable = nullptr;

    // --------------------------------------------------------
    //  Hooked CopyResource
    //  FiveM backbuffer'ı staging texture'a kopyaladığında tetiklenir
    // --------------------------------------------------------

    inline void STDMETHODCALLTYPE HookedCopyResource(
        ID3D11DeviceContext* ctx, ID3D11Resource* dst, ID3D11Resource* src) 
    {
        // Kaynak texture'ın backbuffer olup olmadığını kontrol et
        // Backbuffer genelde D3D11_USAGE_DEFAULT texture olur
        D3D11_RESOURCE_DIMENSION dstDim, srcDim;
        dst->GetType(&dstDim);
        src->GetType(&srcDim);

        // Eğer 2D texture'dan 2D texture'a kopyalanıyorsa
        // bu büyük ihtimalle bir screenshot operasyonu
        if (srcDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && 
            dstDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
            
            // Staging texture'a kopyalama = screenshot alma
            ID3D11Texture2D* dstTex = nullptr;
            if (SUCCEEDED(dst->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&dstTex))) {
                D3D11_TEXTURE2D_DESC desc;
                dstTex->GetDesc(&desc);
                dstTex->Release();

                // CPU erişimli texture = screenshot hedefi
                if (desc.Usage == D3D11_USAGE_STAGING || 
                    (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ)) {
                    // Screenshot tespit edildi! Countdown başlat
                    g_captureCountdown = CAPTURE_COOLDOWN_FRAMES;
                    g_isCapturing = true;
                }
            }
        }

        // Orijinal fonksiyonu çağır
        g_origCopyResource(ctx, dst, src);
    }

    // --------------------------------------------------------
    //  Hooked CopySubresourceRegion
    // --------------------------------------------------------

    inline void STDMETHODCALLTYPE HookedCopySubresource(
        ID3D11DeviceContext* ctx, ID3D11Resource* dst, UINT dstSub,
        UINT dstX, UINT dstY, UINT dstZ,
        ID3D11Resource* src, UINT srcSub, const D3D11_BOX* srcBox)
    {
        D3D11_RESOURCE_DIMENSION srcDim;
        src->GetType(&srcDim);

        if (srcDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
            ID3D11Texture2D* dstTex = nullptr;
            if (SUCCEEDED(dst->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&dstTex))) {
                D3D11_TEXTURE2D_DESC desc;
                dstTex->GetDesc(&desc);
                dstTex->Release();

                if (desc.Usage == D3D11_USAGE_STAGING || 
                    (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ)) {
                    g_captureCountdown = CAPTURE_COOLDOWN_FRAMES;
                    g_isCapturing = true;
                }
            }
        }

        g_origCopySubresource(ctx, dst, dstSub, dstX, dstY, dstZ, src, srcSub, srcBox);
    }

    // --------------------------------------------------------
    //  DeviceContext VMT Hook kurulumu
    //  Present hook'undaki device context üzerinden çağrılır
    // --------------------------------------------------------

    inline bool HookDeviceContext(ID3D11DeviceContext* ctx) {
        if (!ctx || g_ctxVtable) return false; // Zaten hook'lanmış

        g_ctxVtable = *(uintptr_t**)ctx;

        // Orijinal fonksiyonları kaydet
        g_origCopyResource = (fn_CopyResource)g_ctxVtable[CTX_COPYRESOURCE_INDEX];
        g_origCopySubresource = (fn_CopySubresourceRegion)g_ctxVtable[CTX_COPYSUBRESOURCE_INDEX];

        // VMT entry'lerini değiştir (data patch - kod değiştirmez)
        DWORD oldProtect;
        VirtualProtect(&g_ctxVtable[CTX_COPYSUBRESOURCE_INDEX], 
                       sizeof(uintptr_t) * 2, PAGE_READWRITE, &oldProtect);

        g_ctxVtable[CTX_COPYSUBRESOURCE_INDEX] = (uintptr_t)&HookedCopySubresource;
        g_ctxVtable[CTX_COPYRESOURCE_INDEX] = (uintptr_t)&HookedCopyResource;

        VirtualProtect(&g_ctxVtable[CTX_COPYSUBRESOURCE_INDEX], 
                       sizeof(uintptr_t) * 2, oldProtect, &oldProtect);

        return true;
    }

    // --------------------------------------------------------
    //  Frame tick - her Present çağrısında çağır
    //  Capture countdown'ı azaltır
    // --------------------------------------------------------

    __forceinline void OnPresent() {
        if (g_captureCountdown > 0) {
            g_captureCountdown--;
            if (g_captureCountdown == 0) {
                g_isCapturing = false;
            }
        }
    }

    // --------------------------------------------------------
    //  Çizim yapılıp yapılmayacağını kontrol et
    //  Screenshot sırası + cooldown boyunca false döner
    // --------------------------------------------------------

    __forceinline bool CanDraw() {
        return !g_isCapturing;
    }

    // --------------------------------------------------------
    //  Initialize (artık WDA kullanmıyor)
    // --------------------------------------------------------

    inline bool Initialize() {
        // DeviceContext hook'u Present hook'undan sonra kurulur
        // Burada sadece state sıfırlanır
        g_isCapturing = false;
        g_captureCountdown = 0;
        return true;
    }

    // --------------------------------------------------------
    //  Cleanup
    // --------------------------------------------------------

    inline void Shutdown() {
        if (g_ctxVtable && g_origCopyResource) {
            DWORD oldProtect;
            VirtualProtect(&g_ctxVtable[CTX_COPYSUBRESOURCE_INDEX], 
                           sizeof(uintptr_t) * 2, PAGE_READWRITE, &oldProtect);
            
            g_ctxVtable[CTX_COPYSUBRESOURCE_INDEX] = (uintptr_t)g_origCopySubresource;
            g_ctxVtable[CTX_COPYRESOURCE_INDEX] = (uintptr_t)g_origCopyResource;
            
            VirtualProtect(&g_ctxVtable[CTX_COPYSUBRESOURCE_INDEX], 
                           sizeof(uintptr_t) * 2, oldProtect, &oldProtect);
            g_ctxVtable = nullptr;
        }
    }
}
