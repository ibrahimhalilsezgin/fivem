#include "Core/Common.hpp"
#include "Features/ESP.hpp"
#include "UI/Drawing.hpp"

namespace Features {
    bool ESP::Enabled = false;

    bool ESP::WorldToScreen(Vector3 pos, Vector2& screen, float* matrix, int sw, int sh) {
        float w = matrix[3] * pos.x + matrix[7] * pos.y + matrix[11] * pos.z + matrix[15];
        if (w < 0.01f) return false;

        float invw = 1.0f / w;
        float x = (matrix[0] * pos.x + matrix[4] * pos.y + matrix[8] * pos.z + matrix[12]) * invw;
        float y = (matrix[1] * pos.x + matrix[5] * pos.y + matrix[9] * pos.z + matrix[13]) * invw;

        screen.x = (sw / 2.0f) * (1.0f + x);
        screen.y = (sh / 2.0f) * (1.0f - y);
        return true;
    }

    void ESP::Render(HDC hdc, int sw, int sh) {
        if (!Enabled) return;

        uintptr_t base = Core::Memory::GetModuleBase();
        uintptr_t vp = Core::Memory::Read<uintptr_t>(base + Game::Offsets::ViewPort);
        if (!vp) return;

        float matrix[16];
        for (int i = 0; i < 16; i++) {
            matrix[i] = Core::Memory::Read<float>(vp + Game::Offsets::ViewMatrix + (i * 4));
        }

        uintptr_t replay = Core::Memory::Read<uintptr_t>(base + Game::Offsets::ReplayInterface);
        uintptr_t pedInterface = Core::Memory::Read<uintptr_t>(replay + Game::Offsets::PedInterface);
        uintptr_t pedList = Core::Memory::Read<uintptr_t>(pedInterface + Game::Offsets::PedList);
        int maxPeds = Core::Memory::Read<int>(pedInterface + Game::Offsets::PedCount);

        if (!pedList || maxPeds < 1) return;

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 255, 0));

        uintptr_t localPed = 0;
        uintptr_t world = Core::Memory::Read<uintptr_t>(base + Game::Offsets::World);
        if (world) localPed = Core::Memory::Read<uintptr_t>(world + Game::Offsets::LocalPed);

        for (int i = 0; i < (maxPeds > 128 ? 128 : maxPeds); i++) {
            uintptr_t ped = Core::Memory::Read<uintptr_t>(pedList + (i * 0x10));
            if (!ped || ped == localPed) continue;

            Vector3 pos;
            pos.x = Core::Memory::Read<float>(ped + Game::Offsets::Position);
            pos.y = Core::Memory::Read<float>(ped + Game::Offsets::Position + 4);
            pos.z = Core::Memory::Read<float>(ped + Game::Offsets::Position + 8);

            Vector2 sFoot, sHead;
            Vector3 headPos = { pos.x, pos.y, pos.z + 0.8f };

            if (WorldToScreen(pos, sFoot, matrix, sw, sh) && WorldToScreen(headPos, sHead, matrix, sw, sh)) {
                float h = sFoot.y - sHead.y;
                float w = h * 0.6f;
                // Draw Box
                UI::Drawing::Box(hdc, (int)(sHead.x - w / 2), (int)sHead.y, (int)w, (int)h, RGB(255, 0, 0));
                
                // Draw Health
                char buf[16];
                int hpValue = (int)Core::Memory::Read<float>(ped + Game::Offsets::Health);
                sprintf_s(buf, "%d HP", hpValue);
                UI::Drawing::Text(hdc, (int)(sHead.x - 10), (int)(sHead.y - 15), buf);
            }
        }
    }
}
