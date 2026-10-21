// test_aggbook_host.cpp -- lsBidAskStats 1.13.0 (default) / lsBookFlow 1.3.0 (-DBOOK) end to end on the mock host (2026-10-18):
//   the forming candle is drawn dim with no label (no repaint), the closed candle bright + its ratio label (one rule, >= 2.0x the plain
//   20-candle average), Book Flow colours by side + the net-based label, the record's spike columns, the settings rows (load / apply /
//   chart re-reads), the Market State facts (SRC AGG / BOOK) posted once decided and cleared on removal.
#ifdef BOOK
#include "BookFlow.cpp"
typedef BookFlow Ind;
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
static long long& NOW = off::off_test_now_ms;
#else
#include "BidAskStats.cpp"
typedef BidAskStats Ind;
long long bas_test_now_ms = 1000000;
void* bas_test_cursor_wnd = nullptr;
static long long& NOW = bas_test_now_ms;
#endif
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
// (1.15.0 / 1.4.0) the Read lines drawn: every one inside the box (left / right / bottom), joined = the read shown (nothing cut)
[[maybe_unused]] static bool readFits(mock::Host& h, const bp::Box& b, const br::Read& r, const std::vector<std::string>& lines, std::string& why) {
    if (lines.empty()) { why = "no lines"; return false; }
    std::string joined;
    for (const auto& ln : lines) {
        const mock::Draw* d = nullptr; for (auto& x : h.draws) if (x.type == "text" && x.text == ln && x.color == (COLOR)r.color()) d = &x;
        if (!d) { why = "line not drawn: " + ln; return false; }
        if (d->x < b.l || d->r > b.r + 2 || d->y < b.t || d->b > b.b + 1) { why = "outside the box: " + ln; return false; }
        joined += (joined.empty() ? "" : " ") + ln;
    }
    bool ok = false; for (size_t n = 0; n <= r.opt.size(); ++n) ok = ok || joined == r.text(n);
    if (!ok) { why = "cut: " + joined; return false; }
    return true;
}   // (1.14.1 / 1.3.4) the screen cursor (PaneCursor.h)
[[maybe_unused]] static bool hasPrefix(mock::Host& h, const std::string& p) { for (auto& d : h.draws) if (d.type == "text" && d.text.compare(0, p.size(), p) == 0) return true; return false; }
static RTX_EVENT moveAt(int h, int v, long bar) { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)h; e.v.mouse.v = (short)v; e.v.mouse.bar = bar; return e; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); static bool once = false; if (!once) { once = true; p->setup(); } return p; }   // (2026-10-18) IRT runs setup once per DLL: the settings rows are registered there (SettingsHost.h)
static const unsigned long T0 = 1791900000UL;
[[maybe_unused]] static int count(mock::Host& h, const char* type, COLOR c) { int n = 0; for (auto& d : h.draws) if (d.type == type && d.color == c) ++n; return n; }
static const mock::Draw* text(mock::Host& h, const std::string& t, COLOR c = 0xFFFFFFFF) { for (auto& d : h.draws) if (d.type == "text" && d.text == t && (c == 0xFFFFFFFF || d.color == c)) return &d; return nullptr; }
static int pTexts(mock::Host& h) { int n = 0; for (auto& d : h.draws) if (d.type == "text" && d.text.size() > 1 && d.text[0] == 'p' && d.text[1] == ' ') ++n; return n; }
// the boxes drawn inside one candle's column (x between its slot edges): bars only (the info box has a grey border and black fill)
static std::vector<mock::Draw> barsAt(mock::Host& h, int bar) {
    std::vector<mock::Draw> v; const int xc = 50 + bar * h.barW, half = h.barW / 2;
    for (auto& d : h.draws) if (d.type == "box" && d.color != 0x00000000 && d.border != off::C_BOX_BORDER && d.x >= xc - half && d.r <= xc + half && d.color != off::C_NET_ZERO) v.push_back(d);
    return v;
}
// (2026-10-18 settings layer) the plugins no longer carry index constants: the rows keep their 1.13.0 / 1.3.0 positions (registration order)
static const int POS_PARM = 0, BIGLOT_PARM = 1;   // (1.14.0) Big Lot mark on the Position line   // (1.13.3) the window shows the Position only (Ratio bars / Spike vs average hidden, Offset X / Y removed)
static const int BF_POS = 0, BF_NEAR = 1;   // (1.3.3) the window: Position + Near-price ticks (Ratio bars / Spike vs average hidden, Record and Offset X / Y removed)
#define APPLY(p) do { const std::string sv_ = h.period; h.period = ""; (p)->parmsApply(); h.period = sv_; } while (0)
#define LOAD(p) do { const std::string sv_ = h.period; h.period = ""; (p)->parmsLoad(); h.period = sv_; } while (0)

