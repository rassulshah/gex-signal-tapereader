// test_lmz_host.cpp -- lsLiquidityProfile 0.54.0 LMT ZONES (LmtZonesLogic 1.3.0: + the after-fill read RE-BID / ACCEPTED, HELD? logged only; 1.2.0: grey map lines + FILLING tags; 0.48.0 first) on the mock host (the whole plugin: order feed -> zone engine -> draw / columns /
// hover / status / record / settings / restart). CL 3-min, a real 40-lot offer resting 2.5 min on a gamma level.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_lmz/test_lmz_host.cpp -o tlmzh && ASAN_OPTIONS=detect_leaks=0 ./tlmzh
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int count(mock::Host& h, const char* type, COLOR c, int pen = -1) { int n = 0; for (auto& d : h.draws) if (d.type == type && (c == 0xFFFFFFFF || d.color == c || d.border == c || d.pen == c) && (pen < 0 || d.pen == (COLOR)pen)) ++n; return n; }
static bool hasText(mock::Host& h, const std::string& t, COLOR c = 0xFFFFFFFF) { for (auto& d : h.draws) if (d.type == "text" && d.text == t && (c == 0xFFFFFFFF || d.color == c)) return true; return false; }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/lmzhXXXXXX"; const char* tmp = mkdtemp(tmpl);
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
    // the feed: a real 40-lot offer ON the gamma level 80.10 (id 7001), a 5-lot offer (never a zone), a 30-lot bid at 79.95 (log only in CL)
    mock::feed.push(0, 0, 80.10f, 40, (RTDATE)(t0 - 150), 7001);
    mock::feed.push(0, 0, 80.08f, 5, (RTDATE)(t0 - 150), 7002);
    mock::feed.push(0, 1, 79.95f, 30, (RTDATE)(t0 - 150), 7003);
    for (int k = 0; k < 21; ++k) mock::feed.push(0, 1, (float)(79.97 - 0.01 * (k % 6)), 3, (RTDATE)(t0 - 150), 7100 + k);   // (0.51.0 / LmtZones 1.1.0) the book: 3-lot bids -> median 3 -> LMT >= 9
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 6; ++s) { h.now = t0 - 100 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    CHECK(S->lz.book.size() == 24, "the zone engine keeps its own book from the chart's order feed");
    CHECK(S->lz.zones.empty(), "no zone before a bar closes");
    { mock::Bar b; b.time = t0 + 180; b.open = b.high = b.low = b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 1, 1, 2)); h.bars.push_back(b); }   // bar 30 (end t0) closes
    for (int s = 0; s < 4; ++s) { h.now = t0 + 5 + s; p->calc(0); usleep(5000); }
    std::printf("  zones %zu, S20 %lld (%s), gamma %zu, rule %s\n", S->lz.zones.size(), S->lzS20, S->lzS20Src.c_str(), S->lzGamma.size(), S->lz.R.src.c_str());
    int offer = -1, bid = -1; for (size_t i = 0; i < S->lz.zones.size(); ++i) { if (S->lz.zones[i].oid == 7001) offer = (int)i; if (S->lz.zones[i].oid == 7003) bid = (int)i; }
    CHECK(offer >= 0 && S->lz.zones[(size_t)offer].kind == lmz::K_NEAR && S->lz.zones[(size_t)offer].conf && S->lz.zones[(size_t)offer].drawn, "CL: the 40-lot offer on the gamma level is a drawn zone with the diamond");
    CHECK(bid >= 0 && !S->lz.zones[(size_t)bid].drawn, "the bid is tracked (LMT columns) but not drawn");
    bool small = false; for (auto& z : S->lz.zones) if (z.oid == 7002) small = true;
    CHECK(!small, "the 5-lot offer is under 3x the median (9 lots): no zone");
    CHECK(S->lz.medLots == 3 && S->lz.threshold() == 9, "(0.51.0) LMT >= 3x the live median resting order (3) = 9 lots, not the S20");
    h.draws.clear(); p->draw();
    const COLOR RED = 0xEF4444, GAM = 0x6B7280, GREY = (COLOR)lmz::C_GREYZ;
    const int yOrd = (int)(h.paneBottom - (80.10f - h.priceBase) * h.pxPer);
    // (0.53.0) the zone = a thin grey 1 px map line at the order's price; no filled red box any more
    auto zoneLines = [&](COLOR c) { int n = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == c && d.penW == 1 && std::abs(d.y - yOrd) <= 1) ++n; return n; };
    CHECK(zoneLines(GREY) >= 1, "(0.53.0) the zone is a thin grey 1 px line at the order's price");
    CHECK(count(h, "box", RED) == 0, "(0.53.0) no side-colour zone box / fill");
    const mock::Draw* lab = nullptr; for (auto& d : h.draws) if (d.type == "text" && d.text == "LMT 40 (NEAR)") lab = &d;
    int lineX0 = 1 << 30; for (auto& d : h.draws) if (d.type == "line" && d.pen == GREY && std::abs(d.y - yOrd) <= 1) lineX0 = std::min(lineX0, d.x);
    CHECK(lab && lab->color == GREY, "(0.53.0) label 'LMT 40 (NEAR)' in the zone grey");
    std::printf("  label l %d r %d t %d b %d, line from x %d at y %d\n", lab ? lab->l : -1, lab ? lab->r : -1, lab ? lab->t : -1, lab ? lab->b : -1, lineX0, yOrd);
    CHECK(lab && (lab->r <= lineX0 || lab->b <= yOrd), "(0.53.0) the label sits LEFT of the line start (or above it when there is no room)");
    int dia = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == (COLOR)lmz::C_DIAMOND) ++dia;
    CHECK(dia >= 5, "the yellow diamond (a real order on a gamma level)");
    int gl = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GAM) ++gl;
    CHECK(gl >= 2, "the gamma levels in view as thin grey lines (no purple)");
    bool purple = false; for (auto& d : h.draws) if ((d.type == "line" && (d.pen == 0xAB47BC || d.pen == 0xC084FC || d.pen == 0xA78BFA)) || (d.type == "text" && d.text.find("MQ") != std::string::npos)) purple = true;
    CHECK(!purple, "no purple gamma line and no 'MQ' label");
    CHECK(hasText(h, "LMT") && hasText(h, "LMT F"), "the BIG column is LMT | LMT F");
    bool lmt40 = false, lmt30 = false; for (auto& d : h.draws) if (d.type == "text") { if (d.text == "40" && d.color == (COLOR)lqp::colInk(false, false)) lmt40 = true; if (d.text == "30" && d.color == (COLOR)lqp::colInk(true, true)) lmt30 = true; }
    CHECK(lmt40, "LMT: 40 in the offer ink (drawn zone = bold / bright)");
    CHECK(lmt30, "LMT: the bid's 30 in the dim bid ink (log only, bids never drawn)");
    bool bidLine = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find("LMT 30") == 0) bidLine = true;
    CHECK(!bidLine, "the 30-lot bid zone is not drawn on the chart");
    // the hover: mouse over the zone -> question / what / why / odds / how / example / now
    { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.bar = 31; e.v.mouse.price = 80.10f;
      CHECK(p->mouse(&e) == RTX_FAIL, "mouse() never swallows the event");
      CHECK(S->lzHov == offer, "the mouse on the zone picks it");
      h.draws.clear(); p->draw();
      bool title = false, odds = false, magnet = false, xm = false, now = false;
      for (auto& d : h.draws) if (d.type == "text") { const std::string& t = d.text;
          if (t.find("LMT 40 (NEAR): sell order @ 80.10") == 0) title = true;
          if (t.find("reverses here 30% vs 35% at a random price") != std::string::npos) odds = true;
          if (t.find("TARGET / magnet, not a wall") != std::string::npos) magnet = true;
          if (t.find("13x the median resting order (3 lots)") != std::string::npos) xm = true;
          if (t.find("Now: RESTING - resting 40 lots | traded 0 (0% of 40 shown)") == 0) now = true; }
      CHECK(title && magnet, "hover title: LMT 40 (NEAR) - a target / magnet, not a wall");
      CHECK(odds && xm && now, "hover: the honest odds, x median, traded so far, status RESTING");
      e.v.mouse.price = 79.60f; p->mouse(&e); CHECK(S->lzHov < 0, "the mouse away: no hover"); }
    // the record + status (the record is flushed at most once a second)
    usleep(1100000); h.now = t0 + 20; p->calc(0);
    const std::string recPath = flex + "\\LiquidityProfile\\CL-CLEX26-180-lmtzones-" + std::to_string(S->lzSession) + ".csv";
    const std::string rec = readAll(recPath);
    CHECK(rec.find("\nZ|") != std::string::npos && rec.find("|7001|") != std::string::npos, "the zone is recorded (Z line)");
    const std::string st = readAll(flex + "\\LiquidityProfile.status-CL-180.txt");
    CHECK(st.find(std::string("LMT_ZONES,") + lmz::VERSION + ",on") != std::string::npos && st.find("LMT rule >= 3x the median resting order") != std::string::npos && st.find("= 9 lots") != std::string::npos, "status: LMT_ZONES line with the live-median rule");
    CHECK(st.find("pooled: print / abc / origin / near offers as grey map lines") != std::string::npos && st.find(",tags filling 0 held (log only) 0") != std::string::npos && st.find(",after-fill on re-bid 0 accepted 0 watching 0") != std::string::npos, "(0.54.0) status: the pooled rule + the tag counts + the after-fill read");
    CHECK(st.compare(0, 8 + std::strlen(LQ_VERSION), std::string("VERSION,") + LQ_VERSION) == 0 && std::string(LQ_VERSION) == "0.54.0" && std::string(lpset::PLUGIN_VERSION) == LQ_VERSION && std::string(lqp::VERSION) == LQ_VERSION, "status VERSION = LQ_VERSION = PLUGIN_VERSION = lqp::VERSION = 0.54.0");
    // (0.53.0) HELD?: bar 31 (end t0 + 180) trades into the band (high 80.08 >= 80.10 - 4 ticks) and closes back at 80.02 with the order resting
    const lmz::Zone& Z0 = S->lz.zones[(size_t)offer];
    std::printf("  zone atr %.1f ticks, band w %lld ticks\n", Z0.atr, (long long)Z0.w(S->lz.R.tagAtr));
    h.bars[31].high = 80.08f; h.bars[31].low = 80.00f; h.bars[31].close = 80.02f;
    { mock::Bar b; b.time = t0 + 360; b.open = b.high = b.low = b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 1, 1, 2)); h.bars.push_back(b); }   // bar 32 forming -> bar 31 closed
    for (int s = 0; s < 3; ++s) { h.now = t0 + 200 + s; p->calc(0); usleep(5000); }
    std::printf("  tags: held %lld fill %lld, contact %lld\n", Z0.tagHeldMs, Z0.tagFillMs, Z0.contactBarMs);
    CHECK(S->lz.zones[(size_t)offer].tagHeldMs > 0 && S->lz.zones[(size_t)offer].tagFillMs == 0, "HELD decided on bar 31's close (the order still resting)");
    h.draws.clear(); p->draw();
    CHECK(!hasText(h, "HELD?") && !hasText(h, "HELD"), "(0.54.0) HELD is decided and recorded but never drawn");
    // FILLING: bar 32 (end t0 + 360) trades up to the order and lifts 10 lots of it
    h.bars[32].high = 80.10f; h.bars[32].low = 80.02f; h.bars[32].close = 80.05f;
    mock::feed.push(3, 1, 80.10f, 10, (RTDATE)(t0 + 300), 7001, 9001);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 300 + s; p->calc(0); usleep(5000); }
    { mock::Bar b; b.time = t0 + 540; b.open = b.high = b.low = b.close = 80.05f; b.rows.push_back(mock::Price(80.05f, 1, 1, 2)); h.bars.push_back(b); }   // bar 33 forming -> bar 32 closed
    for (int s = 0; s < 3; ++s) { h.now = t0 + 380 + s; p->calc(0); usleep(5000); }
    CHECK(S->lz.zones[(size_t)offer].tagFillMs > S->lz.zones[(size_t)offer].tagHeldMs && S->lz.zones[(size_t)offer].traded == 10, "FILLING on bar 32 (10 lots traded in the band); the HELD stays");
    h.draws.clear(); p->draw();
    const mock::Draw* fill = nullptr; for (auto& d : h.draws) if (d.type == "text" && d.text == "FILLING") fill = &d;
    CHECK(fill && fill->color == GREY, "'FILLING' in grey at the zone line");
    CHECK(!hasText(h, "HELD?"), "(0.54.0) still no HELD? on the chart");
    // the order is pulled -> grey X, the zone ends once
    mock::feed.push(2, 0, 80.10f, 30, (RTDATE)(t0 + 400), 7001);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 401 + s; p->calc(0); usleep(5000); }
    CHECK(S->lz.zones[(size_t)offer].end == lmz::PULLED, "pulled: the zone ends (void)");
    h.draws.clear(); p->draw();
    int x = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GREY && d.penW == 2) ++x;
    CHECK(x >= 2, "the grey X at the pull");
    // (0.54.0) THE AFTER-FILL READ on the 30-lot BID zone (no line in CL - bids never drawn - but its read is): fully filled at t0+402,
    // new bids stack 2 ticks below (+20), buyers lift an offer 12 s later (+3 for the bid defender) -> RE-BID (green) at t0+432
    const lmz::Zone& ZB = S->lz.zones[(size_t)bid];
    std::printf("  bid zone atr %.1f ticks, rest %lld ms\n", ZB.atr, ZB.restMs);
    mock::feed.push(3, 0, 79.95f, 30, (RTDATE)(t0 + 402), 7003, 9002);
    mock::feed.push(2, 1, 79.95f, 0, (RTDATE)(t0 + 402), 7003);
    mock::feed.push(0, 1, 79.93f, 20, (RTDATE)(t0 + 404), 7301);
    mock::feed.push(3, 1, 80.08f, 3, (RTDATE)(t0 + 414), 7002, 9003);
    mock::feed.push(0, 0, 80.20f, 1, (RTDATE)(t0 + 433), 7302);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 434 + s; p->calc(0); usleep(5000); }
    std::printf("  after-fill: read %d stack %lld delta %lld fill %lld dec %lld (skip %d)\n", S->lz.zones[(size_t)bid].afRead, S->lz.zones[(size_t)bid].afStack, S->lz.zones[(size_t)bid].afDelta, S->lz.zones[(size_t)bid].afFillMs, S->lz.zones[(size_t)bid].afDecMs, S->lz.zones[(size_t)bid].afSkip);
    CHECK(S->lz.zones[(size_t)bid].end == lmz::FILLED && S->lz.zones[(size_t)bid].afRead == 1 && S->lz.zones[(size_t)bid].afStack == 20 && S->lz.zones[(size_t)bid].afDelta == 3, "(0.54.0) bid zone filled: RE-BID 30 s later (stack 20, delta +3)");
    h.draws.clear(); p->draw();
    const COLOR GREEN = 0x22C55E; const int yBid = (int)(h.paneBottom - (79.95f - h.priceBase) * h.pxPer);
    const mock::Draw* rb = nullptr; for (auto& d : h.draws) if (d.type == "text" && d.text == "RE-BID") rb = &d;
    CHECK(rb && rb->color == GREEN && rb->t >= yBid - 1, "(0.54.0) 'RE-BID' in green BELOW the filled bid price (bid zones: the tag only, no line)");
    bool bidLine2 = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find("LMT 30") == 0) bidLine2 = true;
    CHECK(!bidLine2 && !hasText(h, "ACCEPTED"), "no bid line / label; no ACCEPTED");
    usleep(1100000); h.now = t0 + 420; p->calc(0);
    const std::string rec2 = readAll(recPath);
    CHECK(rec2.find("|PULLED|") != std::string::npos, "the end is recorded (E line)");
    CHECK(rec2.find("\nR|") != std::string::npos && rec2.find("|RE-BID|20|3|20|0") != std::string::npos, "(0.54.0) the read is recorded (R|id|fill|decide|RE-BID|stack 20|delta 3|adds 20|cancels 0)");
    CHECK(rec2.find("|HELD|0|40") != std::string::npos && rec2.find("|FILLING|10|30") != std::string::npos && rec2.find("\nB|") != std::string::npos, "the tags are recorded (T lines: HELD resting 40, FILLING traded 10) + the judged bar (B)");
    // a restart (a new plugin object, the feed empty, every bar fed again): the same zones and tags come back - no repaint
    const size_t nz = S->lz.zones.size(); const long long hMs = S->lz.zones[(size_t)offer].tagHeldMs, fMs = S->lz.zones[(size_t)offer].tagFillMs;
    auto snap = [&]() { std::vector<std::string> v; for (auto& d : h.draws) if ((d.type == "line" && (d.pen == GREY || d.pen == (COLOR)lmz::C_DIAMOND)) || (d.type == "text" && (d.text == "HELD?" || d.text == "FILLING" || d.text == "RE-BID" || d.text == "ACCEPTED" || d.text.find("LMT 40") == 0)))
        v.push_back(d.type + "|" + d.text + "|" + std::to_string(d.x) + "," + std::to_string(d.y) + "," + std::to_string(d.l) + "," + std::to_string(d.t) + "," + std::to_string(d.r) + "," + std::to_string(d.b) + "|" + std::to_string(d.color) + "|" + std::to_string(d.pen)); return v; };
    h.draws.clear(); p->draw(); const std::vector<std::string> before = snap();
    p->destroy(); p->mdPark_.clear(); delete p; h.data = nullptr;
    mock::feed.ev.clear(); mock::feed.at = 0;
    LiquidityProfile* q = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 4; ++s) { h.now = t0 + 460 + s; q->calc(0); usleep(5000); }
    LQState* S2 = static_cast<LQState*>(h.data);
    CHECK(S2 && S2->lz.zones.size() == nz && S2->lz.zones[(size_t)offer].end == lmz::PULLED, "restart: the zones are read back with their ends");
    CHECK(S2 && S2->lz.zones[(size_t)offer].tagHeldMs == hMs && S2->lz.zones[(size_t)offer].tagFillMs == fMs && S2->lz.tally.held == 1 && S2->lz.tally.filling == 1 && S2->lz.tally.rebid == 1 && S2->lz.zones[(size_t)bid].afRead == 1, "restart: the tags and the RE-BID are read back, none decided again on the re-fed bars");
    h.draws.clear(); q->draw(); const std::vector<std::string> after = snap();
    CHECK(!before.empty() && before == after, "restart: the line, label, diamond, X and tags are drawn identically (never repaint)");
    usleep(1100000); h.now = t0 + 470; q->calc(0);
    const std::string rec3 = readAll(recPath); int tLines = 0; for (size_t k = rec3.find("\nT|"); k != std::string::npos; k = rec3.find("\nT|", k + 1)) ++tLines;
    int rLines = 0; for (size_t k = rec3.find("\nR|"); k != std::string::npos; k = rec3.find("\nR|", k + 1)) ++rLines;
    CHECK(tLines == 2 && rLines == 1, "restart: still exactly 2 T lines and 1 R line in the record");
    // settings: the master off hides everything (zones, gamma lines, LMT column); the file is per market
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); lmz::Settings z; z.on = false; f << lmz::settingsLine(z) << "\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 480 + s; q->calc(0); usleep(1100000 / 3); }
    h.draws.clear(); q->draw();
    int gl2 = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GAM) ++gl2;
    // (0.54.0) the after-fill fixture (a 30-lot bid filled + an offer lifted) also lights the absorption banner's MKT / LMT / FP / DP chips:
    // a chip "LMT" is not the LMT column header - only a header-row "LMT" (the column's top line) counts here
    int lmtHead = 0; for (auto& d : h.draws) if (d.type == "text" && d.text == "LMT") { std::printf("    'LMT' text at t %d l %d\n", d.t, d.l); if (d.t <= h.paneTop + 4) ++lmtHead; }
    CHECK(!S2->lzSet.on && gl2 == 0 && lmtHead == 0 && zoneLines(GREY) == 0 && !hasText(h, "HELD?"), "LMT zones off: no zones, no tags, no gamma lines, LMT F alone again");
    // (0.54.0) the After-fill box off: RE-BID goes, the line / label / FILLING stay; colours: the RE-BID bid colour + the zone line colour applied
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); lmz::Settings z; z.tags = false; f << lmz::settingsLine(z) << "\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 490 + s; q->calc(0); usleep(1100000 / 3); }
    h.draws.clear(); q->draw();
    CHECK(!S2->lzSet.tags && zoneLines(GREY) >= 1 && !hasText(h, "RE-BID") && hasText(h, "FILLING") && hasText(h, "LMT 40 (NEAR)"), "(0.54.0) After-fill box off: no RE-BID; the line, label and FILLING stay");
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); lmz::Settings z; z.gammaLines = false; z.cBid = 0x112233; z.cGrey = 0x445566; f << lmz::settingsLine(z) << "\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 500 + s; q->calc(0); usleep(1100000 / 3); }
    h.draws.clear(); q->draw();
    int gl3 = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GAM) ++gl3;
    CHECK(S2->lzSet.on && gl3 == 0 && hasText(h, "RE-BID", 0x112233) && hasText(h, "FILLING", 0x445566) && zoneLines(0x445566) >= 1 && zoneLines(GREY) == 0, "gamma lines off; the RE-BID bid colour and the zone line colour are applied");
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); lmz::Settings z; z.nearO = false; f << lmz::settingsLine(z) << "\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 505 + s; q->calc(0); usleep(1100000 / 3); }
    h.draws.clear(); q->draw();
    CHECK(!hasText(h, "RE-BID") && !hasText(h, "LMT 40 (NEAR)"), "(0.54.0) the Near box off hides the near zone and the near bid's RE-BID");
    // an old 0.48-0.52 settings file (LMZ1, no tags field) still loads: tags on
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); f << "LMZ1,1,1,1,1,1,1,1,1,1122867,2278750,4478310,7041664\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 510 + s; q->calc(0); usleep(1100000 / 3); }
    CHECK(S2->lzWhy == "file" && S2->lzSet.tags && S2->lzSet.cOffer == 1122867, "an old LMZ1 settings file loads (tags default on)");
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); f << "LMZ2,1,1,1,1,1,1,1,1,1122867,2278750,4478310,7041664,0\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 515 + s; q->calc(0); usleep(1100000 / 3); }
    CHECK(S2->lzWhy == "file" && S2->lzSet.tags, "(0.54.0) a 0.53 LMZ2 file with State tags off loads with the After-fill tags ON (default)");
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); f << "LMZ2,garbage\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 520 + s; q->calc(0); usleep(1100000 / 3); }
    CHECK(S2->lzWhy.find("garbled") != std::string::npos && S2->lzSet.cOffer == 1122867, "a garbled settings file keeps the last good one");
    // (0.54.0) the settings window table: the tags row renamed (key / position kept), parameter version 10, colour rows say what they colour
    { const ss::Spec& sp = lpset::profileSpec(); CHECK(sp.paramVersion == 10 && sp.n == lpset::K_N && std::string(sp.rows[lpset::K_LMZ_TAGS].key) == "lmz_tags" && lpset::K_LMZ_TAGS == lpset::K_N - 1
          && std::string(sp.rows[lpset::K_LMZ_TAGS].label) == "After-fill tags (RE-BID / ACCEPTED)" && sp.rows[lpset::K_LMZ_TAGS].def == 1
          && std::string(sp.rows[lpset::K_LMZ_C_OFFER].label) == "RE-BID offer colour" && std::string(sp.rows[lpset::K_LMZ_C_BID].label) == "RE-BID bid colour"
          && std::string(sp.rows[lpset::K_LMZ_C_OFFER].key) == "lmz_c_offer" && std::string(sp.rows[lpset::K_LMZ_C_GREY].label) == "Zone line colour", "settings table: After-fill tags row (default on) last, parameter version 10, keys unchanged"); }
    q->destroy(); q->mdPark_.clear(); delete q;
    std::printf("lsLiquidityProfile %s LMT zones host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
