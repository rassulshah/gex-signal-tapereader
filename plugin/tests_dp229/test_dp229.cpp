// test_dp229.cpp -- lsDeltaProfile 2.29.0 (2026-10-28 overnight, Rassul 21:52 CT "go"): the READ TIERS A / A? / A+? from lsBookFlow's node
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
    const std::string root = "/tmp/dp229-run", h = root + "/" + name; rmrf(h);
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
int main() {
    const long long bar35 = (long long)(T0 + 35 * 180);
    // 1) HOLDING at the node -> A+? ; decided once, the record wins after a restart even if the ladder changes
    {   home("hold"); g_tierWaitMs = 20000; bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), "");
        mock::Host h = chart(); addq(h); DeltaProfile p; redraw(h, p); redraw(h, p);
        ChartState& s = state(h); const int l = s.lay.delta.l, r = s.lay.delta.l + 30;
        check(textIn(h, "A", l, r) && s.tierWait.size() == 1, "[wait] no ladder yet: plain A while waiting (<= 20 s) for lsBookFlow");
        postLadder(bar35, 3000, 100); redraw(h, p);
        check(textIn(h, "A+?", l, r) && !textIn(h, "A", l, r), "[A+?] same candle, RTH mid, the bids HELD at the node (added 3000, pulled 100): A+?");
        std::vector<sb::Fact> Y, R; sb::readFacts('Y', "ESZ26", 180, "test", steadyMs(), Y, true); sb::readFacts('R', "ESZ26", 180, "test", steadyMs(), R, true);
        const sb::Fact* y = yAt(Y, bar35);
        check(y && y->code == 31 && y->side == 1 && std::fabs(y->val - 2.9) < 1e-6 && y->proven == 2, "[Y] kind Y: tier A+ / holding, net +2.9x, '?'");
        check(!R.empty() && R[0].code == 4 && R[0].proven == 2, "[R] the READ fact carries A+ (code 4), '?'");
        const std::string rec = slurp(s.tierPath);
        check(rec.find(",P,1,1,2.9,") != std::string::npos, "[record] one T line: tier P, side +1, holding, net 2.9");
        const std::string st = slurp(flex + "DeltaProfile.status-ES-180.txt");
        check(st.find("READ_TIER,1.0.0,nightly read tier,A+ 1 ?") != std::string::npos, "[status] READ_TIER line");
        finish(h, p);
        postLadder(bar35, 100, 3000);   // the board now says PULLING - a restart must keep the decided A+?
        mock::Host h2 = chart(); addq(h2); DeltaProfile p2; redraw(h2, p2); redraw(h2, p2);
        check(textIn(h2, "A+?", state(h2).lay.delta.l, state(h2).lay.delta.l + 30), "[no repaint] after a restart the recorded A+? stands (the ladder changed)");
        int tl = 0; { std::istringstream in(slurp(state(h2).tierPath)); std::string x; while (std::getline(in, x)) if (x.compare(0, 2, "T,") == 0) ++tl; }
        check(tl == 1, "[no repaint] never written twice");
        finish(h2, p2);
    }
    // 2) PULLING at the node -> A (net -2.9x, read 2)
    {   home("pull"); postLadder(bar35, 100, 3000);
        mock::Host h = chart(); addq(h); DeltaProfile p; redraw(h, p); redraw(h, p);
        check(textIn(h, "A", state(h).lay.delta.l, state(h).lay.delta.l + 30), "[A] the bids PULLED at the node: plain A");
        std::vector<sb::Fact> Y; sb::readFacts('Y', "ESZ26", 180, "test", steadyMs(), Y, true); const sb::Fact* y = yAt(Y, bar35);
        check(y && y->code == 12 && y->val < -2.8, "[Y] tier A / pulling, net -2.9x");
        finish(h, p);
    }
    // 3) no order book on the chart: after the wait, plain A, read 0 (no Y for lsBookFlow to show)
    {   home("nobook"); bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), ""); g_tierWaitMs = 0;
        mock::Host h = chart(); addq(h); DeltaProfile p; redraw(h, p); redraw(h, p); redraw(h, p);
        check(textIn(h, "A", state(h).lay.delta.l, state(h).lay.delta.l + 30) && state(h).tierNoBook == 1, "[no book] plain A, counted 'no book'");
        std::vector<sb::Fact> Y; sb::readFacts('Y', "ESZ26", 180, "test", steadyMs(), Y, true); const sb::Fact* y = yAt(Y, bar35);
        check(y && y->code == 10, "[Y] code 10 = A without a read (lsBookFlow draws nothing for it)");
        finish(h, p); g_tierWaitMs = 20000;
    }
    // 4) late (post-close 15:55 CT): holding, but never A+
    {   home("late"); const unsigned long keep = T0; T0 = 1790000000UL; const long long b2 = (long long)(T0 + 35 * 180); postLadder(b2, 3000, 100);
        mock::Host h = chart(); addq(h); DeltaProfile p; redraw(h, p); redraw(h, p);
        check(textIn(h, "A", state(h).lay.delta.l, state(h).lay.delta.l + 30) && !textIn(h, "A+?", state(h).lay.delta.l, state(h).lay.delta.l + 30) && state(h).tierLate == 1, "[late] post-close candle: A, never A+");
        std::vector<sb::Fact> Y; sb::readFacts('Y', "ESZ26", 180, "test", steadyMs(), Y, true); const sb::Fact* y = yAt(Y, b2);
        check(y && y->code == 11, "[late] still read as holding (code 11) for the Defense pane");
        finish(h, p); T0 = keep;
    }
    // 5) next-candle A -> A?
    {   home("next"); bb::bbSet(sb::ladderKey("ESZ26", 180, "test"), "");
        mock::Host h = chart(true); addq(h); DeltaProfile p; redraw(h, p); redraw(h, p);
        const int l = state(h).lay.delta.l;
        check(textIn(h, "A?", l, l + 30) && state(h).tierNext == 1, "[A?] the next candle confirmed it: A?");
        finish(h, p);
    }
    // 6) the combiner's swing odds with "?" (round-2 weights): SHOW -> "swing NN% ?", per-market LOG -> nothing; turn LOG never draws "NN%"
    {   home("swing");
        const std::string w = slurp("../tests_chain/LRA-Combiner.csv");
        std::string t; { std::istringstream in(w); std::string x; while (std::getline(in, x)) if (x.compare(0, 6, "STATUS") != 0 && x.compare(0, 4, "SHOW") != 0) t += x + "\n"; }
        put(flex + "LRA-Combiner.csv", t + "SHOW,turn,0.72\nSHOW,swing,0.25\nSTATUS,turn,LOG\nSTATUS,swing,SHOW\n");
        std::vector<sb::Fact> v(1); v[0].bar = 1; v[0].lo = v[0].hi = 104.25; v[0].side = 1; v[0].code = 3; v[0].state = 1; sb::postFacts('V', "ESZ26", 180, "test", steadyMs(), v);
        mock::Host h = chart(); addq(h); DeltaProfile p; redraw(h, p); redraw(h, p);
        bool sw = false, pct = false; for (const mock::Draw& d : h.draws) if (d.type == "text") { if (d.text.compare(0, 6, "swing ") == 0 && d.text.size() > 3 && d.text.substr(d.text.size() - 3) == "% ?") sw = true; else if (d.text.size() >= 2 && d.text.back() == '%') pct = true; }
        check(sw && !pct, "[swing] the swing odds drawn with '?' (\"swing NN% ?\"), the turn odds not drawn (LOG)");
        std::vector<sb::Fact> K; sb::readFacts('K', "ESZ26", 180, "test", steadyMs(), K, true);
        check(!K.empty() && (K[0].code & 2) && K[0].proven == 2, "[swing] K fact code 2 = swing shown, '?'");
        const std::string st = slurp(flex + "DeltaProfile.status-ES-180.txt");
        check(st.find("COMBINER,0.2.0") != std::string::npos && st.find(",swing SHOW ? >= 0.25") != std::string::npos, "[status] COMBINER 0.2.0 swing SHOW");
        finish(h, p);
        home("swinglog"); put(flex + "LRA-Combiner.csv", t + "SHOW,swing,0.25\nSTATUS,turn,LOG\nSTATUS,swing,SHOW\nSTATUS,swing,LOG,ES\n");
        sb::postFacts('V', "ESZ26", 180, "test", steadyMs(), v);
        mock::Host h2 = chart(); addq(h2); DeltaProfile p2; redraw(h2, p2); redraw(h2, p2);
        check(!textPrefix(h2, "swing "), "[swing] STATUS,swing,LOG,ES: nothing drawn on ES");
        finish(h2, p2);
    }
    check(std::string(DLT_VERSION) == "2.29.0" && std::string(dpset::PLUGIN_VERSION) == DLT_VERSION && std::string(cmb::VERSION) == "0.2.0" && std::string(nl2::VERSION) == "1.0.0" && std::string(sb::VERSION) == "1.5.0", "[ver] 2.29.0 / CombinerLogic 0.2.0 / NodeLadder 1.0.0 / SignalBoard 1.5.0");
    std::cout << "dp 2.29.0: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
