#include "LocalPlayer.hpp"

namespace Features {
    bool LocalPlayer::GodMode = false;
    bool LocalPlayer::InfArmor = false;

    void LocalPlayer::Update() {
        uintptr_t localPed = GetLocalPed();
        if (!localPed) return;

        if (GodMode) {
            Core::Memory::Write<float>(localPed + Game::Offsets::Health, 200.0f);
        }

        if (InfArmor) {
            Core::Memory::Write<float>(localPed + Game::Offsets::Armor, 200.0f);
        }
    }

    uintptr_t LocalPlayer::GetLocalPed() {
        uintptr_t base = Core::Memory::GetModuleBase();
        uintptr_t world = Core::Memory::Read<uintptr_t>(base + Game::Offsets::World);
        if (!world) return 0;

        return Core::Memory::Read<uintptr_t>(world + Game::Offsets::LocalPed);
    }
}
