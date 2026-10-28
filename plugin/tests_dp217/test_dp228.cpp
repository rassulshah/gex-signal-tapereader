// test_dp228.cpp -- lsDeltaProfile 2.28.0 (2026-10-28 overnight build): the column on the RIGHT with the READ strip, the ratio-only labels,
// the SignalBoard 1.4.0 facts (R / N / C), the layer-3 COMBINER (decided once, logged, restored after a restart, load-order safe, bounded).
// Build (from tests_dp217/): g++ -std=c++17 -O1 -I. -I../tests_delta -I.. test_dp228.cpp -o /tmp/t_dp228 && /tmp/t_dp228
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string flex;
static void home(const std::string& name) {
    const std::string root = "/tmp/dp228-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1); flex = h + "/InvestorRT/rtx/lsFlexLevels/";
}
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; } mock::current = &h; p.draw(); }
static void finish(mock::Host& h, DeltaProfile& p) { mock::current = &h; p.destroy(); latchBooks().clear(); }
static const unsigned long T0 = 1790000000UL;
static mock::Bar qb(int i) { mock::Bar b; b.time = T0 + static_cast<unsigned long>(i) * 180; b.open = 111; b.close = 111; b.low = 109; b.high = 113;
    b.rows = { mock::Price(109, 2, 1, 3), mock::Price(110, 2, 1, 3), mock::Price(111, 2, 1, 3), mock::Price(112, 2, 1, 3), mock::Price(113, 2, 1, 3) }; b.volume = 15; return b; }
