#pragma once
#include <JuceHeader.h>
#include <fstream>
#include <mutex>
#include <thread>
#include <cstdio>

// ================================================================
// Cross-platform crash & trace logging for UTT plugin
// ================================================================

#ifdef _WIN32
  #include <windows.h>
#else
  #include <signal.h>
  #include <execinfo.h>
#endif

namespace UTT {

// -- crash handler --------------------------------------------------
#ifdef _WIN32
inline void installCrashHandler()
{
    static bool installed = false;
    if (installed) return;
    installed = true;

    ::AddVectoredExceptionHandler(1, [](EXCEPTION_POINTERS* ex) -> LONG {
        DWORD code = ex->ExceptionRecord->ExceptionCode;
        if (code == 0xE06D7363) return EXCEPTION_CONTINUE_SEARCH;
        auto f = std::ofstream(
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                .getChildFile("utt_crash_log.txt").getFullPathName().toStdString(),
            std::ios::app);
        if (f.is_open())
            f << "CRASH: code=0x" << std::hex << code
              << " addr=" << ex->ExceptionRecord->ExceptionAddress << std::endl;
        return EXCEPTION_CONTINUE_SEARCH;
    });
}
#else
inline void crashSignalHandler(int sig)
{
    auto f = std::ofstream(
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile("utt_crash_log.txt").getFullPathName().toStdString(),
        std::ios::app);
    if (f.is_open()) {
        f << "CRASH: signal=" << sig << "\n";
        void* stack[32];
        int frames = backtrace(stack, 32);
        char** symbols = backtrace_symbols(stack, frames);
        for (int i = 0; i < frames && symbols; ++i)
            f << "  " << (symbols[i] ? symbols[i] : "???") << "\n";
        free(symbols);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

inline void installCrashHandler()
{
    static bool installed = false;
    if (installed) return;
    installed = true;
    signal(SIGSEGV, crashSignalHandler);
    signal(SIGBUS,  crashSignalHandler);
    signal(SIGABRT, crashSignalHandler);
}
#endif

// -- trace logger ---------------------------------------------------
inline void debugTrace(const char* msg)
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    auto f = std::ofstream(
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile("utt_trace.log").getFullPathName().toStdString(),
        std::ios::app);
    if (f.is_open())
        f << "[" << std::this_thread::get_id() << "] " << msg << std::endl;
}

struct ScopedTrace {
    const char* func;
    ScopedTrace(const char* f) : func(f) { debugTrace((std::string("> ") + func).c_str()); }
    ~ScopedTrace() { debugTrace((std::string("< ") + func).c_str()); }
};

} // namespace UTT

#define UTT_TRACE() UTT::ScopedTrace _utt_trace(__FUNCTION__)
#define UTT_LOG(msg) UTT::debugTrace(msg)
