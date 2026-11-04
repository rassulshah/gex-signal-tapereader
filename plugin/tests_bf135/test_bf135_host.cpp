// test_bf135_host.cpp -- lsBookFlow 1.35.0 on the mock host (2026-11-04, Rassul "add wave poc as one of the levels for bookflow"):
//   lsWavePOC's record (lsFlexLevels\WavePOC\ES-180-nodes.csv) -> the WPOC level -> the box "WPOC 7010.00 | Defenders ..." on an arrival,
//   hidden away from it, nothing over the box, the S record line typed wave_poc, the status counts, the incremental read (a torn line waits
//   for its newline), an old trading day's wave ignored, a missing record = no level, no crash.
// Build (tests_bf135/): g++ -std=c++17 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-misleading-indentation -DOFF_TEST_CLOCK
//        -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock test_bf135_host.cpp -o th && ./th
#include "BookFlow.cpp"
typedef BookFlow Ind;
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
static long long& NOW = off::off_test_now_ms;
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); static bool once = false; if (!once) { once = true; p->setup(); } return p; }
static const unsigned long T0 = 1791900000UL;
static mock::Bar bar(unsigned long t) { mock::Bar b; b.time = t; b.close = 7000.0f; b.open = 7000.0f; b.low = 6999.5f; b.high = 7001.0f; b.volume = 100; b.rows.push_back(mock::Price(7000.0f, 50, 50, 100)); return b; }
static int nextId = 1;
static void ev(unsigned char a, unsigned char side, float px, unsigned long size, RTDATE t, int id, float bid = 0, float ask = 0) { mock::feed.push(a, side, px, size, t, id); mock::feed.ev.back().bid = bid; mock::feed.ev.back().ask = ask; }
static void step(mock::Host& h, Ind* p, int k) { h.bars.push_back(bar(T0 + 60UL * k)); NOW += 100; p->calc(0); NOW += 600; p->calc(0); }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static const mock::Draw* textPre(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text.compare(0, t.size(), t) == 0) return &d; return nullptr; }
static void candle(int k, float trade, int addLots, int every) {
    const RTDATE t0 = (RTDATE)(T0 + 60UL * (k - 1));
    for (int s = 1; s < 60; ++s) {
        const RTDATE t = t0 + s; const int r = nextId++; ev(0, 0, trade, 5, t, r, trade - 0.25f, trade); ev(3, 0, trade, 1, t, r, trade - 0.25f, trade);
        if (addLots > 0 && s % every == 0) ev(0, 1, 7010.0f, (unsigned long)addLots, t, nextId++, trade - 0.25f, trade);
    }
}
static void append(const std::string& path, const std::string& s) { std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); f << s; }
int main() {
    char tmpl[] = "/tmp/bf135XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mkdir(flex.c_str(), 0700); mkdir((flex + "\\WavePOC").c_str(), 0700);
    { std::ofstream f((flex + "\\BookFlow.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0,0,0\n"; }
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 300; h.paneRight = 2000; h.scaleLeft = 1980; h.ppb = 10; h.barW = 20; h.listIndex = 0;
    h.priceBase = -0.5f;
    for (int i = 0; i < 31; ++i) h.bars.push_back(bar(T0 + 60UL * i));
    Ind* p = make();
    // no record yet: no WPOC level, no crash
    p->calc(0); auto* S = p->testState(); h.draws.clear(); p->draw(); Ind::Ext* e = p->ext(*S);
    CHECK(std::string(BF_VERSION) == "1.35.0" && std::string(bf::VERSION) == "1.35.0" && std::string(blb::VERSION) == "1.1.0", "versions 1.35.0 / BookLevelBox 1.1.0");
    CHECK(readAll("../BookFlow.cpp").find("p->setVersion(\"1.35.0\")") != std::string::npos, "the setVersion literal 1.35.0 (gex-build reads it)");
    CHECK(e->lbN[blb::L_WPOC] == 0 && e->lbWpNote.find("no Wave POC record") != std::string::npos, "no record: no WPOC level (" + e->lbWpNote + ")");
    // the record: an up wave (bars 5 -> 20, confirmed at 23) with its POC 7010.00 (+ other nodes / ROWS / STOP lines, skipped), and a wave of
    // the previous trading day (never used); times = WavePOC::localSec of the bar ENDs
    bool ok = false; Ind* cur = p; auto L = [&](int k) { return std::to_string(cur->lbLocalSec((RTDATE)(T0 + 60UL * k), ok)); };
    const std::string path = flex + "\\WavePOC\\ES-180-nodes.csv";
    const long long old = p->lbLocalSec((RTDATE)(T0 - 2 * 86400), ok);
    append(path, "# WavePOC record: WAVE,ver,N,dir,t0,t1,... (header)\n");
    append(path, "WAVE,1.8.3,3,up," + std::to_string(old) + "," + std::to_string(old + 600) + "," + std::to_string(old + 900) + ",a,b,c,1,2,3,4,5,1,9,1,0,WAVE,0,BOTH,4.0\nNODE," + std::to_string(old) + "," + std::to_string(old + 600) + ",0,7009.00,9,4.0,WAVE\n");
    append(path, "WAVE,1.8.3,3,up," + L(5) + "," + L(20) + "," + L(23) + ",2026-10-14 09:00,b,c,6990.00,7020.00,30.00,20.00,20,1,121,3,0,WAVE,0,BOTH,4.0\n"
                 "NODE," + L(5) + "," + L(20) + ",0,7010.00,900,4.0,WAVE\nNODE," + L(5) + "," + L(20) + ",1,7015.00,700,3.0,WAVE\n"
                 "ROWS," + L(5) + "," + L(20) + ",6990.00,1,1;2;3\nSTOP," + L(5) + "," + L(20) + ",7015.00,new wave," + L(25) + ",x,WAVE\n");
    NOW += 6000; step(h, p, 31);
    CHECK(ok && e->lbN[blb::L_WPOC] == 1, "the drawn WPOC is watched (" + std::to_string(e->lbN[blb::L_WPOC]) + ", " + e->lbWpNote + ")");
    { bool found = false; for (auto& l : e->lb.L) found = found || (l.lv.name == "WPOC 7010.00" && l.lv.type == blb::L_WPOC && l.lv.lo == 28036 && l.lv.hi == 28044); CHECK(found, "level 'WPOC 7010.00', zone +-4 ticks, type wave_poc"); }
    { bool oldLv = false; for (auto& l : e->lb.L) oldLv = oldLv || l.lv.name == "WPOC 7009.00"; CHECK(e->lbWp.w.size() == 2 && e->lbWp.bad == 0 && !oldLv, "the previous trading day's wave is read (kept 3 days) but never a level"); }
    // ---- away (8 ticks above the zone): no box; then the arrival (support) ----
    for (int k = 31; k <= 34; ++k) { candle(k, 7013.0f, 2, 2); step(h, p, k + 1); }
    h.draws.clear(); p->draw();
    CHECK(!e->lbShown.on && !text(h, "WPOC 7010.00"), "price 8 ticks above the WPOC zone: no box");
    for (int k = 35; k <= 37; ++k) { candle(k, 7011.0f, 10, 1); step(h, p, k + 1); }
    h.draws.clear(); p->draw();
    CHECK(e->lbShown.on && e->lbShown.c.arrived && e->lbShown.c.ds == 0 && e->lbShown.c.type == blb::L_WPOC, "arrival at the WPOC from above: support");
    {   const mock::Draw* nm = text(h, "WPOC 7010.00"); const mock::Draw* d = textPre(h, "Defenders ");
        CHECK(nm && d && d->color == (COLOR)blb::BC_GREEN, "box: WPOC 7010.00 | Defenders ... green (" + (d ? d->text : std::string("none")) + ")");
        const bf::OneLine& OL = e->lastLine; const blb::Lay& Ly = e->lbLay; off::PaneGeo g = off::paneGeo(p, off::MODE_NONE);
        CHECK(OL.ok && Ly.ok && Ly.l >= OL.r + blb::BOX_AFTER - 1 && Ly.t == OL.t && Ly.r <= g.edge, "one row right of the Defending? line, inside the pane");
        int over = 0; for (auto& x : h.draws) if (x.type == "text" && x.y < Ly.b && x.b > Ly.t && x.r > Ly.l && x.x < Ly.r) { bool mine = false; for (auto& sg : Ly.segs) mine = mine || x.text == sg.text; if (!mine) ++over; }
        bool order = true; for (size_t q = 1; q < Ly.segs.size(); ++q) order = order && Ly.segs[q].x >= Ly.segs[q - 1].x + Ly.segs[q - 1].w;
        CHECK(over == 0 && order && !Ly.segs.empty() && Ly.segs[0].text == "WPOC 7010.00", "no overlap: nothing else over the box, segments left to right, the level name first (" + std::to_string(over) + ")"); }
    // the record S line carries ltype wave_poc
    CHECK(readAll(e->lbPath).find("|WPOC 7010.00|wave_poc|support|") != std::string::npos, "levelbox record: S line '|WPOC 7010.00|wave_poc|support|'");
    // ---- incremental read: a torn line waits for its newline; then a down wave POC 7005.00 is added ----
    append(path, "WAVE,1.8.3,3,down," + L(20) + "," + L(26) + "," + L(29) + ",a,b,c,1,2,3,4,5,1,9,1,0,WAVE,0,BOTH,4.0\nNODE," + L(20) + "," + L(26) + ",0,7005.0");
    NOW += 6000; step(h, p, 38);
    CHECK(e->lbN[blb::L_WPOC] == 1 && !e->lbWpPend.empty(), "a torn NODE line is held back (no level from it yet)");
    append(path, "0,800,3.0,WAVE\n");
    NOW += 6000; step(h, p, 39);
    { bool found = false; for (auto& l : e->lb.L) found = found || l.lv.name == "WPOC 7005.00"; CHECK(e->lbN[blb::L_WPOC] == 2 && found && e->lbWpPend.empty(), "completed: WPOC 7005.00 added (" + std::to_string(e->lbN[blb::L_WPOC]) + ")"); }
    // status
    {   NOW += 40000; p->calc(0); const std::string st = readAll(flex + "\\BookFlow\\status-ES-ESZ6-60.txt");
        CHECK(st.find("LEVEL_BOX,1.1.0") != std::string::npos && st.find(" wave_poc 2") != std::string::npos && st.find(",WPOC 2 WPOC (2 untested, 0 tested) from 3 recorded waves (N 3") != std::string::npos, "status: wave_poc 2 + the WPOC note"); }
    // a restart reads the record again from its tail: the same levels
    dispose(p);
    Ind* p2 = make(); cur = p2; p2->calc(0); NOW += 6000; p2->calc(0); Ind::Ext* e2 = p2->ext(*p2->testState());
    CHECK(e2->lbN[blb::L_WPOC] == 2, "restart: the same 2 WPOC levels from the record");
    // a record rewritten shorter (truncated): read again from the start
    { std::ofstream f(path.c_str(), std::ios::binary | std::ios::trunc); f << "WAVE,1.8.3,3,up," + L(5) + "," + L(20) + "," + L(23) + ",a,b,c,1,2,3,4,5,1,9,1,0\nNODE," + L(5) + "," + L(20) + ",0,7010.00,9,4.0\n"; }
    NOW += 6000; h.bars.push_back(bar(T0 + 60UL * 41)); p2->calc(0);
    CHECK(e2->lbN[blb::L_WPOC] == 1, "a truncated / rewritten record is read again from the start (" + std::to_string(e2->lbN[blb::L_WPOC]) + ")");
    dispose(p2);
    std::printf("BookFlow 1.35.0 host (WPOC): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
