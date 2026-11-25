// t_vr180.cpp -- VolRangeLogic 1.8.0: the panes' Bk / Sw come from the price chart's marks file (wy), count from 10 on Bk / Sw bars
#include "VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int ok = 0, bad = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++bad; std::printf("FAIL %s\n", m); } }
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int idx(const vrr::Engine& E, long long localEnd) { for (size_t i = 0; i < E.LT.size(); ++i) if (E.LT[i] == localEnd) return (int)i; return -1; }
int main() {
    // 2026-11-19 00:00 as unix-naive local
    const long long day = 1795046400LL;   // 2026-11-19 00:00:00
    for (int mode = 0; mode < 2; ++mode) {
        vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180;
        std::ifstream f("nq_1119.csv"); std::string l; int n = 0;
        while (std::getline(f, l)) { vrr::Bar b; long long L = 0; if (std::sscanf(l.c_str(), "%lld,%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &L, &b.o, &b.h, &b.l, &b.c, &b.v) != 7) continue; E.addBar(b, L); ++n; }
        C(n > 300, "bars loaded");
        E.hlOn = true;
        std::map<long long, vrr::Mk> w; const int k = vrr::parseWy(slurp("wy_NQ_1119.txt"), w); C(k > 10, "wy parsed");
        E.wy.swap(w); E.wyLines = k; E.markLevels();
        const int i1739 = idx(E, day + 17 * 3600 + 39 * 60), i1736 = idx(E, day + 17 * 3600 + 36 * 60), i1800 = idx(E, day + 18 * 3600), i1054 = idx(E, day + 10 * 3600 + 54 * 60);
        C(i1739 > 0 && i1736 > 0 && i1800 > 0, "bars found");
        const vrr::Mk a = E.markOf(i1739), b2 = E.markOf(i1736), c = E.markOf(i1800);
        C(a.code == vrr::MK_SW && a.dir == 1, "5:39 PM = Sw (bullish), as the price chart");
        C(b2.code == vrr::MK_BK && b2.dir == -1, "5:36 PM = Bk (bearish), as the price chart");
        C(c.code != vrr::MK_SW && c.code != vrr::MK_BK, "6:00 PM is NOT a Sw / Bk (the price chart has none)");
        if (mode == vrr::VOL) C(E.S[(size_t)i1739].mx() == 15, "5:39 PM volume Max = 15 (shown: >= 10 on a Sw bar)");
        C(E.S[(size_t)i1739].mx() >= vrr::SHOW_MIN_LV, "5:39 PM count passes the Bk / Sw threshold of 10");
        // every Bk / Sw of the file on a loaded bar is marked, and nothing else is a Bk / Sw
        int match = 0, extra = 0;
        for (size_t i = 0; i < E.T.size(); ++i) { const vrr::Mk m = E.markOf((int)i); const bool inWy = E.wy.count(E.T[i]) > 0;
            if (inWy && (m.code == vrr::MK_BK || m.code == vrr::MK_SW || m.code == vrr::MK_CB)) ++match;
            if (!inWy && (m.code == vrr::MK_BK || m.code == vrr::MK_SW)) ++extra; }
        C(match > 5, "the file's Bk / Sw candles are marked"); C(extra == 0, "no Bk / Sw the price chart does not have");
        (void)i1054;
        // no file -> no Bk / Sw
        E.wy.clear(); E.wyLines = -1; int any = 0; for (size_t i = 0; i < E.T.size(); ++i) { const int cd = E.markOf((int)i).code; if (cd == vrr::MK_BK || cd == vrr::MK_SW) ++any; }
        C(any == 0, "no wy file = no Bk / Sw (not the old rule)");
        const std::string st = vrr::statusText(E, "NQ", "NQZ6", 180); C(st.find("WY,1.8.0") != std::string::npos && st.find("no file") != std::string::npos, "status says no file");
    }
    // parser: garbage / torn
    { std::map<long long, vrr::Mk> w; C(vrr::parseWy("garbage", w) == -1 && w.empty(), "no header = not parsed (keep the last good)"); }
    { std::map<long long, vrr::Mk> w; C(vrr::parseWy("WY|1|NQ|180|0.20.0\n123|Bk|1|PDL|100|V20\n124|Sw|2|x|1|\n125|Tst|-1|PDL|0|62%\nabc|Sw|1\n126|Sw|-1|PDH|5|", w) == 2 && w.size() == 2, "Bk / Sw only, bad lines dropped"); }
    C(std::string(vrr::VERSION) == "1.16.0", "version");   // (1.10.0) Cx from the marks file too (t_vr1100); (1.11.0) the AR (t_vr1110)
    std::printf("%s %d pass, %d fail\n", bad ? "FAILED" : "ALL PASS", ok, bad); return bad;
}
