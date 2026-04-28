#pragma once
#include <windows.h>
#include "../Core/Memory.hpp"
#include "../Game/Offsets.hpp"

namespace Feat {
    struct V3 { float x, y, z; };
    struct V2 { float x, y; };

    class Vis {
    public:
        static void Draw(HDC hdc, int sw, int sh);
        static bool bOn;

    private:
        static bool _W2S(V3 p, V2& s, float* m, int sw, int sh);
    };
}
