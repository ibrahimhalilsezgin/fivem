#include "Memory.h"
#include <vector>

namespace Memory {

    uintptr_t GetPointerAddress(uintptr_t base, const std::vector<uintptr_t>& offsets) {
        uintptr_t addr = base;
        for (size_t i = 0; i < offsets.size(); ++i) {
            if (addr < 0x1000) return 0;
            addr = *reinterpret_cast<uintptr_t*>(addr);
            addr += offsets[i];
        }
        return addr;
    }

    uintptr_t GetModuleBase(const char* moduleName) {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
    }
}
