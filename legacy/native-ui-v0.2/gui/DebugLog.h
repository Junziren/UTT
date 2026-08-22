#include <fstream>
namespace {
    inline std::ofstream& gDbg() {
        static std::ofstream f("D:/utt_debug.log", std::ios::app);
        return f;
    }
    inline void dbg(const char* s) {
        gDbg() << s << std::endl << std::flush;
    }
}
