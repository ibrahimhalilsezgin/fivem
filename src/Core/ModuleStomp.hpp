#pragma once
#include <windows.h>
#include <cstdint>
#include <winternl.h>
#include "Crypto.hpp"

// ============================================================
//  Code Cave Engine V3
//
//  ❌ version.dll / mswsock.dll HOLLOWING KALDIRILDI!
//     → version.dll'den DirectX Present çağrısı yapılması
//       davranışsal analiz motorlarını tetikler.
//     → "Bu DLL'in görevi grafik çizdirmek değil" → Anomali.
//
//  ✅ Strateji: d3d11.dll'in KENDİ code cave'lerini kullan.
//     MSVC derleyicisi fonksiyonlar arası hizalama için 0xCC 
//     (INT3) padding byte'ları ekler. Bu boşluklar "code cave"
//     olarak kullanılabilir.
//
//     Trampoline d3d11.dll'in .text bölümüne yazıldığında:
//     1. Adres d3d11.dll'e ait → pointer validasyon geçer ✅
//     2. d3d11.dll zaten D3D fonksiyon çağırır → davranış tutarlı ✅
//     3. Bellek backed (MEM_IMAGE) → bellek taraması geçer ✅
//     4. Call stack'te d3d11.dll görünür → tamamen meşru ✅
// ============================================================

namespace Stomp {

    struct CodeCave {
        HMODULE hModule;          // Hedef modül (d3d11.dll)
        uintptr_t caveBase;       // Bulunan code cave başlangıcı
        size_t    caveSize;       // Kullanılabilir boyut
        uintptr_t writePtr;       // Yazma kursörü
        bool      valid;
    };

    inline CodeCave g_cave = {};

    // --------------------------------------------------------
    //  PE Section yardımcıları
    // --------------------------------------------------------

