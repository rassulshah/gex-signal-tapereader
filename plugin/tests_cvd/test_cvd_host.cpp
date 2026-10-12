// test_cvd_host.cpp -- lsCVD 1.0.0 on the mock host (BUILD-LESSONS proofs). From tests/:  g++ -std=c++17 -Wall -Wextra -Imock -I../plugin test_cvd_host.cpp -o th && ./th
#include "CVD.cpp"
#include <cstdio>
#include <random>
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
        CHECK(l == "VERSION,1.0.0", "status file"); }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
