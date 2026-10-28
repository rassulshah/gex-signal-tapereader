// test_sessionvwap_185.cpp -- lsSessionVWAP 1.8.5 (2026-10-28): every line / band / label stops at the Key Levels column's right edge
// (lsDealerProfile at the pane's LEFT edge, ColumnBoard cb::leftKeep - the rule BigBidAsk / DeltaProfile / LiquidityProfile / HodLod use);
// the lsHodLod table keep-out (1.6.3, KeepOutDraw.h) still works on top of it; no settings change (parameter version 11).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -ISessionVWAP_test_support -I. test_sessionvwap_185.cpp && ./a.out
// The mock PNT maps bar -> x (2 px per bar) and price -> y (H.pxBar > 0), and every drawn line records its pixel ends.
#include "irtsdk.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace mock { Host* current = NULL; }
#include "SessionVWAP.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

static RTDATE at(int y, int mo, int d, int h, int mi) { struct tm t = {}; t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; t.tm_min = mi; return (RTDATE)timegm(&t); }
static void bars(mock::Host& H)   // = test_sessionvwap_184: RTH, overnight, RTH (ES 3 min)
{
    std::vector<RTDATE> T;
    for (RTDATE t = at(2026, 10, 12, 8, 33); t <= at(2026, 10, 12, 15, 0); t += 180) T.push_back(t);
    for (RTDATE t = at(2026, 10, 12, 17, 3); t <= at(2026, 10, 13, 8, 30); t += 180) T.push_back(t);
    for (RTDATE t = at(2026, 10, 13, 8, 33); t <= at(2026, 10, 13, 10, 0); t += 180) T.push_back(t);
    const size_t n = T.size();
    for (int a = 0; a < 15; ++a) H.f[a].assign(n, 0.0f);
    for (int a = 0; a < 2; ++a) H.i[a].assign(n, 0UL);
    for (size_t i = 0; i < n; ++i) {
        float tp = 5800.0f + 12.0f * (float)std::sin((double)i / 9.0) + 0.05f * (float)i;
        if (T[i] >= at(2026, 10, 13, 8, 33)) { const int j = (int)((T[i] - at(2026, 10, 13, 8, 33)) / 180); tp = 5810.0f + (j < 12 ? 1.5f * (float)j : 18.0f - 2.5f * (float)(j - 12)); }
        H.i[(int)barDateTime][i] = T[i]; H.i[(int)barVolume][i] = 100 + (unsigned long)(i % 37) * 20;
        H.f[(int)barHigh][i] = tp + 1.5f; H.f[(int)barLow][i] = tp - 1.25f; H.f[(int)barClose][i] = tp + 0.25f; H.f[(int)barOpen][i] = tp;
    }
}
static long long nowMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static void redraw(SessionVWAP& x, mock::Host& H) { mock::current = &H; H.dialog = false; H.lines.clear(); x.draw(); }
static void openWin(SessionVWAP& x, mock::Host& H) { mock::current = &H; H.dialog = true; x.parmsLoad(); }
static void okWin(SessionVWAP& x, mock::Host& H) { mock::current = &H; x.parmsApply(); H.dialog = false; }
static void box(SessionVWAP& x, mock::Host& H, int row, bool on) { openWin(x, H); x.checkBox(x.P[row], on); x.parmsUpdt((unsigned)x.P[row]); okWin(x, H); }
static int minX(const mock::Host& H) { int m = 1 << 30; for (size_t i = 0; i < H.lines.size(); ++i) m = std::min(m, std::min(H.lines[i].h0, H.lines[i].h1)); return m; }
static std::string findStatus(const std::string& dir)
{
    std::string cmd = "find '" + dir + "' -name '*SessionVWAP-ES-*.status.txt' 2>/dev/null | head -1", path;
    FILE* p = popen(cmd.c_str(), "r"); if (!p) return std::string();
    char buf[1024]; if (fgets(buf, sizeof(buf), p)) { path = buf; while (!path.empty() && (path.back() == '\n' || path.back() == '\r')) path.pop_back(); }
    pclose(p); if (path.empty()) return std::string();
    std::ifstream f(path.c_str()); std::stringstream s; s << f.rdbuf(); return s.str();
}