    inline IMAGE_SECTION_HEADER* FindSection(HMODULE hMod, const char* name) {
        auto dos = (IMAGE_DOS_HEADER*)hMod;
        auto nt  = (IMAGE_NT_HEADERS*)((uint8_t*)hMod + dos->e_lfanew);
        auto sec = IMAGE_FIRST_SECTION(nt);

        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
            if (strncmp((const char*)sec[i].Name, name, 8) == 0) {
                return &sec[i];
            }
        }
        return nullptr;
    }

    // --------------------------------------------------------
    //  INT3 Padding Scanner
    //  d3d11.dll'in .text bölümünde ardışık 0xCC byte dizileri
    //  (compiler alignment padding) arar.
    //  Minimum 14 byte gerekir (x64 absolute JMP stub boyutu).
    //  Güvenlik için minimum 32 byte isteriz.
    // --------------------------------------------------------

    struct PaddingRegion {
        uintptr_t addr;
        size_t    size;
    };

    // d3d11.dll .text section'ında en büyük padding bölgesini bul
    inline PaddingRegion FindLargestPadding(HMODULE hMod) {
        PaddingRegion best = { 0, 0 };

        auto textSec = FindSection(hMod, ".text");
        if (!textSec) return best;

        uintptr_t base = (uintptr_t)hMod + textSec->VirtualAddress;
        size_t    size = textSec->Misc.VirtualSize;

        uintptr_t currentRun = 0;
        size_t    currentLen = 0;

        for (size_t i = 0; i < size; i++) {
            uint8_t byte = *(uint8_t*)(base + i);
            
            // INT3 (0xCC) veya NOP (0x90) veya zero (0x00) padding
            if (byte == 0xCC || byte == 0x90 || byte == 0x00) {
                if (currentLen == 0) {
                    currentRun = base + i;
                }
                currentLen++;
            } else {
                if (currentLen > best.size) {
                    best.addr = currentRun;
                    best.size = currentLen;
                }
                currentLen = 0;
            }
        }

        // Son bölgeyi kontrol et
        if (currentLen > best.size) {
            best.addr = currentRun;
            best.size = currentLen;
        }

        return best;
    }

    // Birden fazla code cave bul ve toplam alanı hesapla
    // En az 14 byte olan tüm padding bölgelerini topla
    inline bool FindCodeCaves(HMODULE hMod, PaddingRegion* caves, int maxCaves, int* found) {
        auto textSec = FindSection(hMod, ".text");
        if (!textSec) return false;

        uintptr_t base = (uintptr_t)hMod + textSec->VirtualAddress;
        size_t    size = textSec->Misc.VirtualSize;
        
        *found = 0;
        uintptr_t currentRun = 0;
        size_t    currentLen = 0;

        for (size_t i = 0; i < size && *found < maxCaves; i++) {
            uint8_t byte = *(uint8_t*)(base + i);
            
            if (byte == 0xCC || byte == 0x90 || byte == 0x00) {
                if (currentLen == 0) currentRun = base + i;
                currentLen++;
            } else {
                // Minimum 14 byte (JMP [RIP+0] + 8-byte addr)
                if (currentLen >= 14) {
                    caves[*found].addr = currentRun;
                    caves[*found].size = currentLen;
                    (*found)++;
                }
                currentLen = 0;
            }
        }

        if (currentLen >= 14 && *found < maxCaves) {
            caves[*found].addr = currentRun;
            caves[*found].size = currentLen;
            (*found)++;
        }

        return *found > 0;
    }

    // --------------------------------------------------------
    //  Initialize - d3d11.dll'de code cave bul
    // --------------------------------------------------------

    inline bool Initialize() {
        // d3d11.dll modül adını runtime'da çöz
        XC("d3d11.dll", modName);

        HMODULE hD3D = GetModuleHandleA(modName);
        SecureZeroMemory(modName, sizeof(modName));

        if (!hD3D) {
            // D3D11 henüz yüklenmemişse bekle
            // (oyun başlangıcında geç yüklenebilir)
            return false;
        }

        // En büyük padding bölgesini bul
        PaddingRegion best = FindLargestPadding(hD3D);

        // Minimum 32 byte gerekiyor (2 trampoline için)
        if (best.size < 32) {
            // Büyük tek parça bulunamadıysa, birden fazla küçük cave ara
            PaddingRegion caves[64];
            int numCaves = 0;
            if (!FindCodeCaves(hD3D, caves, 64, &numCaves)) {
                return false;
            }

            // En büyüğünü seç
            for (int i = 0; i < numCaves; i++) {
                if (caves[i].size > best.size) {
                    best = caves[i];
                }
            }

            if (best.size < 14) return false; // Hiç yer yok
        }

        g_cave.hModule  = hD3D;
        g_cave.caveBase = best.addr;
        g_cave.caveSize = best.size;
        g_cave.writePtr = best.addr;
        g_cave.valid    = true;

        return true;
    }

    // --------------------------------------------------------
    //  Code cave'e trampoline yaz
    //  d3d11.dll'in kendi .text bölümüne yazıldığı için:
    //  - Adres d3d11.dll adres aralığında → pointer validasyon ✅
    //  - Backed bellek (MEM_IMAGE) → unbacked tespit ✅
    //  - Davranışsal tutarlılık → d3d11.dll D3D çağrısı yapar ✅
    // --------------------------------------------------------

    inline void* CreateTrampoline(void* targetFunc) {
        if (!g_cave.valid) return nullptr;

        // x64 trampoline: 14 bytes
        // FF 25 00 00 00 00   jmp [rip+0]
        // XX XX XX XX XX XX XX XX  (absolute address)
        const size_t trampolineSize = 14;

        // Yeterli alan var mı?
        size_t used = g_cave.writePtr - g_cave.caveBase;
        if (used + trampolineSize > g_cave.caveSize) return nullptr;

        void* cave = (void*)g_cave.writePtr;

        // Geçici yazma izni
        DWORD oldProtect = 0;
        VirtualProtect(cave, trampolineSize, PAGE_READWRITE, &oldProtect);

        uint8_t* code = (uint8_t*)cave;

        // JMP [RIP+0]
        code[0] = 0xFF;
        code[1] = 0x25;
        code[2] = 0x00;
        code[3] = 0x00;
        code[4] = 0x00;
        code[5] = 0x00;

        // Absolute target address
        *(uintptr_t*)(code + 6) = (uintptr_t)targetFunc;

        // Tekrar çalıştırılabilir yap
        VirtualProtect(cave, trampolineSize, PAGE_EXECUTE_READ, &oldProtect);

        // Kursörü ilerlet (16-byte aligned)
        g_cave.writePtr += trampolineSize;
        g_cave.writePtr = (g_cave.writePtr + 15) & ~15;

        return cave;
    }

    // --------------------------------------------------------
    //  Adres doğrulama yardımcıları
    // --------------------------------------------------------

    inline bool IsAddressBacked(void* addr) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) return false;
        return (mbi.Type == MEM_IMAGE);
    }

    inline HMODULE GetBackingModule(void* addr) {
        HMODULE hMod = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)addr, &hMod
        );
        return hMod;
    }

    // Trampoline'ın d3d11.dll'e ait olduğunu doğrula
    inline bool VerifyTrampoline(void* trampoline) {
        if (!g_cave.valid || !trampoline) return false;
        HMODULE owner = GetBackingModule(trampoline);
        return (owner == g_cave.hModule);
    }

    // --------------------------------------------------------
    //  Cleanup - code cave'i orijinal padding'e geri döndür
    // --------------------------------------------------------

    inline void Shutdown() {
        if (g_cave.valid) {
            size_t usedSize = g_cave.writePtr - g_cave.caveBase;
            if (usedSize > 0) {
                DWORD oldProtect = 0;
                VirtualProtect((void*)g_cave.caveBase, usedSize, 
                              PAGE_READWRITE, &oldProtect);
                // Orijinal INT3 padding'e geri döndür
                memset((void*)g_cave.caveBase, 0xCC, usedSize);
                VirtualProtect((void*)g_cave.caveBase, usedSize, 
                              PAGE_EXECUTE_READ, &oldProtect);
            }
            g_cave.valid = false;
        }
    }
}
