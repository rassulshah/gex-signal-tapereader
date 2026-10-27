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
static const char* const VERSION = "1.3.0";   // (1.3.0, 2026-10-27 LP 0.60.0) Tag.px = the level's price (the order -> its row in the Dealer Profile 2.28.0 Levels column)     // (1.2.0, Rassul 11:32 "it should be PDH + 1ATR, meaning the level or slightly higher within the 1 ATR range, it cannot be below PDH") ONE-SIDED   // (1.1.0, Rassul 11:27 "use 1atr, for example the large order is within pdh+1atr or pdl-1atr")
static const double TOL_TICKS = 2.0;     // PARAM
static const double TOL_ATR = 1.0;       // HIS RULE (2026-10-27 11:27): within 1 x the 3-min ATR of the level (e.g. up to PDH + 1 ATR, down to
                                         // PDL - 1 ATR); 11:17 "it doesnt have to be the exact level, if its close enough it should be badged"
enum Kind { NONE = 0, KEY = 1, VOLUME = 2, GAMMA = 3 };
struct Level { std::string name; double px = 0; };
struct Tag { Kind kind = NONE; std::string name; double dist = 0; double px = 0; std::string text() const { return kind == NONE ? std::string() : name + "?"; } };
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
// (1.2.0) which side of the level the tag reaches: +1 = the level up to level + 1 ATR (a HIGH: PDH, ONH, LonHI, PFH, AsiaH, VAH, CR call
// resistance), -1 = the level down to level - 1 ATR (a LOW: PDL, ONL, LonLO, PFL, AsiaL, VAL, PS put support), 0 = a level with no
// high / low meaning (POC, HVL, Open, G-levels, GW): the order's own side decides - an offer (resistance) up, a bid (support) down.
inline int dirOf(const std::string& nameIn) {
    const std::string n = upper(nameIn);
    if (n == "VAH" || n.compare(0, 2, "CR") == 0) return 1;
    if (n == "VAL" || n.compare(0, 2, "PS") == 0) return -1;
    if (n.find("POC") != std::string::npos || n == "HVL" || n == "OPEN") return 0;
    const size_t k = n.size();
    if (k >= 4 && (n.compare(k - 4, 4, "HIGH") == 0)) return 1;
    if (k >= 3 && (n.compare(k - 3, 3, "LOW") == 0)) return -1;
    if (k >= 2 && n.compare(k - 2, 2, "HI") == 0) return 1;
    if (k >= 2 && n.compare(k - 2, 2, "LO") == 0) return -1;
    if (k >= 2 && n[k - 1] == 'H' && std::isalpha((unsigned char)n[k - 2])) return 1;    // PDH, ONH, PFH, AsiaH, PWH ...
    if (k >= 2 && n[k - 1] == 'L' && std::isalpha((unsigned char)n[k - 2]) && n != "HVL") return -1;   // PDL, ONL, PFL, AsiaL ...
    return 0;
}
// offer = true for a sell order (resistance), false for a bid (support): used only for a level with no high / low meaning
inline Tag tagFor(double px, const std::vector<Level>& levels, double tick, double atr, bool offer) {
    Tag best; if (!(tick > 0) || !std::isfinite(px)) return best;
    const double tol = std::max(TOL_TICKS * tick, (std::isfinite(atr) && atr > 0) ? TOL_ATR * atr : 0.0) + tick * 1e-6;
    double bestD = 1e300;
    for (size_t i = 0; i < levels.size(); ++i) {
        const Level& L = levels[i]; if (!(L.px > 0) || !std::isfinite(L.px) || L.name.empty()) continue;
        int dir = dirOf(L.name); if (dir == 0) dir = offer ? 1 : -1;
        const double d = (px - L.px) * dir;                     // >= 0 = on the level or beyond it, on its own side
        if (d < -tick * 1e-6 || d > tol) continue;             // below a high / above a low: never tagged (his 11:32 rule)
        const Kind k = kindOf(L.name);
        if (d < bestD - tick * 1e-6 || (std::fabs(d - bestD) <= tick * 1e-6 && k > best.kind)) { bestD = d; best.kind = k; best.name = L.name.substr(0, 8); best.dist = d; best.px = L.px; }
    }
    return best;
}
inline Tag tagFor(double px, const std::vector<Level>& levels, double tick, double atr) { return tagFor(px, levels, tick, atr, true); }   // (old call shape)
inline const char* kindName(Kind k) { return k == GAMMA ? "gamma" : k == VOLUME ? "volume" : k == KEY ? "key" : "none"; }
}  // namespace llt
#endif
