#pragma once
#include <Windows.h>
#include <cstdint>
#include <vector>

namespace Memory {
    template <typename T>
    T Read(uintptr_t address) {
        if (address < 0x1000) return T();
        return *reinterpret_cast<T*>(address);
    }

    template <typename T>
    void Write(uintptr_t address, T value) {
        if (address < 0x1000) return;
        *reinterpret_cast<T*>(address) = value;
    }

    uintptr_t GetPointerAddress(uintptr_t base, const std::vector<uintptr_t>& offsets);
    uintptr_t GetModuleBase(const char* moduleName);
}
