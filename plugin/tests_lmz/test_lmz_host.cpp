// test_lmz_host.cpp -- lsLiquidityProfile 0.48.0 LMT ZONES on the mock host (the whole plugin: order feed -> zone engine -> draw / columns /
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
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 6; ++s) { h.now = t0 - 100 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    CHECK(S->lz.book.size() == 3, "the zone engine keeps its own book from the chart's order feed");
    CHECK(S->lz.zones.empty(), "no zone before a bar closes");
    { mock::Bar b; b.time = t0 + 180; b.open = b.high = b.low = b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 1, 1, 2)); h.bars.push_back(b); }   // bar 30 (end t0) closes
    for (int s = 0; s < 4; ++s) { h.now = t0 + 5 + s; p->calc(0); usleep(5000); }
    std::printf("  zones %zu, S20 %lld (%s), gamma %zu, rule %s\n", S->lz.zones.size(), S->lzS20, S->lzS20Src.c_str(), S->lzGamma.size(), S->lz.R.src.c_str());
    int offer = -1, bid = -1; for (size_t i = 0; i < S->lz.zones.size(); ++i) { if (S->lz.zones[i].oid == 7001) offer = (int)i; if (S->lz.zones[i].oid == 7003) bid = (int)i; }
    CHECK(offer >= 0 && S->lz.zones[(size_t)offer].kind == lmz::K_NEAR && S->lz.zones[(size_t)offer].conf && S->lz.zones[(size_t)offer].drawn, "CL: the 40-lot offer on the gamma level is a drawn zone with the diamond");
    CHECK(bid >= 0 && !S->lz.zones[(size_t)bid].drawn, "the bid is tracked (LMT columns) but not drawn");
    bool small = false; for (auto& z : S->lz.zones) if (z.oid == 7002) small = true;
    CHECK(!small, "the 5-lot offer is under S20: no zone");
    h.draws.clear(); p->draw();
    const COLOR RED = 0xEF4444, GAM = 0x6B7280;
    const int yOrd = (int)(h.paneBottom - (80.10f - h.priceBase) * h.pxPer);
    int zoneBox = 0; for (auto& d : h.draws) if (d.type == "box" && d.border == RED && d.t <= yOrd && d.b >= yOrd) ++zoneBox;
    CHECK(zoneBox >= 1, "the zone box at the order's price, red frame");
    CHECK(hasText(h, "Offer 40", RED), "label 'Offer 40' in red (short, no grey sub-text)");
    int dia = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == (COLOR)lmz::C_DIAMOND) ++dia;
    CHECK(dia >= 5, "the yellow diamond (a real order on a gamma level)");
    int gl = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GAM) ++gl;
    CHECK(gl >= 2, "the gamma levels in view as thin grey lines (no purple)");
    bool purple = false; for (auto& d : h.draws) if ((d.type == "line" && (d.pen == 0xAB47BC || d.pen == 0xC084FC || d.pen == 0xA78BFA)) || (d.type == "text" && d.text.find("MQ") != std::string::npos)) purple = true;
    CHECK(!purple, "no purple gamma line and no 'MQ' label");
    CHECK(hasText(h, "LMT") && hasText(h, "LMT F"), "the BIG column is LMT | LMT F");
    bool lmt40 = false, lmt30 = false; for (auto& d : h.draws) if (d.type == "text") { if (d.text == "40" && d.color == (COLOR)lqp::colInk(false, false)) lmt40 = true; if (d.text == "30" && d.color == (COLOR)lqp::colInk(true, true)) lmt30 = true; }
    CHECK(lmt40, "LMT: 40 in the offer ink (drawn zone = bold / bright)");
    CHECK(lmt30, "LMT: the bid's 30 in the dim bid ink (log only)");
    // the hover: mouse over the zone -> the box with the question / how / study rows
    { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.bar = 31; e.v.mouse.price = 80.10f;
      CHECK(p->mouse(&e) == RTX_FAIL, "mouse() never swallows the event");
      CHECK(S->lzHov == offer, "the mouse on the zone picks it");
      h.draws.clear(); p->draw();
      CHECK(hasText(h, "Real offer: 40 lots @ 80.10", RED) || [&]() { for (auto& d : h.draws) if (d.type == "text" && d.text.find("Real offer: 40 lots @ 80.10") == 0) return true; return false; }(), "hover title");
      bool how = false, study = false; for (auto& d : h.draws) if (d.type == "text") { if (d.text.find("close back") != std::string::npos) how = true; if (d.text.find("+32.2") != std::string::npos) study = true; }
      CHECK(how && study, "hover: how to use (close back, not the touch) + the CL study numbers");
      e.v.mouse.price = 79.60f; p->mouse(&e); CHECK(S->lzHov < 0, "the mouse away: no hover"); }
    // the record + status (the record is flushed at most once a second)
    usleep(1100000); h.now = t0 + 20; p->calc(0);
    const std::string rec = readAll(flex + "\\LiquidityProfile\\CL-CLEX26-180-lmtzones-" + std::to_string(S->lzSession) + ".csv");
    CHECK(rec.find("\nZ|") != std::string::npos && rec.find("|7001|") != std::string::npos, "the zone is recorded (Z line)");
    const std::string st = readAll(flex + "\\LiquidityProfile.status-CL-180.txt");
    CHECK(st.find("LMT_ZONES,1.0.0,on") != std::string::npos, "status: LMT_ZONES line");
    CHECK(st.compare(0, 14, "VERSION,0.48.0") == 0, "status VERSION 0.48.0");
    // the order is pulled -> X, the zone ends once
    mock::feed.push(2, 0, 80.10f, 40, (RTDATE)(t0 + 30), 7001);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 31 + s; p->calc(0); usleep(5000); }
    CHECK(S->lz.zones[(size_t)offer].end == lmz::PULLED, "pulled: the zone ends (void)");
    h.draws.clear(); p->draw();
    int x = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == RED) ++x;
    CHECK(x >= 2, "the X at the pull");
    usleep(1100000); h.now = t0 + 40; p->calc(0);
    const std::string rec2 = readAll(flex + "\\LiquidityProfile\\CL-CLEX26-180-lmtzones-" + std::to_string(S->lzSession) + ".csv");
    CHECK(rec2.find("|PULLED|") != std::string::npos, "the end is recorded (E line)");
    // a restart (a new plugin object, the feed empty): the same zones come back from the record - no repaint
    const size_t nz = S->lz.zones.size();
    h.draws.clear(); p->draw(); std::vector<std::string> before; for (auto& d : h.draws) if (d.type == "box" && d.border == RED) before.push_back(std::to_string(d.l) + "," + std::to_string(d.t) + "," + std::to_string(d.r) + "," + std::to_string(d.b));
    p->destroy(); p->mdPark_.clear(); delete p; h.data = nullptr;
    mock::feed.ev.clear(); mock::feed.at = 0;
    LiquidityProfile* q = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 4; ++s) { h.now = t0 + 60 + s; q->calc(0); usleep(5000); }
    LQState* S2 = static_cast<LQState*>(h.data);
    CHECK(S2 && S2->lz.zones.size() == nz && S2->lz.zones[(size_t)offer].end == lmz::PULLED, "restart: the zones are read back with their ends");
    h.draws.clear(); q->draw(); std::vector<std::string> after; for (auto& d : h.draws) if (d.type == "box" && d.border == RED) after.push_back(std::to_string(d.l) + "," + std::to_string(d.t) + "," + std::to_string(d.r) + "," + std::to_string(d.b));
    CHECK(before == after, "restart: the zone is drawn identically (never repaints)");
    // settings: the master off hides everything (zones, gamma lines, LMT column); the file is per market
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); lmz::Settings z; z.on = false; f << lmz::settingsLine(z) << "\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 70 + s; q->calc(0); usleep(1100000 / 3); }
    h.draws.clear(); q->draw();
    int gl2 = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GAM) ++gl2;
    CHECK(!S2->lzSet.on && gl2 == 0 && !hasText(h, "LMT") && count(h, "box", RED) == 0, "LMT zones off: no zones, no gamma lines, LMT F alone again");
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); lmz::Settings z; z.gammaLines = false; z.cOffer = 0x112233; f << lmz::settingsLine(z) << "\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 80 + s; q->calc(0); usleep(1100000 / 3); }
    h.draws.clear(); q->draw();
    int gl3 = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GAM) ++gl3;
    CHECK(S2->lzSet.on && gl3 == 0 && count(h, "box", 0x112233) >= 1, "gamma lines checkbox off; the offer colour setting is applied");
    { std::ofstream f((flex + "\\LiquidityProfile.lmtzones-CL.txt").c_str()); f << "LMZ1,garbage\n"; }
    S2->lzCheckMs = -1; usleep(20000);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 90 + s; q->calc(0); usleep(1100000 / 3); }
    CHECK(S2->lzWhy.find("garbled") != std::string::npos && S2->lzSet.cOffer == 0x112233, "a garbled settings file keeps the last good one");
    q->destroy(); q->mdPark_.clear(); delete q;
    std::printf("lsLiquidityProfile 0.48.0 LMT zones host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
