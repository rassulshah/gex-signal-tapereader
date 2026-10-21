// test_defenselines_host.cpp -- lsDefenseLines 1.1.0 on the mock host: reads the turns file written by lsBookFlow, draws the R line from the
// candle it refers to (yellow dot), the gray E line from its X record, at most 3 candles / the next line / a close through it (dimmed, "x");
// the live "?" line; the settings file switches R lines / E lines / dots off; a torn file never removes a line.
#include "DefenseLines.cpp"
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <utime.h>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static int lines(mock::Host& h) { int n = 0; for (auto& d : h.draws) n += d.type == "line"; return n; }
static const unsigned long T0 = 1791900000UL;
static void settings(const std::string& flex, int key, double v) {
    std::ofstream f((flex + "\\DefenseLines.settings-ES.txt").c_str()); ss::Values V = ss::defaults(dlset::spec(), 0); if (key >= 0) V.v[(size_t)key] = v; f << ss::format(dlset::spec(), V);
}
int main() {
    char tmpl[] = "/tmp/dl110XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 300; h.paneRight = 2000; h.ppb = 10; h.barW = 20;
    for (int i = 0; i < 40; ++i) { mock::Bar b; b.time = T0 + 180UL * i; b.open = 7000; b.high = 7001; b.low = 6999; b.close = 7000; h.bars.push_back(b); }
    mkdir(flex.c_str(), 0700); mkdir((flex + "\\BookFlow").c_str(), 0700);
    // a high turn on bar 10 (pivot = bar 10, line 7001.0), the 2nd candle after it closes above -> broken there
    h.bars[11].close = 7000.5f; h.bars[12].close = 7001.5f;
    dt::Turn a; a.t = (long long)h.bars[10].time; a.pivotT = a.t; a.side = dt::HIGH; a.pivotPx = 28004; a.linePx = 28004; a.marginPct = 23;
    // a low turn on bar 21, pivot bar 20 (low 6998), refill line 6998.25, exhaustion at 6998.0 on bar 20
    h.bars[20].low = 6998; h.bars[21].low = 6998.5f;
    dt::Turn b; b.t = (long long)h.bars[21].time; b.pivotT = (long long)h.bars[20].time; b.side = dt::LOW; b.pivotPx = 27992; b.linePx = 27993; b.marginPct = 60; b.A = true;
    dt::Turn c; c.t = (long long)h.bars[30].time; c.pivotT = c.t; c.side = dt::HIGH; c.pivotPx = 28004; c.linePx = 28004; c.marginPct = 5;
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str());
      f << dt::lineOf('T', a, 0.25) << "\n" << dt::lineOf('T', b, 0.25) << "\n" << dt::xLineOf(b.t, dt::LOW, 27992, 0.25, b.pivotT) << "\n" << dt::lineOf('T', c, 0.25) << "\nT|garbage\n"; }
    DefenseLines* p = static_cast<DefenseLines*>(CreateExtension()); p->setup();
    h.draws.clear(); p->draw();
    DlState* s = p->stateFor();
    CHECK(s && s->lines.size() == 3 && s->xs.size() == 1 && s->bad == 1, "3 turns + 1 exhaustion line read, 1 unreadable line skipped");
    CHECK(text(h, "x R ?") == nullptr, "(1.2.0) the high line closed through 2 candles later: removed (no dimmed 'x R ?')");
    {   int red = 0; for (auto& d : h.draws) red += d.type == "text" && d.color == 0xEF4444; CHECK(red == 1, "(1.2.0) only the third (holding) high line's label is drawn in red"); }
    {   const mock::Draw* d = text(h, "R A ?"); CHECK(d && d->color == 0x22C55E, "the low line: green 'R A ?'"); }
    {   const mock::Draw* d = text(h, "E ?"); CHECK(d && d->color == 0xA3A3A3, "the gray exhaustion line 'E ?'"); }
    CHECK(text(h, "R ?") != nullptr, "the third line: R ?");
    {   const mock::Draw* r = text(h, "R A ?"); const mock::Draw* e = text(h, "E ?"); CHECK(r && e && e->y > r->y, "E (6998.0) drawn below R (6998.25) at a low"); }
    const int all = lines(h);
    settings(flex, dlset::K_SHOW_DOTS, 0); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(lines(h) < all && text(h, "R A ?") != nullptr, "dots off: fewer strokes, the lines stay");
    settings(flex, dlset::K_SHOW_E, 0); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(text(h, "E ?") == nullptr && text(h, "R A ?") != nullptr, "E lines off: no E, R stays");
    settings(flex, dlset::K_SHOW_LINES, 0); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(text(h, "R A ?") == nullptr && text(h, "E ?") != nullptr, "R lines off: no R, E stays");
    settings(flex, dlset::K_MAX_CANDLES, 1); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    {   int r = 0; for (auto& d : h.draws) r += d.type == "text" && d.text == "R ?"; CHECK(r == 2, "1 candle: the high line ends before the close through, so it stays (2 'R ?' lines)"); }
    settings(flex, -1, 0);
    // the live "?" on the forming candle
    dt::Turn lv; lv.t = (long long)h.bars[39].time; lv.side = dt::LOW; lv.pivotPx = 27990; lv.linePx = 27990; lv.marginPct = 12;
    const std::string livePath = flex + "\\BookFlow\\ES-180-turns-live.txt";
    lv.linePx = 27989; const std::string liveFirst = dt::lineOf('L', lv, 0.25); lv.linePx = 27991; const std::string liveSecond = dt::lineOf('L', lv, 0.25);
    CHECK(liveFirst.size() == liveSecond.size(), "same-size live fixture");
    { std::ofstream f(livePath.c_str()); f << liveFirst << "\n"; }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(text(h, "? R") != nullptr && s->haveLive && s->liveLine.linePx == 6997.25, "the forming candle: dotted '? R' line");
    struct stat liveStat; CHECK(stat(livePath.c_str(), &liveStat) == 0, "live fixture stat");
    { std::ofstream f(livePath.c_str()); f << liveSecond << "\n"; }
    struct utimbuf sameLiveTime; sameLiveTime.actime = liveStat.st_atime; sameLiveTime.modtime = liveStat.st_mtime;
    CHECK(utime(livePath.c_str(), &sameLiveTime) == 0, "restore live fixture mtime");
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->haveLive && s->liveLine.linePx == 6997.75, "same-size, same-mtime live update replaces the cached '?' line");
    CHECK(unlink(livePath.c_str()) == 0, "remove live fixture");
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(!s->haveLive && text(h, "? R") == nullptr, "a removed live file clears the cached '?' line");
    { std::ofstream f(livePath.c_str()); f << "N\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(text(h, "? R") == nullptr && text(h, "R A ?") != nullptr, "not confirmed: the ? is gone, the decided lines stay");
    // a shorter (torn) file never removes drawn lines
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str()); f << dt::lineOf('T', a, 0.25) << "\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->lines.size() == 3 && s->xs.size() == 1 && text(h, "R A ?") != nullptr && text(h, "E ?") != nullptr, "a torn shorter file: the lines stay (no repaint)");
    std::ifstream st((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string body((std::istreambuf_iterator<char>(st)), std::istreambuf_iterator<char>());
    CHECK(body.find("failed lines removed") != std::string::npos, "(1.2.0) the status counts the removed lines");
    CHECK(body.find("VERSION,1.7.0") != std::string::npos && body.find("3 turns") != std::string::npos && body.find("exhaustion lines 1") != std::string::npos, "status file");
    {   // (1.3.0, Top Defender #5) lsBookFlow decided the defenders hit back after the A: the label reads "R A H ?"; "absorbed only" keeps "R A ?"
        { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); dt::Counter C; C.t = b.t; C.side = b.side; C.H = true; C.lots = 640; C.attack = 266; C.at = b.t + 360; f << dt::hLineOf(C) << "\n"; }
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        CHECK(text(h, "R A H ?") != nullptr, "(1.3.0) H decided -> the line label R A H ?");
        { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); f << "H|garbage\n"; }
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        CHECK(text(h, "R A H ?") != nullptr && s->hs.size() == 1, "(1.3.0) a garbled H line is ignored, the decided H stays (no repaint)");
        const long long bad0 = s->bad;
        { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); dt::Retest q; q.t = b.t; q.side = b.side; q.res = 'L'; q.ratio = 1.4; q.rest = 700; q.at = b.t + 720; f << dt::qLineOf(q) << "\n"; }
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        CHECK(s->bad == bad0 && text(h, "R A H ?") != nullptr && text(h, "R A H L ?") == nullptr, "(1.4.0) a Q line (re-test L / X) is read, not counted bad; the line label stays R A H ? (L / X in the pane)");
        {   // (1.7.0) lsBookFlow 1.19.0's G and W lines: read, never counted bad, nothing drawn here; a garbled one is counted
            { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); dt::GiveUp g; g.t = b.t; g.side = dt::HIGH; g.G = true; g.pull = 900; g.secs = 60; g.x = 2.1; g.normal = 17.8;
              dt::WRead w; w.key = b.t; w.px = 27980; w.side = dt::LOW; w.pulled = 300; w.filled = 40; w.W = true; w.name = "PDL"; w.t0 = 5;
              f << dt::gLineOf(g) << "\n" << dt::wLineOf(w, 0.25) << "\n"; }
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            CHECK(s->bad == bad0 && text(h, "R A H ?") != nullptr && text(h, "G?") == nullptr && text(h, "W?") == nullptr, "(1.7.0) G / W lines read, not counted bad, nothing new on the price chart");
            { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); f << "G|garbage\nW|1|2\n"; }
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            CHECK(s->bad == bad0 + 2, "(1.7.0) garbled G / W lines counted bad"); }
        // (1.5.0, Top Defender #3) an M line where the defenders pressed in (P): 12 bid lots moved from 6997.50 to 6998.00 on the low candle
        {   mvl::Read R; R.t = b.t; R.ext = 0; R.px = 27992; R.tick = 0.25; R.s.in[0] = 12; R.s.bigSize[0] = 12; R.s.bigOld[0] = 27990; R.s.bigNew[0] = 27992; R.s.ticksIn[0] = 2; R.norm[0] = 4; R.norm[1] = 4; R.q[mvl::C_DEF_IN] = true; R.x[0] = 3;
            mvl::Read B2 = R; B2.t = (long long)h.bars[30].time; B2.q[mvl::C_DEF_IN] = false; B2.q[mvl::C_DEF_OUT] = true;   // (1.6.0) stepping back: now boxed too (12 bids 6998.00 -> 6997.00)
            B2.s.out[0] = 12; B2.s.outSize[0] = 12; B2.s.outOld[0] = 27992; B2.s.outNew[0] = 27988; B2.s.ticksOut[0] = 4;
            mvl::Read D = R; D.t = (long long)h.bars[35].time; D.s.in[0] = 9; D.s.bigSize[0] = 9; D.s.bigOld[0] = 27990; D.s.bigNew[0] = 27992;   // (1.6.0) both sides on one candle
            D.s.in[1] = 11; D.s.bigSize[1] = 11; D.s.bigOld[1] = 28000; D.s.bigNew[1] = 27996; D.q[mvl::C_ATT_IN] = true;   // offers pressed DOWN toward price
            mvl::Read N = R; N.t = (long long)h.bars[25].time; N.s.bigOld[0] = 27991; N.s.bigNew[0] = 27992; N.s.bigSize[0] = 7;   // a 1-tick move (5 px here, the box is 10)
            { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); f << mvl::mLineOf(R) << "\n" << mvl::mLineOf(B2) << "\n" << mvl::mLineOf(N) << "\n" << mvl::mLineOf(D) << "\nM|garbage\n"; }
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            if (const char* dp = std::getenv("D3_DUMP")) { std::ofstream o(dp); for (auto& d : h.draws) o << d.type << '\t' << d.x << '\t' << d.y << '\t' << d.r << '\t' << d.b << '\t' << d.color << '\t' << d.border << '\t' << d.fsize << '\t' << d.text << '\n'; }   // render check (not a test)
            CHECK(s->ms.size() == 4 && s->lastBoxes.size() == 5, ("(1.6.0) 4 M lines read (a garbled one skipped), 5 boxes: P, B, the 1-tick P, both sides of one candle (" + std::to_string(s->lastBoxes.size()) + ")").c_str());
            {   cppExtension::PNT pb; pb.set(30, 6997.0f); const mock::Draw* t12b = nullptr; for (auto& d : h.draws) if (d.type == "text" && d.text == "12" && std::abs(d.y - pb.v) <= 8) t12b = &d;
                CHECK(t12b != nullptr, "(1.6.0) stepping back (B) boxed: the solid box with 12 at the new, lower price 6997.00"); }
            if (const char* bp = std::getenv("D3_BOXES")) { std::ofstream o(bp); for (auto& bx : s->lastBoxes) o << bx.side << '\t' << bx.l << '\t' << bx.r << '\t' << bx.ot << '\t' << bx.ob << '\t' << bx.nt << '\t' << bx.nb << '\n';
                for (auto& d : h.draws) if (d.type == "text" && !d.text.empty() && d.text.find_first_not_of("0123456789") == std::string::npos) o << "T\t" << d.x << '\t' << d.y << '\t' << d.text << '\n'; }   // render check (not a test)
            {   std::vector<DlState::BoxRec> c35; cppExtension::PNT pc; pc.set(35, 6998.0f); for (auto& bx : s->lastBoxes) if (bx.l <= pc.h + 30 && bx.r >= pc.h - 30) c35.push_back(bx);
                bool sides = c35.size() == 2 && c35[0].side != c35[1].side;
                bool apart = sides && (c35[0].r < c35[1].l || c35[1].r < c35[0].l);
                CHECK(sides && apart, "(1.6.0) both sides on one candle: a bids box and an offers box, side by side, never overlapping");
                const mock::Draw* t11 = text(h, "11"); CHECK(t11 != nullptr, "(1.6.0) the offers box shows its lots (11)");
                bool red = false; for (auto& d : h.draws) if (d.type == "line" && d.pen == (COLOR)0xEF4444 && d.x > pc.h && d.x <= pc.h + 30) red = true;
                CHECK(red, "(1.6.0) the offers box in the offers colour (the HIGH colour)"); }
            const mock::Draw* t12 = text(h, "12"); const mock::Draw* t7 = text(h, "7");
            CHECK(t12 && t12->color == 0 && t7, "(1.5.0) the solid box shows only the lot size, black text");
            {   cppExtension::PNT pn; pn.set(21, 6998.0f); cppExtension::PNT po; po.set(21, 6997.5f);
                CHECK(t12 && t12->y - 1 <= pn.v && t12->b >= pn.v && t12->x >= po.h - 20 && t12->r <= po.h + 20, "(1.5.0) the lot size sits in the solid box at the new price (6998.00), on the candle");
                bool hollow = false; for (auto& d : h.draws) if (d.type == "line" && d.y == po.v + 5) hollow = true;
                CHECK(hollow, "(1.5.0) the hollow box at the old price (6997.50)"); }
            {   cppExtension::PNT pn; pn.set(25, 6998.0f); int lo = 1 << 30; for (auto& d : h.draws) if (d.type == "line" && d.x >= pn.h - 15 && d.x <= pn.h + 15) lo = std::min(lo, d.y);
                int maxY = -1; for (auto& d : h.draws) if (d.type == "line" && d.x >= pn.h - 15 && d.x <= pn.h + 15) maxY = std::max(maxY, d.y);
                CHECK(t7 && maxY >= pn.v + 15 && lo >= pn.v - 5, "(1.5.0) a 1-tick move: the hollow box sits under the solid one, never on top of it"); }
            CHECK(text(h, "R A H ?") != nullptr, "(1.5.0) the R line label unchanged by the boxes");
            const long long n0 = s->boxesDrawn; s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            CHECK(s->boxesDrawn - n0 == 5 && text(h, "12") != nullptr, "(1.6.0) redrawn the same on the next refresh (no repaint)"); }
    }
    { std::ofstream f(livePath.c_str()); f << liveSecond << "\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->haveLive && text(h, "? R") != nullptr, "live fixture restored before the bar-period switch");
    // Changing the turns or live file must not carry data into an identically named market with another bar period.
    h.seconds = 60;
    { std::ofstream f((flex + "\\BookFlow\\ES-60-turns.csv").c_str()); f << dt::lineOf('T', b, 0.25) << "\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->lines.size() == 1 && s->xs.empty() && !s->haveLive && text(h, "E ?") == nullptr && text(h, "? R") == nullptr, "a new period clears stale exhaustion records and the live cache");
    p->destroy(); p->~DefenseLines(); ::operator delete(p);
    std::printf("DefenseLines host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
