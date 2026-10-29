// test_sessionvwap_187.cpp -- lsSessionVWAP 1.8.7 (2026-10-29): pivot VWAP ON by default, yellow 0xFACC15 2 px, no bands, IRT ZigZag anchors
// (PivotVwapLogic 1.1.0); parameter version 12 resets every older set once; the ADD path (setup -> parmsInit(old) or none -> first fill -> first draw).
// Helpers copied from test_sessionvwap_184.cpp. Was:
// test_sessionvwap_184.cpp -- lsSessionVWAP 1.8.4 (2026-10-28 16:26 CT Rassul "the chart is extremely messy" + remove the Pivot VWAP line:
// fresh load draws NOTHING, parameter version 11 resets every v10 (1.8.3 pivot-only) and older set once to all-off, the window shows all
// unticked, ticking the pivot box draws the connected pivot line). Was 1.8.3 (2026-10-28, Rassul 14:05 / 14:08 CT "only 1 vwap level should be displayed" = the
// 3-bar anchored pivot VWAP; 14:09 CT "make sure that the vwap is connected and not broken").
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -ISessionVWAP_test_support -I. test_sessionvwap_184.cpp && ./a.out
// (compiled WITHOUT -DSVS_TEST_ALL_ON_DEFAULTS: the plugin's real defaults.) Covers: fresh load = only the pivot VWAP drawn, window
// shows it; parameter version 10 - his 1.8.1 record (v8) and a 1.8.2 all-on set (v9) handed to parmsInit are replaced by the new
// defaults, a v10 set is left alone; every box adds / removes only its own line; window == chart; save -> restart round trip; the
// pivot VWAP is ONE connected line per RTH session (segment across every anchor change, segments = bars with a value - 1).
#include "irtsdk.h"
#include <cmath>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace mock { Host* current = NULL; }
#include "SessionVWAP.cpp"

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

static RTDATE at(int y, int mo, int d, int h, int mi) { struct tm t = {}; t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; t.tm_min = mi; return (RTDATE)timegm(&t); }
static void bars(mock::Host& H)
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
static int segments(mock::Host& H, int k)
{
    const int n = (int)H.f[(int)barClose].size(); int a = n > 300 ? n - 300 : 0, b = n - 1; if (a > 0) a--; int c = 0;
    for (int i = a + 1; i <= b; ++i) { const float y0 = H.f[k][(size_t)i - 1], y1 = H.f[k][(size_t)i]; if (y0 > 0 && y1 > 0) ++c; }
    return c;
}
typedef std::map<std::pair<unsigned long, int>, int> Drawn;
static Drawn redraw(SessionVWAP& x, mock::Host& H) { mock::current = &H; H.dialog = false; H.lines.clear(); x.draw();
    Drawn m; for (size_t i = 0; i < H.lines.size(); ++i) m[std::make_pair((unsigned long)H.lines[i].c, (int)H.lines[i].w)]++; return m; }
