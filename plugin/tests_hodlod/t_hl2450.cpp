// t_hl2450.cpp -- lsHodLod 2.45.0 / NowLevels 1.5.0 (Rassul 2026-11-25 11:41 CT "update the level line to look for potential sweep levels", 11:47 "rename to
// Sweep Watch also . go", mockup watch-sweep-levels-mockup.html): the level badge reads "SWEEP WATCH:" and names only major levels NOT yet traded through in
// the current session part (day = since the RTH opening candle). Real candles: GC / ES 3-minute 2026-11-25 01:54 .. 11:51 AM (LRA-IRT-Bars, bar END stamps)
// and that morning's LRA-KeyLevels files.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2450.cpp -o t && ./t
#include "../LevelMarks.h"
#include "../NowLevels.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const char* w) { return s.find(w) != std::string::npos; }
static long long wall(const std::string& s) { std::tm t{}; std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec); t.tm_year -= 1900; t.tm_mon -= 1; return (long long)timegm(&t); }
static std::vector<nowl::Bar> bars(const char* p) { std::vector<nowl::Bar> A; std::ifstream in(p); std::string l; while (std::getline(in, l)) { if (l.size() < 20 || l[4] != '-') continue;
    std::stringstream ss(l); std::string x; std::vector<std::string> v; while (std::getline(ss, x, '|')) v.push_back(x); if (v.size() < 6) continue;
    nowl::Bar b; b.t = wall(v[0]); b.o = std::stod(v[1]); b.h = std::stod(v[2]); b.l = std::stod(v[3]); b.c = std::stod(v[4]); b.v = std::stod(v[5]); A.push_back(b); } return A; }
static std::vector<nowl::Key> keys(const char* p) { std::vector<nowl::Key> K; std::ifstream in(p); std::string l; while (std::getline(in, l)) { if (l.size() < 3 || l[0] != 'L') continue;
    std::stringstream ss(l); std::string x; std::vector<std::string> v; while (std::getline(ss, x, '|')) v.push_back(x); if (v.size() < 3) continue;
    std::stringstream ns(v[1]); std::string one; while (std::getline(ns, one, '/')) if (!one.empty()) K.push_back(nowl::Key{ one, std::stod(v[2]) }); } return K; }
// the line with the candle stamped hh:mm forming (closed candles before it)
static std::string at(const std::vector<nowl::Bar>& A, const std::vector<nowl::Key>& K, int hh, int mm, double tick, int openMin, double* live = nullptr) {
    std::vector<nowl::Bar> B; for (const nowl::Bar& b : A) { const long long q = ((b.t % 86400) + 86400) % 86400; B.push_back(b); if (q == hh * 3600 + mm * 60) break; }
    if (live) { *live = B.back().c; }
    return nowl::plain(nowl::segments(nowl::compute(B, {}, K, B.back().c, tick, openMin))); }
