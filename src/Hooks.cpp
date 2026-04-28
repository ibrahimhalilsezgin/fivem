#include "Hooks.h"
#include <Windows.h>
#include <vector>
#include <Psapi.h>
#include "LuaEngine.h"
#include "Core/Memory.hpp"
#include "Core/Crypto.hpp"
#include "Core/Stealth.hpp"
#include "Core/ModuleStomp.hpp"
#include "Core/StackSpoof.hpp"
#include "Core/ScreenGuard.hpp"
#include "Game/Offsets.hpp"
#include "Render/D3DRenderer.hpp"

extern LE g_le;

// MinHook stub definitions (replace with real MinHook in production)
#define MH_OK 0
#define MH_ALL_HOOKS nullptr
typedef int MH_STATUS;
MH_STATUS MH_Initialize() { return MH_OK; }
MH_STATUS MH_Uninitialize() { return MH_OK; }
MH_STATUS MH_CreateHook(void* target, void* detour, void** original) { *original = target; return MH_OK; }
MH_STATUS MH_EnableHook(void* target) { return MH_OK; }
MH_STATUS MH_DisableHook(void* target) { return MH_OK; }

// ============================================================
//  Pattern Scanner
// ============================================================

static std::vector<int> _PtoB(const char* p) {
    std::vector<int> b;
    char* s = const_cast<char*>(p);
    char* e = s + strlen(p);
    for (char* c = s; c < e; ++c) {
        if (*c == '?') {
            ++c;
            if (*c == '?') ++c;
            b.push_back(-1);
        } else if (*c != ' ') {
            b.push_back(strtol(c, &c, 16));
        }
    }
    return b;
}

static void* _Scan(const char* mod, const char* sig) {
    HMODULE hm = GetModuleHandleA(mod);
    if (!hm) return nullptr;

    MODULEINFO mi;
    GetModuleInformation(GetCurrentProcess(), hm, &mi, sizeof(MODULEINFO));

    uint8_t* base = (uint8_t*)mi.lpBaseOfDll;
    size_t sz = mi.SizeOfImage;
    auto pat = _PtoB(sig);
    size_t psz = pat.size();
    auto d = pat.data();

    for (size_t i = 0; i < sz - psz; ++i) {
        bool ok = true;
        for (size_t j = 0; j < psz; ++j) {
            if (d[j] != -1 && d[j] != base[i + j]) {
                ok = false;
                break;
            }
        }
        if (ok) return (void*)&base[i];
    }
    return nullptr;
}

// ============================================================
//  Lua Hook - Trampoline stomped modülde
// ============================================================

struct lua_State;
typedef int(__cdecl* _lb_t)(lua_State*, const char*, size_t, const char*);
_lb_t _pOrig = nullptr;

int __cdecl _Detour(lua_State* L, const char* buf, size_t sz, const char* name) {
    g_le.Attach(L);
    if (_pOrig) return _pOrig(L, buf, sz, name);
    return 0;
}

// Encrypted signature
static constexpr uint8_t _SIG_KEY = 0x5A;
static const char _ENC_PAT[] = {
    '4'^_SIG_KEY, '8'^_SIG_KEY, ' '^_SIG_KEY, '8'^_SIG_KEY, '9'^_SIG_KEY, ' '^_SIG_KEY,
    '5'^_SIG_KEY, 'C'^_SIG_KEY, ' '^_SIG_KEY, '2'^_SIG_KEY, '4'^_SIG_KEY, ' '^_SIG_KEY,
    '?'^_SIG_KEY, ' '^_SIG_KEY, '4'^_SIG_KEY, '8'^_SIG_KEY, ' '^_SIG_KEY, '8'^_SIG_KEY,
    '9'^_SIG_KEY, ' '^_SIG_KEY, '7'^_SIG_KEY, '4'^_SIG_KEY, ' '^_SIG_KEY, '2'^_SIG_KEY,
    '4'^_SIG_KEY, ' '^_SIG_KEY, '?'^_SIG_KEY, ' '^_SIG_KEY, '5'^_SIG_KEY, '7'^_SIG_KEY,
    ' '^_SIG_KEY, '4'^_SIG_KEY, '8'^_SIG_KEY, ' '^_SIG_KEY, '8'^_SIG_KEY, '3'^_SIG_KEY,
    ' '^_SIG_KEY, 'E'^_SIG_KEY, 'C'^_SIG_KEY, ' '^_SIG_KEY, '3'^_SIG_KEY, '0'^_SIG_KEY,
    ' '^_SIG_KEY, '4'^_SIG_KEY, 'C'^_SIG_KEY, ' '^_SIG_KEY, '8'^_SIG_KEY, 'B'^_SIG_KEY,
    ' '^_SIG_KEY, 'C'^_SIG_KEY, '2'^_SIG_KEY, ' '^_SIG_KEY, '4'^_SIG_KEY, '9'^_SIG_KEY,
    ' '^_SIG_KEY, '8'^_SIG_KEY, 'B'^_SIG_KEY, ' '^_SIG_KEY, 'D'^_SIG_KEY, '8'^_SIG_KEY,
    ' '^_SIG_KEY, '4'^_SIG_KEY, '8'^_SIG_KEY, ' '^_SIG_KEY, '8'^_SIG_KEY, 'B'^_SIG_KEY,
    ' '^_SIG_KEY, 'F'^_SIG_KEY, 'A'^_SIG_KEY, ' '^_SIG_KEY, '4'^_SIG_KEY, '8'^_SIG_KEY,
    ' '^_SIG_KEY, '8'^_SIG_KEY, 'B'^_SIG_KEY, ' '^_SIG_KEY, 'F'^_SIG_KEY, '1'^_SIG_KEY,
    0 ^ _SIG_KEY
};

