// test_cvd_host.cpp -- lsCVD 1.3.0 (1.2.0 + the settings window: show / hide + colour per drawn element; 1.1.0 + the audit "?" marks) on the mock host (BUILD-LESSONS proofs). From tests/:  g++ -std=c++17 -Wall -Wextra -Imock -I../plugin test_cvd_host.cpp -o th && ./th
#include "CVD.cpp"
#include <cstdio>
#include <random>
#include <sstream>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)

// weekday sessions of 3-minute bars, bar END times 17:03 .. 16:00 Central (mock clock = gmtime of RTDATE)
static std::vector<mock::Bar> sessions(int n, unsigned seed, double start = 7000.0) {
    std::mt19937 rng(seed); std::normal_distribution<double> step(0, 1.5); std::uniform_int_distribution<int> vol(200, 1200), skew(-150, 150);
    std::vector<mock::Bar> out; double p = start;
    long day = svl::daysFromCivil(2026, 9, 1);                         // a Tuesday session; its evening starts 2026-08-31 17:00
    for (int s = 0; s < n; ) {
        const int wd = svl::weekdayOf(day);
        if (wd == 0 || wd == 6) { ++day; continue; }
        const long long t0 = (long long)(day - 1) * 86400LL + 17 * 3600;
        for (int k = 1; k <= 460; ++k) {
            mock::Bar b; b.time = (unsigned long)(t0 + k * 180);
            const double o = p; p = std::round((p + step(rng) * 0.25 * 4) / 0.25) * 0.25;
            b.open = (float)o; b.close = (float)p; b.high = (float)(std::max(o, p) + 0.25 * (k % 3)); b.low = (float)(std::min(o, p) - 0.25 * ((k + 1) % 3));
            const long v = vol(rng), d = skew(rng) + (p > o ? 80 : p < o ? -80 : 0);
            const long buy = std::max(0L, (v + d) / 2), sell = std::max(0L, v - buy);
            b.volume = (unsigned long)(buy + sell); b.rows.push_back(mock::Price(b.close, buy, sell, buy + sell));
            out.push_back(b);
        }
        ++s; ++day;
    }
    return out;
}
struct Chart {
    mock::Host h; CVD ind;
    Chart(const std::string& sym = "ESZ6", const std::string& root = "ES", float tick = 0.25f) { h.symbol = sym; h.root = root; h.tick = tick; h.seconds = 180; }
    void use() { mock::current = &h; }
    void run() { use(); ind.calc(0); }
    void draw() { use(); h.draws.clear(); ind.draw(); }
    RuntimeState* st() { use(); return ind.stateFor(); }
};
static unsigned long g_from = 0;   // compare marks from this bar time on (both runs then have the full context)
static std::string sig(const RuntimeState& s) {
    std::string o;
    for (const cvdf::Mark& m : s.feed.marks) if ((unsigned long)m.raw >= g_from) o += std::to_string(m.raw) + "|" + m.e.id + "|" + cvd::name(m.e.state) + "|" + std::to_string(m.e.cvd) + "\n";
    return o;
}

