// test_trails_140.cpp -- lsDefenseLines 1.14.0 (2026-10-28 06:54 CT): the TRAILS are decided + recorded but NOT DRAWN (the order shows at its current price in
// the Dealer Profile liquidity column). Same scenario as test_trails_host.cpp (1.11.0, superseded for drawing).
// was test_trails_host.cpp -- lsDefenseLines 1.11.0 TRAILS on the mock host (2026-10-22, Rassul "ROLLING BIDS / OFFERS DURING THE MOVE"):
// reads lsBookFlow's trail file incrementally, draws the thin stepped line (green bid / red offer), the end marks (dot / x / "B?"),
// at most 2 per side, decides + records each end once, NO REPAINT across a restart (even with other settings), settings checkbox /
// colours real, the hover (question / what / why / use / example) only on a trail.
#include "DefenseLines.cpp"
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const COLOR GRN = 0x22C55E, RED = 0xEF4444;
static const unsigned long T0 = 1791900000UL;
static int yOf(double px) { cppExtension::PNT p; p.set(0, (float)px); return p.v; }
static int xOf(int bar) { cppExtension::PNT p; p.set(bar, 0.0f, cppExtension::kBarCenter); return p.h; }
static int segs(mock::Host& h, COLOR c, bool vertical) { int n = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == c && ((d.x0 == d.x) == vertical) && (d.y0 == d.y) != vertical) ++n; return n; }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static bool hasTextStart(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text.compare(0, t.size(), t) == 0) return true; return false; }
static std::string key(int i) { return std::to_string((long long)(T0 + 180UL * (unsigned long)i)); }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); }
static int countOf(const std::string& s, const std::string& t) { int n = 0; for (size_t a = s.find(t); a != std::string::npos; a = s.find(t, a + 1)) ++n; return n; }
static void settings(const std::string& flex, std::vector<std::pair<int, double> > kv) {
    std::ofstream f((flex + "\\DefenseLines.settings-ES.txt").c_str()); ss::Values V = ss::defaults(dlset::spec(), 0); for (auto& p : kv) V.v[(size_t)p.first] = p.second; f << ss::format(dlset::spec(), V);
}
static void redraw(DefenseLines* p, mock::Host& h) { DlState* s = p->stateFor(); if (s) { s->lastCheckMs = -100000; s->setStore = ss::Store(); s->nChangedMs = -100000; } h.draws.clear(); p->draw(); }
int main() {
    char tmpl[] = "/tmp/trlXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mkdir(flex.c_str(), 0700); mkdir((flex + "\\BookFlow").c_str(), 0700);
    auto trailParams = [&](const char* body) { std::ofstream f((flex + "\\LRA-Trails-ES.csv").c_str()); f << "VERSION,1\nMARKET,ES\n" << body; };   // (1.11.0) the nightly's SHOW / LOG file
    trailParams("PARAM,SHOW,SHOW\nPARAM,SHOW_B,SHOW\n");
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.paneBottom = 600; h.paneRight = 2000; h.ppb = 10; h.barW = 20;
    // a rally: each candle 1 point range, up 0.75 a candle (ATR ~ 1.75 pts = 7 ticks; 0.5 ATR ~ 3.5 ticks)
    for (int i = 0; i < 40; ++i) { mock::Bar b; b.time = T0 + 180UL * i; b.low = 7000 + 0.75f * i; b.high = b.low + 1.0f; b.open = b.low + 0.25f; b.close = b.high - 0.25f; h.bars.push_back(b); }
    const std::string tp = flex + "\\BookFlow\\ES-180-trails.csv", rp = flex + "\\BookFlow\\ES-180-trails-rec.csv";
    // bid 7 (40 lots): steps in candles 5, 6, 7 under the rally, then stops (price runs on) -> B? at candle 9 (ran in 8, N = 2)
    // offer 8 (30 lots): steps down in candles 10, 11 then pulled in 12
    // bid 9: steps in 20, 21, traded in 22
    {   std::ofstream f(tp.c_str());
        f << "RS|0|0.25\n";
        f << "TS|7|0|40|" << key(5) << "|1000|28010\n" << "TP|7|" << key(5) << "|1000|28012|0\n" << "TP|7|" << key(6) << "|2000|28015|0\n" << "TP|7|" << key(7) << "|3000|28018|0\n";
        f << "TS|8|1|30|" << key(10) << "|5000|28080\n" << "TP|8|" << key(10) << "|5000|28078|0\n" << "TP|8|" << key(11) << "|6000|28076|0\n" << "TX|8|" << key(12) << "|7000|28076\n";
        f << "TS|9|0|60|" << key(20) << "|9000|28050\n" << "TP|9|" << key(20) << "|9000|28052|0\n" << "TP|9|" << key(21) << "|9500|28054|0\n" << "TF|9|" << key(22) << "|9900|28054|60\n";
        f << "TP|half"; }   // a half-written line at the end: not read yet
    DefenseLines* p = static_cast<DefenseLines*>(CreateExtension()); p->setup();
    redraw(p, h); DlState* s = p->stateFor();
    CHECK(s && s->trails.v.size() == 3 && s->trails.bad == 0, "3 trails read");
    CHECK(s->trails.v[0].end == 'B' && s->trails.v[1].end == 'X' && s->trails.v[2].end == 'F', "ends still decided: B? / pulled / traded");
    CHECK(s->trailsDrawn == 0 && segs(h, GRN, true) == 0 && segs(h, GRN, false) == 0 && segs(h, RED, true) == 0 && text(h, "B?") == nullptr, "nothing drawn: no stepped lines, no B?");
    const std::string rec1 = slurp(rp);
    CHECK(countOf(rec1, "TR|1|") == 3, "each end recorded once for the nightly");
    {   std::ifstream st((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string b((std::istreambuf_iterator<char>(st)), std::istreambuf_iterator<char>());
        CHECK(b.find("VERSION,1.14.0") != std::string::npos && b.find("TRAILS,not drawn") != std::string::npos, "status: 1.14.0, trails not drawn"); }
    std::printf("lsDefenseLines 1.14.0 trails (not drawn): %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
