#include "Hooks.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <Psapi.h>
#include "LuaEngine.h"
#include "Offsets.h"
#include "Memory.h"

extern LuaEngine g_LuaEngine; // Global lua motorumuza erişim için

// Gerçek projede MinHook kütüphanesini projeye dahil etmelisin.
// #include "MinHook.h" 

// --- MinHook sembolik tanımları (Derlenebilmesi için sahte tanımlar, projeye MinHook ekleyince bunları silin) ---
#define MH_OK 0
#define MH_ALL_HOOKS nullptr
typedef int MH_STATUS;
MH_STATUS MH_Initialize() { return MH_OK; }
MH_STATUS MH_Uninitialize() { return MH_OK; }
MH_STATUS MH_CreateHook(void* target, void* detour, void** original) { *original = target; return MH_OK; } // Sahte atama
MH_STATUS MH_EnableHook(void* target) { return MH_OK; }
MH_STATUS MH_DisableHook(void* target) { return MH_OK; }
// ---------------------------------------------------------------------------------------------------------

// --- PATTERN SCANNER (Hafıza Tarayıcı) ---
// Pattern'i byte array'e çeviren yardımcı fonksiyon
std::vector<int> PatternToByte(const char* pattern) {
    std::vector<int> bytes;
    char* start = const_cast<char*>(pattern);
    char* end = start + strlen(pattern);

    for (char* current = start; current < end; ++current) {
        if (*current == '?') {
            ++current;
            if (*current == '?') ++current; // Eger ?? yazildiysa
            bytes.push_back(-1); // Wildcard
        } else if (*current != ' ') { // Boşlukları atla
            bytes.push_back(strtol(current, &current, 16));
        }
    }
    return bytes;
}

// Modül (DLL) içindeki hafızayı tarayan asıl fonksiyon
void* FindPattern(const char* moduleName, const char* signature) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) {
        std::cerr << "[-] " << moduleName << " modulu bulunamadi. (Oyun dogru mu veya DLL inject oldu mu?)" << std::endl;
        return nullptr;
    }

    MODULEINFO moduleInfo;
    GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO));

    uint8_t* moduleBase = (uint8_t*)moduleInfo.lpBaseOfDll;
    size_t sizeOfImage = moduleInfo.SizeOfImage;
    std::vector<int> patternBytes = PatternToByte(signature);

    size_t patternSize = patternBytes.size();
    auto data = patternBytes.data();

    // Hafızada baştan sona linear arama
    for (size_t i = 0; i < sizeOfImage - patternSize; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternSize; ++j) {
            // Eğer wildcard ( -1 ) ise veya o anki memory bytı tutuyorsa devam et
            if (data[j] != -1 && data[j] != moduleBase[i + j]) {
                found = false;
                break;
            }
        }
        if (found) {
            // Memory adresini döndür
            return (void*)&moduleBase[i];
        }
    }
    return nullptr;
}
// -----------------------------------------

// 1. Hooklanacak Lua Fonksiyonunun Prototipi
// Örnek: luaL_loadbuffer(lua_State *L, const char *buff, size_t sz, const char *name)
struct lua_State;
typedef int(__cdecl* luaL_loadbuffer_t)(lua_State*, const char*, size_t, const char*);
luaL_loadbuffer_t pOriginalLoadBuffer = nullptr;

// 2. Detour (Hook) Fonksiyonu
int __cdecl Hooked_luaL_loadbuffer(lua_State* L, const char* buff, size_t sz, const char* name) {
    // 1. ADIM: Gelen lua_State pointer'ını çalıp kendi motorumuza kaydediyoruz.
    // Artık bu 'L' üzerinden kendi kodlarımızı FiveM içinden çalıştırabiliriz!
    g_LuaEngine.SetActiveState(L);

    std::cout << "[Hook] luaL_loadbuffer yakalandi! (Script Name: " << (name ? name : "Unknown") << ")" << std::endl;

    // isterseniz gelen orjinal buff/script içeriğini burada engelleyebilir veya değiştirebilirsiniz
    // return LUA_OK;

    // 2. ADIM: Orijinal akışı bozmamak için gerçek fonksiyona devam et
    if (pOriginalLoadBuffer) {
        return pOriginalLoadBuffer(L, buff, sz, name);
    }
    return 0;
}

void Hooks::Initialize() {
    std::cout << "[+] MinHook baslatiliyor..." << std::endl;
    
    if (MH_Initialize() != MH_OK) {
        std::cerr << "[-] MinHook baslatilamadi!" << std::endl;
        return;
    }

    // FiveM modülü var mı kontrol et (Zaman aşımıyla)
    bool isFiveM = false;
    std::cout << "[*] Modul kontrolu yapiliyor..." << std::endl;
    for (int i = 0; i < 10; ++i) { // 5 saniye bekle
        if (GetModuleHandleA("citizen-scripting-lua.dll")) {
            isFiveM = true;
            break;
        }
        Sleep(500);
    }

    if (isFiveM) {
        std::cout << "[+] FiveM modu tespit edildi! Lua hook'lari kuruluyor..." << std::endl;
        const char* pattern = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 4C 8B C2 49 8B D8 48 8B FA 48 8B F1";
        void* targetAddress = FindPattern("citizen-scripting-lua.dll", pattern); 
        
        if (targetAddress != nullptr) {
            std::cout << "[+] luaL_loadbuffer BAŞARIYLA BULUNDU! Adres: " << targetAddress << std::endl;
            if (MH_CreateHook(targetAddress, &Hooked_luaL_loadbuffer, reinterpret_cast<void**>(&pOriginalLoadBuffer)) == MH_OK) {
                std::cout << "[+] luaL_loadbuffer hook'u basariyla yaratildi." << std::endl;
            }
        } else {
            std::cerr << "[-] luaL_loadbuffer PATTERN BULUNAMADI! (FiveM guncellenmis olabilir)" << std::endl;
        }
    } else {
        std::cout << "[*] Normal GTA 5 modu tespit edildi (veya FiveM henuz hazir degil)." << std::endl;
        std::cout << "[*] Lua Executor devre disi, ancak Hafiza offsetleri aktif!" << std::endl;
    }

    // Offset ve Sürüm Kontrolü
    uintptr_t base = Memory::GetModuleBase(nullptr); // Mevcut exe'nin base'ini al (GTA5.exe veya FiveM)
    if (!Offsets::Initialize(base)) {
        std::cerr << "[-] Offsetler yuklenemedi! Bazi ozellikler calismayabilir." << std::endl;
    }

    // Oluşturulan tüm hookları aktifleştir
    if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK) {
        std::cout << "[+] Tum hook'lar aktiflestirildi." << std::endl;
    }
}

void Hooks::Shutdown() {
    std::cout << "[-] Hook'lar devre disi birakiliyor ve temizleniyor..." << std::endl;
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
