// t_zd.cpp -- lsZoneDefense 1.0.0 on the mock host, fed REAL order-feed sessions (fx/: CL 2026-11-04 07:45-09:15 CT incl. the open,
// HG 2026-11-04 06:30-08:30 CT; exported from raw Databento MBO by zdb/fx_export.py).
//   1 PARITY: the plugin path (DEPTH_ORDER -> onRaw -> evOf -> engine -> journal file) writes exactly the lines the engine writes on
//     the same events directly (= the zdrun CLI the jumpstart / nightly use) - every line, in order.
//   2 NO REPAINT: a restart half way: the journal before it is byte-identical afterwards, ids never reused, the pane draws the same
//     lines for the zones decided before the restart.
//   3 LIVE vs CLOSE: every Read line, once decided (not "watching" / "counting"), is the same text at the end of the session.
//   4 LAYOUT: the 4 lines never overlap each other or their time; the time is right-justified; nothing outside the pane.
//   5 VERSIONS: ZD_VERSION == zdset::PLUGIN_VERSION == the setVersion literals == "1.0.8", title "Zone Defense v1.0.8".
// Build (tests_zd/): g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock t_zd.cpp -o t && ./t
#include "ZoneDefense.cpp"
#include <set>
#include <cctype>
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
static int fullBars = 0;   // (1.0.6) fixtures where bars were drawn under the box (at least one must)
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

