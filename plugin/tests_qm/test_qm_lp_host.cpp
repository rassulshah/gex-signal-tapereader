// test_qm_lp_host.cpp -- lsLiquidityProfile 0.49.0 QUEUE METER on the mock host (the whole plugin: order feed -> QueueMeter engine -> the live
// file LRA-QueueMeter-<MKT>.csv for the Dealer Profile -> the Q record at the candle close -> restart reads the record back, nothing twice).
// CL 3-min: 30 bids resting on the MenthorQ G1 79.80, price trades into the zone from 79.85, then breaks to 79.70 before the close.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_qm/test_qm_lp_host.cpp -o tqmh && ASAN_OPTIONS=detect_leaks=0 ./tqmh
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/home/claude/qmwork/qmhXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    { std::ofstream f((flex + "\\MenthorQLevels.csv").c_str()); f << "SYMBOL,PRICE,LABEL\nCLEX26,80.10,HVL 0D\nCLEX26,79.80,G1\nCLEX26,80.60,G2\n"; }
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    h.priceBase = 79.5f; h.pxPer = 1000.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 31; ++i) { mock::Bar b; b.time = t0 - (30 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b); }
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(80.00 - i * 0.01); L.ask = (float)(80.01 + i * 0.01); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    // the feed (buySell 1 = bid): 30 lots + 8 lots on G1 79.80 (rested 2 min+), a 1-lot bid at 79.85 traded first (price above the zone)
    mock::feed.push(0, 1, 79.80f, 30, (RTDATE)(t0 - 150), 8001);
    mock::feed.push(0, 1, 79.79f, 8, (RTDATE)(t0 - 150), 8004);
    mock::feed.push(0, 1, 79.85f, 1, (RTDATE)(t0 - 150), 8002);
    mock::feed.push(0, 1, 79.70f, 3, (RTDATE)(t0 - 150), 8003);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 6; ++s) { h.now = t0 - 100 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    S->qmE.q0Seen.assign(5, 20);   // (0.51.0 / QueueMeter 1.3.0) this session's normal queue at touched levels = 20 lots (the 38-lot G1 queue is above it)
    CHECK(S->qmE.book.size() == 4, "the queue meter keeps its own book from the chart's order feed");
    CHECK(S->qmE.atr > 0 && S->qmE.levels.size() >= 3, "ATR from the chart's candles; the MenthorQ levels are anchors");
    mock::feed.push(3, 0, 79.85f, 1, (RTDATE)(t0 - 20), 8002, 9001);   // a seller hits 79.85 (price above the G1 zone)
    mock::feed.push(3, 0, 79.80f, 6, (RTDATE)(t0 - 10), 8001, 9002);   // ... then trades into G1: the test starts
    for (int s = 0; s < 3; ++s) { h.now = t0 - 9 + s; p->calc(0); usleep(5000); }
    CHECK(S->qmE.tests.size() == 1 && S->qmE.tests[0].side == qm::BID && S->qmE.tests[0].q0 == 38, "a bid test at G1: q0 = 38 lots (30 + 8)");
    usleep(1100000); h.now = t0 - 6; p->calc(0);
    const std::string live = readAll(flex + "\\LRA-QueueLive-CL.csv");
    CHECK(live.compare(0, 9 + std::strlen(qm::VERSION), std::string("VERSION,") + qm::VERSION + "\n") == 0 && live.find("\nMARKET,CL\n") != std::string::npos && live.find("\nT,B,79.80,") != std::string::npos, "the live file: VERSION / MARKET / the G1 test row");
    qm::File f = qm::parseFile(qm::split(live, '\n'), "CL");
    CHECK(f.ok && f.t.size() == 1 && f.t[0].q == 32 && f.t[0].pct > 0.83 && f.t[0].pct < 0.85, "the Dealer Profile parses it: 32 of 38 left (84%)");
    // the break before the close, then the candle closes: a red Q in the record, decided once
    mock::feed.push(3, 0, 79.70f, 1, (RTDATE)(t0 - 5), 8003, 9003);
    for (int s = 0; s < 2; ++s) { h.now = t0 - 4 + s; p->calc(0); usleep(5000); }
    CHECK(!S->qmE.tests.empty() && S->qmE.tests[0].broke, "traded 0.25 ATR+ under the zone: live broke (the badge)");
    // (0.57.0, QueueMeter 1.5.0 HIS HELD rule) the contact candle (bar 30: traded 79.85 .. 79.70) cannot decide a break - it traded AT the bid;
    // the next candle (bar 31) lies wholly under the bid (high 79.76 < 79.80) = broke, decided at its close (bar 32 opens)
    h.bars.back().open = 79.86f; h.bars.back().high = 79.86f; h.bars.back().low = 79.70f; h.bars.back().close = 79.72f;
    { mock::Bar b; b.time = t0 + 180; b.open = 79.72f; b.high = 79.76f; b.low = 79.70f; b.close = 79.74f; b.rows.push_back(mock::Price(79.75f, 1, 1, 2)); h.bars.push_back(b); }   // bar 30 (end t0) closes: in contact, no green close above -> waits
    for (int s = 0; s < 2; ++s) { h.now = t0 + 2 + s; p->calc(0); usleep(5000); }
    CHECK(S->qmE.tests.empty() || S->qmE.tests[0].decided < 0, "the contact candle closed: undecided (no bar wholly beyond, no green close above)");
    { mock::Bar b; b.time = t0 + 360; b.open = b.high = b.low = b.close = 79.74f; b.rows.push_back(mock::Price(79.74f, 1, 1, 2)); h.bars.push_back(b); }   // bar 31 closes: wholly under the bid
    for (int s = 0; s < 3; ++s) { h.now = t0 + 5 + s; p->calc(0); usleep(400000); }
    usleep(1100000); h.now = t0 + 10; p->calc(0);
    const std::string recPath = flex + "\\LiquidityProfile\\CL-CLEX26-180-queuemeter-" + std::to_string(S->qmSession) + ".csv";
    const std::string rec = readAll(recPath);
    int nq = 0; { std::istringstream in(rec); std::string ln; while (std::getline(in, ln)) if (ln.rfind("Q|", 0) == 0 && ln.find("|R|broke|") != std::string::npos) ++nq; }
    CHECK(nq == 1, "the record holds one red Q (broke) for the G1 test");
    { int nc = 0; std::istringstream in(rec); std::string ln; while (std::getline(in, ln)) if (ln.rfind("C|", 0) == 0 && ln.find("|B|7980|N|") != std::string::npos) ++nc;
      CHECK(nc == 1, "the 10-s call: broke before 10 s -> one C line 'N' (no call)"); }
    CHECK(S->qmE.rule.mark == qm::M_SHOW && S->qmE.rule.greenShow, "CL: the verdict's defaults (SHOW, green on) with no params file");
    {   // (coordinator) the same Q as a Holding-strip letter: LetterStrips.h line format, pane Q, the decision candle's raw bar time, pivot 0 (low)
        const std::string strip = readAll(flex + "\\Strips\\QUEUE-CL-180.csv");
        const std::string want = "LT|1|Q|" + std::to_string(t0 + 180) + "|Q|0|-1|broke ";   // (0.57.0) the decision candle = bar 31
        int n = 0, fields = 0; { std::istringstream in(strip); std::string ln; while (std::getline(in, ln)) if (ln.rfind("LT|", 0) == 0) { ++n; fields = 1; for (char c : ln) if (c == '|') ++fields; } }
        CHECK(strip.rfind(want, 0) == 0 && n == 1 && fields == 8, "Strips\\QUEUE-CL-180.csv: one LT|1|Q|<bar key>|Q|0|-1|<numbers> line (8 fields)");
        CHECK(strip.find(" tk7980") != std::string::npos, "(0.51.0) the strip line carries the Q anchor tk7980 (lsReversalStage 1.2.0)");
    }
    const std::string st = readAll(flex + "\\LiquidityProfile.status-CL-180.txt");
    CHECK(st.find(std::string("QUEUE_METER,") + qm::VERSION + ",") != std::string::npos && st.find("Q rows written ") != std::string::npos && st.find(",call SHOW at 10 s") != std::string::npos, "status: QUEUE_METER line");
    CHECK(st.compare(0, 8 + std::strlen(LQ_VERSION), std::string("VERSION,") + LQ_VERSION) == 0 && std::string(LQ_VERSION) == "0.57.0", "status VERSION 0.57.0");
    // nothing new is drawn on the price chart by the meter (the Dealer Profile draws it in the Levels column)
    h.draws.clear(); p->draw(); bool qtext = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find("Q") == 0 && d.text.find('%') != std::string::npos) qtext = true;
    CHECK(!qtext, "no meter text drawn by the Liquidity Profile");
    // restart: a new plugin object reads the record back; the same test again (same anchor / side / start candle) is never written twice
    p->destroy(); p->mdPark_.clear(); delete p; h.data = nullptr;
    mock::feed.ev.clear(); mock::feed.at = 0;
    LiquidityProfile* q = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 4; ++s) { h.now = t0 + 60 + s; q->calc(0); usleep(5000); }
    LQState* S2 = static_cast<LQState*>(h.data);
    CHECK(S2 && S2->qmRestored == 1, "restart: the Q line is read back");
    const std::string rec2 = readAll(recPath);
    CHECK(rec2 == rec, "restart: the record is unchanged (no repaint)");
    { int n = 0; std::istringstream in(readAll(flex + "\\Strips\\QUEUE-CL-180.csv")); std::string ln; while (std::getline(in, ln)) if (ln.rfind("LT|", 0) == 0) ++n;
      CHECK(n == 1, "restart: no second strip line"); }
    q->destroy(); q->mdPark_.clear(); delete q;
    std::printf("lsLiquidityProfile 0.50.0 queue meter host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
