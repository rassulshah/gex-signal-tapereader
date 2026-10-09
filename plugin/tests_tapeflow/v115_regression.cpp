// TapeFlow 1.1.5 regressions (fabricated data): quiet / sparse baseline windows, slot pooling, the adaptive QUIET rule,
// the header title, the fitted pane range, the store's traded-seconds field and the deep back-fill's complete-session helpers.
#include <vector>
#include <deque>
#include <map>
#include <string>
#include <cmath>
#include <climits>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <locale>
#include <set>
#include <unordered_set>
#include <iostream>
#define private public
#include "../TapeFlowLogic.h"
#undef private

using namespace tfl;
static int checks = 0, fails = 0;
#define CHECK(x, msg) do { ++checks; if (!(x)) { ++fails; std::printf("FAIL %d: %s\n", __LINE__, msg); } } while (0)

static Tick tk(long long t, int px, long long q = 1) { Tick k; k.t = t; k.px = px; k.bid = 998; k.ask = 1000; k.q = q; return k; }
static const long long D0 = 86400LL * 20000 + 17 * 3600;                 // a session start (17:00)
static Win win(long long endT, float rate, float act = 10)
{
    Win w; w.endT = endT; w.bin = binOf(endT); w.rate20 = rate; w.spread = 2; w.act = act;
    for (int h = 0; h < 8; ++h) { w.Mb[h] = rate * 5; w.Ms[h] = rate * 6; }
    for (int k = 0; k < 4; ++k) w.r5[k] = rate;
    return w;
}

