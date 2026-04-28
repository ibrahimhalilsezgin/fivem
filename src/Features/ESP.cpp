#include "Core/Common.hpp"
#include "Features/ESP.hpp"
#include "UI/Drawing.hpp"

namespace Feat {
    bool Vis::bOn = false;

    bool Vis::_W2S(V3 p, V2& s, float* m, int sw, int sh) {
        float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
        if (w < 0.01f) return false;

        float iw = 1.0f / w;
        float x = (m[0] * p.x + m[4] * p.y + m[8]  * p.z + m[12]) * iw;
        float y = (m[1] * p.x + m[5] * p.y + m[9]  * p.z + m[13]) * iw;

        s.x = (sw / 2.0f) * (1.0f + x);
        s.y = (sh / 2.0f) * (1.0f - y);
        return true;
    }

    void Vis::Draw(HDC hdc, int sw, int sh) {
        if (!bOn) return;

        uintptr_t base = Core::Mem::Base();
        uintptr_t vp = Core::Mem::Rd<uintptr_t>(base + Cfg::ViewPort());
        if (!vp) return;

        float mtx[16];
        for (int i = 0; i < 16; i++) {
            mtx[i] = Core::Mem::Rd<float>(vp + Cfg::VMatrix() + (i * 4));
        }

        uintptr_t rp = Core::Mem::Rd<uintptr_t>(base + Cfg::ReplayIf());
        uintptr_t pi = Core::Mem::Rd<uintptr_t>(rp + Cfg::PedIf());
        uintptr_t pl = Core::Mem::Rd<uintptr_t>(pi + Cfg::PedLst());
        int mc = Core::Mem::Rd<int>(pi + Cfg::PedCnt());

        if (!pl || mc < 1) return;

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 255, 0));

        uintptr_t myPed = 0;
        uintptr_t world = Core::Mem::Rd<uintptr_t>(base + Cfg::World());
        if (world) myPed = Core::Mem::Rd<uintptr_t>(world + Cfg::LocalPed());

        int limit = mc > 128 ? 128 : mc;
        for (int i = 0; i < limit; i++) {
            uintptr_t ped = Core::Mem::Rd<uintptr_t>(pl + (i * 0x10));
            if (!ped || ped == myPed) continue;

            V3 pos;
            pos.x = Core::Mem::Rd<float>(ped + Cfg::Pos());
            pos.y = Core::Mem::Rd<float>(ped + Cfg::Pos() + 4);
            pos.z = Core::Mem::Rd<float>(ped + Cfg::Pos() + 8);

            V2 sF, sH;
            V3 hp = { pos.x, pos.y, pos.z + 0.8f };

            if (_W2S(pos, sF, mtx, sw, sh) && _W2S(hp, sH, mtx, sw, sh)) {
                float h = sF.y - sH.y;
                float w = h * 0.6f;
                UI::Gfx::Rect(hdc, (int)(sH.x - w / 2), (int)sH.y, (int)w, (int)h, RGB(255, 0, 0));

                char buf[16];
                int hpVal = (int)Core::Mem::Rd<float>(ped + Cfg::Hp());
                sprintf_s(buf, "%d", hpVal);
                UI::Gfx::Txt(hdc, (int)(sH.x - 10), (int)(sH.y - 15), buf);
            }
        }
    }
}
