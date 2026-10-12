// test_liquidityprofile_host.cpp -- the whole plugin (LiquidityProfile.cpp) driven by a mock Investor/RT host (mock/irtsdk.h):
// depth + VAP in, a zone touch, an LX decided on the closed bar, the record written, a restart redrawing the same mark.
//   g++ -std=c++17 -Imock -I. test_liquidityprofile_host.cpp -o th && ./th
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>

namespace mock { Host* current = nullptr; }
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
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }

int main() {
    char tmpl[] = "/tmp/lqpXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    {   // (0.3.1) the per-market thresholds the data chose, in the norms file (bad lines keep the defaults)
        std::ofstream nf((flex + "\\LRA-LiqNorms-CL.csv").c_str());
        nf << "VERSION,test\r\nMARKET,CL\r\nZONE,0.25,4,8\r\nPARAMS_VERSION,t\r\nPARAM,pull_share,0.7,data,10,5,0.1,0.1,why\r\n"
           << "PARAM,eaten_break,1.5,data,1,1,0.1,0.1,why\r\nPARAM,exhaust,7,data,1,1,0,0,out of range\r\n";
    }
    {   // (0.10.0) a MenthorQ put support on the zone (CL: LQ logged, not drawn) and a gamma wall (never confirms)
        std::ofstream mq((flex + "\\MenthorQLevels.csv").c_str());
        mq << "SYMBOL,PRICE,LABEL\r\nCLEX26,79.95,PS 0D\r\nCLEX26,79.94,G3\r\nEPZ26,7000,CR 0D\r\n";
    }
    mock::Host h; mock::current = &h;
    h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);   // Monday, CT clock as epoch
    unsigned long t0 = day + 9 * 3600;                                              // 9:00 AM
    for (int i = 0; i < 25; ++i) {                                                  // history: 3-min bars ending 7:48 .. 9:00
        mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f;
        b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b);
    }
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }  // forming bar
    setDepth(h, 80.00, 80, 79.95, 200);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    h.now = t0;
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); usleep(260 * 1000 / 26); }   // the zone rests 45 s
    LQState* S = static_cast<LQState*>(h.data);
    CHECK(S != nullptr, "per-chart state lives in the host slot");
    if (S) std::printf("state=%s reads=%lld books=%lld bad=%lld why=%s md=%p avail=%d tick=%f\n", S->state.c_str(), S->depthReads, S->e.books, S->e.badBooks, S->e.why.c_str(), (void*)S->md, (int)S->depthAvail, S->tick);
    CHECK(S && S->e.haveBook && S->e.books > 3, "depth snapshots accepted");
    CHECK(S && S->e.P.pullShare == 0.7 && S->e.P.eatenBreak == 1.5, "the market's data-chosen thresholds are used");
    CHECK(S && S->e.P.exhaust == 0.5, "an out-of-range value keeps the default");
    CHECK(S && S->params.find("pull_share 0.7 data") != std::string::npos, "the status says where each threshold came from");
    bool zone = false; for (auto& z : S->e.zones) { std::printf("zone %d c=%lld [%lld,%lld] %.0f\n", z.side, z.center, z.lo, z.hi, z.size); if (z.side == lqp::BID && z.lo <= 7995 && z.hi >= 7995 && z.size >= 200) zone = true; }
    CHECK(zone, "the 200-lot bid at 79.95 is a zone");
    {   // (0.10.0) shown after 30 s at 2x+ the book behind, drawn with its bx label, no box lines across the chart, LQ logged in CL
        bool shown = false; for (auto& z : S->e.zones) if (z.side == lqp::BID && z.lo <= 7995 && z.hi >= 7995) { shown = z.shown; std::printf("bx %.2f rank %d behind %d shown %d\n", z.bx, z.rank, z.behindN, (int)z.shown); }
        CHECK(shown, "(0.10.0) the 200-lot wall is shown (rank 1, 2x+, 30 s)");
        h.draws.clear(); h.priceBase = 79.0f; p->draw(); h.priceBase = 80;
        int lines = 0; bool xl = false, lq = false;
        for (auto& d : h.draws) { if (d.type == "line") lines++; if (d.type == "text" && d.text.size() > 1 && d.text.back() == 'x' && std::isdigit((unsigned char)d.text[0])) xl = true; if (d.type == "text" && d.text.find("LQ") == 0) lq = true; }
        CHECK(lines == 0, "(0.10.0) no boxes across the chart");
        CHECK(xl, "(0.10.0) the bx label is drawn");
        CHECK(!lq, "(0.10.0) CL: LQ is logged, not drawn");
        usleep(1100000); h.now = t0 + 46; p->calc(0);
        const std::string t = readAll(flex + "\\LiquidityProfile.status-CL-180.txt");
        CHECK(t.find("CONFIRM,PS 0D,79.95,support") != std::string::npos && t.find(",logged") != std::string::npos, "(0.10.0) PS on a 2.5x+ support zone confirms (logged in CL)");
        CHECK(t.find("CONFIRM,G3") == std::string::npos, "(0.10.0) a gamma wall never confirms");
    }
    CHECK(S->e.trades == 0, "volume already on the bar at load is history, not new trades");
    // price comes down: sellers hit 79.97 (arrival) then eat 300 at 79.95, then trade 79.92
    mock::Bar& fb = h.bars.back();
    fb.rows.push_back(mock::Price(79.97f, 0, 5, 5)); fb.low = 79.97f; fb.close = 79.97f;
    setDepth(h, 79.97, 80, 79.95, 200);
    h.now = t0 + 47; usleep(1100000); p->calc(0);
    CHECK(S->e.attempts.size() == 1, "arrival into the zone starts an attempt");
    fb.rows.push_back(mock::Price(79.95f, 0, 450, 450)); fb.low = 79.95f; fb.close = 79.95f;   // 1.5x+ the shown size (this market's eaten_break)
    setDepth(h, 79.95, 80, 79.95, 0);
    h.now = t0 + 60; usleep(300000); p->calc(0);
    fb.rows.push_back(mock::Price(79.92f, 0, 20, 20)); fb.low = 79.89f; fb.close = 79.89f;
    setDepth(h, 79.91, 80);
    h.now = t0 + 90; usleep(300000); p->calc(0);
    CHECK(S->e.attempts.size() == 1 && S->e.attempts[0].attackVol >= 300, "attackers' volume counted from the VAP increase");
    // (0.11.4) a read 1 s before the close, then the last second's sells (not read yet) and the close: they must reach the attempt
    // BEFORE it is decided, stamped no later than the bar's end (before: the depth step's gate kept them for the next read)
    h.now = t0 + 179; usleep(1100000); p->calc(0);
    const double volBefore = S->e.attempts.empty() ? -1 : S->e.attempts[0].attackVol;
    fb.rows.push_back(mock::Price(79.96f, 0, 7, 7));
    // the bar closes (a new bar appears) below the zone
    { mock::Bar n; n.time = t0 + 360; n.open = n.high = n.low = n.close = 79.91f; n.rows.push_back(mock::Price(79.91f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = t0 + 181; usleep(300000); p->calc(0);
    {   const lqp::Attempt* fa = S->e.finished.empty() ? nullptr : &S->e.finished.back();
        CHECK(volBefore > 0 && fa && std::fabs(fa->attackVol - (volBefore + 7)) < 1e-9, "(0.11.4) the closing bar's last trades are counted before the bar is decided");
        CHECK(fa && !fa->attacks.empty() && fa->attacks.back().first <= (long long)(t0 + 180) * 1000LL, "(0.11.4) ... stamped at the bar's end, inside its window");
        CHECK(LiquidityProfile::clampFont(9) == 10 && LiquidityProfile::clampFont(0) == 11 && LiquidityProfile::clampFont(12) == 12, "(0.11.4) fonts are 10 pt at least, 11 by default");
    }
    // (0.4.0) the price chart draws no signals: the break is LOGGED for the nightly study, nothing is recorded or drawn as a mark
    bool brk = false; for (auto& m : S->e.marks) if (m.code == "Broke" && m.reasons.find("Broke: sellers hit the bid") == 0) brk = true;
    CHECK(brk && S->marksWritten == S->e.marks.size(), "(0.7.0) the chart build records the touch's reason and its letters");
    const std::string att = flex + "\\LiquidityProfile\\CL-180-attempts-" + std::to_string(S->session) + ".csv";
    CHECK(readAll(att).find("|break eaten") != std::string::npos, "the attempt is logged as an eaten break for the nightly scoring");
    p->draw();
    bool drawn = false; for (auto& d : h.draws) if (d.type == "text" && (d.text == "LX" || d.text.find("L?") == 0 || d.text.find("Exh") == 0)) drawn = true;
    CHECK(!drawn, "no signal text on the price chart");
    bool header = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "LIQ TESTING") header = true;
    CHECK(header, "header says TESTING");
    {   // (0.9.0) the column is on the LEFT edge (facing price): no box or text of the column right of the old column's place,
        // and the zone bars start at the pane's left edge
        int minBox = 1 << 30, over = 0;
        for (auto& d : h.draws) { if (d.type == "box") minBox = std::min(minBox, d.x); if ((d.type == "box" || d.type == "text") && d.x > 1700 && d.text != "LIQ TESTING") over++; }
        if (over) std::printf("  %d items right of x 1700\n", over);
        CHECK(over == 0, "(0.9.0) nothing of the column on the right (the Delta / Dealer Profiles' side)");
        CHECK(minBox <= 2, "(0.9.0) the bars start at the pane's left edge");
    }
    {   // (0.13.0) the picks: the strongest (most contracts) shown zone each side + every shown zone on a level, named
        LQState P; P.e.P.tick = 0.25;
        auto zone = [&](long long id, int side, lqp::Tick lo, lqp::Tick hi, double size, double bx) { lqp::Zone z; z.id = id; z.side = side; z.lo = lo; z.hi = hi;
            z.edgeLo = lo; z.edgeHi = hi; z.size = size; z.bx = bx; z.shown = true; P.e.zones.push_back(z); };
        zone(1, lqp::ASK, 31450, 31460, 844, 2.2);     // 7862.5-7865
        zone(2, lqp::BID, 31400, 31410, 1223, 2.1);    // 7850-7852.5
        zone(3, lqp::BID, 31386, 31394, 928, 2.1);
        zone(4, lqp::BID, 31374, 31382, 712, 2.1);
        zone(5, lqp::BID, 31356, 31364, 669, 2.2);     // 7839-7841
        LQState::Lvl g2; g2.code = "G2"; g2.px = 31400; LQState::Lvl hvl; hvl.code = "HVL"; hvl.px = 31360;
        P.mqLevels.push_back(g2); P.mqLevels.push_back(hvl);
        const std::vector<LqPick> pk = lqPicks(P);
        std::string got; for (auto& k : pk) got += std::to_string(P.e.zones[k.idx].size) .substr(0, 4) + (k.top ? "T" : "L") + k.names + " ";
        std::printf("  picks: %s\n", got.c_str());
        bool a844 = false, b1223 = false, h669 = false, no928 = true, no712 = true;
        for (auto& k : pk) { const double sz = P.e.zones[k.idx].size;
            if (sz == 844 && k.top) a844 = true; if (sz == 1223 && k.top && k.names == "G2") b1223 = true; if (sz == 669 && !k.top && k.names == "HVL") h669 = true;
            if (sz == 928) no928 = false; if (sz == 712) no712 = false; }
        CHECK(a844 && b1223 && h669 && no928 && no712 && pk.size() == 3, "(0.13.0) 844 offers + 1223 bids (on G2) solid, 669 on HVL outlined, 928 / 712 hidden");
        {   // (0.13.1) two offer zones touching ONL / LonLO (13:27 screenshot): each level on ONE zone - the one holding its price
            LQState Q; Q.e.P.tick = 0.25;
            auto zq = [&](long long id, int side, lqp::Tick lo, lqp::Tick hi, double size, double bx) { lqp::Zone z; z.id = id; z.side = side; z.lo = lo; z.hi = hi;
                z.edgeLo = lo; z.edgeHi = hi; z.size = size; z.bx = bx; z.shown = true; Q.e.zones.push_back(z); };
            zq(1, lqp::ASK, 31432, 31440, 854, 1.9);   // 7858-7860
            zq(2, lqp::ASK, 31423, 31431, 954, 1.8);   // 7855.75-7857.75
            LQState::Lvl onl; onl.code = "ONL"; onl.px = 31431; LQState::Lvl lon; lon.code = "LonLO"; lon.px = 31440;
            Q.levels.push_back(onl); Q.levels.push_back(lon);
            std::string n854, n954; for (auto& k : lqPicks(Q)) { if (Q.e.zones[k.idx].size == 854) n854 = k.names; if (Q.e.zones[k.idx].size == 954) n954 = k.names; }
            std::printf("  854: %s | 954: %s\n", n854.c_str(), n954.c_str());
            CHECK(n854 == "LonLO" && n954 == "ONL", "(0.13.1) LonLO on the 854 zone, ONL on the 954 zone - one zone per level");
        }
    }
    const std::string st = flex + "\\LiquidityProfile.status-CL-180.txt";
    CHECK(readAll(st).find("VERSION,0.13.1") != std::string::npos && std::string(lqp::VERSION) == LQ_VERSION, "status file written (0.13.1, one version everywhere)");
    {   // (0.12.0) the level-liquidity file for the Dealer Profile badges: header + every zone of 1.5x or more, side and prominence
        const std::string lz = readAll(flex + "\\LRA-LiqZones-CL.csv"), t = readAll(st);
        CHECK(lz.compare(0, 7, "LIQZ,1,") == 0, ("(0.12.0) LRA-LiqZones-CL.csv written with its header: " + lz.substr(0, 40)).c_str());
        // (0.13.0) the file holds only the picked zones: at most one solid (strongest) per side + zones on a level; all are shown zones
        int got = 0, shownN = 0; size_t q = 0;
        while ((q = t.find(",shown,", q)) != std::string::npos) { q += 7; shownN++; }
        int sup = 0, res = 0; for (size_t k = 0; (k = lz.find("\nZ,", k)) != std::string::npos; k += 3) { got++; if (lz[k + 3] == 'S') sup++; else res++; }
        std::printf("  liq zones: %d in the file (%d bid, %d offer), %d shown in the status\n", got, sup, res, shownN);
        CHECK(got <= shownN, "(0.13.0) the file: the picked zones only (strongest each side + on a level)");
    }
    CHECK(readAll(st).find("\nSTATS,") != std::string::npos && readAll(st).find("bars_known") != std::string::npos, "(0.6.0) the status carries the pane statistics and their lines");
    {   // (0.10.0) the reach, the prominence, the LQ confirmation (CL logs it), no boxes across the chart, the bx label
        const std::string t = readAll(st);
        CHECK(t.find("\nREACH,6x 3-min range") != std::string::npos, "(0.10.0) reach falls back to 6x with no HodLod file and < 10 sessions");
        CHECK(t.find("\nLEVELS,") != std::string::npos && t.find(",1 gamma1") != std::string::npos, "(0.10.0) one CR / PS / HVL level for CL (G3 and the ES line ignored)");
        CHECK(t.find("ZONE,bx ") != std::string::npos && t.find(",rank ") != std::string::npos, "(0.10.0) zones report bx and rank");
    }
    const size_t nMarks = S->e.marks.size();
    {   // (0.8.0) the s letter on the price chart (the process in every market: drawn at the standard Counter line with no data line)
        lqp::Mark m; m.code = "s"; m.value = 2.0; m.side = lqp::ASK; m.barTime = (long long)h.bars[h.bars.size() - 3].time; m.reasons = "Buyers stepped in: 2.0x normal, out-traded sellers";
        lqp::Mark lo = m; lo.value = 1.0; lo.barTime = (long long)h.bars[h.bars.size() - 4].time;
        S->e.marks.push_back(m); S->e.marks.push_back(lo); h.draws.clear(); p->draw();
        int ns = 0; for (auto& d : h.draws) if (d.type == "text" && d.text == "s") ++ns;
        CHECK(ns == 0, "0.11.3: no s on the price chart (recorded only)");
        { lqp::Mark pm; pm.code = "Pull"; pm.value = 0.95; pm.side = lqp::BID; pm.barTime = (long long)h.bars[h.bars.size() - 3].time; lqp::Mark em = pm; em.code = "Exh"; em.value = 0.1; lqp::Mark tm = pm; tm.code = "t";
          S->e.marks.push_back(pm); S->e.marks.push_back(em); S->e.marks.push_back(tm); h.draws.clear(); p->draw();
          int nl = 0; for (auto& d : h.draws) if (d.type == "text" && (d.text == "p" || d.text == "e" || d.text == "t" || d.text == "s")) ++nl;
          CHECK(nl == 0, "0.11.3: p / e / t at the line are recorded, never drawn on the price chart"); S->e.marks.pop_back(); S->e.marks.pop_back(); S->e.marks.pop_back(); }
        S->e.marks.pop_back(); S->e.marks.pop_back();
    }
    p->destroy(); CHECK(h.data == nullptr, "destroy frees the slot");
    LiquidityProfile* q = static_cast<LiquidityProfile*>(CreateExtension());
    h.draws.clear(); h.now = t0 + 200; q->calc(0); q->draw();
    LQState* R = static_cast<LQState*>(h.data);
    CHECK(R && R->e.marks.size() == nMarks, "(0.7.0) after a restart the record is read back (no repaint)");
    bool letters = false; for (auto& d : h.draws) if (d.type == "text" && (d.text == "e" || d.text == "p" || d.text == "t" || d.text == "e?" || d.text == "p?")) letters = true;
    bool watches = false; for (auto& d : h.draws) if (d.type == "text" && (d.text == "e?" || d.text == "p?")) watches = true;
    CHECK(!watches, "the e? / p? watches are recorded, never drawn");
    CHECK(!letters, "no letters for a touch where no e / p / t step happened (an eaten break)");
    CHECK(R->e.trades == 0, "a restart does not replay old volume as trades");
    // no depth from the feed: says so, draws no book, no crash
    h.depthOn = false; h.draws.clear(); LiquidityProfile* z = static_cast<LiquidityProfile*>(CreateExtension()); z->destroy();
    q->destroy(); h.data = nullptr;
    LiquidityProfile* w = static_cast<LiquidityProfile*>(CreateExtension());
    h.now = t0 + 300; w->calc(0); w->draw();
    bool noDepth = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "LIQ no depth") noDepth = true;
    CHECK(noDepth, "no depth from this feed is shown as LIQ no depth");
    w->destroy();
    CHECK(h.depthObjects == 0, "every depth object is released (unsubscribed)");
    {   // (0.5.0) old orders: a zone that rests 10 min+ untouched is outlined with "old 100%" where the market's study confirmed it
        { std::ofstream nf((flex + "\\LRA-LiqNorms-CL.csv").c_str()); nf << "MARKET,CL\r\nZONE,0.25,4,8\r\nSIGNAL,Old,confirmed\r\n"; }
        mock::Host h2; mock::current = &h2; h2.tick = 0.01f; h2.seconds = 180; h2.symbol = "CLEX26"; h2.root = "CLEX26";
        for (int i = 0; i < 25; ++i) { mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h2.bars.push_back(b); }
        { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h2.bars.push_back(f); }
        setDepth(h2, 80.00, 80, 79.95, 200); h2.priceBase = 79.0f;
        LiquidityProfile* o = static_cast<LiquidityProfile*>(CreateExtension());
        h2.now = t0 + 300; o->calc(0); o->draw();
        bool early = false; for (auto& d : h2.draws) if (d.type == "text" && d.text.find("old ") == 0) early = true;
        CHECK(!early, "(0.5.0) no old mark before the zone has rested 10 min");
        for (unsigned long s2 = 0; s2 <= 640; s2 += 5) { if (s2 % 100 == 0) usleep(1100000); h2.now = t0 + 300 + s2; o->calc(0); }   // (0.10.0) depth / zones refresh on the steady clock
        LQState* O = static_cast<LQState*>(h2.data);
        CHECK(O && O->confirmed.count("Old"), "SIGNAL,Old,confirmed is read");
        h2.draws.clear(); o->draw();
        bool old = false; for (auto& d : h2.draws) if (d.type == "text" && d.text.find("old 100%") == 0) old = true;
        CHECK(old, "(0.5.0) the zone resting 10 min+ is marked old 100%");
        o->destroy();
        { std::ofstream nf((flex + "\\LRA-LiqNorms-CL.csv").c_str()); nf << "MARKET,CL\r\nZONE,0.25,4,8\r\nSIGNAL,Old,testing\r\n"; }
        LiquidityProfile* o2 = static_cast<LiquidityProfile*>(CreateExtension());
        for (unsigned long s2 = 0; s2 <= 640; s2 += 5) { h2.now = t0 + 1000 + s2; o2->calc(0); }
        h2.draws.clear(); o2->draw();
        bool old2 = false; for (auto& d : h2.draws) if (d.type == "text" && d.text.find("old ") == 0) old2 = true;
        CHECK(!old2, "(0.5.0) not drawn where the market's study has not confirmed old orders");
        o2->destroy();
    }
    {   // (0.11.4) a symbol change on the same chart: the new market's RTH hours, MenthorQ levels and Delta A zones, nothing of the old one
        mock::Host h3; mock::current = &h3; h3.tick = 0.01f; h3.seconds = 180; h3.symbol = "CLEX26"; h3.root = "CLEX26";
        for (int i = 0; i < 40; ++i) { mock::Bar b; b.time = t0 - (39 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h3.bars.push_back(b); }
        { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h3.bars.push_back(f); }
        setDepth(h3, 80.00, 80);
        LiquidityProfile* c = static_cast<LiquidityProfile*>(CreateExtension());
        h3.now = t0 + 10; c->calc(0);
        LQState* C = static_cast<LQState*>(h3.data);
        CHECK(C && C->rthOpen == 8 * 60 && C->mqLevels.size() == 1 && C->mqLevels[0].code == "PS 0D", "fixture: CL hours and CL's PS level");
        h3.symbol = "ESZ26"; h3.root = "ES"; h3.tick = 0.25f; usleep(1100000); h3.now = t0 + 12; c->calc(0);
        CHECK(C && C->market == "ES" && C->rthOpen == 8 * 60 + 30, "(0.11.4) after a symbol change the new market's RTH hours are used");
        CHECK(C && C->mqLevels.size() == 1 && C->mqLevels[0].code == "CR 0D" && C->mqLevels[0].px == 28000, "(0.11.4) ... and its own MenthorQ levels (not the old market's)");
        // (0.11.4) a Delta Profile A older than an hour is dropped even when the Delta Profile writes nothing new
        const std::string dpf = flex + "\\DeltaProfile\\ES-180-signals-" + std::to_string(C->session) + ".csv";
        { std::ofstream d(dpf.c_str()); d << (long long)(t0 - 1800) << "|x|A|support|x|6999.00|7000.00|x\r\n"; }
        usleep(1100000); h3.now = t0 + 14; c->calc(0);
        CHECK(C->dpBid.size() == 1 && C->e.dpZone[lqp::BID].size() == 1, "fixture: an A 30 min old is used");
        usleep(1100000); h3.now = t0 + 14 + 3600; c->calc(0);
        CHECK(C->dpBid.empty() && C->e.dpZone[lqp::BID].empty(), "(0.11.4) 90 min old, file unchanged: dropped (the last hour only)");
        c->destroy();
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
