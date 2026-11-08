// t_zd.cpp -- lsZoneDefense 1.0.0 on the mock host, fed REAL order-feed sessions (fx/: CL 2026-11-04 07:45-09:15 CT incl. the open,
// HG 2026-11-04 06:30-08:30 CT; exported from raw Databento MBO by zdb/fx_export.py).
//   1 PARITY: the plugin path (DEPTH_ORDER -> onRaw -> evOf -> engine -> journal file) writes exactly the lines the engine writes on
//     the same events directly (= the zdrun CLI the jumpstart / nightly use) - every line, in order.
//   2 NO REPAINT: a restart half way: the journal before it is byte-identical afterwards, ids never reused, the pane draws the same
//     lines for the zones decided before the restart.
//   3 LIVE vs CLOSE: every Read line, once decided (not "watching" / "counting"), is the same text at the end of the session.
//   4 LAYOUT: the 4 lines never overlap each other or their time; the time is right-justified; nothing outside the pane.
//   5 VERSIONS: ZD_VERSION == zdset::PLUGIN_VERSION == the setVersion literals == "1.0.0", title "Zone Defense v1.0.0".
// Build (tests_zd/): g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock t_zd.cpp -o t && ./t
#include "ZoneDefense.cpp"
#include <set>
int getTextWidthMock(const mock::Draw* d);
typedef ZoneDefense Ind;
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
static long long& NOW = off::off_test_now_ms;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
#pragma pack(push, 1)
struct FR { int t; signed char a, s; int px; unsigned sz; int oid; };
#pragma pack(pop)
static std::vector<FR> load(const std::string& p) { std::vector<FR> v; FILE* f = std::fopen(p.c_str(), "rb"); if (!f) return v; FR r; while (std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); std::fclose(f); return v; }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::vector<std::string> linesOf(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string l; while (std::getline(ss, l)) if (!l.empty()) v.push_back(l); return v; }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); return p; }
static mock::Bar bar(unsigned long t, float px) { mock::Bar b; b.time = t; b.close = px; b.open = px; b.low = px; b.high = px; b.volume = 100; b.rows.push_back(mock::Price(px, 50, 50, 100)); return b; }
static std::string clk(long long t) { if (t <= 0) return ""; time_t v = (time_t)(t / 1000); struct tm m; gmtime_r(&v, &m); return zdh::clock12(m.tm_hour, m.tm_min); }

struct Run { std::vector<std::string> journal; std::map<int, std::vector<std::string> > firstDecided; size_t zones = 0; };
// feed [from, to) of the fixture to plugin p on host h, one 180-s candle at a time; records each zone's decided lines when first seen
static void feedRange(mock::Host& h, Ind* p, const std::vector<FR>& v, size_t from, size_t to, double tick, Run& R, const std::string& path) {
    size_t i = from;
    while (i < to) {
        const unsigned long end = (unsigned long)((v[i].t / 180 + 1) * 180);
        for (; i < to && (unsigned long)v[i].t < end; ++i) { mock::feed.push((unsigned char)v[i].a, v[i].s == 0 ? 1 : 0, (float)(v[i].px * tick), v[i].sz, (RTDATE)v[i].t, v[i].oid); }
        h.bars.push_back(bar(end + 180, (float)(v[i - 1].px * tick)));     // the forming candle after the closed one
        h.visFrom = std::max(0, (int)h.bars.size() - 60); h.visTo = (int)h.bars.size() - 1;
        NOW += 250; p->calc(0); NOW += 250; p->calc(0);
        for (int k = 0; k < 50 && mock::feed.at < mock::feed.ev.size(); ++k) { NOW += 250; p->calc(0); }   // a busy candle: the reads catch up (as the 250 ms clock does live)
        h.draws.clear(); p->draw();
        zdh::JFile jf; jf.path = path; jf.poll();
        for (std::map<int, zdr::ZoneView>::const_iterator it = jf.J.z.begin(); it != jf.J.z.end(); ++it) {
            if (!it->second.touched) continue;
            const std::vector<zdr::RLine> L = zdr::readLines(it->second, clk, zdh::signsOf(h.root == "6E" ? "EU" : h.root));
            std::vector<std::string>& got = R.firstDecided[it->first]; if (got.empty()) got.assign(4, "");
            for (int k = 0; k < 4; ++k) {
                const std::string& t = L[k].text;
                const bool open = t.find("watching") != std::string::npos || t.find("counting") != std::string::npos;
                if (!open && got[k].empty()) got[k] = t + " @" + L[k].time;
            }
        }
    }
}

