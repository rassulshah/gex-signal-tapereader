// test_sessionvwap_160.cpp -- lsSessionVWAP 1.6.0 (2026-10-18): the Market State post SRC "VWAP" (MARKET-STATE-SPEC 1.0).
//   g++ -std=c++17 -Wall -Wextra -Werror -DSVS_TEST_ALL_ON_DEFAULTS -fsanitize=address,undefined -ISessionVWAP_test_support -I. test_sessionvwap_160.cpp && ./a.out
// Part 1 = svl::vwapFacts alone (pos bands, slope rule, '?'). Part 2 = the REAL SessionVWAP.cpp on the mock SDK: the post equals the
// outputs IRT holds at the last CLOSED bar (RTH and overnight), the bar stamp, re-post every 10 s only, decided once per bar, a second
// chart has its own entry, done() clears it; the version literals.
#include "irtsdk.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <unistd.h>

namespace mock { Host* current = NULL; }
#include "SessionVWAP.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

static void testFacts()
{
    const float v[5] = { 100.0f, 102.0f, 98.0f, 104.0f, 96.0f };
    std::map<std::string, std::string> kv = svl::vwapFacts(v, 101.0, 99.5, 2);
    const std::set<std::string> want = { "vwap", "sd1_hi", "sd1_lo", "sd2_hi", "sd2_lo", "pos", "slope" };
    std::set<std::string> got; for (const auto& p : kv) got.insert(p.first);
    CHECK(got == want, "VWAP keys = the spec's 7");
    CHECK(kv["vwap"] == "100.00" && kv["sd1_hi"] == "102.00" && kv["sd1_lo"] == "98.00" && kv["sd2_hi"] == "104.00" && kv["sd2_lo"] == "96.00", "lines");
    CHECK(kv["pos"] == "above" && kv["slope"] == "up", "101 = above; VWAP +0.5 over 5 bars (> 0.1 SD = 0.2) = up");
    const double px[8] = { 104.0, 103.0, 102.0, 100.0, 99.0, 98.0, 97.0, 96.0 };
    const char* pos[8] = { "above2", "above1", "above1", "above", "below", "below1", "below1", "below2" };
    for (int i = 0; i < 8; ++i) CHECK(svl::vwapFacts(v, px[i], 0, 2)["pos"] == pos[i], "pos at " << px[i] << " = " << pos[i] << " got " << svl::vwapFacts(v, px[i], 0, 2)["pos"]);
    CHECK(svl::vwapFacts(v, 101, 100.15, 2)["slope"] == "flat" && svl::vwapFacts(v, 101, 100.3, 2)["slope"] == "down" && svl::vwapFacts(v, 101, 0, 2)["slope"] == "?", "slope: within 0.1 SD flat, -0.3 down, unknown '?'");
    const float z[5] = { 0, 0, 0, 0, 0 };
    std::map<std::string, std::string> q = svl::vwapFacts(z, 101, 100, 2); bool allQ = true; for (const auto& p : q) if (p.second != "?") allQ = false;
    CHECK(allQ, "no VWAP yet (0 = no line): every fact '?'");
    const float flatV[5] = { 100.0f, 100.0f, 100.0f, 100.0f, 100.0f };   // one bar: SD 0
    CHECK(svl::vwapFacts(flatV, 100, 100, 2)["slope"] == "flat" && svl::vwapFacts(flatV, 100, 100, 2)["pos"] == "above2", "zero SD: no division, flat");
    CHECK(svl::decOf("ES") == 2 && svl::decOf("GC") == 1 && svl::decOf("HG") == 4 && svl::decOf("NG") == 3 && svl::decOf("EU") == 5 && svl::decOf("CL") == 2, "decimals per market");
}

