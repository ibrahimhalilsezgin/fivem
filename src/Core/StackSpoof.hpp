#pragma once
#include <windows.h>
#include <cstdint>
#include <intrin.h>
#include "ModuleStomp.hpp"

// ============================================================
//  Call Stack Spoofing Engine
//  Çözüm: FiveM'in "Call Stack Walking" tespitini aşar.
//
//  FiveM, thread'lerin call stack'lerini tarar ve return 
//  address'lerin "backed" (kayıtlı) modüllere işaret edip 
//  etmediğini kontrol eder.
//
//  Stratejiler:
//  1. Return address'leri stomped modülün adres alanına yönlendir
//  2. Gadget-based spoofing: meşru modüllerde "jmp rbx" veya 
//     "ret" gadget'ları bulup stack frame'leri taklit et
//  3. Kritik API çağrılarını stomped modülden yap
// ============================================================

namespace StackSpoof {

    // --------------------------------------------------------
    //  ROP Gadget Finder
    //  Meşru modüllerde (ntdll.dll, kernelbase.dll) 
    //  "ret" (0xC3) gadget'ları bulur
    // --------------------------------------------------------

    struct Gadget {
        uintptr_t addr;
        HMODULE   parentModule;
    };

    inline Gadget g_retGadget = {};  // "ret" gadget in legitimate module

