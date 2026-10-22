// test_oh_host.cpp -- lsDealerHedge 0.1.1 on the mock host: MenthorQLevels.csv + the chart's order feed -> an oH zone that lights,
// pulses (the 250 ms timer asks for redraws only while live), settles, is recorded, comes back identical after a restart, the
// settings remove / colour what is drawn, the hover answers, the status file has the status line. Two charts on one DLL.
// From plugin\:  g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Itests_oh/mock -I. tests_oh/test_oh_host.cpp -o /tmp/tohh && /tmp/tohh
#include "DealerHedge.cpp"
#include <cstdio>
#include <sstream>
#include <unistd.h>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int boxes(mock::Host& h, int mode) { int n = 0; for (auto& d : h.draws) if (d.type == "box" && d.fid == mode) ++n; return n; }
static const mock::Draw* firstBox(mock::Host& h, int mode) { for (auto& d : h.draws) if (d.type == "box" && d.fid == mode) return &d; return nullptr; }
static bool hasText(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return true; return false; }
static bool hasTextStart(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text.compare(0, t.size(), t) == 0) return true; return false; }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/ohhXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    { std::ofstream f((flex + "\\MenthorQLevels.csv").c_str()); f << "SYMBOL,PRICE,LABEL\nEPZ26,7840,CR 0D / GW 0D\nEPZ26,7800,HVL\nEPH27,7900,CR\n"; }
    { std::ofstream f((flex + "\\LRA-DealerHedge-ES.csv").c_str()); f << "VERSION,1\nMARKET,ES\nPARAM,SHOW,2\nPARAM,MIN_LOTS,10\nPARAM,REP_MIN_LOTS,3\n"; for (int s = 0; s < 1440; s += 30) f << "NORM," << s << ",5,5,5,5\n"; }
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 7) * 86400LL), t0 = day + 10 * 3600;
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ26"; h.root = "ES"; h.priceBase = 7820; h.pxPer = 20; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    for (int i = 0; i < 30; ++i) { mock::Bar b; b.time = t0 - (29 - i) * 180; b.open = 7842; b.high = 7844; b.low = 7840; b.close = 7842; h.bars.push_back(b); }   // ATR(14) = 4
    h.now = t0 - 170;
    DealerHedge* p = static_cast<DealerHedge*>(CreateExtension()); p->setup();
    p->calc(0);
    DHState* S = static_cast<DHState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    CHECK(S->market == "ES" && S->mqN == 3 && S->E.zones.size() == 2, "ES Dec levels from MenthorQLevels.csv (March CR ignored): CR 0DTE + GW 0DTE share one zone, HVL its own");
    CHECK(std::fabs(S->atr - 4.0) < 1e-9 && S->E.P.minLots == 10 && S->E.slotNorm.size() == 48, "ATR(14) from the chart, params + slot norms from the file");
    CHECK(S->timerOn && h.timers.size() == 1, "the 250 ms pulse timer is granted");
    h.draws.clear(); p->draw(); CHECK(boxes(h, 1) == 0 && !hasText(h, "oH"), "INVISIBLE until activity");
    // six 6-lot bids every 2 s at 7,839.50 (inside CR 0DTE 7,840 +/- 2)
    for (int i = 0; i < 6; ++i) mock::feed.push(0, 1, 7839.5f, 6, (RTDATE)(t0 - 160 + 2 * i), 1000 + i);
    h.now = t0 - 148; p->calc(0);
    const dh::Zone* z = nullptr; for (const dh::Zone& x : S->E.zones) if (x.name.find("CR 0DTE") == 0) z = &x;
    CHECK(z && z->state == dh::LIVE && z->side == dh::BUY && z->paths.find('a') != std::string::npos, "the feed lights CR 0DTE: BUY, same-size repeats");
    CHECK(z && z->barKey == (long long)h.bars.back().time, "decided on the forming candle");
    const std::string rec = S->recPath; const std::string recText = readAll(rec);
    CHECK(recText.find("\nZ|CR 0DTE@31360|CR 0DTE/GW 0DTE|31360|31352|31368|0|") != std::string::npos, "Z line appended to the session record");
    // draw: a translucent band, green blended toward the black background, "oH" below the candle
    h.draws.clear(); p->draw();
    const mock::Draw* b = firstBox(h, 1);
    CHECK(b && b->l <= h.barX0 + 29 * h.barW && b->r <= h.scaleLeft && b->t < b->b, "a translucent band from the decision candle to the last bar");
    CHECK(b && ((b->color >> 8) & 0xFF) > ((b->color >> 16) & 0xFF) && b->color != 0x0022C55E, "green, pulsing (blended toward the background)");
    CHECK(hasText(h, "oH"), "oH below the bar");
    { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = 0; e.v.timer.id = dhTimerId(S); const int before = h.invalidates; p->timer(&e);
      CHECK(h.invalidates == before + 1, "the timer asks for a redraw while LIVE");
      e.v.timer.id = 1; p->timer(&e); CHECK(h.invalidates == before + 1, "another chart's timer id is ignored"); }
    // 90 s later with no activity: settled, the timer stops asking
    h.now = t0 - 60; p->calc(0);
    CHECK(z->state == dh::SETTLED, "SETTLED after the activity stops");
    { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.v.timer.id = dhTimerId(S); const int before = h.invalidates; p->timer(&e); CHECK(h.invalidates == before, "no redraws asked while nothing is live"); }
    h.draws.clear(); p->draw(); const mock::Draw* fb = firstBox(h, 1);
    CHECK(fb && fb->color == (COLOR)dh::blend(0x0022C55E, 0, 0.30), "settled = the faint band (30%)");
    // hover over the band
    hb::fakeCursor() = 77; { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)(fb->l + 5); e.v.mouse.v = (short)((fb->t + fb->b) / 2); p->mouse(&e); }
    h.draws.clear(); p->draw();
    CHECK(hasText(h, "oH ? Dealer hedging BUYING at CR 0DTE/GW 0DTE 7,840.00") && hasTextStart(h, "Question:") && hasTextStart(h, "Dealers: LONG gamma") && hasTextStart(h, "How to use:"), "hover: header + question + dealers + how to use");
    hb::fakeCursor() = 0;
    // settings: hide zones / labels; colours
    { std::ofstream f((flex + "\\DealerHedge.settings-ES.txt").c_str()); f << "SPEC1|DealerHedge|0.1.0|ES\nshow_zones=0\nshow_labels=0\n"; }
    h.now = t0 - 50; p->calc(0); h.draws.clear(); p->draw(); CHECK(boxes(h, 1) == 0 && !hasText(h, "oH"), "Show unchecked: band and label gone (still recorded)");
    { std::ofstream f((flex + "\\DealerHedge.settings-ES.txt").c_str()); f << "SPEC1|DealerHedge|0.1.0|ES\nshow_zones=1\nshow_labels=1\nbuy_color=0000FF\nfaint_pct=60\n"; }
    usleep(1100000);                                                     // the settings file is re-read at most once a second
    h.now = t0 - 40; p->calc(0); h.draws.clear(); p->draw(); const mock::Draw* cb = firstBox(h, 1);
    CHECK(cb && cb->color == (COLOR)dh::blend(255, 0, 0.60), "the Buying colour and the band strength apply");
    // status file
    p->release();   // writes the status, flushes, releases the slot (= IRT removing / reloading the indicator)
    const std::string st = readAll(flex + "\\DealerHedge.status-ES-ESZ26-180.txt");
    CHECK(st.find("VERSION,0.2.0") != std::string::npos && st.find("STATUS_LINE,oH 0.2.0") != std::string::npos && st.find("SHOW,2 (drawn)") != std::string::npos && st.find("ZONE,CR 0DTE/GW 0DTE 7,840.00 BUY") != std::string::npos, "status file: version, status line, the zone");
    // restart: a new chart object -> the zone comes back from the record, settled, identical
    mock::feed.ev.clear(); mock::feed.at = 0;
    // (0.2.0) the nightly's verdict LOG ONLY: PARAM,SHOW,0 -> nothing drawn after the restart, the record still read / kept
    { std::ofstream f((flex + "\\LRA-DealerHedge-ES.csv").c_str()); f << "VERSION,1\nMARKET,ES\nPARAM,SHOW,0\nPARAM,MIN_LOTS,10\nPARAM,REP_MIN_LOTS,3\n"; for (int s = 0; s < 1440; s += 30) f << "NORM," << s << ",5,5,5,5\n"; }
    DealerHedge* q = static_cast<DealerHedge*>(CreateExtension()); h.now = t0 - 30; q->calc(0);
    DHState* R = static_cast<DHState*>(h.data);
    const dh::Zone* rz = nullptr; for (const dh::Zone& x : R->E.zones) if (x.state != dh::HIDDEN) rz = &x;
    CHECK(R->restored == 1 && rz && rz->state == dh::SETTLED && rz->side == dh::BUY && rz->barKey == (long long)h.bars.back().time, "restart: the zone is redrawn from the record (settled)");
    CHECK(rz && R->E.zLine(*rz) == recText.substr(recText.find("Z|"), recText.find('\n', recText.find("Z|")) - recText.find("Z|")), "restart: byte-identical Z line (no repaint)");
    h.draws.clear(); q->draw(); CHECK(R->E.P.show == 0 && boxes(h, 1) == 0 && !hasText(h, "oH") && !hasText(h, "oH?") && readAll(rec) == recText, "SHOW 0 (LOG ONLY): nothing drawn, the record unchanged");
    { std::ofstream f((flex + "\\LRA-DealerHedge-ES.csv").c_str()); f << "VERSION,1\nMARKET,ES\n# promoted\nPARAM,SHOW,1\nPARAM,MIN_LOTS,10\nPARAM,REP_MIN_LOTS,3\n"; for (int s = 0; s < 1440; s += 30) f << "NORM," << s << ",5,5,5,5\n"; }
    usleep(5200000); h.now = t0 - 29; q->calc(0); h.draws.clear(); q->draw();
    CHECK(R->E.P.show == 1 && boxes(h, 1) >= 1 && hasText(h, "oH?"), "SHOW 1: band + 'oH?'");
    // a second chart (NQ) on the same DLL keeps its own state
    mock::Host h2; mock::current = &h2; h2.tick = 0.25f; h2.seconds = 60; h2.symbol = "NQZ26"; h2.root = "NQ";
    for (int i = 0; i < 30; ++i) { mock::Bar bb; bb.time = t0 - (29 - i) * 60; bb.open = 29800; bb.high = 29805; bb.low = 29795; bb.close = 29800; h2.bars.push_back(bb); }
    h2.now = t0; q->calc(0); DHState* S2 = static_cast<DHState*>(h2.data);
    CHECK(S2 && S2 != R && S2->market == "NQ" && S2->mqN == 0 && R->market == "ES", "two charts: separate state (NQ has no levels in this file)");
    mock::current = &h; q->release(); mock::current = &h2; q->release();
    CHECK(h.data == nullptr && h2.data == nullptr && h.timers.empty(), "release: slots and timers cleaned up");
    delete p; delete q;
    std::printf("test_oh_host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