static RTDATE at(int y, int mo, int d, int h, int mi) { struct tm t = {}; t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; t.tm_min = mi; return (RTDATE)timegm(&t); }
static void bars(mock::Host& H, RTDATE to)
{
    std::vector<RTDATE> T;
    for (RTDATE t = at(2026, 10, 12, 8, 33); t <= at(2026, 10, 12, 15, 0); t += 180) T.push_back(t);
    for (RTDATE t = at(2026, 10, 12, 17, 3); t <= to && t <= at(2026, 10, 13, 8, 30); t += 180) T.push_back(t);
    for (RTDATE t = at(2026, 10, 13, 8, 33); t <= to; t += 180) T.push_back(t);
    const size_t n = T.size();
    for (int a = 0; a < 14; ++a) H.f[a].assign(n, 0.0f);
    for (int a = 0; a < 2; ++a) H.i[a].assign(n, 0UL);
    for (size_t i = 0; i < n; ++i) {
        const float tp = 5800.0f + 12.0f * (float)std::sin((double)i / 9.0) + 0.05f * (float)i;
        H.i[(int)barDateTime][i] = T[i]; H.i[(int)barVolume][i] = 100 + (unsigned long)(i % 37) * 20;
        H.f[(int)barHigh][i] = tp + 1.5f; H.f[(int)barLow][i] = tp - 1.25f; H.f[(int)barClose][i] = tp + 0.25f;
    }
}
static std::string f2(float v) { char b[32]; std::snprintf(b, sizeof b, "%.2f", (double)v); return b; }
static long long ctOf(RTDATE t) { struct tm m; time_t tt = (time_t)t; gmtime_r(&tt, &m); return (long long)svl::daysFromCivil(m.tm_year + 1900, m.tm_mon + 1, m.tm_mday) * 86400 + m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec; }

