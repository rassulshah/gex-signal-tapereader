// (2.0.3) the REAL TapeFlow.cpp (TapeFlowES, bar-node A, drawing ON) against mockirt: the 1-minute tape pane, its alignment and two charts.
//  (1) minute windows: the engine's per-minute bought / sold over [end-180+60k, end-180+60(k+1)) equal the trades in that window, the
//      3 minutes add up to the chart bar's volume ([end-180, end), bar time = the bar END), and every drawn minute bar's height is
//      exactly that minute's contracts on the pane scale
//  (2) pixels at 3 zooms (narrow / normal / wide): every minute bar inside its candle's x-range, the minutes in time order with a
//      gap, no overlap with the next candle; every A arrowhead's x inside the x-range of the minute where its node traded
//  (3) the dashed grey normal-minute line at +- the normal minute; only green / red / grey are drawn; the header says TESTING
//  (4) two charts on ONE object (ES 3-min + ES 60-min): separate states, separate records and status files, ONE writer of the
//      market files (no duplicate second), a timer fired in the other chart's context still runs its own chart, removing the writer
//      chart hands the writing to the other one without a duplicate or a lost second, a dead chart's timer runs nothing
#include "mockirt.h"
#include <vector>
std::vector<float> mockOut(int k);
void mockBar(int i, float o, float h, float l, float c);
#define TF_TESTING_MODE 0
#define TF_ABSORB_METHOD 1
#include "TapeFlowES.cpp"
#undef mkt
#undef root
#undef sym
#undef err
#undef tick
#undef eng
#undef store
#undef storeLoaded
#undef cur
#undef timerOn
#undef timerRefused
#undef busy
#undef firstPass
#undef lastTimerTick
#undef timerAt
#undef lastStepAt
#undef lastSave
#undef lastEvWrite
#undef lastStatus
#undef lastFallback
#undef step
#undef blocked
#undef guardsRead
#undef stepNote
#undef evWritten
#undef evLastT
#undef written
#undef keysLoaded
#undef savedDigest
#undef liveTicks
#undef passes
#include <random>
#include <set>
#include <fstream>
#include <iostream>
#include <chrono>
static int fails = 0, npass_ = 0;
#define CHECK(c, m) do { if (c) npass_++; else { fails++; printf("FAIL %d: %s\n", __LINE__, m); } } while (0)
static const long long DAY0 = 1791417600LL;               // 2026-10-08 00:00 "local"
static std::vector<MTick> dayTape(long long from, long long to, unsigned seed, float px0)
{
    std::mt19937 r(seed); std::vector<MTick> v; int p = (int)(px0 / 0.25f); int attack = 0, dirA = 1, trend = 0;
    for (long long t = from; t < to; t++) {
        int hod = (int)(((t % 86400) + 86400) % 86400 / 3600);
        if (hod == 16) continue;
        bool rth = hod >= 8 && hod < 15;
        if (t % 1200 == 0) trend = (int)(r() % 3) - 1;
        if (attack <= 0 && r() % (rth ? 400 : 1500) == 0) { attack = 25 + (int)(r() % 20); dirA = r() % 2 ? 1 : -1; }
        if (attack <= 0 && r() % 5 == 0) p += (int)(r() % 3) - 1 + (r() % 4 == 0 ? trend : 0);
        int n = rth ? 4 + (int)(r() % 6) : (r() % 3 ? 1 : 0);
        for (int k = 0; k < n; k++) {
            float bid = p * 0.25f, ask = (p + 1) * 0.25f; bool buy = (int)(r() % 100) < 50 + 25 * trend;
            long q = 1 + (long)(r() % 4);
            if (r() % 13 == 0) { v.push_back(MTick{t, (p + 1) * 0.25f, bid, (p + 2) * 0.25f, q}); continue; }   // a 2-tick spread, traded inside: unknown side
            v.push_back(MTick{t, buy ? ask : bid, bid, ask, q});
        }
        if (attack > 0) { attack--; float bid = p * 0.25f, ask = (p + 1) * 0.25f; v.push_back(MTick{t, dirA > 0 ? bid : ask, bid, ask, 15 + (long)(r() % 10)}); v.push_back(MTick{t, dirA > 0 ? ask : bid, bid, ask, 3}); }
    }
    return v;
}
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static bool exists(const std::string& p) { std::ifstream f(p.c_str()); return f.good(); }
static const std::string LS = "/tmp/tfv203/home\\InvestorRT\\rtx\\lsFlexLevels";
// a chart: bar END stamps; a bar covers the trades in [end - spb, end)  (ChartView-verified: "13:54" = 13:51:00-13:53:59)
struct Chart { int id, spb; std::vector<long long> te; std::vector<long long> vol; };
static void addBar(Chart& c, long long te)
{
    mockSelectChart(c.id);
    float o = 0, h = 0, l = 0, cl = 0; bool any = false; long long v = 0;
    for (auto it = std::lower_bound(g_ticks.begin(), g_ticks.end(), te - c.spb, [](const MTick& k, long long t) { return k.t < t; }); it != g_ticks.end() && it->t < te; ++it) {
        if (!any) { o = h = l = it->px; any = true; } h = std::max(h, it->px); l = std::min(l, it->px); cl = it->px; v += it->q;
    }
    if (!any) { o = h = l = cl = 5000; }
    g_bars.push_back(te); mockBar((int)g_bars.size() - 1, o, h, l, cl); c.te.push_back(te); c.vol.push_back(v);
}
static Chart C3{0, 180, {}, {}}, C60{1, 3600, {}, {}};
static int tid3 = -1, tid60 = -1;
static double g_liveMax = 0, g_liveSum = 0; static long g_liveN = 0; static bool g_measure = false;   // (#18) the live per-second cost in RTH
static void fire(TapeFlow* tf, int ctx, int id) { mockSelectChart(ctx); RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = id;
    const auto a = std::chrono::steady_clock::now(); tf->timer(&ev);
    if (g_measure) { const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - a).count(); g_liveMax = std::max(g_liveMax, ms); g_liveSum += ms; g_liveN++; } }
