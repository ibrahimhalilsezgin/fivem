#include "Gui.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>
#include "LuaEngine.h"

// main.cpp içerisinde tanımladığımız global LuaEngine nesnesine erişim
extern LuaEngine g_LuaEngine;

namespace Gui {
    HWND hMainWindow = nullptr;
    HWND hEditControl = nullptr;
    HWND hButtonControl = nullptr;
    HANDLE hGuiThread = nullptr;

    // GUI mesaj döngülerini işleyen ana Window Procedure fonksiyonu
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_COMMAND:
                // Butona tıklanma olayı (Buton ID'miz 1)
                if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
                    // Edit kontrolündeki yazının uzunluğunu al
                    int length = GetWindowTextLengthA(hEditControl);
                    if (length > 0) {
                        // Yazıyı almak için buffer oluştur
                        std::vector<char> buffer(length + 1);
                        GetWindowTextA(hEditControl, buffer.data(), length + 1);
                        
                        // Lua motoruna kodu gönder ve çalıştır
                        g_LuaEngine.ExecuteString(buffer.data());
                    } else {
                        std::cerr << "[-] Calistirilacak Lua betigi bos!" << std::endl;
                    }
                }
                break;
            case WM_CLOSE:
                DestroyWindow(hwnd);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    // Arayüzün donmaması için ayrı bir thread içerisinde çalışan GUI döngüsü
    DWORD WINAPI WindowThread(LPVOID) {
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        const char* CLASS_NAME = "LuaExecutorGUIClass";

        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Standart arka plan rengi

        RegisterClassA(&wc);

        // Ana Pencereyi Oluştur
        hMainWindow = CreateWindowExA(
            0, CLASS_NAME, "Lua Executor (C++ DLL)",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 450, 350,
            nullptr, nullptr, hInstance, nullptr
        );

        if (!hMainWindow) {
            std::cerr << "[-] GUI penceresi olusturulamadi." << std::endl;
            return 0;
        }

        // Metin Giriş Alanını Oluştur (Lua kodunu yazacağımız yer)
        hEditControl = CreateWindowExA(
            0, "EDIT", "-- Buraya Lua kodunuzu yazin...\nprintToConsole('GUI uzerinden merhaba!')\n",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
            10, 10, 410, 240,
            hMainWindow, nullptr, hInstance, nullptr
        );

        // 'Çalıştır' Butonunu Oluştur
        hButtonControl = CreateWindowExA(
            0, "BUTTON", "Calistir (Execute)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 260, 150, 35,
            hMainWindow, (HMENU)1, hInstance, nullptr
        );

        // Pencereyi Göster
        ShowWindow(hMainWindow, SW_SHOW);
        UpdateWindow(hMainWindow);

        // Windows Mesaj Döngüsü (Pencerenin yanıt vermesini sağlar)
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Temizlik
        UnregisterClassA(CLASS_NAME, hInstance);
        hMainWindow = nullptr;
        return 0;
    }

    void Initialize() {
        std::cout << "[+] Windows GUI (Arayuz) baslatiliyor..." << std::endl;
        hGuiThread = CreateThread(nullptr, 0, WindowThread, nullptr, 0, nullptr);
    }

    void Shutdown() {
        if (hMainWindow) {
            std::cout << "[-] GUI kapatiliyor..." << std::endl;
            SendMessage(hMainWindow, WM_CLOSE, 0, 0); // Pencereye kapanma mesajı gönder
        }
        if (hGuiThread) {
            // Thread'in kapanması için kısa bir süre bekle
            WaitForSingleObject(hGuiThread, 1000);
            CloseHandle(hGuiThread);
            hGuiThread = nullptr;
        }
    }
}