static void testPlugin()
{
    mock::declared.clear();
    SessionVWAP x;
    mock::Host H; H.root = "ESZ6"; mock::current = &H;
    CHECK(x.setup() == RTX_OK, "setup");
    // RTH (Tue 10:00): the post = outputs 0-4 at n - 2
    bars(H, at(2026, 10, 13, 10, 0)); x.fill(0); x.draw();
    const int n = (int)H.f[(int)barClose].size(), lc = n - 2;
    mst::Facts F = mst::read("VWAP", "ESZ6", 180, "ES 3 Min", -1);
    CHECK(F.ok, "posted on the first draw");
    CHECK(F.bar == ctOf((RTDATE)H.i[(int)barDateTime][(size_t)lc]), "the bar = the last CLOSED bar's CT stamp (not the forming one)");
    CHECK(mst::getS(F, "vwap") == f2(H.f[0][(size_t)lc]) && mst::getS(F, "sd1_hi") == f2(H.f[1][(size_t)lc]) && mst::getS(F, "sd2_lo") == f2(H.f[4][(size_t)lc]), "RTH: the plugin's own outputs at n - 2");
    const double c = H.f[(int)barClose][(size_t)lc];
    const std::string want = c >= H.f[3][(size_t)lc] ? "above2" : c >= H.f[1][(size_t)lc] ? "above1" : c >= H.f[0][(size_t)lc] ? "above" : c <= H.f[4][(size_t)lc] ? "below2" : c <= H.f[2][(size_t)lc] ? "below1" : "below";
    CHECK(mst::getS(F, "pos") == want && mst::getS(F, "slope") != "?", "pos from the close; slope known (5 bars in the session)");
    // decided once: the forming bar moves, the post for the closed bar does not
    const std::string before = mst::encode(0, 0, F.kv);
    H.f[(int)barClose][(size_t)n - 1] += 50; H.f[(int)barHigh][(size_t)n - 1] += 50; x.fill(n - 1); x.draw();
    CHECK(mst::encode(0, 0, mst::read("VWAP", "ESZ6", 180, "ES 3 Min", -1).kv) == before, "the forming bar changes nothing (no repaint)");
    // re-post: not before 10 s (same stamp), so the entry ages and is refreshed
    std::string raw1; bb::bbGet(mst::keyOf("VWAP", "ESZ6", 180, "ES 3 Min"), raw1); x.draw(); std::string raw2; bb::bbGet(mst::keyOf("VWAP", "ESZ6", 180, "ES 3 Min"), raw2);
    CHECK(raw1 == raw2, "no re-post within 10 s");
    {   // (1.6.3, Rassul 16:05 CT 2026-10-21 "fix this, it should be solid") the lsHodLod table on this chart: no line part painted over it
        // (this mock puts every PNT at x 400, y 100: a table there hides every segment); the posted values are unchanged; another chart = no effect
        mock::current = &H; H.lines.clear(); x.draw(); const size_t l0 = H.lines.size();
        bdk::publishHodLod("ESZ6", 180, "ES 3 Min", 1, bdk::mk(390, 90, 410, 110));
        H.lines.clear(); x.draw();
        CHECK(l0 > 0 && H.lines.empty(), "[keep-out] lines under the table are not painted");
        CHECK(mst::encode(0, 0, mst::read("VWAP", "ESZ6", 180, "ES 3 Min", -1).kv) == before, "[keep-out] the posted VWAP facts are unchanged");
        bdk::clearHodLod("ESZ6", 180, "ES 3 Min"); bdk::publishHodLod("ESZ6", 180, "ES 3 Min #2", 1, bdk::mk(390, 90, 410, 110));
        H.lines.clear(); x.draw(); CHECK(H.lines.size() == l0, "[keep-out] a table on another chart: every line drawn");
        bdk::clearHodLod("ESZ6", 180, "ES 3 Min #2");
    }
    // overnight (Mon 23:00): outputs 5-9
    mock::Host O; O.root = "ESZ6"; O.chart = "ES 3 Min overnight"; mock::current = &O;
    bars(O, at(2026, 10, 12, 23, 0)); x.fill(0); x.draw();
    const int m = (int)O.f[(int)barClose].size(), ol = m - 2;
    mst::Facts G = mst::read("VWAP", "ESZ6", 180, "ES 3 Min overnight", -1);
    CHECK(G.ok && mst::getS(G, "vwap") == f2(O.f[5][(size_t)ol]) && mst::getS(G, "sd2_hi") == f2(O.f[8][(size_t)ol]), "overnight: the O/N outputs at n - 2");
    CHECK(mst::read("VWAP", "ESZ6", 180, "ES 3 Min", -1).ok, "the first chart keeps its own entry");
    // a new closed bar: a new decision at once
    mock::current = &H; bars(H, at(2026, 10, 13, 10, 3)); x.fill(0); x.draw();
    mst::Facts F2 = mst::read("VWAP", "ESZ6", 180, "ES 3 Min", -1);
    CHECK(F2.bar == ctOf((RTDATE)H.i[(int)barDateTime][H.f[(int)barClose].size() - 2]) && F2.bar > F.bar, "a new bar is posted at once");
    // removal clears
    x.done(); CHECK(!mst::read("VWAP", "ESZ6", 180, "ES 3 Min", -1).ok, "done(): the entry is cleared");
    mock::current = &O; x.destroy(); CHECK(!mst::read("VWAP", "ESZ6", 180, "ES 3 Min overnight", -1).ok, "destroy(): cleared");
}
static void testVersion()
{
    std::ifstream f("SessionVWAP.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
    CHECK(src.find("static const char* SV_VERSION = \"1.8.5\";") != std::string::npos && src.find("p->setVersion(\"1.8.5\");") != std::string::npos, "SV_VERSION == setVersion == 1.8.3 (1.8.3 = pivot-only defaults + connected pivot line; 1.8.2 = settings reset + window = drawing; 1.8.1 = native per-indicator settings; 1.6.2 = the settings window guard, 1.6.3 = the HOD/LOD keep-out)");
}
int main()
{
    char tmpl[] = "/tmp/sv160XXXXXX"; char* dir = mkdtemp(tmpl);
    if (!dir) { std::cerr << "no temp dir\n"; return 1; }
    setenv("USERPROFILE", (std::string(dir) + "/u").c_str(), 1);
    testFacts(); testPlugin(); testVersion();
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::cout << "SessionVWAP 1.6.0 market state: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
