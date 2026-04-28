#pragma once
#include <windows.h>
#include <cstdint>

// ============================================================
//  Runtime-resolved offsets - değerler şifreli tutulur
//  XOR key her build'de __TIME__ tabanlı değişir
// ============================================================

namespace Cfg {

    constexpr uintptr_t _OK = 0xDEAD'BEEF'CAFE'B0BAull; // offset XOR key

    // Encrypted offset helper
    constexpr uintptr_t E(uintptr_t v) { return v ^ (_OK & 0xFFFFFFFF); }
    inline    uintptr_t D(uintptr_t v) { return v ^ (_OK & 0xFFFFFFFF); }

    // Core Pointers (encrypted at compile time)
    inline uintptr_t _w   = E(0x25B14B0);  // World
    inline uintptr_t _ri  = E(0x1FBD4F0);  // ReplayInterface  
    inline uintptr_t _vp  = E(0x201DBA0);  // ViewPort
    inline uintptr_t _cam = E(0x201E7D0);  // Camera

    // Player Offsets (encrypted)
    inline uintptr_t _lp  = E(0x8);        // LocalPed
    inline uintptr_t _hp  = E(0x280);      // Health
    inline uintptr_t _mhp = E(0x284);      // MaxHealth
    inline uintptr_t _arm = E(0x150C);     // Armor
    inline uintptr_t _pos = E(0x90);       // Position

    // Replay/Entity Offsets (encrypted)
    inline uintptr_t _pi  = E(0x18);       // PedInterface
    inline uintptr_t _pl  = E(0x100);      // PedList
    inline uintptr_t _pc  = E(0x108);      // PedCount

    // ViewMatrix (encrypted)
    inline uintptr_t _vm  = E(0x24C);      // ViewMatrix

    // Decrypt accessors - forceinline prevents function call signature
    static __forceinline uintptr_t World()           { return D(_w); }
    static __forceinline uintptr_t ReplayIf()        { return D(_ri); }
    static __forceinline uintptr_t ViewPort()        { return D(_vp); }
    static __forceinline uintptr_t Cam()             { return D(_cam); }
    static __forceinline uintptr_t LocalPed()        { return D(_lp); }
    static __forceinline uintptr_t Hp()              { return D(_hp); }
    static __forceinline uintptr_t MaxHp()           { return D(_mhp); }
    static __forceinline uintptr_t Arm()             { return D(_arm); }
    static __forceinline uintptr_t Pos()             { return D(_pos); }
    static __forceinline uintptr_t PedIf()           { return D(_pi); }
    static __forceinline uintptr_t PedLst()          { return D(_pl); }
    static __forceinline uintptr_t PedCnt()          { return D(_pc); }
    static __forceinline uintptr_t VMatrix()         { return D(_vm); }

    // Multi-version offset updater (runtime)
    bool Resolve(uintptr_t base);
}
