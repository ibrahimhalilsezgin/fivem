#pragma once
#include <windows.h>

namespace Core {
    class Memory {
    public:
        template <typename T>
        static T Read(uintptr_t address) {
            __try {
                if (address < 0x20000 || address > 0x7FFFFFFEFFFF) return T();
                return *reinterpret_cast<T*>(address);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return T();
            }
        }

        template <typename T>
        static bool Write(uintptr_t address, T value) {
            __try {
                if (address < 0x20000 || address > 0x7FFFFFFEFFFF) return false;
                *reinterpret_cast<T*>(address) = value;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }

        static uintptr_t GetModuleBase(const char* moduleName = nullptr) {
            return reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
        }
    };
}
