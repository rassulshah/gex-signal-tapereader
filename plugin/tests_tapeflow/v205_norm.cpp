// (2.0.5) the robust normal minute, the provisional effort floor and the "10x+" cap - on REAL HG and GC Friday data
// (the plugin's own tape export of session 20735, used here only as TEST DATA; the plugin itself reads IRT's trades):
//  - HG 14:03 / 15:30 (2.0.4 drew "A 24.2x" / "A 32.0x" off a 5-contract fallback normal): the robust normal = max(slot normal from
//    the baselines | last hour, the session's median one-side minute) -> the multiples and their labels
//  - the whole Friday decided again with 2.0.5's rules: no A? under 1.5x ("weak effort" logged), labels capped at "10x+", GC's 0.85x gone
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <cmath>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "../TapeFlowLogic.h"
using namespace tfl;
static int checks = 0, fails = 0;
#define CHECK(x, msg) do { ++checks; if (!(x)) { ++fails; std::printf("FAIL %d: %s\n", __LINE__, msg); } } while (0)
static const std::string D = "/mnt/user-data/uploads/lsFlexLevels/TapeFlow/";
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> c; size_t a = 0; while (true) { size_t p = s.find('|', a); c.push_back(s.substr(a, p == std::string::npos ? p : p - a)); if (p == std::string::npos) break; a = p + 1; } return c; }
struct Day { std::vector<BarIn> bars; std::vector<SecRec> H; BarRows R; bool ok = false; };
static Day load(const std::string& m)
{
    Day d; d.R.spb = 180;
    std::ifstream b((D + m + "-bars3-20735.csv").c_str()), t((D + m + "-tape-20735.csv").c_str()); if (!b.good() || !t.good()) return d;
    std::string ln; long long x = 0; int v = 0;
    while (std::getline(b, ln)) { if (ln.empty() || !isdigit((unsigned char)ln[0])) continue; auto c = split(ln); if (c.size() < 5) continue; BarIn bi;
        if (!parseStoreLong(c[0], &bi.te)) continue; bi.ts = bi.te - 180; parseStoreInt(c[1], &bi.o); parseStoreInt(c[2], &bi.h); parseStoreInt(c[3], &bi.l); parseStoreInt(c[4], &bi.c); d.bars.push_back(bi); }
    while (std::getline(t, ln)) { if (ln.empty() || !isdigit((unsigned char)ln[0])) continue; auto c = split(ln); if (c.size() < 5) continue;
        long long ts = 0, bu = 0, se = 0, un = 0; if (!parseStoreLong(c[0], &ts) || !parseStoreInt(c[1], &v) || !parseStoreLong(c[2], &bu) || !parseStoreLong(c[3], &se) || !parseStoreLong(c[4], &un)) continue;
        if (d.H.empty() || d.H.back().t != ts) { SecRec r; r.t = ts; r.flags = SR_SIDES; d.H.push_back(r); }
        d.H.back().b += bu; d.H.back().s += se; d.H.back().u += un;
        Tick k; k.t = ts; k.px = v; if (bu > 0) { k.q = bu; k.bid = v - 1; k.ask = v; d.R.add(k); } if (se > 0) { k.q = se; k.bid = v; k.ask = v + 1; d.R.add(k); } (void)x; }
    d.ok = !d.bars.empty() && !d.H.empty();
    return d;
}
static void slotNorms(const std::string& m, const std::string& sym, BinBase* base)
{
    Store st;
    for (long long s = 20736; s >= 20715; --s) { std::ifstream f((D + m + "-base-v110-" + std::to_string(s) + ".csv").c_str()); std::string ln;
        while (std::getline(f, ln)) { long long x; std::string sy; Win w; if (parseStoreLine(ln, &x, &sy, &w) && x == s) st[s][sy].push_back(w); } }
    Cfg cfg; int used = 0, dropped = 0; freezeBase(st, 20736, cfg.sessionsBack, base, &used, sym, cfg.frontRoll, cfg.minWin, cfg.maxPool, &dropped);
}
static std::string hm(long long t) { char b[8]; snprintf(b, sizeof(b), "%02d:%02d", (int)(t % 86400 / 3600), (int)(t % 3600 / 60)); return b; }
// the whole session through the 2.0.5 book (the plugin's nodeFor: nodeN from the median bar range / 10, robust normal)
static SignalBook decide(const Day& d, const BinBase* base, int* below, double* maxMult)
{
    SignalBook bk; bk.cfg.absorbMethod = 1; bk.version = "2.0.5"; const long long sessStart = (20735 - 1) * 86400LL + 17 * 3600;
    *below = 0; *maxMult = 0;
    for (size_t i = 0; i < d.bars.size(); ++i) {
        const BarIn& b = d.bars[i]; BarFlow fl; fl.secs = 180;
        std::vector<int> rg; for (size_t j = (i > 300 ? i - 300 : 0); j < i; ++j) rg.push_back(d.bars[j].h - d.bars[j].l);
        int nodeN = 1; if (!rg.empty()) { std::sort(rg.begin(), rg.end()); nodeN = std::max(1, (int)std::lround(rg[rg.size() / 2] / 10.0)); }
        nodeFill(fl, d.R.at(b.te), b, nodeN);
        nodeMinuteFill(fl, d.R, b, d.H, robustMinuteNorm(d.H, sessStart, b.ts, base[binOf(b.ts)].minNorm));
        bk.feed(b, fl);
    }
    for (auto& m : bk.marks) if (m.drawn()) { if (m.mult < 1.5) (*below)++; *maxMult = std::max(*maxMult, m.mult); }
    return bk;
}
int main()
{
    CHECK(markLabel([] { Mark m; m.kind = 'A'; m.state = MK_CONFIRMED; m.mult = 24.2; return m; }()) == "A 10x+", "(cap) 24.2x reads 'A 10x+'");
    CHECK(markLabel([] { Mark m; m.kind = 'A'; m.mult = 9.94; return m; }()) == "A? 9.9x", "(cap) under 10x the multiple is shown");
    const Day hg = load("HG");
    if (!hg.ok) { std::printf("SKIPPED - HG tape / bars of 20735 not here\nv205: %d checks, %d failed\n", checks, fails); return fails ? 1 : 0; }
    static BinBase hb[NBINS]; slotNorms("HG", "CPEZ26", hb);
    const long long sessStart = (20735 - 1) * 86400LL + 17 * 3600;
    std::printf("HG Friday: session median one-side minute (whole day) %.1f\n", sessionMinuteMedian(hg.H, sessStart, sessStart + 86400));
    struct Case { long long te; long long vol; double old; } cs[] = { {1791554580, 121, 5.0}, {1791559800, 160, 5.0}, {1791525600, 79, 7.0} };
    for (auto& k : cs) {
        const long long ts = k.te - 180; const double slot = hb[binOf(ts)].minNorm, last = recentMinuteNorm(hg.H, ts), sm = sessionMinuteMedian(hg.H, sessStart, (ts / 60) * 60), rn = robustMinuteNorm(hg.H, sessStart, ts, slot);
        Mark m; m.kind = 'A'; m.state = MK_CONFIRMED; m.mult = k.vol / rn;
        std::printf("HG bar %s: node minute %lld contracts; 2.0.4 normal %.1f (%.1fx) -> slot %.1f, last hour %.1f, session so far %.1f -> normal %.1f = %.1fx, label '%s'\n",
            hm(k.te).c_str(), k.vol, k.old, k.vol / k.old, slot, last, sm, rn, k.vol / rn, markLabel(m).c_str());
        CHECK(rn >= sm && rn >= std::max(slot, -1.0) && rn > k.old, "(1) the robust normal is at least the session's median minute and the slot normal");
    }
    int below = 0; double mx = 0;
    SignalBook b = decide(hg, hb, &below, &mx);
    int nA = 0; for (auto& m : b.marks) if (m.drawn()) nA++;
    auto rj = [&](const SignalBook& x, const char* k) { auto it = x.rejected.find(k); return it == x.rejected.end() ? 0L : it->second; };
    std::printf("HG Friday with 2.0.5: %d node A (2.0.4 drew 30), weak effort (< 1.5x) rejected %ld, close not beyond node %ld, not at pivot %ld; largest multiple %.1f\n",
        nA, rj(b, "weak effort"), rj(b, "close not beyond node"), rj(b, "not at pivot"), mx);
    CHECK(below == 0 && rj(b, "weak effort") > 0, "(2) no A under 1.5x a normal minute; the weak ones are logged");
    const Day gc = load("GC");
    if (gc.ok) {
        static BinBase gb[NBINS]; slotNorms("GC", "GCEZ26", gb);
        int gbelow = 0; double gmx = 0; SignalBook g = decide(gc, gb, &gbelow, &gmx); int gA = 0; for (auto& m : g.marks) if (m.drawn()) gA++;
        std::printf("GC Friday with 2.0.5: %d node A (2.0.4 drew 17 incl. 'A? 0.9x'), weak effort rejected %ld; largest %.1f, any under 1.5x: %d\n", gA, rj(g, "weak effort"), gmx, gbelow);
        CHECK(gbelow == 0, "(2) GC: no A under 1.5x (the 0.85x is gone)");
    }
    std::printf("v205: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