// one second of IRT: each live chart's timer (fired in the OTHER chart's context when cross = true), new bars, calc / draw
static void second(TapeFlow* tf, bool with3, bool with60, bool cross)
{
    g_now++;
    if (with3) fire(tf, cross ? C60.id : C3.id, tid3);
    if (with60) fire(tf, cross ? C3.id : C60.id, tid60);
    if (g_now % 180 == 0) addBar(C3, g_now);
    if (g_now % 3600 == 0) addBar(C60, g_now);
    if (g_now % 20 == 0) { if (with3) { mockSelectChart(C3.id); static_cast<cppExtension*>(tf)->calc((int)g_bars.size() - 2); }
                           if (with60) { mockSelectChart(C60.id); static_cast<cppExtension*>(tf)->calc((int)g_bars.size() - 2); } }
    if (g_now % 60 == 0) { if (with3) { mockSelectChart(C3.id); g_text.clear(); tf->draw(); } if (with60) { mockSelectChart(C60.id); g_text.clear(); tf->draw(); } }
}
static std::vector<tfl::Mark> marksOf(TapeFlow* tf, int ctx) { mockSelectChart(ctx); tf->bindHost(true); std::vector<tfl::Mark> m; if (tf->S().book) { std::lock_guard<std::mutex> g(tf->S().book->mx); m = tf->S().book->book.marks; } return m; }
static bool dupFree(const std::string& f, long long* lastT, size_t* rows)
{
    std::stringstream ss(f); std::string ln; long long prev = LLONG_MIN; bool ok = true; *rows = 0;
    while (std::getline(ss, ln)) { if (ln.empty() || !isdigit((unsigned char)ln[0])) continue; long long t = std::atoll(ln.c_str()); if (t <= prev) ok = false; prev = t; (*rows)++; }
    *lastT = prev; return ok;
}

