// t_parity.cpp -- lsZoneDefense: plugin path vs engine, line for line, on any fixture (fx_export.py), + time / memory per session.
// usage: ./tp FIXTURE.bin MKT TICK   (prints one PARITY line and one PERF line; exit 1 on a mismatch)
// Build: g++ -std=c++17 -O2 -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock t_parity.cpp -o tp
#include "ZoneDefense.cpp"
#include <chrono>
#include <cctype>
#include <sstream>
#include <unistd.h>
typedef ZoneDefense Ind;
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
namespace mock { Host* current = nullptr; Feed feed; }
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
#pragma pack(push, 1)
struct FR { int t; signed char a, s; int px; unsigned sz; int oid; };
#pragma pack(pop)
static long long& NOW = off::off_test_now_ms;
static double nowS() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
// the live clock: whole-second RTDATE stamps + the arrival ms inside the second -> compare the decisions with every time field
// (13-digit ms) floored to its second; count the lines whose ms differ (same decision, a different arrival offset)
static std::string sec(const std::string& l) { std::string o, f; std::stringstream ss(l); bool first = true;
    while (std::getline(ss, f, ',')) { if (f.size() == 13 && std::isdigit((unsigned char)f[0])) f = f.substr(0, 10) + "000"; o += (first ? "" : ",") + f; first = false; } return o; }
static mock::Bar bar(unsigned long t, float px) { mock::Bar b; b.time = t; b.close = b.open = b.low = b.high = px; b.volume = 100; b.rows.push_back(mock::Price(px, 50, 50, 100)); return b; }
int main(int argc, char** argv) {
    if (argc < 4) return 2;
    const std::string mkt = argv[2]; const double tick = std::atof(argv[3]);
    std::vector<FR> v; { FILE* f = std::fopen(argv[1], "rb"); FR r; while (f && std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); if (f) std::fclose(f); }
    if (v.empty()) { std::printf("no fixture\n"); return 2; }
    // the engine alone (= zdrun / the nightly)
    double t0 = nowS(); zd::Engine ref(zdp::forMarket(mkt)); ref.barSec = 180;
    for (auto& r : v) { zd::In e; e.t = (long long)r.t * 1000; e.act = r.a; e.side = r.s; e.px = r.px; e.sz = r.sz; e.oid = r.oid; ref.feed(e); }
    const double engS = nowS() - t0;
    // the plugin path on the mock host
    char tmpl[] = "/tmp/zdpXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    mock::Host h; mock::current = &h; h.tick = (float)tick; h.seconds = 180; h.root = mkt == "EU" ? "6E" : mkt; h.symbol = mkt + "Z6"; h.paneBottom = 160; h.paneRight = 900; h.scaleLeft = 880; h.barW = 12;
    const unsigned long tb = (unsigned long)(v[0].t / 180 * 180);
    for (int k = -40; k <= 0; ++k) h.bars.push_back(bar(tb + 180UL * k, (float)(v[0].px * tick)));
    Ind* p = static_cast<Ind*>(CreateExtension()); p->setup(); NOW += 1000; p->calc(0); p->calc(0);
    double drawS = 0; long draws = 0; t0 = nowS(); size_t i = 0;
    while (i < v.size()) {
        const unsigned long end = (unsigned long)((v[i].t / 180 + 1) * 180);
        for (; i < v.size() && (unsigned long)v[i].t < end; ++i) mock::feed.push((unsigned char)v[i].a, v[i].s == 0 ? 1 : 0, (float)(v[i].px * tick), v[i].sz, (RTDATE)v[i].t, v[i].oid);
        h.bars.push_back(bar(end + 180, (float)(v[i - 1].px * tick))); h.visFrom = std::max(0, (int)h.bars.size() - 60); h.visTo = (int)h.bars.size() - 1;
        // ARRIVAL = 0: the whole candle is read at one clock instant, so every event keeps its own whole second (as the reference);
        // ARRIVAL = 1: 250 ms per read like the live clock (the plugin then adds the arrival ms inside each second)
        const int adv = std::getenv("ARRIVAL") && std::atoi(std::getenv("ARRIVAL")) ? 250 : 0;
        NOW += 1000; p->calc(0); NOW += adv; p->calc(0);
        for (int k = 0; k < 200 && mock::feed.at < mock::feed.ev.size(); ++k) { NOW += adv; p->calc(0); }
        if (h.bars.size() > 400) { h.bars.erase(h.bars.begin(), h.bars.begin() + 100); }
        h.draws.clear(); const double d0 = nowS(); p->draw(); drawS += nowS() - d0; ++draws;
        if (mock::feed.ev.size() > 2000000 && mock::feed.at == mock::feed.ev.size()) { mock::feed.ev.clear(); mock::feed.at = 0; }
    }
    const double plugS = nowS() - t0;
    auto* S = p->testState(); auto* e = S ? p->ext(*S) : nullptr;
    p->calc(0);
    std::ifstream in(e ? e->path.c_str() : ""); std::vector<std::string> got; std::string l; while (std::getline(in, l)) if (!l.empty() && l[0] != 'H') got.push_back(l);
    size_t same = 0, msDiff = 0; while (same < got.size() && same < ref.out.size() && sec(got[same]) == sec(ref.out[same].text)) { if (got[same] != ref.out[same].text) ++msDiff; ++same; }
    const bool ok = got.size() == ref.out.size() && same == got.size();
    int zn = 0, zt = 0, zr = 0, zs = 0; for (auto& o : ref.out) { zn += o.text[1] == 'N'; zt += o.text[1] == 'T'; zr += o.text[1] == 'R'; zs += o.text[1] == 'S'; }
    if (const char* dp = std::getenv("ZD_DUMP")) { FILE* df = std::fopen(dp, "w"); if (df) { for (size_t i = 0; i < ref.out.size(); ++i) std::fprintf(df, "%s\n", ref.out[i].text.c_str()); std::fclose(df); } }
    std::printf("PARITY %s %s: %s - plugin %zu lines, engine %zu, identical %zu, of them %zu with a sub-second arrival offset (zones %d, touches %d, pressure %d, sweeps %d)%s\n", mkt.c_str(), argv[1], ok ? "OK" : "MISMATCH",
                got.size(), ref.out.size(), same, msDiff, zn, zt, zr, zs, (!ok && same < got.size() && same < ref.out.size()) ? ("\n  plugin " + got[same] + "\n  engine " + ref.out[same].text).c_str() : "");
    std::printf("PERF %s: %zu events, engine %.3f us/event, plugin path (feed + MboPane + engine) %.3f us/event, draw %.3f ms avg over %ld, max book %zu orders, max ended %zu, max live zones %zu, zones %zu\n",
                mkt.c_str(), v.size(), engS * 1e6 / v.size(), plugS * 1e6 / v.size(), draws ? drawS * 1e3 / draws : 0.0, draws, ref.maxBook, ref.maxEnded, ref.maxLive, ref.zones.size());
    return ok ? 0 : 1;
}
