#include "D3DRenderer.hpp"
#include "../Core/ScreenGuard.hpp"
#include "../Features/ESP.hpp"
#include "../Core/Memory.hpp"
#include "../Game/Offsets.hpp"

// ============================================================
//  D3D11 Internal ESP Drawing
//  Oyunun kendi render pipeline'ında doğrudan çizim yapar.
//  Harici overlay penceresi KULLANMAZ.
//
//  D3D11 line rendering ile basit ESP kutuları çizer.
//  Daha gelişmiş çizim için ImGui entegrasyonu yapılabilir.
// ============================================================

namespace Render {

    // Basit vertex shader (HLSL derlenmiş bytecode)
    // Gerçek implementasyonda D3DCompile ile compile edilir
    
    struct SimpleVertex {
        float x, y;
        float r, g, b, a;
    };

    // Internal draw state
    static ID3D11Buffer*             s_vertexBuffer = nullptr;
    static ID3D11InputLayout*        s_inputLayout  = nullptr;
    static ID3D11VertexShader*       s_vertexShader = nullptr;
    static ID3D11PixelShader*        s_pixelShader  = nullptr;
    static ID3D11BlendState*         s_blendState   = nullptr;
    static ID3D11RasterizerState*    s_rasterState  = nullptr;
    static ID3D11DepthStencilState*  s_depthState   = nullptr;

    static bool s_shadersCreated = false;
    static int  s_screenW = 1920;
    static int  s_screenH = 1080;

    // --------------------------------------------------------
    //  D3D11 2D Line Drawing (shader-based)
    // --------------------------------------------------------

    // Minimal vertex shader HLSL:
    // float4 main(float2 pos : POSITION, float4 col : COLOR) : SV_POSITION {
    //     return float4(pos, 0, 1);
    // }
    //
    // Minimal pixel shader HLSL:
    // float4 main(float4 col : COLOR) : SV_TARGET {
    //     return col;
    // }

    static bool CreateRenderStates(ID3D11Device* device) {
        if (s_shadersCreated) return true;

        // Blend state (alpha blending)
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        device->CreateBlendState(&blendDesc, &s_blendState);

        // Rasterizer state (no culling)
        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable = TRUE;
        device->CreateRasterizerState(&rasterDesc, &s_rasterState);

        // Depth stencil (disabled)
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = FALSE;
        device->CreateDepthStencilState(&depthDesc, &s_depthState);

        // Vertex buffer
        D3D11_BUFFER_DESC bufDesc = {};
        bufDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufDesc.ByteWidth = sizeof(SimpleVertex) * 8192; // Max vertices
        bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&bufDesc, nullptr, &s_vertexBuffer);