int main()
{
    // 1. a thin tape (a trade every 7 s): 1.1.4 dropped every window (no trade within 5 s in every second); 1.1.5 keeps them
    {
        Store st; Engine e; e.attach(&st, "HG");
        for (long long t = D0; t < D0 + 600; t += 7) { e.advanceTo(t - 1); e.add(tk(t, (t / 7) % 2 ? 1000 : 998)); }
        e.advanceTo(D0 + 620); e.flushSession();
        const auto& v = st[sessionOf(D0)]["HG"];
        CHECK(v.size() >= 28, "thin tape: the 20-s windows are kept (1.1.4: none)");
        bool actOk = true; for (const Win& w : v) if (w.endT < D0 + 600 && !(w.act >= 2 && w.act <= 3)) actOk = false;
        CHECK(actOk, "each window records its traded seconds (2-3 of 20 at one trade every 7 s)");
    }
    // 2. a 20-s window with no trade at all counts (rate 0); a window with a broken quote does not
    {
        Store st; Engine e; e.attach(&st, "HG");
        e.add(tk(D0, 1000)); e.add(tk(D0 + 1, 998));
        e.advanceTo(D0 + 59); e.flushSession();
        const auto& v = st[sessionOf(D0)]["HG"];
        int zero = 0; for (const Win& w : v) if (w.rate20 == 0 && w.act == 0) zero++;
        CHECK(v.size() == 3 && zero == 2, "two quiet windows after the trades are observations of rate 0");
        Store st2; Engine b; b.attach(&st2, "HG");
        for (long long t = D0; t < D0 + 40; ++t) { Tick k = tk(t, 1000); if (t == D0 + 25) { k.bid = 1000; k.ask = 1000; } b.add(k); }
        b.advanceTo(D0 + 45); b.flushSession();
        const auto& w2 = st2[sessionOf(D0)]["HG"];
        CHECK(w2.size() == 1 && w2[0].endT == D0 + 19, "a locked / crossed quote in a window drops that window only");
    }
    // 3. pooling: a slot with < minWin windows borrows +-1, then +-2 slots; never more than maxPool
    {
        Store st; long long sid = sessionOf(D0) + 10;
        for (int d = 1; d <= 4; ++d) {
            long long s0 = (sid - d - 1) * 86400 + 17 * 3600;           // the session's 17:00
            for (int b = 100; b <= 104; ++b) for (int k = 0; k < (b == 102 ? 5 : 12); ++k) st[sid - d]["X"].push_back(win(s0 + b * 300 + 20 * k + 19, 10.f + b - 100));
        }
        BinBase out[NBINS]; int used = 0, dropped = 0;
        freezeBase(st, sid, 10, out, &used, std::string(), true, 60, 2, &dropped);
        CHECK(used == 4 && dropped == 0, "4 sessions used, none dropped");
        CHECK(out[102].own == 20 && out[102].k == 1 && out[102].n == 20 + 2 * 48, "slot 102: 20 own -> +-1 pooled = 116");
        CHECK(out[100].own == 48 && out[100].k == 1 && out[100].n == 48 + 48, "slot 100: +-1 (bin 99 is empty)");
        CHECK(out[101].k == 1 && out[101].n == 48 + 48 + 20, "slot 101");
        CHECK(out[106].own == 0 && out[106].k == 2 && out[106].n == 48, "slot 106: +-2 reaches slot 104 only");
        CHECK(out[110].n == 0 && out[110].k == 0, "a slot with no neighbours inside 2 is not pooled");
        BinBase plain[NBINS]; freezeBase(st, sid, 10, plain, &used);
        CHECK(plain[102].n == 20 && plain[102].k == 0, "no pooling when minWin is not given (1.1.4 call)");
        // the typical gap from traded seconds: act 10 of 20 -> 2 s between trades
        CHECK(std::fabs(out[102].gap - 2.0f) < 1e-4, "typical gap = 20 / mean traded seconds");
        // the session-quality filter: a session at < 0.3 x the typical rate is dropped and counted
        long long sx = sid - 5, s0 = (sx - 1) * 86400 + 17 * 3600;
        for (int k = 0; k < 30; ++k) st[sx]["X"].push_back(win(s0 + 102 * 300 + 20 * (k % 15) + 19 + (k / 15) * 300, 0.5f));
        freezeBase(st, sid, 10, out, &used, std::string(), true, 60, 2, &dropped);
        CHECK(used == 4 && dropped == 1, "a thin session (a holiday) is left out and counted");
    }
    // 4. median of mostly-quiet windows is 0 -> the mean is used, so the slot can calibrate
    {
        std::vector<float> v = {0, 0, 0, 4, 8};
        CHECK(std::fabs(typicalRate(v) - 2.4f) < 1e-5, "typicalRate: the mean when the median is 0");
        std::vector<float> w = {1, 2, 3};
        CHECK(typicalRate(w) == 2, "typicalRate: the median otherwise");
    }
    // 5. adaptive quote age / QUIET: the slot's typical gap x 3, at least quoteAge, at most maxQuoteAge
    {
        Engine e; BinBase b; b.n = 100; b.med20 = 1; b.med5 = 1; b.medSpread = 2; b.gap = 6; for (int h = 0; h < 8; ++h) b.qb[h] = b.qs[h] = 50;
        e.setFixedBase(b);
        CHECK(e.quoteAgeAt(D0) == 18, "gap 6 s -> quote current for 18 s");
        b.gap = 1; e.setFixedBase(b); CHECK(e.quoteAgeAt(D0) == 5, "ES-like gap 1 s -> 5 s (unchanged)");
        b.gap = 40; e.setFixedBase(b); CHECK(e.quoteAgeAt(D0) == 30, "capped at maxQuoteAge 30 s");
        b.gap = -1; e.setFixedBase(b); CHECK(e.quoteAgeAt(D0) == 5, "no slot gap and no live estimate -> 5 s");
        // a market trading every 4 s: 1.1.4 called it QUIET / no path most of the time; 1.1.5 (gap 4 -> 12 s) reads it
        b.gap = 4; e.setFixedBase(b); e.cfg.warm = 50;
        for (long long t = D0 + 3600; t < D0 + 3600 + 400; t += 4) { e.advanceTo(t - 1); e.add(tk(t, (t / 4) % 2 ? 1000 : 998, 3)); }
        e.advanceTo(D0 + 3600 + 399);
        StateInfo s = classify(e, true, false);
        CHECK(e.last.quoteOk && e.last.warm, "a trade every 4 s keeps the quote current and warms up");
        CHECK(s.code != "QUIET" && s.code != "NOQUOTE" && s.code != "NOSIDES", "and the state is a read, not QUIET");
        e.advanceTo(D0 + 3600 + 399 + 13);
        s = classify(e, true, false);
        CHECK(s.code == "QUIET" && s.sinceTrade > 12 && s.quietAfter == 12, "13+ s without a trade -> QUIET (allowance 12 s)");
        e.add(tk(D0 + 3600 + 399 + 300 + 5, 1000));
        e.advanceTo(D0 + 3600 + 399 + 300 + 6);
        CHECK(e.hist.back().t < D0 + 3600 + 399 + 300 + 5 || e.warmN <= 2, "5 minutes without a trade is still a hard stop (warm-up starts over)");
        // the same tape with the 1.1.4 rule (5 s) is QUIET
        Engine o; BinBase ob = b; ob.gap = -1; o.setFixedBase(ob); o.cfg.warm = 50; o.cfg.quietMult = 0;
        for (long long t = D0 + 3600; t < D0 + 3600 + 400; t += 7) { o.advanceTo(t - 1); o.add(tk(t, 1000, 3)); }
        o.advanceTo(o.lastTrade() + 6);
        CHECK(classify(o, true, false).code == "QUIET" && o.last.quoteAgeEff == 5, "quietMult 0 = the 1.1.4 5-s rule: QUIET 6 s after a trade");
    }
    // 6. live typical gap: the mean spacing of the last traded seconds, used when the slot has none
    {
        Engine e; BinBase b; b.n = 100; b.med20 = 1; b.med5 = 1; b.medSpread = 2; e.setFixedBase(b);
        for (long long t = D0; t < D0 + 200; t += 3) { e.advanceTo(t - 1); e.add(tk(t, 1000)); }
        bool fromBase = true; double g = e.typicalGap(D0 + 200, &fromBase);
        CHECK(!fromBase && std::fabs(g - 3.0) < 1e-9 && e.quoteAgeAt(D0 + 200) == 9, "live gap 3 s -> 9 s");
    }
    // 7. the header title and the fitted range
    {
        StateInfo s; s.code = "READY"; s.act = 1.23;
        CHECK(shortTitle("1.1.5", "GC", s) == "TF 1.1.5 GC  READY  act 1.2x", "title READY");
        s.code = "CAL"; s.n = 51; s.need = 60;
        CHECK(shortTitle("1.1.5", "GC", s) == "TF 1.1.5 GC  CAL 51/60", "title CAL");
        s.code = "QUIET";
        CHECK(shortTitle("1.1.5", "GC", s) == "TF 1.1.5 GC  QUIET", "title QUIET");
        s.code = "WATCH"; s.dir = -1; s.left = 42;
        CHECK(shortTitle("1.1.5", "HG", s) == "TF 1.1.5 HG  WATCH- 42s", "title WATCH");
        s.code = "SIGNAL"; s.dir = 1; s.kind = "AR";
        CHECK(shortTitle("1.1.5", "ES", s) == "TF 1.1.5 ES  AR+?", "title SIGNAL keeps '?' until proven");
        s.code = "LOWACT"; s.act = 0.3;
        CHECK(shortTitle("1.1.5", "ES", s) == "TF 1.1.5 ES  LOW ACT  act 0.3x", "title LOW ACT");
        CHECK(fitRange(0) == 60 && fitRange(40) == 60 && fitRange(53) == 70 && fitRange(100) == 120 && fitRange(NAN) == 60, "range = max(60, 1.15 x max, up to 10)");
    }
    // 8. the store line: traded seconds appended (field 27), still read by 1.1.4-style W2 readers' indices, both forms parse
    {
        Win w = win(D0 + 19, 3.5f, 7);
        std::string ln = storeLine(sessionOf(D0), "GCEZ26", w);
        size_t bars = (size_t)std::count(ln.begin(), ln.end(), '|');
        CHECK(bars == 27 && ln.substr(ln.rfind('|') + 1) == "7", "W2 + |act (28 fields; fields 0-26 unchanged)");
        long long sid; std::string sy; Win r;
        CHECK(parseStoreLine(ln, &sid, &sy, &r) && r.act == 7 && r.rate20 == 3.5f && sy == "GCEZ26", "W2+act parses");
        std::string old = ln.substr(0, ln.rfind('|'));
        CHECK(parseStoreLine(old, &sid, &sy, &r) && r.act == -1, "a 1.1.4 W2 line parses (act unknown)");
        CHECK(!parseStoreLine(old + "|21", &sid, &sy, &r) && !parseStoreLine(old + "|-1", &sid, &sy, &r), "act outside 0..20 is rejected");
        // merging: the first measurement wins, but a missing act is filled in
        std::vector<Win> dst = {r}; std::vector<Win> src = {w};
        mergeWindows(dst, src);
        CHECK(dst.size() == 1 && dst[0].act == 7, "merge fills a missing act, keeps one window");
    }
    // 9. complete sessions for the deep back-fill
    {
        Store st; long long sid = 20735;                         // Friday 2026-10-09
        CHECK(tradingWeekday(20735) && tradingWeekday(20731) && !tradingWeekday(20733 - 4) && !tradingWeekday(20729), "weekdays: Fri / Mon trade, Sat / Sun do not");
        auto fill = [&](long long s, size_t n) { for (size_t k = 0; k < n; ++k) st[s]["X"].push_back(win((s - 1) * 86400 + 17 * 3600 + 20 * (long long)k + 19, 5)); };
        fill(20734, 3000); fill(20733, 3000); fill(20732, 900); fill(20731, 3000);
        CHECK(completeSessions(st, sid, 2400) == 3, "3 complete (one 1.1.4-sized session is not)");
        CHECK(completeRunStart(st, sid, 2400) == 20733, "the run of complete sessions starts Wednesday (Tuesday is incomplete)");
        fill(20732, 3000);
        CHECK(completeRunStart(st, sid, 2400) == 20731, "complete Mon-Thu -> the run starts Monday");
        fill(20728, 3000);
        CHECK(completeRunStart(st, sid, 2400) == 20728, "the weekend does not break the run");
        CHECK(completeRunStart(st, 20731, 2400) == 20728, "from Monday the run reaches back over the weekend");
    }
    std::printf("v115 regression: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
