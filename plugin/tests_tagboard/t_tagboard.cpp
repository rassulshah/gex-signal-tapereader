// t_tagboard.cpp -- SignalBoard 1.6.0 kinds Z / K and tagEv / tagBox (lsZzoComboMarks 3.0.4, lsCobSignal 1.9.2, lsRevReadMarks 0.19.1)
#include "../SignalBoard.h"
#include "../WyMarks.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m); } }
static bool hit(int l1, int t1, int r1, int b1, int l2, int t2, int r2, int b2) { return !(r1 < l2 || l1 > r2 || b1 < t2 || t1 > b2); }
int main() {
    C(std::string(sb::prefix('Z')) == "LRA_SGZ_" && std::string(sb::prefix('K')) == "LRA_SGK_" && std::string(sb::prefix('L')) == "LRA_SGL_", "own prefixes for Z / K / L");
    C(std::string(sb::prefix('Z')) != sb::prefix('F') && std::string(sb::prefix('K')) != sb::prefix('F'), "Z / K no longer share F's key");
    // a CB badge over a candle: candle high at y 100, low at y 140 (pixels grow down); badge 16 px above the high, 30 px wide
    sb::Ev e; C(sb::tagEv(1000, 100, 140, 85, 80, 115, 96, e), "tagEv ok");
    C(e.side == 1 && (int)e.val == 20 && (int)e.px == 30, "over the candle: side +1, 20 px from the high to the outer edge, 30 px wide");
    int l, t, r, b; sb::tagBox(e, 100, 100, 140, l, t, r, b);
    C(l == 84 && r == 116 && t == 80 && b == 100, "tagBox gives the same box back (from the high to the outer edge)");
    sb::Ev u; C(sb::tagEv(1000, 100, 140, 90, 144, 110, 158, u) && u.side == -1 && (int)u.val == 18, "under the candle: side -1, 18 px from the low");
    sb::tagBox(u, 100, 100, 140, l, t, r, b); C(t == 140 && b == 158, "tagBox under the candle");
    sb::Ev z; z.side = 1; z.val = 20; z.px = 0; sb::tagBox(z, 100, 100, 140, l, t, r, b); C(l == 86 && r == 114, "unknown width = 14 px each side (the BO 'L' events)");
    C(!sb::tagEv(0, 100, 140, 85, 80, 115, 96, e) && !sb::tagEv(1000, 100, 140, 85, 80, 85, 96, e), "a bad stamp / an empty box is not posted");
    // publish / read round trip, each kind on its own key
    std::vector<sb::Ev> vz(1, e), vk(1, u), got;
    sb::publish('Z', "ESZ6", 180, "c1", 5000, vz); sb::publish('K', "ESZ6", 180, "c1", 5000, vk);
    C(sb::readAll('Z', "ESZ6", 180, "c1", 6000, got) && got.size() == 1 && got[0].side == 1 && (int)got[0].px == 30, "Z round trip (width kept)");
    C(sb::readAll('K', "ESZ6", 180, "c1", 6000, got) && got.size() == 1 && got[0].side == -1, "K round trip, not overwritten by Z");
    C(!sb::readAll('Z', "ESZ6", 180, "c1", 5000 + 61000, got), "older than 60 s = ignored");
    C(!sb::readAll('Z', "NQZ6", 180, "c1", 6000, got), "another symbol = nothing");
    // the marks place around it: one candle, a green candle's I + Bk tag wanting the space over the high that the CB badge holds
    std::vector<wyi::Candle> Cd(1); Cd[0].x = 100; Cd[0].yHigh = 100; Cd[0].yLow = 140; Cd[0].halfW = 4; Cd[0].green = true;
    std::vector<wyi::Want> W(1); W[0].candle = 0; W[0].w = 24; W[0].h = wyi::LINE1_H + wyi::LINE2_H;
    std::vector<wyi::Box> taken(1); sb::tagBox(e, 100, 100, 140, taken[0].l, taken[0].t, taken[0].r, taken[0].b);
    const std::vector<wyi::Box> P = wyi::layout(Cd, W, 0, 400, 300, taken);
    C(P.size() == 1 && P[0].ok, "the Bk tag is placed");
    C(P.size() == 1 && !hit(P[0].l, P[0].t, P[0].r, P[0].b, taken[0].l, taken[0].t, taken[0].r, taken[0].b), "and never covers the CB badge");
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
