// SUPERSEDED for drawing by test_trails_140.cpp (lsDefenseLines 1.14.0 does not draw trails) - kept for history, its draw checks fail by design
// test_trails_host.cpp -- lsDefenseLines 1.11.0 TRAILS on the mock host (2026-10-22, Rassul "ROLLING BIDS / OFFERS DURING THE MOVE"):
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
    CHECK(s && s->trails.v.size() == 3 && s->trails.bad == 0, "3 trails read, the half line left for later");
    CHECK(s->trails.v[0].end == 'B' && s->trails.v[0].endKey == (long long)h.bars[9].time, "bid 7: B? at candle 9 (price ran 0.5 ATR in 8, no step in 8-9)");
    CHECK(s->trails.v[1].end == 'X' && s->trails.v[2].end == 'F', "offer 8 pulled, bid 9 traded");
    CHECK(s->trailsDrawn == 3, "3 trails drawn");
    CHECK(segs(h, GRN, true) >= 5 && segs(h, GRN, false) >= 5 && segs(h, RED, true) >= 2, "stepped lines: vertical steps + horizontal runs, green bids, red offer");
    {   bool thin = true; for (auto& d : h.draws) if (d.type == "line" && (d.pen == GRN || d.pen == RED)) { if (d.penW != 1 && std::getenv("TR_DBG")) std::printf("w%d %d,%d-%d,%d\n", d.penW, d.x0, d.y0, d.x, d.y); thin &= d.penW <= 1; } CHECK(thin, "thin lines (setPen width 1; the mock records 0 for a line)"); }
    const mock::Draw* bq = text(h, "B?");
    CHECK(bq && bq->color == GRN && std::abs(bq->x - xOf(9)) <= 12, "B? in the bid colour at candle 9");
    {   bool horizAt = false; for (auto& d : h.draws) if (d.type == "line" && d.pen == GRN && d.y0 == d.y && d.y == yOf(28018 * 0.25) && std::min(d.x0, d.x) <= xOf(7) && std::max(d.x0, d.x) >= xOf(9)) horizAt = true;
        CHECK(horizAt, "the last price runs from its step (candle 7) to the B? candle (9)"); }
    {   bool beyond = false; for (auto& d : h.draws) if (d.type == "line" && d.pen == GRN && d.y == yOf(28018 * 0.25) && std::max(d.x0, d.x) > xOf(9)) beyond = true; CHECK(!beyond, "the trail ends once at B? (nothing after candle 9)"); }
    {   int diag = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == RED && d.x0 != d.x && d.y0 != d.y) ++diag; CHECK(diag == 2, "pulled offer: an x (2 strokes)"); }
    {   int dot = 0; for (auto& d : h.draws) if (d.type == "line" && d.pen == GRN && d.y0 == d.y && std::abs(d.y - yOf(28054 * 0.25)) <= 3 && std::abs(d.x0 - xOf(22)) <= 4 && std::abs(d.x - xOf(22)) <= 5) ++dot; CHECK(dot >= 7, "traded bid: a filled dot at candle 22"); }
    const std::string rec1 = slurp(rp);
    CHECK(countOf(rec1, "TR|1|") == 3 && countOf(rec1, "|B|") == 1 && countOf(rec1, "|X|") == 1 && countOf(rec1, "|F|") == 1, "each end recorded once for the nightly");
    redraw(p, h); CHECK(countOf(slurp(rp), "TR|1|") == 3, "a redraw never records twice");
    {   std::ifstream st((flex + "\\DefenseLines.status-ES.txt").c_str()); std::string b((std::istreambuf_iterator<char>(st)), std::istreambuf_iterator<char>());
        CHECK(b.find("VERSION,1.13.0") != std::string::npos && b.find("TRAILS,on,3 trails") != std::string::npos && b.find("recorded 3") != std::string::npos, "status line: version + trails"); }
    // incremental read: the half line completes, a 4th trail (bid 11) starts and is still following (no end mark, runs to the last candle)
    {   std::ofstream f(tp.c_str(), std::ios::app); f << "|9|1|1|0\n"; f << "TS|11|0|50|" << key(36) << "|20000|28140\n" << "TP|11|" << key(36) << "|20000|28142|0\n" << "TP|11|" << key(37) << "|21000|28145|0\n"; }
    redraw(p, h);
    CHECK(s->trails.v.size() == 4 && s->trails.bad == 1 && s->trails.v[3].end == 0, "appended lines read from where the last read stopped (the joined half line is malformed, counted)");
    CHECK(s->trailsDrawn == 3, "at most 2 bids on screen (bid 11 + bid 9) + the offer");
    {   bool bid7 = false; for (auto& d : h.draws) if (d.type == "line" && d.pen == GRN && d.y == yOf(28018 * 0.25) && d.y0 == d.y) bid7 = true; CHECK(!bid7, "the oldest bid (7) is not drawn: 2 per side"); }
    {   bool toEnd = false; for (auto& d : h.draws) if (d.type == "line" && d.pen == GRN && d.y == yOf(28145 * 0.25) && d.y0 == d.y && std::max(d.x0, d.x) >= xOf(39)) toEnd = true; CHECK(toEnd, "an open trail runs to the last candle"); }
    // the hover: a mouse move onto bid 11's last run
    {   hb::fakeCursor() = 77; RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)xOf(38); e.v.mouse.v = (short)yOf(28145 * 0.25);
        const int inv0 = h.invalidates; p->mouse(&e); CHECK(h.invalidates == inv0 + 1 && s->hovKey == "3", "hovering a trail asks for one redraw");
        redraw(p, h);
        CHECK(hasTextStart(h, "Trailing BID 50 lots") && hasTextStart(h, "Question:") && hasTextStart(h, "Use: a bid that stops following under a rally = buyers quitting") && hasTextStart(h, "Example:") && hasTextStart(h, "? = not proven yet") && hasTextStart(h, "Still following"), "hover rows: numbers, question, use, example, '?', open trail");
        e.v.mouse.h = 5; e.v.mouse.v = 5; p->mouse(&e); redraw(p, h); CHECK(!hasTextStart(h, "Trailing"), "off a trail: no hover");
        hb::fakeCursor() = 0; }
    // settings: colours applied, the checkbox removes every trail drawing (records still kept)
    settings(flex, { { dlset::K_COL_TRAIL_BID, 0x3B82F6 } }); redraw(p, h);
    CHECK(segs(h, (COLOR)0x3B82F6, true) >= 2 && segs(h, GRN, true) == 0, "bid colour setting applied");
    settings(flex, { { dlset::K_SHOW_TRAILS, 0 } }); redraw(p, h);
    CHECK(s->trailsDrawn == 0 && text(h, "B?") == nullptr && segs(h, RED, true) == 0 && countOf(slurp(rp), "TR|1|") == 3, "Trails unchecked: nothing drawn, the records stay");
    // (1.11.0) the nightly's verdict LOG ONLY (default in every market): nothing drawn with the checkbox on, records kept; SHOW_B LOG = lines without B?
    settings(flex, {}); trailParams("# log only\nPARAM,SHOW,0\nPARAM,SHOW_B,0\n"); usleep(3200000); redraw(p, h);
    CHECK(s->trailsDrawn == 0 && text(h, "B?") == nullptr && segs(h, RED, true) == 0 && segs(h, GRN, true) == 0 && countOf(slurp(rp), "TR|1|") == 3, "LRA-Trails SHOW 0: LOG ONLY - nothing drawn, the records stay");
    trailParams("PARAM,SHOW,SHOW\nPARAM,SHOW_B,LOG\n"); usleep(3200000); redraw(p, h);
    CHECK(s->trailsDrawn > 0 && text(h, "B?") == nullptr && segs(h, GRN, true) >= 2, "SHOW + SHOW_B LOG: the lines without B?");
    { struct stat st0; const bool none = stat((flex + "\\LRA-Trails-GC.csv").c_str(), &st0) != 0; CHECK(none && mp::mode("Trails", "GC", "SHOW", mp::LOG) == mp::LOG, "no file = LOG (the default)"); }
    trailParams("PARAM,SHOW,SHOW\nPARAM,SHOW_B,SHOW\n# back on for the restart test\n"); usleep(3200000);
    // NO REPAINT across a restart: a new instance with N = 5 and gap 2.0 ATR still shows B? at candle 9 and writes nothing new
    std::vector<std::string> before; settings(flex, {}); redraw(p, h); for (auto& d : h.draws) if (d.type == "text" || (d.type == "line" && (d.pen == GRN || d.pen == RED))) before.push_back(d.type + std::to_string(d.x0) + "," + std::to_string(d.y0) + "-" + std::to_string(d.x) + "," + std::to_string(d.y) + d.text);
    p->destroy(); p->~DefenseLines(); ::operator delete(p);
    {   DefenseLines* q = static_cast<DefenseLines*>(CreateExtension()); q->setup(); redraw(q, h);
        std::vector<std::string> after; for (auto& d : h.draws) if (d.type == "text" || (d.type == "line" && (d.pen == GRN || d.pen == RED))) after.push_back(d.type + std::to_string(d.x0) + "," + std::to_string(d.y0) + "-" + std::to_string(d.x) + "," + std::to_string(d.y) + d.text);
        CHECK(after == before && !before.empty(), "a restart redraws exactly the same trails (byte-identical draw list)");
        settings(flex, { { dlset::K_TRAIL_N, 5 }, { dlset::K_TRAIL_GAP, 2.0 } }); redraw(q, h);
        DlState* s2 = q->stateFor();
        // bid 7 is off screen now (2 per side) but its decision must be the recorded one
        CHECK(s2->trails.v[0].end == 'B' && s2->trails.v[0].endKey == (long long)h.bars[9].time && s2->trails.v[0].fromRec, "recorded B? kept with other settings (never repainted)");
        CHECK(countOf(slurp(rp), "TR|1|") == 3, "no duplicate records after the restart");
        // bid 11 with N = 5 / gap 2.0: decided on its own rule later; the forming candle never decides
        CHECK(s2->trails.v[3].end == 0, "the open trail stays open (forming candle never decides)");
        q->destroy(); q->~DefenseLines(); ::operator delete(q);
    }
    // bar-by-bar replay: growing the chart one candle at a time and restarting after every candle = one straight pass
    {   std::remove(rp.c_str()); std::vector<mock::Bar> all = h.bars; std::string straight, stepwise;
        {   DefenseLines* q = static_cast<DefenseLines*>(CreateExtension()); q->setup(); settings(flex, {}); redraw(q, h); straight = slurp(rp); q->destroy(); q->~DefenseLines(); ::operator delete(q); }
        std::remove(rp.c_str());
        for (int n = 6; n <= 40; ++n) { h.bars.assign(all.begin(), all.begin() + n); DefenseLines* q = static_cast<DefenseLines*>(CreateExtension()); q->setup(); redraw(q, h); q->destroy(); q->~DefenseLines(); ::operator delete(q); }
        stepwise = slurp(rp);
        auto sorted = [](const std::string& x) { std::vector<std::string> v; std::stringstream ss(x); std::string l; while (std::getline(ss, l)) v.push_back(l); std::sort(v.begin(), v.end()); return v; };
        CHECK(sorted(straight) == sorted(stepwise) && !straight.empty(), "candle-by-candle with a restart after every candle = one straight pass (same records)");
        h.bars = all;
    }
    {   // A BookFlow trail end must bind to its recorded chart key.  An RTH/session chart can omit the
        // source candle; mapping that immutable F/X/B end to the first later bar is a false timestamp.
        char tmpl3[] = "/tmp/trlgapXXXXXX"; const char* tmp3 = mkdtemp(tmpl3); setenv("USERPROFILE", tmp3, 1);
        const std::string flex3 = std::string(tmp3) + "\\InvestorRT\\rtx\\lsFlexLevels";
        mkdir(flex3.c_str(), 0700); mkdir((flex3 + "\\BookFlow").c_str(), 0700);
        { std::ofstream f((flex3 + "\\LRA-Trails-GC.csv").c_str()); f << "VERSION,1\nMARKET,GC\nPARAM,SHOW,SHOW\nPARAM,SHOW_B,SHOW\n"; }
        mock::Host h3; mock::current = &h3; h3.tick = 0.25f; h3.seconds = 180; h3.symbol = "GCZ6"; h3.root = "GC"; h3.paneBottom = 600; h3.paneRight = 2000; h3.ppb = 10; h3.barW = 20;
        for (int i = 0; i < 20; ++i) { if (i == 9) continue; mock::Bar b; b.time = T0 + 180UL * (unsigned long)i; b.low = 7000; b.high = 7001; b.open = 7000.25f; b.close = 7000.75f; h3.bars.push_back(b); }
        const std::string tp3 = flex3 + "\\BookFlow\\GC-180-trails.csv";
        { std::ofstream f(tp3.c_str());
          f << "RS|0|0.25\n" << "TS|31|0|40|" << key(5) << "|1000|28000\n" << "TP|31|" << key(5) << "|1000|28002|0\n" << "TP|31|" << key(6) << "|2000|28004|0\n" << "TF|31|" << key(9) << "|3000|28004|40\n"; }
        DefenseLines* q = static_cast<DefenseLines*>(CreateExtension()); q->setup(); q->draw(); DlState* s3 = q->stateFor();
        CHECK(s3 && s3->trails.v.size() == 1 && s3->trails.v[0].end == 'F' && s3->trailsDrawn == 0, "a final trail end on a missing chart candle is not shifted to the next bar");
        q->destroy(); q->~DefenseLines(); ::operator delete(q);
    }
    std::printf("Trails host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
