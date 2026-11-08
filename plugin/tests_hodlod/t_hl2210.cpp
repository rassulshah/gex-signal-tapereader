// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2210.cpp -o t2210 && ./t2210 [dir]   (from plugin\tests_hodlod)
// lsHodLod 2.21.0 (2026-11-07, Rassul approved option B on the "HOD LOD Confirm Badge" mockup) - HodLodConfirm.h, SDK-free:
//  [1] PARITY: on real 3-min bars of all 7 markets (hl2210/bars_<MKT>.txt, CT END stamps, ticks) every badge equals the Python
//      study's (hl2210/expected_<MKT>.txt: day, side, extreme bar end, confirm bar end) - same count per market, same list
//  [2] NO REPAINT: replaying the bars one closed bar at a time, every badge, once it appears, keeps its extreme / confirm / price
//      in every later pass, and the final pass equals the one straight pass
//  [3] rule edges: a confirm needs BOTH the 0.25 ADR close and the 30-minute hold; a higher high starts a new HOD; ties keep the
//      first bar; 13 prior sessions = no badges; a roll day = no badges; a session cut by the first loaded bar is not in the ADR
//  [4] the pill: above the HOD / below the LOD, never on an obstacle or outside the pane, steps away / sideways, none clear = false
//  [5] record line format and versions 2.21.0
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include "../HodLodConfirm.h"
#include "../HodLodSettings.h"
#include "../HodLodLogic.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 40) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
struct MK { const char* m; int open, close; };
static const MK MKS[7] = { { "ES", 510, 900 }, { "NQ", 510, 900 }, { "CL", 480, 810 }, { "GC", 440, 750 }, { "HG", 440, 750 }, { "NG", 480, 810 }, { "EU", 440, 840 } };
static std::vector<hcf::Bar> loadBars(const std::string& p)
{ std::vector<hcf::Bar> B; std::ifstream f(p); hcf::Bar b; while (f >> b.absEnd >> b.h >> b.l >> b.c >> b.o) B.push_back(b); return B; }
static std::vector<std::string> lines(const std::vector<hcf::Session>& S, const std::vector<hcf::Bar>& B)
{ std::vector<std::string> out; for (const hcf::Session& s : S) for (const hcf::Badge& g : s.badges) out.push_back(hcf::key(s, B, g) + "," + hcf::hm(B[(size_t)g.conf].absEnd)); std::sort(out.begin(), out.end()); return out; }
static std::vector<hcf::Bar> day(long long d, int open, const std::vector<std::vector<long long>>& hlc)   // bars every 3 min from the open
{ std::vector<hcf::Bar> B; for (size_t i = 0; i < hlc.size(); i++) B.push_back(hcf::Bar{ d * 86400 + open * 60 + 180 * (long long)(i + 1), hlc[i][0], hlc[i][1], hlc[i][2], hlc[i][3] }); return B; }
int main(int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : "hl2210";
    std::printf("[1] parity with the Python study on real bars\n");
    for (const MK& k : MKS) {
        const std::vector<hcf::Bar> B = loadBars(dir + "/bars_" + k.m + ".txt");
        std::vector<std::string> exp; { std::ifstream f(dir + "/expected_" + k.m + ".txt"); std::string ln; while (std::getline(f, ln)) if (!ln.empty()) exp.push_back(ln); std::sort(exp.begin(), exp.end()); }
        const std::vector<hcf::Session> S = hcf::sessions(B, 180, k.open, k.close);
        const std::vector<std::string> got = lines(S, B);
        int nS = 0; for (const hcf::Session& s : S) if (s.usable) nS++;
        size_t same = 0; for (size_t i = 0; i < got.size() && i < exp.size(); i++) if (got[i] == exp[i]) same++;
        std::printf("  %s: bars %zu, sessions with badges allowed %d, badges C++ %zu / Python %zu, identical %zu\n", k.m, B.size(), nS, got.size(), exp.size(), same);
        CHECK(!B.empty() && got.size() == exp.size() && same == exp.size(), std::string(k.m) + ": C++ badges = Python badges");
        if (same != exp.size()) for (size_t i = 0; i < got.size() && i < exp.size(); i++) if (got[i] != exp[i]) { std::printf("   first diff C++ %s / Py %s\n", got[i].c_str(), exp[i].c_str()); break; }
        if (std::string(k.m) == "ES" || std::string(k.m) == "GC") {   // [2] no repaint on the last 6 sessions, bar by bar
            int from = 0, w0 = 0; { int seen = 0; for (int i = (int)B.size() - 1; i > 0; i--) if (hcf::dayOf(B[(size_t)i].absEnd) != hcf::dayOf(B[(size_t)i - 1].absEnd)) { ++seen; if (seen == 6) from = i; if (seen == 34) { w0 = i; break; } } }
            const std::vector<hcf::Bar> W(B.begin() + w0, B.end()); from -= w0;   // a 34-day window (the chart's history), replayed over its last 6 days
            const std::vector<std::string> straight = lines(hcf::sessions(W, 180, k.open, k.close), W);
            std::set<std::string> ever; bool stable = true; std::vector<std::string> fin;
            for (int n = from; n <= (int)W.size(); n += 1) {
                const std::vector<hcf::Bar> P(W.begin(), W.begin() + n);
                const std::vector<std::string> now = lines(hcf::sessions(P, 180, k.open, k.close), P);
                std::set<std::string> nowS(now.begin(), now.end());
                for (const std::string& e : ever) if (!nowS.count(e)) stable = false;
                ever.insert(now.begin(), now.end()); if (n == (int)W.size()) fin = now;
                if (n - from > 2600) break;
            }
            CHECK(stable, std::string(k.m) + ": no badge ever disappears or changes in a bar-by-bar replay");
            CHECK(!fin.empty() && fin == straight, std::string(k.m) + ": the replay's last pass = the straight pass (" + std::to_string(fin.size()) + " badges)");
            int inW = 0; for (const std::string& x : straight) if (std::binary_search(got.begin(), got.end(), x)) inW++;
            CHECK(inW == (int)straight.size(), std::string(k.m) + ": the 34-day chart window gives the same badges as the full history (" + std::to_string(inW) + " / " + std::to_string(straight.size()) + ")");
        }
    }
    std::printf("[3] rule edges\n");
    {   // 14 flat prior sessions of range 40 ticks -> ADR 40, threshold 10 ticks; Monday 2026-11-02 = day 20759
        std::vector<hcf::Bar> B; long long d0 = 20759 - 21;
        int made = 0; for (long long d = d0; made < 14; d++) { const int wd = hcf::weekday(d); if (wd == 0 || wd == 6) continue;
            std::vector<hcf::Bar> x = day(d, 510, { { 1040, 1000, 1020, 1020 }, { 1030, 1010, 1020, 1020 } }); B.insert(B.end(), x.begin(), x.end()); made++; }
        const long long D = 20759;
        // HOD at bar 0 (1050), closes back 10 ticks at bar 3 (9 min: no hold) ... first close >= 10 below after 30 min = bar 10
        std::vector<std::vector<long long>> t; t.push_back({ 1050, 1018, 1020, 1020 });
        for (int i = 1; i < 20; i++) t.push_back({ 1045, 1035, i < 3 ? 1044 : 1040, 1040 });
        std::vector<hcf::Bar> x = day(D, 510, t), A = B; A.insert(A.end(), x.begin(), x.end());
        std::vector<hcf::Session> S = hcf::sessions(A, 180, 510, 900);
        CHECK(S.size() == 15 && std::fabs(S.back().adrT - 40) < 1e-9, "ADR = mean of the 14 prior ranges (40 ticks)");
        int hodConf = -1; for (const hcf::Badge& g : S.back().badges) if (g.hod) hodConf = g.conf - S.back().first;
        CHECK(hodConf == 10, "HOD confirmed only once the 30-minute hold is met (bar END +30 min), not at the first 10-tick close (got " + std::to_string(hodConf) + ")");
        // a higher high at bar 5 starts a new HOD: the bar-0 HOD never confirms
        t[5] = { 1060, 1035, 1040, 1040 }; x = day(D, 510, t); A = B; A.insert(A.end(), x.begin(), x.end()); S = hcf::sessions(A, 180, 510, 900);
        int nH = 0, ext = -1; for (const hcf::Badge& g : S.back().badges) if (g.hod) { nH++; ext = g.ext - S.back().first; }
        CHECK(nH == 1 && ext == 5, "a higher high replaces the running HOD before it confirmed (one badge, on the new high)");
        // tie: the same high again at bar 6 keeps bar 5
        t[6] = { 1060, 1035, 1040, 1040 }; x = day(D, 510, t); A = B; A.insert(A.end(), x.begin(), x.end()); S = hcf::sessions(A, 180, 510, 900);
        ext = -1; for (const hcf::Badge& g : S.back().badges) if (g.hod) ext = g.ext - S.back().first;
        CHECK(ext == 5, "a tie keeps the FIRST bar that printed the high");
        // 13 prior sessions -> no badges; roll day -> no badges
        std::vector<hcf::Bar> C(B.begin() + 2, B.end()); C.insert(C.end(), x.begin(), x.end());
        S = hcf::sessions(C, 180, 510, 900); CHECK(S.back().badges.empty() && !S.back().usable && S.back().why.find("14") != std::string::npos, "13 prior sessions: no badges, status says why");
        std::vector<std::vector<long long>> r = t; for (auto& v : r) for (auto& p : v) p += 400;   // gap 10 ADR
        x = day(D, 510, r); A = B; A.insert(A.end(), x.begin(), x.end()); S = hcf::sessions(A, 180, 510, 900);
        CHECK(S.back().roll && S.back().badges.empty(), "roll day (gap > 4 ADR): no badges");
        // a session cut by the first loaded bar is not used for the ADR
        std::vector<hcf::Bar> E(B.begin() + 1, B.end()); E.insert(E.end(), x.begin(), x.end());   // first session now starts mid-RTH
        S = hcf::sessions(E, 180, 510, 900); CHECK(!S.back().usable, "a cut first session does not count toward the 14");
        // LOD mirror
        std::vector<std::vector<long long>> u; u.push_back({ 1022, 990, 1000, 1000 }); for (int i = 1; i < 14; i++) u.push_back({ 1005, 995, 1000, 1000 });
        for (int i = 10; i < 14; i++) u[(size_t)i][2] = 1002;
        x = day(D, 510, u); A = B; A.insert(A.end(), x.begin(), x.end()); S = hcf::sessions(A, 180, 510, 900);
        int lodConf = -1; long long lpx = 0; for (const hcf::Badge& g : S.back().badges) if (!g.hod) { lodConf = g.conf - S.back().first; lpx = g.px; }
        CHECK(lodConf == 10 && lpx == 990, "LOD: close >= 10 ticks above the low after 30 minutes (bar 10, price 990)");
    }
    std::printf("[4] the pill\n");
    {   const hcf::Px pane{ 0, 0, 800, 600 }; hcf::Px p;
        CHECK(hcf::pill(100, 200, 300, true, 30, 15, pane, {}, p) && p.b <= 300 - 4 && p.l == 85, "HOD pill above the extreme, centred on it");
        CHECK(hcf::pill(100, 200, 300, false, 30, 15, pane, {}, p) && p.t >= 304, "LOD pill below the extreme");
        std::vector<hcf::Px> ob{ hcf::Px{ 80, 270, 120, 300 } };
        CHECK(hcf::pill(100, 200, 300, true, 30, 15, pane, ob, p) && !hcf::hit(p, ob[0]), "an obstacle on the first spot: the pill steps away, never on it");
        std::vector<hcf::Px> all{ hcf::Px{ 0, 0, 800, 600 } };
        CHECK(!hcf::pill(100, 200, 300, true, 30, 15, pane, all, p), "no clear spot: no pill");
        CHECK(!hcf::pill(100, 200, 10, true, 30, 15, pane, {}, p) || p.t >= 0, "never above the pane");
        int bad = 0;   // 2,000 random scenes: every pill clear of every obstacle and inside the pane
        unsigned s = 7; auto rnd = [&](int n) { s = s * 1103515245u + 12345u; return (int)((s >> 8) % (unsigned)n); };
        for (int k = 0; k < 2000; k++) { std::vector<hcf::Px> o; for (int j = 0; j < 6; j++) { int l = rnd(760), t = rnd(560); o.push_back(hcf::Px{ l, t, l + 20 + rnd(200), t + 10 + rnd(60) }); }
            const int x = rnd(800), y = rnd(600);
            if (hcf::pill(x, x + rnd(200) - 100, y, rnd(2) == 0, 30, 15, pane, o, p)) { if (p.l < 0 || p.r > 800 || p.t < 0 || p.b > 600) bad++; for (const hcf::Px& q : o) if (hcf::hit(p, q)) bad++; } }
        CHECK(bad == 0, "2,000 random scenes: no pill outside the pane or on an obstacle (" + std::to_string(bad) + ")");
    }
    std::printf("[5] record line + versions\n");
    {   std::vector<hcf::Bar> B{ hcf::Bar{ 20764LL * 86400 + 8 * 3600 + 42 * 60, 27011, 26990, 27000, 27000 }, hcf::Bar{ 20764LL * 86400 + 9 * 3600 + 15 * 60, 27000, 26950, 26960, 26990 } };
        hcf::Session s; s.day = 20764; s.adrT = 341.2; const hcf::Badge g{ true, 0, 1, 27011 };
        const std::string ln = hcf::recordLine(s, B, g, 0.25, 2, "lsHodLod 2.21.0");
        CHECK(ln == "B,20261107,HOD,08:42,09:15,6752.75,85.30,lsHodLod 2.21.0\n", "record line: " + ln);
        CHECK(std::string(hl::HL_VERSION) == "2.21.0" && std::string(hlset::PLUGIN_VERSION) == "2.21.0" && std::string(hcf::VERSION) == "1.0.0", "versions 2.21.0 / HodLodConfirm 1.0.0");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
