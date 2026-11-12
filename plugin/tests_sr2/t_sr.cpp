// t_sr.cpp -- lsSR 1.1.0 (+ the strength grade: G lines, the n/4? label) on the mock host, fed REAL order-feed slices (fx/: HG 2026-10-07 06:00-11:30 CT, GC 2026-10-26 06:00-10:00 CT,
// exported from the Databento MBO store by fx_export.py; key levels of the study for that day).
//   1 PARITY: the plugin path (DEPTH_ORDER -> onRaw -> evOf -> engine -> journal file) writes exactly the lines the engine writes on the
//     same events directly (= the srrun CLI the study parity and the nightly use).
//   2 CONTENT: gold stop runs and reversal arrows exist in the slice (the study's cases).
//   3 NO REPAINT: a restart half way - the journal before it is byte-identical afterwards; the pane draws the same gold bars / arrows.
//   4 DRAW: gold bar + its ratio drawn, bright bars carry a ratio, nothing outside the pane, no white-pen outline.
//   5 VERSIONS: SR_VERSION == sr::VERSION == "1.0.0", title "SR v1.1.0".
// Build (tests_sr2/): g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock t_sr.cpp -o t && ./t
#include "SR.cpp"
#include <set>
typedef SRPane Ind;
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
#include <unistd.h>
static long long& NOW = off::off_test_now_ms;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
#pragma pack(push, 1)
struct FR { int t; signed char a, s; int px; unsigned sz; int oid, aggr; };
#pragma pack(pop)
static std::vector<FR> load(const std::string& p) { std::vector<FR> v; FILE* f = std::fopen(p.c_str(), "rb"); if (!f) return v; FR r; while (std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); std::fclose(f); return v; }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::vector<std::string> linesOf(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string l; while (std::getline(ss, l)) if (!l.empty()) v.push_back(l); return v; }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); return p; }
static mock::Bar bar(unsigned long t, float px) { mock::Bar b; b.time = t; b.close = px; b.open = px; b.low = px; b.high = px; b.volume = 100; b.rows.push_back(mock::Price(px, 50, 50, 100)); return b; }
static std::vector<sr::Level> levelsOf(const std::string& p) { std::vector<sr::Level> L; std::ifstream f(p.c_str()); std::string ln; while (std::getline(f, ln)) { const size_t b = ln.find('|'); if (b == std::string::npos) continue; sr::Level l; l.name = ln.substr(0, b); l.px = std::atoll(ln.c_str() + b + 1); l.side = sr::Engine::sideOfName(l.name); L.push_back(l); } return L; }
static void feedRange(mock::Host& h, Ind* p, const std::vector<FR>& v, size_t from, size_t to, double tick) {
    size_t i = from;
    while (i < to) {
        const unsigned long end = (unsigned long)((v[i].t / 180 + 1) * 180);
        for (; i < to && (unsigned long)v[i].t < end; ++i) { DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.timestamp = (RTDATE)v[i].t; o.price = (float)(v[i].px * tick); o.size = v[i].sz; o.orderID = v[i].oid; o.action = (unsigned char)v[i].a; o.buySell = v[i].s == 0 ? 1 : 0; o.aggressorOrderID = v[i].aggr; mock::feed.ev.push_back(o); }
        h.bars.push_back(bar(end + 180, (float)(v[i - 1].px * tick)));
        h.visFrom = std::max(0, (int)h.bars.size() - 60); h.visTo = (int)h.bars.size() - 1;
        for (int k = 0; k < 400 && mock::feed.at < mock::feed.ev.size(); ++k) p->calc(0);   // the clock stands still while one candle drains (every event of a second gets the same arrival)
        p->calc(0); NOW += 1000;
    }
}
static void session(const std::string& mkt, const std::string& fx, double tick) {
    const std::vector<FR> v = load(fx); const std::vector<sr::Level> L = levelsOf(fx + ".levels");
    CHECK(v.size() > 100000 && L.size() >= 8, mkt + " fixture loaded (" + std::to_string(v.size()) + " events, " + std::to_string(L.size()) + " levels)");
    if (v.empty()) return;
    const srp::Mkt& M = srp::forMarket(mkt);
    auto mkRef = [&]() { std::unique_ptr<sr::Engine> e(new sr::Engine()); e->spb = 180; e->rthOpenSec = M.rthOpen * 60LL; e->rthCloseSec = M.rthClose * 60LL; e->P.recBars = M.recBars; e->P.drawRev = M.drawRev; e->P.burstX = M.burstX; { const srg::Mkt& G = srg::forMarket(mkt); e->P.bigA = G.bigA; e->P.bigP = G.bigP; e->P.gShare = G.share; e->P.gBids = G.bids; e->P.gDeep = G.deep; e->P.gHeavy = G.heavy; e->P.gStrong = G.strong; } e->setLevels(L); e->setNextId(1000); return e; };
    std::unique_ptr<sr::Engine> ref = mkRef();
    for (size_t i = 0; i < v.size(); ++i) { const long long t = (long long)v[i].t * 1000;
        if (v[i].a == 3) ref->onFill(t, v[i].px, v[i].sz, v[i].s == 0 ? -1 : 1, (unsigned long long)(v[i].aggr ? v[i].aggr : v[i].oid), (unsigned long long)(v[i].aggr ? v[i].oid : 0)); else ref->onBook(t, v[i].s, v[i].a == 0, v[i].px, v[i].sz); }
    int gold = 0, rev = 0, bright = 0, grades = 0, gbad = 0; for (size_t i = 0; i < ref->journal.size(); ++i) { const std::string& l = ref->journal[i]; if (l[0] == 'V') ++rev; if (l[0] == 'G') { ++grades; if (sr::Journal::split(l).size() != 20) ++gbad; } if (l[0] == 'R' && l.size() > 2 && l.substr(l.size() - 2) == "|1") ++gold; }
    for (size_t i = 0; i < ref->bars.size(); ++i) bright += (ref->bars[i].xU >= 2 && ref->bars[i].hiN) || (ref->bars[i].xD >= 2 && ref->bars[i].loN);
    CHECK(gold >= 2 && rev >= 1 && bright >= 3, mkt + " CONTENT: gold stop runs " + std::to_string(gold) + ", reversal arrows " + std::to_string(rev) + ", bright bars " + std::to_string(bright));
    CHECK(grades >= 3 && gbad == 0, mkt + " GRADE: every run through a level is graded once (" + std::to_string(grades) + " G lines, 20 fields each)");
    // ---- the plugin ----
    char tmpl[] = "/tmp/srXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels"; srh::makeDir(std::string(tmp) + "\\InvestorRT"); srh::makeDir(std::string(tmp) + "\\InvestorRT\\rtx"); srh::makeDir(flex);
    srh::testWall() = (long long)v[0].t; srh::clockCheck() = false;
    { std::ofstream f((flex + "\\LRA-KeyLevels-" + mkt + ".csv").c_str(), std::ios::binary); f << "KEYLV,1," << v[0].t << "\n"; for (size_t i = 0; i < L.size(); ++i) { char b[96]; std::snprintf(b, sizeof b, "L|%s|%.6f|x\n", L[i].name.c_str(), L[i].px * tick); f << b; } }
    mock::Host h; mock::current = &h; h.tick = (float)tick; h.seconds = 180; h.root = mkt; h.symbol = mkt + "Z6"; h.paneTop = 0; h.paneBottom = 120; h.paneLeft = 0; h.paneRight = 900; h.scaleLeft = 880; h.barW = 12;
    mock::feed = mock::Feed();
    const unsigned long t0 = (unsigned long)(v[0].t / 180 * 180);
    for (int k = -40; k <= 0; ++k) h.bars.push_back(bar(t0 + 180UL * k, (float)(v[0].px * tick)));
    Ind* p = make(); NOW += 1000; p->calc(0); p->calc(0);
    struct tm m0; time_t tt = v[0].t; gmtime_r(&tt, &m0); const std::string path = srh::journalPath(flex, mkt, 180, srh::sessionOf(m0));
    const size_t half = v.size() / 2;
    feedRange(h, p, v, 0, half, tick);
    const std::string before = readAll(path);
    h.draws.clear(); p->draw(); std::multiset<std::string> d1; for (auto& d : h.draws) if (d.type == "text") d1.insert(d.text + "@" + std::to_string(d.x));
    dispose(p);                                                          // ---- restart half way ----
    mock::feed = mock::Feed(); p = make(); NOW += 1000; p->calc(0);
    h.draws.clear(); p->draw(); std::multiset<std::string> d2; for (auto& d : h.draws) if (d.type == "text") d2.insert(d.text + "@" + std::to_string(d.x));
    CHECK(d1 == d2 && !d1.empty(), mkt + " NO REPAINT: the same labels before and after the restart (" + std::to_string(d1.size()) + " / " + std::to_string(d2.size()) + ")");
    feedRange(h, p, v, half, v.size(), tick);
    const std::string after = readAll(path);
    CHECK(after.compare(0, before.size(), before) == 0 && after.size() > before.size(), mkt + " NO REPAINT: the journal before the restart is unchanged (" + std::to_string(before.size()) + " -> " + std::to_string(after.size()) + " bytes)");
    // ---- parity on the first half (one straight run of the plugin = the engine) ----
    {   std::unique_ptr<sr::Engine> r2 = mkRef();
        for (size_t i = 0; i < half; ++i) { const long long t = (long long)v[i].t * 1000; if (v[i].a == 3) r2->onFill(t, v[i].px, v[i].sz, v[i].s == 0 ? -1 : 1, (unsigned long long)(v[i].aggr ? v[i].aggr : v[i].oid), (unsigned long long)(v[i].aggr ? v[i].oid : 0)); else r2->onBook(t, v[i].s, v[i].a == 0, v[i].px, v[i].sz); }
        std::vector<std::string> got = linesOf(before); if (!got.empty()) got.erase(got.begin());
        size_t same = 0; while (same < got.size() && same < r2->journal.size() && got[same] == r2->journal[same]) ++same;
        CHECK(!got.empty() && same == got.size() && got.size() + 3 >= r2->journal.size(), mkt + " PARITY: plugin journal " + std::to_string(got.size()) + " lines, engine " + std::to_string(r2->journal.size()) + ", identical prefix " + std::to_string(same)
              + (same < got.size() && same < r2->journal.size() ? "\n  plugin: " + got[same] + "\n  engine: " + r2->journal[same] : std::string())); }
    // ---- draw ----
    srh::JFile jf; jf.path = path; jf.poll();
    CHECK(jf.J.bad == 0 && jf.J.bars.size() > 30, mkt + ": every journal line parses (" + std::to_string(jf.J.bad) + " bad, " + std::to_string(jf.J.bars.size()) + " candles)");
    { int strong = 0, gtxt = 0; for (auto& kv : jf.J.grade) if (kv.second.score >= srg::forMarket(mkt).strong || jf.J.gold.count(kv.second.barEnd)) ++strong;
      h.visFrom = 0; h.visTo = (int)h.bars.size() - 1; h.draws.clear(); p->draw(); for (auto& d : h.draws) if (d.type == "text" && d.text.size() >= 4 && d.text.compare(1, 2, "/4") == 0 && d.text.back() == '?' && d.color == SRPane::C_GOLD) ++gtxt;
      CHECK(!jf.J.grade.empty() && gtxt >= 1 && gtxt <= strong, mkt + " GRADE DRAW: " + std::to_string(jf.J.grade.size()) + " graded candles, " + std::to_string(strong) + " strong or gold, " + std::to_string(gtxt) + " grade labels drawn in gold with ?"); }
    CHECK(!jf.J.gold.empty(), mkt + ": gold stop runs in the journal (" + std::to_string(jf.J.gold.size()) + "), reversal arrows " + std::to_string(jf.J.rev.size()));
    h.visFrom = 0; h.visTo = (int)h.bars.size() - 1; h.draws.clear(); p->draw();
    int goldFill = 0, texts = 0, outside = 0, white = 0;
    for (auto& d : h.draws) { if (d.color == SRPane::C_GOLD && d.type != "text") ++goldFill; if (d.type == "text") ++texts; if (d.y < h.paneTop - 1 || d.y > h.paneBottom + 1) ++outside; if (d.type != "text" && d.pen == 0x00FFFFFF && d.penW > 0 && d.border == 0x00FFFFFF) ++white; }
    CHECK(goldFill >= 1 && texts >= 2 && outside == 0, mkt + " DRAW: gold bars " + std::to_string(goldFill) + ", ratio labels " + std::to_string(texts) + ", outside the pane " + std::to_string(outside) + ", white outlines " + std::to_string(white));
    char title[64] = { 0 }; p->parmsTitle(title, sizeof title); CHECK(std::string(title) == "SR v1.1.0", std::string("title '") + title + "'");
    dispose(p);
}
int main() {
    CHECK(std::string(SR_VERSION) == sr::VERSION && std::string(SR_VERSION) == "1.1.0", "versions");
    session("HG", "fx/HG_20261007.bin", 0.0005);
    session("GC", "fx/GC_20261026.bin", 0.1);
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
