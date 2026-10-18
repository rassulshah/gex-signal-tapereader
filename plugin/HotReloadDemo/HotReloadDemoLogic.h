#ifndef LS_HOT_RELOAD_DEMO_LOGIC_H
#define LS_HOT_RELOAD_DEMO_LOGIC_H
// HotReloadDemoLogic.h  1.0.0 (2026-10-18) -- the text the Hot Reload Demo draws (SDK-free, tested on Linux)
#include <string>
namespace hrdemo {
static const int BOX_W = 420;        // fixed box size (his rule), top left under the IRT title row
static const int BOX_H = 44;
static const int MAX_STATUS = 70;    // characters of the shim's status line that fit the box
inline std::string title(const char* version, long long build, long calcs)
{
    std::string s = "Hot Reload Demo v";
    s += version ? version : "?";
    s += "  build " + std::to_string(build);
    s += "  calcs " + std::to_string(calcs);
    return s;
}
inline std::string statusLine(const std::string& shim)
{
    if (shim.empty()) return "hot reload OK";
    if ((int)shim.size() <= MAX_STATUS) return shim;
    return shim.substr(0, (size_t)MAX_STATUS - 3) + "...";
}
}
#endif
