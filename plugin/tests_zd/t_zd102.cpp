// t_zd102.cpp -- lsZoneDefense 1.0.2 / 1.0.3: the independent review's fixes, each with its own test (mock host + the engine directly).
//   markets (mkey::norm), one writer per journal, restart mid-reload (ZE 5 + "not decided"), torn last line, session roll (book kept,
//   no warm-up), contract roll / reconnect (reset), missed DEL (ghost order dropped), implied fill ignored, a price-moving MOD = a cancel
//   at the old price, first-second events, periodOf with the real CDT / CST offsets, the same-distance queue normal (not 0%), bar-end
//   mapping of the marks (strict), the GC 2026-10-26 08:57 sweep regression (lift 1.07x, back above the zone by the candle close = swept).
// Build (tests_zd/): g++ -std=c++17 -O2 -Wall -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock t_zd102.cpp -o t102 && ./t102 fx
#include "ZoneDefense.cpp"
#include <set>
#include <unistd.h>
typedef ZoneDefense Ind;
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
static long long& NOW = off::off_test_now_ms;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
#pragma pack(push, 1)
struct FR { int t; signed char a, s; int px; unsigned sz; int oid; };
#pragma pack(pop)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); return p; }
static mock::Bar bar(unsigned long t, float px) { mock::Bar b; b.time = t; b.close = b.open = b.low = b.high = px; b.volume = 100; b.rows.push_back(mock::Price(px, 50, 50, 100)); return b; }
static zd::In ev(long long ts, int a, int s, long long px, long long sz, long long oid) { zd::In e; e.t = ts * 1000; e.act = a; e.side = s; e.px = px; e.sz = sz; e.oid = oid; return e; }
static std::string flexOf() { return zdh::flexDir(); }
static void freshHome() { char tmpl[] = "/tmp/zd102XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1); }
// a pane on host h, ES-like ticks, bars from t0
static Ind* pane(mock::Host& h, unsigned long t0, const char* root = "ES", const char* sym = "ESZ6") {
    h.tick = 0.25f; h.seconds = 180; h.root = root; h.symbol = sym; h.paneBottom = 160; h.paneRight = 900; h.scaleLeft = 880; h.barW = 12;
    for (int k = -40; k <= 0; ++k) h.bars.push_back(bar(t0 + 180UL * k, 7000.0f));
    Ind* p = make(); NOW += 1000; p->calc(0); p->calc(0); return p;
}
static void pump(mock::Host& h, Ind* p, unsigned long tEnd) {
    h.bars.push_back(bar(tEnd, 7000.0f)); h.visFrom = std::max(0, (int)h.bars.size() - 60); h.visTo = (int)h.bars.size() - 1;
    NOW += 1000; p->calc(0); for (int k = 0; k < 50 && mock::feed.at < mock::feed.ev.size(); ++k) { NOW += 1000; p->calc(0); }
}

int main(int argc, char** argv) {
    const std::string fx = argc > 1 ? argv[1] : "fx";
    // ---- H1 markets ----
    {   struct C { const char* r; const char* s; const char* want; } cs[] = { {"QGC", "", "GC"}, {"@ES", "", "ES"}, {"EP", "", "ES"}, {"QCL", "", "CL"}, {"CLE", "", "CL"},
            {"QHG", "", "HG"}, {"6E", "", "EU"}, {"NGZ6", "", "NG"}, {"", "ESZ6", "ES"}, {"QG", "", ""}, {"ZB", "ZBZ6", ""}, {"MNQ", "", ""} };
        int ok = 0; for (auto& c : cs) { const std::string g = zdh::marketOf(c.r, c.s); if (g == c.want) ++ok; else std::printf("  market %s/%s -> '%s' want '%s'\n", c.r, c.s, g.c_str(), c.want); }
        CHECK(ok == (int)(sizeof cs / sizeof cs[0]), "H1 markets through mkey::norm (QGC = GC, @ES / EP = ES, QCL / CLE = CL, QHG = HG, 6E = EU, ESZ6; unknown = '')"); }
    // ---- periodOf with the real offsets: 08:30 CT = 13:30 UTC (CDT, -300) / 14:30 UTC (CST, -360) ----
    {   zd::Engine a(zdp::forMarket("ES")); a.ctOffsetMin = -300; zd::Engine b(zdp::forMarket("ES")); b.ctOffsetMin = -360;
        const long long cdt = 1793021400000LL, cst = 1793716200000LL;   // 2026-10-26 13:30 UTC (08:30 CDT), 2026-11-03 14:30 UTC (08:30 CST)
        CHECK(zd::periodOf(a.ctMinute(cdt), a.M) == zdp::P_OPEN && zd::periodOf(b.ctMinute(cst), b.M) == zdp::P_OPEN && zd::periodOf(b.ctMinute(cst - 60000), b.M) == zdp::P_PREOPEN,
              "periodOf: the RTH open on CDT and CST offsets"); }
    // ---- engine: implied fill, MOD price move, ghost order, first second, warm-up, roll carry ----
    {   zd::Engine E(zdp::forMarket("ES")); E.barSec = 180;
        E.feed(ev(1000, zd::ADD, 0, 28000, 5, 1)); E.feed(ev(1000, zd::ADD, 1, 28004, 5, 2));
        CHECK(E.events == 2 && E.bookSize() == 2, "first-second events are taken");
        E.feed(ev(1001, zd::FILL, -1, 28000, 2, 1)); CHECK(E.implied == 1 && E.fills == 0, "M3 an implied fill (no side) is not counted");
        E.feed(ev(1002, zd::MOD, 0, 27996, 5, 1)); CHECK(E.modMoves == 1, "M4 a price-moving MOD counts as a cancel at the old price");
        E.feed(ev(1003, zd::ADD, 0, 28050, 3, 3));                       // a bid far above the trades: its DEL is "missed"
        E.feed(ev(1010, zd::FILL, 1, 28004, 1, 2)); E.feed(ev(1061, zd::ADD, 0, 27990, 1, 4));   // a trade at 28004, then a minute closes
        CHECK(E.ghosts == 1 && E.bookSize() == 3, "M1 / missed DEL: a bid crossing the last trade by > 2 ticks is dropped at the minute close (" + std::to_string(E.ghosts) + ")");
        CHECK(E.warmUntil == 1000 * 1000 + zd::WARM_MS, "M2 a cold start warms 10 min");
        zd::Engine N(zdp::forMarket("ES")); N.carryBook(E); N.feed(ev(2000, zd::ADD, 0, 27980, 1, 9));
        CHECK(N.bookSize() == 4 && N.warmUntil == 2000 * 1000, "M2 the session roll keeps the book and needs no warm-up");
        E.reset(1100 * 1000); CHECK(E.bookSize() == 0 && E.resets == 1, "M1 reset drops the book"); }
    // ---- the queue normal is not ~0% (CoB mockup finding): the HG fixture ----
    {   std::vector<FR> v; { FILE* f = std::fopen((fx + "/HG-2026-11-04.bin").c_str(), "rb"); FR r; while (f && std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); if (f) std::fclose(f); }
        zd::Engine E(zdp::forMarket("HG")); E.barSec = 180; E.minLotsOverride = 18;
        for (auto& r : v) E.feed(ev(r.t, r.a, r.s, r.px, r.sz, r.oid));
        int n = 0, zero = 0, pull = 0; for (auto& z : E.zones) if (z.touchT >= 0) { ++n; zero += z.cnorm <= 0.001; pull += z.pulling; }
        CHECK(n >= 5 && zero * 4 <= n && pull * 2 <= n, "queue normal (same distance): " + std::to_string(n) + " touches, " + std::to_string(zero) + " with a 0% normal, " + std::to_string(pull) + " pulling"); }
    // ---- the GC 2026-10-26 08:57 regression: lift 1.07x and price back above the zone by the 3-min close = SWEPT ----
    {   std::vector<FR> v; { FILE* f = std::fopen((fx + "/GC-2026-10-26-sweep.bin").c_str(), "rb"); FR r; while (f && std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); if (f) std::fclose(f); }
        zd::Engine E(zdp::forMarket("GC")); E.barSec = 180; E.minLotsOverride = 36;   // (1.0.5) pinned at the floor it was written with (the live floors move nightly)
        for (auto& r : v) E.feed(ev(r.t, r.a, r.s, r.px, r.sz, r.oid));
        const zd::Zone* z = nullptr; for (auto& q : E.zones) if (q.side == 0 && q.lo == 40045 && q.hi == 40048) z = &q;
        CHECK(z && z->touchT >= 0 && z->sweep == 2 && z->lift >= 1.0 && z->backIn, "GC 10-26 4004.5-4004.8: the 8:58 stop run reads SWEPT (lift " + std::string(z ? std::to_string(z->lift) : "-") + ", back in " + (z && z->backIn ? "yes" : "no") + ")");
        CHECK(z && z->sweepDecT % (zd::SWEEP_BAR_S * 1000LL) == 0, "the sweep is decided at the run candle's close (never changed after)"); }
    // ---- the GC 2026-11-06 example (Rassul's chart levels PFL/PDL 4035.1, PS0 4050, VAL 4053, WPoc 4055; 06:30-08:30 CT slice) ----
    //  stage 1: the 7:30:02 flush to 4035.1 went through a THIN book - no bid zone at / near 4035 before it (nothing to defend)
    //  stage 2: the 50% pullback - the bid zone 4054.0-4054.3 (WPoc) is touched, its stop run SWEPT, the zone HELD
    {   std::vector<FR> v; { FILE* f = std::fopen((fx + "/GC-2026-11-06-sweep.bin").c_str(), "rb"); FR r; while (f && std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); if (f) std::fclose(f); }
        zd::Engine E(zdp::forMarket("GC")); E.barSec = 180; E.minLotsOverride = 15;   // the research floor (the chart's GC floor is 32-36 in these periods)
        std::vector<zd::Lvl> L;
        for (auto& x : zd::levelsOf("PFL/PDL", 40351)) L.push_back(x); for (auto& x : zd::levelsOf("PS0", 40500)) L.push_back(x);
        for (auto& x : zd::levelsOf("VAL", 40530)) L.push_back(x); for (auto& x : zd::levelsOf("WPoc", 40550)) L.push_back(x);
        E.setLevels(L);
        for (auto& r : v) E.feed(ev(r.t, r.a, r.s, r.px, r.sz, r.oid));
        const long long t730 = (1793923200LL + 7 * 3600 + 1800) * 1000;   // 2026-11-06 07:30 (CT wall clock as if UTC)
        int nearSweep = 0; const zd::Zone* w = nullptr;
        for (auto& q : E.zones) { if (q.side == 0 && q.T < t730 && q.hi <= 40400) ++nearSweep; if (q.side == 0 && q.lo <= 40543 && q.hi >= 40540 && q.T > t730 && !w) w = &q; }
        CHECK(!v.empty() && nearSweep == 0, "GC 11-06 stage 1: no bid zone at 4035-4040 before the 7:30 flush (a thin book, nothing defended) (" + std::to_string(nearSweep) + ")");
        CHECK(w && w->touchT >= 0 && w->sweep == 2 && (w->state == 2 || w->state == 3) && w->tags.size() >= 1 && w->tags[0].name == "WPoc",
              "GC 11-06 stage 2: the WPoc zone 4054.0 is touched, swept and HELD (" + (w ? "sweep " + std::to_string(w->sweep) + ", state " + std::to_string(w->state) + ", tag " + (w->tags.empty() ? std::string("-") : w->tags[0].name) : std::string("no zone")) + ")"); }
    // ---- (1.0.3) the at-level floor on CHART settings (no research override), the same GC 11-06 slice ----
    {   std::vector<FR> v; { FILE* f = std::fopen((fx + "/GC-2026-11-06-sweep.bin").c_str(), "rb"); FR r; while (f && std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); if (f) std::fclose(f); }
        auto run = [&](double x, int& nearSweep) { zd::Engine E(zdp::forMarket("GC")); E.barSec = 180; E.M.levelFloorX = x;
            std::vector<zd::Lvl> L; for (auto& q : zd::levelsOf("PFL/PDL", 40351)) L.push_back(q); for (auto& q : zd::levelsOf("PS0", 40500)) L.push_back(q);
            for (auto& q : zd::levelsOf("VAL", 40530)) L.push_back(q); for (auto& q : zd::levelsOf("WPoc", 40550)) L.push_back(q); E.setLevels(L);
            for (auto& r : v) E.feed(ev(r.t, r.a, r.s, r.px, r.sz, r.oid));
            const long long t730 = (1793923200LL + 7 * 3600 + 1800) * 1000; nearSweep = 0; zd::Zone w; w.id = -1;
            for (auto& q : E.zones) { if (q.side == 0 && q.T < t730 && q.hi <= 40400) ++nearSweep; if (q.side == 0 && q.lo <= 40543 && q.hi >= 40540 && q.T > t730 && w.id < 0) w = q; }
            return w; };
        int ns = 0; const zd::Zone w5 = run(0.5, ns);
        CHECK(ns == 0 && w5.id >= 0 && w5.lowFloor && w5.touchT >= 0 && w5.sweep == 2 && (w5.state == 2 || w5.state == 3),
              "1.0.3 chart settings, GC at-level floor ON (0.5x): the 7:41 WPoc zone appears (" + std::to_string(w5.size) + " lots), swept + held; nothing at 4035-4040 before 7:30");
        int ns2 = 0; const zd::Zone wc = run(zdp::forMarket("GC").levelFloorX, ns2);
        CHECK(ns2 == 0 && zdp::forMarket("GC").levelFloorX >= 1.0 && wc.id < 0, "1.0.3 as shipped: GC is CAPPED for noise (levelFloorX 1.0) - no 21-lot zone, as 1.0.2");
        CHECK(zdp::forMarket("EU").levelFloorX == 0.5 && zdp::forMarket("EU").levelFloor == 400, "1.0.3 EU at-level floor on: max(400, 0.5x)"); }
    // ---- plugin: one writer per journal; restart mid-reload; torn line; contract roll ----
    {   freshHome(); mock::Host h; mock::current = &h; mock::feed = mock::Feed(); const unsigned long t0 = 1793030400UL;   // 2026-10-26 ~14:00 (mock local = UTC)
        Ind* a = pane(h, t0);
        int id = 1;   // a cold start warms 10 min: book first, then 15 min of trades
        for (int k = 0; k < 40; ++k) mock::feed.push(0, 1, 6990.0f, 10, (RTDATE)t0 + 1, id++);
        for (int s = 2; s < 900; ++s) { mock::feed.push(0, 0, 7000.25f, 1, (RTDATE)(t0 + s), id); mock::feed.push(3, 0, 7000.25f, 1, (RTDATE)(t0 + s), id); ++id; }
        for (unsigned long e = t0 + 180; e <= t0 + 900; e += 180) pump(h, a, e);
        // a SECOND CHART (another host) with the same market and bar size: it must only read the journal
        mock::Host h2; mock::current = &h2; Ind* b = pane(h2, t0 + 900);
        mock::feed.push(0, 1, 6989.0f, 1, (RTDATE)(t0 + 901), id++); pump(h2, b, t0 + 1080);
        auto* eb = b->ext(*b->testState()); mock::current = &h; auto* ea = a->ext(*a->testState());
        CHECK(ea->writer && !eb->writer && eb->path == ea->path, "H4 one writer per journal (the second chart reads it)");
        mock::current = &h;
        const std::string path = ea->writer ? ea->path : eb->path;
        // torn last line + an open zone in the journal, then a restart
        { std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); f << "ZN,900,1793030500000,0,27950,27953,400,2.00,5.0,4,0,0\nZT,900,1793030600000,300,0.1,0.1,5,0\nZR,900,17930"; }
        mock::current = &h2; dispose(b); mock::current = &h; dispose(a);
        mock::feed = mock::Feed(); Ind* c = pane(h, t0 + 900);
        mock::feed.push(0, 1, 6980.0f, 1, (RTDATE)(t0 + 1000), 99999); pump(h, c, t0 + 1080);
        const std::string j = readAll(path);
        CHECK(j.find("ZR,900,17930\n") != std::string::npos, "M7 a torn last line is ended (the next record starts on its own line)");
        CHECK(j.find("ZE,900,") != std::string::npos && j.find(",5,0\n", j.find("ZE,900,")) != std::string::npos, "H3 an open zone left in the journal is closed (ZE code 5) on reopen");
        zdh::JFile jf; jf.path = path; jf.poll(); const zdr::ZoneView& zv = jf.J.z[900];
        const auto L = zdr::readLines(zv, [](long long) { return std::string("x"); }, zdr::Signs(), 0.25);
        CHECK(zv.ended && zv.endCode == 5 && L[0].verdict == "closed" && L[2].verdict == "not decided", "H3 restart mid-reload: Zone 'closed', Pressure 'not decided' (never resumed)");
        // contract roll: a new symbol on the chart resets the book (open zones closed 5)
        auto* ec = c->ext(*c->testState()); const long long r0 = ec->eng ? ec->eng->resets : -1;
        h.symbol = "ESH7"; mock::feed.push(0, 1, 6980.0f, 1, (RTDATE)(t0 + 1100), 100000); pump(h, c, t0 + 1260);
        CHECK(ec->eng && ec->eng->resets == r0 + 1, "M1 a contract roll (new symbol) resets the book");
        // H3 removal: done / destroy close the open zones (5) and give up the journal
        dispose(c); CHECK(zdh::Owners::get().by.empty(), "H3 the removed pane gives up the journal (no writer left)"); }
    // ---- session roll: events across 17:00 -> a new journal, the book kept ----
    {   freshHome(); mock::Host h; mock::current = &h; mock::feed = mock::Feed(); const unsigned long t0 = 1793034000UL - 300;   // 16:55 (mock local = UTC)
        Ind* p = pane(h, t0); mock::feed.push(0, 1, 6990.0f, 5, (RTDATE)(t0 + 10), 1); pump(h, p, t0 + 180);
        auto* e = p->ext(*p->testState()); const std::string s1 = e->session; const size_t b1 = e->eng ? e->eng->bookSize() : 0;
        mock::feed.push(0, 1, 6989.0f, 5, (RTDATE)(t0 + 400), 2); pump(h, p, t0 + 540);
        CHECK(e->session != s1 && e->eng && e->eng->bookSize() == b1 + 1 && e->eng->warmUntil == (long long)(t0 + 400) * 1000, "M2 the 17:00 roll: a new session journal, the book kept, no warm-up");
        dispose(p); }
    // ---- M6 marks map a time to the bar whose END is after it ----
    {   mock::Host h; mock::current = &h; for (int k = 1; k <= 5; ++k) h.bars.push_back(bar(1000UL + 180UL * k, 1.0f));
        std::vector<long long> T; for (auto& b : h.bars) T.push_back((long long)b.time);   // bar ENDS 1180, 1360, 1540 ...
        CHECK(zdh::barOfEnd(T, (int)T.size(), 1179999) == 0 && zdh::barOfEnd(T, (int)T.size(), 1180000) == 1 && zdh::barOfEnd(T, (int)T.size(), 1181000) == 1,
              "M6 bar ENDS: 1179.999 s -> bar 0, exactly 1180 s (its end) -> the NEXT bar"); }
    // ---- (1.0.2 levels) his level names, the one-sided reach, the ZL journal line -> the label, the host reads the level files ----
    {   const std::vector<zd::Lvl> a = zd::levelsOf("PFL/PDL", 100), b = zd::levelsOf("CR 0D/GW 0D", 200), c = zd::levelsOf("G 7000", 300);
        CHECK(a.size() == 2 && a[0].name == "PFL" && a[1].name == "PDL" && a[0].dir == -1 && a[0].kind == zd::LK_KEY && b.size() == 1 && b[0].name == "CR0" && b[0].kind == zd::LK_MQ && c.empty(),
              "levels: PFL/PDL -> 2 key lows, CR 0D/GW 0D -> CR0 only, a gamma node is not his list");
        zd::Lvl S; S.px = 100; S.dir = -1;
        CHECK(zd::levelGap(95, 98, 0, S) == 2 && zd::levelGap(99, 101, 0, S) == 0 && zd::levelGap(101, 103, 0, S) == -1, "levels: support reaches the zone BELOW it only (gap 2), overlap = 0, above = never"); }
    {   zdr::Journal J; J.add("H,0.25,1.0.2"); J.add("ZN,1,1000,0,27960,27963,450,2.6,14,3,0,0"); J.add("ZL,1,1000,VAL:27962;PS0:27960");
        const std::string l1 = zdr::zoneLabel(J.z[1], 0.25); J.add("ZL,1,2000,"); const std::string l2 = zdr::zoneLabel(J.z[1], 0.25);
        CHECK(l1.find("at VAL + PS0") != std::string::npos && l2.find(" at ") == std::string::npos && J.bad == 0, "label: \"at VAL + PS0\" from the ZL line, gone when the latest ZL has none (" + l1 + ")"); }
    {   freshHome(); mock::Host h; mock::current = &h; mock::feed = mock::Feed(); const unsigned long t0 = 1793034000UL - 3600;
        const std::string dir = zdh::flexDir(); { std::ofstream f((dir + "\\LRA-KeyLevels-ES.csv").c_str()); f << "KEYLV,1," << t0 << "\nL|PDL|6990.00|0\nL|ONH|7010.50|0\nL|VWAP|7000|0\n"; }
        zdh::testWall() = (long long)t0 + 60;   // (1.0.5) the files are judged on the real UTC clock: pin it next to the file time
        Ind* p = pane(h, t0); mock::feed.push(0, 1, 6990.0f, 5, (RTDATE)(t0 + 10), 1); pump(h, p, t0 + 180);
        auto* e = p->ext(*p->testState());
        CHECK(e->eng && e->nLevels == 2 && e->eng->levels.size() == 2 && e->eng->levels[0].name == "PDL" && e->eng->levels[0].px == 27960 && e->eng->levels[1].px == 28042,
              "host: the key-level file -> PDL 6990 and ONH 7010.50 in ticks (VWAP is not his list) (" + std::to_string(e->nLevels) + ", " + e->lvlNote + ")");
        { std::ofstream f((dir + "\\LRA-VolLevels-ES.csv").c_str()); f << "SYMBOL|ESZ6\nDATE|2000-01-01\nLEVEL|VAL|prior session VAL|6995.00\n"; }
        NOW += 6000; mock::feed.push(0, 1, 6989.0f, 5, (RTDATE)(t0 + 200), 2); pump(h, p, t0 + 360);
        CHECK(e->nLevels == 2 && e->lvlNote.find("not for this session") != std::string::npos, "host: a volume file for another session is not used (" + e->lvlNote + ")");
        { std::ofstream f((dir + "\\LRA-VolLevels-ES.csv").c_str()); f << "SYMBOL|ESZ6\nDATE|" << e->session << "\nLEVEL|VAL|prior session VAL|6995.00\nLEVEL|WPoc|prior week POC|7001.25\n"; }
        NOW += 6000; mock::feed.push(0, 1, 6988.0f, 5, (RTDATE)(t0 + 380), 3); pump(h, p, t0 + 540);
        CHECK(e->nLevels == 4 && e->eng->levels.size() == 4 && e->eng->levels[2].name == "VAL" && e->eng->levels[2].px == 27980 && e->eng->levels[3].name == "WPoc",
              "host: LRA-VolLevels for this session + contract -> VAL / WPoc join the levels (" + std::to_string(e->nLevels) + ", " + e->lvlNote + ")");
        dispose(p); }
    // ---- (1.0.5 levels fix, live 2026-11-09 "levels 0 (0 levels from 0 files)") the files carry the real UTC time; the order stamps inside
    //      IRT are CT-as-epoch (5-6 h behind). 1.0.4 judged freshness on the stamps -> every file "from the future" -> 0. Now the UTC clock. ----
    {   freshHome(); mock::Host h; mock::current = &h; mock::feed = mock::Feed(); const unsigned long W = 1794240000UL;   // 2026-11-09 ~ 13:20 UTC
        const unsigned long t0 = W - 6 * 3600;                                                                        // the CT-as-epoch stamps (CST)
        const std::string dir = zdh::flexDir();
        { std::ofstream f((dir + "\\LRA-KeyLevels-ES.csv").c_str()); f << "KEYLV,1," << (W - 1800) << "\n";
          const char* nm[16] = { "ONH", "ONL", "PDH", "PDL", "PFH", "PFL", "LonHI", "LonLO", "PhrH", "PhrL", "AsiaHI", "AsiaLO", "VWAP", "G 7000", "IBH", "Settle" };
          for (int i = 0; i < 16; ++i) f << "L|" << nm[i] << "|" << (6960 + 4 * i) << ".00|0\n"; }
        zdh::testWall() = (long long)W;
        Ind* p = pane(h, t0); mock::feed.push(0, 1, 6990.0f, 5, (RTDATE)(t0 + 10), 1); pump(h, p, t0 + 180);
        auto* e = p->ext(*p->testState());
        CHECK(e->lvlNote.find("16 levels from 1 files") != std::string::npos && e->nLevels == 12 && e->eng && e->eng->levels.size() == 12,
              "1.0.5 levels: ES key file (16 levels, UTC time) with CT-as-epoch order stamps -> all 16 read, his 12 used (" + std::to_string(e->nLevels) + ", " + e->lvlNote + ")");
        {   nl::Cache C; const std::vector<nl::Level> old = nl::levels(C, dir + "\\", "ES", "ESZ6", 1, (long long)(t0 + 10));
            CHECK(old.empty() && C.note.find("0 levels from 0 files") != std::string::npos, "1.0.5 levels: the 1.0.4 call (wall = the CT stamp) reproduces the live bug (" + C.note + ")"); }
        dispose(p); zdh::testWall() = 0; }
    // ---- (1.0.2 review L4 / L6) bar sizes the epoch-cut bars are right for; the Central offset (DST) ----
    CHECK(zd::Engine::barSizeOk(60) && zd::Engine::barSizeOk(180) && zd::Engine::barSizeOk(300) && zd::Engine::barSizeOk(900) && zd::Engine::barSizeOk(3600)
          && !zd::Engine::barSizeOk(90) && !zd::Engine::barSizeOk(2700) && !zd::Engine::barSizeOk(7200) && !zd::Engine::barSizeOk(30), "L4 bar sizes: 1/3/5/15/60 min ok; 90 s, 45 min, 2 h, 30 s refused");
    { auto u = [](int y, int mo, int d, int h) { struct tm t; std::memset(&t, 0, sizeof t); t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; return (long long)timegm(&t); };
      CHECK(zd::Engine::chicagoOffsetMin(u(2026, 11, 8, 12)) == -360 && zd::Engine::chicagoOffsetMin(u(2026, 10, 30, 12)) == -300
            && zd::Engine::chicagoOffsetMin(u(2026, 11, 1, 6)) == -300 && zd::Engine::chicagoOffsetMin(u(2026, 11, 1, 7)) == -360
            && zd::Engine::chicagoOffsetMin(u(2026, 3, 8, 7)) == -360 && zd::Engine::chicagoOffsetMin(u(2026, 3, 8, 8)) == -300
            && zd::Engine::chicagoOffsetMin(u(2027, 3, 14, 8)) == -300 && zd::Engine::chicagoOffsetMin(u(2027, 3, 13, 12)) == -360, "L6 Chicago offset: CDT / CST with the 2026 and 2027 switch hours"); }
    {   freshHome(); mock::Host h; mock::current = &h; mock::feed = mock::Feed(); const unsigned long t0 = 1793034000UL - 3600;
        Ind* p = pane(h, t0); mock::feed.push(0, 1, 6990.0f, 5, (RTDATE)(t0 + 10), 1); pump(h, p, t0 + 180);
        auto* e = p->ext(*p->testState());
        CHECK(e->clockWarn.empty(), "1.0.4 L6: the zone unknown (no API answer) -> NO warning, whatever the order stamps say (" + e->clockWarn + ")");
        const int cst = zd::Engine::chicagoOffsetMin((long long)std::time(nullptr));
        zdh::testTz().known = true; zdh::testTz().offMin = cst; NOW += 1000; mock::feed.push(0, 1, 6989.0f, 5, (RTDATE)(t0 + 200), 2); pump(h, p, t0 + 360);
        CHECK(e->clockWarn.empty(), "1.0.4 L6: the API says Central (" + std::to_string(cst) + ") -> no warning, though the stamps are local-as-UTC");
        zdh::testTz().offMin = 0; mock::feed.push(0, 1, 6988.0f, 5, (RTDATE)(t0 + 380), 3); pump(h, p, t0 + 540);
        CHECK(e->clockWarn.find("UTC+0,") != std::string::npos, "1.0.4 L6: UTC+0 only when the API really says so (" + e->clockWarn + ")");
        zdh::testTz().offMin = -300 == cst ? -360 : -300; mock::feed.push(0, 1, 6987.0f, 5, (RTDATE)(t0 + 560), 4); pump(h, p, t0 + 720);
        CHECK(e->clockWarn.find("UTC-") != std::string::npos, "1.0.4 L6: a US zone one hour off is reported with its sign (" + e->clockWarn + ")");
        zdh::testTz() = zdh::PcTz();
        dispose(p); }
    {   freshHome(); mock::Host h; mock::current = &h; mock::feed = mock::Feed(); const unsigned long t0 = 1793034000UL - 3600;
        Ind* p = pane(h, t0); h.seconds = 90; mock::feed.push(0, 1, 6990.0f, 5, (RTDATE)(t0 + 10), 1); pump(h, p, t0 + 180);
        auto* e = p->ext(*p->testState());
        CHECK(!e->eng && e->note.find("unsupported bar size") != std::string::npos, "L4 a 90-s chart: no engine, the note says why (" + e->note + ")");
        dispose(p); }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