// ---- (2) the pixel layout of one frame of the 3-min chart at ppb
struct Frame { int bars = 0, rects = 0, outside = 0, overlapNext = 0, unordered = 0, minGap = 99, tips = 0, tipsOk = 0, heightBad = 0, heightChecked = 0; };
static Frame frameAt(TapeFlow* tf, int ppb, bool print)
{
    mockSelectChart(C3.id); g_ppb = ppb; g_rectList.clear(); g_segs.clear(); g_text.clear(); g_dashLines = 0;
    tf->draw();
    Frame F; const int n = (int)g_bars.size(); int b0 = 0, b1 = 0; tf->getVisibleBars(&b0, &b1);
    const double rng = tf->S().range;
    const int fs = FONT_PT, top = fs + 6, band = 22; const short yInnerTop = (short)(top + band), yInnerBot = (short)(300 - band);
    const double mid = (yInnerTop + yInnerBot) / 2.0, half = std::max(4.0, (yInnerBot - yInnerTop) / 2.0);
    auto Y = [&](double v) { return (short)(mid - v / rng * half + 0.5); };
    // the candle i's x-range: centre 40 + i * ppb, [centre - ppb/2, centre - ppb/2 + ppb - 1]
    auto cl = [&](int i) { return 40 + i * ppb - ppb / 2; }; auto cr = [&](int i) { return cl(i) + ppb - 1; };
    std::map<int, std::vector<std::pair<int, int> > > slots;              // bar -> its minute bars' [l, r] (distinct)
    std::map<std::pair<int, int>, std::pair<int, int> > yOf;               // (bar, l) -> (green top, red bottom)
    for (const MRect& r : g_rectList) {
        if (r.fill != (unsigned long)C_BUY && r.fill != (unsigned long)C_SELL) continue;
        F.rects++;
        const int i = (int)std::floor((r.l - 40 + ppb / 2) / (double)ppb);
        if (i < 0 || i >= n || r.l < cl(i) || r.r - 1 > cr(i)) { F.outside++; continue; }
        auto& v = slots[i]; if (std::find(v.begin(), v.end(), std::make_pair((int)r.l, (int)r.r - 1)) == v.end()) v.push_back(std::make_pair((int)r.l, (int)r.r - 1));
        auto& yy = yOf[std::make_pair(i, (int)r.l)]; if (r.fill == (unsigned long)C_BUY) yy.first = r.t; else yy.second = r.b - 1;
    }
    for (auto& kv : slots) {
        auto v = kv.second; std::sort(v.begin(), v.end()); F.bars++;
        for (size_t k = 1; k < v.size(); ++k) { if (v[k].first <= v[k - 1].second) F.unordered++; F.minGap = std::min(F.minGap, v[k].first - v[k - 1].second - 1); }
        auto nx = slots.find(kv.first + 1); if (nx != slots.end()) { int mn = 1 << 20; for (auto& s : nx->second) mn = std::min(mn, s.first); if (v.back().second >= mn) F.overlapNext++; }
    }
    // (1, drawn) every minute bar's height = that minute's contracts: the engine's bought / sold over [ts + 60k, ts + 60(k+1))
    for (auto& kv : slots) {
        const int i = kv.first; const long long te = g_bars[(size_t)i], ts = te - 180;
        auto v = kv.second; std::sort(v.begin(), v.end());
        const int ns = (int)v.size(); if (ns != 3 && ns != 1) continue;
        for (int k = 0; k < ns; ++k) {
            long long B = 0, S = 0; tfl::sideSums(tf->S().eng.hist, ts + (180 * k) / ns, ts + (180 * (k + 1)) / ns, &B, &S);
            auto yy = yOf[std::make_pair(i, v[(size_t)k].first)]; F.heightChecked++;
            if ((B > 0 && yy.first != Y((double)B)) || (S > 0 && yy.second != Y(-(double)S))) F.heightBad++;
        }
    }
    // the arrowheads: the tip row is the 2-px line (x, x+1) in green / red; its x must sit in the node minute's x-range
    std::map<long long, tfl::Mark> byT; for (auto& m : marksOf(tf, C3.id)) if (m.drawn()) byT[m.barT] = m;
    mockSelectChart(C3.id);
    for (const MSeg& s : g_segs) {
        if ((s.color != (unsigned long)C_BUY && s.color != (unsigned long)C_SELL) || s.y1 != s.y2 || s.x2 - s.x1 != 1) continue;
        F.tips++; const int ax = s.x1;
        const int i = (int)std::floor((ax - 40 + ppb / 2) / (double)ppb); if (i < 0 || i >= n) continue;
        auto mk = byT.find(g_bars[(size_t)i]); if (mk == byT.end()) continue;
        auto v = slots[i]; std::sort(v.begin(), v.end());
        const long long ts = g_bars[(size_t)i] - 180; const int mi = (int)((mk->second.minuteT - ts) / 60);
        bool ok;
        if (v.size() == 3) ok = mi >= 0 && mi < 3 && ax >= v[(size_t)mi].first && ax <= v[(size_t)mi].second;
        else ok = ax >= cl(i) && ax <= cr(i);                               // a narrow candle has one bar: the arrow is on the candle
        if (ok) F.tipsOk++;
        if (print && F.tips <= 2) printf("    A at bar %s: node minute %d (%s-%s), arrow x %d, minute bar x %d..%d, candle x %d..%d\n", stamp(g_bars[(size_t)i]).c_str(), mi,
            stamp(ts + 60 * mi).substr(11, 5).c_str(), stamp(ts + 60 * mi + 60).substr(11, 5).c_str(), ax, v.size() == 3 ? v[(size_t)mi].first : cl(i), v.size() == 3 ? v[(size_t)mi].second : cr(i), cl(i), cr(i));
    }
    if (print) printf("  ppb %2d: %d candles drawn, %d minute bars, outside candle %d, out of order %d, overlap next candle %d, min gap %dpx, heights checked %d bad %d, arrowheads %d in their minute %d\n",
        ppb, F.bars, F.rects, F.outside, F.unordered, F.overlapNext, F.minGap == 99 ? -1 : F.minGap, F.heightChecked, F.heightBad, F.tips, F.tipsOk);
    return F;
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setenv("USERPROFILE", "/tmp/tfv203/home", 1); setenv("TZ", "UTC", 1); tzset();
    system("rm -rf /tmp/tfv203 && mkdir -p /tmp/tfv203");
    tfl::Store prior;
    for (int d = 5; d >= 1; d--) {
        long long s0 = DAY0 - d * 86400 - 7 * 3600, s1 = DAY0 - d * 86400 + 16 * 3600;
        auto v = dayTape(s0, s1, 300 + d, 5000);
        tfl::Engine e; e.store = &prior; e.sym = "ESZ26";
        for (auto& k : v) { tfl::Tick t; t.t = k.t; t.px = (int)llround(k.px / 0.25); t.bid = (int)llround(k.bid / 0.25); t.ask = (int)llround(k.ask / 0.25); t.q = k.q; e.add(t); }
        e.advanceTo(s1); e.flushSession();
    }
    { for (auto& kv : prior) { std::ofstream o((LS + "\\TapeFlow\\ES-base-v110-" + std::to_string(kv.first) + ".csv").c_str()); for (auto& c : kv.second) for (auto& w : c.second) o << tfl::storeLine(kv.first, c.first, w) << "\n"; } }
    g_ticks = dayTape(DAY0 - 7 * 3600, DAY0 + 14 * 3600, 23, 5000);
    for (long long b = DAY0 - 7 * 3600 + 180; b <= DAY0 + 9 * 3600; b += 180) addBar(C3, b);
    for (long long b = DAY0 - 7 * 3600 + 3600; b <= DAY0 + 9 * 3600; b += 3600) addBar(C60, b);
    mockSelectChart(C3.id); mockSetSpb(180); mockSelectChart(C60.id); mockSetSpb(3600);
    g_now = DAY0 + 9 * 3600;
    TapeFlow* tf = static_cast<TapeFlow*>(CreateExtension()); tf->setup();
    mockSelectChart(C3.id); static_cast<cppExtension*>(tf)->calc(0); tid3 = g_timerId;
    for (int i = 0; i < 45 * 60; i++) second(tf, true, false, false);        // 09:00 -> 09:45, the 3-min chart alone
    const long long sid = tfl::sessionOf(g_now);
    mockSelectChart(C3.id); tf->bindHost(true);
    printf("timer %d, engine at %s, ticks %zu\n", tid3, stamp(tf->S().eng.lastT).c_str(), g_ticks.size());
    CHECK(tf->S().sessionDone, "the session replay is the live engine");
    // ---- (3) the header
    { char tt[200] = {0}; tf->parmsTitle(tt, sizeof(tt)); printf("header: %s\n", tt);
      CHECK(std::string(tt).find("TF 2.0.3 ES  TESTING") == 0, "header 'TF 2.0.3 ES  TESTING ...'"); }
    // ---- (1) minute windows vs the trades and the chart bar volume, every closed bar of the session so far
    {
        int bars = 0, minuteBad = 0, barBad = 0; long long totB = 0, totS = 0, totU = 0, totV = 0; const auto& H = tf->S().eng.hist;
        for (size_t j = 0; j + 1 < C3.te.size(); ++j) {
            const long long te = C3.te[j], ts = te - 180; if (ts < (sid - 1) * 86400 + 17LL * 3600) continue;
            long long sumAll = 0; bars++;
            for (int k = 0; k < 3; ++k) {
                const long long a0 = ts + 60 * k, a1 = a0 + 60;
                long long b = 0, s = 0, u = 0; for (auto& x : g_ticks) if (x.t >= a0 && x.t < a1) { const int sd = tfl::sideOf((int)llround(x.px / 0.25), (int)llround(x.bid / 0.25), (int)llround(x.ask / 0.25)); (sd == tfl::SIDE_BUY ? b : sd == tfl::SIDE_SELL ? s : u) += x.q; }
                long long eb = 0, es = 0, eu = 0; for (auto it = std::lower_bound(H.begin(), H.end(), a0, [](const tfl::SecRec& r, long long t) { return r.t < t; }); it != H.end() && it->t < a1; ++it) { eb += it->b; es += it->s; eu += it->u; }
                if (eb != b || es != s || eu != u) minuteBad++;
                sumAll += eb + es + eu; totB += eb; totS += es; totU += eu;
            }
            if (sumAll != C3.vol[j]) barBad++; totV += C3.vol[j];
        }
        printf("(1) %d closed bars: minutes [end-180+60k, end-180+60(k+1)) bought %lld sold %lld unknown %lld = chart volume %lld; minutes != trades: %d, 3 minutes != bar volume: %d\n",
            bars, totB, totS, totU, totV, minuteBad, barBad);
        CHECK(bars > 100 && minuteBad == 0 && barBad == 0 && totB + totS + totU == totV, "(1) each minute = its trades; the 3 minutes = the chart bar's volume");
    }
    // ---- (2) pixels at three zooms + (1, drawn) the bar heights
    {
        printf("(2) the 3-min pane at three zooms (candle i centre = 40 + i*ppb):\n");
        bool ok = true; int tipsAll = 0;
        for (int ppb : {4, 9, 12, 24}) {
            Frame F = frameAt(tf, ppb, true);
            if (F.rects == 0 || F.outside || F.unordered || F.overlapNext || F.heightBad || F.heightChecked == 0 || F.tipsOk != F.tips) ok = false;
            if (F.minGap < 1 && F.minGap != 99) ok = false;   // three minute bars always have a gap
            tipsAll += F.tips;
        }
        CHECK(ok, "(2) minute bars inside their candle, in order with a gap, never into the next candle; heights = the minute's contracts; arrows over their minute");
        CHECK(tipsAll > 0, "(2) the frames had A arrowheads to check");
        Frame F = frameAt(tf, 12, false);
        bool dashOk = g_dashLines == 2; std::set<short> ys; for (auto& s : g_segs) if (s.style == (int)P_DASH) { ys.insert(s.y1); if (s.color != 0x505C6Fu || s.y1 != s.y2) dashOk = false; }
        const double nm = tf->S().normNow;
        printf("(3) normal minute %.0f contracts: dashed lines %ld at y %d / %d (zero line y %d)\n", nm, g_dashLines, ys.empty() ? -1 : *ys.begin(), ys.empty() ? -1 : *ys.rbegin(), ys.empty() ? -1 : (int)((*ys.begin() + *ys.rbegin()) / 2));
        CHECK(dashOk && ys.size() == 2 && nm > 0, "(3) the dashed grey normal-minute line at +- the normal minute");
        bool colours = true; for (auto& kv : g_lineColors) if (kv.first != 0x22C55Eu && kv.first != 0xEF4444u && kv.first != 0x373F4Cu && kv.first != 0x505C6Fu) { colours = false; printf("  unexpected line colour %06lX\n", kv.first); }
        for (auto& r : g_rectList) if (r.fill != (unsigned long)C_BUY && r.fill != (unsigned long)C_SELL) colours = false;
        CHECK(colours, "(3) only green / red / grey drawn (no 180-s line, no other colour)");
        (void)F;
    }
    // ---- (audit #18) nothing heavy in RTH: the live timer pass (09:45-10:00, 3-min chart, drawing on) and one frame
    {
        g_measure = true; g_liveMax = g_liveSum = 0; g_liveN = 0;
        for (int i = 0; i < 15 * 60; i++) second(tf, true, false, false);
        g_measure = false;
        mockSelectChart(C3.id); g_ppb = 4; const auto a = std::chrono::steady_clock::now(); for (int k = 0; k < 20; ++k) { g_rectList.clear(); g_segs.clear(); tf->draw(); }
        const double drawMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - a).count() / 20;
        printf("(#18) RTH live timer pass: %ld passes, mean %.3f ms, max %.2f ms; one frame (160 candles) %.3f ms\n", g_liveN, g_liveSum / std::max(1L, g_liveN), g_liveMax, drawMs);
        CHECK(g_liveN > 800 && g_liveMax < 50 && drawMs < 20, "(#18) the RTH timer pass and a frame stay light (max < 50 ms, frame < 20 ms)");
    }
    // ---- (audit #6) a failing write: the signal lines stay queued (never marked written) and land once the file can be written again
    {
        const std::string rec = LS + "\\TapeFlow\\ES-signals-" + std::to_string(sid) + ".csv";
        mockSelectChart(C3.id); tf->bindHost(true);
        const std::string before = slurp(rec); const long long w0 = tf->S().sigWrittenLines;
        std::rename(rec.c_str(), (rec + ".moved").c_str()); system(("mkdir -p '" + rec + "'").c_str());   // the path is now a directory: open fails
        for (int i = 0; i < 9 * 60; i++) second(tf, true, false, false);                                   // 3 bars decided while the write fails
        size_t queued = 0; { std::lock_guard<std::mutex> g(tf->S().book->mx); queued = tf->S().book->book.journal.size(); }
        const long long w1 = tf->S().sigWrittenLines;
        system(("rmdir '" + rec + "'").c_str()); std::rename((rec + ".moved").c_str(), rec.c_str());
        for (int i = 0; i < 60; i++) second(tf, true, false, false);
        const std::string after = slurp(rec); size_t queued2 = 0; { std::lock_guard<std::mutex> g(tf->S().book->mx); queued2 = tf->S().book->book.journal.size(); }
        std::set<std::string> rows; bool dup = false; std::stringstream ss(after); std::string ln; size_t n = 0; while (std::getline(ss, ln)) { if (ln.empty() || ln[0] == 't') continue; n++; if (!rows.insert(ln).second) dup = true; }
        printf("(#6) write failing for 9 min: %zu lines queued, written count %lld -> %lld (unchanged); after it recovers: queue %zu, file %zu -> %zu bytes, duplicates %s\n",
            queued, w0, w1, queued2, before.size(), after.size(), dup ? "YES" : "none");
        CHECK(queued > 0 && w1 == w0 && queued2 == 0 && after.compare(0, before.size(), before) == 0 && after.size() > before.size() && !dup, "(#6) a failed write keeps the lines queued; they land once, in order, when the file is writable");
    }
    // ---- (audit #35) a pane too short: says so, draws no bars; a short pane (60 px) draws arrowheads only, still inside it
    {
        mockSelectChart(C3.id); g_ppb = 12;
        g_paneH = 20; g_rectList.clear(); g_segs.clear(); g_text.clear(); tf->draw();
        bool said = false; for (auto& s : g_text) if (s.find("pane too short") != std::string::npos) said = true;
        const bool none = g_rectList.empty() && g_segs.empty();
        g_paneH = 60; g_rectList.clear(); g_segs.clear(); g_text.clear(); tf->draw();
        bool inside = !g_rectList.empty(); for (auto& r : g_rectList) if (r.t < 0 || r.b > 61) inside = false; for (auto& s : g_segs) if (s.y1 < 0 || s.y1 > 60 || s.y2 < 0 || s.y2 > 60) inside = false;
        int labels = 0; for (auto& s : g_text) if (!s.empty() && s[0] == 'A') labels++;
        printf("(#35) 20-px pane: '%s', nothing else drawn: %s; 60-px pane: %zu bars, all inside: %s, letters %d (arrowheads only)\n", said ? "TapeFlow: pane too short" : "-", none ? "yes" : "NO", g_rectList.size(), inside ? "yes" : "NO", labels);
        CHECK(said && none && inside && labels == 0, "(#35) a too-short pane says so; a short pane stays inside, arrowheads only");
        g_paneH = 300;
    }
    // ---- (4) a second chart on the SAME object: ES 60-min
    const std::string tfd = LS + "\\TapeFlow\\";
    mockSelectChart(C60.id); g_timerId = -1; static_cast<cppExtension*>(tf)->calc(0); tid60 = g_timerId;
    CHECK(tid60 > 0 && tid60 != tid3, "(4) the 60-min chart gets its own state and timer on the same object");
    const size_t marks3Before = marksOf(tf, C3.id).size();
    for (int i = 0; i < 70 * 60; i++) second(tf, true, true, i % 2 == 1);   // 70 minutes, half the timers fired in the other chart's context
    {
        auto m3 = marksOf(tf, C3.id), m60 = marksOf(tf, C60.id);
        bool sep = true; for (auto& m : m3) if (m.barT % 180) sep = false; for (auto& m : m60) if (m.barT % 3600) sep = false;
        mockSelectChart(C3.id); tf->bindHost(true); TapeFlowState* s3 = &tf->S(); const std::string k3 = s3->bookKey;
        mockSelectChart(C60.id); tf->bindHost(true); TapeFlowState* s60 = &tf->S(); const std::string k60 = s60->bookKey;
        printf("(4) two charts: 3-min %zu signals (was %zu) key %s; 60-min %zu signals key %s; states %s\n", m3.size(), marks3Before, k3.c_str(), m60.size(), k60.c_str(), s3 != s60 ? "separate" : "SHARED");
        CHECK(s3 != s60 && k3 != k60 && sep && m3.size() >= marks3Before, "(4) separate states and records; the 3-min chart's signals untouched by the 60-min chart");
        CHECK(s60->sessionDone && s60->eng.lastT >= g_now - 5, "(4) the 60-min chart's engine is live (its timer ran whatever context it was fired in)");
        CHECK(exists(tfd + "ES-signals-" + std::to_string(sid) + ".csv") && exists(tfd + "ES-signals-" + std::to_string(sid) + "-3600s.csv"), "(4) one signals record per bar size");
        CHECK(exists(LS + "\\TapeFlow.status-ES.txt") && exists(LS + "\\TapeFlow.status-ES-3600s.txt"), "(4) one status file per bar size (status keyed by seconds per bar)");
        long long lastT = 0; size_t rows = 0; const bool nodup = dupFree(slurp(tfd + "ES-sec-" + std::to_string(sid) + ".csv"), &lastT, &rows);
        printf("    per-second file: %zu rows, strictly increasing: %s, last %s\n", rows, nodup ? "yes" : "NO", stamp(lastT).c_str());
        CHECK(nodup && rows > 1000, "(4) one writer: the per-second file never has a second twice");
        // (2) again with the second chart live: the 3-min pane still lines up
        Frame F = frameAt(tf, 12, false); CHECK(F.rects > 0 && !F.outside && !F.overlapNext && !F.heightBad && F.tips == F.tipsOk, "(4) the 3-min pane unchanged by the 60-min chart");
        // the 60-min pane: 6 ten-minute bars per candle, inside the candle
        mockSelectChart(C60.id); g_ppb = 24; g_rectList.clear(); tf->draw(); int out = 0, nr = 0;
        for (auto& r : g_rectList) { if (r.fill != (unsigned long)C_BUY && r.fill != (unsigned long)C_SELL) continue; nr++; const int i = (int)std::floor((r.l - 40 + 12) / 24.0); if (r.l < 40 + i * 24 - 12 || r.r - 1 > 40 + i * 24 + 11) out++; }
        printf("    60-min pane at 24 px: %d bars, %d outside their candle\n", nr, out);
        CHECK(nr > 0 && out == 0, "(4) the 60-min pane's bars inside their candles");
    }
    // the writer chart (3-min) is removed: the 60-min chart takes over writing, no second lost or doubled
    {
        long long t0 = 0; size_t r0 = 0; dupFree(slurp(tfd + "ES-sec-" + std::to_string(sid) + ".csv"), &t0, &r0);
        mockSelectChart(C3.id); static_cast<cppExtension*>(tf)->destroy();
        mockSelectChart(C60.id);
        const long long removedAt = g_now;
        for (int i = 0; i < 15 * 60; i++) { second(tf, false, true, false); if (i == 30) fire(tf, C60.id, tid3); }   // the removed chart's timer fires once
        long long t1 = 0; size_t r1 = 0; const bool nodup = dupFree(slurp(tfd + "ES-sec-" + std::to_string(sid) + ".csv"), &t1, &r1);
        printf("    after removing the 3-min chart at %s: per-second file %zu -> %zu rows, last %s, strictly increasing: %s\n", stamp(removedAt).c_str(), r0, r1, stamp(t1).c_str(), nodup ? "yes" : "NO");
        CHECK(nodup && t1 >= removedAt + 10 * 60 && r1 > r0, "(4) the other chart takes over writing: no second doubled, the file keeps growing");
        mockSelectChart(C60.id); tf->bindHost(true);
        CHECK(tf->S().eng.lastT >= g_now - 5, "(4) the removed chart's timer firing does not disturb the live one");
        // gaps: every second with trades (outside the 16:00 hour) is in the file
        std::set<long long> have; { std::stringstream ss(slurp(tfd + "ES-sec-" + std::to_string(sid) + ".csv")); std::string ln; while (std::getline(ss, ln)) if (!ln.empty() && isdigit((unsigned char)ln[0])) have.insert(std::atoll(ln.c_str())); }
        long long missing = 0, checked = 0; for (auto& k : g_ticks) if (k.t >= DAY0 + 9 * 3600 && k.t < t1 - 30) { checked++; if (!have.count(k.t)) missing++; }
        printf("    trade seconds 09:00-%s in the file: %lld checked, %lld missing\n", stamp(t1 - 30).substr(11, 5).c_str(), checked, missing);
        CHECK(missing == 0, "(4) no second lost through the hand-over");
        // (4b) a timer fired in a chart showing ANOTHER contract (ESH27) never resets this chart's engine; the chart's own roll does
        const size_t histBefore = tf->S().eng.hist.size(); const std::string symBefore = tf->S().sym;
        g_sym = "ESH27"; fire(tf, C3.id, tid60); g_sym = "ESZ26";
        mockSelectChart(C60.id); tf->bindHost(true);
        printf("    timer in a chart on ESH27: engine kept %zu -> %zu seconds, contract %s\n", histBefore, tf->S().eng.hist.size(), tf->S().sym.c_str());
        CHECK(tf->S().sym == symBefore && tf->S().eng.hist.size() >= histBefore, "(4b) a timer in another contract's chart context does not reset this chart");
        g_sym = "ESH27"; mockSelectChart(C60.id); static_cast<cppExtension*>(tf)->calc((int)g_bars.size() - 2); fire(tf, C60.id, tid60);
        mockSelectChart(C60.id); tf->bindHost(true);
        printf("    this chart rolled to ESH27 (its own calc saw it): contract now %s\n", tf->S().sym.c_str());
        CHECK(tf->S().sym == "ESH27", "(4b) the chart's own contract roll is followed");
        g_sym = "ESZ26";
        static_cast<cppExtension*>(tf)->destroy();
    }
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
