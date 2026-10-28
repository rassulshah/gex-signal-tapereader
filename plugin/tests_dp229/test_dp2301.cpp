// test_dp2301.cpp - lsDeltaProfile 2.30.1 (2026-10-28 Key Levels column keep-left): test_dp230.cpp with the version bumped.
// test_dp230.cpp (copied harness of test_dp229.cpp) -- lsDeltaProfile 2.29.0 (2026-10-28 overnight, Rassul 21:52 CT "go"): the READ TIERS A / A? / A+? from lsBookFlow's node
// ladder (NodeLadder.h, SignalBoard 1.5.0), decided once (ReadTier record, restored after a restart: no repaint), kind 'Y' facts, late
// candles never A+, no order book -> plain A after the wait, and the combiner's SWING odds with "?" (CombinerLogic 0.2.0, per-market LOG).
// Build (from tests_dp229/): g++ -std=c++17 -O1 -w -I../tests_dp217 -I../tests_delta -I.. test_dp229.cpp -o /tmp/t_dp229 && /tmp/t_dp229
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static void put(const std::string& p, const std::string& t) { std::ofstream o(p.c_str(), std::ios::binary); o << t; }
static std::string flex;
static void home(const std::string& name) {
    const std::string root = "/tmp/dp230-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1); flex = h + "/InvestorRT/rtx/lsFlexLevels/";
    std::string t = "VERSION,1\nMARKET,ES\nZ_ATR,0.15\nHOLD,0.05,0.94\nLATE,60\nSHOW,A+,?\nSHOW,A?,on\n";
    for (int hr = 0; hr < 24; ++hr) t += "HOURMED," + std::to_string(hr) + ",1000\n";
    put(flex + "LRA-ReadTier-ES.csv", t);
}
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; state(h).factsPostedMs = -100000; } mock::current = &h; p.draw(); }
static void finish(mock::Host& h, DeltaProfile& p) { mock::current = &h; p.destroy(); latchBooks().clear(); }
static unsigned long T0 = 1789978400UL;   // candle 35 ends 09:58:20 (the mock clock = UTC as Central): RTH mid, not late
static mock::Bar qb(int i) { mock::Bar b; b.time = T0 + static_cast<unsigned long>(i) * 180; b.open = 111; b.close = 111; b.low = 109; b.high = 113;
    b.rows = { mock::Price(109, 2, 1, 3), mock::Price(110, 2, 1, 3), mock::Price(111, 2, 1, 3), mock::Price(112, 2, 1, 3), mock::Price(113, 2, 1, 3) }; b.volume = 15; return b; }
static mock::Host chart(bool nextCandle = false) {
    mock::Host h; h.seconds = 180; h.root = "ES"; h.tick = 0.25f; h.period = "3 min";
    for (int i = 0; i < 35; ++i) h.bars.push_back(qb(i));
    mock::Bar p = qb(35); p.low = 104; p.high = 111; p.open = 105.5f; p.close = 108;
    if (nextCandle) { p.open = 108; p.close = 105.5f; }   // the node's own candle closes RED (the attackers' colour): no same-candle A
    p.rows.push_back(mock::Price(105, 0, 500, 500)); p.volume += 500; h.bars.push_back(p);
    if (nextCandle) { mock::Bar q = qb(36); q.low = 105; q.high = 109.5f; q.open = 105.5f; q.close = 109; h.bars.push_back(q); }
    return h;
}
static void addq(mock::Host& h) { h.bars.push_back(qb(static_cast<int>(h.bars.size()))); }
static bool textIn(const mock::Host& h, const std::string& t, int l, int r) { for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text == t && d.x >= l && d.x <= r) return true; return false; }
static bool textPrefix(const mock::Host& h, const std::string& t) { for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text.compare(0, t.size(), t) == 0) return true; return false; }
// the ladder lsBookFlow would post for candle 35: bids at the node (105 = tick 420) added / pulled, a fill in each half
static void postLadder(long long bar, long long add, long long pull) {
    nl2::PBar b; b.bar = bar; b.key = bar; b.tick = 0.25; nl2::Row r; r.add[0] = add; r.pull[0] = pull; r.fill[0] = 400; r.fill1[0] = 300;
    b.rows[420] = r; nl2::Row ra; ra.pull[1] = 50; b.rows[428] = ra; b.p0 = 420;
    sb::postLadder("ESZ26", 180, "test", nl2::encode(steadyMs(), std::vector<nl2::PBar>(1, b)));
}
static const sb::Fact* yAt(std::vector<sb::Fact>& v, long long bar) { for (const sb::Fact& f : v) if (f.bar == bar) return &f; return nullptr; }