static void testClip()
{
    int x1 = 100, y1 = 50, x2 = 300, y2 = 150;
    CHECK(svk::clipLeft(x1, y1, x2, y2, 200) && x1 == 200 && y1 == 100 && x2 == 300 && y2 == 150, "clipLeft: left end moved to the edge, y on the line");
    x1 = 300; y1 = 150; x2 = 100; y2 = 50;
    CHECK(svk::clipLeft(x1, y1, x2, y2, 200) && x1 == 300 && x2 == 200 && y2 == 100, "clipLeft: either direction");
    x1 = 10; y1 = 0; x2 = 199; y2 = 0;
    CHECK(!svk::clipLeft(x1, y1, x2, y2, 200), "clipLeft: wholly left = not drawn");
    x1 = 200; y1 = 7; x2 = 250; y2 = 9;
    CHECK(svk::clipLeft(x1, y1, x2, y2, 200) && x1 == 200 && y1 == 7 && x2 == 250 && y2 == 9, "clipLeft: on the edge = unchanged");
    x1 = -500; x2 = 500; y1 = y2 = 3;
    CHECK(svk::clipLeft(x1, y1, x2, y2, -32768) && x1 == -500, "clipLeft: no keep-out (-32768) = unchanged");
}

static void testAll(const std::string& dir)
{
    CHECK(svset::spec().paramVersion == 11, "parameter version unchanged (11): no saved set is reset by 1.8.5");
    mock::declared.clear();
    SessionVWAP x;
    mock::Host H; H.root = "ES"; mock::current = &H;
    CHECK(x.setup() == RTX_OK, "setup");
    bars(H); x.fill(0);
    const int n = (int)H.f[(int)barClose].size();
    H.pxBar = 2; H.bar0 = n - 301;                                  // visible bars (mock: n-300 .. n-1, joined from n-301) -> x 0 .. 600; pane 0 .. 800
    const int rows[8] = { svs::R_VWAP, svs::R_P1, svs::R_M1, svs::R_P2, svs::R_M2, svs::R_ON, svs::R_PRIOR, svs::R_PIVOT };
    for (int r = 0; r < 8; ++r) box(x, H, rows[r], true);           // every line ticked
    const std::string sym = "ES", chart = H.chart; const long spb = H.spb;
    cb::clearKeys(sym, spb, chart); bdk::clearHodLod(sym, spb, chart);

    // ---- 1. no Key Levels column: lines run to the pane's left edge (as 1.8.4)
    redraw(x, H);
    const std::vector<mock::Line> base = H.lines;
    CHECK(!base.empty() && minX(H) <= 2, "no column: lines start at the pane edge (min x " << minX(H) << ", " << base.size() << " segments)");

    // ---- 2. the Key Levels column (230 px) on this chart: keep = pane.left + 1 + 230 + 2 = 233
    cb::publishKeys(sym, spb, chart, nowMs(), 230);
    const int keep = 233;
    redraw(x, H);
    int whollyRight = 0, crossing = 0;
    for (size_t i = 0; i < base.size(); ++i) { const int lo = std::min(base[i].h0, base[i].h1), hi = std::max(base[i].h0, base[i].h1);
        if (lo >= keep) ++whollyRight; else if (hi >= keep) ++crossing; }
    int startAtEdge = 0; bool yOk = true;
    for (size_t i = 0; i < H.lines.size(); ++i) { const mock::Line& l = H.lines[i];
        if (std::min(l.h0, l.h1) == keep) ++startAtEdge;
        if (std::abs(l.v0 - l.v1) > 2000) yOk = false; }
    CHECK(minX(H) >= keep, "column on: nothing drawn left of its right edge (min x " << minX(H) << " >= " << keep << ")");
    CHECK((int)H.lines.size() == whollyRight + crossing, "column on: every segment right of the edge kept, each crossing one cut once (" << H.lines.size() << " = " << whollyRight << " + " << crossing << ")");
    CHECK(crossing > 0 && startAtEdge >= crossing && yOk, "column on: the lines still start right AT the edge (" << startAtEdge << " start at x " << keep << ")");
    { std::vector<mock::Line> A, B;   // the segments wholly right of the edge: drawn exactly as before (same order, pixels, colours)
      for (size_t i = 0; i < base.size(); ++i) if (std::min(base[i].h0, base[i].h1) > keep) A.push_back(base[i]);
      for (size_t i = 0; i < H.lines.size(); ++i) if (std::min(H.lines[i].h0, H.lines[i].h1) > keep) B.push_back(H.lines[i]);
      bool same = A.size() == B.size() && !A.empty();
      for (size_t i = 0; same && i < A.size(); ++i) same = A[i].h0 == B[i].h0 && A[i].v0 == B[i].v0 && A[i].h1 == B[i].h1 && A[i].v1 == B[i].v1 && A[i].c == B[i].c && A[i].w == B[i].w;
      CHECK(same, "column on: the rest of every line unchanged (" << A.size() << " segments: pixels, colours, widths)"); }
    { int pivR = 0, pivB = 0; for (size_t i = 0; i < H.lines.size(); ++i) if (H.lines[i].c == 0x22C55E || H.lines[i].c == 0xEF4444) ++pivR;
      for (size_t i = 0; i < base.size(); ++i) if ((base[i].c == 0x22C55E || base[i].c == 0xEF4444) && std::max(base[i].h0, base[i].h1) >= keep) ++pivB;
      CHECK(pivR == pivB && pivR > 0, "column on: the pivot VWAP is cut the same way (" << pivR << " segments)"); }
    mock::current = &H; x.fill(0);
    { const std::string st = findStatus(dir);
      CHECK(st.find("VERSION,1.8.5\n") != std::string::npos && st.find("LEFT_KEEP,233 (ColumnBoard ") != std::string::npos, "status: VERSION 1.8.5, LEFT_KEEP 233"); }

    // ---- 3. + the lsHodLod table at x 400..460: cut there too (1.6.3 keep-out still works), still nothing left of the column
    bdk::Box tb = bdk::mk(400, 0, 460, 600);
    bdk::publishHodLod(sym, spb, chart, nowMs(), tb);
    redraw(x, H);
    bool clear = true;
    for (size_t i = 0; i < H.lines.size(); ++i) { const mock::Line& l = H.lines[i]; const int lo = std::min(l.h0, l.h1), hi = std::max(l.h0, l.h1);
        if (lo <= 460 + bdk::KEEP_MARGIN && hi >= 400 - bdk::KEEP_MARGIN) clear = false; }
    CHECK(clear && !H.lines.empty(), "HOD/LOD table on: no line drawn over it");
    CHECK(minX(H) >= keep, "HOD/LOD table on: still nothing left of the Key Levels column (min x " << minX(H) << ")");
    bdk::clearHodLod(sym, spb, chart);

    // ---- 4. the column removed (its stamp older than 10 s): lines back to the pane edge
    cb::publishKeys(sym, spb, chart, nowMs() - 20000, 230);
    redraw(x, H);
    CHECK(H.lines.size() == base.size() && minX(H) <= 2, "column gone (stale): lines back to the pane edge, as before");
    cb::clearKeys(sym, spb, chart);
    redraw(x, H);
    CHECK(H.lines.size() == base.size(), "column cleared: as before");

    // ---- 5. another chart's column does not move this chart's lines
    cb::publishKeys(sym, spb, "ES 1 Min", nowMs(), 230);
    redraw(x, H);
    CHECK(H.lines.size() == base.size() && minX(H) <= 2, "another chart's Key Levels column: this chart unchanged");
    cb::clearKeys(sym, spb, "ES 1 Min");
    mock::current = &H; x.done();
}
static void testVersion()
{
    std::ifstream f("SessionVWAP.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
    const size_t a = src.find("setVersion(\""), b = src.find("SV_VERSION = \"");
    const std::string v1 = src.substr(a + 12, src.find('"', a + 12) - a - 12), v2 = src.substr(b + 14, src.find('"', b + 14) - b - 14);
    CHECK(v1 == v2 && v1 == "1.8.5" && std::string(svset::PLUGIN_VERSION) == v1, "versions agree: 1.8.5 (setVersion literal = SV_VERSION = PLUGIN_VERSION)");
}

int main()
{
    char tmpl[] = "/tmp/sv185XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 1;
    const std::string profile = std::string(dir) + "/u"; setenv("USERPROFILE", profile.c_str(), 1);
    testClip(); testAll(dir); testVersion();
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::cout << "SessionVWAP 1.8.5 Key Levels column keep-out: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
