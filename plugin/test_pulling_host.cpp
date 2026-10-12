// test_pulling_host.cpp -- the whole lsPulling plugin against the mock host: order events in, two lines out, record, restart.
//   g++ -std=c++17 -Imock -I. test_pulling_host.cpp -o th && ./th
#include "Pulling.cpp"
#include <cstdio>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static DEPTH_ORDER ev(int a, int side, float px, unsigned long sz, int id, float bid = 0, float ask = 0) { DEPTH_ORDER o; std::memset(&o, 0, sizeof(o)); o.action = (unsigned char)a; o.buySell = (unsigned char)side; o.price = px; o.size = sz; o.orderID = id; o.bid = bid; o.ask = ask; return o; }
int main() {
    char tmpl[] = "/tmp/plXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL); unsigned long t0 = day + 9 * 3600;
    DEPTH_LEVEL L; L.bid = 7000.00f; L.ask = 7000.25f; L.bidsize = 10; L.asksize = 10; h.depth.push_back(L);
    { mock::Bar b; b.time = t0; b.volume = 0; h.bars.push_back(b); }
    h.now = t0;
    Pulling* p = static_cast<Pulling*>(CreateExtension());
    p->calc(0);
    PLState* S = p->testState();
    CHECK(S && S->dbo && h.dboObjects == 1, "the order feed is opened once for the chart");
    int id = 1;
    // 22 bars: each side pulls 10 lots near price per bar (the normal), the 23rd bar sellers pull 30 and buyers 5
    for (int k = 0; k < 23; ++k) {
        const bool last = k == 22;
        for (int j = 0; j < (last ? 3 : 1); ++j) { h.dboQueue.push_back(ev(0, 0, 7000.50f, 10, id, 7000.00f, 7000.25f)); h.dboQueue.push_back(ev(2, 0, 7000.50f, 10, id, 7000.00f, 7000.25f)); ++id; }
        h.dboQueue.push_back(ev(0, 1, 6999.75f, last ? 5 : 10, id, 7000.00f, 7000.25f)); h.dboQueue.push_back(ev(2, 1, 6999.75f, last ? 5 : 10, id, 7000.00f, 7000.25f)); ++id;
        h.dboQueue.push_back(ev(0, 1, 7000.00f, 4, id, 7000.00f, 7000.25f)); h.dboQueue.push_back(ev(3, 1, 7000.00f, 4, id, 7000.00f, 7000.25f)); h.dboQueue.push_back(ev(2, 1, 7000.00f, 4, id, 7000.00f, 7000.25f)); ++id;   // a fill: never a pull
        h.dboQueue.push_back(ev(0, 0, 7010.00f, 50, id, 7000.00f, 7000.25f)); h.dboQueue.push_back(ev(2, 0, 7010.00f, 50, id, 7000.00f, 7000.25f)); ++id;   // far away: not counted
        h.now = t0 + k * 180 + 5; p->calc(0);
        h.bars.back().volume = 4;
        if (!last) { mock::Bar n; n.time = h.bars.back().time + 180; h.bars.push_back(n); h.now = n.time; p->calc(0); }
    }
    const int lb = (int)h.bars.size() - 1;
    CHECK(S->e.bars[lb].pull[pl::OFFER] == 30 && S->e.bars[lb].pull[pl::BID] == 5, "pulls counted per side on the forming bar");
    CHECK(S->e.bars[lb].filled == 4, "the fill is counted as a fill");
    CHECK(std::fabs(S->e.x(lb, pl::OFFER) - 3.0) < 1e-9 && std::fabs(S->e.x(lb, pl::BID) - 0.5) < 1e-9, "sellers 3.0x, buyers 0.5x the normal");
    h.draws.clear(); p->draw();
    bool head = false, sellers = false, label = false, feed = false; int lines = 0;
    for (auto& d : h.draws) {
        if (d.type == "text" && d.text == "PULLING   ") head = true;
        if (d.type == "text" && d.text == "3.0x" && d.color == C_SELL) { if (d.y < 30) sellers = true; else label = true; }
        if (d.type == "text" && d.text.find("MBO") == 0) feed = true;
        if (d.type == "line") ++lines;
    }
    CHECK(head && sellers, "header: PULLING ... Sellers pulled: 3.0x");
    CHECK(label, "the 3.0x is written over the sellers' dot (at or over the line)");
    CHECK(feed, "the order feed's completeness is shown (MBO ...)");
    CHECK(lines >= 10, "the two lines are drawn");
    bool mbo100 = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "MBO 100%") mbo100 = true;
    CHECK(mbo100, "fills = bar volume: MBO 100%");
    // the record and a restart
    const std::string rec = flex + "\\Pulling\\ES-180-" + std::to_string(S->session) + ".csv";
    const std::string r = readAll(rec);
    CHECK(r.find("bar_time|bids_pulled") == 0 && std::count(r.begin(), r.end(), '\n') >= 22, "each closed bar is recorded");
    usleep(1100000); p->calc(0);
    std::string st = readAll(flex + "\\Pulling.status-ES-180.txt");
    CHECK(st.find("VERSION,0.1.0") != std::string::npos && st.find("STATE,ready") != std::string::npos, "status file: version and ready");
    p->destroy(); CHECK(h.dboObjects == 0 && h.data == nullptr, "destroy unsubscribes and frees the slot");
    Pulling* q = static_cast<Pulling*>(CreateExtension());
    h.now = t0 + 22 * 180 + 30; q->calc(0);
    PLState* R = q->testState();
    CHECK(R && R->e.bars.count(lb - 1) && R->e.bars[lb - 1].pull[pl::OFFER] == 10, "after a restart the recorded bars are back");
    CHECK(R && R->e.x(lb - 1, pl::OFFER) > 0.9 && R->e.x(lb - 1, pl::OFFER) < 1.1, "and their x normal is drawn again");
    q->destroy();
    // no order feed: says so after 60 s
    h.dboOn = false; h.data = nullptr;
    Pulling* z = static_cast<Pulling*>(CreateExtension()); z->calc(0);
    PLState* Z = z->testState(); Z->openedMs -= 61000; z->calc(0);
    CHECK(Z->state == "no order feed (MBO) on this chart", "no MBO on the chart is said plainly");
    z->destroy();
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