    // ntdll.dll içinde tek bir "ret" (0xC3) instruction'ı bul
    inline bool FindRetGadget() {
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (!hNtdll) return false;

        auto dos = (IMAGE_DOS_HEADER*)hNtdll;
        auto nt  = (IMAGE_NT_HEADERS*)((uint8_t*)hNtdll + dos->e_lfanew);
        auto sec = IMAGE_FIRST_SECTION(nt);

        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
            if (!(sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;

            uintptr_t base = (uintptr_t)hNtdll + sec[i].VirtualAddress;
            size_t size = sec[i].Misc.VirtualSize;

            // Basit "ret" instruction'ı ara (0xC3)
            for (size_t j = 0; j < size; j++) {
                uint8_t* ptr = (uint8_t*)(base + j);
                if (*ptr == 0xC3) {
                    g_retGadget.addr = (uintptr_t)ptr;
                    g_retGadget.parentModule = hNtdll;
                    return true;
                }
            }
        }
        return false;
    }

    // --------------------------------------------------------
    //  Indirect Call Helper
    //  API çağrılarını stomped modülün adres alanından yapar.
    //  Bu sayede call stack'te çağrının kaynağı olarak
    //  meşru modül görünür.
    // --------------------------------------------------------

    // x64 indirect call stub şablonu:
    // Bu stub stomped modüle yerleştirilir ve gerçek API'ye atlar
    // call stack walking sırasında bu stub'ın adresi meşru görünür

    // Genel amaçlı API wrapper - stomped modülden çağrı yapar
    template<typename Fn, typename... Args>
    __forceinline auto SpoofedCall(Fn func, Args... args) {
        // Eğer stomped modül varsa, trampoline üzerinden çağır
        if (Stomp::g_stomped.valid) {
            // Fonksiyonu doğrudan çağır ama return address'i spoof et
            // Trampoline zaten stomped modülde olduğu için
            // call stack meşru görünür
            auto trampoline = reinterpret_cast<Fn>(
                Stomp::CreateTrampoline((void*)func)
            );
            if (trampoline) {
                return trampoline(args...);
            }
        }
        // Fallback: doğrudan çağır
        return func(args...);
    }

    // --------------------------------------------------------
    //  Stack Frame Mangler
    //  Mevcut thread'in stack frame'lerini manipüle eder
    //  Return address'leri meşru modüle yönlendirir
    // --------------------------------------------------------

    // Çağıran fonksiyonun return address'ini spoof et
    // Bu, call stack walk sırasında mevcut frame'in
    // meşru bir modülden geliyormuş gibi görünmesini sağlar
    __forceinline void SpoofCurrentFrame() {
        if (!g_retGadget.addr) return;

        // _AddressOfReturnAddress() ile mevcut fonksiyonun
        // return address'ine erişip onu ntdll'deki ret gadget'a yönlendirebiliriz.
        // Ancak bu tehlikelidir - sadece suspend durumunda scan yapan AC'ler için etkili.
        
        // Daha güvenli yaklaşım: SetUnhandledExceptionFilter yerine
        // structured exception handler kullanarak stack frame'leri temizle
    }

    // --------------------------------------------------------
    //  Asynchronous Procedure Call (APC) tabanlı yürütme
    //  Meşru bir thread'in APC kuyruğuna kod ekleyerek
    //  call stack'i temiz tutar (thread hijacking)
    // --------------------------------------------------------

    // Meşru bir thread bul ve APC üzerinden kod çalıştır
    // Bu, call stack'in tamamen meşru görünmesini sağlar
    // çünkü thread zaten meşru bir modüle ait

    using ApcCallback = void(WINAPI*)(ULONG_PTR);

    inline bool ExecuteViaApc(ApcCallback callback, ULONG_PTR param) {
        // Mevcut process'teki meşru bir thread'i bul
        // ve APC kuyruğuna callback'imizi ekle

        // Kendi thread'imize APC queue'la
        HANDLE hThread = GetCurrentThread();
        DWORD result = QueueUserAPC((PAPCFUNC)callback, hThread, param);
        
        if (result) {
            // Alertable wait ile APC'nin çalışmasını tetikle
            SleepEx(0, TRUE);
            return true;
        }
        return false;
    }

    // --------------------------------------------------------
    //  Timer-based execution
    //  CreateTimerQueueTimer ile kod çalıştırarak
    //  call stack'i Windows thread pool'undan başlatır
    //  FiveM bu durumda call stack'te ntdll.dll thread pool
    //  fonksiyonlarını görür (tamamen meşru)
    // --------------------------------------------------------

    using TimerCallback = WAITORTIMERCALLBACK;

    inline bool ExecuteViaTimer(TimerCallback callback, PVOID param) {
        HANDLE hTimer = nullptr;
        HANDLE hTimerQueue = CreateTimerQueue();
        if (!hTimerQueue) return false;

        // 0ms delay = hemen çalıştır, 0 period = tek seferlik
        BOOL ok = CreateTimerQueueTimer(
            &hTimer, hTimerQueue,
            callback, param,
            0,     // DueTime: hemen
            0,     // Period: tekrarlama yok
            WT_EXECUTEDEFAULT
        );

        if (!ok) {
            DeleteTimerQueue(hTimerQueue);
            return false;
        }

        // Timer'ın çalışmasını bekle
        Sleep(100);

        // Cleanup
        DeleteTimerQueueTimer(hTimerQueue, hTimer, INVALID_HANDLE_VALUE);
        DeleteTimerQueueEx(hTimerQueue, INVALID_HANDLE_VALUE);
        return true;
    }

    // --------------------------------------------------------
    //  Windows Thread Pool execution
    //  TpAllocWork / TpPostWork / TpWaitForWork zinciri
    //  ile call stack tamamen ntdll thread pool çağrılarından oluşur
    // --------------------------------------------------------

    using WorkCallback = PTP_WORK_CALLBACK;

    inline bool ExecuteViaThreadPool(WorkCallback callback, PVOID param) {
        PTP_WORK work = CreateThreadpoolWork(callback, param, nullptr);
        if (!work) return false;

        SubmitThreadpoolWork(work);
        WaitForThreadpoolWorkCallbacks(work, FALSE);
        CloseThreadpoolWork(work);
        return true;
    }

    // --------------------------------------------------------
    //  Initialization
    // --------------------------------------------------------

    inline bool Initialize() {
        return FindRetGadget();
    }
}
