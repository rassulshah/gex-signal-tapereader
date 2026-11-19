// t_reclaim.cpp -- WyRead 1.1.0: the I = a reclaim of a swept level
#include "../WyRead.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m); } }
static wyr::Bar bar(long long o, long long h, long long l, long long c, long long buy, long long sell, int k) { wyr::Bar b; b.t = 1000 + k * 180; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = buy; b.sell = sell; b.when = std::to_string(k); return b; }
static bool hasI(const wyr::Out& o, long long t, int dir) { auto it = o.marks.find(t); if (it == o.marks.end()) return false; for (auto& m : it->second) if (m.k == 'I' && m.dir == dir) return true; return false; }
// a flat range, a pivot low at 100 (bar 8), a bounce, then a lower low (bar 16) that sweeps 100, then the I candle
static std::vector<wyr::Bar> base(long long lowSweep, long long iClose, long long iBuy) {
    std::vector<wyr::Bar> B; int k = 0;
    for (; k < 6; ++k) B.push_back(bar(110, 112, 108, 110, 50, 50, k));
    B.push_back(bar(110, 110, 104, 105, 30, 60, k++)); B.push_back(bar(105, 106, 102, 103, 30, 60, k++)); B.push_back(bar(103, 104, 100, 101, 30, 60, k++));   // down to the 100 pivot low (bar 8)
    B.push_back(bar(101, 105, 101, 104, 60, 30, k++)); B.push_back(bar(104, 108, 103, 107, 60, 30, k++)); B.push_back(bar(107, 109, 105, 108, 60, 30, k++));   // bounce (pivot low confirmed)
    B.push_back(bar(108, 108, 104, 105, 30, 60, k++)); B.push_back(bar(105, 106, 102, 103, 30, 60, k++)); B.push_back(bar(103, 104, 101, 102, 30, 60, k++));
    B.push_back(bar(102, 102, 99, 100, 30, 60, k++)); B.push_back(bar(100, 101, lowSweep, 99, 30, 60, k++));   // bar 16: the sweep of 100
    B.push_back(bar(99, 100, 98, 99, 40, 40, k++));
    B.push_back(bar(99, iClose + 1, 98, iClose, iBuy, 20, k++));                                                 // bar 18: the I candidate
    for (int j = 0; j < 3; ++j) B.push_back(bar(iClose, iClose + 2, iClose - 1, iClose + 1, 50, 50, k++));
    return B;
}
int main() {
    { auto B = base(97, 102, 400); auto o = wyr::run(B, 0.25, 2); C(hasI(o, B[18].t, 1), "swept 100 by 3 ticks, closed back above it at 102 with heavy buying = I"); }
    { auto B = base(97, 99, 400); auto o = wyr::run(B, 0.25, 2); C(!hasI(o, B[18].t, 1), "no close back above 100 = no I"); }
    { auto B = base(100, 102, 400); for (auto& b : B) if (b.l < 100) b.l = 100; auto o = wyr::run(B, 0.25, 2); C(!hasI(o, B[18].t, 1), "a touch of 100, never through it = no sweep = no I"); }
    { auto B = base(97, 102, 400); std::vector<wyr::Lvl> lv(1); lv[0].px = 101; lv[0].name = "VAH"; auto o = wyr::run(B, 0.25, 2, wyr::Params(), lv); C(hasI(o, B[18].t, 1), "key level 101 swept and reclaimed = I");
      auto r = o.reads.find(B[18].t); C(r != o.reads.end() && r->second.lines.size() > 1 && r->second.lines[1].find("VAH") != std::string::npos, "the read names the furthest reclaimed level (VAH 101 over the 100 low)"); }
    { auto B = base(97, 102, 60); auto o = wyr::run(B, 0.25, 2); C(!hasI(o, B[18].t, 1), "weak buying (not 2x delta) = no I"); }
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