int main() {
    char tmpl[] = "/tmp/cvdhXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::vector<mock::Bar> all = sessions(8, 7);
    g_from = all[5 * 460].time;                                        // sessions 6-8: inside the indicator's 8-day load window either way
    // 1. bar by bar: CVD = cumulative buy - sell from the session start; first (partial) session shows nothing
    Chart a; a.h.bars.assign(all.begin(), all.begin() + 3);
    for (size_t i = 3; i <= all.size(); ++i) { if (i > 3) a.h.bars.push_back(all[i - 1]); a.run(); }
    RuntimeState* s = a.st();
    CHECK(s && s->market == "ES", "identified ES");
    CHECK(s->lastRaw == (long long)all[all.size() - 2].time, "the forming (last) bar is never read");
    { const cvdf::Out& o = s->feed.out[(long long)all[100].time]; CHECK(!o.counted && !std::isfinite(o.cvd), "first session (no full history) has no CVD"); }
    {   // session 3, bar 200: sum from its first bar
        const size_t first = 2 * 460, at = first + 200; double cum = 0;
        for (size_t i = first; i <= at; ++i) cum += (double)all[i].rows[0].buy - (double)all[i].rows[0].sell;
        const cvdf::Out& o = s->feed.out[(long long)all[at].time];
        CHECK(o.counted && std::fabs(o.cvd - cum) < 1e-9, "CVD = cumulative (ask - bid traded) from the 17:00 session start");
        const double pct = 100.0 * ((double)all[at].rows[0].buy - (double)all[at].rows[0].sell) / (double)all[at].volume;
        CHECK(std::fabs(o.pct - pct) < 1e-9, "delta % = 100 x delta / bar volume");
    }
    CHECK(std::isfinite(s->feed.rangeTicks), "a per-session range scale from prior sessions");
    // 2. forming bar changes never change anything
    { Chart b; b.h.bars = all; b.run(); const std::string before = sig(*b.st());
      b.h.bars.back().rows[0].buy += 5000; b.h.bars.back().volume += 5000; b.run(); CHECK(sig(*b.st()) == before, "forming-bar changes ignored"); }
    // 3. replay = live: one pass over the whole chart gives the same marks and CVD as bar-by-bar (#11, #28)
    { Chart b; b.h.bars = all; b.run(); CHECK(sig(*b.st()) == sig(*s), "one pass == bar by bar (no repaint)");
      CHECK(b.st()->feed.out[(long long)all[3000].time].cvd == s->feed.out[(long long)all[3000].time].cvd, "same CVD values"); }
    // 4. restart after every 50 bars (a fresh indicator each time, like reloading IRT) gives the same result (#11)
    {   bool same = true;
        for (size_t n = 600; n <= all.size(); n += 50) {
            if (n < 6 * 460) continue;                                 // compare once the compared sessions exist
            Chart c; c.h.bars.assign(all.begin(), all.begin() + n); c.run();
            Chart d; d.h.bars.assign(all.begin(), all.begin() + std::min(n, (size_t)40)); for (size_t k = 40; k <= n; ++k) { if (k > 40) d.h.bars.push_back(all[k - 1]); d.run(); }
            if (sig(*c.st()) != sig(*d.st())) { same = false; break; }
        }
        CHECK(same, "restart at any point == continuous run"); }
    // 5. two charts on one DLL (same and different markets) do not share state (#1, #9)
    {   Chart e("ESZ6", "ES"), g("GCZ6", "GC", 0.1f);
        std::vector<mock::Bar> gc = sessions(8, 11, 4000.0); for (mock::Bar& b : gc) { b.open = (float)(b.open * 0.5); b.close = (float)(b.close * 0.5); b.high = (float)(b.high * 0.5); b.low = (float)(b.low * 0.5); if (b.low > std::min(b.open, b.close)) b.low = std::min(b.open, b.close); if (b.high < std::max(b.open, b.close)) b.high = std::max(b.open, b.close); }
        e.h.bars = all; g.h.bars = gc; e.run(); g.run(); e.run();
        CHECK(e.st()->market == "ES" && g.st()->market == "GC" && e.st() != g.st(), "separate state per chart");
        CHECK(sig(*e.st()) == sig(*s), "ES unaffected by the GC chart"); }
    // 6. volume-at-price over the bar volume: up to 2% + 1 is the same trades (accepted); more taints the session (no CVD)
    {   std::vector<mock::Bar> v = all; v[2000].rows[0].buy += 5; Chart c; c.h.bars = v; c.run();
        CHECK(c.st()->feed.overTotal >= 1 && std::isfinite(c.st()->feed.out[(long long)v[2000].time].cvd), "small VAP excess accepted");
        v = all; v[2000].rows[0].buy += 900; Chart d; d.h.bars = v; d.run();
        CHECK(!std::isfinite(d.st()->feed.out[(long long)v[2001].time].cvd), "large VAP excess: no CVD for the rest of that session");
        CHECK(std::isfinite(d.st()->feed.out[(long long)v[5 * 460 + 10].time].cvd), "next session recovers"); }
    // 7. VAP not loaded on the newest closed bar: wait (nothing committed), then use it once it arrives
    {   std::vector<mock::Bar> v(all.begin(), all.begin() + 2500); v[2498].failStats = 1; Chart c; c.h.bars = v; c.run();
        CHECK(c.st()->lastRaw == (long long)v[2497].time, "waits on a missing newest VAP");
        CHECK(c.st()->state == "WAITING FOR ORDER-FLOW DATA", "says why");
        c.h.bars[2498].failStats = 0; c.run();
        CHECK(c.st()->lastRaw == (long long)v[2498].time && std::isfinite(c.st()->feed.out[(long long)v[2498].time].cvd), "then reads it"); }
    // 8. old bars without VAP are not waited for: counted missing (the session shows no CVD)
    {   std::vector<mock::Bar> v = all; v[1500].failStats = 1; Chart c; c.h.bars = v; c.run();
        CHECK(c.st()->lastRaw == (long long)v[v.size() - 2].time && c.st()->feed.badBars >= 1, "history VAP failure does not block"); }
    // 9. IRT trims the oldest bars: the frontier is found again by time; nothing is fed twice
    {   Chart c; c.h.bars.assign(all.begin(), all.begin() + 3000); c.run(); const size_t m0 = c.st()->feed.marks.size();
        c.h.bars.erase(c.h.bars.begin(), c.h.bars.begin() + 500); c.h.bars.push_back(all[3000]); c.run();
        CHECK(c.st()->lastRaw == (long long)all[2999].time && c.st()->feed.marks.size() >= m0, "trim handled by bar time"); }
    // 10. unknown market / micro: says so, draws only the header
    {   Chart c("MESZ6", "MES"); c.h.bars = all; c.run(); CHECK(c.st()->fault && c.st()->state.find("UNKNOWN MARKET") == 0, "micro rejected"); }
    // 11. drawing: header readout, labels never overlap each other, every label box inside the pane, no drawing in a tiny pane
    {   Chart c; c.h.bars = all; c.h.barPx = 3; c.h.firstBar = (int)all.size() - 600; c.run(); c.draw();
        bool header = false; for (const mock::Draw& d : c.h.draws) if (d.type == "text" && d.text.find("CVD: ") == 0) header = true;
        CHECK(header, "header 'CVD: now'");
        std::vector<mock::Draw> boxes; for (const mock::Draw& d : c.h.draws) if (d.type == "box" && d.r - d.x > 12 && d.b - d.y == 14) boxes.push_back(d);
        bool overlap = false, outside = false;
        for (size_t i = 0; i < boxes.size(); ++i) {
            if (boxes[i].y < c.h.paneTop || boxes[i].b > c.h.paneBottom) outside = true;
            for (size_t j = i + 1; j < boxes.size(); ++j)
                if (boxes[i].x < boxes[j].r && boxes[j].x < boxes[i].r && boxes[i].y < boxes[j].b && boxes[j].y < boxes[i].b) overlap = true;
        }
        std::printf("  labels drawn: %zu, marks: %zu\n", boxes.size(), c.st()->feed.marks.size());
        CHECK(!overlap, "labels never overlap"); CHECK(!outside, "labels inside the pane");
        c.h.paneBottom = 40; c.draw(); CHECK(c.h.draws.empty(), "pane too short: nothing drawn"); }
    // 12. status file written once, keyed by market and bar size
    {   std::ifstream in((std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\CVD.status-ES-180.txt").c_str()); std::string l; std::getline(in, l);
        CHECK(l == "VERSION,1.3.0", "status file"); }
    // 13. (1.1.0) the chart shows only shown marks: confirmed, at PDH/PDL/PFH/PFL, stop within 4.0x the bar range; no '?' and no 'x'
    {   Chart c; c.h.bars = all; c.h.barPx = 1; c.h.firstBar = 0; c.run(); c.draw();
        int labels = 0; bool warn = false;
        // (1.2.0, audit list 18:25 CT) the shown marks read "Div?" / "Exh?" / "BR?"; a plain "Div" / "Exh" / "BR" is never drawn
        bool plain = false;
        for (const mock::Draw& d : c.h.draws) if (d.type == "text" && (d.text == "Div" || d.text == "Exh" || d.text == "BR")) plain = true;
        CHECK(!plain, "1.2.0: no plain Div / Exh / BR - every shown mark carries its ?");
        for (const mock::Draw& d : c.h.draws) if (d.type == "text" && (d.text == "Div?" || d.text == "Exh?" || d.text == "BR?" || d.text.compare(0, 3, "Abs") == 0 || d.text.find(" x") != std::string::npos)) {
            ++labels; if (d.text.find(" x") != std::string::npos || d.text.compare(0, 3, "Abs") == 0) warn = true; }
        int shown = 0; bool rule = true, abs = false;
        for (const cvdf::Mark& m : c.st()->feed.marks) {
            if (m.e.type == cvd::Type::Abs) abs = true;
            if (!m.shown) continue;
            ++shown;
            if (m.e.state != cvd::State::Confirmed || !cvdf::shownLevel(m.e.level_id) || !(m.risk <= 4.0 + 1e-9)) rule = false;
        }
        std::printf("  shown %d of %zu marks, labels drawn %d\n", shown, c.st()->feed.marks.size(), labels);
        CHECK(!warn, "no warnings / expired / Abs on the chart"); CHECK(labels <= shown, "only shown marks are drawn"); CHECK(rule, "every shown mark meets the rule");
        CHECK(!abs, "Abs is off"); CHECK(c.st()->feed.shownCount == shown, "shown count"); }
    CHECK(cvdf::shownLevel("PFL/PDL@31263|S") && cvdf::shownLevel("ONH/PDH@1|R") && !cvdf::shownLevel("ONH/LonHI@31415|R") && !cvdf::shownLevel("CR@100|S"), "level rule");
    // 14. nightly range rows apply from their own session on: a future row changes nothing already drawn, a past row does
    {   Chart base; base.h.bars = all; base.run(); const std::string before = sig(*base.st());
        const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-CVD-ranges.csv";
        { std::ofstream o(flex.c_str()); o << "ES,2030-01-01,40\nGC,2020-01-01,99\nbad line\n"; }
        Chart fut; fut.h.bars = all; fut.run();
        CHECK(sig(*fut.st()) == before && fut.st()->feed.rangeTicks == 15, "a row for a future session changes nothing");
        { std::ofstream o(flex.c_str()); o << "ES,2020-01-01,30\n"; }
        Chart past; past.h.bars = all; past.run();
        CHECK(past.st()->feed.rangeTicks == 30 && past.st()->feed.cfg.stall_range_ticks == 16, "a row in force rescales (8 x 30/15)");
        std::remove(flex.c_str()); }
    // 15. the nightly verdict: proven drops 'testing'; retired hides every mark and says so
    {   const std::string vf = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-CVD-verdict.csv";
        auto drawn = [&](const char* v, bool& testingTag, bool& offTag, int& labels) {
            { std::ofstream o(vf.c_str()); o << v; }
            Chart c; c.h.bars = all; c.h.barPx = 1; c.h.firstBar = 0; c.run(); c.draw(); testingTag = offTag = false; labels = 0;
            for (const mock::Draw& d : c.h.draws) if (d.type == "text") { if (d.text == "testing") testingTag = true; if (d.text.find("marks off") == 0) offTag = true;
                if (d.text == "Div?" || d.text == "Exh?" || d.text == "BR?") ++labels; }
        };
        bool t = false, off = false; int n = 0, n0 = 0;
        drawn("VERDICT,testing\n", t, off, n0); CHECK(t && !off && n0 > 0, "testing: tag and marks");
        drawn("VERDICT,proven\n", t, off, n); CHECK(!t && !off && n == n0, "proven: no tag, same marks");
        drawn("VERDICT,retired\n", t, off, n); CHECK(!t && off && n == 0, "retired: no marks, says so");
        drawn("VERDICT,garbage\n", t, off, n); CHECK(t && n == n0, "unknown verdict = testing");
        std::remove(vf.c_str()); }
    // 16. (1.3.0) the settings window and file: show / hide + colour for the line, the delta % bars and the marks
    {   const std::string sf = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\CVD.settings-ES.txt";
        std::remove(sf.c_str());
        auto run = [&](Chart& c) { c.h.bars = all; c.h.barPx = 1; c.h.firstBar = 0; c.run(); c.draw(); };
        auto lines = [](const Chart& c, COLOR col) { int n = 0; for (const mock::Draw& d : c.h.draws) if (d.type == "line" && d.color == col) ++n; return n; };
        auto boxes = [](const Chart& c, COLOR col) { int n = 0; for (const mock::Draw& d : c.h.draws) if (d.type == "box" && d.color == col) ++n; return n; };
        Chart a; run(a);
        CHECK(lines(a, 0x60A5FA) > 10 && boxes(a, 0x205C49) + boxes(a, 0x5E2B35) > 10 && boxes(a, 0x34D399) + boxes(a, 0xF87171) > 0, "no file: what 1.2.0 drew (blue line, muted bars, green / red marks)");
        { std::ofstream o(sf.c_str()); o << "SPEC1|CVD|1.3.0|ES\nshow_line=1\nline_color=FFFF00\nshow_bars=0\nbar_up=205C49\nbar_dn=5E2B35\nshow_marks=1\nmark_bull=0000FF\nmark_bear=FF00FF\n"; }
        Chart b; run(b);
        CHECK(lines(b, 0x60A5FA) == 0 && lines(b, 0xFFFF00) > 10, "Line colour from the file");
        CHECK(boxes(b, 0x205C49) + boxes(b, 0x5E2B35) == 0, "Delta % bars off: none drawn");
        CHECK(boxes(b, 0x34D399) + boxes(b, 0xF87171) == 0 && boxes(b, 0x0000FF) + boxes(b, 0xFF00FF) > 0, "mark colours from the file");
        { std::ofstream o(sf.c_str()); o << "SPEC1|CVD|1.3.0|ES\nshow_line=0\nshow_marks=0\n"; }
        Chart d; run(d);
        CHECK(lines(d, 0x60A5FA) == 0 && boxes(d, 0x34D399) + boxes(d, 0xF87171) == 0 && boxes(d, 0x205C49) + boxes(d, 0x5E2B35) > 10, "line and marks off; bars back on (missing key = default)");
        { std::ofstream o(sf.c_str()); o << "garbage\n"; }
        Chart g; run(g); CHECK(lines(g, 0x60A5FA) > 10, "a bad file = the defaults");
        // the window: rows from the table, filled from the file, Apply saves only from the window (never from the chart copy)
        std::remove(sf.c_str());
        Chart w; w.use(); w.ind.setup();
        CHECK(cppExtension::P().size() == 8 && cppExtension::pver() == 2, "setup registers the 8 rows (parameter version 2)");
        w.h.period = ""; w.ind.parmsLoad();
        CHECK(w.ind.getParameterColor(1) == 0x60A5FAul && w.ind.isBoxChecked(2) == 1, "the window opens with the defaults");
        { std::ifstream in(sf.c_str()); CHECK(!in, "opening the window never writes"); }
        w.ind.checkBox(2, 0); w.ind.setParameterColor(6, 0x123456); w.ind.parmsApply();
        std::string t; { std::ifstream in(sf.c_str()); std::stringstream ss2; ss2 << in.rdbuf(); t = ss2.str(); }
        CHECK(t.find("\nshow_bars=0\n") != std::string::npos && t.find("\nmark_bull=123456\n") != std::string::npos && t.find("\nline_color=60A5FA\n") != std::string::npos, "Apply saves the changes, the rest at their values");
        w.h.period = "3 Minutes"; std::remove(sf.c_str()); w.ind.parmsApply(); { std::ifstream in(sf.c_str()); CHECK(!in, "the chart copy never saves"); }
        std::remove(sf.c_str()); }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
