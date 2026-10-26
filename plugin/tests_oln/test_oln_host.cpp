// test_oln_host.cpp -- lsLiquidityProfile 0.55.0 THE LIMIT-ORDER LINES on the mock host (order feed -> OrderLines engine -> draw / hover /
// status / record / settings / restart). ES 3-min; a 100-lot bid rolled 7,675 -> 7,680, filled, green close above = HELD; a 50-lot offer
// pulled = very dark line; no LMT column; no RE-BID? / ACCEPTED / X.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_oln/test_oln_host.cpp -o tolnh && ASAN_OPTIONS=detect_leaks=0 ./tolnh
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static bool hasText(mock::Host& h, const std::string& t, COLOR c = 0xFFFFFFFF) { for (auto& d : h.draws) if (d.type == "text" && d.text == t && (c == 0xFFFFFFFF || d.color == c)) return true; return false; }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static void bar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; b.rows.push_back(mock::Price(c, 50, 40, 90)); h.bars.push_back(b); }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/olnhXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 22) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26";
    h.priceBase = 7660.0f; h.pxPer = 20.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 12 * 3600 + 30 * 60;   // 12:30 (the mock clock is CT)
    for (int i = 0; i < 31; ++i) bar(h, t0 - (30 - i) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(7689.75 - i * 0.25); L.ask = (float)(7690.0 + i * 0.25); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    // 12:30: a 100-lot bid at 7,675 (id 1) and a 50-lot offer at 7,701 (id 2)
    mock::feed.push(0, 1, 7675.0f, 100, (RTDATE)(t0 - 60), 1);
    mock::feed.push(0, 0, 7701.0f, 50, (RTDATE)(t0 - 60), 2);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 5; ++s) { h.now = t0 - 50 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    std::printf("  market %s floor %lld rule %s events %lld min %d late %lld lines %zu book %zu lz book %zu bo %d\n", S->market.c_str(), S->ol.R.floorLots, S->ol.R.src.c_str(), S->ol.tally.events, oln::Engine::minuteCT(S->ol.lastT), S->ol.tally.lateSkipped, S->ol.lines.size(), S->ol.book.size(), S->lz.book.size(), S->boSet.on ? 1 : 0);
    CHECK(S->market == "ES" && S->ol.R.floorLots == 50, "ES: floor 50 lots (defaults; no params file)");
    CHECK(S->ol.lines.size() == 2 && !S->ol.lines[0].drawn, "two tracked lines, none drawn before 9 min");
    // 10 minutes later (bars 31.. close), both drawn
    for (int i = 31; i < 35; ++i) bar(h, t0 + (i - 30) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    mock::feed.push(1, 1, 7675.0f, 100, (RTDATE)(t0 + 600), 1);
    for (int s = 0; s < 4; ++s) { h.now = t0 + 601 + s; p->calc(0); usleep(5000); }
    CHECK(S->ol.tally.drawn == 2, "both drawn after resting 9 min");
    h.draws.clear(); p->draw();
    const COLOR GREEN = 0x22C55E, RED = 0xEF4444;
    const int yBid = (int)(h.paneBottom - (7675.0f - h.priceBase) * h.pxPer), yOff = (int)(h.paneBottom - (7701.0f - h.priceBase) * h.pxPer);
    int nb = 0, no = 0; for (auto& d : h.draws) if (d.type == "line") { if (d.pen == GREEN && std::abs(d.y - yBid) <= 1) ++nb; if (d.pen == RED && std::abs(d.y - yOff) <= 1) ++no; }
    CHECK(nb >= 1 && no >= 1, "the bid line (green) and the offer line (red) at their prices");
    const mock::Draw* l100 = text(h, "100"); const mock::Draw* l50 = text(h, "50");
    CHECK(l100 && l100->color == GREEN && l50 && l50->color == RED, "labels = size only, in the side colour");
    CHECK(!hasText(h, "LMT") && !hasText(h, "RE-BID?") && !hasText(h, "ACCEPTED") && !hasText(h, "FILLING"), "no LMT column header, no RE-BID? / ACCEPTED / FILLING");
    CHECK(hasText(h, "LMT F"), "LMT F stays");
    // the roll: a new 100-lot bid at 7,680 (id 3), the old one cancelled 0.5 s later; 20 s later the roll is decided
    mock::feed.push(0, 1, 7680.0f, 100, (RTDATE)(t0 + 640), 3);
    mock::feed.push(2, 1, 7675.0f, 0, (RTDATE)(t0 + 640), 1);
    mock::feed.push(0, 1, 7600.0f, 7, (RTDATE)(t0 + 660), 99);
    // the offer pulled (no re-placement): dark after the 15 s window
    mock::feed.push(2, 0, 7701.0f, 0, (RTDATE)(t0 + 641), 2);
    mock::feed.push(0, 1, 7600.0f, 7, (RTDATE)(t0 + 670), 98);
    for (int s = 0; s < 4; ++s) { h.now = t0 + 671 + s; p->calc(0); usleep(5000); }
    const oln::Line* B = nullptr; const oln::Line* A = nullptr; for (auto& L : S->ol.lines) if (S->ol.visible(L)) { if (L.side == oln::BID) B = &L; else A = &L; }
    CHECK(B && B->segs.size() == 2 && B->rolls == 1, "rolled 7,675 -> 7,680: one stepping line");
    CHECK(A && A->end == oln::PULLED, "the offer ends PULLED (no candidate within 15 s)");
    h.draws.clear(); p->draw();
    const int yNew = (int)(h.paneBottom - (7680.0f - h.priceBase) * h.pxPer);
    bool oldSeg = false, darkOff = false, newSeg = false, xMark = false;   // the mock records each line's END point
    for (auto& d : h.draws) if (d.type == "line") {
        if (d.pen == GREEN && std::abs(d.y - yBid) <= 1) oldSeg = true;
        if (d.pen == GREEN && std::abs(d.y - yNew) <= 1) newSeg = true;
        if (std::abs(d.y - yOff) <= 1 && d.pen == (COLOR)0x471414) darkOff = true;
        if (d.penW == 2 && d.pen == (COLOR)0x8A8F98) xMark = true; }
    CHECK(oldSeg && newSeg, "the line steps: a segment at 7,675 and one at 7,680");
    CHECK(darkOff, "the pulled offer line is very dark red (0x471414)");
    CHECK(!xMark, "no grey X");
    // filled at 7,680, the next bar closes green above it -> HELD beside the fill dot
    mock::feed.push(3, 0, 7680.0f, 100, (RTDATE)(t0 + 700), 3, 777);
    mock::feed.push(2, 1, 7680.0f, 0, (RTDATE)(t0 + 700), 3);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 701 + s; p->calc(0); usleep(5000); }
    bar(h, t0 + 6 * 180, 7682.0f, 7684.0f, 7679.0f, 7681.0f);   // bar 35 (ends t0+720 .. contains the fill) red
    bar(h, t0 + 7 * 180, 7681.0f, 7688.0f, 7680.5f, 7687.0f);   // bar 36 green close above
    bar(h, t0 + 8 * 180, 7687.0f, 7688.0f, 7686.0f, 7687.0f);   // bar 37 forming
    for (int s = 0; s < 3; ++s) { h.now = t0 + 8 * 180 + 5 + s; p->calc(0); usleep(5000); }
    for (auto& L : S->ol.lines) if (S->ol.visible(L) && L.side == oln::BID) B = &L;
    std::printf("  bid end %d word %d shown %d fill %lld\n", B->end, B->word, B->wordShown ? 1 : 0, B->fillMs);
    CHECK(B->end == oln::FILLED && B->word == oln::W_HELD && B->wordShown, "filled, then a green close above = HELD");
    h.draws.clear(); p->draw();
    const mock::Draw* held = text(h, "HELD");
    CHECK(held && held->color == GREEN && std::abs((held->t + held->b) / 2 - yNew) <= 12, "HELD in green beside the fill dot at 7,680");
    int dot = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GREEN && std::abs(d.y - yNew) <= 5 && d.y != yNew) ++dot;
    CHECK(dot >= 5, "the fill dot");
    // hover on the bid line
    { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.bar = 34; e.v.mouse.price = 7680.0f;
      p->mouse(&e); CHECK(S->olHov >= 0, "the mouse on the line picks it");
      h.draws.clear(); p->draw();
      bool q = false, r = false, x = false, w = false; for (auto& d : h.draws) if (d.type == "text") { if (d.text.rfind("Question:", 0) == 0) q = true; if (d.text.find("Rolled: 7675.00 -> 7680.00") == 0) r = true; if (d.text.rfind("Example:", 0) == 0) x = true; if (d.text.rfind("HELD:", 0) == 0) w = true; }
      CHECK(q && r && x && w, "hover: question / rolled path / HELD reason / example");
      e.v.mouse.price = 7620.0f; p->mouse(&e); CHECK(S->olHov < 0, "the mouse away: no hover"); }
    // record + status + restart
    usleep(1100000); h.now = t0 + 8 * 180 + 30; p->calc(0);
    const std::string recPath = flex + "\\LiquidityProfile\\ES-EPZ26-180-orderlines-" + std::to_string(S->lzSession) + ".csv";
    const std::string rec = readAll(recPath);
    CHECK(rec.find("\nN|") != std::string::npos && rec.find("\nS|") != std::string::npos && rec.find("\nE|") != std::string::npos && rec.find("|HELD|") != std::string::npos, "record: N / S / E / W lines");
    const std::string st = readAll(flex + "\\LiquidityProfile.status-ES-180.txt");
    CHECK(st.find(std::string("ORDER_LINES,") + oln::VERSION + ",on") != std::string::npos && st.find("floor 50 lots rest 9 min roll 15 s") != std::string::npos && st.find(",held 1 ") != std::string::npos, "status: ORDER_LINES line");
    CHECK(std::string(LQ_VERSION) == "0.55.0" && std::string(lpset::PLUGIN_VERSION) == LQ_VERSION && std::string(lqp::VERSION) == LQ_VERSION, "versions = 0.55.0");
    // the settings rows: relabelled, parameter version 11
    { const ss::Spec& sp = lpset::profileSpec(); bool m = false, ice = false, lad = false, pul = false, q = false, wd = false;
      for (int k = 0; k < sp.n; ++k) { const std::string lb = sp.rows[k].label; if (lb == "Limit-order lines (resting 9+ min)") m = true; if (lb == "Icebergs (I R15 500)") ice = true; if (lb == "Ladders (4o 87)") lad = true; if (lb == "Pulled lines (very dark)") pul = true; if (lb == "Queue Q at levels") q = true; if (lb == "HELD / THRU words") wd = true; }
      CHECK(m && ice && lad && pul && q && wd && sp.paramVersion == 11, "settings rows relabelled, parameter version 11"); }
    // restart: a new plugin instance reads the record back - the same line, step, end and word
    {   p->destroy(); p->mdPark_.clear(); delete p; h.data = nullptr;
        LiquidityProfile* p2 = static_cast<LiquidityProfile*>(CreateExtension());
        for (int s = 0; s < 4; ++s) { h.now = t0 + 8 * 180 + 40 + s; p2->calc(0); usleep(5000); }
        LQState* S2 = static_cast<LQState*>(h.data);
        const oln::Line* R = nullptr; for (auto& L : S2->ol.lines) if (S2->ol.visible(L) && L.side == oln::BID) R = &L;
        CHECK(R && R->segs.size() == 2 && R->end == oln::FILLED && R->word == oln::W_HELD && R->wordShown, "after a restart the line, its step, end and HELD come back from the record");
        h.draws.clear(); p2->draw(); CHECK(hasText(h, "HELD", GREEN), "HELD drawn again after the restart (never repainted)");
        p2->destroy(); p2->mdPark_.clear(); delete p2; }
    std::printf("lsLiquidityProfile %s ORDER LINES host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