// (2.30.0) the hourly flow: 100 candles before the A's 30-candle window; 05:00-08:00 CT buying, 08:00-09:00 (the last CLOSED hour before the
// A candle that ends 09:58:20) selling or buying
static mock::Host chartFlow(bool lastHourSells, bool next = false) {
    mock::Host h = chart(next); std::vector<mock::Bar> pre;
    for (int i = -100; i < 0; ++i) { mock::Bar b = qb(0); b.time = T0 + (long)i * 180;
        const long long st = (long long)b.time - 180, hr = ((st % 86400) + 86400) % 86400 / 3600;
        const bool sell = lastHourSells && hr == 8;
        b.rows = { mock::Price(110, sell ? 1 : 30, sell ? 40 : 1, 31), mock::Price(111, sell ? 1 : 30, sell ? 40 : 1, 31) };
        for (mock::Price& p : b.rows) p.total = p.buy + p.sell; b.volume = 0; for (mock::Price& p : b.rows) b.volume += p.total; pre.push_back(b); }
    h.bars.insert(h.bars.begin(), pre.begin(), pre.end());
    return h;
}
static void addn(mock::Host& h) { mock::Bar b = qb(0); b.time = h.bars.back().time + 180; h.bars.push_back(b); }
int main() {
    const long long bar35 = (long long)(T0 + 35 * 180); g_tierWaitMs = 0;
    {   home("pb"); bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), "");
        mock::Host h = chartFlow(true); addn(h); DeltaProfile p; redraw(h, p); redraw(h, p); redraw(h, p);
        ChartState& s = state(h); const int l = s.lay.delta.l, r = s.lay.delta.l + 44;
        check(textIn(h, "A F?", l, r), "[F?] same-candle bull A in a pullback of a BUYING 4-hour flow: 'A F?' in the READ strip");
        const std::string rec = slurp(s.tierPath);
        check(rec.find(",F1,") != std::string::npos, "[record] T line carries F1 + net4 + net1");
        std::vector<sb::Fact> H; sb::readFacts('H', "ESZ26", 180, "test", steadyMs(), H, true);
        bool atA = false, now = false; for (const sb::Fact& f : H) { if (f.bar == bar35 && f.code == 1 && f.side == 1 && f.val > 0 && f.lo < 0) atA = true; if (f.code == 9) now = true; }
        check(atA, "[H] kind H at the A: code 1 (pullback in its favour), net4 > 0, last hour < 0");
        check(now, "[H] the current flow fact (code 9)");
        const std::string st = slurp(flex + "DeltaProfile.status-ES-180.txt");
        check(st.find("HOUR_FLOW,dph_read 1.0.0,A checked 1,F tagged 1,show ?") != std::string::npos, "[status] HOUR_FLOW line");
        finish(h, p);
        // restart: the record wins (no repaint) even with different bars before the window
        mock::Host h2 = chartFlow(false); addn(h2); DeltaProfile p2; redraw(h2, p2); redraw(h2, p2);
        check(textIn(h2, "A F?", state(h2).lay.delta.l, state(h2).lay.delta.l + 44), "[no repaint] after a restart the recorded F? stands");
        finish(h2, p2);
    }
    {   home("with"); bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), "");
        mock::Host h = chartFlow(false); addn(h); DeltaProfile p; redraw(h, p); redraw(h, p); redraw(h, p);
        ChartState& s = state(h); const int l = s.lay.delta.l, r = s.lay.delta.l + 44;
        check(textIn(h, "A", l, r) && !textIn(h, "A F?", l, r), "[no F] the last hour bought too (no pullback): plain A");
        check(slurp(s.tierPath).find(",F0,") != std::string::npos, "[record] F0");
        finish(h, p);
    }
    {   home("short"); bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), "");
        mock::Host h = chart(); addn(h); DeltaProfile p; redraw(h, p); redraw(h, p); redraw(h, p);
        ChartState& s = state(h);
        check(textIn(h, "A", s.lay.delta.l, s.lay.delta.l + 44) && slurp(s.tierPath).find(",F") == std::string::npos, "[short] under 4 closed hours of candles: no flow, plain A, no F field");
        finish(h, p);
    }
    {   home("gate"); bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), ""); put(flex + "LRA-HtfFlow-ES.csv", "VERSION,1\nMARKET,ES\nSHOW,F,off\n");
        mock::Host h = chartFlow(true); addn(h); DeltaProfile p; redraw(h, p); redraw(h, p); redraw(h, p);
        check(!textIn(h, "A F?", state(h).lay.delta.l, state(h).lay.delta.l + 44) && textIn(h, "A", state(h).lay.delta.l, state(h).lay.delta.l + 44), "[gate] SHOW,F,off: plain A (still recorded F1)");
        check(slurp(state(h).tierPath).find(",F1,") != std::string::npos, "[gate] the record keeps F1 when hidden");
        finish(h, p);
        home("gate2"); put(flex + "LRA-HtfFlow-ES.csv", "VERSION,1\nMARKET,ES\nSHOW,F,proven\n");
        mock::Host h2 = chartFlow(true); addn(h2); DeltaProfile p2; redraw(h2, p2); redraw(h2, p2); redraw(h2, p2);
        check(textIn(h2, "A F", state(h2).lay.delta.l, state(h2).lay.delta.l + 44), "[gate] SHOW,F,proven: 'A F' (no ?)");
        finish(h2, p2);
    }
    {   home("next"); bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), "");
        mock::Host h = chartFlow(true, true); addn(h); DeltaProfile p; redraw(h, p); redraw(h, p);
        check(textIn(h, "A?", state(h).lay.delta.l, state(h).lay.delta.l + 44), "[A?] a next-candle A never gets F? (not tested for it)");
        finish(h, p);
    }
    check(std::string(DLT_VERSION) == "2.30.1" && std::string(dph_read::VERSION) == "1.0.0", "[ver] 2.30.1 / dph_read 1.0.0");
    std::cout << "dp 2.30.0: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
