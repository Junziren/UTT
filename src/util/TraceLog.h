#pragma once
#include <windows.h>
#include <fstream>
#include <mutex>

namespace UTT {

inline void debugTrace(const char* msg)
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream f("D:\\UTT\\utt_trace.log", std::ios::app);
    if (f.is_open()) {
        DWORD tid = ::GetCurrentThreadId();
        f << "[" << tid << "] " << msg << std::endl;
    }
}

// RAII trace for function entry/exit
struct ScopedTrace {
    const char* func;
    ScopedTrace(const char* f) : func(f) { debugTrace((std::string("> ") + func).c_str()); }
    ~ScopedTrace() { debugTrace((std::string("< ") + func).c_str()); }
};

} // namespace UTT

#define UTT_TRACE() UTT::ScopedTrace _utt_trace(__FUNCTION__)
#define UTT_LOG(msg) UTT::debugTrace(msg)