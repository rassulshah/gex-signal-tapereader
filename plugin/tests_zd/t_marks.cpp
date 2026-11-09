// t_marks.cpp -- lsZoneDefenseMarks 1.0.0 on the mock host: draws ONLY from the journal the pane writes (here: the engine's lines over
// the real HG 2026-11-04 slice) - at most N zones per side, labels never overlap, one big-order line per zone at most, the reload
// diamond for each decided reload in view, a sweep triangle per decided sweep in view, the same picture on every draw and after a
// restart (no repaint), nothing without a journal.
// Build (tests_zd/): g++ -std=c++17 -O2 -Wall -Wextra -I.. -I../tests_aggbook/mock t_marks.cpp -o tm && ./tm
#define ZD_MARKS 1
#include "ZoneDefense.cpp"
namespace mock { Host* current = nullptr; Feed feed; }
#include <cstdio>
#include <unistd.h>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
#pragma pack(push, 1)
struct FR { int t; signed char a, s; int px; unsigned sz; int oid; };
#pragma pack(pop)
typedef ZoneDefenseMarks Ind;
static Ind* make() { Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); return p; }
static void dispose(Ind* p) { p->destroy(); p->~Ind(); ::operator delete(p); }
static std::string sig(mock::Host& h) { std::string s; for (auto& d : h.draws) s += d.type + ":" + std::to_string(d.x) + "," + std::to_string(d.y) + "," + std::to_string(d.r) + "," + std::to_string(d.b) + "," + d.text + "|"; return s; }
int main(int argc, char** argv) {
    const std::string fx = std::string(argc > 1 ? argv[1] : "fx") + "/HG-2026-11-04.bin"; const double tick = 0.0005;
    std::vector<FR> v; { FILE* f = std::fopen(fx.c_str(), "rb"); FR r; while (f && std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); if (f) std::fclose(f); }
    CHECK(v.size() > 100000, "fixture");
    if (v.empty()) { std::printf("%d passed, %d failed\n", passes, fails); return 1; }
    zd::Engine E(zdp::forMarket("HG")); E.barSec = 180;
    for (auto& r : v) { zd::In e; e.t = (long long)r.t * 1000; e.act = r.a; e.side = r.s; e.px = r.px; e.sz = r.sz; e.oid = r.oid; E.feed(e); }
    char tmpl[] = "/tmp/zdmXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    mock::Host h; mock::current = &h; h.tick = (float)tick; h.seconds = 180; h.root = "HG"; h.symbol = "HGZ6"; h.paneBottom = 600; h.paneRight = 1500; h.scaleLeft = 1480; h.barW = 12;
    // candles over the slice; the mock clock = the fixture's (CT as UTC); the session = the fixture day's
    const long t0 = v.front().t / 180 * 180, t1 = v.back().t / 180 * 180 + 180;
    float lastPx = (float)(v.front().px * tick); size_t k = 0;
    for (long t = t0 + 180; t <= t1 + 180; t += 180) { mock::Bar b; b.time = (unsigned long)t; float hi = -1e9f, lo = 1e9f;
        for (; k < v.size() && v[k].t < t; ++k) if (v[k].a == 3) { const float p = (float)(v[k].px * tick); hi = std::max(hi, p); lo = std::min(lo, p); lastPx = p; }
        if (hi < lo) hi = lo = lastPx; b.high = hi; b.low = lo; b.open = b.close = lastPx; h.bars.push_back(b); }
    h.priceBase = lastPx - 0.02f; h.visFrom = 0; h.visTo = (int)h.bars.size() - 1;
    Ind* p = make(); h.draws.clear(); p->draw();
    CHECK(h.draws.empty(), "no journal = nothing drawn");
    // the journal the pane would write (today's session: the overlay looks for today and the 2 sessions before)
    std::time_t now = std::time(NULL); struct tm lt; localtime_r(&now, &lt); const std::string ses = zdh::sessionOf(lt);
    const std::string flex = zdh::flexDir(); zdh::makeDir(flex); zdh::makeDir(zdh::journalDir(flex));
    { std::ofstream f(zdh::journalPath(flex, "HG", 180, ses).c_str()); f << "H,0.0005,1.0.0\n"; for (auto& l : E.out) f << l.text << "\n"; }
    dispose(p); p = make(); h.draws.clear(); p->draw();
    ZmState* S = p->stateFor();
    CHECK(S && S->files.size() == 1 && S->files[0].J.z.size() == E.zones.size(), "the journal read (" + std::to_string(S ? S->files.size() : 0) + " files, " + std::to_string(S && !S->files.empty() ? S->files[0].J.z.size() : 0) + " zones of " + std::to_string(E.zones.size()) + ")");
    int boxes = 0, texts = 0, lines = 0; for (auto& d : h.draws) { boxes += d.type == "box"; texts += d.type == "text"; lines += d.type == "line"; }
    CHECK(S && S->zonesDrawn >= 1 && S->zonesDrawn <= 6, "zones drawn: " + std::to_string(S ? S->zonesDrawn : 0) + " (at most 3 per side)");
    CHECK(boxes == (int)S->zonesDrawn && texts <= boxes, "one band per zone, at most one label each (" + std::to_string(boxes) + " bands, " + std::to_string(texts) + " labels)");
    bool ov = false; for (size_t a = 0; a < S->labels.size(); ++a) for (size_t b = a + 1; b < S->labels.size(); ++b) { auto& A = S->labels[a]; auto& B = S->labels[b]; if (A.l < B.r && A.r > B.l && A.t < B.b && A.b > B.t) ov = true; }
    CHECK(!ov, "labels never overlap (" + std::to_string(S->labels.size()) + " drawn, " + std::to_string(S->labelsSkipped) + " gave way)");
    bool lab = true; for (auto& d : h.draws) if (d.type == "text" && (d.text.compare(0, 5, "Zone ") != 0 || d.text.find(" lots (") == std::string::npos)) lab = false;
    CHECK(lab, "label format 'Zone lo-hi | N lots (x) | oldest Nm ...'");
    CHECK(lines > 0, "diamonds / triangles / big-order lines drawn (" + std::to_string(lines) + " line segments)");
    const std::string s1 = sig(h); h.draws.clear(); p->draw(); const std::string s2 = sig(h);
    dispose(p); p = make(); h.draws.clear(); p->draw(); const std::string s3 = sig(h);
    CHECK(s1 == s2 && s1 == s3, "NO REPAINT: the same picture on every draw and after a restart");
    // settings: zones off -> no bands, no labels
    { ss::Values V = ss::defaults(zdset::spec(), ss::marketIndex("HG")); V.v[zdset::K_SHOW_ZONES] = 0; ss::Store st; st.S = &zdset::spec(); st.path = zdset::settingsPath(flex, "HG"); st.save(V); }
    dispose(p); p = make(); h.draws.clear(); p->draw(); int b2 = 0; for (auto& d : h.draws) b2 += d.type == "box" || d.type == "text";
    CHECK(b2 == 0, "Zones (price chart) off: no band, no label");
    dispose(p);
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