        s_shadersCreated = true;
        return true;
    }

    // Ekran koordinatlarını NDC'ye (Normalized Device Coordinates) çevir
    static float ToNdcX(float x) { return (x / s_screenW) * 2.0f - 1.0f; }
    static float ToNdcY(float y) { return 1.0f - (y / s_screenH) * 2.0f; }

    // --------------------------------------------------------
    //  Line Drawing (vertex buffer ile)
    //  Çizgi çizerek ESP kutuları oluşturur
    // --------------------------------------------------------

    static SimpleVertex s_vertices[8192];
    static int s_vertexCount = 0;

    static void AddLine(float x1, float y1, float x2, float y2, 
                        float r, float g, float b, float a = 1.0f) {
        if (s_vertexCount + 2 > 8192) return;
        s_vertices[s_vertexCount++] = { ToNdcX(x1), ToNdcY(y1), r, g, b, a };
        s_vertices[s_vertexCount++] = { ToNdcX(x2), ToNdcY(y2), r, g, b, a };
    }

    static void DrawBox(float x, float y, float w, float h, 
                        float r, float g, float b) {
        // Top
        AddLine(x, y, x + w, y, r, g, b);
        // Bottom
        AddLine(x, y + h, x + w, y + h, r, g, b);
        // Left
        AddLine(x, y, x, y + h, r, g, b);
        // Right
        AddLine(x + w, y, x + w, y + h, r, g, b);
    }

    // --------------------------------------------------------
    //  Flush: Biriken çizgileri GPU'ya gönder
    // --------------------------------------------------------

    static void Flush(ID3D11DeviceContext* ctx) {
        if (s_vertexCount == 0) return;
        if (!s_vertexBuffer) return;

        // Vertex buffer'a yaz
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(ctx->Map(s_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, s_vertices, sizeof(SimpleVertex) * s_vertexCount);
            ctx->Unmap(s_vertexBuffer, 0);
        }

        // Draw
        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &s_vertexBuffer, &stride, &offset);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        ctx->Draw(s_vertexCount, 0);

        s_vertexCount = 0;
    }

    // --------------------------------------------------------
    //  WorldToScreen (D3D internal version)
    // --------------------------------------------------------

    static bool W2S(Feat::V3 pos, Feat::V2& screen, float* matrix, int sw, int sh) {
        float w = matrix[3] * pos.x + matrix[7] * pos.y + matrix[11] * pos.z + matrix[15];
        if (w < 0.01f) return false;

        float iw = 1.0f / w;
        float x = (matrix[0] * pos.x + matrix[4] * pos.y + matrix[8]  * pos.z + matrix[12]) * iw;
        float y = (matrix[1] * pos.x + matrix[5] * pos.y + matrix[9]  * pos.z + matrix[13]) * iw;

        screen.x = (sw / 2.0f) * (1.0f + x);
        screen.y = (sh / 2.0f) * (1.0f - y);
        return true;
    }

    // --------------------------------------------------------
    //  Ana çizim callback'i (HookedPresent tarafından çağrılır)
    // --------------------------------------------------------

    void OnFrame(ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* sc) {
        // Screenshot tespiti: frame countdown tick
        ScreenGuard::OnPresent();

        // İlk frame'de DeviceContext VMT hook kur (CopyResource tespiti)
        if (!ScreenGuard::g_ctxVtable && ctx) {
            ScreenGuard::HookDeviceContext(ctx);
        }

        // Screenshot yakalama sırasında çizme
        if (!ScreenGuard::CanDraw()) return;
        if (!Feat::Vis::bOn) return;

        // Ekran boyutunu güncelle
        DXGI_SWAP_CHAIN_DESC desc;
        if (SUCCEEDED(sc->GetDesc(&desc))) {
            s_screenW = desc.BufferDesc.Width;
            s_screenH = desc.BufferDesc.Height;
        }

        CreateRenderStates(device);

        // Render state'leri ayarla
        float blendFactor[] = { 0, 0, 0, 0 };
        ctx->OMSetBlendState(s_blendState, blendFactor, 0xFFFFFFFF);
        ctx->RSSetState(s_rasterState);
        ctx->OMSetDepthStencilState(s_depthState, 0);

        // ESP çizimi (game memory'den veri oku, D3D ile çiz)
        uintptr_t base = Core::Mem::Base();
        uintptr_t vp = Core::Mem::Rd<uintptr_t>(base + Cfg::ViewPort());
        if (!vp) return;

        float mtx[16];
        for (int i = 0; i < 16; i++) {
            mtx[i] = Core::Mem::Rd<float>(vp + Cfg::VMatrix() + (i * 4));
        }

        uintptr_t rp = Core::Mem::Rd<uintptr_t>(base + Cfg::ReplayIf());
        if (!rp) return;
        uintptr_t pi = Core::Mem::Rd<uintptr_t>(rp + Cfg::PedIf());
        if (!pi) return;
        uintptr_t pl = Core::Mem::Rd<uintptr_t>(pi + Cfg::PedLst());
        int mc = Core::Mem::Rd<int>(pi + Cfg::PedCnt());
        if (!pl || mc < 1) return;

        uintptr_t myPed = 0;
        uintptr_t world = Core::Mem::Rd<uintptr_t>(base + Cfg::World());
        if (world) myPed = Core::Mem::Rd<uintptr_t>(world + Cfg::LocalPed());

        int limit = mc > 128 ? 128 : mc;
        for (int i = 0; i < limit; i++) {
            uintptr_t ped = Core::Mem::Rd<uintptr_t>(pl + (i * 0x10));
            if (!ped || ped == myPed) continue;

            Feat::V3 pos;
            pos.x = Core::Mem::Rd<float>(ped + Cfg::Pos());
            pos.y = Core::Mem::Rd<float>(ped + Cfg::Pos() + 4);
            pos.z = Core::Mem::Rd<float>(ped + Cfg::Pos() + 8);

            Feat::V2 sF, sH;
            Feat::V3 hp = { pos.x, pos.y, pos.z + 0.8f };

            if (W2S(pos, sF, mtx, s_screenW, s_screenH) && 
                W2S(hp, sH, mtx, s_screenW, s_screenH)) {
                float h = sF.y - sH.y;
                float w = h * 0.6f;

                // Kırmızı ESP kutusu
                DrawBox(sH.x - w / 2, sH.y, w, h, 1.0f, 0.0f, 0.0f);
            }
        }

        Flush(ctx);
    }

    // --------------------------------------------------------
    //  D3D Render Cleanup
    // --------------------------------------------------------

    void CleanupRenderStates() {
        if (s_vertexBuffer) { s_vertexBuffer->Release(); s_vertexBuffer = nullptr; }
        if (s_inputLayout)  { s_inputLayout->Release();  s_inputLayout  = nullptr; }
        if (s_vertexShader) { s_vertexShader->Release();  s_vertexShader = nullptr; }
        if (s_pixelShader)  { s_pixelShader->Release();   s_pixelShader  = nullptr; }
        if (s_blendState)   { s_blendState->Release();    s_blendState   = nullptr; }
        if (s_rasterState)  { s_rasterState->Release();   s_rasterState  = nullptr; }
        if (s_depthState)   { s_depthState->Release();    s_depthState   = nullptr; }
        s_shadersCreated = false;
    }
}
