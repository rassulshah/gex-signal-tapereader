// test_rules284.cpp -- lsDeltaProfile 2.8.4: the 2.8.x A rules (Rassul 2026-10-14) on the mock SDK.
//   2(c) halfway rule, 2(d) 1-bar ZigZag pivot rule (bars 0..peak only), 2(e) decided once at the peak bar's close, never repainted,
//   bubbles in confirmation order (a new A never evicts an old one), F) rendered check: only A labels and their delta text.
//   The 2.8.3 bug this suite proves fixed: a node first seen AFTER its peak bar closed (chart opened / IRT restarted / the node
//   reached 2.0x or the top 3 later) took classify()'s A, which never applied the pivot rule.
// Build (Linux, from tests_delta):  g++ -std=c++17 -g -fsanitize=address,undefined -I. test_rules284.cpp -o t && ./t
#include "../DeltaProfile.cpp"
#include <iostream>
#include <functional>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <set>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) { ++passed; std::cout << "PASS " << why << '\n'; } else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string home(const std::string& name) {
    const char* r = std::getenv("DP254_RUN"); const std::string root = r && r[0] ? r : "/tmp/dp254-run";
    const std::string h = root + "/r284_" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    return h + "/InvestorRT/rtx/lsFlexLevels/DeltaProfile/";
}
static std::vector<std::vector<std::string> > rows(const std::string& text, const char* event) {
    std::vector<std::vector<std::string> > out; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if ((c.size() == REC_FIELDS || c.size() == REC_FIELDS_CONF) && c[12] == event) out.push_back(c); }
    return out;
}
static std::vector<mock::Draw> rings(const mock::Host& h) { std::vector<mock::Draw> o; for (const mock::Draw& d : h.draws) if (d.type == "ring") o.push_back(d); return o; }
static std::vector<std::string> textsOf(const mock::Host& h) { std::vector<std::string> o; for (const mock::Draw& d : h.draws) if (d.type == "text") o.push_back(d.text); return o; }
static bool starts(const std::string& s, const char* p) { return s.compare(0, std::strlen(p), p) == 0; }
static LatchBook& book() { return latchBooks().begin()->second; }

static const unsigned long DAY = 1791504000UL;                      // 2026-10-09 00:00 (wall clock fields, TZ=UTC)
static const unsigned long T0 = DAY + 9 * 3600 + 30 * 60;           // 09:30
static const long long SESSION = 20735;
static std::string recordPath(const std::string& dir) { return dir + "ES-ESZ26-60-signals-" + std::to_string(SESSION) + ".csv"; }

// A 1-min ES chart (tick 0.25) for the pivot rule:
//   bars 0..19  a down leg (low 120 - i, high low + 2): bar 19 is the down pivot (low 101, high 103)
//   bars 20..   an up leg   (low 102 + (i - 20), high low + 2), lows rising: no flip
//   bar `peak` (in the up leg) holds 10,000 lots SOLD at `nodePx`, green open->close; its low / high given
//   after the peak: rising bars (low = peakHigh - 3 + 0.25 k) that never trade the node again
// every other bar: 1 lot bought at its close (rows inside the bar)
struct Peak { int at; double low, high, open, close, nodePx; };
static mock::Host upLeg(int count, const Peak& pk) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.symbol = "ESZ26"; h.tick = 0.25f;
    for (int i = 0; i < count; ++i) {
        mock::Bar b; b.time = T0 + static_cast<unsigned long>(i * 60);
        double lo;
        if (i < 20) lo = 120 - i;
        else if (i < pk.at) lo = 102 + (i - 20);
        else lo = pk.high - 3 + 0.25 * (i - pk.at);
        b.low = static_cast<float>(lo); b.high = static_cast<float>(lo + 2);
        b.open = static_cast<float>(lo + 0.5); b.close = static_cast<float>(lo + 1.5);
        b.rows = { mock::Price(b.close, 1, 0, 1) }; b.volume = 1;
        h.bars.push_back(b);
    }
    mock::Bar& p = h.bars[pk.at];
    p.low = static_cast<float>(pk.low); p.high = static_cast<float>(pk.high); p.open = static_cast<float>(pk.open); p.close = static_cast<float>(pk.close);
    p.rows = { mock::Price(static_cast<float>(pk.nodePx), 0, 10000, 10000), mock::Price(p.close, 1, 0, 1) }; p.volume = 10001;
    return h;
}
static void freshDraw(DeltaProfile& p, mock::Host& h) { h.draws.clear(); mock::current = &h; if (h.data) static_cast<ChartState*>(h.data)->builtAt -= 100000; p.draw(); }
static std::string verLiteral() {   // the setVersion("x.y.z") literal gex-build.bat reads with findstr
    const std::string src = slurp("../DeltaProfile.cpp"); const std::size_t at = src.find("setVersion(\"");
    if (at == std::string::npos) return "";
    const std::size_t b = at + 12, e = src.find('"', b); return src.substr(b, e - b);
}