// candle 35: a 6-candle low (104) that sells 500 at 105 and closes GREEN at 108 in its upper half -> a same-candle A in the wick + a pivot read
static mock::Host chart() {
    mock::Host h; h.seconds = 180; h.root = "ES"; h.tick = 0.25f; h.period = "3 min";
    for (int i = 0; i < 35; ++i) h.bars.push_back(qb(i));
    mock::Bar p = qb(35); p.low = 104; p.high = 111; p.open = 105.5f; p.close = 108;
    p.rows.push_back(mock::Price(105, 0, 500, 500)); p.volume += 500; h.bars.push_back(p);
    return h;
}
static void addq(mock::Host& h) { h.bars.push_back(qb(static_cast<int>(h.bars.size()))); }
static std::vector<const mock::Draw*> texts(const mock::Host& h, const std::string& t) { std::vector<const mock::Draw*> v; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text == t) v.push_back(&d); return v; }
static int kLines(const std::string& path) { int n = 0; std::istringstream in(slurp(path)); std::string l; while (std::getline(in, l)) if (l.compare(0, 2, "K,") == 0) ++n; return n; }
int main() {
    // 1) layout + READ + ratio-only labels + facts + combiner
    {   home("main");
        // the level producer is on the chart first (lsDealerProfile posts 'V'): a key level at 104.25 = the candle's low within 0.5 ATR
        std::vector<sb::Fact> v(1); v[0].bar = 1; v[0].lo = v[0].hi = 104.25; v[0].side = 1; v[0].code = 3; v[0].state = 1;
        sb::postFacts('V', "ESZ26", 180, "test", steadyMs(), v);
        mock::Host h = chart(); addq(h);
        DeltaProfile p; redraw(h, p); redraw(h, p);
        ChartState& s = state(h);
        check(!cb::DELTA_LEFT && s.lay.delta.r > s.lay.priceRight && s.lay.delta.l > s.lay.priceRight, "[layout] the Delta column is on the RIGHT of the price room");
        std::vector<const mock::Draw*> a = texts(h, "A");
        bool inStrip = false; for (const mock::Draw* d : a) if (d->x >= s.lay.delta.l && d->x <= s.lay.delta.l + 30) inStrip = true;
        check(inStrip && s.readDrawn == 1, "[read] the A is drawn in the READ strip (first 26 px of the column)");
        bool aPrefix = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text.compare(0, 2, "A ") == 0) aPrefix = true;
        check(!aPrefix, "[read] no \"A 7.2x\" labels: the bars carry the ratio only");
        bool ratio = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text.size() > 1 && d.text.back() == 'x' && d.x > s.lay.delta.l + 26) ratio = true;
        check(ratio, "[read] a ratio label sits right of the READ strip");
        std::vector<sb::Fact> R, N, C;
        check(sb::readFacts('R', "ESZ26", 180, "test", steadyMs(), R, true) && R.size() == 1 && R[0].code == 1 && R[0].side == 1 && R[0].state == 1 && R[0].proven == 1, "[facts] R: one decided bullish A, proven");
        check(sb::readFacts('N', "ESZ26", 180, "test", steadyMs(), N, false) && !N.empty() && N.size() <= 3, "[facts] N: the top-3 nodes (live)");
        check(sb::readFacts('C', "ESZ26", 180, "test", steadyMs(), C, true) && !C.empty(), "[facts] C: candle trap lots (decided)");
        bool c35 = false; for (const sb::Fact& f : C) if (f.lo >= 500 && f.side == 1) c35 = true;
        check(c35, "[facts] C: candle 35's 500 sold below its close = sellers trapped (bullish)");
        const std::string rec = s.cmbPath;
        check(kLines(rec) >= 1 && s.cmbScored >= 1, "[combiner] the pivot-reversal candle is scored and logged");
        std::string line; { std::istringstream in(slurp(rec)); std::string l; while (std::getline(in, l)) if (l.compare(0, 2, "K,") == 0) line = l; }
        const std::vector<std::string> f = cmb::split(line, ',');
        check(f.size() == 7 + cmb::NF && f[2] == "1" && f[6] == "1" && f[7] == "1" && f[8] == "1" && f[19] == "1", "[combiner] inputs: bullish, levels seen, A = 1, wick = 1, key level = 1");
        std::vector<sb::Fact> K; check(sb::readFacts('K', "ESZ26", 180, "test", steadyMs(), K, true) && !K.empty() && K[0].val > 0 && K[0].val < 100, "[combiner] K fact posted with odds in (0, 100)");
        {   redraw(h, p); const mst::Facts F = mst::read("DP", "ESZ26", 180, "test", steadyMs());   // (awareness) the read in SRC DP
            check(mst::getS(F, "cmb") == "bull" && mst::getD(F, "cmb_turn") > 0 && mst::getS(F, "cmb_a") == "1" && mst::getS(F, "cmb_status") == "SHOW",
                  "[awareness] Market State SRC DP carries the combiner read of the closed candle (cmb bull, odds, A, status)"); }
        const std::string st = slurp(flex + "DeltaProfile.status-ES-180.txt");
        check(st.find("COMBINER,0.") != std::string::npos && st.find("READ,strip 26 px,A drawn 1") != std::string::npos && st.find("FACTS,SignalBoard 1.") != std::string::npos, "[status] READ / FACTS / COMBINER lines");
        const int k0 = kLines(rec);
        for (int i = 0; i < 30; ++i) { addq(h); redraw(h, p); }
        check(kLines(rec) == k0, "[combiner] quiet candles after it: no new reads (not pivot-reversal candles), the read is never re-scored");
        // restart: the reads come back from the record, nothing written twice
        const std::string before = slurp(rec);
        finish(h, p); sb::postFacts('V', "ESZ26", 180, "test", steadyMs(), v);
        DeltaProfile p2; redraw(h, p2); redraw(h, p2);
        check(slurp(rec) == before && state(h).cmbRestored, "[restart] the combiner record is read back, nothing written twice");
        // board stays bounded over a long replay
        for (int i = 0; i < 300; ++i) { addq(h); redraw(h, p2); }
        std::string kb; bb::bbGet(sb::factKey('C', "ESZ26", 180, "test"), kb);
        check(state(h).cmbReads.size() <= 2000 && kb.size() < 16000, "[bounded] reads and board size bounded over 300 candles");
        finish(h, p2); }
    // 1b) LOG ONLY: LRA-Combiner.csv STATUS,turn,LOG -> nothing drawn, K facts "?", SRC DP cmb_status "?"
    {   home("logonly");
        { std::ofstream f((flex + "LRA-Combiner.csv").c_str()); std::ifstream in("../tests_chain/LRA-Combiner.csv"); std::string l;
          while (std::getline(in, l)) f << (l == "STATUS,turn,SHOW" ? std::string("STATUS,turn,LOG") : l) << "\n"; }
        std::vector<sb::Fact> v(1); v[0].bar = 1; v[0].lo = v[0].hi = 104.25; v[0].side = 1; v[0].code = 3; v[0].state = 1;
        sb::postFacts('V', "ESZ26", 180, "test", steadyMs(), v);
        mock::Host h = chart(); addq(h);
        DeltaProfile p; redraw(h, p); redraw(h, p); redraw(h, p);
        bool pct = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && !d.text.empty() && d.text.back() == '%') pct = true;
        std::vector<sb::Fact> K; sb::readFacts('K', "ESZ26", 180, "test", steadyMs(), K, true);
        const mst::Facts F = mst::read("DP", "ESZ26", 180, "test", steadyMs());
        check(!state(h).cmbParams.showOn && !pct && !K.empty() && K[0].proven == 2 && mst::getS(F, "cmb_status") == "?",
              "[log only] STATUS LOG: no % drawn, K facts '?', SRC DP cmb_status '?', still scored and logged");
        finish(h, p); }
    // 2) load order: the level producer arrives AFTER this chart -> the read waits (deferred) and decides WITH the level, same as case 1
    {   home("order");
        bb::bbSet(sb::factKey('V', "ESZ26", 180, "test"), "");
        mock::Host h = chart(); addq(h);
        DeltaProfile p; redraw(h, p);
        check(state(h).cmbScored == 0 && state(h).cmbDeferred >= 1, "[order] no level producer yet: the read is deferred, not decided");
        std::vector<sb::Fact> v(1); v[0].bar = 1; v[0].lo = v[0].hi = 104.25; v[0].side = 1; v[0].code = 3; v[0].state = 1;
        sb::postFacts('V', "ESZ26", 180, "test", steadyMs(), v);
        redraw(h, p);
        std::string line; { std::istringstream in(slurp(state(h).cmbPath)); std::string l; while (std::getline(in, l)) if (l.compare(0, 2, "K,") == 0) line = l; }
        const std::vector<std::string> f = cmb::split(line, ',');
        check(f.size() == 7 + cmb::NF && f[19] == "1", "[order] decided once the producer posts: key level = 1, same as when it loaded first");
        finish(h, p); }
    // 3) a missing producer never hangs: after the wait the read is decided without levels and counted
    {   home("missing"); bb::bbSet(sb::factKey('V', "ESZ26", 180, "test"), "");
        mock::Host h = chart(); addq(h);
        DeltaProfile p; redraw(h, p); state(h).cmbFirstMs -= 20000; redraw(h, p);
        check(state(h).cmbScored >= 1 && state(h).cmbNoLevels >= 1, "[missing] no level producer for 15 s: decided without levels, counted (NO_LEVELS)");
        finish(h, p); }
    check(std::string(DLT_VERSION) >= "2.28.0" && std::string(dpset::PLUGIN_VERSION) == DLT_VERSION, "[ver] 2.28.0");
    std::cout << "dp 2.28.0: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