static int at2(const Drawn& m, unsigned long c, int w) { Drawn::const_iterator i = m.find(std::make_pair(c, w)); return i == m.end() ? 0 : i->second; }
// the window: open (parmsLoad), change, OK
static void openWin(SessionVWAP& x, mock::Host& H) { mock::current = &H; H.dialog = true; x.parmsLoad(); }
static void okWin(SessionVWAP& x, mock::Host& H) { mock::current = &H; x.parmsApply(); H.dialog = false; }
static void box(SessionVWAP& x, mock::Host& H, int row, bool on) { openWin(x, H); x.checkBox(x.P[row], on); x.parmsUpdt((unsigned)x.P[row]); okWin(x, H); }
// what the window shows, as a style (opened fresh, then closed with Cancel = nothing applied)
static svs::Style shown(SessionVWAP& x, mock::Host& H)
{
    openWin(x, H);
    svs::Style s;
    const int show[5] = { svs::R_VWAP, svs::R_P1, svs::R_M1, svs::R_P2, svs::R_M2 }, col[5] = { svs::R_VWAP_COL, svs::R_P1_COL, svs::R_M1_COL, svs::R_P2_COL, svs::R_M2_COL };
    for (int k = 0; k < 5; ++k) { s.show[k] = x.isBoxChecked(x.P[show[k]]) != 0; s.col[k] = x.getParameterColor(x.P[col[k]]) & 0xFFFFFFUL; }
    s.vwapW = svs::W_MIN + x.getListIndex(x.P[svs::R_VWAP_W]); s.bandW = svs::W_MIN + x.getListIndex(x.P[svs::R_BAND_W]);
    s.on = x.isBoxChecked(x.P[svs::R_ON]) != 0; s.prior = x.isBoxChecked(x.P[svs::R_PRIOR]) != 0; s.priorCol = x.getParameterColor(x.P[svs::R_PRIOR_COL]) & 0xFFFFFFUL;
    s.pivot = x.isBoxChecked(x.P[svs::R_PIVOT]) != 0; s.pivUp = x.getParameterColor(x.P[svs::R_PIV_UP]) & 0xFFFFFFUL; s.pivDn = x.getParameterColor(x.P[svs::R_PIV_DN]) & 0xFFFFFFUL;
    s.pivW = svs::W_MIN + x.getListIndex(x.P[svs::R_PIV_W]);
    H.dialog = false;
    return s;
}
static std::string drawnStyle(SessionVWAP& x, mock::Host& H) { mock::current = &H; SVState* S = x.state(false); return S ? S->parmsStyle : std::string(); }
static std::string why(SessionVWAP& x, mock::Host& H) { mock::current = &H; SVState* S = x.state(false); return S ? S->parmsWhy : std::string(); }
// the window == the chart: what the window shows is what the last draw used
static bool agree(SessionVWAP& x, mock::Host& H) { redraw(x, H); const std::string d = drawnStyle(x, H); return svp::describe(shown(x, H)) == d && !d.empty(); }
// Rassul's ES record as the 11:25-11:26 window left it (screenshot): VWAP off / black / 1 px, bands off with his colours, band 1 px,
// overnight off, prior off (grey), pivot on (green / red, 2 px)
static void hisRecord(SessionVWAP& x, mock::Host& H)
{
    H.val = mock::declared;
    H.val[(size_t)x.P[svs::R_VWAP]].b = false; H.val[(size_t)x.P[svs::R_VWAP_COL]].c = 0; H.val[(size_t)x.P[svs::R_VWAP_W]].list = 0;
    const int show[4] = { svs::R_P1, svs::R_M1, svs::R_P2, svs::R_M2 }; for (int k = 0; k < 4; ++k) H.val[(size_t)x.P[show[k]]].b = false;
    H.val[(size_t)x.P[svs::R_BAND_W]].list = 0; H.val[(size_t)x.P[svs::R_ON]].b = false; H.val[(size_t)x.P[svs::R_PRIOR]].b = false;
}
static std::string readStatus(const std::string& dir)
{
    std::string out; DIR* d = opendir(dir.c_str()); if (!d) return out;
    for (dirent* e; (e = readdir(d)) != NULL; ) { const std::string n = e->d_name;
        if (n.find("SessionVWAP-ES-") != std::string::npos && n.size() > 11 && n.substr(n.size() - 11) == ".status.txt") {
            std::ifstream f((dir + "/" + n).c_str()); std::stringstream s; s << f.rdbuf(); out = s.str(); } }
    closedir(d); return out;
}


// one RTH session (ES 2026-10-13 08:33-15:00, 3 min, 130 bars - all visible) that zig-zags: several 3-bar pivots = anchor changes
static void zigzag(mock::Host& H)
{
    std::vector<RTDATE> T;
    for (RTDATE t = at(2026, 10, 13, 8, 33); t <= at(2026, 10, 13, 15, 0); t += 180) T.push_back(t);
    const size_t n = T.size();
    for (int a = 0; a < 15; ++a) H.f[a].assign(n, 0.0f);
    for (int a = 0; a < 2; ++a) H.i[a].assign(n, 0UL);
    for (size_t i = 0; i < n; ++i) {
        const float tp = 5800.0f + 10.0f * (float)std::sin((double)i / 3.0) + 0.1f * (float)i;
        H.i[(int)barDateTime][i] = T[i]; H.i[(int)barVolume][i] = 100 + (unsigned long)(i % 37) * 20;
        H.f[(int)barHigh][i] = tp + 1.5f; H.f[(int)barLow][i] = tp - 1.25f; H.f[(int)barClose][i] = tp + 0.25f; H.f[(int)barOpen][i] = tp;
    }
}
static void allOn(SessionVWAP& x, mock::Host& H)   // a 1.8.2 (version 9) saved set: every line on
{
    H.val = mock::declared;
    const int rows[8] = { svs::R_VWAP, svs::R_P1, svs::R_M1, svs::R_P2, svs::R_M2, svs::R_ON, svs::R_PRIOR, svs::R_PIVOT };
    for (int r = 0; r < 8; ++r) H.val[(size_t)x.P[rows[r]]].b = true;
}
static bool isDeclared(const mock::Host& H) { bool same = true;
    for (size_t i = 0; i < H.val.size(); ++i) same = same && H.val[i].b == mock::declared[i].b && H.val[i].c == mock::declared[i].c && H.val[i].list == mock::declared[i].list;
    return same; }


