#ifndef LS_HOT_RELOAD_DEMO_LOGIC_H
#define LS_HOT_RELOAD_DEMO_LOGIC_H
// HotReloadDemoLogic.h  1.0.1 (2026-10-18) -- the text the Hot Reload Demo draws (SDK-free, tested on Linux)
#include <string>
namespace hrdemo {
static const int BOX_W = 420;        // fixed box size (his rule), top left under the IRT title row
static const int BOX_H = 44;
// (1.0.1, 2026-10-18 17:21 CT his chart: the box sat on the Liquidity Profile read at the top left - the REV / ABS headline + its
// 2 - 3 lines, ~95 px under the title row) the box starts BELOW that read block
static const int TOP_Y = 24 + 100;   // px under the pane top: the IRT title row (24) + the read block (~95) + a small gap
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
