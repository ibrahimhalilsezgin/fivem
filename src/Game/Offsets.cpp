#include "Game/Offsets.hpp"
#include "Core/Memory.hpp"

namespace Cfg {

    // Version-specific offset sets - all encrypted
    struct VerSet {
        uintptr_t gw, ri, vp, cam, bl;
        uintptr_t lp, pinfo, id, hp, mhp, arm, wm, bone;
        uintptr_t sil, veh, vflag, wp;
        uintptr_t pedIf, pedLst, pedCnt, vmtx, pos;
    };

    static bool TryVersion(uintptr_t base, const VerSet& vs) {
        uintptr_t worldAddr = Core::Mem::Rd<uintptr_t>(base + D(vs.gw));
        if (worldAddr == 0) return false;

        uintptr_t lpAddr = Core::Mem::Rd<uintptr_t>(worldAddr + D(vs.lp));
        uintptr_t vpAddr = Core::Mem::Rd<uintptr_t>(base + D(vs.vp));
        uintptr_t riAddr = Core::Mem::Rd<uintptr_t>(base + D(vs.ri));

        if (lpAddr != 0 && vpAddr != 0 && riAddr != 0) {
            _w   = vs.gw;
            _ri  = vs.ri;
            _vp  = vs.vp;
            _cam = vs.cam;
            _lp  = vs.lp;
            _hp  = vs.hp;
            _mhp = vs.mhp;
            _arm = vs.arm;
            _pos = vs.pos;
            _pi  = vs.pedIf;
            _pl  = vs.pedLst;
            _pc  = vs.pedCnt;
            _vm  = vs.vmtx;
            return true;
        }
        return false;
    }

    bool Resolve(uintptr_t base) {
        // Version 3570
        {
            VerSet v;
            v.gw = E(0x25EC580); v.ri = E(0x1FB0418); v.vp = E(0x2058BA0); v.cam = E(0x2059778);
            v.bl = E(0x205E000); v.lp = E(0x8); v.pinfo = E(0x10A8); v.id = E(0xE8);
            v.hp = E(0x280); v.mhp = E(0x284); v.arm = E(0x150C); v.wm = E(0x10B8);
            v.bone = E(0x430); v.sil = E(0x102D550); v.veh = E(0xD10); v.vflag = E(0x145C);
            v.wp = E(0x205E000); v.pedIf = E(0x18); v.pedLst = E(0x100); v.pedCnt = E(0x108);
            v.vmtx = E(0x24C); v.pos = E(0x90);
            if (TryVersion(base, v)) return true;
        }

        // Version 3407
        {
            VerSet v;
            v.gw = E(0x25D7108); v.ri = E(0x1F9A9D8); v.vp = E(0x20431C0); v.cam = E(0x2043DF8);
            v.bl = E(0x20440C8); v.lp = E(0x8); v.pinfo = E(0x10A8); v.id = E(0xE8);
            v.hp = E(0x280); v.mhp = E(0x284); v.arm = E(0x150C); v.wm = E(0x10B8);
            v.bone = E(0x410); v.sil = E(0x102FF89); v.veh = E(0xD10); v.vflag = E(0x145C);
            v.wp = E(0x2047D50); v.pedIf = E(0x18); v.pedLst = E(0x100); v.pedCnt = E(0x108);
            v.vmtx = E(0x24C); v.pos = E(0x90);
            if (TryVersion(base, v)) return true;
        }

        // Version 3323
        {
            VerSet v;
            v.gw = E(0x25C15B0); v.ri = E(0x1F85458); v.vp = E(0x202DC50); v.cam = E(0x202E878);
            v.bl = E(0x2002888); v.lp = E(0x8); v.pinfo = E(0x10A8); v.id = E(0xE8);
            v.hp = E(0x280); v.mhp = E(0x284); v.arm = E(0x150C); v.wm = E(0x10B8);
            v.bone = E(0x410); v.sil = E(0x1026CAD); v.veh = E(0xD10); v.vflag = E(0x145C);
            v.wp = E(0x2022DE0); v.pedIf = E(0x18); v.pedLst = E(0x100); v.pedCnt = E(0x108);
            v.vmtx = E(0x24C); v.pos = E(0x90);
            if (TryVersion(base, v)) return true;
        }

        // Version 3258 (default)
        {
            VerSet v;
            v.gw = E(0x25B14B0); v.ri = E(0x1FBD4F0); v.vp = E(0x201DBA0); v.cam = E(0x201E7D0);
            v.bl = E(0x2002FA0); v.lp = E(0x8); v.pinfo = E(0x10A8); v.id = E(0xE8);
            v.hp = E(0x280); v.mhp = E(0x284); v.arm = E(0x150C); v.wm = E(0x10B8);
            v.bone = E(0x410); v.sil = E(0x101A65D); v.veh = E(0xD10); v.vflag = E(0x145C);
            v.wp = E(0x2023400); v.pedIf = E(0x18); v.pedLst = E(0x100); v.pedCnt = E(0x108);
            v.vmtx = E(0x24C); v.pos = E(0x90);
            if (TryVersion(base, v)) return true;
        }

        // Version 3095
        {
            VerSet v;
            v.gw = E(0x2593320); v.ri = E(0x1F58B58); v.vp = E(0x20019E0); v.cam = E(0x20025B8);
            v.bl = E(0x2002888); v.lp = E(0x8); v.pinfo = E(0x10A8); v.id = E(0xE8);
            v.hp = E(0x280); v.mhp = E(0x284); v.arm = E(0x150C); v.wm = E(0x10B8);
            v.bone = E(0x410); v.sil = E(0x100F5A4); v.veh = E(0xD10); v.vflag = E(0x145C);
            v.wp = E(0x2002FA0); v.pedIf = E(0x18); v.pedLst = E(0x100); v.pedCnt = E(0x108);
            v.vmtx = E(0x24C); v.pos = E(0x90);
            if (TryVersion(base, v)) return true;
        }

        // Version 2944
        {
            VerSet v;
            v.gw = E(0x257BEA0); v.ri = E(0x1F42068); v.vp = E(0x1FEAAC0); v.cam = E(0x1FEB968);
            v.bl = E(0x1FEB968); v.lp = E(0x8); v.pinfo = E(0x10A8); v.id = E(0xE8);
            v.hp = E(0x280); v.mhp = E(0x284); v.arm = E(0x150C); v.wm = E(0x10B8);
            v.bone = E(0x410); v.sil = E(0x1003F80); v.veh = E(0xD10); v.vflag = E(0x145C);
            v.wp = E(0x1FF3130); v.pedIf = E(0x18); v.pedLst = E(0x100); v.pedCnt = E(0x108);
            v.vmtx = E(0x24C); v.pos = E(0x90);
            if (TryVersion(base, v)) return true;
        }

        // Version 2802
        {
            VerSet v;
            v.gw = E(0x254D448); v.ri = E(0x1F5B820); v.vp = E(0x1FBC100); v.cam = E(0x1FBCCD8);
            v.bl = E(0x1FBCFA8); v.lp = E(0x8); v.pinfo = E(0x10A8); v.id = E(0xE8);
            v.hp = E(0x280); v.mhp = E(0x284); v.arm = E(0x150C); v.wm = E(0x10B8);
            v.bone = E(0x410); v.sil = E(0xFF716C); v.veh = E(0xD10); v.vflag = E(0x145C);
            v.wp = E(0x1FBD6E0); v.pedIf = E(0x18); v.pedLst = E(0x100); v.pedCnt = E(0x108);
            v.vmtx = E(0x24C); v.pos = E(0x90);
            if (TryVersion(base, v)) return true;
        }

        return false;
    }
}