#ifndef BOOK
static mock::Bar bar(unsigned long t, long bid, long ask) {
    mock::Bar b; b.time = t; b.close = 7000.0f; b.open = 7000.0f; b.low = 6999.5f; b.high = 7000.5f;
    b.rows.push_back(mock::Price(7000.0f, ask, bid, bid + ask)); b.rows.push_back(mock::Price(7000.25f, 2, 2, 4)); b.volume = (unsigned long)(bid + ask + 4); return b;
}
int main() {
    char tmpl[] = "/tmp/agg113XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 300; h.paneRight = 1400; h.scaleLeft = 1380; h.ppb = 20; h.barW = 20; h.listIndex = 0;
    for (int i = 0; i < 30; ++i) h.bars.push_back(bar(T0 + 60UL * i, 10, 10));
    h.bars.push_back(bar(T0 + 60UL * 30, 50, 10));                        // forming: 50 at the bid = 5.0x if it closed now
    Ind* p = make(); p->calc(0); p->draw(); p->calc(0);
    h.draws.clear(); p->draw();
    {   std::vector<mock::Draw> v = barsAt(h, 30);
#ifndef BOOK
        { const size_t all = v.size(); v.erase(std::remove_if(v.begin(), v.end(), [](const mock::Draw& d) { return d.color == off::C_WHITE; }), v.end());
          CHECK(all == 3 && v.size() == 2, "(1.17.0) the 50-lot bid bar is over the scale cap (1.5x the 10-lot p90): a white tip on it"); }
#endif
        CHECK(v.size() == 2 && ((v[0].color == C_BID_BAR && v[1].color == C_ASK_BAR) || (v[1].color == C_BID_BAR && v[0].color == C_ASK_BAR)), "(1.16.0) forming candle: BOTH sides drawn, dim red + dim green (no spike colour)");
        CHECK(count(h, "box", C_BID_SPIKE) == 0 && !text(h, "5.0"), "forming candle: no bright bar, no ratio label (no repaint)"); }
    h.bars.push_back(bar(T0 + 60UL * 31, 10, 10)); NOW += 100; p->calc(0); NOW += 600; p->calc(0);
    BASState* S = p->testState();
    CHECK(S->fin.count(T0 + 1800) && S->ser.bars[T0 + 1800].bidSpike && !S->ser.bars[T0 + 1800].askSpike, "closed: the bid side is a spike (5.0x >= 2.0x)");
    h.draws.clear(); p->draw();
    {   std::vector<mock::Draw> v = barsAt(h, 30);
        v.erase(std::remove_if(v.begin(), v.end(), [](const mock::Draw& d) { return d.color == off::C_WHITE; }), v.end());   // (1.17.0) the cap tip
        const mock::Draw* rd = nullptr; const mock::Draw* gr = nullptr; for (auto& d : v) { if (d.color == C_BID_SPIKE) rd = &d; if (d.color == C_ASK_BAR) gr = &d; }
        CHECK(v.size() == 2 && rd && gr, "(1.16.0) closed candle: bright red sellers + dim green buyers");
        CHECK(rd && gr && gr->x < rd->x && gr->b == rd->b, "(1.18.0) side by side above zero: buyers (green) LEFT, sellers (red) RIGHT, same bottom");
        CHECK(text(h, bas::QUESTION) != nullptr, "(1.18.0) the question line on top");
        {   const mock::Draw* q = text(h, bas::QUESTION); const mock::Draw* ring = nullptr; for (auto& d : h.draws) if (d.type == "ring") ring = &d;
            CHECK(q && ring && ring->x > q->r && ring->y >= q->y - 8 && ring->y <= q->y + 8, "(1.22.0) the info icon just right of the question, same line");
            CHECK(text(h, "i", 0x00000000) != nullptr, "(1.22.0) the icon's black i");
            CHECK(std::string(bas::QUESTION) == "Is the effort rewarded?", "(1.22.0) the question text");
            off::IconRect r; r.l = 100; r.t = 2; r.r = 112; r.b = 14; bool on = false; const size_t n0 = h.draws.size();
            CHECK(off::infoTip(r, on, 106, 8, bas::INFO_TIP) && on, "(1.22.0) mouse on the icon: the tip turns on");
            CHECK(!off::infoTip(r, on, 107, 9, bas::INFO_TIP) && on, "(1.22.0) still on it: no change");
            {   off::PaneGeo pg; pg.left = 0; pg.right = 1200; pg.top = 0; pg.bottom = 150; pg.edge = 1100;
                off::drawInfoTip(p, pg, r, bas::INFO_TIP);
                int lines = 0; bool ex = false; int maxY = 0, minY = 99999, cols = 0; std::set<int> xs;
                for (size_t k = n0; k < h.draws.size(); ++k) if (h.draws[k].type == "text") { ++lines; if (h.draws[k].text.find("Example - ES 8:18 PM") != std::string::npos) ex = true; maxY = std::max(maxY, h.draws[k].b); minY = std::min(minY, h.draws[k].y); xs.insert(h.draws[k].x); }
                cols = (int)xs.size();
                CHECK(lines >= 12 && ex, "(1.23.0) the tip is drawn as many lines (not one long line), the example included");
                CHECK(maxY <= 148 && cols >= 2, "(1.23.0) a 150 px pane: the lines fit in the pane, in 2+ columns"); }
            CHECK(off::infoTip(r, on, 300, 80, bas::INFO_TIP) && !on, "(1.22.0) mouse leaves: the tip goes");
            h.draws.erase(h.draws.begin() + (long)n0, h.draws.end()); }
        CHECK(text(h, "4.3", off::C_LBL_RATIO) == nullptr && count(h, "box", C_BID_SPIKE) >= 1, "(1.24.0, Rassul 14:33) the closed 4.3x candle is bright but carries no number label");
        CHECK(pTexts(h) == 0, "no \"p \" pace prefix");
#ifndef BOOK
        CHECK(text(h, "Effort") != nullptr && text(h, "Reward") != nullptr, "(1.20.0) box rows Effort + Reward");
#endif
    }
    // a closed 1.9x candle: dim, no label; exactly 2.0x: bright (one rule, at the close)
    h.bars.back() = bar(T0 + 60UL * 31, 23, 10); h.bars.push_back(bar(T0 + 60UL * 32, 10, 10)); NOW += 100; p->calc(0); NOW += 600; p->calc(0);
    CHECK(!S->ser.bars[T0 + 1860].bidSpike, "a 1.9x candle (23 vs 11.95) is not a spike");
    // the Market State: facts of the newest decided candle, keyed by symbol / bar size / chart label, re-posted, cleared on removal
    h.draws.clear(); p->draw(); NOW += 16000; p->calc(0);
    mst::Facts F = mst::read("AGG", "ESZ6", 60, "test", NOW);
    CHECK(F.ok && F.kv.size() == 9 && F.kv["spike"] == "none" && F.kv["sell_lots"] == "23" && F.kv["sweep"] == "?", "AGG facts for the last closed candle (no order feed: sweep ?)");
    {   time_t v = (time_t)(T0 + 1860); struct tm x; gmtime_r(&v, &x);
        CHECK(F.bar == off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec, "bar = the candle's Central wall-clock seconds"); }
    NOW += 50000; p->calc(0);
    CHECK(mst::read("AGG", "ESZ6", 60, "test", NOW).ok, "re-posted while the chart runs (stays fresh past 60 s)");
    // settings: the dialog rows, Apply writes the 8-field file (old fields carried), the chart re-reads, a change re-decides
    LOAD(p);
    CHECK(h.lists[POS_PARM] == 0, "parmsLoad fills the Position");
    CHECK(basIds().id[ofset::bask::K_BIG_LOT] == BIGLOT_PARM && h.lists[BIGLOT_PARM] == 1, "(1.14.0) parmsLoad fills Big Lot mark = BL5 (an 8-field file)");
    h.lists[POS_PARM] = 4; APPLY(p);
    std::string line = readAll(flex + "\\BidAskStats.place-ES.txt");
    CHECK(line == "20,4,20,15,10,0,0,0,1\n", "Apply: 9-field file (1.14.0: + Big Lot mark BL5), the hidden Ratio bars / Spike vs average / jump / lookback / Box shows carried, Position 4, offsets 0");
    // the hidden Spike vs average still comes from the file (data-tuned, his decision: not in the window); a saved offset is ignored
    { std::ofstream f((flex + "\\BidAskStats.place-ES.txt").c_str()); f << "20,4,60,15,10,7,3,0\n"; }
    NOW += 1500; p->calc(0);
    CHECK(S->ser.spike.avgT == 60 && S->dx == 0 && S->dy == 0 && !S->ser.bars[T0 + 1800].bidSpike, "the chart reads the file: 5.0x < 6.0x is no longer a spike; a saved Offset 7 / 3 is ignored");
    h.draws.clear(); p->draw();
    CHECK(count(h, "box", C_BID_SPIKE) >= 1 && text(h, "4.3") == nullptr, "(1.19.0) the bars use Attack Effort 2.0x (the validated rule), not the record's Spike vs average (bright, no label since 1.24.0)");
    // (1.14.1, Rassul 23:19 CT) the box follows the mouse ONLY while the cursor is inside this pane; anywhere else = Live (the forming candle), updating
    {   void* W = (void*)0x1; bas_test_cursor_wnd = W; pc::test_ok = true;
        h.draws.clear(); p->draw();
        CHECK(text(h, "Live") != nullptr && text(h, "12") != nullptr, "(1.19.0) no mouse event yet: Live = the forming candle (12 total lots at the bid so far)");
        CHECK(text(h, "BBI / BAI") == nullptr && text(h, "Pace") == nullptr, "(1.19.0) no BBI / BAI and no Pace rows");
        pc::test_cx = 1450; pc::test_cy = 650; RTX_EVENT ev = moveAt(450, 150, 10); p->mouse(&ev);   // inside the pane over candle 10
        h.draws.clear(); p->draw();
        CHECK(S->hover.bar == 10 && text(h, "Live") == nullptr, "cursor inside the pane over candle 10: the box shows that candle");
        {   RTX_EVENT e30 = moveAt(450, 150, 30); p->mouse(&e30); h.draws.clear(); p->draw();
            CHECK(text(h, "52", C_BID) != nullptr, "(1.19.0) candle 30: Lots = 52 total lots sold");
            p->mouse(&ev); h.draws.clear(); p->draw(); }
        NOW += 300; p->calc(0); h.draws.clear(); p->draw();
        CHECK(S->hover.bar == 10, "still inside (no new event): the candle stays");
        pc::test_cy = 900;                                                  // the cursor moved below the pane (price pane / elsewhere): no event comes
        NOW += 300; p->calc(0); h.draws.clear(); p->draw();
        CHECK(S->hover.bar == -1 && text(h, "Live") != nullptr && text(h, "12") != nullptr, "cursor outside the pane: Live, the forming candle");
        h.bars.back() = bar(h.bars.back().time, 77, 10);                    // the forming candle trades: 77 lots at the bid
        const int inv = h.invalidates; NOW += 300; p->calc(0);
        CHECK(h.invalidates > inv, "a new tick on the forming candle while Live: redrawn");
        h.draws.clear(); p->draw();
        CHECK(text(h, "79") != nullptr && text(h, "Live") != nullptr, "(1.19.0) Live box shows the updated 79 total lots with the mouse outside the pane");
        {   std::string why; const bool fits = readFits(h, S->lastBox, S->lastRead, S->lastReadLines, why);
            CHECK(text(h, "Read") != nullptr && text(h, "BEARISH", br::C_BEAR) != nullptr && S->lastRead.head.find("Sellers attacking") == 0, "(1.19.0) Read: BEARISH, Sellers attacking (Attack Effort, live)");
            CHECK(fits && S->lastReadLines.size() <= 2, ("(1.15.0) the Read lines fit inside the box, <= 2 lines, nothing cut " + why).c_str());
            CHECK(text(h, "BL5") == nullptr, "(1.15.0) no BL5 row"); }
        CHECK(text(h, "32", C_BID) == nullptr, "(1.19.0) no BBI count any more");
        pc::test_cy = 650; ev = moveAt(450, 150, 12); p->mouse(&ev); NOW += 300; p->calc(0);
        CHECK(S->hover.bar == 12, "back inside: follows the mouse again");
        bas_test_cursor_wnd = (void*)0x2; NOW += 300; p->calc(0);        // same screen spot but another window on top / another program
        CHECK(S->hover.bar == -1, "the cursor over another window: Live");
        bas_test_cursor_wnd = W; ev = moveAt(450, 400, 12); p->mouse(&ev);  // an event reported outside the pane rectangle
        CHECK(S->hover.bar == -1, "an event outside the pane rectangle never picks a candle");
        pc::test_ok = false; bas_test_cursor_wnd = nullptr; }
    LOAD(p); h.lists[BIGLOT_PARM] = 3; APPLY(p);                          // (1.14.0) Big Lot mark BL20 saved as the 9th field, read by the chart
    CHECK(readAll(flex + "\\BidAskStats.place-ES.txt") == "20,4,60,15,10,0,0,0,3\n", "(1.14.0) Apply: Big Lot mark BL20 = field 9 (3), the hidden values carried");
    NOW += 1500; p->calc(0); CHECK(S->bigLot == 3, "(1.14.0) the chart reads Big Lot mark BL20");
    {   // (1.24.0, Rassul 07:43 "do it", on after the 2026-10-20 close) ORDER-BY-ORDER TOTALS: bars / Lots / Effort / Reward from the exchange's aggressor fills
        BASState T; T.firstBarKey = 100; long long o[2] = {0, 0};
        BasFin d; d.dbo = true; d.r.aggrC[off::BID] = 700; d.r.aggrC[off::ASK] = 400; T.fin[200] = d;
        CHECK(mboTotOf(T, 200, o) && o[0] == 700 && o[1] == 400, "a decided order-feed candle: its aggressor totals (bids hit 700, offers lifted 400)");
        BasFin pp = d; pp.r.partial = true; T.fin[300] = pp; CHECK(!mboTotOf(T, 300, o), "a partial candle: falls back to IRT");
        BasFin nd = d; nd.dbo = false; T.fin[310] = nd; CHECK(!mboTotOf(T, 310, o), "no order feed for that candle: falls back to IRT");
        off::Acc ac; off::Aggr g1; g1.side = off::BID; g1.size = 30; off::Aggr g2; g2.side = off::ASK; g2.size = 12; off::Aggr g3; g3.side = off::BID; g3.size = 5;
        ac.aggr[1] = g1; ac.aggr[2] = g2; ac.aggr[3] = g3; T.accs[400] = ac;
        CHECK(mboTotOf(T, 400, o) && o[0] == 35 && o[1] == 12, "a candle still collecting: its open accumulator summed by side (35 / 12)");
        T.accs[100] = ac; CHECK(!mboTotOf(T, 100, o), "the first candle after the feed opened: partial, falls back");
        CHECK(!mboTotOf(T, 999, o), "no data at all: falls back");
        bas::BarMax m; m.totBid = 11; m.totAsk = 22; bool src = false;
        bas::BarMax w = withMbo(T, 200, m, &src); CHECK(src && w.totBid == 700 && w.totAsk == 400, "withMbo: the bar takes the order-feed totals");
        w = withMbo(T, 999, m, &src); CHECK(!src && w.totBid == 11 && w.totAsk == 22, "withMbo: IRT's bid / ask volume kept, flagged as IRT");
        T.accs[400].aggrCap = true;
        CHECK(!mboTotOf(T, 400, o), "audit: capped live aggressor map is incomplete, not MBO totals");
        w = withMbo(T, 400, m, &src); CHECK(!src && w.totBid == 11 && w.totAsk == 22, "audit: capped live map keeps IRT fallback values");
        T.accs[400].aggrCap = false; T.accs[400].aggrUnknown = 5;
        CHECK(!mboTotOf(T, 400, o), "audit: unidentified aggressor fills force IRT fallback");
        T.accs[400].aggrUnknown = 0; T.accs[400].aggr[1].size = -1;
        CHECK(!mboTotOf(T, 400, o), "audit: negative live aggressor size rejected");
        T.accs[400].aggr[1].size = 30; T.accs[400].aggr[1].side = 7;
        CHECK(!mboTotOf(T, 400, o), "audit: invalid live aggressor side rejected");
        T.accs[400].aggr[1].side = off::BID; T.accs[400].aggr[1].size = LLONG_MAX;
        CHECK(!mboTotOf(T, 400, o), "audit: per-side accumulation overflow rejected");
        T.fin[200].r.aggrC[0] = -1; CHECK(!mboTotOf(T, 200, o), "audit: negative persisted total rejected");
        T.fin[200].r.aggrC[0] = LLONG_MAX; T.fin[200].r.aggrC[1] = 1;
        CHECK(mboTotOf(T, 200, o) && o[0] == LLONG_MAX && o[1] == 1, "audit: valid independent side totals do not overflow a combined sum");
        CHECK(BAS_MBO_TOTALS, "the switch is on"); }
    {   bool lotsIrt = false; for (auto& d : h.draws) lotsIrt = lotsIrt || (d.type == "text" && d.text == "Lots (IRT)");
        CHECK(lotsIrt, "(1.24.0) no order data on the test chart: the box row reads 'Lots (IRT)'"); }
    {   // Exercise actual private finalization via the public calc/pump path, not a sliced substitute.
        S->firstBarKey = T0 + 1800;
        for (int scenario = 0; scenario < 4; ++scenario) {
            const unsigned long t = h.bars.back().time;
            if (scenario != 2) {
                off::Acc a; off::Aggr g; g.side = off::BID; g.size = 20; a.aggr[1] = g;
                a.aggrCap = scenario == 0; a.aggrUnknown = scenario == 1 ? 5 : 0; S->accs[t] = a;
            }
            h.bars.push_back(bar(t + 60, 10, 10)); NOW += 100; p->calc(0); NOW += 600; p->calc(0);
            const auto f = S->fin.find(t); long long totals[2] = {0, 0};
            CHECK(f != S->fin.end() && f->second.dbo && f->second.r.partial == (scenario != 3), "audit: candle-close finalization persists capped/unknown/missing accumulator quality");
            CHECK(mboTotOf(*S, t, totals) == (scenario == 3), "audit: closed incomplete data falls back; complete closed MBO remains available");
        }
    }
    dispose(p);
    CHECK(!mst::read("AGG", "ESZ6", 60, "test", NOW).ok, "removed: the Market State entry is cleared");
    std::printf("BidAskStats host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
#else
static mock::Bar bar(unsigned long t) { mock::Bar b; b.time = t; b.close = 7000.0f; b.open = 7000.0f; b.low = 6999.5f; b.high = 7001.0f; b.volume = 100; b.rows.push_back(mock::Price(7000.0f, 50, 50, 100)); return b; }
static int nextId = 1;
static void ev(unsigned char a, unsigned char side, float px, unsigned long size, RTDATE t, int id) { mock::feed.push(a, side, px, size, t, id); }
static void step(mock::Host& h, Ind* p, int k) { h.bars.push_back(bar(T0 + 60UL * k)); NOW += 100; p->calc(0); NOW += 600; p->calc(0); }
int main() {
    char tmpl[] = "/tmp/bf130XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 60; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 300; h.paneRight = 2000; h.ppb = 10; h.barW = 20; h.listIndex = 0;
    for (int i = 0; i < 31; ++i) h.bars.push_back(bar(T0 + 60UL * i));
    mkdir(flex.c_str(), 0700);
    { std::ofstream f((flex + "\\BookFlow.place-ES.txt").c_str()); f << "20,0,20,15,10,0,0,0,0,0\n"; }   // record off (no MBO files in the test)
    Ind* p = make(); p->calc(0); auto* S = p->testState();
    h.draws.clear(); p->draw();
    std::vector<int> offers;                                              // resting offer ids (5 lots each) to pull later
    for (int k = 30; k < 52; ++k) {                                       // 22 normal candles: +20 bids, +20 offers resting near price
        const RTDATE t = T0 + 60 * k - 30;
        for (int i = 0; i < 4; ++i) { ev(0, 1, 6999.75f, 5, t, nextId++); const int a = nextId++; ev(0, 0, 7000.25f, 5, t, a); offers.push_back(a); }
        NOW += 50; p->calc(0); step(h, p, k + 1);
    }
    CHECK(S->ser.bars.size() == 22 && S->ser.bars.begin()->second.r.partial, "22 decided candles, the first partial");
    const long long k51 = T0 + 60 * 51;
    CHECK(S->ser.bars[k51].r.addC[0] == 20 && S->ser.bars[k51].r.addC[1] == 20, "a normal candle: 20 lots added per side");
    // candle 52 (forming): 100 bid lots arrive, 100 resting offer lots are pulled
    {   const RTDATE t = T0 + 60 * 52 - 30;
        for (int i = 0; i < 5; ++i) ev(0, 1, 6999.75f, 20, t, nextId++);
        for (int i = 0; i < 20; ++i) ev(2, 0, 7000.25f, 5, t, offers[(size_t)i]); }
    NOW += 50; p->calc(0);
    h.draws.clear(); p->draw();
    {   std::vector<mock::Draw> v = barsAt(h, 52);
        v.erase(std::remove_if(v.begin(), v.end(), [](const mock::Draw& d) { return d.color == off::C_WHITE; }), v.end());   // (1.7.0) a cap tip is not a bar
        int gd = 0, rd = 0, bright = 0; for (auto& d : v) { gd += d.color == off::C_GREEN_BAR; rd += d.color == off::C_RED_BAR; bright += d.color == off::C_GREEN_SPIKE || d.color == off::C_RED_SPIKE; }
        CHECK(v.size() == 2 && gd == 1 && rd == 1 && bright == 0, "forming candle: bid bar dim green, offer bar dim red, nothing bright");
        CHECK(!text(h, "5.0"), "forming candle: no label (no repaint)");
        CHECK(!hasPrefix(h, "P "), "(1.4.0) the forming candle never gets a pull marker (100 offer lots pulled so far)"); }
    step(h, p, 53);
    const long long k52 = T0 + 60 * 52;
    {   h.draws.clear(); p->draw();                                       // (1.4.0) candle 52 closed: offers pulled 100 lots (avg 0 -> floored 1 lot), added 0 -> marker
        int markers = 0, blocks = 0; const mock::Draw* lab = nullptr;
        for (auto& d : h.draws) { if (d.type == "text" && d.text.compare(0, 2, "P ") == 0 && d.color == off::C_LBL_RATIO) { ++markers; lab = &d; } }
        for (auto& d : h.draws) if (d.type == "box" && d.color == off::C_RED_SPIKE && lab && d.b < lab->y + 1 && d.b >= lab->y - 9) ++blocks;
        CHECK(markers == 0 && !lab, "(1.8.0) no pull-marker strip any more (the decision is still kept, below)");
        (void)blocks;
        Ind::Ext* e = p->ext(*S); const bf::PullStat* ps = e ? &e->pulls[k52] : nullptr;
        CHECK(ps && ps->pulling[1] && !ps->pulling[0] && ps->pull[1] == 100 && ps->add[1] == 0, "(1.4.0) the decision kept for candle 52: offers pulling, bids not"); }
    CHECK(S->ser.bars.count(k52) && S->ser.bars[k52].r.addC[0] == 100 && S->ser.bars[k52].r.pullC[1] == 100, "candle 52 decided: +100 bids added, 100 offer lots pulled");
    CHECK(S->ser.bars[k52].spike[0] && S->ser.bars[k52].spike[1], "the NET spike on both sides (|+100| and |-100| = 5.0x the |net| average 20)");
    CHECK(S->file.rows.count(k52) && S->file.rows[k52].spike[0] == 1 && S->file.rows[k52].spike[1] == 1, "the BookFlow record keeps the net spike (spike_left / spike_right)");
    CHECK(S->file.rows.count(k51) && S->file.rows[k51].spike[0] == 0, "a normal candle's record: no spike");
    h.draws.clear(); p->draw();
    {   const std::vector<mock::Draw> v = barsAt(h, 52);
        const int y0 = 300 / 2; (void)y0;
        const mock::Draw* g = nullptr; const mock::Draw* r = nullptr;
        for (auto& d : v) { if (d.color == off::C_GREEN_SPIKE) g = &d; if (d.color == off::C_RED_SPIKE) r = &d; }
        const mock::Draw* stub = nullptr; for (auto& d : v) if (d.color == off::C_RED_BAR) stub = &d;
        CHECK(g && !r && stub, "(1.8.0) closed: bid bar bright GREEN; the offers' NEGATIVE net (100 pulled) is a red stub, never below zero");
        CHECK(g && stub && g->x < stub->x, "bid bar left, offer bar right");
        CHECK(g && stub && stub->b == g->b && stub->b - stub->y <= 2, "(1.8.0) both from the same bottom; the stub is 2 px");
        int labels = 0; for (auto& d : h.draws) if (d.type == "text" && d.text == "5.0" && d.color == off::C_LBL_RATIO) ++labels;
        CHECK(labels == 0, "(1.14.0, Rassul 14:33) no number labels over the bars (the 5.0 bid spike is bright only); the stub carries none");
        CHECK(pTexts(h) == 0, "no \"p \" pace prefix");
        int brightOthers = 0; for (int b = 30; b < 52; ++b) for (auto& d : barsAt(h, b)) brightOthers += d.color == off::C_GREEN_SPIKE || d.color == off::C_RED_SPIKE;
        CHECK(brightOthers == 0, "normal candles are dim"); }
    // the info box Net row carries the ratio
    {   int found = 0; for (auto& d : h.draws) if (d.type == "text" && (d.text == "+100 (5.0x)" || d.text == "-100 (5.0x)")) ++found;
        CHECK(found == 2, "box Net: +100 (5.0x) | -100 (5.0x)"); }
    // the Market State
    NOW += 16000; p->calc(0);
    mst::Facts F = mst::read("BOOK", "ESZ6", 60, "test", NOW);
    CHECK(F.ok && F.kv.size() == 8 && F.kv["bid_net"] == "100" && F.kv["ask_net"] == "-100" && F.kv["bid_net_x"] == "5.0" && F.kv["ask_net_x"] == "5.0"
          && F.kv["bid_state"] == "arriving" && F.kv["ask_state"] == "leaving" && F.kv["spike"] == "bid_arriving", "BOOK facts of candle 52");
    {   time_t v = (time_t)k52; struct tm x; gmtime_r(&v, &x);
        CHECK(F.bar == off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec, "bar = the candle's Central wall-clock seconds"); }
    NOW += 50000; p->calc(0);
    CHECK(mst::read("BOOK", "ESZ6", 60, "test", NOW).ok, "re-posted while the chart runs");
    // settings: the 2 rows, Apply keeps the file layout, the chart re-reads
    LOAD(p);
    CHECK(h.lists[BF_POS] == 0 && h.parms[BF_NEAR] == 0, "parmsLoad fills Position / Near-price ticks from the file");
    h.parms[BF_NEAR] = 3; APPLY(p);
    CHECK(readAll(flex + "\\BookFlow.place-ES.txt") == "20,0,20,15,10,3,1,0,0,0\n", "Apply: Near 3 written, the hidden Ratio / Spike vs average / jump / lookback carried, record 1 (always on), offsets 0");
    { std::ofstream f((flex + "\\BookFlow.place-ES.txt").c_str()); f << "20,0,60,15,10,0,0,0,5,0\n"; }   // an old file: record off, Offset Y 5, Spike vs average 6.0
    NOW += 1500; p->calc(0);
    CHECK(S->dlgSpike.avgT == 60 && S->set.dy == 0 && S->set.record == 1, "the chart reads the file: Spike vs average 6.0 (hidden, from the file), a saved offset ignored, recording on despite a saved 0");
    h.draws.clear(); p->draw();
    {   int bright = 0; for (auto& d : barsAt(h, 52)) bright += (d.color == off::C_GREEN_SPIKE || d.color == off::C_RED_SPIKE) && d.y < 250;   // (1.4.0) not the pull-marker block in the bottom strip
        CHECK(bright == 0 && !text(h, "5.0"), "5.0x under a 6.0x setting: dim, no label (the setting works)"); }
    // (1.3.4) the box follows the mouse only inside this pane; outside = Live (the forming candle), updating with new order events
    {   void* W = (void*)0x1; off::off_test_cursor_wnd = W; pc::test_ok = true;
        pc::test_cx = 1450; pc::test_cy = 650; RTX_EVENT e2 = moveAt(450, 150, 52); p->mouse(&e2);
        h.draws.clear(); p->draw();
        CHECK(S->hov.hover.bar == 52 && text(h, "Live") == nullptr && hasPrefix(h, "+100 ("), "inside the pane over candle 52: its Net +100");
        pc::test_cy = 900; NOW += 300; p->calc(0); h.draws.clear(); p->draw();
        CHECK(S->hov.hover.bar == -1 && text(h, "Live") != nullptr, "cursor outside the pane: Live");
        {   const RTDATE t = T0 + 60 * 53 - 30; for (int i = 0; i < 3; ++i) ev(0, 1, 6999.75f, 10, t, nextId++); }
        const int inv = h.invalidates; NOW += 300; p->calc(0);
        CHECK(h.invalidates > inv, "new order events on the forming candle while Live: redrawn");
        h.draws.clear(); p->draw();
        CHECK(text(h, "Live") != nullptr && hasPrefix(h, "+30 ("), "Live box: the forming candle's Net +30 bids so far with its ratio (1.4.0), mouse outside the pane");
        CHECK(text(h, "Refill Effort") != nullptr && text(h, "Refilled") != nullptr && text(h, "Added") == nullptr && text(h, "Pulled") == nullptr
              && text(h, "Pace") == nullptr && text(h, "Net") == nullptr, "(1.5.0) the box shows Refill Effort + Refilled only (Added / Pulled / Pace hidden)");
        CHECK(text(h, "Reward") == nullptr, "(1.8.0) no Reward row (it repeats Aggression's)");
        CHECK(text(h, bf::QUESTION) != nullptr, "(1.8.0) the question line on top");
        {   Ind::Ext* e = p->ext(*S); std::string why; const bool fits = e && readFits(h, S->lastBox, e->lastRead, e->lastReadLines, why);
            CHECK(e && text(h, "BULLISH", br::C_BULL) != nullptr && e->lastRead.head.find("Bids stacking") == 0, "(1.4.0) Read: BULLISH, Bids stacking (live, offers flat)");
            CHECK(fits, ("(1.4.0) the Read lines fit inside the box, nothing cut " + why).c_str()); }
        pc::test_ok = false; off::off_test_cursor_wnd = nullptr; }
    // (1.4.0) no repaint: later candles never move or remove the marker (decided once at the close)
    for (int k = 54; k < 58; ++k) step(h, p, k);
    {   h.draws.clear(); p->draw(); int markers = 0; for (auto& d : h.draws) if (d.type == "text" && d.text == "P 100.0") ++markers;
        Ind::Ext* e = p->ext(*S); CHECK(markers == 0 && e && e->pulls.count(k52) && e->pulls[k52].pulling[1], "(1.8.0) 4 candles later: the same decision kept (no marker drawn)"); }
    {   // (1.5.0) spotAt reads the chart's own bars: a new 10-bar low on 2.5x the volume of the 20 before it
        std::vector<mock::Bar>& B = mock::current->bars; const int i = (int)B.size() - 2;
        for (int k = i - 20; k < i; ++k) { B[(size_t)k].low = 6999.5f; B[(size_t)k].high = 7001.0f; B[(size_t)k].volume = 100; }
        B[(size_t)i].low = 6998.0f; B[(size_t)i].high = 7000.5f; B[(size_t)i].volume = 250;
        bf::Spot sp = p->spotAt(i); CHECK(sp.ok && sp.newLow && !sp.newHigh && sp.busy, "(1.5.0) spotAt: new 10-bar low, busy (2.5x volume)");
        B[(size_t)i].low = 7000.0f; B[(size_t)i].volume = 120; sp = p->spotAt(i); CHECK(sp.ok && !sp.newLow && !sp.busy, "(1.5.0) spotAt: not new, not busy");
        CHECK(!p->spotAt(3).ok && !p->spotAt((int)B.size()).ok, "(1.5.0) spotAt: too early / out of range -> none"); }
    {   // (1.12.0) the pivot turn: candle i makes a new low, closes green, bids' Defense bar bigger -> decided once, written, drawn "+NN%"
        std::vector<mock::Bar>& B = mock::current->bars; const long long k = S->ser.bars.rbegin()->first; int i = -1;
        for (size_t q = 0; q < B.size(); ++q) if ((long long)B[q].time == k) i = (int)q;
        CHECK(i >= 2 && S->barEnds.count(k) && S->barEnds[k] == i, "(1.12.0) the last decided candle is on the chart");
        B[(size_t)i - 2].open = 7001; B[(size_t)i - 2].high = 7002; B[(size_t)i - 2].low = 7000; B[(size_t)i - 2].close = 7000.5f;
        B[(size_t)i - 1].open = 7000.5f; B[(size_t)i - 1].high = 7001; B[(size_t)i - 1].low = 6999; B[(size_t)i - 1].close = 6999.5f;
        B[(size_t)i].open = 6999.5f; B[(size_t)i].high = 7000.5f; B[(size_t)i].low = 6997; B[(size_t)i].close = 7000.25f;
        CHECK(S->ser.bars.count(k) == 1, "(1.12.0) the turn candle is decided");
        off::PaneBar nb = S->ser.bars[k]; nb.r.addC[0] = 90; nb.r.pullC[0] = 0; nb.r.addC[1] = 60; nb.r.pullC[1] = 0; nb.r.aggrC[0] = 700; nb.r.aggrC[1] = 400;
        S->ser.bars[k] = nb;
        p->decideTurns(*S, k, nb); p->decideTurns(*S, k, nb);
        Ind::Ext* e = p->ext(*S);
        const auto it = e ? e->dtTurns.find(std::make_pair((int)dt::LOW, k)) : decltype(e->dtTurns.end())();
        CHECK(e && it != e->dtTurns.end() && it->second.marginPct == 50 && it->second.A && it->second.linePx == 27988, "(1.12.0) LOW turn: +50%, sellers out-hit (A), line at the low 6997 (no refill seen)");
        const std::string tf = readAll(flex + "\\BookFlow\\ES-60-turns.csv");
        int lines = 0; for (char ch : tf) lines += ch == '\n';
        CHECK(lines == 1 && tf.compare(0, 2, "T|") == 0, "(1.12.0) one line in the turns file (decided once, not twice)");
        h.draws.clear(); p->draw();
        CHECK(text(h, "50%", 0x22C55E) != nullptr, "(1.12.0) the pane mark +50% in green over the bids' bar");
        CHECK(readAll(flex + "\\BookFlow\\ES-60-turns-live.txt").size() >= 2, "(1.12.0) the live file is published (N or the forming ?)");
        { std::ofstream f((flex + "\\DefenseLines.settings-ES.txt").c_str()); f << ss::format(dlset::spec(), [] { ss::Values V = ss::defaults(dlset::spec(), 0); V.v[dlset::K_SHOW_MARKS] = 0; return V; }()); }
        NOW += 1500; h.draws.clear(); p->draw();
        CHECK(text(h, "50%") == nullptr, "(1.12.0) Turn marks off in the shared settings: no pane mark");
        // a reload (restart): the turn comes back from the file and is never decided again
        e->dtLoaded = false; e->dtPath.clear(); p->decideTurns(*S, k, nb);
        CHECK(e->dtTurns.count(std::make_pair((int)dt::LOW, k)) == 1 && readAll(flex + "\\BookFlow\\ES-60-turns.csv") == tf, "(1.12.0) after a reload: read back, not written twice");
        // (1.15.0, Top Defender #5) the A turn is waiting for its counter; then H decided on that candle: the badge row + the Counter row
        CHECK(e->dtPendH.empty() || e->dtPendH.back() == std::make_pair((int)dt::LOW, k), "(1.15.0) the A turn waits for its counter (or the reload cleared it)");
        h.draws.clear(); p->draw();
        CHECK(text(h, "A?", 0x22C55E) != nullptr, "(1.15.0) badge row: A? in green on the turn candle (buyers defend a low)");
        {   dt::Counter C; C.t = k; C.side = dt::LOW; C.H = true; C.lots = 920; C.attack = 400; C.at = k; e->dtH[std::make_pair((int)dt::LOW, k)] = C; }
        h.draws.clear(); p->draw();
        if (const char* dp = std::getenv("D5_DUMP")) { std::ofstream o(dp); for (auto& d : h.draws) o << d.type << '\t' << d.x << '\t' << d.y << '\t' << d.r << '\t' << d.b << '\t' << d.color << '\t' << d.border << '\t' << d.fsize << '\t' << d.text << '\n'; }   // render check (not a test)
        const mock::Draw* bd = text(h, "A|H?", 0x22C55E);
        CHECK(bd != nullptr && text(h, "A?") == nullptr, "(1.15.0) one combined badge A|H? on the candle (not two)");
        CHECK(bd && bd->y >= 0 && bd->y < 60, "(1.15.0) the badge row sits at the top of the pane");
        {   bool over = false; if (bd) over = bd->x < S->lastBox.r && bd->x + 10 > S->lastBox.l && bd->y < S->lastBox.b && bd->y + 10 > S->lastBox.t;
            CHECK(!over, "(1.15.0) the badge never over the info box"); }
        CHECK(text(h, "Counter") != nullptr && text(h, "Hit back 2.3x") != nullptr, "(1.15.0) the box row Counter: Hit back 2.3x (920 / 400)");
        {   const mock::Draw* q = text(h, bf::QUESTION);
            CHECK(q && q->x >= S->lastBox.l && q->x < S->lastBox.r && q->y >= S->lastBox.t - 2 && q->y < S->lastBox.t + 20, "(1.15.0, Rassul 11:30) the question is the box's first line, upper left inside the box");
            const mock::Draw* ring = nullptr; for (auto& d : h.draws) if (d.type == "ring" || d.type == "oval") ring = &d;
            CHECK(q && ring && ring->x > q->x && ring->x < S->lastBox.r && ring->y >= S->lastBox.t - 2 && ring->y < S->lastBox.t + 20, "(1.15.0) the info icon right of it, inside the box");
            CHECK(e->infoIcon.r > e->infoIcon.l && e->infoIcon.l >= S->lastBox.l && e->infoIcon.r <= S->lastBox.r, "(1.15.0) the icon's hover rectangle moved with it (hover keeps working)"); }
        e->dtH[std::make_pair((int)dt::LOW, k)].H = false; h.draws.clear(); p->draw();
        CHECK(text(h, "A?") != nullptr && text(h, "Absorbed only") != nullptr, "(1.15.0) decided without a counter: A? only, Counter = Absorbed only");
        // (1.16.0, Top Defender #4) the re-test: the engine's resting lots in the zone are read; a decided L on the candle -> one badge A|L?, box Re-test
        S->e.restAdd(off::BID, 27990, 300); S->e.restAdd(off::BID, 27995, 50);
        CHECK(p->restIn(*S, dt::LOW, 27988, 27992) == 300 && p->restIn(*S, dt::LOW, 27988, 27995) == 350, "(1.16.0) restIn sums the defenders' resting lots in the zone");
        {   dt::Retest q; q.t = k; q.side = dt::LOW; q.res = 'L'; q.ratio = 1.9; q.rest = 950; q.at = k; q.state = 2; e->dtQ[std::make_pair((int)dt::LOW, k)] = q; }
        e->dtH[std::make_pair((int)dt::LOW, k)].H = true;
        h.draws.clear(); p->draw();
        if (const char* dp = std::getenv("D4_DUMP")) { std::ofstream o(dp); for (auto& d : h.draws) o << d.type << '\t' << d.x << '\t' << d.y << '\t' << d.r << '\t' << d.b << '\t' << d.color << '\t' << d.border << '\t' << d.fsize << '\t' << d.text << '\n'; }   // render check (not a test)
        CHECK(text(h, "A|H|L?", 0x22C55E) != nullptr && text(h, "A|H?") == nullptr, "(1.16.0) one combined badge A|H|L? (font fits), in the defenders' green");
        CHECK(text(h, "Re-test") != nullptr && text(h, "Reloaded 1.9x") != nullptr, "(1.16.0) the box row Re-test: Reloaded 1.9x");
        {   const mock::Draw* rr = text(h, "Re-test"); const mock::Draw* cc = text(h, "Counter"); CHECK(rr && cc && rr->y > cc->y && rr->y < S->lastBox.b, "(1.16.0) Re-test under Counter, inside the box"); }
        e->dtQ[std::make_pair((int)dt::LOW, k)].res = 'X'; e->dtQ[std::make_pair((int)dt::LOW, k)].ratio = 0.2; e->dtH[std::make_pair((int)dt::LOW, k)].H = false;
        h.draws.clear(); p->draw();
        CHECK(text(h, "A|X?") != nullptr && text(h, "Empty 0.2x") != nullptr, "(1.16.0) X: badge A|X?, box Empty 0.2x");
        {   const std::string tf0 = readAll(flex + "\\BookFlow\\ES-60-turns.csv");
            { std::ofstream f((flex + "\\BookFlow\\ES-60-turns.csv").c_str(), std::ios::app); dt::Retest q; q.t = k; q.side = dt::LOW; q.res = 'L'; q.ratio = 1.25; q.rest = 625; q.at = k; f << dt::qLineOf(q) << "\n"; }
            e->dtLoaded = false; e->dtPath.clear(); p->decideTurns(*S, k, nb);
            CHECK(e->dtQ.count(std::make_pair((int)dt::LOW, k)) == 1 && e->dtQ[std::make_pair((int)dt::LOW, k)].res == 'L' && std::fabs(e->dtQ[std::make_pair((int)dt::LOW, k)].ratio - 1.25) < 1e-9, "(1.16.0) after a reload the Q line is read back (no repaint)"); (void)tf0; } }
    {   // (1.13.0) the touch rule in the engine: 40 lots hit at 7000.00 in the first 20 s, 2 in 20-60 s, 60 of 80 resting lots stay -> attack died
        off::Engine g; g.adMedian = 0.095; g.curKey = 77; g.restAdd(off::BID, 28000, 80);
        g.touchFill(off::BID, 28000, 30, 1000, 80); g.restAdd(off::BID, 28000, -30);
        g.touchFill(off::BID, 28000, 10, 1010, 50); g.restAdd(off::BID, 28000, -10); g.restAdd(off::BID, 28000, 20);
        g.touchFill(off::BID, 28000, 2, 1030, 60); g.restAdd(off::BID, 28000, -2);
        CHECK(g.touches.size() == 1 && !g.touches[0].done && g.touches[0].h1 == 40 && g.touches[0].h2 == 2 && g.touches[0].key == 77, "(1.13.0) one fresh touch, 40 lots in 0-20 s, 2 in 20-60 s");
        g.touchFill(off::BID, 27999, 1, 1061, 5);
        CHECK(g.touches[0].done && g.touches[0].ad && g.touches.size() == 2, "(1.13.0) finished at 60 s: slowed (0.05 / 2.05 = 0.02 <= 0.095) and 58 of 80 stayed -> attack died; a new touch at 6999.75");
        off::Engine q; q.adMedian = 0.095; q.restAdd(off::BID, 28000, 80);
        q.touchFill(off::BID, 28000, 10, 1000, 80); q.restAdd(off::BID, 28000, -70);
        q.finishTouches(1060); CHECK(q.touches[0].done && !q.touches[0].ad, "(1.13.0) 10 of 80 left: the defenders pulled / were eaten -> not exhaustion");
        q.touchFill(off::BID, 28000, 5, 1100, 10); CHECK(q.touches.size() == 1, "(1.13.0) the same price traded again within 5 min: not a fresh touch"); }
    {   // (1.13.0) checkX: the LOW turn decided above gets its exhaustion line once the touches on its pivot / turn candle are finished
        const long long k = S->ser.bars.rbegin()->first; Ind::Ext* e = p->ext(*S); const dt::Turn& t = e->dtTurns[std::make_pair((int)dt::LOW, k)];
        e->dtPendX.clear(); e->dtXDone.clear(); e->dtPendX.push_back(std::make_pair((int)dt::LOW, k));
        S->e.touches.clear(); off::Engine::Touch a1; a1.s = off::BID; a1.px = 27989; a1.key = t.pivotT; a1.t0 = 5000; a1.done = false;
        S->e.touches.push_back(a1); S->e.lastS = 5030;
        p->checkX(*S);
        CHECK(e->dtPendX.size() == 1 && e->dtXWritten == 0, "(1.13.0) a touch on the pivot candle still running: waiting");
        S->e.touches[0].done = true; S->e.touches[0].ad = true; S->e.lastS = 5070;
        off::Engine::Touch a2 = a1; a2.px = 27988; a2.done = true; a2.ad = false; S->e.touches.push_back(a2);
        p->checkX(*S);
        const std::string tf2 = readAll(flex + "\\BookFlow\\ES-60-turns.csv");
        CHECK(e->dtPendX.empty() && e->dtXWritten == 1 && tf2.find("X|" + std::to_string(k) + "|0|6997.25|") != std::string::npos, "(1.13.0) decided once: X line at 6997.25 (the touch where the attack died, not the one where it did not)");
        p->checkX(*S); CHECK(e->dtXWritten == 1, "(1.13.0) never written twice"); }
    dispose(p);
    CHECK(!mst::read("BOOK", "ESZ6", 60, "test", NOW).ok, "removed: the Market State entry is cleared");
    std::printf("BookFlow host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
#endif