int main()
{
    setenv("TZ", "UTC", 1); tzset();
    check(std::string(DLT_VERSION) == "2.8.5", "DLT_VERSION is 2.8.5");
    check(verLiteral() == DLT_VERSION, "setVersion literal (" + verLiteral() + ") = DLT_VERSION (gex-build reads the literal)");

    // ---- 2(d) pivotOk unit tests (pure function, plain arrays) ----
    {
        //            0    1    2    3    4    5    6
        std::vector<double> H = { 110, 109, 108, 107, 109, 111, 113 };
        std::vector<double> L = { 108, 107, 106, 104, 106, 108, 110 };
        int piv = -2;
        check(dp::pivotOk(H, L, 3, true, &piv) && piv == 3, "bullish: the bar that makes the down leg's low creates the down pivot");
        check(dp::pivotOk(H, L, 4, true, &piv) && piv == 3, "bullish: bar 4 low 106 < down-pivot bar 3 high 107 -> pass");
        check(!dp::pivotOk(H, L, 5, true, &piv) && piv == 3, "bullish: bar 5 low 108 >= down-pivot bar 3 high 107 -> fail");
        check(!dp::pivotOk(H, L, 6, true), "bullish: bar 6 (low 110) mid up-leg -> fail");
        // later bars never change the answer at bar 5 (only bars 0..pk are read)
        std::vector<double> H2 = H, L2 = L; H2.push_back(105); L2.push_back(100); H2.push_back(104); L2.push_back(99);
        check(!dp::pivotOk(H2, L2, 5, true) && dp::pivotOk(H2, L2, 4, true), "bars after the peak never change the pivot answer");
        // bearish mirror: a rally to bar 3, then down
        std::vector<double> Hb = { 100, 101, 102, 105, 103, 101, 99 };
        std::vector<double> Lb = { 98,  99,  100, 103, 101, 99,  97 };
        check(dp::pivotOk(Hb, Lb, 3, false, &piv) && piv == 3, "bearish: the bar that makes the up leg's high creates the up pivot");
        check(!dp::pivotOk(Hb, Lb, 4, false, &piv) && piv == 3, "bearish: bar 4 high 103 is NOT above up-pivot bar 3 low 103 (equal) -> fail");
        check(!dp::pivotOk(Hb, Lb, 5, false, &piv) && piv == 3, "bearish: bar 5 high 101 <= up-pivot bar 3 low 103 -> fail");
        std::vector<double> Hn = { 1, 2 }, Ln = { 0, std::nan("") };
        check(!dp::pivotOk(Hn, Ln, 1, true), "NaN low -> fail (never A on bad data)");
        check(!dp::pivotOk(H, L, 0, true), "bar 0 has no ZigZag history -> fail");
        // parity with CobImpulseLogic::legPivot's walk: an up leg's start is the down pivot
        check(dp::pivotOk(H, L, 6, false, &piv) && piv == 6, "bearish at bar 6: it makes the up leg's high = creates the up pivot");
    }
    // ---- 2(c) halfwayOk unit tests ----
    {
        // a bar 100.00 - 102.00 (tick 0.25): mid 101.00 = key 404
        check(dp::halfwayOk(400, 401, 102, 100, 0.25, true), "bullish: node 100.00-100.25 in the lower half -> pass");
        check(dp::halfwayOk(403, 405, 102, 100, 0.25, true), "bullish: node middle exactly at the halfway mark -> pass (at or below)");
        check(!dp::halfwayOk(405, 406, 102, 100, 0.25, true), "bullish: node above the halfway mark -> fail");
        check(dp::halfwayOk(406, 407, 102, 100, 0.25, false), "bearish: node in the upper half -> pass");
        check(dp::halfwayOk(404, 404, 102, 100, 0.25, false), "bearish: node exactly at the halfway mark -> pass (at or above)");
        check(!dp::halfwayOk(400, 401, 102, 100, 0.25, false), "bearish: node in the lower half -> fail");
        check(!dp::halfwayOk(400, 401, std::nan(""), 100, 0.25, true), "unknown high -> fail");
        // float ticks that are not exact in binary (IRT hands float prices / tick): a node exactly AT the halfway mark passes
        { const float t = 0.1f, lo = 4214.3f, hi = 4214.7f; const dp::Tick k = dp::priceKey(4214.5f, t);
          check(dp::halfwayOk(k, k, hi, lo, t, true) && dp::halfwayOk(k, k, hi, lo, t, false), "GC float tick 0.1: node AT the halfway mark passes both ways (2.8.3 refused it)"); }
        { const float t = 0.0005f, lo = 4.999f, hi = 5.001f; const dp::Tick k = dp::priceKey(5.0f, t);
          check(dp::halfwayOk(k, k, hi, lo, t, true) && dp::halfwayOk(k, k, hi, lo, t, false), "HG float tick 0.0005: node AT the halfway mark passes both ways"); }
    }
    // ---- 2(c) on the core: classify() with the halfway rule ----
    {
        std::vector<dp::Bar> b(2);
        for (int i = 0; i < 2; ++i) { b[i].index = i; b[i].time = 60 * i; b[i].closed = true; }
        b[0].low = 100; b[0].high = 104; b[0].open = 100.5; b[0].close = 103.5; b[0].rows.push_back(dp::Row(404, -500, 500));   // node 101.00 (lower half)
        b[1].low = b[1].high = b[1].open = b[1].close = 103;
        check(dp::classify(b, 404, 404, -500, 0.25).code == "A", "core: green peak bar, close above, node in the lower half -> A");
        b[0].rows[0].key = 412;                                   // node 103.00: upper half of a 100-104 bar
        check(dp::classify(b, 412, 412, -500, 0.25).code != "A", "core: node in the upper half of the green peak bar -> not A");
    }

    // ---- host: the live path (node seen while its peak bar forms) and the late path (first seen after it closed) ----
    // pivot FAILS: peak bar 30 low 112 is above the down-pivot bar 19's high (103); colour / close / halfway all pass
    const Peak fail{ 30, 112, 116, 112.5, 115.5, 112 };
    // pivot PASSES: peak bar 30 low 101.5 < low[29] (111) flips the ZigZag down - this bar creates the down pivot
    const Peak pass{ 30, 101.5, 116, 102, 115.5, 101.5 };
    {
        const std::string dir = home("late_fail");
        mock::Host h = upLeg(110, fail); DeltaProfile p; freshDraw(p, h);
        const LatchBook& bk = book();
        check(bk.v.size() == 1, "late path: one event tracked (" + std::to_string(bk.v.size()) + ")");
        if (!bk.v.empty()) check(bk.v[0].code != "A", "late path: pivot rule applied to a node first seen after its peak bar closed (got " + bk.v[0].code + ")");
        check(rings(h).empty(), "late path: no bubble for a pivot-rule failure");
        check(rows(slurp(recordPath(dir)), "D").empty() || rows(slurp(recordPath(dir)), "D")[0][2] != "A", "late path: no A line recorded");
        for (const std::string& t : textsOf(h)) if (starts(t, "A")) check(false, "late path: an A label was drawn: " + t);
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    {
        const std::string dir = home("late_pass");
        mock::Host h = upLeg(110, pass); DeltaProfile p; freshDraw(p, h);
        const LatchBook& bk = book();
        check(bk.v.size() == 1 && bk.v[0].code == "A" && bk.v[0].support, "late path: pivot passes (bar creates the down pivot) -> A support");
        check(rings(h).size() == 1, "late path: its bubble is drawn");
        const auto D = rows(slurp(recordPath(dir)), "D");
        check(D.size() == 1 && D[0][2] == "A" && D[0][10] == std::to_string(T0 + 30 * 60), "late path: one A line, decided_by_t = the peak bar's end");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }

    // ---- host edge: a peak-bar close exactly ON the node row's top edge (whole ticks) is not beyond it; one tick above is ----
    {
        home("edge_probe");
        Peak e{ 30, 101.5, 116, 101.5, 115.5, 101.5 };
        mock::Host h0 = upLeg(110, e); DeltaProfile p0; freshDraw(p0, h0);
        const dp::Tick top = book().v.empty() ? 0 : book().v[0].high, lowK = book().v.empty() ? 0 : book().v[0].low;
        mock::current = &h0; p0.destroy(); latchBooks().clear();
        check(top > lowK, "fixture: the node row spans more than one tick (" + std::to_string(lowK) + ".." + std::to_string(top) + ")");
        for (int up = 0; up <= 1; ++up) {
            home(up ? "edge_above" : "edge_on");
            Peak q = e; q.close = static_cast<double>(top + up) * 0.25;
            mock::Host h = upLeg(110, q); DeltaProfile p; freshDraw(p, h);
            const bool isA = !book().v.empty() && book().v[0].code == "A" && book().v[0].high == top;
            check(up ? isA : !isA, up ? "host: a green peak close ONE TICK above the row's top edge -> A" : "host: a green peak close exactly ON the row's top edge -> not A");
            mock::current = &h; p.destroy(); latchBooks().clear();
        }
    }
    // ---- 2(e) decided ONCE at the peak bar's close (live, bar by bar) and never repainted ----
    {
        const std::string dir = home("live_once");
        // 200 bars so the window (90 min) is full before the peak; the peak bar is bar 150 (the up leg runs from bar 20)
        Peak pk{ 150, 101.5, 116, 102, 115.5, 101.5 };
        mock::Host full = upLeg(200, pk);
        // flatten bars 20..149 into a gentle up leg ending above 103 so bar 150's low (101.5) flips the ZigZag down
        for (int i = 20; i < 150; ++i) { mock::Bar& b = full.bars[i]; const double lo = 104 + 0.05 * (i - 20);
            b.low = (float)lo; b.high = (float)(lo + 2); b.open = (float)(lo + 0.5); b.close = (float)(lo + 1.5); b.rows = { mock::Price(b.close, 1, 0, 1) }; }
        mock::Host h = full; h.bars.resize(151);                         // bar 150 forming
        DeltaProfile p; freshDraw(p, h);
        check(book().v.size() == 1 && book().v[0].code == "A?", "forming peak bar: pending A? (got " + (book().v.empty() ? std::string("none") : book().v[0].code) + ")");
        check(rings(h).empty(), "forming peak bar: no bubble");
        h.bars.push_back(full.bars[151]); freshDraw(p, h);               // bar 150 closed
        check(book().v.size() == 1 && book().v[0].code == "A", "peak bar closed: A decided at its close");
        const auto r1 = rings(h);
        check(r1.size() == 1, "peak bar closed: one bubble");
        const std::string rec1 = slurp(recordPath(dir));
        // non-repaint: run the same latch book through 40 extra bars with LOWER LOWS (a new down leg below the A, a later
        // lower low that would move the ZigZag's down pivot) and assert the A and its bubble are unchanged
        for (int k = 0; k < 40; ++k) {
            mock::Bar b = h.bars.back(); b.time += 60; const double lo = 100 - 0.5 * k;
            b.low = (float)lo; b.high = (float)(lo + 1); b.open = (float)(lo + 0.75); b.close = (float)(lo + 0.25);
            b.rows = { mock::Price(b.close, 0, 1, 1) }; b.volume = 1; h.bars.push_back(b);
            freshDraw(p, h);
        }
        check(book().v.size() >= 1 && book().v[0].code == "A" && book().v[0].support, "after 40 lower-low bars: the A is still A");
        const auto r2 = rings(h);
        bool same = false; for (const mock::Draw& d : r2) if (d.x == r1[0].x && d.y == r1[0].y && d.color == r1[0].color) same = true;
        check(r1.size() == 1 && same, "after 40 lower-low bars: the bubble is on the same bar and price, same colour");
        const auto D = rows(slurp(recordPath(dir)), "D");
        // as the span grows the row size changes (ticksPerRow 1 -> 2 -> 3), so the same absorption is re-tracked as a new row-event
        // (pre-existing 2.6.0 event identity = row edges + peak bar). Every such line must be the SAME physical A: same peak bar,
        // same side, same price - and they share the one bubble. No A on any other bar may appear.
        std::set<std::string> physical; int aLines = 0;
        for (const auto& c : D) if (c[2] == "A") { ++aLines; physical.insert(c[15] + "|" + c[3] + "|" + c[17]); }
        check(aLines >= 1 && physical.size() == 1, "every A line is the one physical A (peak bar / side / price): " + std::to_string(aLines) + " lines, " + std::to_string(physical.size()) + " distinct");
        check(slurp(recordPath(dir)).compare(0, rec1.size(), rec1) == 0, "the record is append-only: earlier lines unchanged");
        // restart: reload from the record -> the same A and the same bubble
        mock::current = &h; p.destroy(); latchBooks().clear();
        DeltaProfile p2; mock::Host h2 = h; h2.data = NULL; h2.draws.clear(); mock::current = &h2; p2.draw();
        bool again = false; for (const mock::Draw& d : rings(h2)) if (d.x == r1[0].x && d.y == r1[0].y) again = true;
        check(again && book().v[0].code == "A", "restart: the A and its bubble reload unchanged");
        mock::current = &h2; p2.destroy(); latchBooks().clear();
    }

    // ---- bubbles in confirmation order; a new A never evicts an old one ----
    {
        home("order");
        mock::Host h = upLeg(110, pass); DeltaProfile p; freshDraw(p, h);
        LatchBook& bk = book();
        const auto before = rings(h);
        check(before.size() == 1, "fixture: one A bubble");
        // 12 more confirmed A's on the SAME bar and price band, decided later: they may share or be placed beside it, never move it
        for (int k = 0; k < 12; ++k) {
            Latch L = bk.v[0]; L.id = bk.nextId++; L.low += 0; L.high += 0; L.price = bk.v[0].price + 0.25 * (k + 1);
            L.tDecided = bk.v[0].tDecided + 60 * (k + 1); L.logged = true; bk.v.push_back(L);
        }
        freshDraw(p, h);
        const auto after = rings(h);
        bool kept = false; for (const mock::Draw& d : after) if (d.x == before[0].x && d.y == before[0].y) kept = true;
        check(kept, "the first A keeps its bubble position when 12 later A's arrive");
        ChartState& s = *static_cast<ChartState*>(h.data);
        check(s.bubbleState[bk.v[0].id] == "drawn", "the first A is still drawn (not evicted)");
        {   // (2.8.5) the 12 later A's are on the SAME candle and side and decided later: one bubble per candle per side -> merged into it
            int merged = 0; for (std::size_t k = 1; k < bk.v.size(); ++k) if (s.bubbleState[bk.v[k].id] == "merged:" + std::to_string(bk.v[0].id)) ++merged;
            check(merged == 12 && rings(h).size() == 1, "2.8.5: the 12 later same-candle A's are merged into the first bubble (" + std::to_string(merged) + ")"); }
        int suppressed = 0; for (const auto& kv : s.bubbleState) if (kv.second == "suppressed") ++suppressed;
        bool oldestFirst = true;   // any suppressed bubble is a LATER one than every drawn one
        long long lastDrawnT = -1, firstSupT = LLONG_MAX;
        for (const Latch& L : bk.v) { const std::string q = s.bubbleState[L.id]; if (q == "drawn") lastDrawnT = std::max(lastDrawnT, L.tDecided); if (q == "suppressed") firstSupT = std::min(firstSupT, L.tDecided); }
        if (suppressed) oldestFirst = lastDrawnT <= firstSupT;
        check(oldestFirst, "when room runs out only the NEWEST A's lose their bubble (" + std::to_string(suppressed) + " suppressed)");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }

    // ---- F) rendered check: no label or text for non-A nodes; delta text only on LABELLED A nodes ----
    {
        home("render_nonA");
        mock::Host h = upLeg(110, fail); DeltaProfile p; freshDraw(p, h);       // the node is I? (pivot failure)
        for (const std::string& t : textsOf(h)) {
            const bool bad = starts(t, "A") || starts(t, "I") || starts(t, "AE") || starts(t, "Ex") || starts(t, "+") || starts(t, "-");
            if (bad) check(false, "non-A node drew text: '" + t + "'");
        }
        check(true, "non-A node: no A / I / I? / A? / AE / Ex? label and no delta text drawn");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    {
        home("render_A");
        mock::Host h = upLeg(110, pass); DeltaProfile p; freshDraw(p, h);
        int aLabels = 0, amounts = 0, other = 0;
        for (const std::string& t : textsOf(h)) {
            if (starts(t, "A ") || t == "A") ++aLabels;
            else if (starts(t, "+") || starts(t, "-")) ++amounts;
            else if (starts(t, "I") || starts(t, "AE") || starts(t, "Ex") || t.find('?') != std::string::npos) ++other;
        }
        check(aLabels == 1 && amounts <= aLabels && other == 0, "A node: one A label, delta text only on it (" + std::to_string(aLabels) + " / " + std::to_string(amounts) + " / " + std::to_string(other) + ")");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    {
        home("render_offscreen");
        mock::Host h = upLeg(110, pass); h.barOffset = 60;              // scrolled: the peak bar (30) is left of the pane, its row still visible
        DeltaProfile p; freshDraw(p, h);
        int aLabels = 0, amounts = 0;
        for (const std::string& t : textsOf(h)) { if (starts(t, "A ") || t == "A") ++aLabels; else if (starts(t, "+") || starts(t, "-")) ++amounts; }
        check(aLabels == 0 && amounts == 0, "offscreen A: no label AND no orphan delta text (" + std::to_string(aLabels) + " / " + std::to_string(amounts) + ")");
        mock::current = &h; p.destroy(); latchBooks().clear();
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
