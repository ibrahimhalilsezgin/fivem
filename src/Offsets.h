#pragma once
#include <cstdint>
#include <string>

namespace Offsets {
    // Dinamik seçilecek offsetler
    inline uintptr_t GameBase = 0;
    inline uintptr_t GameWorld = 0;
    inline uintptr_t ReplayInterface = 0;
    inline uintptr_t ViewPort = 0;
    inline uintptr_t Camera = 0;
    inline uintptr_t BlipList = 0;
    inline uintptr_t LocalPlayer = 0x8;
    inline uintptr_t PlayerInfo = 0;
    inline uintptr_t Id = 0;
    inline uintptr_t Health = 0;
    inline uintptr_t MaxHealth = 0;
    inline uintptr_t Armor = 0;
    inline uintptr_t WeaponManager = 0;
    inline uintptr_t BoneList = 0;
    inline uintptr_t Silent = 0;
    inline uintptr_t Vehicle = 0;
    inline uintptr_t VisibleFlag = 0;
    inline uintptr_t Waypoint = 0;

    // Versiyon Algılama Fonksiyonu
    bool Initialize(uintptr_t baseModule);
    
    // O anki oyun sürümünü tutar
    inline std::string CurrentVersion = "Unknown";
}