static void testAll(const std::string& dir)
{
    (void)dir; (void)&zigzag; (void)&readStatus; (void)&box; (void)&segments;
    const svs::Style D;
    CHECK(svset::spec().paramVersion == 12, "(1.8.6) parameter version 12");
    CHECK(!D.show[0] && !D.show[1] && !D.show[2] && !D.show[3] && !D.show[4] && !D.on && !D.prior && D.pivot && D.pivW == 2
          && D.pivUp == 0xFACC15 && D.pivDn == 0xFACC15, "(1.8.6) defaults: ONLY the pivot VWAP, yellow 2 px; no session VWAP, no bands, no overnight / prior");
    mock::declared.clear();
    SessionVWAP x;
    mock::Host H; H.root = "ES"; mock::current = &H;
    CHECK(x.setup() == RTX_OK && mock::declared.size() == (size_t)svs::R_COUNT, "ADD: setup declares 19 rows");
    CHECK(mock::declared[(size_t)x.P[svs::R_PIVOT]].b, "ADD: the pivot box is declared ticked");
    // ADD with NO saved set: first fill + first draw
    bars(H); x.fill(0);
    Drawn m = redraw(x, H);
    const int yel = at2(m, 0xFACC15, 2);
    CHECK(yel > 0 && (int)H.lines.size() == yel, "ADD fresh: only yellow 2 px pivot segments drawn (" << yel << " of " << H.lines.size() << ")");
    CHECK(agree(x, H), "fresh: window == chart");
    CHECK(drawnStyle(x, H).find("PIVOT 1 FACC15 FACC15 2px") != std::string::npos && drawnStyle(x, H).find("VWAP 0 ") == 0, "status STYLE: PIVOT 1 yellow, VWAP 0 (" << drawnStyle(x, H) << ")");
    // ADD onto a chart that holds an OLDER saved set (versions 7..11): parmsInit saves the defaults once -> yellow pivot drawn
    for (unsigned ver = 7; ver <= 11; ++ver) {
        if (ver == 9) allOn(x, H); else if (ver == 11) { H.val = mock::declared; H.val[(size_t)x.P[svs::R_PIVOT]].b = false; } else hisRecord(x, H);
        CHECK(x.parmsInit(ver) == RTX_OK && isDeclared(H), "parmsInit(" << ver << "): the saved set = the 1.8.7 defaults, row by row");
        m = redraw(x, H);
        CHECK(at2(m, 0xFACC15, 2) == yel && (int)H.lines.size() == yel && why(x, H) == "ok", "parmsInit(" << ver << "): yellow pivot drawn (" << H.lines.size() << ")");
    }
    { allOn(x, H); CHECK(x.parmsInit(12) == RTX_OK && H.val[(size_t)x.P[svs::R_VWAP]].b, "parmsInit(12): his own set is left alone"); }
    // a brand-new DLL object (second ADD) on a second chart, before any bar: draw then calc never faults, nothing drawn without bars
    { SessionVWAP y; mock::Host E; E.root = "ES"; mock::current = &E; CHECK(y.setup() == RTX_OK, "second ADD: setup");
      E.lines.clear(); y.draw(); CHECK(E.lines.empty(), "second ADD: draw before any bar draws nothing");
      bars(E); y.fill(0); Drawn m2 = redraw(y, E); CHECK(at2(m2, 0xFACC15, 2) == yel, "second chart: same yellow pivot");
      mock::current = &E; y.done(); }
    mock::current = &H; x.done();
    // ZigZag anchors (PivotVwapLogic 1.1.0): no look-ahead - each prefix gives the same values for the bars it holds
    { std::vector<double> h, l, c, v; for (int i = 0; i < 120; ++i) { const double t = 5800 + 10 * std::sin(i / 3.0) + 0.1 * i; h.push_back(t + 1.5); l.push_back(t - 1.25); c.push_back(t + 0.25); v.push_back(100 + (i % 37) * 20); }
      const pvw::Out full = pvw::compute(h, l, c, v); bool same = true; int changes = 0;
      for (int n = 2; n <= 120; n += 7) { std::vector<double> h2(h.begin(), h.begin() + n), l2(l.begin(), l.begin() + n), c2(c.begin(), c.begin() + n), v2(v.begin(), v.begin() + n);
        const pvw::Out p = pvw::compute(h2, l2, c2, v2); for (int k = 0; k < n; ++k) same = same && (p.anchor[k] == full.anchor[k]) && ((std::isnan(p.line[k]) && std::isnan(full.line[k])) || p.line[k] == full.line[k]); }
      for (int k = 1; k < 120; ++k) changes += full.anchor[k] != full.anchor[k - 1];
      CHECK(same && changes >= 3, "pivot VWAP: no repaint over prefixes, " << changes << " anchor changes"); }
}

static void testVersion()
{
    std::ifstream f("SessionVWAP.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
    const size_t a = src.find("setVersion(\""), b = src.find("SV_VERSION = \"");
    const std::string v1 = src.substr(a + 12, src.find('"', a + 12) - a - 12), v2 = src.substr(b + 14, src.find('"', b + 14) - b - 14);
    CHECK(v1 == v2 && v1 == "1.8.7" && std::string(svset::PLUGIN_VERSION) == v1 && std::string(pvw::VERSION) == "1.1.0", "versions agree: 1.8.7, PivotVwapLogic 1.1.0");
}

int main()
{
    char tmpl[] = "/tmp/sv187XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 1;
    const std::string profile = std::string(dir) + "/u"; setenv("USERPROFILE", profile.c_str(), 1);
    testAll(dir); testVersion();
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::cout << "SessionVWAP 1.8.7 ADD path + yellow pivot VWAP: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
