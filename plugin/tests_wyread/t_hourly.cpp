// t_hourly.cpp -- WyRead 1.2.0: the I reclaims a prior HOUR's low / high the pivot swept (the HTF breakout failure)
#include "../WyRead.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m); } }
static const long long T0 = 3600LL * 26;   // 2 AM of day 1
static wyr::Bar bar(long long o, long long h, long long l, long long c, long long buy, long long sell, int k) { wyr::Bar b; b.t = T0 + (k + 1) * 180; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = buy; b.sell = sell; b.when = std::to_string(k); return b; }
static bool hasI(const wyr::Out& o, long long t, int dir) { auto it = o.marks.find(t); if (it == o.marks.end()) return false; for (auto& m : it->second) if (m.k == 'I' && m.dir == dir) return true; return false; }
static std::string line1(const wyr::Out& o, long long t) { auto r = o.reads.find(t); return r != o.reads.end() && r->second.lines.size() > 1 ? r->second.lines[1] : ""; }
// the 2 AM hour drifts down to a low of 100 on its last candle (no pivot low yet); 3 AM: a sweep under it, then the I candle
static std::vector<wyr::Bar> down(long long lowSweep, long long iClose, long long iBuy, long long startOff = 0) {
    std::vector<wyr::Bar> B; int k = 0;
    for (; k < 20; ++k) { long long hi = 140 - 2 * k, lo = hi - 3; if (k == 19) lo = 100; B.push_back(bar(hi, hi, lo, lo + 1, 40, 60, k)); }
    B.push_back(bar(101, 102, 100, 101, 40, 60, k++)); B.push_back(bar(101, 101, lowSweep, lowSweep + 1, 30, 70, k++));   // 3 AM: the sweep (bar 21)
    B.push_back(bar(lowSweep + 1, iClose + 1, lowSweep + 1, iClose, iBuy, 20, k++));                                       // bar 22: the I candidate
    for (int j = 0; j < 3; ++j) B.push_back(bar(iClose, iClose + 2, iClose - 1, iClose + 1, 50, 50, k++));
    if (startOff) for (auto& b : B) b.t += startOff;
    return B;
}
int main() {
    { auto B = down(97, 102, 400); auto o = wyr::run(B, 0.25, 2); C(hasI(o, B[22].t, 1), "swept the 2 AM hour low 100, closed back above at 102 = I");
      C(line1(o, B[22].t).find("the 2 AM hour low") != std::string::npos, "the read names the 2 AM hour low"); }
    { auto B = down(97, 99, 400); auto o = wyr::run(B, 0.25, 2); C(!hasI(o, B[22].t, 1), "closed under the hour low = no I"); }
    { auto B = down(97, 102, 400, 900); auto o = wyr::run(B, 0.25, 2); bool any = false; for (auto& kv : o.marks) for (auto& m : kv.second) if (m.k == 'I' && m.dir == 1) any = true;
      C(!any || line1(o, B[22].t).find("hour low") == std::string::npos, "an hour the candles only partly cover is not a level"); }
    { auto B = down(97, 102, 400); for (auto& b : B) { long long o = b.o, h = b.h, l = b.l, c = b.c; b.o = 300 - o; b.h = 300 - l; b.l = 300 - h; b.c = 300 - c; std::swap(b.buy, b.sell); }
      auto o = wyr::run(B, 0.25, 2); C(hasI(o, B[22].t, -1), "mirror: swept the 2 AM hour high, closed back under = bearish I");
      C(line1(o, B[22].t).find("the 2 AM hour high") != std::string::npos, "the read names the 2 AM hour high"); }
    C(wyr::hourName(26) == "2 AM" && wyr::hourName(24) == "12 AM" && wyr::hourName(36) == "12 PM" && wyr::hourName(15) == "3 PM", "hour names");
    C(wyr::hourOf(3600 * 3) == 2 && wyr::hourOf(3600 * 3 + 180) == 3, "the 3:00 candle END belongs to the 2 AM hour");
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
