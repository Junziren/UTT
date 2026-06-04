#pragma once
#include <windows.h>

namespace UTT {

// Writes exception info to D:\UTT\crash_log.txt before crash
inline void installCrashHandler()
{
    static bool installed = false;
    if (installed) return;
    installed = true;

    ::AddVectoredExceptionHandler(1, [](EXCEPTION_POINTERS* ex) -> LONG {
        // Only handle fatal exceptions
        DWORD code = ex->ExceptionRecord->ExceptionCode;
        if (code == 0xE06D7363) // C++ exception
            return EXCEPTION_CONTINUE_SEARCH;

        auto f = ::CreateFileA("D:\\UTT\\crash_log.txt", GENERIC_WRITE, 0,
                               nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            ::SetFilePointer(f, 0, nullptr, FILE_END);
            char buf[256];
            int len = wsprintfA(buf, "CRASH: code=0x%08X addr=0x%p\r\n",
                                code, ex->ExceptionRecord->ExceptionAddress);
            DWORD written;
            ::WriteFile(f, buf, len, &written, nullptr);
            ::CloseHandle(f);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    });
}

} // namespace UTT