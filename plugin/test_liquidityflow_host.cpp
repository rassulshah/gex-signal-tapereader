// test_liquidityflow_host.cpp -- the pane build (lsLiquidityFlow) (LiquidityProfile.cpp) driven by a mock Investor/RT host (mock/irtsdk.h):
// depth + VAP in, a zone touch, an LX decided on the closed bar, the record written, a restart redrawing the same mark.
//   g++ -std=c++17 -Imock -I. test_liquidityprofile_host.cpp -o th && ./th
#define LQ_FLOW 1
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>

namespace mock { Host* current = nullptr; Feed feed; }   // (0.15.0) + the order feed
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)

static void setDepth(mock::Host& h, double bestBid, int levels, double extraPx = 0, double extra = 0) {
    h.depth.clear();
    for (int i = 0; i < levels; ++i) {
        DEPTH_LEVEL L; L.bid = (float)(bestBid - i * 0.01); L.ask = (float)(bestBid + 0.01 + i * 0.01);
        L.bidsize = 10; L.asksize = 10;
        if (extra > 0 && std::fabs(L.bid - extraPx) < 0.001) L.bidsize = (unsigned short)(10 + extra);
        h.depth.push_back(L);
    }
}
static std::string flex_attempts(const std::string& path) { std::ifstream f(path.c_str()); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }

int main() {
    char tmpl[] = "/tmp/lqfXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string normsPath = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-LiqNorms-CL.csv";
    {   // (0.5.0) the study confirmed Exh for this market; Pull is not confirmed here; old codes in the file are ignored
        std::ofstream nf(normsPath.c_str());
        nf << "MARKET,CL\r\nZONE,0.25,4,8\r\nSIGNAL,Exh,confirmed\r\nSIGNAL,Pull,testing\r\nSIGNAL,LoF,confirmed\r\nPARAM,exhaust,0.5,data\r\n"
           << "PARAM,attack_x,2.0,data,1,1,0,0,w\r\nPARAM,eaten_total,0.12,data,1,1,0,0,w\r\nPARAM,pull_share,0.7,default,1,1,0,0,w\r\nPARAM,rebuilt,1.0,data,1,1,0,0,w\r\n";
    }
    mock::Host h; mock::current = &h; h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL); unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 25; ++i) { mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 20, 20, 40)); h.bars.push_back(b); }   // normal pace: 20 a side per bar
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }
    setDepth(h, 80.00, 80, 79.95, 200);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); }
    LQState* S = static_cast<LQState*>(h.data);
    {   // (0.11.0) always on: before any touch the pane already draws the bars, both headers and what it is watching
        h.draws.clear(); p->draw();
        bool buyers = false, watching = false, bidsNear = false; int boxes = 0;
        for (auto& d : h.draws) { if (d.type == "text" && d.text == "Buyers: ") buyers = true; if (d.type == "text" && d.text.find("Watching:") == 0) watching = true;
            if (d.type == "text" && d.text == "Bids near price: ") bidsNear = true; if (d.type == "box") ++boxes; }
        CHECK(buyers, "(0.11.0) the Aggressive header reads Buyers: now (line) with no touch");
        CHECK(bidsNear, "(0.11.0) the Passive header reads the resting bids / offers near price");
        CHECK(watching, "(0.11.0) idle, the readout says what it is watching");
        CHECK(boxes >= 4, "(0.11.0) the buyers / sellers bars are drawn with no touch");
    }
    // six quiet live bars first
    unsigned long T = t0;
    for (int k = 0; k < 6; ++k) {
        h.bars.back().rows.push_back(mock::Price(80.03f, 5, 5, 10));
        for (int s = 0; s < 180; s += 20) { setDepth(h, 80.00, 80, 79.95, 200 + ((s / 20 + k) % 2) * 6); h.now = T + 45 + s; p->calc(0); }
        mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 80.03f; n.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(n);
        T += 180; h.now = T + 41; p->calc(0);
    }
    setDepth(h, 80.00, 80, 79.95, 200); h.now = T + 44; p->calc(0);
    mock::Bar& fb = h.bars.back();
    fb.rows.push_back(mock::Price(79.97f, 0, 40, 40)); setDepth(h, 79.97, 80, 79.95, 120); h.now = T + 50; p->calc(0);
    fb.rows.push_back(mock::Price(79.96f, 3, 30, 33)); h.now = T + 90; p->calc(0);
    { mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 79.96f; n.rows.push_back(mock::Price(79.96f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = T + 181; p->calc(0);
    mock::Bar& nb = h.bars.back();
    setDepth(h, 79.96, 80, 79.95, 300); h.now = T + 200; p->calc(0);
    nb.rows[0] = mock::Price(79.96f, 90, 4, 94); h.now = T + 260; p->calc(0);
    { mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 80.00f; n.rows.push_back(mock::Price(80.00f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = T + 361; p->calc(0);
    h.draws.clear(); p->draw();
    bool head = false, lines = false; int nl = 0; for (auto& d : h.draws) { if (d.type == "text" && d.text.find("AGGRESSIVE") == 0) head = true; if (d.type == "line") ++nl; }
    lines = nl >= 8;
    CHECK(head, "(0.8.0) the panes are labelled AGGRESSIVE / PASSIVE");
    {   // (0.11.0) the touch = a yellow window with its price, and the readout names it
        bool win = false, rd = false;
        for (auto& d : h.draws) { if (d.type == "text" && d.color == 0x003CC8FA && d.text.find("79.9") == 0) win = true;
            if (d.type == "text" && (d.text.find("ABSORPTION  ") == 0 || d.text.find("LAST  ") == 0)) rd = true; }
        CHECK(win, "(0.11.0) the absorption window carries its price in yellow");
        bool longPx = false; for (auto& d : h.draws) if (d.type == "text" && (d.text.find("79.979") != std::string::npos || d.text.find("79.959") != std::string::npos)) longPx = true;
        CHECK(!longPx && S->tick == 0.01, "(0.11.4) CL prices read 79.98, not 79.979998 (IRT's float tick is snapped)");
        CHECK(rd, "(0.11.0) the readout is headed with the absorption it reads");
    }
    CHECK(lines, "buyers / sellers aggression and both net resting lines are drawn");
    const lqp::Attempt* a = !S->e.finished.empty() ? &S->e.finished.back() : (!S->e.attempts.empty() ? &S->e.attempts.back() : nullptr);
    CHECK(a && a->flowBars.size() >= 2, "the touch has per-bar flow");
    CHECK(a && a->barO >= 0 && a->barR >= 0, "buyers took over the trading and the bids' net crossed above the offers'");
    bool exh = false, removed = false, qm = false;
    for (auto& d : h.draws) if (d.type == "text") {
        if (d.text.find("Exh ") == 0 && d.text.find("x") != std::string::npos) exh = true;
        if (d.text.find("LoF") == 0 || d.text.find("HiB") == 0 || d.text.find("RpB") == 0 || d.text.find("RpO") == 0 || d.text.find("Stop") == 0) removed = true;
        if (d.text.find('?') != std::string::npos && d.text.find("AGGRESSIVE") != 0 && d.text.find("PASSIVE") != 0) qm = true;
    }
    CHECK(exh, "Exh drawn with its number (confirmed for this market)");
    // (0.6.0) the statistics column: drawn for the lines that came from the data, in process order; Pulled's line is a default -> not drawn
    // (0.8.0) the process in every market: AGGRESSIVE Attack / Exh / Counter, PASSIVE Absorbed / Pulled / Rebuilt, each with its line
    // (the market's data line, or the standard line until the study sets one - Pulled here is a default line and is still drawn)
    std::vector<std::string> labels; for (auto& d : h.draws) if (d.type == "text" && (d.text == "Attack: " || d.text == "Exh: " || d.text == "Counter: " || d.text == "Absorbed: " || d.text == "Pulled: " || d.text == "Rebuilt: ")) labels.push_back(d.text);
    CHECK(labels.size() == 6 && labels[0] == "Attack: " && labels[1] == "Exh: " && labels[2] == "Counter: " && labels[3] == "Absorbed: " && labels[4] == "Pulled: " && labels[5] == "Rebuilt: ",
          "(0.8.0) the process in order: Attack, Exh, Counter | Absorbed, Pulled, Rebuilt");
    bool pct = false, pullLine = false;
    for (auto& d : h.draws) if (d.type == "text" && d.text == " (12%)") pct = true;
    for (auto& d : h.draws) if (d.type == "text" && d.text == " (70%)") pullLine = true;   // (0.11.4) Pulled reads as a %, like the Pull signal
    CHECK(pct, "Absorbed's line shows as a % of everything resting");
    CHECK(pullLine, "(0.8.0) a step whose line is not from the data yet is drawn at the standard line");
    CHECK(!removed, "(0.5.0) LoF / HiB / RpB / RpO / Stop are never drawn");
    {   // (0.11.4) nothing overlaps: the reasons sit below the header / window / step rows and end before the readout; steps are not on
        // the window-label row; no two texts on one row cover each other
        const int rowH = rowPx(S->font), t0 = rowH + 2, plotR = h.paneRight - std::min(330, h.paneRight / 3) - 8;
        int reasons = 0, bad = 0;
        for (auto& d : h.draws) if (d.type == "text" && ((d.text.find("Absorbed: ") == 0 && d.text.size() > 10) || d.text.find("Buyers stepped") == 0 || d.text.find("Sellers stepped") == 0 || d.text.find("Broke:") == 0 || d.text.find("  ") == 0)) {
            ++reasons; if (d.y < t0 + 3 * rowH || d.x + (int)d.text.size() * 6 > plotR) { ++bad; std::printf("  reason at %d,%d: %s\n", d.x, d.y, d.text.c_str()); } }
        CHECK(reasons > 0 && bad == 0, "(0.11.4) the reasons are under the header / window / step rows and stop before the readout");
        bool stepOnWin = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find("Exh ") == 0 && d.y < t0 + rowH) stepOnWin = true;
        CHECK(!stepOnWin, "(0.11.4) Exh is not drawn on the window-label row");
        int clash = 0;
        for (size_t i = 0; i < h.draws.size(); ++i) for (size_t j = i + 1; j < h.draws.size(); ++j) {
            const auto& a = h.draws[i]; const auto& b = h.draws[j];
            if (a.type != "text" || b.type != "text" || std::abs(a.y - b.y) >= rowH - 2 || a.text.find("Exh") != 0 && a.text.find("Absorbed") != 0 && a.text.find("79.9") != 0) continue;
            const int aw = (int)a.text.size() * 6, bw = (int)b.text.size() * 6;
            if (a.x < b.x + bw && b.x < a.x + aw && !(a.x == b.x && a.text == b.text)) { ++clash; std::printf("  clash '%s' (%d,%d) / '%s' (%d,%d)\n", a.text.c_str(), a.x, a.y, b.text.c_str(), b.x, b.y); }
        }
        CHECK(clash == 0, "(0.11.4) steps, window prices and reasons never cover another text");
    }
    CHECK(!qm, "(0.5.0) nothing is drawn with a '?'");
    const std::string rec = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LiquidityProfile\\" + (S->market + "-" + fileToken(S->symbol) + "-" + std::to_string(S->spb)) + "-panes-" + std::to_string(S->session) + ".csv";
    CHECK(readAll(rec).find("|Exh|") != std::string::npos && readAll(rec).find("|LoF|") == std::string::npos, "Exh is recorded for the session, no removed codes");
    std::string attempts = flex_attempts(std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LiquidityProfile\\" + (S->market + "-" + fileToken(S->symbol) + "-" + std::to_string(S->spb)) + "-attempts-" + std::to_string(S->session) + ".csv");
    CHECK(attempts.empty(), "the pane build writes no attempt log (the profile owns it)");
    const std::string stemS = S->market + "-" + fileToken(S->symbol) + "-" + std::to_string(S->spb); const long long sessS = S->session;   // (0.15.0 ASan) S is freed by destroy()
    p->destroy();
    LiquidityProfile* q = static_cast<LiquidityProfile*>(CreateExtension());
    h.draws.clear(); h.now = T + 400; q->calc(0); q->draw();
    bool again = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find("Exh ") == 0) again = true;
    CHECK(again, "after a restart the same signals are drawn again (no repaint)");
    {   // (0.11.1) backfill: the pane's own record (book near price per bar + the absorption window) is re-read after a restart
        const std::string fr = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LiquidityProfile\\" + stemS + "-flow-" + std::to_string(sessS) + ".csv";
        const std::string t = readAll(fr);
        CHECK(t.find("\nB|") != std::string::npos && t.find("\nW|") != std::string::npos, "(0.11.1) each bar's book and the window are recorded");
        LQState* Q = static_cast<LQState*>(h.data);
        CHECK(Q && Q->bookHist.size() >= 3 && Q->winHist.size() >= 1, "(0.11.1) a restart reads the record back");
        bool win = false; for (auto& d : h.draws) if (d.type == "text" && d.color == 0x003CC8FA && d.text.find("79.9") == 0) win = true;
        CHECK(win, "(0.11.1) the absorption window is redrawn after the restart");
    }
    q->destroy();
    {   // the same session, but this market's study no longer confirms Exh: it is not drawn at all
        std::ofstream nf(normsPath.c_str());
        nf << "MARKET,CL\r\nZONE,0.25,4,8\r\nSIGNAL,Exh,testing\r\n";
    }
    LiquidityProfile* r = static_cast<LiquidityProfile*>(CreateExtension());
    h.draws.clear(); h.now = T + 420; r->calc(0); r->draw();
    bool any = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find("Exh 0") == 0) any = true;
    CHECK(any, "(0.8.0) the process is drawn in every market: Exh still drawn at the standard line when the study has not set one");
    {   // (0.11.4) a line moved by the nightly study (exhaust 0.5 -> 0.1) does not hide the Exh already recorded at 0.5: no repaint
        r->destroy();
        { std::ofstream nf(normsPath.c_str()); nf << "MARKET,CL\r\nZONE,0.25,4,8\r\nPARAM,exhaust,0.1,data,1,1,0,0,w\r\n"; }
        r = static_cast<LiquidityProfile*>(CreateExtension());
        h.now = T + 425; r->calc(0);
        LQState* R = static_cast<LQState*>(h.data);
        lqp::Mark em; em.code = "Exh"; em.value = 0.4; em.side = lqp::BID; em.barTime = (long long)h.bars[h.bars.size() - 12].time;   // recorded under the 0.5 line
        R->e.marks.push_back(em); h.draws.clear(); r->draw();
        bool kept = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "Exh 0.4x") kept = true;
        CHECK(R && R->e.P.exhaust == 0.1 && kept, "(0.11.4) Exh recorded under the old line is still drawn after the line changes");
        // (0.11.4) the engine keeps the last 50 finished touches: the 51st is still recorded as a window (before: never, after the 50th)
        const long cnt = (long)h.bars.size();
        lqp::Attempt a; a.side = lqp::BID; a.lo = 7990; a.hi = 7994; a.done = true; a.barA = (int)cnt - 3; a.tA = 1;
        R->e.finished.assign(50, a); R->e.finTotal = 50; R->finSeen = 50;
        lqp::Attempt b = a; b.barA = (int)cnt - 2; R->e.finished.erase(R->e.finished.begin()); R->e.finished.push_back(b); R->e.finTotal = 51;
        const long long want = (long long)h.bars[(size_t)(cnt - 2)].time;
        { mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 80.00f; n.rows.push_back(mock::Price(80.00f, 0, 0, 0)); h.bars.push_back(n); }
        h.now = T + 545; r->calc(0);
        bool rec = false; for (auto& w : R->winHist) if (w.tA == want) rec = true;
        CHECK(rec && R->finSeen == 51, "(0.11.4) the 51st finished touch is recorded (windows counted by total, not list index)");
    }
    r->destroy();
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
