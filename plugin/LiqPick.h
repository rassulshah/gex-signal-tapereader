#ifndef LS_LIQPICK_H
#define LS_LIQPICK_H
// LiqPick.h 1.0.0 (2026-11-01, Rassul 09:49-10:21 CT, mockup lp_proposal.html; "+" -> its real rank 10:21 CT) -- WHICH whole-book nodes the
// Liquidity column shows (lsDealerProfile 2.40.0). SDK-free, tested (tests_liqpick/test_liqpick.cpp).
//   ranked = the steady board's nodes (rank 1 = the KING = the biggest real-order node in the whole book, both sides together).
//   SHOWN  = rank 1..TOP (5)  +  any other ranked node with lots >= PCT (50%) of the king that lies within ATR_X (1.0) x the 3-min ATR
//            ABOVE a HIGH level (PDH, PFH, ONH, LonHI, VAH, ...) or BELOW a LOW level (PDL, PFL, ONL, LonLO, VAL, ...); a POC level (weekly
//            POC, prior-day dPOC) counts on either side. Such a node keeps its real rank (6, 7, ...) and carries the level names ("PDL/PFL").
//   pct    = lots / the king's lots x 100, rounded (the king = 100%).
#include <string>
#include <vector>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <utility>
namespace lpk {
static const char* const VERSION = "1.0.0";
static const int TOP = 5;
static const double PCT = 0.50, ATR_X = 1.0;
enum Kind { NONE = 0, HIGH = 1, LOW = 2, POC = 3 };
// a level name -> its kind (key / volume levels only; VWAP, Open, settlement, gamma names = NONE)
inline int kindOf(const std::string& name)
{
    std::string u; for (char c : name) if (std::isalnum((unsigned char)c)) u += (char)std::toupper((unsigned char)c);
    if (u.empty()) return NONE;
    if (u.find("POC") != std::string::npos) return POC;
    if (u.find("VWAP") != std::string::npos || u.find("OPEN") != std::string::npos || u.find("SETTLE") != std::string::npos || u.find("CLOSE") != std::string::npos
        || u.find("MID") != std::string::npos || u.find("GEX") != std::string::npos || u.find("HVL") != std::string::npos || u.find("CALL") != std::string::npos || u.find("PUT") != std::string::npos) return NONE;
    auto ends = [&](const char* s) { const std::string e(s); return u.size() >= e.size() && u.compare(u.size() - e.size(), e.size(), e) == 0; };
    if (u == "HOD") return HIGH;
    if (u == "LOD") return LOW;
    if (ends("HIGH") || ends("HI")) return HIGH;
    if (ends("LOW") || ends("LO")) return LOW;
    if (u.size() <= 6 && ends("H")) return HIGH;
    if (u.size() <= 6 && ends("L")) return LOW;
    return NONE;
}
struct Level { double px = 0; std::string name; int kind = NONE; };
struct Node { double px = 0; long long lots = 0; bool bid = false; int rank = 0; };
struct Pick { size_t idx = 0; int pct = 0; bool extra = false; std::string names; };
// is node price p inside level L's link band (inclusive)? HIGH: [L, L + band]; LOW: [L - band, L]; POC: [L - band, L + band]
inline bool linked(double p, const Level& L, double band)
{
    const double e = 1e-9 * std::max(1.0, std::fabs(L.px));
    if (L.kind == HIGH) return p >= L.px - e && p <= L.px + band + e;
    if (L.kind == LOW) return p <= L.px + e && p >= L.px - band - e;
    if (L.kind == POC) return std::fabs(p - L.px) <= band + e;
    return false;
}
// the rows to show, in rank order. atr = the 3-min ATR (price units); no ATR = no level-linked extras
inline std::vector<Pick> pick(const std::vector<Node>& nodes, const std::vector<Level>& levels, double atr)
{
    std::vector<Pick> out; long long king = 0;
    for (const Node& n : nodes) if (n.rank == 1) king = n.lots;
    if (king <= 0) for (const Node& n : nodes) king = std::max(king, n.lots);
    if (king <= 0) return out;
    const double band = (std::isfinite(atr) && atr > 0) ? ATR_X * atr : -1.0;
    std::vector<size_t> ord; for (size_t i = 0; i < nodes.size(); ++i) if (nodes[i].rank > 0 && nodes[i].lots > 0) ord.push_back(i);
    std::stable_sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return nodes[a].rank < nodes[b].rank; });
    for (size_t i : ord) {
        const Node& n = nodes[i]; Pick p; p.idx = i; p.pct = (int)std::lround(100.0 * (double)n.lots / (double)king);
        if (n.rank <= TOP) { out.push_back(p); continue; }
        if (band < 0 || (double)n.lots + 1e-9 < PCT * (double)king) continue;
        std::vector<std::pair<double, std::string> > hit;   // nearest level first (a long list is shortened to its first name + "+")
        for (const Level& L : levels) if (L.kind != NONE && linked(n.px, L, band)) { bool dup = false; for (const auto& h : hit) if (h.second == L.name) dup = true; if (!dup) hit.push_back(std::make_pair(std::fabs(n.px - L.px), L.name)); }
        std::stable_sort(hit.begin(), hit.end(), [](const std::pair<double, std::string>& a, const std::pair<double, std::string>& b) { return a.first < b.first; });
        std::vector<std::string> nm; for (const auto& h : hit) nm.push_back(h.second);
        if (nm.empty()) continue;
        p.extra = true; for (size_t k = 0; k < nm.size(); ++k) p.names += (k ? "/" : "") + nm[k];
        out.push_back(p);
    }
    return out;
}
} // namespace lpk
#endif
