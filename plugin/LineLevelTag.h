#ifndef LS_LINELEVELTAG_H
#define LS_LINELEVELTAG_H
// LineLevelTag.h 1.0.0 (2026-10-27, Rassul 11:14-11:16 CT "do you put a badge on the large limit lines ... if they are key levels /
// volume levels / gamma levels" -> "build"). The LEVEL TAG of a limit-order line: when the order rests ON a named level, its label
// gets that level's name and a "?" ("100 PDH?") - "?" until the nightly proves, per market and per kind, that orders on that kind of
// level hold / attract better than the same orders away from levels (ticket T-20261027-17). SDK-free, C++17, no state.
// ON = inside the level's ZONE: within max(TOL_TICKS ticks, TOL_ATR x the chart's 3-min ATR) of it - "close enough", not the exact
// price (Rassul 11:17). [the zone width is our PARAM]
// Kinds (for the record and the test): GAMMA = MenthorQ (CR, PS, HVL, GW, G1.., 0DTE variants), VOLUME = POC / VAH / VAL / HVN
// (DPoc, WPoc, VPOC ...), KEY = every other named level (PDH, PDL, PFH, PFL, ONH, ONL, LonHI, LonLO, AsiaH, Open ...).
// Several levels on the order: the NEAREST wins; a tie -> GAMMA, then VOLUME, then KEY order of the input.
#include <cmath>
#include <string>
#include <vector>
#include <cctype>
namespace llt {
static const char* const VERSION = "1.1.0";   // (1.1.0, Rassul 11:27 "use 1atr, for example the large order is within pdh+1atr or pdl-1atr")
static const double TOL_TICKS = 2.0;     // PARAM
static const double TOL_ATR = 1.0;       // HIS RULE (2026-10-27 11:27): within 1 x the 3-min ATR of the level (e.g. up to PDH + 1 ATR, down to
                                         // PDL - 1 ATR); 11:17 "it doesnt have to be the exact level, if its close enough it should be badged"
enum Kind { NONE = 0, KEY = 1, VOLUME = 2, GAMMA = 3 };
struct Level { std::string name; double px = 0; };
struct Tag { Kind kind = NONE; std::string name; double dist = 0; std::string text() const { return kind == NONE ? std::string() : name + "?"; } };
inline std::string upper(const std::string& s) { std::string o; for (char c : s) o += (char)std::toupper((unsigned char)c); return o; }
inline Kind kindOf(const std::string& nameIn) {
    const std::string n = upper(nameIn);
    if (n.empty()) return NONE;
    if (n.find("POC") != std::string::npos || n == "VAH" || n == "VAL" || n.find("HVN") != std::string::npos || n.find("LVN") != std::string::npos
        || n.compare(0, 3, "DVA") == 0 || n.compare(0, 3, "WVA") == 0) return VOLUME;
    if (n == "HVL" || n.compare(0, 2, "CR") == 0 || n.compare(0, 2, "PS") == 0 || n.compare(0, 2, "GW") == 0 || n.find("GEX") != std::string::npos
        || (n.size() >= 2 && n[0] == 'G' && std::isdigit((unsigned char)n[1])) || n.find("0DTE") != std::string::npos || n.find("CALL") != std::string::npos
        || n.find("PUT") != std::string::npos || n.find("GAMMA") != std::string::npos) return GAMMA;
    return KEY;
}
inline Tag tagFor(double px, const std::vector<Level>& levels, double tick, double atr) {
    Tag best; if (!(tick > 0) || !std::isfinite(px)) return best;
    const double tol = std::max(TOL_TICKS * tick, (std::isfinite(atr) && atr > 0) ? TOL_ATR * atr : 0.0) + tick * 1e-6;
    double bestD = 1e300;
    for (size_t i = 0; i < levels.size(); ++i) {
        const Level& L = levels[i]; if (!(L.px > 0) || !std::isfinite(L.px) || L.name.empty()) continue;
        const double d = std::fabs(L.px - px); if (d > tol) continue;
        const Kind k = kindOf(L.name);
        if (d < bestD - tick * 1e-6 || (std::fabs(d - bestD) <= tick * 1e-6 && k > best.kind)) { bestD = d; best.kind = k; best.name = L.name.substr(0, 8); best.dist = d; }
    }
    return best;
}
inline const char* kindName(Kind k) { return k == GAMMA ? "gamma" : k == VOLUME ? "volume" : k == KEY ? "key" : "none"; }
}  // namespace llt
#endif
