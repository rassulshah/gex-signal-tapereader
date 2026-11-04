// test_bf134_host.cpp -- lsBookFlow 1.34.0 end to end on the mock host (2026-11-04): the LEVEL BOX + SQUARES + the turn-mark label.
//   order events -> the engine (onRaw) -> the box (afterPump, 5 s late) -> drawn right of the Defending? line; hidden away from levels;
//   15-s persistence; squares decided per candle, drawn under the bars, recorded, reloaded after a restart unchanged (no repaint); the
//   axis keeps 10 px under the bars only while squares are in view; nothing overlaps; the status line; versions.
// Build (tests_bf134/): g++ -std=c++17 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-misleading-indentation -DOFF_TEST_CLOCK
//        -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock test_bf134_host.cpp -o t && ./t
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
static const unsigned long T0 = 1791900000UL;   // 14:00 UTC = the mock's clock (RTH for ES)
static mock::Bar bar(unsigned long t) { mock::Bar b; b.time = t; b.close = 7000.0f; b.open = 7000.0f; b.low = 6999.5f; b.high = 7001.0f; b.volume = 100; b.rows.push_back(mock::Price(7000.0f, 50, 50, 100)); return b; }
static int nextId = 1;
static void ev(unsigned char a, unsigned char side, float px, unsigned long size, RTDATE t, int id, float bid = 0, float ask = 0) { mock::feed.push(a, side, px, size, t, id); mock::feed.ev.back().bid = bid; mock::feed.ev.back().ask = ask; }
static void step(mock::Host& h, Ind* p, int k) { h.bars.push_back(bar(T0 + 60UL * k)); NOW += 100; p->calc(0); NOW += 600; p->calc(0); }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static const mock::Draw* textPre(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text.compare(0, t.size(), t) == 0) return &d; return nullptr; }
// one candle's events (60 s): a trade each second at `trade`, defender bids in the zone every `every` s, behind bids once
static void candle(int k, float trade, int addLots, int every, int behindLots) {
    const RTDATE t0 = (RTDATE)(T0 + 60UL * (k - 1));
    for (int s = 1; s < 60; ++s) {
        const RTDATE t = t0 + s;
        const int r = nextId++; ev(0, 0, trade, 5, t, r, trade - 0.25f, trade); ev(3, 0, trade, 1, t, r, trade - 0.25f, trade);   // an offer rests, 1 lot trades
        if (addLots > 0 && s % every == 0) ev(0, 1, 7010.0f, (unsigned long)addLots, t, nextId++, trade - 0.25f, trade);       // bids in the zone (7010 +- 1)
        if (behindLots > 0 && s == 1) ev(0, 1, 7007.0f, (unsigned long)behindLots, t, nextId++, trade - 0.25f, trade);         // bids 12 ticks under the level (behind)
    }
}
int main() {
    char tmpl[] = "/tmp/bf134XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mkdir(flex.c_str(), 0700);
    { std::ofstream f((flex + "\\BookFlow.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0,0,0\n"; }   // record off (no MBO files in the test)
    { std::ofstream f((flex + "\\LRA-KeyLevels-ES.csv").c_str()); f << "KEYLV,1," << (long long)std::time(NULL) << "\nL|TEST|7010.000000|0\n"; }
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 300; h.paneRight = 2000; h.scaleLeft = 1980; h.ppb = 10; h.barW = 20; h.listIndex = 0;
    h.priceBase = -0.5f;                                                    // the mock's value 0 sits 10 px above the pane bottom (room for the squares)
    for (int i = 0; i < 31; ++i) h.bars.push_back(bar(T0 + 60UL * i));
    Ind* p = make(); p->calc(0); auto* S = p->testState(); h.draws.clear(); p->draw();
    Ind::Ext* e = p->ext(*S);
    CHECK(std::string(BF_VERSION) == "1.34.0" && std::string(bf::VERSION) == "1.34.0", "versions 1.34.0 (BF_VERSION / bf::VERSION; ofset::BF_VERSION in test_blb_logic)");
    CHECK(readAll("../BookFlow.cpp").find("p->setVersion(\"1.34.0\")") != std::string::npos, "the setVersion literal 1.34.0 (gex-build reads it)");
    step(h, p, 31);
    // ---- phase A: 6 candles at 7013 (8 ticks above the zone 7009-7011): far from the level -> no box ----
    for (int k = 31; k <= 36; ++k) { candle(k, 7013.0f, 2, 2, k == 31 ? 20 : 0); step(h, p, k + 1); }
    h.draws.clear(); p->draw();
    CHECK(e->lb.L.size() >= 1 && e->lbN[blb::L_KEY] == 1, "the KeyLevels level TEST is watched (" + std::to_string(e->lbN[blb::L_KEY]) + " key levels)");
    CHECK(!e->lbShown.on && !text(h, "TEST") && !e->lbLay.ok, "price 8 ticks above the zone: no box");
    CHECK(e->lbSq.empty() && e->lbAcc.empty(), "no squares for candles away from levels");
    int baseZero = 0; { double mn = 0, mx = 0; p->scale(0, (int)h.bars.size() - 1, &mn, &mx); baseZero = (int)std::lround(mn * 1000); }
    const size_t drawsAway = h.draws.size();
    // ---- phase B: price comes down into the zone (support) and stays 3 candles; defenders stack 10 lots / s, 200 lots arrive behind ----
    const long long tArr = (long long)(T0 + 60UL * 36 + 1);
    std::vector<std::string> texts; std::vector<long long> ks;
    for (int k = 37; k <= 39; ++k) {
        candle(k, 7011.0f, 10, 1, k == 37 ? 200 : 0);
        step(h, p, k + 1);
        h.draws.clear(); p->draw();
        texts.push_back(e->lbText.part[0] + " | " + e->lbText.part[1] + " | " + e->lbText.part[2]); ks.push_back(e->lbShown.k);
    }
    CHECK(e->lb.arrivals >= 1 && e->lbShown.on && e->lbShown.c.arrived && e->lbShown.c.anchor == tArr && e->lbShown.c.ds == 0, "arrival at the first trade in the zone after 8 ticks away: support (anchor " + std::to_string(e->lbShown.c.anchor - (long long)T0) + ")");
    CHECK(e->lbLastE == e->lb.cur - blb::MINREST, "the box shows the second 5 s before the newest (the durable-add rule)");
    {   const mock::Draw* nm = text(h, "TEST"); const mock::Draw* d = textPre(h, "Defenders ");
        CHECK(nm && d && d->text == "Defenders stacking 20x+?" && d->color == (COLOR)blb::BC_GREEN, "box: TEST | Defenders stacking 20x+? in green (" + (d ? d->text : std::string("none")) + ")");
        const mock::Draw* b = textPre(h, "Behind "); const mock::Draw* a = textPre(h, "Attackers ");
        CHECK(b && b->text.find("Behind stacking") == 0 && b->color == (COLOR)blb::BC_GREEN && b->text.back() == '?', "Behind stacking ...x? green at a support (" + (b ? b->text : std::string("none")) + ")");
        CHECK(a && a->text == "Attackers normal?" && a->color == (COLOR)blb::BC_GRAY, "Attackers normal? gray, always with ?");
        const bf::OneLine& OL = e->lastLine; const blb::Lay& L = e->lbLay;
        CHECK(OL.ok && L.ok && L.l >= OL.r + blb::BOX_AFTER - 1 && L.t == OL.t && L.b - L.t == bf::LINE_H, "one row, right of the Defending? line, same top, 16 px high");
        off::PaneGeo g = off::paneGeo(p, off::MODE_NONE);
        CHECK(L.r <= g.edge, "inside the pane, left of the price axis");
        int over = 0; for (auto& x : h.draws) if (x.type == "text" && x.y < L.b && x.b > L.t && x.r > L.l && x.x < L.r) { bool mine = false; for (auto& sg : L.segs) mine = mine || x.text == sg.text; if (!mine) ++over; }
        CHECK(over == 0, "nothing else drawn over the box (" + std::to_string(over) + ")");
        bool order = true; for (size_t q = 1; q < L.segs.size(); ++q) order = order && L.segs[q].x >= L.segs[q - 1].x + L.segs[q - 1].w; CHECK(order && L.segs.size() == 7, "level | D | B | A left to right"); }
    // 15-s persistence: the shown state at k = 10 was still the previous one, at k >= 15 + the window it is stacking
    {   blb::Engine::Case c = e->lbShown.c; blb::Engine T = e->lb; T.sm = blb::Engine::Smooth(); T.cache.clear();
        T.rebuild(c, 2); const int w2 = T.sm.cur[0].word; T.sm = blb::Engine::Smooth(); T.rebuild(c, 40); const int w40 = T.sm.cur[0].word;
        // the raw read turns stacking once 40 lots arrived in 30 s (k ~ 3); the box follows 15 s later
        T.sm = blb::Engine::Smooth(); T.rebuild(c, 10); const int w10 = T.sm.cur[0].word;
        CHECK(w2 == blb::W_QUIET && w10 == blb::W_QUIET && w40 == blb::W_STACK, "persistence: quiet at +2 / +10 s, stacking at +40 s (a change shows after 15 s)"); }
    // ---- phase C: price leaves (7016, 20 ticks above): the box hides; the 3 candles at the level get their squares ----
    for (int k = 40; k <= 41; ++k) { candle(k, 7016.0f, 0, 1, 0); step(h, p, k + 1); }
    h.draws.clear(); p->draw();
    CHECK(!e->lbShown.on && !text(h, "TEST") && !textPre(h, "Defenders "), "price 20 ticks away: the box is hidden");
    const long long k37 = (long long)(T0 + 60UL * 37), k38 = k37 + 60, k39 = k38 + 60;
    CHECK(e->lbSq.size() == 4 && e->lbSq.count(k37) && e->lbSq.count(k38) && e->lbSq.count(k39) && !e->lbSq.count(k37 - 60), "squares for the candles that spent time at the level only (" + std::to_string(e->lbSq.size()) + ")");
    CHECK(e->lbSq.count(k39 + 60) && e->lbSq[k39 + 60].secs == 1 && e->lbSq[k39 + 60].col[0] == blb::GRAY, "the candle that left after 1 s at the level: gray (< 20 s of a colour)");
    CHECK(e->lbSq[k38].col[0] == blb::GREEN && e->lbSq[k38].col[1] == blb::GREEN && e->lbSq[k38].secs >= 55, "a candle with 60 s of green Defenders / Behind: two green squares");
    {   int nD = 0, nB = 0; for (auto& x : h.draws) if (x.type == "box" && x.color == (COLOR)blb::BC_GREEN && x.b - x.y == blb::SQ_H) { if (x.y == blb::sqRowTop(std::min(h.paneBottom - 1, (int)(h.paneBottom - (0 - h.priceBase) * 20)), 0)) ++nD; else ++nB; }
        CHECK(nD >= 2 && nB >= 2, "squares drawn: two rows under the bars (" + std::to_string(nD) + " / " + std::to_string(nB) + ")");
        int hit = 0; for (auto& sq : h.draws) { if (!(sq.type == "box" && sq.b - sq.y == blb::SQ_H && (sq.color == (COLOR)blb::BC_GREEN || sq.color == (COLOR)blb::BC_RED || sq.color == (COLOR)blb::BC_GRAY))) continue;
            for (auto& b : h.draws) if (b.type == "box" && &b != &sq && (b.color == off::C_GREEN_BAR || b.color == off::C_RED_BAR || b.color == off::C_GREEN_SPIKE || b.color == off::C_RED_SPIKE) && b.y < sq.b && b.b > sq.y && b.x < sq.r && b.r > sq.x) ++hit; }
        CHECK(hit == 0, "the squares never overlap a bar (" + std::to_string(hit) + ")"); }
    {   const int n = (int)h.bars.size() - 1; double mn = 0, mx = 0; p->scale(0, n, &mn, &mx);
        std::map<long long, blb::Square> keep; keep.swap(e->lbSq); double mn2 = 0, mx2 = 0; p->scale(0, n, &mn2, &mx2); keep.swap(e->lbSq);
        double mn3 = 0, mx3 = 0; p->scale(n - 1, n, &mn3, &mx3);           // squares out of view (the last 2 candles: no square)
        const double unit = -mn2;
        CHECK(unit > 0 && std::fabs(mn - (mn2 - blb::SQ_RESERVE * unit)) < 1e-6 * unit && std::fabs(mx - (mx2 - blb::SQ_RESERVE * unit)) < 1e-6 * unit, "squares in view: the axis keeps 10 px under the bars (same px per lot)");
        keep.swap(e->lbSq); double mn4 = 0, mx4 = 0; p->scale(n - 1, n, &mn4, &mx4); keep.swap(e->lbSq);
        CHECK(mn3 == mn4 && mx3 == mx4, "squares out of view: the axis exactly as without the level box"); (void)baseZero; }
    (void)drawsAway;
    // the record: S lines on state changes, C lines per square
    const std::string rec = readAll(e->lbPath);
    {   int nS = 0, nC = 0; std::istringstream is(rec); std::string ln; while (std::getline(is, ln)) { if (ln.compare(0, 2, "S|") == 0) ++nS; if (ln.compare(0, 2, "C|") == 0) ++nC; }
        CHECK(!e->lbPath.empty() && e->lbPath.find("BookFlow\\ES-ESZ6-60-") != std::string::npos && e->lbPath.find("-levelbox.csv") != std::string::npos, "record BookFlow\\ES-ESZ6-60-<session>-levelbox.csv");
        CHECK(nC == 4 && nS >= 3 && nS < 30 && rec.compare(0, 13, "# lsBookFlow ") == 0, "record: a header, 4 C lines, a few S lines on changes (" + std::to_string(nS) + " S)"); }
    // no repaint: more candles, then a restart (a new chart object reading the record) -> the same squares
    std::map<long long, blb::Square> before = e->lbSq;
    for (int k = 42; k <= 43; ++k) { candle(k, 7016.0f, 0, 1, 0); step(h, p, k + 1); }
    bool same = e->lbSq.size() == before.size(); for (auto& kv : before) same = same && e->lbSq.count(kv.first) && e->lbSq[kv.first].col[0] == kv.second.col[0] && e->lbSq[kv.first].col[1] == kv.second.col[1];
    CHECK(same, "decided squares never change");
    dispose(p);
    Ind* p2 = make(); p2->calc(0); auto* S2 = p2->testState(); NOW += 1000; p2->calc(0); h.draws.clear(); p2->draw(); Ind::Ext* e2 = p2->ext(*S2);
    bool same2 = e2->lbSq.size() == before.size(); for (auto& kv : before) same2 = same2 && e2->lbSq.count(kv.first) && e2->lbSq[kv.first].col[0] == kv.second.col[0] && e2->lbSq[kv.first].col[1] == kv.second.col[1];
    CHECK(same2 && e2->lbLoaded == 4, "restart: the squares come back from the record, identical (" + std::to_string(e2->lbSq.size()) + ")");
    {   int n = 0; for (auto& x : h.draws) if (x.type == "box" && x.b - x.y == blb::SQ_H && x.color == (COLOR)blb::BC_GREEN) ++n; CHECK(n >= 4, "restart: the squares are drawn again"); }
    {   std::string st = readAll(flex + "\\BookFlow\\status-ES-ESZ6-60.txt"); NOW += 40000; p2->calc(0); st = readAll(flex + "\\BookFlow\\status-ES-ESZ6-60.txt");
        CHECK(st.find("LEVEL_BOX,1.0.0,params defaults") != std::string::npos && st.find("TURN_LABEL,") != std::string::npos, "status: LEVEL_BOX + TURN_LABEL lines"); }
    // the nightly's params: SHOW D drops the "?" on Defenders; a wrong-market file = defaults with the reason
    {   std::ofstream f((flex + "\\LRA-BookLevel-ES.csv").c_str()); f << "VERSION|1\nMARKET|ES\nPARAM|def_x|1.5\nPARAM|quiet_lots|40\nSHOW|D|1\nSHOW|B|0\n"; }
    NOW += 6000; p2->calc(0);
    CHECK(e2->lbP.file && e2->lbP.showD && !e2->lbP.showB && e2->lb.P.showD, "LRA-BookLevel-ES.csv read: SHOW D 1");
    {   blb::Engine::Shown sh; sh.on = true; sh.c.name = "X"; sh.c.ds = 0; sh.r[0].word = blb::W_STACK; sh.r[0].x = 2.1; sh.r[0].col = blb::GREEN; sh.r[1].word = blb::W_STEADY; sh.r[2].word = blb::W_NORMAL;
        const blb::BoxText t = blb::boxText(sh, e2->lb.P); CHECK(t.part[0] == "Defenders stacking 2.1x" && t.part[1] == "Behind steady?" && t.part[2] == "Attackers normal?", "proven line: no ?; unproven: ?"); }
    {   std::ofstream f((flex + "\\LRA-BookLevel-ES.csv").c_str()); f << "VERSION|1\nMARKET|NQ\nPARAM|def_x|1.5\n"; }
    NOW += 6000; p2->calc(0);
    CHECK(!e2->lbP.file && e2->lbP.why.find("wrong market") != std::string::npos, "a wrong-market file: defaults, the reason kept (" + e2->lbP.why + ")");
    dispose(p2);
    std::printf("BookFlow 1.34.0 host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
