#include "LocalPlayer.hpp"

namespace Feat {
    bool LP::bGod = false;
    bool LP::bArm = false;

    void LP::Tick() {
        uintptr_t ped = _GetPed();
        if (!ped) return;

        if (bGod) {
            Core::Mem::Wr<float>(ped + Cfg::Hp(), 200.0f);
        }

        if (bArm) {
            Core::Mem::Wr<float>(ped + Cfg::Arm(), 200.0f);
        }
    }

    uintptr_t LP::_GetPed() {
        uintptr_t base = Core::Mem::Base();
        uintptr_t world = Core::Mem::Rd<uintptr_t>(base + Cfg::World());
        if (!world) return 0;
        return Core::Mem::Rd<uintptr_t>(world + Cfg::LocalPed());
    }
}