// a decided line as compared live vs later: the Zone line's STATE progresses at candle closes (forming -> testing -> held / reclaimed /
// broken: each a new decision, never a repaint) - its range / lots / oldest never change; the Sweep line may become "reclaimed" (the
// run + the reclaim read together, a later decision); every other line, once decided, never changes at all
static std::string key(const zdr::RLine& l, int k) { return k == 0 ? l.detail : l.label + ": " + l.text + " @" + l.time; }
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
            const std::vector<zdr::RLine> L = zdr::readLines(it->second, clk, zdh::signsOf(h.root == "6E" ? "EU" : h.root), tick);
            std::vector<std::string>& got = R.firstDecided[it->first]; if (got.empty()) got.assign(5, "");
            for (int k = 0; k < 5; ++k) {
                const std::string& t = key(L[k], k);
                const bool open = t.find("watching") != std::string::npos || t.find("counting") != std::string::npos;
                if (!open && got[k].empty()) got[k] = t;
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
        const zdr::ZoneView& zv = jf.J.z[it->first]; const std::vector<zdr::RLine> L = zdr::readLines(zv, clk, zdh::signsOf(mkt), tick);
        for (int k = 0; k < 5; ++k) if (   // a broken zone that is reclaimed later moves on to "broken -> reclaimed" by design (his 10-bar window)
            !it->second[k].empty() && !(k == 4 && L[k].verdict == "reclaimed") && !(k == 0 && L[k].text.find("reclaimed") != std::string::npos && L[k].detail.compare(0, it->second[k].size(), it->second[k]) == 0)) { ++checked; if (it->second[k] != key(L[k], k)) { ++changed; std::printf("  changed %d/%d: %s -> %s\n", it->first, k, it->second[k].c_str(), L[k].text.c_str()); } }
    }
    CHECK(checked > 4 && changed == 0, mkt + " LIVE vs CLOSE: " + std::to_string(checked) + " decided lines, " + std::to_string(changed) + " changed");
    CHECK(jf.J.bad == 0, mkt + ": every journal line parses (" + std::to_string(jf.J.bad) + " bad)");
    // ---- 4 LAYOUT: the last draw ----
    h.draws.clear(); p->draw();
    {   // (1.0.8) the MBO quiet notice: a draw at a later Tuesday 10:00 (local clock) with no new event shows it, top-right, alone
        bool none = true; for (auto& d : h.draws) if (d.type == "text" && d.text.compare(0, 10, "MBO QUIET ") == 0) none = false;
        struct tm tq; std::memset(&tq, 0, sizeof tq); tq.tm_year = 2026 - 1900; tq.tm_mon = 10; tq.tm_mday = 10; tq.tm_hour = 10; tq.tm_isdst = -1;
        zdh::testWall() = (long long)std::mktime(&tq); std::vector<mock::Draw> keep = h.draws; h.draws.clear(); p->draw();
        const mock::Draw* q = nullptr; for (auto& d : h.draws) if (d.type == "text" && d.text.compare(0, 10, "MBO QUIET ") == 0) q = &d;
        CHECK(none && q != nullptr, mkt + ": 1.0.8 no notice while the feed is live; 'MBO QUIET ..m' when it went silent" + (q ? " (" + q->text + ")" : std::string()));
        zdh::testWall() = 0; h.draws = keep; }
    std::vector<const mock::Draw*> tx; for (auto& d : h.draws) if (d.type == "text") tx.push_back(&d);
    CHECK(tx.size() >= 4, mkt + ": the pane drew its lines (" + std::to_string(tx.size()) + " texts)");
    // (1.0.2) every text is drawn left-aligned in a rectangle exactly its width: [x, r] x [y - 6, y + 6]
    bool overlap = false, outside = false; std::vector<std::string> labels;
    for (size_t a = 0; a < tx.size(); ++a) {
        const mock::Draw* A = tx[a]; const int aL = A->x, aR = A->r, aT = A->y - 6, aB = A->y + 6;
        if (aL < h.paneLeft || aR > h.paneRight || aT < h.paneTop - 1 || aB > h.paneBottom) outside = true;
        if (A->text.size() > 3 && A->text[1] == '.' && A->text[2] == ' ' && std::isdigit((unsigned char)A->text[0])) labels.push_back(A->text);
        for (size_t b = a + 1; b < tx.size(); ++b) { const mock::Draw* B = tx[b]; if (aL < B->r && aR > B->x && aT < B->y + 6 && aB > B->y - 6) overlap = true; }
    }
    for (auto* d : tx) std::printf("  %s [%06lx] x %d..%d y %d %s\n", mkt.c_str(), (unsigned long)d->color, d->x, d->r, d->y, d->text.c_str());
    CHECK(!overlap && !outside, mkt + " LAYOUT: no two texts overlap (5 lines), all inside the pane");
    // ---- (1.0.6, Rassul 2026-11-09 09:03) plain ASCII text; bars at full pane height; the read in an opaque box sized to its text ----
    {   bool ascii = true; for (auto* d : tx) for (unsigned char ch : d->text) if (ch > 126 || (ch < 32 && ch != 0)) ascii = false;
        CHECK(ascii, mkt + " 1.0.6: every drawn string is 7-bit ASCII (no mojibake)");
        int boxAt = -1, lastBar = -1, barTop = 1 << 30, i = 0; const mock::Draw* box = nullptr;
        for (auto& d : h.draws) { if (d.type == "box" && d.color == zdh::C_BOX_BG) { box = &d; boxAt = i; } if (d.type == "box" && d.color == zdh::C_BAR_RED) { lastBar = i; barTop = std::min(barTop, d.y); } ++i; }
        int txR = 0, txB = 0; for (auto* d : tx) { txR = std::max(txR, d->r); txB = std::max(txB, d->y + 6); }
        bool inside = box != nullptr; if (box) for (auto* d : tx) if (d->x < box->x || d->r > box->r || d->y - 6 < box->y || d->y + 6 > box->b) inside = false;
        CHECK(inside && box->r <= txR + 8 && box->b <= txB + 4, mkt + " 1.0.6: one opaque box behind the read, every text inside it, sized to the text (box r " + std::to_string(box ? box->r : -1) + " vs text " + std::to_string(txR) + ")");
        CHECK(lastBar < 0 || (lastBar < boxAt && barTop == h.paneTop + 2), mkt + " 1.0.6: the bars are drawn first and the tallest reaches the pane top (top " + std::to_string(barTop) + ", pane " + std::to_string(h.paneTop) + ")");
        if (lastBar >= 0) ++fullBars; }
    const std::vector<std::string> want = { "1. Zone", "2. Queue", "3. Pressure", "4. Refill", "5. Sweep" };
    CHECK(labels == want, mkt + ": the 5 numbered lines in event order (" + std::to_string(labels.size()) + ")");
    {   std::map<std::string, int> xOf; std::set<int> verdictX, timeR, labelX;
        for (auto* d : tx) { if (d->color == zdh::C_GRAY && d->text.find(':') != std::string::npos && (d->text.find("AM") != std::string::npos || d->text.find("PM") != std::string::npos)) timeR.insert(d->r);
                             else if (d->color == zdh::C_GRAY) labelX.insert(d->x);
                             else if (d->fstyle == cppExtension::BOLD) verdictX.insert(d->x); }
        CHECK(labelX.size() == 1 && verdictX.size() == 1 && timeR.size() == 1, mkt + " LAYOUT: labels at one x, verdicts at one x (bold), times right-aligned in one column (" + std::to_string(labelX.size()) + "/" + std::to_string(verdictX.size()) + "/" + std::to_string(timeR.size()) + ")"); }
    { std::set<int> ys; for (auto* d : tx) ys.insert(d->y); std::vector<int> v(ys.begin(), ys.end()); bool even = v.size() == 5; for (size_t i = 2; i < v.size(); ++i) if (v[i] - v[i - 1] != v[1] - v[0]) even = false;
      CHECK(even, mkt + " LAYOUT: 5 rows, even spacing"); }
    bool colours = true; for (auto* d : tx) if (d->color != zdh::C_GRAY && d->color != zdh::C_GREEN && d->color != zdh::C_RED && d->color != zdh::C_WHITE && d->color != zdh::C_GREEN_T && d->color != zdh::C_RED_T && d->color != zdh::C_WHITE_T) colours = false;
    CHECK(colours, mkt + ": green / red / white (verdicts) + their tints (details) + grey labels / times only");
    { bool barsDim = true; for (auto& d : h.draws) if (d.type == "box" && (d.color == zdh::C_RED || d.color == zdh::C_GREEN)) barsDim = false;
      CHECK(barsDim, mkt + ": the pane bars are dimmed (never the line colours)"); }
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
    for (auto& it : RA.firstDecided) { const zdr::ZoneView& zv = j2.J.z[it.first]; const auto L = zdr::readLines(zv, clk, zdh::signsOf(mkt), tick);
        for (int k = 0; k < 5; ++k) if (!it.second[k].empty() && !(k == 4 && L[k].verdict == "reclaimed")) { ++n2; if (it.second[k] != key(L[k], k)) ++ch2; } }
    CHECK(n2 > 0 && ch2 == 0, mkt + " NO REPAINT: " + std::to_string(n2) + " lines decided before the restart, " + std::to_string(ch2) + " changed");
    dispose(b);
}
int getTextWidthMock(const mock::Draw* d) { int pt = d->fsize < 6 ? 6 : d->fsize > 14 ? 14 : d->fsize; (void)pt; return (int)d->text.size() * 5; }

