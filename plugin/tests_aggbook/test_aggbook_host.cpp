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
static const int RATIO_PARM = 0, POS_PARM = 1, SPIKE_AVG_PARM = 2, OFFX_PARM = 3, OFFY_PARM = 4;
static const int BF_RATIO = 0, BF_POS = 1, BF_AVG = 2, BF_NEAR = 3, BF_RECORD = 4, BF_OFFX = 5, BF_OFFY = 6;
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
    {   const std::vector<mock::Draw> v = barsAt(h, 30);
        CHECK(v.size() == 1 && v[0].color == C_BID_BAR, "forming candle: drawn, dim red (no spike colour)");
        CHECK(count(h, "box", C_BID_SPIKE) == 0 && !text(h, "5.0"), "forming candle: no bright bar, no ratio label (no repaint)"); }
    h.bars.push_back(bar(T0 + 60UL * 31, 10, 10)); NOW += 100; p->calc(0); NOW += 600; p->calc(0);
    BASState* S = p->testState();
    CHECK(S->fin.count(T0 + 1800) && S->ser.bars[T0 + 1800].bidSpike && !S->ser.bars[T0 + 1800].askSpike, "closed: the bid side is a spike (5.0x >= 2.0x)");
    h.draws.clear(); p->draw();
    {   const std::vector<mock::Draw> v = barsAt(h, 30);
        CHECK(v.size() == 1 && v[0].color == C_BID_SPIKE, "closed candle: bright red");
        CHECK(text(h, "5.0", off::C_LBL_RATIO) != nullptr, "closed candle: its ratio label 5.0 (yellow), no x");
        CHECK(pTexts(h) == 0, "no \"p \" pace prefix"); }
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
    CHECK(h.parms[RATIO_PARM] == 20 && h.floats[SPIKE_AVG_PARM] == 2.0f && h.parms[OFFX_PARM] == 0 && h.parms[OFFY_PARM] == 0, "parmsLoad fills Ratio bars, Spike vs average, Offset X / Y");
    h.floats[SPIKE_AVG_PARM] = 6.0f; h.floatSet[SPIKE_AVG_PARM] = true; h.parms[OFFX_PARM] = 7; APPLY(p);
    const std::string line = readAll(flex + "\\BidAskStats.place-ES.txt");
    CHECK(line == "20,0,60,15,10,7,0,0\n", "Apply: 8-field file, jump / lookback / old Box shows carried, Spike vs average 6.0, Offset X 7");
    NOW += 1500; p->calc(0);
    CHECK(S->ser.spike.avgT == 60 && S->dx == 7 && !S->ser.bars[T0 + 1800].bidSpike, "the chart reads it: 5.0x < 6.0x is no longer a spike (a settings change re-decides)");
    h.draws.clear(); p->draw();
    CHECK(count(h, "box", C_BID_SPIKE) == 0 && !text(h, "5.0"), "under 6.0x nothing is bright or labelled");
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
    {   const std::vector<mock::Draw> v = barsAt(h, 52);
        int gd = 0, rd = 0, bright = 0; for (auto& d : v) { gd += d.color == off::C_GREEN_BAR; rd += d.color == off::C_RED_BAR; bright += d.color == off::C_GREEN_SPIKE || d.color == off::C_RED_SPIKE; }
        CHECK(v.size() == 2 && gd == 1 && rd == 1 && bright == 0, "forming candle: bid bar dim green, offer bar dim red, nothing bright");
        CHECK(!text(h, "5.0"), "forming candle: no label (no repaint)"); }
    step(h, p, 53);
    const long long k52 = T0 + 60 * 52;
    CHECK(S->ser.bars.count(k52) && S->ser.bars[k52].r.addC[0] == 100 && S->ser.bars[k52].r.pullC[1] == 100, "candle 52 decided: +100 bids added, 100 offer lots pulled");
    CHECK(S->ser.bars[k52].spike[0] && S->ser.bars[k52].spike[1], "the NET spike on both sides (|+100| and |-100| = 5.0x the |net| average 20)");
    CHECK(S->file.rows.count(k52) && S->file.rows[k52].spike[0] == 1 && S->file.rows[k52].spike[1] == 1, "the BookFlow record keeps the net spike (spike_left / spike_right)");
    CHECK(S->file.rows.count(k51) && S->file.rows[k51].spike[0] == 0, "a normal candle's record: no spike");
    h.draws.clear(); p->draw();
    {   const std::vector<mock::Draw> v = barsAt(h, 52);
        const int y0 = 300 / 2; (void)y0;
        const mock::Draw* g = nullptr; const mock::Draw* r = nullptr;
        for (auto& d : v) { if (d.color == off::C_GREEN_SPIKE) g = &d; if (d.color == off::C_RED_SPIKE) r = &d; }
        CHECK(g && r, "closed: bid bar bright GREEN, offer bar bright RED (colour by side)");
        CHECK(g && r && g->x < r->x, "bid bar left, offer bar right");
        CHECK(g && r && g->b < r->y, "bids arriving above the zero line, offers leaving below it (red, not the old meaning-green)");
        int labels = 0; for (auto& d : h.draws) if (d.type == "text" && d.text == "5.0" && d.color == off::C_LBL_RATIO) ++labels;
        CHECK(labels == 2, "both net spikes labelled with the net ratio 5.0 (yellow, no x)");
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
    // settings: the 7 rows, Apply keeps the file layout, the chart re-reads
    LOAD(p);
    CHECK(h.parms[BF_RATIO] == 20 && h.floats[BF_AVG] == 2.0f && h.parms[BF_NEAR] == 0 && h.boxes[BF_RECORD] == 0 && h.parms[BF_OFFX] == 0 && h.parms[BF_OFFY] == 0, "parmsLoad fills the 7 rows from the file");
    h.floats[BF_AVG] = 6.0f; h.floatSet[BF_AVG] = true; h.parms[BF_OFFY] = 5; APPLY(p);
    CHECK(readAll(flex + "\\BookFlow.place-ES.txt") == "20,0,60,15,10,0,0,0,5,0\n", "Apply: Spike vs average 6.0 and Offset Y 5 written, jump / lookback / Box shows carried");
    NOW += 1500; p->calc(0);
    CHECK(S->dlgSpike.avgT == 60 && S->set.dy == 5, "the chart reads the file");
    h.draws.clear(); p->draw();
    {   int bright = 0; for (auto& d : barsAt(h, 52)) bright += d.color == off::C_GREEN_SPIKE || d.color == off::C_RED_SPIKE;
        CHECK(bright == 0 && !text(h, "5.0"), "5.0x under a 6.0x setting: dim, no label (the setting works)"); }
    dispose(p);
    CHECK(!mst::read("BOOK", "ESZ6", 60, "test", NOW).ok, "removed: the Market State entry is cleared");
    std::printf("BookFlow host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
#endif
