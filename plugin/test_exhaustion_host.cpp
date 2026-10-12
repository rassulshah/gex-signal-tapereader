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
    auto norms = [&](const std::string& root, const std::string& text) { const std::string fx = root + "\\InvestorRT\\rtx\\lsFlexLevels"; std::ofstream nf((fx + "\\LRA-ExhNorms-ES.csv").c_str()); nf << text; };
    long long eTime = (long long)h.bars[32].time;
    {   // (0.3.0) no study confirmation yet: the two lines only - no E, no written value, header "testing"
        Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
        EXState* S = p->testState();
        CHECK(S && S->P.N == 5 && S->P.S == 2 && S->P.T == 3.0 && S->P.K == 0, "ES uses its own study settings (0.2 read until the norms say else)");
        CHECK(S->marks.empty(), "testing: no E");
        h.draws.clear(); p->draw(); bool val = false, testing = false, head = false;
        for (auto& d : h.draws) { if (d.type == "text" && d.text == "3.3x") val = true; if (d.type == "text" && d.text == "testing") testing = true; if (d.type == "text" && d.text == "EXHAUSTION   ") head = true; }
        CHECK(head && testing && !val, "testing: header and lines, no value written");
        CHECK(S->out.fb.size() == h.bars.size() - 1, "closed bars only: the bar in progress is not computed");
        p->destroy();
    }
    norms(tmp, "PARAM,N,5\r\nPARAM,S,2\r\nPARAM,T,3.0\r\nSTATUS,confirmed,x\r\n");
    {   // confirmed: E and its value, once
        h.data = nullptr; Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
        EXState* S = p->testState();
        // pace (S 2): bars 25.. -> 2000,2000,1900,1650,1350,1050,800,600,450,400; peak 2000; first >= 3.0x: 600 (3.33) at bar 32
        CHECK(S->marks.size() == 1 && S->marks.count(eTime) && S->marks[eTime] == -1, "one top E, at the first new high where buyers trade at a third of their peak");
        h.draws.clear(); p->draw();
        int vals = 0; bool e = false, conf = false;
        for (auto& d : h.draws) { if (d.type == "text" && d.text.size() > 1 && d.text.back() == 'x' && d.color == C_SELL && d.text != "3.0x" && d.text != "5.0x") ++vals;   // 5.0x = the header (last closed bar 2000/400)
                                  if (d.type == "text" && d.text == "E" && d.color == C_SELL) e = true; if (d.type == "text" && d.text == "confirmed") conf = true; }
        CHECK(e && conf, "the red E and 'confirmed' are drawn");
        CHECK(vals == 1, "the value is written once (bars 33-34 are over the line too, but not first crossings)");
        const std::string rec = readAll(flex + "\\Exhaustion\\ES-180-" + std::to_string(S->session) + ".csv");
        CHECK(rec.find("|top|3.33|") != std::string::npos && rec.find("K0 P0.0") != std::string::npos, "the E is recorded with its fade and read");
        p->destroy();
        Exhaustion* q = static_cast<Exhaustion*>(CreateExtension()); q->calc(0);
        CHECK(q->testState()->marks.count(eTime) && q->testState()->marks.size() == 1, "after a restart the same single E (never repainted)");
        q->destroy();
    }
    {   // (0.3.0) the bar in progress: a new high on 10 contracts used to read 2000/10 = 200x; now it is never read and the header shows the last closed bar
        h.bars.back() = bar(t, 7015.0f, 7008.0f, 10, 10); h.data = nullptr;
        Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
        EXState* S = p->testState();
        CHECK(S->liveB < 6.0 && std::fabs(S->liveB - S->out.fb.back()) < 1e-12, "header = the last closed bar, not the half-built one");
        h.bars.back() = bar(t, 7015.0f, 7008.0f, 400, 10); p->calc(0);
        CHECK(S->marks.size() == 1, "the forming bar never adds an E");
        p->destroy(); h.bars.back() = bar(t, 7009.0f, 7008.0f, 10, 10);
    }
    {   // (0.3.0) the push gate from the norms: K 3 P 5 -> the push's peak (2000) is under 5 x normal (500): no fade, no E
        norms(tmp, "PARAM,N,5\r\nPARAM,S,2\r\nPARAM,T,3.0\r\nPARAM,K,3\r\nPARAM,P,5\r\nPARAM,SIDES,3\r\nSTATUS,confirmed,x\r\n");
        const std::string d2 = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\Exhaustion"; std::remove((d2 + "\\ES-180-" + std::to_string(civilDays(2026, 10, 12) - civilDays(2026, 10, 12) + 0) + ".csv").c_str());
        h.data = nullptr; Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
        EXState* S = p->testState();
        CHECK(S->P.K == 3 && S->P.P == 5.0 && S->P.sides == 3, "K / P / SIDES read from the norms");
        bool any = false; for (size_t k = 25; k < S->out.fb.size(); ++k) if (S->out.fb[k] != 1.0) any = true;
        CHECK(!any, "a push under P x normal pace reads 1.0");
        p->destroy();
        norms(tmp, "PARAM,N,5\r\nPARAM,S,2\r\nPARAM,T,3.0\r\nPARAM,SIDES,2\r\nSTATUS,confirmed,x\r\n");
        h.data = nullptr; Exhaustion* q = static_cast<Exhaustion*>(CreateExtension()); q->calc(0);
        EXState* Q = q->testState(); int e = 0; for (char c : Q->out.eTop) e += c;
        CHECK(Q->P.sides == 2 && e == 0, "SIDES 2: no top E");
        q->destroy();
    }
    // overnight (a 6 PM push), own folder
    h.data = nullptr; h.bars.clear();
    unsigned long u = day + 18 * 3600;
    for (int i = 0; i < 25; ++i, u += 180) h.bars.push_back(bar(u, 7000.0f + 0.25f * (i % 2), 6999.0f, 500, 500));
    for (int i = 0; i < 10; ++i, u += 180) h.bars.push_back(bar(u, 7001.0f + i, 7000.0f + i, buyers[i], 300));
    h.bars.push_back(bar(u, 7009.0f, 7008.0f, 10, 10)); h.now = u;
    const std::string tx = std::string(tmp) + "x"; setenv("USERPROFILE", tx.c_str(), 1);
    {
        Exhaustion* z = static_cast<Exhaustion*>(CreateExtension()); z->calc(0);
        CHECK(z->testState()->marks.empty() && z->testState()->liveB >= 1.0, "overnight with no study line: lines drawn, no E");
        z->destroy();
    }
    {   // 0.2 norms (bare T_overnight = the RTH read at that line, E where it agreed): 4.0x first at bar 33 (2000/450 = 4.4)
        norms(tx, "PARAM,N,5\r\nPARAM,S,2\r\nPARAM,T,3.0\r\nPARAM,T_overnight,4.0\r\nSTATUS,testing,x\r\n");
        h.data = nullptr; Exhaustion* w = static_cast<Exhaustion*>(CreateExtension()); w->calc(0);
        EXState* W = w->testState();
        CHECK(W->tOvernight == 4.0 && W->marks.size() == 1 && W->marks.count((long long)h.bars[33].time), "0.2 norms: overnight E at the overnight line (4.0x)");
        w->destroy();
    }
    {   // 0.3 overnight read, testing: lines at its line, no E
        const std::string d = tx + "\\InvestorRT\\rtx\\lsFlexLevels\\Exhaustion";
        norms(tx, "PARAM,N,5\r\nPARAM,S,2\r\nPARAM,T,3.0\r\nPARAM,T_overnight,4.0\r\nPARAM,K_overnight,10\r\nPARAM,P_overnight,1.0\r\nPARAM,SIDES_overnight,1\r\nSTATUS,confirmed,x\r\nSTATUS_overnight,testing,x\r\n");
        h.data = nullptr; Exhaustion* w = static_cast<Exhaustion*>(CreateExtension()); w->calc(0);
        EXState* W = w->testState();
        CHECK(W->onSet && W->Pon.K == 10 && W->Pon.T == 4.0 && W->provenOn == "testing" && std::fabs(W->liveT - 4.0) < 1e-12, "overnight read set, its line 4.0x shown");
        CHECK(W->marks.size() == 1, "testing overnight: no new E (only the one recorded earlier this session)");
        w->destroy();
    }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
