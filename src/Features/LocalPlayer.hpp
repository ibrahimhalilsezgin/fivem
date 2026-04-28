#pragma once
#include "../Core/Memory.hpp"
#include "../Game/Offsets.hpp"

namespace Feat {
    class LP {
    public:
        static void Tick();

        static bool bGod;
        static bool bArm;

    private:
        static uintptr_t _GetPed();
    };
}
