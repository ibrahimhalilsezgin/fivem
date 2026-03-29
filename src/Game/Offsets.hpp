#pragma once
#include <windows.h>

namespace Game {
    struct Offsets {
        // Core Pointers
        static constexpr uintptr_t World = 0x25B14B0;
        static constexpr uintptr_t ReplayInterface = 0x1FBD4F0;
        static constexpr uintptr_t ViewPort = 0x201DBA0;
        static constexpr uintptr_t Camera = 0x201E7D0;

        // Player Offsets
        static constexpr uintptr_t LocalPed = 0x8;
        static constexpr uintptr_t Health = 0x280;
        static constexpr uintptr_t MaxHealth = 0x284;
        static constexpr uintptr_t Armor = 0x150C;
        static constexpr uintptr_t Position = 0x90;

        // Replay/Entity Offsets
        static constexpr uintptr_t PedInterface = 0x18;
        static constexpr uintptr_t PedList = 0x100;
        static constexpr uintptr_t PedCount = 0x108;

        // ViewMatrix (Relative to ViewPort)
        static constexpr uintptr_t ViewMatrix = 0x24C;
    };
}
