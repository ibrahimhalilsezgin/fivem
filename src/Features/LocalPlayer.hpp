#pragma once
#include "../Core/Memory.hpp"
#include "../Game/Offsets.hpp"

namespace Features {
    class LocalPlayer {
    public:
        static void Update();

        static bool GodMode;
        static bool InfArmor;

    private:
        static uintptr_t GetLocalPed();
    };
}