int main(int argc, char** argv) {
    zdh::clockCheck() = false;   // the mock clock is UTC (the Central check has its own test in t_zd102)
    const std::string dir = argc > 1 ? argv[1] : "fx";
    session("CL", dir + "/CL-2026-11-04.bin", 0.01, -6 * 3600);
    session("HG", dir + "/HG-2026-11-04.bin", 0.0005, -6 * 3600);
    // ---- 5 VERSIONS ----
    const std::string src = readAll("../ZoneDefense.cpp"), marks = readAll("../ZoneDefenseMarks.cpp");
    CHECK(std::string(ZD_VERSION) == "1.0.8" && std::string(zdset::PLUGIN_VERSION) == ZD_VERSION && std::string(zd::VERSION) == ZD_VERSION, "versions 1.0.8 (ZD_VERSION, settings spec, engine)");
    size_t n = 0, at = 0; while ((at = src.find("p->setVersion(\"1.0.8\")", at)) != std::string::npos) { ++n; ++at; }
    CHECK(n == 1 && marks.find("p->setVersion(\"1.0.8\")") != std::string::npos, "the setVersion literals (pane, marks, the gex-build line) = 1.0.8");
    {   // (1.0.8) MBO quiet notice: the pane's helper on a fresh Ext - quiet after 2 min in the day, cleared by an event
        CHECK(std::string(mbq::VERSION) == "MboQuiet 1.0.0" && mbq::text(1000, 1000 + 12 * 60, 7 * 60 + 45, 1) == "MBO QUIET 12m", "1.0.8: MboQuiet 7:33 -> 7:45 = MBO QUIET 12m");
        CHECK(src.find("drawQuiet(e, right, pane.top + 3)") != std::string::npos && src.find("e->lastEvWall = zdh::wallNow();") != std::string::npos, "1.0.8: every order event stamps the feed clock; the notice is drawn top-right");
        CHECK(src.find("MBO_QUIET,") != std::string::npos, "1.0.8: the status file has the MBO_QUIET line"); }
    mock::Host hv; mock::current = &hv;   // (1.0.7) the sessions' hosts are gone: a live host for make() (ASan: stack-use-after-return)
    char t[64]; Ind* p = make(); p->parmsTitle(t, sizeof t); CHECK(std::string(t) == "Zone Defense v1.0.8", std::string("title ") + t); dispose(p);
    CHECK(zdset::spec().n == zdset::K_N && ss::check(zdset::spec()).empty(), "settings spec valid (" + std::to_string(ss::check(zdset::spec()).size()) + " problems)");
    CHECK(fullBars >= 1, "1.0.6: at least one fixture drew bars under the read box (" + std::to_string(fullBars) + ")");
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
