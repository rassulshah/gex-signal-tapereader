// test_defenselines_host.cpp -- lsDefenseLines 1.0.0 on the mock host: reads the turns file written by lsBookFlow, draws the triangle and the
// refill line with its label, stops the line at a close through it (dimmed, "x"), at the next line, at the minutes cap; the live "?" line;
// the settings file switches lines / marks off.
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
int main() {
    char tmpl[] = "/tmp/dl100XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 300; h.paneRight = 2000; h.ppb = 10; h.barW = 20;
    for (int i = 0; i < 40; ++i) { mock::Bar b; b.time = T0 + 180UL * i; b.open = 7000; b.high = 7001; b.low = 6999; b.close = 7000; h.bars.push_back(b); }
    mkdir(flex.c_str(), 0700); mkdir((flex + "\\BookFlow").c_str(), 0700);
    // a high turn at bar 10 (line 7001.0), then candles stay below for 3 bars, the 4th closes above -> broken after 4 bars
    for (int i = 11; i < 14; ++i) h.bars[(size_t)i].close = 7000.5f;
    h.bars[14].close = 7001.5f;
    dt::Turn a; a.t = (long long)h.bars[10].time; a.side = dt::HIGH; a.pivotPx = 28004; a.linePx = 28004; a.marginPct = 23; a.E = true;
    dt::Turn b; b.t = (long long)h.bars[20].time; b.side = dt::LOW; b.pivotPx = 27996; b.linePx = 27996; b.marginPct = 60; b.A = true;
    dt::Turn c; c.t = (long long)h.bars[23].time; c.side = dt::HIGH; c.pivotPx = 28008; c.linePx = 28008; c.marginPct = 5;
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str()); f << dt::lineOf('T', a, 0.25) << "\n" << dt::lineOf('T', b, 0.25) << "\n" << dt::lineOf('T', c, 0.25) << "\nT|garbage\n"; }
    DefenseLines* p = static_cast<DefenseLines*>(CreateExtension()); p->setup();
    h.draws.clear(); p->draw();
    DlState* s = p->stateFor();
    CHECK(s && s->lines.size() == 3 && s->bad == 1, "3 turns read, 1 unreadable line skipped");
    CHECK(text(h, "x R E ?") != nullptr, "the high line is closed through on bar 14: 'x R E ?' (dimmed)");
    CHECK(text(h, "R A ?") != nullptr, "the low line: R A ? (absorption)");
    CHECK(text(h, "R ?") != nullptr, "the third line: R ?");
    {   const mock::Draw* d = text(h, "R A ?"); CHECK(d && d->color == 0x22C55E, "low line label green"); }
    {   const mock::Draw* d = text(h, "x R E ?"); CHECK(d && d->color != 0xEF4444, "broken line dimmed (not the full red)"); }
    {   dt::Life L = dt::lifeOf(dt::LOW, 6999.0, 0, std::vector<double>{7000, 7000, 7000}, std::vector<long long>{180, 360, 540}, 540, 900); CHECK(L.bars == 2, "the low line ends where the next line (bar 23) starts"); }
    const int withMarks = lines(h);
    // settings: marks off -> fewer line strokes (no triangles), labels stay; lines off -> no labels
    { std::ofstream f((flex + "\\DefenseLines.settings-ES.txt").c_str()); ss::Values V = ss::defaults(dlset::spec(), 0); V.v[dlset::K_SHOW_MARKS] = 0; f << ss::format(dlset::spec(), V); }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(lines(h) < withMarks && text(h, "R A ?") != nullptr, "Turn marks off: no triangles, the lines stay");
    { std::ofstream f((flex + "\\DefenseLines.settings-ES.txt").c_str()); ss::Values V = ss::defaults(dlset::spec(), 0); V.v[dlset::K_SHOW_LINES] = 0; f << ss::format(dlset::spec(), V); }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(text(h, "R A ?") == nullptr && lines(h) > 0, "Refill lines off: no lines / labels, the triangles stay");
    { std::ofstream f((flex + "\\DefenseLines.settings-ES.txt").c_str()); f << ss::format(dlset::spec(), ss::defaults(dlset::spec(), 0)); }
    // the live "?" on the forming candle
    dt::Turn lv; lv.t = (long long)h.bars[39].time; lv.side = dt::LOW; lv.pivotPx = 27990; lv.linePx = 27990; lv.marginPct = 12;
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns-live.txt").c_str()); f << dt::lineOf('L', lv, 0.25) << "\n"; }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(text(h, "? R") != nullptr, "the forming candle: dotted '? R' line");
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns-live.txt").c_str()); f << "N\n"; }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(text(h, "? R") == nullptr && text(h, "R A ?") != nullptr, "not confirmed: the ? is gone, the decided lines stay");
    // a shorter (torn) file never removes drawn lines
    { std::ofstream f((flex + "\\BookFlow\\ES-180-turns.csv").c_str()); f << dt::lineOf('T', a, 0.25) << "\n"; }
    s->lastCheckMs = -100000; s->setStore = ss::Store(); h.draws.clear(); p->draw();
    CHECK(s->lines.size() == 3 && text(h, "R A ?") != nullptr, "a torn shorter file: the 3 lines stay (no repaint)");
    std::ifstream st((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string body((std::istreambuf_iterator<char>(st)), std::istreambuf_iterator<char>());
    CHECK(body.find("VERSION,1.0.0") != std::string::npos && body.find("3 turns") != std::string::npos, "status file");
    p->destroy(); p->~DefenseLines(); ::operator delete(p);
    std::printf("DefenseLines host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