int main() {
    C(std::string(nowl::VERSION) == "1.5.0", "NowLevels 1.5.0");
    for (const char* m : { "ES", "NQ", "MES", "MNQ", "CL", "NG", "MCL", "GC", "EU", "6E", "MGC", "HG", "ZB", "" }) C(nowl::rthOpenMin(m) == lvm::rthOpenMin(m), std::string("RTH open = LevelMarks: ") + m);
    const std::vector<nowl::Bar> G = bars("gc1125_bars.csv"), E = bars("es1125_bars.csv"); const std::vector<nowl::Key> GK = keys("gc1125_keys.csv"), EK = keys("es1125_keys.csv");
    C(G.size() > 150 && E.size() > 150 && GK.size() >= 18 && EK.size() >= 18, "fixtures loaded");
    // the Next levels the badge names (at 11:15 / 11:51 the forming GC candle is at a level, so the line shows TESTING with "Next" - the same pick)
    auto out = [&](const std::vector<nowl::Bar>& A, const std::vector<nowl::Key>& K, int hh, int mm, double tick, int om) { std::vector<nowl::Bar> B; for (const nowl::Bar& b : A) { const long long q = ((b.t % 86400) + 86400) % 86400; B.push_back(b); if (q == hh * 3600 + mm * 60) break; } return nowl::compute(B, {}, K, B.back().c, tick, om); };
    { const nowl::Out o = out(G, GK, 11, 15, 0.1, 440); C(o.up.ok && o.up.name == "ALO" && !o.dn.ok, "GC 11:15: AsiaLO above, nothing untouched below - up " + o.up.name + " dn " + (o.dn.ok ? o.dn.name : "-"));
      const std::string l = at(G, GK, 11, 15, 0.1, 440); C(has(l, "Next \xE2\x86\x91 ALO") && !has(l, "PWL"), "GC 11:15 line - " + l); }
    { const nowl::Out o = out(G, GK, 11, 51, 0.1, 440); C(o.up.ok && o.up.name == "LHI" && !o.dn.ok, "GC 11:51: LonHI above (AsiaLO had its sweep) - up " + o.up.name + " dn " + (o.dn.ok ? o.dn.name : "-")); }
    { const nowl::Out o = out(E, EK, 11, 51, 0.25, 510); C(o.up.ok && o.up.name == "PFL" && o.dn.ok && o.dn.name == "PWL", "ES 11:51: PWL below, PFL above - up " + o.up.name + " dn " + o.dn.name);
      const std::string l = at(E, EK, 11, 51, 0.25, 510); C(has(l, "SWEEP WATCH: PWL \xE2\x86\x93 \xC2\xB7 PFL \xE2\x86\x91 \xC2\xB7 Bk / Sw"), "ES 11:51 badge - " + l); }
    // old callers (no open minute) keep the 1.4.0 choice
    { const nowl::Out o = out(G, GK, 11, 15, 0.1, -2); C(o.up.ok && o.up.name == "PWL", "GC 11:15 without the filter = the old pick (PWL above) - " + o.up.name); }
    // overnight: the window is the whole session; a level printed by the session itself is not 'traded through' by the candle that made it
    { std::vector<nowl::Bar> B; for (const nowl::Bar& b : G) { const long long q = ((b.t % 86400) + 86400) % 86400; B.push_back(b); if (q == 5 * 3600) break; }
      C(nowl::partStart(B, (int)B.size() - 2, 440) == 0, "05:00 is overnight: the window starts at the session's first candle");
      std::vector<nowl::Bar> D; for (const nowl::Bar& b : G) { D.push_back(b); const long long q = ((b.t % 86400) + 86400) % 86400; if (q == 9 * 3600) break; }
      const int ps = nowl::partStart(D, (int)D.size() - 2, 440); const long long q0 = ((D[(size_t)ps].t % 86400) + 86400) % 86400;
      C(q0 == 7 * 3600 + 21 * 60, "09:00 GC: the day part starts at the 7:21 candle (the first candle ending after the 7:20 open, as LevelMarks dayStart) - got " + std::to_string(q0 / 60)); }
    // every candle close of the day part: the named levels are untouched since the open
    { int bad = 0, steps = 0; for (size_t n = 3; n < G.size(); ++n) { const long long q = ((G[n].t % 86400) + 86400) % 86400; if (q <= 7 * 3600 + 21 * 60) continue; ++steps;
        std::vector<nowl::Bar> B(G.begin(), G.begin() + (long)n + 1); const nowl::Out o = nowl::compute(B, {}, GK, B.back().c, 0.1, 440);
        double hi = -1e9, lo = 1e9; for (int k = nowl::partStart(B, (int)B.size() - 2, 440); k <= (int)B.size() - 2; ++k) { hi = std::max(hi, B[(size_t)k].h); lo = std::min(lo, B[(size_t)k].l); }
        if ((o.up.ok && hi > o.up.px + 0.05) || (o.dn.ok && lo < o.dn.px - 0.05)) ++bad; }
      C(bad == 0 && steps > 80, "GC day part: no named level was already traded through (" + std::to_string(steps) + " closes, " + std::to_string(bad) + " bad)"); }
    std::printf("t_hl2450 (lsHodLod 2.45.0): %d passed, %d failed\n", ok, f); return f ? 1 : 0;
}
