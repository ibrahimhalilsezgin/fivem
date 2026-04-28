#pragma once
#include <windows.h>

namespace Core {
    class Mem {
    public:
        template <typename T>
        static __forceinline T Rd(uintptr_t a) {
            __try {
                if (a < 0x20000 || a > 0x7FFFFFFEFFFF) return T();
                return *reinterpret_cast<T*>(a);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return T();
            }
        }

        template <typename T>
        static __forceinline bool Wr(uintptr_t a, T v) {
            __try {
                if (a < 0x20000 || a > 0x7FFFFFFEFFFF) return false;
                *reinterpret_cast<T*>(a) = v;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }

        static __forceinline uintptr_t Base(const char* n = nullptr) {
            return reinterpret_cast<uintptr_t>(GetModuleHandleA(n));
        }

        // Pointer chain resolver
        static __forceinline uintptr_t Chain(uintptr_t base, const uintptr_t* ofs, size_t cnt) {
            uintptr_t addr = base;
            for (size_t i = 0; i < cnt; ++i) {
                if (addr < 0x20000) return 0;
                addr = Rd<uintptr_t>(addr);
                if (!addr) return 0;
                addr += ofs[i];
            }
            return addr;
        }
    };
}
