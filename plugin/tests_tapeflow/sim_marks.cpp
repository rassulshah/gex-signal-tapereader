// (2.0.3) the REAL TapeFlowMarks.cpp (lsTapeFlowMarks) against mockirt:
//  - the dash: at the node price (ticks x the chart's tick), across EXACTLY its candle's body span (the same 90% span TapeFlow's minute
//    bars use), never into the next candle, at 3 zooms; red bearish / green bullish; the label to the right of the dash
//  - "A? 3.6x" until the record confirms it, then "A 3.6x"; an A? that expires stays A?; P / E / F never drawn
//  - re-reads a file only when it changed (size / time); a half-written last line is not drawn until it is complete; a missing,
//    garbage or oversized (> 4 MB) file draws nothing and never crashes
//  - bar by bar, the record growing as TapeFlow writes it: a mark once drawn never moves or disappears, a '?' only confirms; the
//    end state == one pass over the finished file; a chart reload mid-way redraws the same
//  - two charts on ONE object: ES + NQ (each its own market's file), ES 3-min + ES 60-min (each its own bar size's file); timers
//    fired in the other chart's context still refresh their own chart
//  - the 17:00 session roll: yesterday's marks still drawn from the previous session's file; today's file picked up when it appears
#include "mockirt.h"
#include <vector>
void mockBar(int i, float o, float h, float l, float c);
#include "TapeFlowMarks.cpp"
#include <fstream>
#include <sstream>
#include <set>
static int fails = 0, npass_ = 0;
#define CHECK(c, m) do { if (c) npass_++; else { fails++; printf("FAIL %d: %s\n", __LINE__, m); } } while (0)
static const long long DAY0 = 1791417600LL;               // 2026-10-08 00:00 "local"
static const std::string LS = "/tmp/tfmk/home\\InvestorRT\\rtx\\lsFlexLevels\\TapeFlow\\";
static std::string pathFor(const char* mkt, long long sid, int spb) { return LS + mkt + "-signals-" + std::to_string(sid) + (spb != 180 ? "-" + std::to_string(spb) + "s" : std::string()) + ".csv"; }
struct ChartCtx { int id; std::string root, sym; int spb; };
static ChartCtx ES3{0, "ES", "ESZ26", 180}, NQ3{1, "NQ", "NQZ26", 180}, ES60{2, "ES", "ESZ26", 3600}, GC3{3, "GC", "GCZ26", 180}, CL3{4, "CL", "CLX26", 180}, HG3{5, "HG", "HGZ26", 180};
static void sel(const ChartCtx& c) { mockSelectChart(c.id); g_root = c.root; g_sym = c.sym; }
static void bars(const ChartCtx& c, long long from, long long to) { sel(c); mockSetSpb(c.spb); for (long long t = from; t <= to; t += c.spb) { g_bars.push_back(t); mockBar((int)g_bars.size() - 1, 5000, 5001, 4999, 5000); } }
static void growBars(const ChartCtx& c) { sel(c); while (!g_bars.empty() && g_bars.back() + c.spb <= g_now) { g_bars.push_back(g_bars.back() + c.spb); mockBar((int)g_bars.size() - 1, 5000, 5001, 4999, 5000); } }
// a record as TapeFlow writes it: (the time the line is written, the line)
struct Rec { long long at; std::string line; };
static std::vector<Rec> makeRecord(long long from, long long to, int spb, int px0, unsigned seed)
{
    std::vector<Rec> v; unsigned r = seed; int k = 0;
    for (long long te = from; te <= to; te += spb) {
        r = r * 1103515245u + 12345u; const unsigned x = (r >> 16) % 100;
        v.push_back(Rec{te + 1, std::to_string(te) + "|D|0|-|0|0|0|0|0||2.0.3"});
        if (x < 20) {
            tfl::Mark m; m.barT = te; m.kind = 'A'; m.dir = (k++ % 2) ? 1 : -1; m.px = px0 + (int)((r >> 8) % 80); m.lo = m.hi = m.px;
            m.mult = 2.0 + (x % 7) * 0.4; m.absorbed = m.dir < 0 ? 900 : -900; m.held = 2; m.minuteT = te - spb + 60 * (x % 3); m.minuteVol = 900; m.norm = 300; m.why = "bar node";
            v.push_back(Rec{te + 1, tfl::SignalBook::line(m, "2.0.3")});
            if (x < 14) { m.state = tfl::MK_CONFIRMED; m.confT = te + spb * (1 + x % 3); v.push_back(Rec{m.confT + 1, tfl::SignalBook::line(m, "2.0.3")}); }
            else if (x < 17) { m.state = tfl::MK_EXPIRED; m.confT = te + spb * 4; v.push_back(Rec{m.confT + 1, tfl::SignalBook::line(m, "2.0.3")}); }
        } else if (x < 30) { tfl::Mark m; m.barT = te; m.kind = "PEF"[x % 3]; m.dir = 1; m.px = px0; m.lo = m.hi = m.px; m.why = "logged"; v.push_back(Rec{te + 1, tfl::SignalBook::line(m, "2.0.3")}); }
    }
    std::stable_sort(v.begin(), v.end(), [](const Rec& a, const Rec& b) { return a.at < b.at; });
    return v;
}
static void writeUpTo(const std::string& path, const std::vector<Rec>& v, long long t)
{
    std::ofstream o(path.c_str(), std::ios::binary | std::ios::trunc); o << tfl::SignalBook::header() << "\n";
    for (auto& r : v) if (r.at <= t) o << r.line << "\n";
}
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static TapeFlowMarks* TF = nullptr;
static std::map<int, int> tidOf;
static void tick(const std::vector<const ChartCtx*>& charts, bool cross)
{
    g_now += 2;
    for (size_t i = 0; i < charts.size(); ++i) {
        const ChartCtx& c = *charts[i]; growBars(c);
        const ChartCtx& ctx = cross ? *charts[(i + 1) % charts.size()] : c; sel(ctx);
        RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = tidOf[c.id]; TF->timer(&ev);
    }
}
static void attach(const ChartCtx& c) { sel(c); g_timerId = -1; static_cast<cppExtension*>(TF)->calc(0); tidOf[c.id] = g_timerId; }
static MarksState* st(const ChartCtx& c) { sel(c); return TF->slot_.get(TF, false); }
// one frame: bar index -> (price, colour, label, dash x1, dash x2, label x)
struct Drawn { float price; unsigned long color; std::string label; int x1, x2, lx, leadTo; };
static std::map<long long, Drawn> frame(const ChartCtx& c, int ppb)
{
    sel(c); g_ppb = ppb; g_pntSets.clear(); g_segs.clear(); g_textAt.clear(); TF->draw();
    std::map<long long, Drawn> out; size_t si = 0;
    for (size_t k = 0; k < g_pntSets.size(); ++k) {
        const int bar = g_pntSets[k].first; Drawn d; d.price = g_pntSets[k].second;
        d.color = si < g_segs.size() ? g_segs[si].color : 0; d.x1 = si < g_segs.size() ? g_segs[si].x1 : -1; d.x2 = si < g_segs.size() ? g_segs[si].x2 : -1; si++;
        d.leadTo = -1; if (si < g_segs.size() && g_segs[si].x1 == d.x2 + 1 && g_segs[si].color == d.color) { d.leadTo = g_segs[si].x2; si++; }   // the thin leader to the label
        d.label = k < g_textAt.size() ? g_textAt[k].first : ""; d.lx = k < g_textAt.size() ? g_textAt[k].second : -1;
        out[g_bars[(size_t)bar]] = d;
    }
    return out;
}
static std::map<long long, tfl::Mark> expected(const std::string& text) { tfl::SignalBook b; b.loadText(text); std::map<long long, tfl::Mark> m; for (auto& k : b.marks) if (k.drawn()) m[k.barT] = k; return m; }

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    setenv("USERPROFILE", "/tmp/tfmk/home", 1); setenv("TZ", "UTC", 1); tzset();
    system("rm -rf /tmp/tfmk && mkdir -p /tmp/tfmk");
    // the market table
    { const char* in[][2] = {{"EP", "ES"}, {"ES", "ES"}, {"MES", "ES"}, {"ENQ", "NQ"}, {"MNQ", "NQ"}, {"CLE", "CL"}, {"QM", "CL"}, {"GCE", "GC"}, {"MGC", "GC"}, {"CPE", "HG"}, {"HG", "HG"}, {"NGE", "NG"}, {"QG", "NG"}, {"EU6", "EU"}, {"6E", "EU"}, {"XYZ", ""}};
      bool ok = true; for (auto& p : in) if (marketOf(p[0], "") != p[1]) { ok = false; printf("  market of %s = '%s' (want %s)\n", p[0], marketOf(p[0], "").c_str(), p[1]); }
      CHECK(ok, "symbol -> market table (roots and micros)"); }
    const long long sid = tfl::sessionOf(DAY0 + 9 * 3600), s0 = (sid - 1) * 86400 + 17 * 3600;   // today's session starts 17:00 yesterday
    // yesterday's session record (drawn after the roll / on yesterday's bars) and today's, ES and NQ, 3-min and 60-min
    const auto esPrev = makeRecord(s0 - 86400 + 180, s0 - 3600, 180, 20000, 7);
    const auto es = makeRecord(s0 + 180, DAY0 + 16 * 3600, 180, 20200, 11);
    const auto nq = makeRecord(s0 + 180, DAY0 + 16 * 3600, 180, 80000, 13);
    const auto es60 = makeRecord(s0 + 3600, DAY0 + 16 * 3600, 3600, 20100, 17);
    writeUpTo(pathFor("ES", sid - 1, 180), esPrev, LLONG_MAX);
    g_now = DAY0 + 9 * 3600;
    bars(ES3, s0 - 86400 + 180, g_now); bars(NQ3, s0 + 180, g_now); bars(ES60, s0 + 3600, g_now);
    bars(GC3, s0 + 180, g_now); bars(CL3, s0 + 180, g_now); bars(HG3, s0 + 180, g_now);
    TF = static_cast<TapeFlowMarks*>(CreateExtension()); TF->setup();
    for (auto* c : {&ES3, &NQ3, &ES60}) attach(*c);
    CHECK(tidOf[ES3.id] > 0 && tidOf[NQ3.id] > 0 && tidOf[ES3.id] != tidOf[NQ3.id] && st(ES3) != st(NQ3), "one object, a state and timer per chart");
    // ---- bar by bar: the ES record grows exactly as TapeFlow appends it, the ES chart re-reads and draws
    std::map<long long, Drawn> seen; int moved = 0, vanished = 0, unconfirmed = 0, steps = 0, frames = 0;
    long reads0 = 0; std::string half;
    const std::vector<const ChartCtx*> both = {&ES3, &NQ3, &ES60};
    for (long long t = g_now - 3600; t <= g_now; t += 180) { writeUpTo(pathFor("ES", sid, 180), es, t); writeUpTo(pathFor("NQ", sid, 180), nq, t); writeUpTo(pathFor("ES", sid, 3600), es60, t); }
    for (int b = 0; b < 100; ++b) {
        for (int k = 0; k < 90; ++k) { if (k % 10 == 0) { writeUpTo(pathFor("ES", sid, 180), es, g_now); writeUpTo(pathFor("NQ", sid, 180), nq, g_now); writeUpTo(pathFor("ES", sid, 3600), es60, g_now); }
            tick(both, (k % 2) == 1); }                                     // 3 minutes; half the timers in another chart's context
        steps++;
        if (b > 0) {                                                           // a chart reload / IRT restart after EVERY bar: a new object, same files
            auto before = frame(ES3, 12);
            sel(ES3); static_cast<cppExtension*>(TF)->destroy(); sel(NQ3); static_cast<cppExtension*>(TF)->destroy(); sel(ES60); static_cast<cppExtension*>(TF)->destroy();
            delete TF; TF = static_cast<TapeFlowMarks*>(CreateExtension()); TF->setup(); for (auto* c : {&ES3, &NQ3, &ES60}) attach(*c);
            tick(both, false);
            auto after = frame(ES3, 12); bool same = before.size() == after.size();
            for (auto& kv : before) { auto it = after.find(kv.first); if (it == after.end() || it->second.price != kv.second.price || it->second.label != kv.second.label) same = false; }
            static int reloads = 0, differ = 0; reloads++; if (!same || before.empty()) differ++;
            if (b == 99) { printf("restart after every bar: %d restarts, frames that differed after a restart: %d\n", reloads, differ);
                CHECK(differ == 0 && reloads == 99, "a restart (new object) after every bar redraws exactly the same marks"); }
        }
        auto f = frame(ES3, 12); frames++;
        for (auto& kv : f) {
            auto it = seen.find(kv.first);
            if (it != seen.end()) {
                if (it->second.price != kv.second.price || it->second.color != kv.second.color) moved++;
                if (it->second.label != kv.second.label && !(it->second.label[1] == '?' && kv.second.label[1] == ' ' && it->second.label.substr(3) == kv.second.label.substr(2))) unconfirmed++;
            }
            seen[kv.first] = kv.second;
        }
        for (auto& kv : seen) { const long long first = g_bars.size() > 160 ? g_bars[g_bars.size() - 160] : 0; if (kv.first >= first && !f.count(kv.first)) vanished++; }
    }
    printf("bar by bar: %d bars, %zu marks drawn over time, moved %d, vanished %d, changed other than ? -> confirmed %d\n", steps, seen.size(), moved, vanished, unconfirmed);
    CHECK(!seen.empty() && moved == 0 && vanished == 0 && unconfirmed == 0, "no repaint: a drawn mark never moves or disappears; '?' only becomes confirmed");
    {   // the end state == one pass over the finished file
        auto f = frame(ES3, 12); auto ex = expected(slurp(pathFor("ES", sid, 180)));
        auto exPrev = expected(slurp(pathFor("ES", sid - 1, 180))); for (auto& kv : exPrev) ex.insert(kv);
        size_t vis = 0; bool same = true; const long long first = g_bars[g_bars.size() - 160], last = g_bars.back();
        for (auto& kv : ex) if (kv.first >= first && kv.first <= last) { vis++; auto it = f.find(kv.first);
            if (it == f.end() || std::fabs(it->second.price - kv.second.px * 0.25f) > 1e-3 || it->second.label != tfl::markLabel(kv.second) || it->second.color != (kv.second.dir > 0 ? (unsigned long)C_BUY : (unsigned long)C_SELL)) same = false; }
        int pend = 0, conf = 0; for (auto& kv : f) (kv.second.label[1] == '?' ? pend : conf)++;
        printf("end state: %zu marks visible, %zu drawn (%d 'A?', %d 'A'), == one pass over the file: %s\n", vis, f.size(), pend, conf, same && vis == f.size() ? "yes" : "NO");
        CHECK(same && vis == f.size() && pend > 0 && conf > 0, "the end state equals one pass over the finished record (price, colour, label)");
        bool noPEF = true; for (auto& kv : f) if (kv.second.label[0] != 'A') noPEF = false; CHECK(noPEF, "P / E / F never drawn");
    }
    // ---- the dash geometry at three zooms
    for (int ppb : {4, 12, 24}) {
        auto f = frame(ES3, ppb); int bad = 0, n = 0; Drawn ex1{}; long long exT = 0;
        for (auto& kv : f) {
            const auto it = std::find(g_bars.begin(), g_bars.end(), kv.first); const int i = (int)(it - g_bars.begin());
            if (!n) printf("    candle x-range %d..%d (centre %d), body span %d..%d\n", 40 + i * ppb - ppb / 2, 40 + i * ppb - ppb / 2 + ppb - 1, 40 + i * ppb, 40 + i * ppb - std::max(1, (int)(ppb * 0.9)) / 2, 40 + i * ppb - std::max(1, (int)(ppb * 0.9)) / 2 + std::max(1, (int)(ppb * 0.9)) - 1);
            const int cx = 40 + i * ppb, span = std::max(1, (int)(ppb * 0.9)), l = cx - span / 2, r = l + span - 1;
            int m1[6], m2[6]; const int ns = tfl::minuteSlots(cx, ppb, 3, m1, m2);
            const bool inCandle = kv.second.x1 == l && kv.second.x2 == r && l >= cx - ppb / 2 && r <= cx - ppb / 2 + ppb - 1;
            const bool coversMinutes = kv.second.x1 <= m1[0] && kv.second.x2 >= m2[ns - 1];
            const bool nextClear = r < (40 + (i + 1) * ppb) - ppb / 2;
            const bool labelRight = kv.second.lx > kv.second.x2 && kv.second.leadTo == kv.second.lx - 2;   // the leader runs from the candle to the label
            if (!inCandle || !coversMinutes || !nextClear || !labelRight) bad++;
            if (!n) { ex1 = kv.second; exT = kv.first; } n++;
        }
        printf("  ppb %2d: %d dashes, bad %d (e.g. bar %lld: %s at %.2f, dash x %d..%d, leader to x %d, label at x %d)\n", ppb, n, bad, exT, ex1.label.c_str(), ex1.price, ex1.x1, ex1.x2, ex1.leadTo, ex1.lx);
        CHECK(n > 0 && bad == 0, "the dash spans exactly its candle's body (the minute bars' span), never into the next candle, label to its right");
    }
    // ---- re-read only on change; a half-written last line
    {
        MarksState* s = st(ES3); const long r0 = s->reads, k0 = s->stats;
        for (int k = 0; k < 30; ++k) { g_now += 2; sel(ES3); RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = tidOf[ES3.id]; TF->timer(&ev); }
        printf("60 s with no change: %ld reads (stat calls %ld)\n", s->reads - r0, s->stats - k0);
        CHECK(s->reads == r0, "an unchanged file is never re-read (only its size / time checked every 2 s)");
        tfl::Mark m; m.barT = g_bars.back(); m.kind = 'A'; m.dir = -1; m.px = 20500; m.lo = m.hi = m.px; m.mult = 3.6; m.minuteT = m.barT - 120;
        const std::string ln = tfl::SignalBook::line(m, "2.0.3");
        { std::ofstream o(pathFor("ES", sid, 180).c_str(), std::ios::app | std::ios::binary); o << ln.substr(0, ln.size() / 2); }
        tick({&ES3}, false); auto f1 = frame(ES3, 12);
        { std::ofstream o(pathFor("ES", sid, 180).c_str(), std::ios::app | std::ios::binary); o << ln.substr(ln.size() / 2) << "\n"; }
        tick({&ES3}, false); auto f2 = frame(ES3, 12);
        const bool halfHidden = !f1.count(m.barT), fullShown = f2.count(m.barT) && f2[m.barT].label == "A? 3.6x";
        printf("half-written line: drawn %s; completed: drawn %s ('%s')\n", halfHidden ? "no" : "YES", fullShown ? "yes" : "NO", f2.count(m.barT) ? f2[m.barT].label.c_str() : "-");
        CHECK(halfHidden && fullShown && s->reads == r0 + 2, "a half-written last line is not drawn until it is complete; each change read once");
    }
    // ---- missing / garbage / oversized files
    {
        { std::ofstream o(pathFor("HG", sid, 180).c_str(), std::ios::binary); for (int i = 0; i < 5000; ++i) o << (char)(i * 7919 % 256); }
        { std::ofstream o(pathFor("GC", sid, 180).c_str(), std::ios::binary); o << tfl::SignalBook::header() << "\n"; std::string big(5 * 1024 * 1024, 'x'); o << big << "\n"; }
        for (auto* c : {&GC3, &CL3, &HG3}) attach(*c);
        for (int k = 0; k < 3; ++k) tick({&CL3, &HG3, &GC3}, false);
        auto fc = frame(CL3, 12), fh = frame(HG3, 12), fg = frame(GC3, 12);
        printf("missing (CL) %zu drawn / reads %ld; garbage (HG) %zu drawn; oversized 5 MB (GC) %zu drawn, skipped %ld\n", fc.size(), st(CL3)->reads, fh.size(), fg.size(), st(GC3)->skipped);
        CHECK(fc.empty() && fh.empty() && fg.empty() && st(GC3)->skipped >= 1 && st(GC3)->reads == 0, "missing / garbage / oversized files draw nothing, never crash; > 4 MB never read");
    }
    // ---- (audit #24) the tick comes from the chart (SYM_TICKINCR): QM (CL market, 0.025) draws at px x 0.025; a bad property falls back to the table
    {
        static ChartCtx QM3{6, "QM", "QMX26", 180}; bars(QM3, s0 + 180, g_now);
        tfl::Mark m; m.barT = g_bars[g_bars.size() - 3]; m.kind = 'A'; m.dir = 1; m.px = 2600; m.lo = m.hi = m.px; m.mult = 2.0;   // an old record line: no tick column
        tfl::Mark m2 = m; m2.barT = g_bars[g_bars.size() - 4]; m2.px = 6512; m2.lo = m2.hi = m2.px; m2.tickSize = 0.01;                   // written by TapeFlowCL on a CLE chart (0.01)
        { std::ofstream o(pathFor("CL", tfl::sessionOf(g_now), 180).c_str(), std::ios::binary); o << tfl::SignalBook::header() << "\n" << tfl::SignalBook::line(m2, "2.0.3") << "\n" << tfl::SignalBook::line(m, "2.0.3") << "\n"; }
        g_tickIncr = 0.025f; attach(QM3); tick({&QM3}, false); auto f1 = frame(QM3, 12);
        g_tickIncr = NAN; sel(QM3); static_cast<cppExtension*>(TF)->calc(0); auto f2 = frame(QM3, 12);
        g_tickIncr = -1; sel(QM3); static_cast<cppExtension*>(TF)->calc(0); auto f3 = frame(QM3, 12);
        g_tickIncr = 0.25f;
        const float p1 = f1.count(m.barT) ? f1[m.barT].price : -1, p2 = f2.count(m.barT) ? f2[m.barT].price : -1, p3 = f3.count(m.barT) ? f3[m.barT].price : -1;
        printf("QM chart, node 2600 ticks: SYM_TICKINCR 0.025 -> %.3f; NaN -> %.3f; -1 -> %.3f (table 0.01)\n", p1, p2, p3);
        const float q1 = f1.count(m2.barT) ? f1[m2.barT].price : -1;
        printf("  a record carrying its tick (6512 ticks x 0.01, from a CLE chart) on the QM chart: %.3f\n", q1);
        CHECK(std::fabs(p1 - 65.0f) < 1e-3 && std::fabs(p2 - 26.0f) < 1e-3 && std::fabs(p3 - 26.0f) < 1e-3, "(#24) the chart's SYM_TICKINCR is the tick; non-finite / negative -> the market table");
        CHECK(std::fabs(q1 - 65.12f) < 1e-3, "(#24) a record written on CLE (0.01) draws at the right price on a QM (0.025) chart: the record carries its tick");
        sel(QM3); static_cast<cppExtension*>(TF)->destroy();
    }
    // ---- two charts on one object: ES + NQ, ES 3-min + ES 60-min
    {
        auto fe = frame(ES3, 12), fn = frame(NQ3, 12), f60 = frame(ES60, 12);
        auto exN = expected(slurp(pathFor("NQ", sid, 180))), ex60 = expected(slurp(pathFor("ES", sid, 3600)));
        bool nqOk = !fn.empty(), e60Ok = !f60.empty(), esOk = true;
        for (auto& kv : fn) { auto it = exN.find(kv.first); if (it == exN.end() || std::fabs(it->second.px * 0.25f - kv.second.price) > 1e-3) nqOk = false; }
        for (auto& kv : f60) { auto it = ex60.find(kv.first); if (it == ex60.end() || kv.first % 3600) e60Ok = false; }
        for (auto& kv : fe) if (kv.second.price > 10000) esOk = false;            // no NQ price (>= 20,000) on the ES chart
        printf("ES chart %zu marks, NQ chart %zu marks (all from NQ's file: %s), ES 60-min %zu marks (all from the -3600s file: %s)\n", fe.size(), fn.size(), nqOk ? "yes" : "NO", f60.size(), e60Ok ? "yes" : "NO");
        CHECK(nqOk && esOk && e60Ok, "ES + NQ and ES 3-min + 60-min on one object: each chart draws only its own market's / bar size's record");
    }
    // ---- the 17:00 roll: today's marks become 'previous' and stay drawn; the new session's file is picked up
    {
        const auto before = frame(ES3, 12);
        while (g_now < DAY0 + 17 * 3600 + 120) tick({&ES3}, false);
        auto f = frame(ES3, 12); int kept = 0; for (auto& kv : before) if (f.count(kv.first) && f[kv.first].label == kv.second.label) kept++;
        tfl::Mark m; m.barT = DAY0 + 17 * 3600 + 180; m.kind = 'A'; m.dir = 1; m.px = 20300; m.lo = m.hi = m.px; m.mult = 2.5;
        { std::ofstream o(pathFor("ES", sid + 1, 180).c_str(), std::ios::binary); o << tfl::SignalBook::header() << "\n" << tfl::SignalBook::line(m, "2.0.3") << "\n"; }
        while (g_now < DAY0 + 17 * 3600 + 200) tick({&ES3}, false);
        auto f2 = frame(ES3, 12);
        printf("17:00 roll: %d of %zu marks still drawn from yesterday's file (visible window moved %zu bars); new session's first A drawn: %s\n", kept, before.size(), (size_t)((g_now - (DAY0 + 9 * 3600)) / 180), f2.count(m.barT) ? "yes" : "NO");
        bool stillThere = true; const long long first = g_bars[g_bars.size() - 160]; for (auto& kv : before) if (kv.first >= first && !f.count(kv.first)) stillThere = false;
        CHECK(stillThere && f2.count(m.barT) && f2[m.barT].label == "A? 2.5x", "the roll keeps the previous session's marks and picks up the new session's file");
    }
    for (auto* c : {&ES3, &NQ3, &ES60, &GC3, &CL3, &HG3}) { sel(*c); static_cast<cppExtension*>(TF)->destroy(); }
    { sel(ES3); RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = tidOf[ES3.id]; CHECK(TF->timer(&ev) == RTX_FAIL, "a removed chart's timer does nothing"); }
    delete TF;
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
