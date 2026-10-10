// (2.0.4) the REAL TapeFlowMarks.cpp (lsTapeFlowMarks 1.1.0) against mockirt:
//  - the letter: "A?" pending / "A" confirmed, centred on its candle, just ABOVE the candle's high (bottom 4 px above it, never on
//    the candle), red bearish / green bullish, at 3 zooms; nothing else drawn (no dash, no multiple)
//  - only bar-node A records: a REAL 2.0.0 record (ES-signals-20735, 20-s method, no multiple) draws nothing; skipped and counted
//    in the status file, which is rewritten only when its content changes
//  - re-reads a file only when it changed; a half-written last line waits; missing / garbage / oversized files draw nothing
//  - bar by bar as the record grows, with a restart after EVERY bar: a letter never moves or disappears, '?' only becomes confirmed;
//    the end state == one pass over the finished file
//  - two charts on ONE object: ES + NQ, ES 3-min + ES 60-min - each draws only its own record; cross-context timers
//  - the 17:00 roll; 50 charts created / switched / destroyed (run under LeakSanitizer: 0 leaks)
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
static float hiOf(long long t) { return 5001.0f + 0.25f * (float)((t / 180) % 9); }
static void bars(const ChartCtx& c, long long from, long long to) { sel(c); mockSetSpb(c.spb); for (long long t = from; t <= to; t += c.spb) { g_bars.push_back(t); mockBar((int)g_bars.size() - 1, 5000, hiOf(t), 4999, 5000); } }
static void growBars(const ChartCtx& c) { sel(c); while (!g_bars.empty() && g_bars.back() + c.spb <= g_now) { g_bars.push_back(g_bars.back() + c.spb); mockBar((int)g_bars.size() - 1, 5000, hiOf(g_bars.back()), 4999, 5000); } }
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
static long st0(const ChartCtx& c) { MarksState* s = st(c); return s ? s->statusWrites : -1; }
// one frame: bar end -> what was drawn for it (the price the letter was anchored at, its text, colour and rectangle)
struct Drawn { float price; unsigned long color; std::string label; int l, t, r, b, cx, yHigh; };
static std::map<long long, Drawn> frame(const ChartCtx& c, int ppb)
{
    sel(c); g_ppb = ppb; g_pntSets.clear(); g_segs.clear(); g_textRects.clear(); g_rectList.clear(); TF->draw();
    std::map<long long, Drawn> out;
    for (size_t k = 0; k < g_pntSets.size() && k < g_textRects.size(); ++k) {
        const int bar = g_pntSets[k].first; const MText& t = g_textRects[k]; Drawn d;
        d.price = g_pntSets[k].second; d.color = t.color; d.label = t.s; d.l = t.l; d.t = t.t; d.r = t.r; d.b = t.b;
        d.cx = 40 + bar * ppb; d.yHigh = (int)(short)std::max(-30000.0, std::min(30000.0, 500.0 - ((double)d.price - g_yOrigin) * g_yPxPerPt));   // the same price -> y as the chart
        out[g_bars[(size_t)bar]] = d;
    }
    if (g_textRects.size() != g_pntSets.size() || !g_segs.empty() || !g_rectList.empty()) out[-1] = Drawn{0, 0, "EXTRA DRAWING", 0, 0, 0, 0, 0, 0};
    return out;
}
static std::string want(const tfl::Mark& m) { return m.question() ? "A?" : "A"; }
static std::map<long long, tfl::Mark> expected(const std::string& text) { tfl::SignalBook b; b.nodeOnly = true; b.loadText(text); std::map<long long, tfl::Mark> m; for (auto& k : b.marks) if (k.drawn()) m[k.barT] = k; return m; }

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
                if (it->second.label != kv.second.label && !(it->second.label == "A?" && kv.second.label == "A")) unconfirmed++;
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
            if (it == f.end() || std::fabs(it->second.price - hiOf(kv.first)) > 1e-3 || it->second.label != want(kv.second) || it->second.color != (kv.second.dir > 0 ? (unsigned long)C_BUY : (unsigned long)C_SELL)) same = false; }
        int pend = 0, conf = 0; for (auto& kv : f) (kv.second.label == "A?" ? pend : conf)++;
        printf("end state: %zu marks visible, %zu drawn (%d 'A?', %d 'A'), == one pass over the file: %s\n", vis, f.size(), pend, conf, same && vis == f.size() ? "yes" : "NO");
        CHECK(same && vis == f.size() && pend > 0 && conf > 0, "the end state equals one pass over the finished record (price, colour, label)");
        bool noPEF = !f.count(-1); for (auto& kv : f) if (kv.second.label != "A" && kv.second.label != "A?") noPEF = false; CHECK(noPEF, "only the letter A / A? is drawn (no dash, no multiple, never P / E / F)");
    }
    // ---- the letter's place at three zooms: centred on the candle, its bottom 4 px above the candle's high
    for (int ppb : {4, 12, 24}) {
        auto f = frame(ES3, ppb); int bad = 0, n = 0; Drawn ex1{}; long long exT = 0;
        for (auto& kv : f) {
            if (kv.first < 0) { bad++; continue; }
            const Drawn& d = kv.second;
            const bool centred = (d.l + d.r) / 2 == d.cx, above = d.b == d.yHigh - 4 && d.t < d.b, atHigh = std::fabs(d.price - hiOf(kv.first)) < 1e-3;
            if (!centred || !above || !atHigh) bad++;
            if (!n) { ex1 = d; exT = kv.first; } n++;
        }
        printf("  ppb %2d: %d letters, bad %d (e.g. bar %lld: '%s' high %.2f at y %d, letter box x %d..%d y %d..%d, candle centre %d)\n", ppb, n, bad, exT, ex1.label.c_str(), ex1.price, ex1.yHigh, ex1.l, ex1.r, ex1.t, ex1.b, ex1.cx);
        CHECK(n > 0 && bad == 0, "the letter sits centred just above its candle's high (4 px gap), never on the candle");
    }
    // ---- re-read only on change; a half-written last line
    {
        MarksState* s = st(ES3); const long r0 = s->reads, k0 = s->stats;
        for (int k = 0; k < 30; ++k) { g_now += 2; sel(ES3); RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = tidOf[ES3.id]; TF->timer(&ev); }
        printf("60 s with no change: %ld reads (stat calls %ld)\n", s->reads - r0, s->stats - k0);
        CHECK(s->reads == r0, "an unchanged file is never re-read (only its size / time checked every 2 s)");
        tfl::Mark m; m.barT = g_bars.back(); m.kind = 'A'; m.dir = -1; m.px = 20500; m.lo = m.hi = m.px; m.mult = 3.6; m.minuteT = m.barT - 120; m.why = "buyers absorbed (bar node)";
        const std::string ln = tfl::SignalBook::line(m, "2.0.3");
        { std::ofstream o(pathFor("ES", sid, 180).c_str(), std::ios::app | std::ios::binary); o << ln.substr(0, ln.size() / 2); }
        tick({&ES3}, false); auto f1 = frame(ES3, 12);
        { std::ofstream o(pathFor("ES", sid, 180).c_str(), std::ios::app | std::ios::binary); o << ln.substr(ln.size() / 2) << "\n"; }
        tick({&ES3}, false); auto f2 = frame(ES3, 12);
        m.why = "buyers absorbed (bar node)";
        const bool halfHidden = !f1.count(m.barT), fullShown = f2.count(m.barT) && f2[m.barT].label == "A?";
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
    // ---- (2.0.4) a REAL 2.0.0 record (20-s method, no multiple): nothing drawn, every old A line skipped and counted in the status
    {
        const char* real = "/mnt/user-data/uploads/lsFlexLevels/TapeFlow/ES-signals-20735.csv";
        std::ifstream in(real, std::ios::binary);
        if (!in.good()) printf("(real 2.0.0 record) SKIPPED - %s not here\n", real);
        else {
            std::stringstream ss; ss << in.rdbuf(); const std::string text = ss.str();
            long aLines = 0; { std::stringstream q(text); std::string ln; while (std::getline(q, ln)) if (ln.find("|A|") != std::string::npos) aLines++; }
            static ChartCtx EU3{7, "EU6", "EU6Z26", 180}; bars(EU3, s0 + 180, g_now);
            { std::ofstream o(pathFor("EU", tfl::sessionOf(g_now), 180).c_str(), std::ios::binary); o << text; }
            attach(EU3); tick({&EU3}, false); auto f = frame(EU3, 12);
            const std::string st = slurp("/tmp/tfmk/home\\InvestorRT\\rtx\\lsFlexLevels\\TapeFlowMarks.status-EU.txt");
            const size_t a = st.find("OLD_RECORDS_SKIPPED,"); const long counted = a == std::string::npos ? -1 : std::atol(st.c_str() + a + 20);
            printf("real 2.0.0 record (ES-signals-20735, %ld A lines): drawn %zu, status OLD_RECORDS_SKIPPED %ld\n", aLines, f.size(), counted);
            CHECK(f.empty() && aLines > 0 && counted == aLines, "a real 2.0.0 record draws nothing; every old A line is skipped and counted in the status");
            const long w0 = st0(EU3); for (int k = 0; k < 15; ++k) tick({&EU3}, false);
            CHECK(st0(EU3) == w0, "the status file is rewritten only when its content changes (30 s unchanged: 0 writes)");
            sel(EU3); static_cast<cppExtension*>(TF)->destroy();
        }
    }
    // ---- two charts on one object: ES + NQ, ES 3-min + ES 60-min
    {
        auto fe = frame(ES3, 12), fn = frame(NQ3, 12), f60 = frame(ES60, 12);
        auto exN = expected(slurp(pathFor("NQ", sid, 180))), ex60 = expected(slurp(pathFor("ES", sid, 3600)));
        bool nqOk = !fn.empty(), e60Ok = !f60.empty(), esOk = true;
        auto exE = expected(slurp(pathFor("ES", sid, 180))); { auto p = expected(slurp(pathFor("ES", sid - 1, 180))); exE.insert(p.begin(), p.end()); }
        for (auto& kv : fn) { auto it = exN.find(kv.first); if (it == exN.end() || kv.second.label != want(it->second)) nqOk = false; }
        for (auto& kv : f60) { auto it = ex60.find(kv.first); if (it == ex60.end() || kv.first % 3600) e60Ok = false; }
        for (auto& kv : fe) { auto it = exE.find(kv.first); if (it == exE.end() || kv.second.label != want(it->second)) esOk = false; }   // only ES's own record on the ES chart
        printf("ES chart %zu letters (all from ES's record: %s), NQ chart %zu (all from NQ's: %s), ES 60-min %zu (all from the -3600s record: %s)\n", fe.size(), esOk ? "yes" : "NO", fn.size(), nqOk ? "yes" : "NO", f60.size(), e60Ok ? "yes" : "NO");
        CHECK(nqOk && esOk && e60Ok, "ES + NQ and ES 3-min + 60-min on one object: each chart draws only its own market's / bar size's record");
    }
    // ---- the 17:00 roll: today's marks become 'previous' and stay drawn; the new session's file is picked up
    {
        const auto before = frame(ES3, 12);
        while (g_now < DAY0 + 17 * 3600 + 120) tick({&ES3}, false);
        auto f = frame(ES3, 12); int kept = 0; for (auto& kv : before) if (f.count(kv.first) && f[kv.first].label == kv.second.label) kept++;
        tfl::Mark m; m.barT = DAY0 + 17 * 3600 + 180; m.kind = 'A'; m.dir = 1; m.px = 20300; m.lo = m.hi = m.px; m.mult = 2.5; m.why = "sellers absorbed (bar node)";
        { std::ofstream o(pathFor("ES", sid + 1, 180).c_str(), std::ios::binary); o << tfl::SignalBook::header() << "\n" << tfl::SignalBook::line(m, "2.0.3") << "\n"; }
        while (g_now < DAY0 + 17 * 3600 + 200) tick({&ES3}, false);
        auto f2 = frame(ES3, 12);
        printf("17:00 roll: %d of %zu marks still drawn from yesterday's file (visible window moved %zu bars); new session's first A drawn: %s\n", kept, before.size(), (size_t)((g_now - (DAY0 + 9 * 3600)) / 180), f2.count(m.barT) ? "yes" : "NO");
        bool stillThere = true; const long long first = g_bars[g_bars.size() - 160]; for (auto& kv : before) if (kv.first >= first && !f.count(kv.first)) stillThere = false;
        CHECK(stillThere && f2.count(m.barT) && f2[m.barT].label == "A?", "the roll keeps the previous session's marks and picks up the new session's file");
    }
    // ---- (leaks) 50 charts created, drawn, switched between ES and NQ, destroyed - the states and timers are released every time
    {
        size_t regBefore = 0; { std::lock_guard<std::mutex> g(regMx()); regBefore = timerReg().size(); }
        for (int k = 0; k < 50; ++k) {
            ChartCtx C{100 + k, k % 2 ? "NQ" : "ES", k % 2 ? "NQZ26" : "ESZ26", 180}; bars(C, g_now - 3600 * 3, g_now);
            TapeFlowMarks* x = static_cast<TapeFlowMarks*>(CreateExtension()); x->setup();
            sel(C); static_cast<cppExtension*>(x)->calc(0); x->draw();
            g_root = k % 2 ? "ES" : "NQ"; g_sym = k % 2 ? "ESZ26" : "NQZ26"; x->draw(); static_cast<cppExtension*>(x)->calc(0); x->draw();   // the chart switches symbol
            static_cast<cppExtension*>(x)->destroy(); delete x;
        }
        size_t regAfter = 0; { std::lock_guard<std::mutex> g(regMx()); regAfter = timerReg().size(); }
        printf("50 charts created / switched / destroyed: timer registry %zu -> %zu\n", regBefore, regAfter);
        CHECK(regAfter == regBefore, "50 charts: every state and timer released (LeakSanitizer reports the memory)");
    }
    for (auto* c : {&ES3, &NQ3, &ES60, &GC3, &CL3, &HG3}) { sel(*c); static_cast<cppExtension*>(TF)->destroy(); }
    { sel(ES3); RTX_EVENT ev; memset(&ev, 0, sizeof(ev)); ev.v.timer.id = tidOf[ES3.id]; CHECK(TF->timer(&ev) == RTX_FAIL, "a removed chart's timer does nothing"); }
    delete TF;
    printf("%d passed, %d failed\n", npass_, fails);
    return fails ? 1 : 0;
}
