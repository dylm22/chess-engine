#include "memory.h"

#include <cstdlib>
#include <ios>
#include <iostream>
#include <ostream>
#include <windows.h>

//"
// The needed Windows API for processor groups could be missed from old Windows
// versions, so instead of calling them directly (forcing the linker to resolve
// the calls at compile time), try to load them at runtime. To do this we need
// first to define the corresponding function pointers.
//" -someone smarter than me

extern "C" {
    using OpenProcessToken_t = bool (*)(HANDLE, DWORD, PHANDLE);
    using LookupPrivilegeValueA_t = bool (*)(LPCSTR, LPCSTR, PLUID);
    using AdjustTokenPrivileges_t =
        bool (*)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);
}

namespace engine {
    void aligned_large_pages_free(void* mem) {

        if (mem && !VirtualFree(mem, 0, MEM_RELEASE))
        {
            DWORD err = GetLastError();
            std::cerr << "failed to free large page memory. Error code: 0x" << std::hex << err
                << std::dec << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    //you're bugging if you think i wrote this 
    static void* aligned_large_pages_alloc_windows([[maybe_unused]] size_t allocSize) {

        HANDLE hProcessToken{};
        LUID   luid{};
        void* mem = nullptr;

        const size_t largePageSize = GetLargePageMinimum();
        if (!largePageSize)
            return nullptr;

        // Dynamically link OpenProcessToken, LookupPrivilegeValue and AdjustTokenPrivileges

        HMODULE hAdvapi32 = GetModuleHandle(TEXT("advapi32.dll"));

        if (!hAdvapi32)
            hAdvapi32 = LoadLibrary(TEXT("advapi32.dll"));

        auto OpenProcessToken_f =
            OpenProcessToken_t((void (*)()) GetProcAddress(hAdvapi32, "OpenProcessToken"));
        if (!OpenProcessToken_f)
            return nullptr;
        auto LookupPrivilegeValueA_f =
            LookupPrivilegeValueA_t((void (*)()) GetProcAddress(hAdvapi32, "LookupPrivilegeValueA"));
        if (!LookupPrivilegeValueA_f)
            return nullptr;
        auto AdjustTokenPrivileges_f =
            AdjustTokenPrivileges_t((void (*)()) GetProcAddress(hAdvapi32, "AdjustTokenPrivileges"));
        if (!AdjustTokenPrivileges_f)
            return nullptr;

        // We need SeLockMemoryPrivilege, so try to enable it for the process

        if (!OpenProcessToken_f(  // OpenProcessToken()
            GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcessToken))
            return nullptr;

        if (LookupPrivilegeValueA_f(nullptr, "SeLockMemoryPrivilege", &luid))
        {
            TOKEN_PRIVILEGES tp{};
            TOKEN_PRIVILEGES prevTp{};
            DWORD            prevTpLen = 0;

            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Try to enable SeLockMemoryPrivilege. Note that even if AdjustTokenPrivileges()
            // succeeds, we still need to query GetLastError() to ensure that the privileges
            // were actually obtained.

            if (AdjustTokenPrivileges_f(hProcessToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &prevTp,
                &prevTpLen)
                && GetLastError() == ERROR_SUCCESS)
            {
                // Round up size to full pages and allocate
                allocSize = (allocSize + largePageSize - 1) & ~size_t(largePageSize - 1);
                mem = VirtualAlloc(nullptr, allocSize, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                    PAGE_READWRITE);

                // Privilege no longer needed, restore previous state
                AdjustTokenPrivileges_f(hProcessToken, FALSE, &prevTp, 0, nullptr, nullptr);
            }
        }

        CloseHandle(hProcessToken);

        return mem;
    }
    //if i understand it correctly, allocating a larger memory page is better because it reduces
    //translation lookaside buffer misses, however you need specific api calls and elevated privileges
    //so shoutout the internet  

    
    void* aligned_large_pages_alloc(size_t allocSize) {

        // Try to allocate large pages
        void* mem = aligned_large_pages_alloc_windows(allocSize);

        // Fall back to regular, page-aligned, allocation if necessary
        if (!mem)
            mem = VirtualAlloc(nullptr, allocSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        return mem;
    }
}