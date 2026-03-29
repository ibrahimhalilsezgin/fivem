#pragma once
#include <windows.h>
#include "../Core/Memory.hpp"
#include "../Game/Offsets.hpp"
#include <vector>

namespace Features {
    struct Vector3 { float x, y, z; };
    struct Vector2 { float x, y; };

    class ESP {
    public:
        static void Render(HDC hdc, int screenWidth, int screenHeight);
        
        static bool Enabled;

    private:
        static bool WorldToScreen(Vector3 pos, Vector2& screen, float* matrix, int sw, int sh);
    };
}
