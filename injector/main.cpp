#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#pragma pack(push, 1)
struct Shellcode {
    uint8_t sub_rsp[4] = { 0x48, 0x83, 0xEC, 0x28 }; // sub rsp, 28h
    uint8_t mov_rcx[2] = { 0x48, 0xB9 };             // mov rcx, ...
    uint64_t pString = 0;
    uint8_t mov_rax[2] = { 0x48, 0xB8 };             // mov rax, ...
    uint64_t pLoadLibrary = 0;
    uint8_t call_rax[2] = { 0xFF, 0xD0 };            // call rax
    uint8_t add_rsp[4] = { 0x48, 0x83, 0xC4, 0x28 }; // add rsp, 28h
    uint8_t jmp[2] = { 0xFF, 0x25 };                 // jmp [rip+0]
    uint32_t zero = 0;
    uint64_t retAddr = 0;
};
#pragma pack(pop)

DWORD FindFiveMProcess() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    DWORD pid = 0;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            std::wstring name(pe.szExeFile);
            if (name.find(L"FiveM") != std::wstring::npos && 
                name.find(L"GTAProcess.exe") != std::wstring::npos) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pid;
}

DWORD GetMainThread(DWORD pid) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    THREADENTRY32 te;
    te.dwSize = sizeof(te);

    DWORD tid = 0;
    if (Thread32First(hSnap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                tid = te.th32ThreadID;
                break;
            }
        } while (Thread32Next(hSnap, &te));
    }
    CloseHandle(hSnap);
    return tid;
}

bool InjectViaThreadHijack(DWORD pid, const std::wstring& dllPath) {
    DWORD tid = GetMainThread(pid);
    if (!tid) {
        std::cout << "[-] Main thread not found.\n";
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cout << "[-] Failed to open process. Are you running as Admin?\n";
        return false;
    }

    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
    if (!hThread) {
        std::cout << "[-] Failed to open thread.\n";
        CloseHandle(hProcess);
        return false;
    }

    SuspendThread(hThread);

    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(hThread, &ctx)) {
        std::cout << "[-] Failed to get thread context.\n";
        ResumeThread(hThread);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return false;
    }

    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    void* pMem = VirtualAllocEx(hProcess, nullptr, sizeof(Shellcode) + pathSize, 
                                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    if (!pMem) {
        std::cout << "[-] Failed to allocate memory in target process.\n";
        ResumeThread(hThread);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return false;
    }

    uint64_t strAddr = (uint64_t)pMem + sizeof(Shellcode);
    
    Shellcode sc;
    sc.pString = strAddr;
    sc.pLoadLibrary = (uint64_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
    sc.retAddr = ctx.Rip;

    WriteProcessMemory(hProcess, pMem, &sc, sizeof(Shellcode), nullptr);
    WriteProcessMemory(hProcess, (void*)strAddr, dllPath.c_str(), pathSize, nullptr);

    ctx.Rip = (uint64_t)pMem;
    if (!SetThreadContext(hThread, &ctx)) {
        std::cout << "[-] Failed to set thread context.\n";
    } else {
        std::cout << "[+] Thread hijacked successfully. Injection triggered.\n";
    }

    ResumeThread(hThread);

    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

int main() {
    SetConsoleTitleA("Loader");
    std::cout << "[*] Waiting for FiveM...\n";

    DWORD pid = 0;
    while (!pid) {
        pid = FindFiveMProcess();
        Sleep(1000);
    }

    std::cout << "[+] Found FiveM GTAProcess (PID: " << pid << ")\n";
    Sleep(2000); // Wait for the game to fully initialize

    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    std::wstring dllPath = std::wstring(currentDir) + L"\\svchost_x64.dll";

    if (!std::filesystem::exists(dllPath)) {
        std::wcout << L"[-] DLL not found: " << dllPath << L"\n";
        system("pause");
        return 1;
    }

    std::cout << "[*] Injecting stealth module...\n";
    if (InjectViaThreadHijack(pid, dllPath)) {
        std::cout << "[+] Injection complete. You can close this window.\n";
    } else {
        std::cout << "[-] Injection failed.\n";
    }

    Sleep(3000);
    return 0;
}
