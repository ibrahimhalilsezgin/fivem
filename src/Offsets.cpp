#include "Offsets.h"
#include "Memory.h"

namespace Offsets {
    bool Initialize(uintptr_t baseModule) {
        GameBase = baseModule;
        // Delegates to encrypted Cfg::Resolve
        return Cfg::Resolve(baseModule);
    }
}
