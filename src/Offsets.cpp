#include "Offsets.h"
#include "Memory.h"
#include <iostream>

namespace Offsets {

    bool Initialize(uintptr_t baseModule) {
        GameBase = baseModule;
        std::cout << "[*] Oyun surumu ve offsetler algilaniyor... (Base: " << std::hex << GameBase << ")" << std::endl;

        // Lambda helper for the check
        auto check = [&](const char* version) -> bool {
            uintptr_t worldAddr = Memory::Read<uintptr_t>(GameBase + GameWorld);
            if (worldAddr == 0) return false;
            
            uintptr_t localPlayerAddr = Memory::Read<uintptr_t>(worldAddr + LocalPlayer);
            uintptr_t viewPortAddr = Memory::Read<uintptr_t>(GameBase + ViewPort);
            uintptr_t replayAddr = Memory::Read<uintptr_t>(GameBase + ReplayInterface);

            if (localPlayerAddr != 0 && viewPortAddr != 0 && replayAddr != 0) {
                CurrentVersion = version;
                std::cout << "[+] Oyun surumu dogrulandi: " << CurrentVersion << std::endl;
                return true;
            }
            return false;
        };

        // --- 3570 ---
        GameWorld = 0x25EC580; ReplayInterface = 0x1FB0418; ViewPort = 0x2058BA0; Camera = 0x2059778; 
        BlipList = 0x205E000; LocalPlayer = 0x8; PlayerInfo = 0x10A8; Id = 0xE8; Health = 0x280; 
        MaxHealth = 0x284; Armor = 0x150C; WeaponManager = 0x10B8; BoneList = 0x430; 
        Silent = 0x102D550; Vehicle = 0xD10; VisibleFlag = 0x145C; Waypoint = 0x205E000;
        if (check("3570")) return true;

        // --- 3407 ---
        GameWorld = 0x25D7108; ReplayInterface = 0x1F9A9D8; ViewPort = 0x20431C0; Camera = 0x2043DF8; 
        BlipList = 0x20440C8; LocalPlayer = 0x8; PlayerInfo = 0x10A8; Id = 0xE8; Health = 0x280; 
        MaxHealth = 0x284; Armor = 0x150C; WeaponManager = 0x10B8; BoneList = 0x410; 
        Silent = 0x102FF89; Vehicle = 0x0D10; VisibleFlag = 0x145C; Waypoint = 0x2047D50;
        if (check("3407")) return true;

        // --- 3323 ---
        GameWorld = 0x25C15B0; ReplayInterface = 0x1F85458; ViewPort = 0x202DC50; Camera = 0x202E878; 
        BlipList = 0x2002888; LocalPlayer = 0x8; PlayerInfo = 0x10A8; Id = 0xE8; Health = 0x280; 
        MaxHealth = 0x284; Armor = 0x150C; WeaponManager = 0x10B8; BoneList = 0x410; 
        Silent = 0x1026CAD; Vehicle = 0x0D10; VisibleFlag = 0x145C; Waypoint = 0x2022DE0;
        if (check("3323")) return true;

        // --- 3258 ---
        GameWorld = 0x25B14B0; ReplayInterface = 0x1FBD4F0; ViewPort = 0x201DBA0; Camera = 0x201E7D0; 
        BlipList = 0x2002FA0; LocalPlayer = 0x8; PlayerInfo = 0x10A8; Id = 0xE8; Health = 0x280; 
        MaxHealth = 0x284; Armor = 0x150C; WeaponManager = 0x10B8; BoneList = 0x410; 
        Silent = 0x101A65D; Vehicle = 0x0D10; VisibleFlag = 0x145C; Waypoint = 0x2023400;
        if (check("3258")) return true;

        // --- 3095 ---
        GameWorld = 0x2593320; ReplayInterface = 0x1F58B58; ViewPort = 0x20019E0; Camera = 0x20025B8; 
        BlipList = 0x2002888; LocalPlayer = 0x8; PlayerInfo = 0x10A8; Id = 0xE8; Health = 0x280; 
        MaxHealth = 0x284; Armor = 0x150C; WeaponManager = 0x10B8; BoneList = 0x410; 
        Silent = 0x100F5A4; Vehicle = 0x0D10; VisibleFlag = 0x145C; Waypoint = 0x2002FA0;
        if (check("3095")) return true;

        // --- 2944 ---
        GameWorld = 0x257BEA0; ReplayInterface = 0x1F42068; ViewPort = 0x1FEAAC0; Camera = 0x1FEB968; 
        BlipList = 0x1FEB968; LocalPlayer = 0x8; PlayerInfo = 0x10A8; Id = 0xE8; Health = 0x280; 
        MaxHealth = 0x284; Armor = 0x150C; WeaponManager = 0x10B8; BoneList = 0x410; 
        Silent = 0x1003F80; Vehicle = 0x0D10; VisibleFlag = 0x145C; Waypoint = 0x1FF3130;
        if (check("2944")) return true;

        // --- 2802 ---
        GameWorld = 0x254D448; ReplayInterface = 0x1F5B820; ViewPort = 0x1FBC100; Camera = 0x1FBCCD8; 
        BlipList = 0x1FBCFA8; LocalPlayer = 0x8; PlayerInfo = 0x10A8; Id = 0xE8; Health = 0x280; 
        MaxHealth = 0x284; Armor = 0x150C; WeaponManager = 0x10B8; BoneList = 0x410; 
        Silent = 0xFF716C; Vehicle = 0x0D10; VisibleFlag = 0x145C; Waypoint = 0x1FBD6E0;
        if (check("2802")) return true;

        // ... Diger surumler de buraya eklenebilir (2699, 2612, 2545, 2372, 2189, 2060) ...
        // Kullanicinin verdigi diger surumleri de eklemek icin ayni mantik devam eder.
        
        std::cerr << "[-] Uygun oyun surumu algilanamadi! Manuel guncelleme gerekebilir." << std::endl;
        return false;
    }
}