static std::string _DecryptPat() {
    std::string r;
    r.reserve(sizeof(_ENC_PAT));
    for (size_t i = 0; i < sizeof(_ENC_PAT); i++) {
        char c = _ENC_PAT[i] ^ _SIG_KEY;
        if (c == 0) break;
        r += c;
    }
    return r;
}

// ============================================================
//  Ana Setup - Tüm anti-detection katmanları burada başlatılır
// ============================================================

void Hk::Setup() {
    // [KATMAN 1] Anti-debug kontrolü
    if (Stealth::IsBeingDebugged()) return;

    // [KATMAN 2] Module Stomping - kayıtsız bellek sorununu çözer
    // Meşru bir DLL'in .text bölümünü hollow eder
    // Tüm hook trampoline'ları bu modülün adres alanında çalışır
    // → FiveM'in "unbacked executable memory" taramasını aşar
    Stomp::Initialize();

    // [KATMAN 3] Call Stack Spoofing hazırlığı
    // ntdll.dll'de ROP gadget'lar bulur
    // Timer/ThreadPool tabanlı yürütme için altyapı kurar
    // → FiveM'in "call stack walking" analizini aşar
    StackSpoof::Initialize();

    // [KATMAN 4] MinHook başlat
    if (MH_Initialize() != MH_OK) return;

    // Hedef modül ismini runtime'da çöz
    XC("citizen-scripting-lua.dll", modName);

    bool isFM = false;
    for (int i = 0; i < 10; ++i) {
        if (GetModuleHandleA(modName)) {
            isFM = true;
            break;
        }
        Sleep(500);
    }

    if (isFM) {
        std::string pat = _DecryptPat();
        void* addr = _Scan(modName, pat.c_str());
        SecureZeroMemory(&pat[0], pat.size());

        if (addr) {
            // [KATMAN 5] Hook trampoline'ı stomped modüle yerleştir
            // Detour fonksiyonuna JMP stub stomped modülde olduğu için
            // call stack walking'de meşru modül adresi görünür
            if (Stomp::g_cave.valid) {
                // Detour'un trampoline'ını stomped modüle koy
                void* stompedDetour = Stomp::CreateTrampoline((void*)&_Detour);
                if (stompedDetour) {
                    MH_CreateHook(addr, stompedDetour, reinterpret_cast<void**>(&_pOrig));
                } else {
                    MH_CreateHook(addr, &_Detour, reinterpret_cast<void**>(&_pOrig));
                }
            } else {
                MH_CreateHook(addr, &_Detour, reinterpret_cast<void**>(&_pOrig));
            }
        }
    }

    // [KATMAN 6] Offset resolver
    uintptr_t base = Core::Mem::Base();
    Cfg::Resolve(base);

    // [KATMAN 7] D3D11 SwapChain VMT Hook
    // Harici overlay penceresi OLUŞTURMAZ
    // Doğrudan oyunun DirectX pipeline'ına hook atar
    // VMT hook = data patch, code integrity taramalarından kaçar
    // → FiveM'in overlay window ve render hook tespitini aşar
    //
    // NOT: SwapChain'i bulmak için pattern scan gerekir
    // Gerçek implementasyonda bu pattern kullanılır:
    // "48 8B 05 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 48 85 C9"
    //
    // Şimdilik D3D draw callback'i Overlay.cpp'de ayarlanır

    MH_EnableHook(MH_ALL_HOOKS);
}

void Hk::Teardown() {
    // D3D cleanup
    Render::CleanupRenderStates();
    Render::Shutdown();

    // Module stomp cleanup
    Stomp::Shutdown();

    // Hook cleanup
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
