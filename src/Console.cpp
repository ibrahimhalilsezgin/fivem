#include "Console.h"
#include <Windows.h>

// Console allocation is intentionally stripped in release builds
// No visible console = no detection surface

void Dbg::Open() {
#ifdef _DEBUG
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
#endif
}

void Dbg::Close() {
#ifdef _DEBUG
    fclose(stdout);
    fclose(stderr);
    fclose(stdin);
    FreeConsole();
#endif
}
