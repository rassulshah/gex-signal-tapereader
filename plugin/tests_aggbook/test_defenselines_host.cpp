// test_defenselines_host.cpp -- lsDefenseLines 1.9.0 (no text on the lines) / 1.1.0 on the mock host: reads the turns file written by lsBookFlow, draws the R line from the
// candle it refers to (yellow dot), the gray E line from its X record, at most 3 candles / the next line / a close through it (dimmed, "x");
// the live "?" line; the settings file switches R lines / E lines / dots off; a torn file never removes a line.
#include "DefenseLines.cpp"
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <utime.h>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
static std::string slurpSrc(const std::string& f) { for (const char* d : { "", "../", "../../" }) { std::ifstream in((std::string(d) + f).c_str()); if (in) return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()); } return std::string(); }
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static int lines(mock::Host& h) { int n = 0; for (auto& d : h.draws) n += d.type == "line"; return n; }
// (1.9.0) the lines carry no text: lines are found by their pen colour at the price's y (the mock records a line's END point)
static int yOf(int bar, double px) { cppExtension::PNT p; p.set(bar, (float)px); return p.v; }
static int lineCount(mock::Host& h, COLOR c, int y) { int n = 0; for (auto& d : h.draws) n += d.type == "line" && d.pen == c && d.y == y; return n; }
static int texts(mock::Host& h) { int n = 0; for (auto& d : h.draws) n += d.type == "text"; return n; }
static bool digitsOnly(mock::Host& h) { for (auto& d : h.draws) if (d.type == "text" && (d.text.empty() || d.text.find_first_not_of("0123456789") != std::string::npos)) return false; return true; }
static const mock::Draw* lastLine(mock::Host& h, COLOR c, int y) { const mock::Draw* r = nullptr; for (auto& d : h.draws) if (d.type == "line" && d.pen == c && d.y == y && (!r || d.x > r->x)) r = &d; return r; }
static const mock::Draw* lineAt(mock::Host& h, COLOR c, int y, int x) { for (auto& d : h.draws) if (d.type == "line" && d.pen == c && d.y == y && d.x == x) return &d; return nullptr; }
static const COLOR GRN = 0x22C55E, RED = 0xEF4444, GRY = 0xA3A3A3;
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
    const int yHi = yOf(0, 7001.0), yR = yOf(0, 6998.25), yE = yOf(0, 6998.0);
    CHECK(lineCount(h, RED, yHi) == 1, "(1.2.0) the high line closed through 2 candles later is removed; only the third (holding) high line is drawn in red");
    CHECK(lineCount(h, GRN, yR) == 1, "the low turn's refill line: green at 6998.25");
    const int rEnd = lastLine(h, GRN, yR) ? lastLine(h, GRN, yR)->x : -1;
    CHECK(lineCount(h, GRY, yE) == 0 && s->xs.size() == 1, "(1.13.0, Rassul 2026-10-26 17:55 CT) the gray exhaustion line is NOT drawn; its X record is still read");
    CHECK(texts(h) == 0, "(1.9.0) NO text on any line: no 'R A ?', no 'E ?', no 'R ?' (draw-call check)");
    CHECK(text(h, "R A ?") == nullptr && text(h, "E ?") == nullptr && text(h, "R ?") == nullptr && text(h, "E") == nullptr && text(h, "?") == nullptr, "(1.9.0) none of the old labels");
    const int all = lines(h);
    settings(flex, dlset::K_SHOW_DOTS, 0); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(lines(h) < all && lineCount(h, GRN, yR) == 1 && texts(h) == 0, "dots off: fewer strokes, the lines stay");
    { int eKeys = 0; const ss::Spec& SP = dlset::spec(); for (int k = 0; k < SP.n; ++k) if (std::string(SP.rows[k].key) == "show_e" || std::string(SP.rows[k].key) == "col_e") ++eKeys;
      CHECK(eKeys == 0 && SP.paramVersion == 5, "(1.13.0) no E settings rows, parameter version 5"); }
    settings(flex, dlset::K_SHOW_LINES, 0); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(lineCount(h, GRN, yR) == 0 && lineCount(h, GRY, yE) == 0, "R lines off: no R line (and no E line: retired 1.13.0)");
    settings(flex, dlset::K_MAX_CANDLES, 1); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(lineCount(h, RED, yHi) == 2 && texts(h) == 0, "1 candle: the high line ends before the close through, so it stays (2 red lines, no text)");
    settings(flex, -1, 0);
    {   // (1.12.0, BUILD-SPEC 22:29) thinner line (1 px), and NO yellow dot on a candle where lsLiquidityProfile drew an order fill dot
        const COLOR YEL = (COLOR)0xFACC15;
        auto yel = [&]() { int n = 0; for (auto& d : h.draws) n += d.type == "line" && d.pen == YEL && std::abs(d.y - yR) <= 3; return n; };
        auto rW = [&]() { int w = -1; for (auto& d : h.draws) if (d.type == "line" && d.pen == GRN && d.y == yR) w = d.penW; return w; };
        s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
        const int y0 = yel();
        CHECK(y0 > 0 && rW() == 1, "(1.12.0) the R line is 1 px (was 2) and its yellow dot is drawn (no fill file)");
        const std::string fills = flex + "\\LRA-OrderFills-ES-180.txt";
        { std::ofstream f(fills.c_str()); f << "VERSION,0.55.1\nF," << h.bars[20].time << "\nF,garbage\n"; }
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        CHECK(yel() == 0 && s->fillKeys.size() == 1 && s->dotsSkipped >= 1 && lineCount(h, GRN, yR) == 1, "(1.12.0) an order fill dot on the pivot candle: the yellow dot is skipped, the line stays");
        { std::ifstream st0((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string b0((std::istreambuf_iterator<char>(st0)), std::istreambuf_iterator<char>());
          CHECK(b0.find("FILL_DOTS,") != std::string::npos && b0.find("1 order-fill candles,yellow dots skipped ") != std::string::npos, "(1.12.0) status FILL_DOTS"); }
        settings(flex, dlset::K_DOT_SKIP_FILL, 0); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
        CHECK(yel() == y0, "(1.12.0) 'No dot where an order fill dot is' unticked: the yellow dot is back");
        settings(flex, dlset::K_LINE_W, 3); s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
        CHECK(rW() == 3, "(1.12.0) Line width 3: the R line is 3 px");
        settings(flex, -1, 0); std::remove(fills.c_str());
        s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
        CHECK(yel() == y0 && s->fillKeys.empty(), "(1.12.0) the fill file gone: every dot drawn again");
        CHECK(slurpSrc("DefenseLines.cpp").find("P_DOT") == std::string::npos, "(1.12.0) no dotted / dashed pen anywhere (his rule)");
    }
    // the live "?" on the forming candle
    dt::Turn lv; lv.t = (long long)h.bars[39].time; lv.side = dt::LOW; lv.pivotPx = 27990; lv.linePx = 27990; lv.marginPct = 12;
    const std::string livePath = flex + "\\BookFlow\\ES-180-turns-live.txt";
    lv.linePx = 27989; const std::string liveFirst = dt::lineOf('L', lv, 0.25); lv.linePx = 27991; const std::string liveSecond = dt::lineOf('L', lv, 0.25);
    CHECK(liveFirst.size() == liveSecond.size(), "same-size live fixture");
    { std::ofstream f(livePath.c_str()); f << liveFirst << "\n"; }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    const int liveEnd = lastLine(h, GRN, yOf(0, 6997.25)) ? lastLine(h, GRN, yOf(0, 6997.25))->x : -1;
    CHECK(lineCount(h, GRN, yOf(0, 6997.25)) == 1 && texts(h) == 0 && s->haveLive && s->liveLine.linePx == 6997.25, "the forming candle: a solid 1 px line (1.12.0), no '? R' text (1.9.0)");
    struct stat liveStat; CHECK(stat(livePath.c_str(), &liveStat) == 0, "live fixture stat");
    { std::ofstream f(livePath.c_str()); f << liveSecond << "\n"; }
    struct utimbuf sameLiveTime; sameLiveTime.actime = liveStat.st_atime; sameLiveTime.modtime = liveStat.st_mtime;
    CHECK(utime(livePath.c_str(), &sameLiveTime) == 0, "restore live fixture mtime");
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->haveLive && s->liveLine.linePx == 6997.75, "same-size, same-mtime live update replaces the cached '?' line");
    CHECK(unlink(livePath.c_str()) == 0, "remove live fixture");
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(!s->haveLive && lineAt(h, GRN, yOf(0, 6997.75), liveEnd) == nullptr, "a removed live file clears the cached forming line");
    { std::ofstream f(livePath.c_str()); f << "N\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(lineAt(h, GRN, yOf(0, 6997.75), liveEnd) == nullptr && lineCount(h, GRN, yR) == 1, "not confirmed: the forming line is gone, the decided lines stay");
    // a shorter (torn) file never removes drawn lines
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str()); f << dt::lineOf('T', a, 0.25) << "\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->lines.size() == 3 && s->xs.size() == 1 && lineCount(h, GRN, yR) == 1 && lineCount(h, GRY, yE) == 0, "a torn shorter file: the lines stay (no repaint)");
    std::ifstream st((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string body((std::istreambuf_iterator<char>(st)), std::istreambuf_iterator<char>());
    CHECK(body.find("failed lines removed") != std::string::npos, "(1.2.0) the status counts the removed lines");
    CHECK(body.find("LABELS,none") != std::string::npos, "(1.9.0) status: LABELS,none");
    CHECK(body.find(std::string("VERSION,") + DL_VERSION) != std::string::npos && std::string(DL_VERSION) == "1.13.0" && std::string(dlset::PLUGIN_VERSION) == DL_VERSION && body.find("3 turns") != std::string::npos && body.find("exhaustion lines 1") != std::string::npos, "status file");
    {   // (1.3.0, Top Defender #5) lsBookFlow decided the defenders hit back after the A: the label reads "R A H ?"; "absorbed only" keeps "R A ?"
        { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); dt::Counter C; C.t = b.t; C.side = b.side; C.H = true; C.lots = 640; C.attack = 266; C.at = b.t + 360; f << dt::hLineOf(C) << "\n"; }
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        CHECK(s->hs.size() == 1 && lineCount(h, GRN, yR) == 1 && texts(h) == 0, "(1.3.0) H decided and read; (1.9.0) the line stays clean (no 'R A H ?')");
        { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); f << "H|garbage\n"; }
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        CHECK(lineCount(h, GRN, yR) == 1 && s->hs.size() == 1, "(1.3.0) a garbled H line is ignored, the decided H stays (no repaint)");
        const long long bad0 = s->bad;
        { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); dt::Retest q; q.t = b.t; q.side = b.side; q.res = 'L'; q.ratio = 1.4; q.rest = 700; q.at = b.t + 720; f << dt::qLineOf(q) << "\n"; }
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        CHECK(s->bad == bad0 && lineCount(h, GRN, yR) == 1 && texts(h) == 0, "(1.4.0) a Q line (re-test L / X) is read, not counted bad; no text (L / X in the pane)");
        {   // (1.7.0) lsBookFlow 1.19.0's G and W lines: read, never counted bad, nothing drawn here; a garbled one is counted
            { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str(), std::ios::app); dt::GiveUp g; g.t = b.t; g.side = dt::HIGH; g.G = true; g.pull = 900; g.secs = 60; g.x = 2.1; g.normal = 17.8;
              dt::WRead w; w.key = b.t; w.px = 27980; w.side = dt::LOW; w.pulled = 300; w.filled = 40; w.W = true; w.name = "PDL"; w.t0 = 5;
              f << dt::gLineOf(g) << "\n" << dt::wLineOf(w, 0.25) << "\n"; }
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            CHECK(s->bad == bad0 && lineCount(h, GRN, yR) == 1 && texts(h) == 0, "(1.7.0) G / W lines read, not counted bad, nothing new on the price chart");
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
            if (const char* dp = std::getenv("D3_DUMP")) { std::ofstream o(dp); for (auto& d : h.draws) o << d.type << '\t' << d.x << '\t' << d.y << '\t' << d.r << '\t' << d.b << '\t' << d.color << '\t' << d.border << '\t' << d.fsize << '\t' << d.text << '\t' << d.x0 << '\t' << d.y0 << '\t' << d.pen << '\n'; }   // render check (not a test)
            CHECK(s->ms.size() == 4 && s->lastBoxes.empty() && s->boxesDrawn == 0, ("(1.13.0, Rassul 2026-10-26 17:59 CT) 4 M lines read (a garbled one skipped), NO move boxes drawn (" + std::to_string(s->lastBoxes.size()) + ")").c_str());
            CHECK(text(h, "12") == nullptr && text(h, "11") == nullptr && text(h, "7") == nullptr && text(h, "9") == nullptr, "(1.13.0) no lot-size text of a pressed-in / stepped-back box");
            CHECK(lineAt(h, GRN, yR, rEnd) != nullptr && texts(h) == 0 && digitsOnly(h), "(1.13.0) the R line unchanged; no text on the price chart"); }
    }
    { std::ofstream f(livePath.c_str()); f << liveSecond << "\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->haveLive && lineAt(h, GRN, yOf(0, 6997.75), liveEnd) != nullptr, "live fixture restored before the bar-period switch");
    {   // (1.8.0, Rassul 16:05 CT 2026-10-21 "fix this, it should be solid") the lsHodLod table on this chart: nothing painted over it
        s->lastCheckMs = -100000; h.draws.clear(); p->draw();
        const mock::Draw* e0p = lastLine(h, GRN, yR); const mock::Draw* r0 = lastLine(h, GRN, yR);   // (1.13.0) the R line is the reference (E retired)
        CHECK(e0p && r0, "(1.8.0) the R line drawn with no table");
        const mock::Draw e0c = e0p ? *e0p : mock::Draw("none", 0, 0); const mock::Draw* e0 = e0p ? &e0c : nullptr;
        if (e0 && r0) {
            const int el = e0->x - 12, et = e0->y - 20, er = e0->x + 5, eb = e0->y + 2;   // (1.9.0) a table over the E line's right end (no label any more)
            const bdk::Box T = bdk::mk(el, et, er, eb);
            const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            bdk::publishHodLod(h.symbol, h.seconds, h.chart, ms, T);
            const size_t xs0 = s->xs.size(), lines0 = s->lines.size(); const int strokes0 = lines(h);
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            bool inside = false; for (auto& d : h.draws) if (bdk::hitsTable(T, d.x, d.y, d.type == "text" || d.type == "box" ? d.r : d.x, d.type == "text" || d.type == "box" ? d.b : d.y)) inside = true;
            CHECK(!inside, "(1.8.0) nothing drawn inside the table (+2 px): labels, line ends, dots");
            if (std::getenv("DL_DBG")) { std::printf("table %d..%d y %d..%d e0 %d,%d\n", el, er, et, eb, e0->x, e0->y); for (auto& d : h.draws) if (d.pen == GRY) std::printf("  gry %s %d %d\n", d.type.c_str(), d.x, d.y); }
            { const mock::Draw* ec = nullptr; for (auto& d : h.draws) if (d.type == "line" && d.pen == GRN && std::abs(d.y - yR) <= 1) ec = &d; CHECK(ec && ec->x < el && s->xs.size() == xs0 && s->lines.size() == lines0, "(1.8.0) the R line is cut at the table's edge, the records are still kept"); }
            CHECK(lines(h) > 0 && lines(h) <= strokes0 + 4, "(1.8.0) lines are cut at the table, not dropped");
            std::ifstream st2((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string b2((std::istreambuf_iterator<char>(st2)), std::istreambuf_iterator<char>());
            CHECK(b2.find("UNDER_HODLOD,table on") != std::string::npos, "(1.8.0) status: UNDER_HODLOD line");
            const std::string chart0 = h.chart; h.chart = "another chart";
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            { const mock::Draw* ec = lastLine(h, GRN, yR); CHECK(ec && ec->x == e0->x, "(1.8.0) a table on ANOTHER chart: no effect"); }
            h.chart = chart0; const std::string sym0 = h.symbol; h.symbol = "NQZ6";
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            { const mock::Draw* ec = lastLine(h, GRN, yR); CHECK(ec && ec->x == e0->x, "(1.8.0) a table for another symbol: no effect"); }
            h.symbol = sym0;
            bdk::publishHodLod(h.symbol, h.seconds, h.chart, ms - 600000, T);   // lsHodLod stopped drawing 10 minutes ago (no clear: a fault)
            for (int k = 0; k < 4; ++k) { s->lastCheckMs = -100000; h.draws.clear(); p->draw(); }
            { const mock::Draw* ec = lastLine(h, GRN, yR); CHECK(ec && ec->x == e0->x, "(1.8.0) a stale table (stamp 10 min old, unchanged for 4 draws): no effect"); }
            bdk::clearHodLod(h.symbol, h.seconds, h.chart);
            s->lastCheckMs = -100000; h.draws.clear(); p->draw();
            { const mock::Draw* ec = lastLine(h, GRN, yR); CHECK(ec && ec->x == e0->x, "(1.8.0) table cleared: drawn again"); }
        }
    }
    // Changing the turns or live file must not carry data into an identically named market with another bar period.
    h.seconds = 60;
    { std::ofstream f((flex + "\\BookFlow\\ES-60-turns.csv").c_str()); f << dt::lineOf('T', b, 0.25) << "\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    if (std::getenv("DL_DBG")) std::printf("np %zu %zu %d %d %d %d\n", s->lines.size(), s->xs.size(), (int)s->haveLive, lineCount(h, GRY, yE), (int)(lineAt(h, GRN, yOf(0, 6997.75), liveEnd) != nullptr), texts(h));
    CHECK(s->lines.size() == 1 && s->xs.empty() && !s->haveLive && lineCount(h, GRY, yE) == 0 && lineAt(h, GRN, yOf(0, 6997.75), liveEnd) == nullptr && texts(h) == 0 && s->ms.empty() && s->hs.empty(), "a new period clears stale exhaustion records and the live cache; (1.9.0) and the old period's move boxes / H (5 lot boxes leaked before)");
    p->destroy(); p->~DefenseLines(); ::operator delete(p);
    std::printf("DefenseLines host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
