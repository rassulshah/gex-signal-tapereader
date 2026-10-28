// test_lft_replay.cpp -- lsLiquidityProfile 0.64.0 LMT F on the mock host, REPLAYING A REAL OVERNIGHT: ES 2026-10-21 20:00-22:00 CT (ASIA),
// every resting order's fills from the Databento MBO archive (tests_lmtf/data/replay_ES_asia.csv: t,act,side,pt,sz,id), the real 3-min bars,
// the nightly's tiers file (lmtf_read 2.0.0: ASIA 73 / 97 / 124). 0.62.0-0.63.0 drew NOTHING here (no single resting order >= 100 lots,
// and the big-order tracker never followed one); 0.64.0 must show the prints >= 73 (lots filled at a price in one candle).
// From plugin\:  g++ -std=c++17 -O1 -Itests_lqp044 -I. tests_lmtf/test_lft_replay.cpp -o tlftr && ./tlftr
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <map>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/lftrXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    { std::ofstream f((flex + "\\LRA-LmtF-ES.csv").c_str()); f << readAll("tests_lmtf/data/LRA-LmtF-ES.csv"); }
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26";
    h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    std::vector<long long> te; std::vector<float> bo, bh, bl, bc;
    { std::ifstream f("tests_lmtf/data/replay_ES_bars.csv"); std::string ln; while (std::getline(f, ln)) { long long t; double o, hi, lo, c; if (std::sscanf(ln.c_str(), "%lld,%lf,%lf,%lf,%lf", &t, &o, &hi, &lo, &c) == 5) { te.push_back(t); bo.push_back((float)(o * 0.25)); bh.push_back((float)(hi * 0.25)); bl.push_back((float)(lo * 0.25)); bc.push_back((float)(c * 0.25)); } } }
    struct E { long long t; int a, s; long long pt, sz; unsigned long long id; }; std::vector<E> ev;
    { std::ifstream f("tests_lmtf/data/replay_ES_asia.csv"); std::string ln; while (std::getline(f, ln)) { E e; if (std::sscanf(ln.c_str(), "%lld,%d,%d,%lld,%lld,%llu", &e.t, &e.a, &e.s, &e.pt, &e.sz, &e.id) == 6) ev.push_back(e); } }
    std::printf("  bars %zu, events %zu\n", te.size(), ev.size());
    CHECK(te.size() > 50 && ev.size() > 1000, "the real ES overnight data loaded");
    double lo = 1e9, hi = 0; for (size_t i = 0; i < bc.size(); ++i) { lo = std::min(lo, (double)bl[i]); hi = std::max(hi, (double)bh[i]); }
    h.priceBase = (float)(lo - 0.5 * (hi - lo)); h.pxPer = (float)(350.0 / std::max(4.0, hi - lo));   // y = bottom - (price - base) x pxPer: lo at ~825, hi at ~475 (clear of the footer)
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(bc[0] - 0.25 - i * 0.25); L.ask = (float)(bc[0] + i * 0.25); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    // bars up to the replay start
    const long long t0 = ev.front().t, tEnd = ev.back().t;
    size_t nb = 0; auto addBars = [&](long long upto) { while (nb < te.size() && te[nb] - 180 <= upto) { mock::Bar b; b.time = (unsigned long)(te[nb] - 180); b.open = bo[nb]; b.high = bh[nb]; b.low = bl[nb]; b.close = bc[nb]; b.rows.push_back(mock::Price(bc[nb], 50, 40, 90)); h.bars.push_back(b); ++nb; } };
    addBars(t0);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 4; ++s) { h.now = (unsigned long)(t0 - 30 + s); p->calc(0); usleep(3000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    std::map<unsigned long long, int> ids; int nextId = 1;
    size_t k = 0; int draws = 0; long long maxShown = 0; int factsSeen = 0, factsLow = 0; bool sawAdapt = false;
    for (long long t = t0; t <= tEnd + 180; t += 60) {   // one calc a minute of replay time
        while (k < ev.size() && ev[k].t <= t) { const E& e = ev[k]; int& id = ids[e.id]; if (!id) id = nextId++;
            mock::feed.push((unsigned char)e.a, (unsigned char)e.s, (float)(e.pt * 0.25), (unsigned long)e.sz, (RTDATE)e.t, id, e.a == 3 ? 900000 + (int)k : 0); ++k; }
        addBars(t); h.now = (unsigned long)t; p->calc(0);
        if ((t - t0) % 600 == 0) { h.draws.clear(); p->draw(); ++draws; maxShown = std::max(maxShown, (long long)S->lftShown);
            if (S->lftShown > 0) { std::vector<sb::Fact> P0; sb::readFacts('P', S->symbol, S->spb, S->chartKey, -1, P0, false); if (S->lftAdapt) sawAdapt = true; for (auto& f : P0) { factsSeen++; if (f.val < S->lftCut[0]) factsLow++; } } }
    }
    h.draws.clear(); p->draw(); maxShown = std::max(maxShown, (long long)S->lftShown);
    std::printf("  priceBase %.2f pxPer %.2f lo %.2f hi %.2f\n", h.priceBase, h.pxPer, lo, hi);
    std::printf("  fills %lld, cells %zu, part %d, cuts %lld/%lld/%lld adaptive %d, prints this part %d, shown now %d, max shown %lld\n",
                S->lfp.fills, S->lfp.cells.size(), S->lftPart, S->lftCut[0], S->lftCut[1], S->lftCut[2], (int)S->lftAdapt, S->lftPartPrints, S->lftShown, maxShown);
    CHECK(S->lfp.fills > 1000, "every passive fill reached the print tally");
    CHECK(S->lftPart == lft::ASIA && S->lftCut[0] == 73 && S->lftCut[1] == 97 && S->lftCut[2] == 124, "ASIA cuts from the nightly file in force (73 / 97 / 124)");
    CHECK(S->lftPartPrints >= 3, "the overnight prints >= S20 are counted (the python check: 3 in this window)");
    CHECK(maxShown >= 1 && S->lftShown >= 1, "LMT F SHOWS overnight prints, also at the end (0.63.0 showed none)");
    for (auto& c : S->lfp.cells) if (c.lots >= 73) std::printf("   print bar %lld side %d px %.2f lots %lld orders %d big %lld\n", c.bar, c.side, c.px * 0.25, c.lots, c.orders, c.big);
    std::printf("  facts seen %d (under 73: %d)\n", factsSeen, factsLow);
    CHECK(factsSeen >= 1 && factsLow == 0, "SignalBoard P carries the prints, each >= the cut in force");
    CHECK(sawAdapt, "the quiet first hour (no fills 19:00-20:00 in the replay, nothing >= 73 by 20:10) switched to the adaptive floor and still showed its biggest print");
    usleep(1100000); h.now = (unsigned long)(tEnd + 200); p->calc(0); usleep(1100000); h.now = (unsigned long)(tEnd + 202); p->calc(0);
    const std::string st = readAll(flex + "\\LiquidityProfile.status-ES-180.txt");
    CHECK(st.find("LMT_F2,1.0.0,print = lots filled at a price per candle,part ASIA,cuts 73/97/124 (file)") != std::string::npos, "status LMT_F2 line");
    std::printf("lsLiquidityProfile %s LMT F real-overnight replay: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