static void session(const std::string& mkt, const std::string& fx, double tick, int ctOffSec) {
    (void)ctOffSec;
    const std::vector<FR> v = load(fx);
    CHECK(v.size() > 100000, mkt + " fixture loaded (" + std::to_string(v.size()) + " events)");
    if (v.empty()) return;
    // reference: the engine directly (zdrun), the same events, t = whole seconds (the plugin's clock in the mock)
    zd::Engine ref(zdp::forMarket(mkt)); ref.ctOffsetMin = 0; ref.barSec = 180;
    for (size_t i = 0; i < v.size(); ++i) { zd::In e; e.t = (long long)v[i].t * 1000; e.act = v[i].a; e.side = v[i].s; e.px = v[i].px; e.sz = v[i].sz; e.oid = v[i].oid; ref.feed(e); }
    std::vector<std::string> refLines; for (size_t i = 0; i < ref.out.size(); ++i) refLines.push_back(ref.out[i].text);
    CHECK(ref.zones.size() >= 3, mkt + ": the engine found zones (" + std::to_string(ref.zones.size()) + ")");
    int touched = 0, reloads = 0, sweeps = 0; for (auto& z : ref.zones) { touched += z.touchT >= 0; reloads += z.decT >= 0; sweeps += z.sweep > 0; }
    CHECK(touched >= 2 && reloads >= 2, mkt + ": touches " + std::to_string(touched) + ", reload decisions " + std::to_string(reloads) + ", sweeps " + std::to_string(sweeps));

    // ---- 1 PARITY: the plugin over the whole slice ----
    char tmpl[] = "/tmp/zdXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h; h.tick = (float)tick; h.seconds = 180; h.root = mkt == "EU" ? "6E" : mkt; h.symbol = mkt + "Z6"; h.paneTop = 0; h.paneBottom = 160; h.paneLeft = 0; h.paneRight = 900; h.scaleLeft = 880; h.barW = 12;
    mock::feed = mock::Feed();
    const unsigned long t0 = (unsigned long)(v[0].t / 180 * 180);
    for (int k = -40; k <= 0; ++k) h.bars.push_back(bar(t0 + 180UL * k, (float)(v[0].px * tick)));
    Ind* p = make(); NOW += 1000; p->calc(0); p->calc(0);
    struct tm m0; time_t tt = v[0].t; gmtime_r(&tt, &m0); const std::string ses = zdh::sessionOf(m0);
    const std::string path = zdh::journalPath(flex, mkt, 180, ses);
    Run R; feedRange(h, p, v, 0, v.size(), tick, R, path);
    std::vector<std::string> got = linesOf(readAll(path));
    CHECK(!got.empty() && got[0].compare(0, 2, "H,") == 0, mkt + ": journal header (" + path + ")");
    if (!got.empty()) got.erase(got.begin());
    size_t same = 0; while (same < got.size() && same < refLines.size() && got[same] == refLines[same]) ++same;
    CHECK(got.size() == refLines.size() && same == refLines.size(), mkt + " PARITY: plugin journal " + std::to_string(got.size()) + " lines, engine " + std::to_string(refLines.size()) + ", identical prefix " + std::to_string(same)
          + (same < got.size() && same < refLines.size() ? "\n  plugin: " + got[same] + "\n  engine: " + refLines[same] : std::string()));
    // ---- 3 LIVE vs CLOSE ----
    zdh::JFile jf; jf.path = path; jf.poll(); int checked = 0, changed = 0;
    for (std::map<int, std::vector<std::string> >::const_iterator it = R.firstDecided.begin(); it != R.firstDecided.end(); ++it) {
        const zdr::ZoneView& zv = jf.J.z[it->first]; const std::vector<zdr::RLine> L = zdr::readLines(zv, clk, zdh::signsOf(mkt));
        for (int k = 0; k < 4; ++k) if (!it->second[k].empty()) { ++checked; if (it->second[k] != L[k].text + " @" + L[k].time) { ++changed; std::printf("  changed %d/%d: %s -> %s\n", it->first, k, it->second[k].c_str(), L[k].text.c_str()); } }
    }
    CHECK(checked > 4 && changed == 0, mkt + " LIVE vs CLOSE: " + std::to_string(checked) + " decided lines, " + std::to_string(changed) + " changed");
    CHECK(jf.J.bad == 0, mkt + ": every journal line parses (" + std::to_string(jf.J.bad) + " bad)");
    // ---- 4 LAYOUT: the last draw ----
    h.draws.clear(); p->draw();
    std::vector<const mock::Draw*> tx; for (auto& d : h.draws) if (d.type == "text") tx.push_back(&d);
    CHECK(tx.size() >= 4, mkt + ": the pane drew its lines (" + std::to_string(tx.size()) + " texts)");
    bool overlap = false, outside = false; int lineTexts = 0;
    for (size_t a = 0; a < tx.size(); ++a) {
        const mock::Draw* A = tx[a]; const int al = A->width ? A->x : (A->x == A->r ? A->r - (int)A->text.size() * 6 : A->x);
        const int aw = getTextWidthMock(A); const int aL = A->x == A->r ? A->r - aw : A->x, aR = aL + aw, aT = A->y - 7, aB = A->y + 7; (void)al;
        if (aL < h.paneLeft || aR > h.paneRight || aT < h.paneTop - 1 || aB > h.paneBottom) outside = true;
        if (A->color != zdh::C_GRAY) ++lineTexts;
        for (size_t b = a + 1; b < tx.size(); ++b) {
            const mock::Draw* B = tx[b]; const int bw = getTextWidthMock(B); const int bL = B->x == B->r ? B->r - bw : B->x, bR = bL + bw, bT = B->y - 7, bB = B->y + 7;
            if (aL < bR && aR > bL && aT < bB && aB > bT) overlap = true;
        }
    }
    CHECK(!overlap && !outside, mkt + " LAYOUT: no two texts overlap, all inside the pane");
    for (auto* d : tx) std::printf("  %s [%06lx] %s\n", mkt.c_str(), (unsigned long)d->color, d->text.c_str());
    CHECK(lineTexts == 4, mkt + ": exactly 4 Read lines (" + std::to_string(lineTexts) + ")");
    { const char* first[4] = { "", "Queue:", "Refill:", "Sweep:" }; int ok = 0; int k = 0;
      for (auto* d : tx) if (d->color != zdh::C_GRAY) { if (k == 0 ? (d->text.find("exhausted") != std::string::npos || d->text.find("holding") != std::string::npos || d->text.find("Quiet") != std::string::npos || d->text.find("Reload:") == 0) : d->text.compare(0, std::strlen(first[k]), first[k]) == 0) ++ok; ++k; }
      CHECK(ok == 4, mkt + ": line order Reload verdict, Queue, Refill, Sweep (" + std::to_string(ok) + "/4)"); }
    bool colours = true; for (auto* d : tx) if (d->color != zdh::C_GRAY && d->color != zdh::C_GREEN && d->color != zdh::C_RED && d->color != zdh::C_WHITE) colours = false;
    CHECK(colours, mkt + ": green / red / white text + grey times only");
    dispose(p);

    // ---- 2 NO REPAINT: run the first half, restart, run the rest ----
    char tmpl2[] = "/tmp/zdrXXXXXX"; const char* tmp2 = mkdtemp(tmpl2); setenv("USERPROFILE", tmp2, 1);
    const std::string path2 = zdh::journalPath(std::string(tmp2) + "\\InvestorRT\\rtx\\lsFlexLevels", mkt, 180, ses);
    mock::Host h2; mock::current = &h2; h2.tick = (float)tick; h2.seconds = 180; h2.root = h.root; h2.symbol = h.symbol; h2.paneBottom = 160; h2.paneRight = 900; h2.scaleLeft = 880; h2.barW = 12;
    mock::feed = mock::Feed();
    for (int k = -40; k <= 0; ++k) h2.bars.push_back(bar(t0 + 180UL * k, (float)(v[0].px * tick)));
    Ind* a = make(); NOW += 1000; a->calc(0); a->calc(0);
    Run RA; const size_t half = v.size() / 2; feedRange(h2, a, v, 0, half, tick, RA, path2);
    h2.draws.clear(); a->draw(); std::vector<std::string> before; for (auto& d : h2.draws) if (d.type == "text") before.push_back(d.text);
    const std::string fileA = readAll(path2); dispose(a);
    Ind* b = make(); NOW += 1000; b->calc(0); b->calc(0);
    h2.draws.clear(); b->draw(); std::vector<std::string> after; for (auto& d : h2.draws) if (d.type == "text") after.push_back(d.text);
    CHECK(!before.empty() && before == after, mkt + " NO REPAINT: the pane draws the same lines after a restart (" + std::to_string(before.size()) + ")");
    Run RB; feedRange(h2, b, v, half, v.size(), tick, RB, path2);
    const std::string fileB = readAll(path2);
    CHECK(fileB.size() >= fileA.size() && fileB.compare(0, fileA.size(), fileA) == 0, mkt + " NO REPAINT: the journal before the restart is unchanged after it (append-only)");
    zdh::JFile j2; j2.path = path2; j2.poll(); std::set<int> ids; int dup = 0;
    for (auto& l : linesOf(fileB)) if (l.compare(0, 3, "ZN,") == 0) { const int id = std::atoi(l.c_str() + 3); if (!ids.insert(id).second) ++dup; }
    CHECK(dup == 0 && j2.J.bad == 0, mkt + ": no zone id reused after the restart (" + std::to_string(ids.size()) + " zones, " + std::to_string(dup) + " dup)");
    // the decided lines of zones decided before the restart are unchanged at the end
    int ch2 = 0, n2 = 0;
    for (auto& it : RA.firstDecided) { const zdr::ZoneView& zv = j2.J.z[it.first]; const auto L = zdr::readLines(zv, clk, zdh::signsOf(mkt));
        for (int k = 0; k < 4; ++k) if (!it.second[k].empty()) { ++n2; if (it.second[k] != L[k].text + " @" + L[k].time) ++ch2; } }
    CHECK(n2 > 0 && ch2 == 0, mkt + " NO REPAINT: " + std::to_string(n2) + " lines decided before the restart, " + std::to_string(ch2) + " changed");
    dispose(b);
}
int getTextWidthMock(const mock::Draw* d) { int pt = d->fsize < 6 ? 6 : d->fsize > 14 ? 14 : d->fsize; (void)pt; return (int)d->text.size() * 5; }

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "fx";
    session("CL", dir + "/CL-2026-11-04.bin", 0.01, -6 * 3600);
    session("HG", dir + "/HG-2026-11-04.bin", 0.0005, -6 * 3600);
    // ---- 5 VERSIONS ----
    const std::string src = readAll("../ZoneDefense.cpp"), marks = readAll("../ZoneDefenseMarks.cpp");
    CHECK(std::string(ZD_VERSION) == "1.0.0" && std::string(zdset::PLUGIN_VERSION) == ZD_VERSION && std::string(zd::VERSION) == ZD_VERSION, "versions 1.0.0 (ZD_VERSION, settings spec, engine)");
    size_t n = 0, at = 0; while ((at = src.find("p->setVersion(\"1.0.0\")", at)) != std::string::npos) { ++n; ++at; }
    CHECK(n == 1 && marks.find("p->setVersion(\"1.0.0\")") != std::string::npos, "the setVersion literals (pane, marks, the gex-build line) = 1.0.0");
    char t[64]; Ind* p = make(); p->parmsTitle(t, sizeof t); CHECK(std::string(t) == "Zone Defense v1.0.0", std::string("title ") + t); dispose(p);
    CHECK(zdset::spec().n == zdset::K_N && ss::check(zdset::spec()).empty(), "settings spec valid (" + std::to_string(ss::check(zdset::spec()).size()) + " problems)");
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
