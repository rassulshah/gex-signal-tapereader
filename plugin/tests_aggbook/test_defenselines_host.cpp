// test_defenselines_host.cpp -- lsDefenseLines 1.1.0 on the mock host: reads the turns file written by lsBookFlow, draws the R line from the
// candle it refers to (yellow dot), the gray E line from its X record, at most 3 candles / the next line / a close through it (dimmed, "x");
// the live "?" line; the settings file switches R lines / E lines / dots off; a torn file never removes a line.
#include "DefenseLines.cpp"
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
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
    CHECK(text(h, "x R ?") != nullptr, "the high line is closed through 2 candles later: 'x R ?' (dimmed)");
    {   const mock::Draw* d = text(h, "x R ?"); CHECK(d && d->color != 0xEF4444, "broken line dimmed (not the full red)"); }
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
    CHECK(text(h, "R ?") != nullptr && text(h, "x R ?") == nullptr && text(h, "R ?") != nullptr, "1 candle: the high line ends before the close through");
    settings(flex, -1, 0);
    // the live "?" on the forming candle
    dt::Turn lv; lv.t = (long long)h.bars[39].time; lv.side = dt::LOW; lv.pivotPx = 27990; lv.linePx = 27990; lv.marginPct = 12;
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns-live.txt").c_str()); f << dt::lineOf('L', lv, 0.25) << "\n"; }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(text(h, "? R") != nullptr, "the forming candle: dotted '? R' line");
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns-live.txt").c_str()); f << "N\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(text(h, "? R") == nullptr && text(h, "R A ?") != nullptr, "not confirmed: the ? is gone, the decided lines stay");
    // a shorter (torn) file never removes drawn lines
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str()); f << dt::lineOf('T', a, 0.25) << "\n"; }
    s->lastCheckMs = -100000; h.draws.clear(); p->draw();
    CHECK(s->lines.size() == 3 && s->xs.size() == 1 && text(h, "R A ?") != nullptr && text(h, "E ?") != nullptr, "a torn shorter file: the lines stay (no repaint)");
    std::ifstream st((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string body((std::istreambuf_iterator<char>(st)), std::istreambuf_iterator<char>());
    CHECK(body.find("VERSION,1.1.0") != std::string::npos && body.find("3 turns") != std::string::npos && body.find("exhaustion lines 1") != std::string::npos, "status file");
    p->destroy(); p->~DefenseLines(); ::operator delete(p);
    std::printf("DefenseLines host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
