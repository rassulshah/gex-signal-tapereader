// test_exhaustion_host.cpp -- lsExhaustion against the mock host. g++ -std=c++17 -Imock -I. test_exhaustion_host.cpp -o th && ./th
#include "Exhaustion.cpp"
#include <cstdio>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static mock::Bar bar(unsigned long t, float h, float l, long buy, long sell) { mock::Bar b; b.time = t; b.high = h; b.low = l; b.open = l; b.close = h; b.rows.push_back(mock::Price(h, buy, sell, buy + sell)); return b; }
int main() {
    char tmpl[] = "/tmp/exXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    unsigned long t = day + 8 * 3600 + 30 * 60 + 180;                       // first RTH bar END 8:33
    for (int i = 0; i < 25; ++i, t += 180) h.bars.push_back(bar(t, 7000.0f + 0.25f * (i % 2), 6999.0f, 500, 500));
    // a push up: new highs every bar, buyers 2000 -> 500 (ES: N 5, S 2, T 3.0)
    const long buyers[10] = {2000, 2000, 1800, 1500, 1200, 900, 700, 500, 400, 400};
    for (int i = 0; i < 10; ++i, t += 180) h.bars.push_back(bar(t, 7001.0f + i, 7000.0f + i, buyers[i], 300));
    h.bars.push_back(bar(t, 7009.0f, 7008.0f, 10, 10));                     // the forming bar (not a new high)
    h.now = t;
    Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
    EXState* S = p->testState();
    CHECK(S && S->P.N == 5 && S->P.S == 2 && S->P.T == 3.0, "ES uses its own study settings");
    // pace (S 2): bars 25.. -> 2000,2000,1900,1650,1350,1050,800,600,450,400; peak 2000; first >= 3.0x: 600 (2000/600 = 3.33) at bar 32
    long long eTime = (long long)h.bars[32].time;
    CHECK(S->marks.size() == 1 && S->marks.count(eTime) && S->marks[eTime] == -1, "one top E, at the first new high where buyers trade at a third of their peak");
    h.draws.clear(); p->draw();
    bool head = false, e = false, v = false;
    for (auto& d : h.draws) { if (d.type == "text" && d.text == "EXHAUSTION at pivots   ") head = true; if (d.type == "text" && d.text == "E?") e = true; if (d.type == "line") v = true; }
    CHECK(head && !e && v, "0.3.0: header 'EXHAUSTION at pivots', the E is recorded but not drawn, only the 2.0x level line");
    bool testing = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "testing") testing = true;
    CHECK(testing, "no nightly confirmation yet: the header says testing");
    const std::string rec = readAll(flex + "\\Exhaustion\\ES-ESZ6-180-" + std::to_string(S->session) + ".csv");
    CHECK(rec.find("|top|3.33|") != std::string::npos, "the E is recorded with its fade");
    // the forming bar never prints an E; after a restart the E is read back (never repainted)
    p->destroy();
    Exhaustion* q = static_cast<Exhaustion*>(CreateExtension()); q->calc(0);
    EXState* R = q->testState();
    CHECK(R && R->marks.count(eTime) && R->marks.size() == 1, "after a restart the same single E");
    // outside RTH: no E (a 6 PM push)
    q->destroy(); h.data = nullptr; h.bars.clear();
    unsigned long u = day + 18 * 3600;
    for (int i = 0; i < 25; ++i, u += 180) h.bars.push_back(bar(u, 7000.0f + 0.25f * (i % 2), 6999.0f, 500, 500));
    for (int i = 0; i < 10; ++i, u += 180) h.bars.push_back(bar(u, 7001.0f + i, 7000.0f + i, buyers[i], 300));
    h.bars.push_back(bar(u, 7009.0f, 7008.0f, 10, 10)); h.now = u;
    setenv("USERPROFILE", (std::string(tmp) + "x").c_str(), 1);
    Exhaustion* z = static_cast<Exhaustion*>(CreateExtension()); z->calc(0);
    CHECK(z->testState()->marks.empty() && z->testState()->liveB >= 1.0, "outside RTH the lines are drawn but no E prints");
    z->destroy();
    {   // (0.2.0) an overnight line from the study: E overnight at the first bar at or over it (4.0x here: 2000/450 = 4.4 at bar 33)
        const std::string fx = std::string(tmp) + "x\\InvestorRT\\rtx\\lsFlexLevels";
        std::ofstream nf((fx + "\\LRA-ExhNorms-ES.csv").c_str()); nf << "PARAM,N,5\r\nPARAM,S,2\r\nPARAM,T,3.0\r\nPARAM,T_overnight,4.0\r\nSTATUS,testing,x\r\n"; nf.close();
        h.data = nullptr; Exhaustion* w = static_cast<Exhaustion*>(CreateExtension()); w->calc(0);
        EXState* W = w->testState();
        CHECK(W->tOvernight == 4.0 && W->marks.size() == 1 && W->marks.count((long long)h.bars[33].time), "overnight E at the overnight line (4.0x), not at the RTH line");
        w->destroy();
    }
    {   // (0.2.1) the bar in progress is never read: a new high on 10 contracts used to read 2000/10 = 200x in the header
        Exhaustion* f = static_cast<Exhaustion*>(CreateExtension()); f->calc(0);
        EXState* F = f->testState();
        CHECK(F->out.fb.size() == h.bars.size() - 1 && std::fabs(F->liveB - F->out.fb.back()) < 1e-12 && F->liveB < 10, "0.2.1: closed bars only; header = the last closed bar");
        f->destroy(); h.data = nullptr;
    }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
