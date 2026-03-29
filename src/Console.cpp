#include "Console.h"
#include <Windows.h>
#include <iostream>

void Console::Allocate() {
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    
    // Konsol penceresinin başlığını ayarlama
    SetConsoleTitleA("Gelistirici Hata Ayiklama Konsolu");
    
    std::cout << "[+] Konsol basariyla ayrildi ve yonlendirildi." << std::endl;
}

void Console::Free() {
    std::cout << "[-] Konsol temizleniyor..." << std::endl;
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    fclose(stdout);
    fclose(stderr);
    fclose(stdin);
    FreeConsole();
}
