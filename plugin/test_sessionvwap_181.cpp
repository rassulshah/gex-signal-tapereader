// test_sessionvwap_181.cpp -- lsSessionVWAP 1.8.1: the chart reads IRT's OWN per-indicator parameter set (getParameters, template
// svp::TEMPLATE), not the per-market file (Rassul 2026-10-27 22:47 CT "the settings never work for any indicator").
//   g++ -std=c++17 -Wall -Wextra -DSVS_TEST_ALL_ON_DEFAULTS -ISessionVWAP_test_support -I. test_sessionvwap_181.cpp
// The mock SDK (SessionVWAP_test_support/irtsdk.h) hands each chart's saved set to getParameters in ANY copy (chart or window), lays the
// struct out at natural alignment, and can refuse (parmsMode 1) or hand a blank set (parmsMode 2). One mock::Host = one indicator on
// one chart; the window edits that host's set, as IRT stores the window's OK with the indicator.
// Covers: every row toggles / recolours its own draw ops; the pivot VWAP is independent of the VWAP box; the Add Indicator window
// (no chart) works; a second ES chart keeps its own settings; a DLL reload (restart) keeps them; the file never overrides the
// indicator's own set; refused / blank sets fall back to the file; a version-7 set is carried over by parmsInit.
#include "irtsdk.h"
#include <cmath>
#include <cstdlib>
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
        if (T[i] >= at(2026, 10, 13, 8, 33)) {   // the last RTH session: a rally, then a clear 3-bar reversal down (pivot VWAP goes red)
            const int j = (int)((T[i] - at(2026, 10, 13, 8, 33)) / 180);
            tp = 5810.0f + (j < 12 ? 1.5f * (float)j : 18.0f - 2.5f * (float)(j - 12));
        }
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
static Drawn drawn(mock::Host& H) { Drawn m; for (size_t i = 0; i < H.lines.size(); ++i) m[std::make_pair((unsigned long)H.lines[i].c, (int)H.lines[i].w)]++; return m; }
static Drawn redraw(SessionVWAP& x, mock::Host& H) { mock::current = &H; H.dialog = false; H.lines.clear(); x.draw(); return drawn(H); }
static int pivotSegs(const Drawn& m, unsigned long up, unsigned long dn, int w) { Drawn::const_iterator a = m.find(std::make_pair(up, w)), b = m.find(std::make_pair(dn, w));
    return (a == m.end() ? 0 : a->second) + (b == m.end() ? 0 : b->second); }
// the window on this indicator: set a box / colour / list, OK
static void box(SessionVWAP& x, mock::Host& H, int row, bool on) { mock::current = &H; H.dialog = true; x.checkBox(x.P[row], on); x.parmsUpdt((unsigned)x.P[row]); x.parmsApply(); H.dialog = false; }
static void colour(SessionVWAP& x, mock::Host& H, int row, COLOR c) { mock::current = &H; H.dialog = true; x.setParameterColor(x.P[row], c); x.parmsUpdt((unsigned)x.P[row]); x.parmsApply(); H.dialog = false; }
static void list(SessionVWAP& x, mock::Host& H, int row, int i) { mock::current = &H; H.dialog = true; x.setListIndex(x.P[row], i); x.parmsUpdt((unsigned)x.P[row]); x.parmsApply(); H.dialog = false; }
static std::string src(SessionVWAP& x, mock::Host& H) { mock::current = &H; SVState* S = x.state(false); return S ? S->parmsSrc : std::string("no state"); }

static void testTemplate()
{
    CHECK(std::strlen(svp::TEMPLATE) == (size_t)svs::R_COUNT && (int)svset::K_N == svs::R_COUNT, "one template token per settings row (19)");
    CHECK(std::strlen(svp::TEMPLATE7) == 16, "the version-7 template = the 16 rows of 1.8.0");
    CHECK(std::string(svp::TEMPLATE).substr(0, 16) == svp::TEMPLATE7, "rows were only appended (IRT keeps values by position)");
    const ss::Spec& S = svset::spec();
    for (int k = 0; k < S.n; ++k) {
        const char t = svp::TEMPLATE[k];
        const char want = S.rows[k].type == ss::T_BOOL ? 'B' : S.rows[k].type == ss::T_COLOR ? 'U' : S.rows[k].type == ss::T_LIST ? 'S' : '?';
        CHECK(t == want, "template token " << k << " matches the row type (" << S.rows[k].label << ")");
    }
    CHECK(S.paramVersion == 10, "parameter version 9 (1.8.2: older sets reset to the defaults; rows unchanged since 1.8.1)");
    int commas = 0; const std::string d = svp::defaults(); for (char c : d) commas += c == ',';
    CHECK(commas == svs::R_COUNT - 1, "the defaults string has one value per row");
    svp::Parms z; std::memset(&z, 0, sizeof z); std::string why;
    CHECK(!svp::valid(z, &why) && why == "blank set", "an all-zero set is refused as blank");
    svp::Parms p; std::memset(&p, 0, sizeof p); p.vwap = 2; p.p1 = 1;
    CHECK(!svp::valid(p, &why) && why == "box not 0/1", "a box that reads 2 is refused");
}

static void testPlugin(const std::string& profile)
{
    mock::declared.clear();
    SessionVWAP x;
    mock::Host H; H.root = "ES"; mock::current = &H;
    CHECK(x.setup() == RTX_OK, "setup");
    CHECK(mock::declared.size() == (size_t)svs::R_COUNT, "19 rows declared");
    CHECK(mock::declared[(size_t)x.P[svs::R_PIV_UP]].c == 0x22C55E && mock::declared[(size_t)x.P[svs::R_PIV_DN]].c == 0xEF4444 && mock::declared[(size_t)x.P[svs::R_PIV_W]].list == 1,
          "pivot rows: above green 22C55E, below red EF4444, width 2 px");
    const std::string esFile = svs::pathFor(profile.c_str(), "ES"); std::remove(esFile.c_str());
    bars(H); x.fill(0);
    int want[11]; for (int k = 0; k < 11; ++k) want[k] = segments(H, k);
    Drawn m = redraw(x, H);
    CHECK(src(x, H) == "native", "the chart draws from IRT's own per-indicator set");
    const int pv = pivotSegs(m, 0x22C55E, 0xEF4444, 2), pvUp = m[std::make_pair(0x22C55EUL, 2)], pvDn = m[std::make_pair(0xEF4444UL, 2)];
    CHECK(pv > 0 && pvUp > 0 && pvDn > 0, "defaults: the pivot VWAP is drawn green AND red, 2 px (up " << pvUp << ", down " << pvDn << ")");
    CHECK(m[std::make_pair(0xFF80FFUL, 2)] == want[0] + want[5], "defaults: the session VWAP (magenta 2 px) is drawn RTH + overnight - not replaced by the pivot");
    int total = pv; for (int k = 0; k < 11; ++k) total += want[k];
    CHECK((int)H.lines.size() == total, "defaults: every line, nothing else");

    // ---- every row toggles its own line, only that line
    box(x, H, svs::R_VWAP, false); m = redraw(x, H);
    CHECK(m[std::make_pair(0xFF80FFUL, 2)] == 0 && pivotSegs(m, 0x22C55E, 0xEF4444, 2) == pv, "VWAP off: magenta gone, the pivot VWAP STAYS (1.8.0 hid it)");
    CHECK((int)H.lines.size() == total - want[0] - want[5], "VWAP off removes exactly its RTH + O/N segments");
    box(x, H, svs::R_VWAP, true); box(x, H, svs::R_PIVOT, false); m = redraw(x, H);
    CHECK(pivotSegs(m, 0x22C55E, 0xEF4444, 2) == 0 && m[std::make_pair(0xFF80FFUL, 2)] == want[0] + want[5], "pivot off: pivot gone, the session VWAP stays");
    box(x, H, svs::R_PIVOT, true);
    const int showRow[5] = { svs::R_VWAP, svs::R_P1, svs::R_M1, svs::R_P2, svs::R_M2 };
    for (int k = 1; k < 5; ++k) {
        box(x, H, showRow[k], false); m = redraw(x, H);
        CHECK((int)H.lines.size() == total - want[k] - want[k + 5], "band " << k << " off removes only its RTH + O/N segments");
        box(x, H, showRow[k], true);
    }
    box(x, H, svs::R_PRIOR, false); m = redraw(x, H);
    CHECK((int)H.lines.size() == total - want[10] && m[std::make_pair(0x6B7280UL, 1)] == 0, "prior RTH VWAP off");
    box(x, H, svs::R_PRIOR, true); box(x, H, svs::R_ON, false); m = redraw(x, H);
    CHECK((int)H.lines.size() == total - want[5] - want[6] - want[7] - want[8] - want[9], "overnight set off: the five O/N lines go");
    box(x, H, svs::R_ON, true);

    // ---- every colour / width row recolours / resizes its own line
    colour(x, H, svs::R_VWAP_COL, 0x00FFFF); m = redraw(x, H);
    CHECK(m[std::make_pair(0x00FFFFUL, 2)] == want[0] + want[5] && m[std::make_pair(0xFF80FFUL, 2)] == 0, "VWAP colour applied");
    const int colRow[4] = { svs::R_P1_COL, svs::R_M1_COL, svs::R_P2_COL, svs::R_M2_COL };
    const COLOR newC[4] = { 0x3366FF, 0xFFAA00, 0x99FF99, 0xAA00AA };
    for (int k = 0; k < 4; ++k) { colour(x, H, colRow[k], newC[k]); m = redraw(x, H);
        CHECK(m[std::make_pair((unsigned long)newC[k], 1)] == want[k + 1] + want[k + 6], "band " << (k + 1) << " colour applied"); }
    colour(x, H, svs::R_PRIOR_COL, 0xAAAAAA); m = redraw(x, H);
    CHECK(m[std::make_pair(0xAAAAAAUL, 1)] == want[10], "prior RTH colour applied");
    colour(x, H, svs::R_PIV_UP, 0x0000FF); colour(x, H, svs::R_PIV_DN, 0xFFFF00); m = redraw(x, H);
    CHECK(m[std::make_pair(0x0000FFUL, 2)] == pvUp && m[std::make_pair(0xFFFF00UL, 2)] == pvDn && pivotSegs(m, 0x22C55E, 0xEF4444, 2) == 0,
          "pivot above / below colours applied (each segment in its side's new colour)");
    list(x, H, svs::R_PIV_W, 3); m = redraw(x, H);
    CHECK(pivotSegs(m, 0x0000FF, 0xFFFF00, 4) == pv && m[std::make_pair(0x00FFFFUL, 2)] == want[0] + want[5], "pivot width 4 px; the VWAP keeps its own width");
    list(x, H, svs::R_VWAP_W, 2); list(x, H, svs::R_BAND_W, 1); m = redraw(x, H);
    CHECK(m[std::make_pair(0x00FFFFUL, 3)] == want[0] + want[5] && m[std::make_pair(0x3366FFUL, 2)] == want[1] + want[6] && pivotSegs(m, 0x0000FF, 0xFFFF00, 4) == pv,
          "VWAP width 3 px, bands 2 px, pivot unchanged");
    colour(x, H, svs::R_P2_COL, 0x000000); m = redraw(x, H);
    CHECK(m[std::make_pair(0x00FF00UL, 2)] == want[3] + want[8], "a black swatch draws the default colour (never an invisible line on his black chart)");

    // ---- the per-market file never overrides this indicator's own set
    { svs::Style s; s.show[svs::K_VWAP] = false; s.pivot = false; std::ofstream f(esFile.c_str(), std::ios::trunc); f << svs::toText(s); }
    m = redraw(x, H);
    CHECK(m[std::make_pair(0x00FFFFUL, 3)] == want[0] + want[5] && pivotSegs(m, 0x0000FF, 0xFFFF00, 4) == pv, "a per-market file does not override the indicator's own set");
    std::remove(esFile.c_str());

    // ---- a second ES chart keeps ITS settings (defaults here) while the first keeps its own
    mock::Host H2; H2.root = "ES"; mock::current = &H2; bars(H2); x.fill(0);
    m = redraw(x, H2);
    CHECK(m[std::make_pair(0xFF80FFUL, 2)] == want[0] + want[5] && pivotSegs(m, 0x22C55E, 0xEF4444, 2) == pv, "a second ES chart draws its own (default) settings");
    box(x, H2, svs::R_P1, false); m = redraw(x, H2);
    CHECK(m[std::make_pair(0x008000UL, 1)] == 0, "the second chart's own change applies to it");
    m = redraw(x, H);
    CHECK(m[std::make_pair(0x3366FFUL, 2)] == want[1] + want[6], "... and the first ES chart is untouched");

    // ---- the Add Indicator window (no chart yet): the change lands in the new indicator's set and the chart draws it
    mock::Host H5; H5.root = ""; H5.dialog = true; mock::current = &H5;
    x.parmsLoad();
    CHECK(x.isBoxChecked(x.P[svs::R_VWAP]) && x.getParameterColor(x.P[svs::R_PIV_UP]) == 0x22C55E, "the Add window shows the new indicator's own (default) values");
    x.setParameterColor(x.P[svs::R_PIV_UP], 0x00CCFF); x.parmsUpdt((unsigned)x.P[svs::R_PIV_UP]);
    x.checkBox(x.P[svs::R_VWAP], false); x.parmsUpdt((unsigned)x.P[svs::R_VWAP]); x.parmsApply();
    H5.root = "ES"; H5.dialog = false; bars(H5); x.fill(0);                // IRT adds the indicator to the ES chart
    m = redraw(x, H5);
    CHECK(m[std::make_pair(0x00CCFFUL, 2)] > 0 && m[std::make_pair(0xFF80FFUL, 2)] == 0 && src(x, H5) == "native",
          "Add-window settings show on the chart (pivot above colour, VWAP off) - 1.8.0 threw them away");

    // ---- restart: a fresh DLL object reads the same saved sets
    mock::current = &H; x.done(); mock::current = &H2; x.done(); mock::current = &H5; x.done();
    {
        SessionVWAP y; mock::declared.clear(); mock::current = &H; y.setup();
        y.fill(0); m = redraw(y, H);
        CHECK(m[std::make_pair(0x00FFFFUL, 3)] == want[0] + want[5] && pivotSegs(m, 0x0000FF, 0xFFFF00, 4) == pv, "after a restart the first chart keeps its settings");
        mock::current = &H5; y.fill(0); m = redraw(y, H5);
        CHECK(m[std::make_pair(0x00CCFFUL, 2)] > 0 && m[std::make_pair(0xFF80FFUL, 2)] == 0, "after a restart the added indicator keeps its settings");
        mock::current = &H; y.done(); mock::current = &H5; y.done();
    }

    // ---- IRT refuses / hands a blank set: the per-market file (fallback), then the defaults
    {
        SessionVWAP z; mock::declared.clear(); mock::Host H7; H7.root = "ES"; H7.parmsMode = 1; mock::current = &H7; z.setup(); bars(H7); z.fill(0);
        { svs::Style s; s.col[svs::K_P1] = 0x123456; std::ofstream f(esFile.c_str(), std::ios::trunc); f << svs::toText(s); }
        m = redraw(z, H7);
        CHECK(m[std::make_pair(0x123456UL, 1)] == want[1] + want[6] && src(z, H7) == "file", "refused set: the per-market file draws (fallback)");
        H7.parmsMode = 2; sleep(1); { svs::Style s; s.col[svs::K_P1] = 0x654321; std::ofstream f(esFile.c_str(), std::ios::trunc); f << svs::toText(s); }
        m = redraw(z, H7);
        CHECK(m[std::make_pair(0x654321UL, 1)] == want[1] + want[6] && src(z, H7) == "file", "blank set: the file draws, never a blank chart");
        SVState* S = z.state(false); CHECK(S && S->parmsWhy.find("blank set") == 0, "the reason is kept for the status file (PARMS,file,blank set ...)");
        std::remove(esFile.c_str()); sleep(1);   // past the 1 s file check
        m = redraw(z, H7);
        CHECK(m[std::make_pair(0x008000UL, 1)] == want[1] + want[6], "no set and no file: the defaults");
        z.done();
    }

    // ---- version upgrade (1.8.2 replaces the 1.8.1 carry-over): ANY older set (7 = 1.8.0, 8 = 1.8.1) becomes the defaults;
    // the full migration cases are in test_sessionvwap_182.cpp
    {
        SessionVWAP v; mock::declared.clear(); mock::Host H8; H8.root = "ES"; mock::current = &H8; v.setup(); bars(H8); v.fill(0);
        H8.val = mock::declared; for (size_t i = 0; i < H8.val.size(); ++i) { H8.val[i].b = false; H8.val[i].c = 0; H8.val[i].list = 0; }
        CHECK(v.parmsInit(7) == RTX_OK, "parmsInit(7) saves the defaults");
        m = redraw(v, H8);
        CHECK(m[std::make_pair(0xFF80FFUL, 2)] == want[0] + want[5] && m[std::make_pair(0x008000UL, 1)] == want[1] + want[6] && pivotSegs(m, 0x22C55E, 0xEF4444, 2) == pv,
              "after parmsInit(7): VWAP magenta 2 px, +1 SD green, pivot green / red 2 px");
        CHECK(v.parmsInit(9) == RTX_OK && v.parmsInit(5) == RTX_OK, "version 9 = nothing to do; any older = the defaults");
        v.done();
    }
}

static void testVersion()
{
    std::ifstream f("SessionVWAP.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
    const size_t a = src.find("setVersion(\""), b = src.find("SV_VERSION = \"");
    const std::string v1 = src.substr(a + 12, src.find('"', a + 12) - a - 12), v2 = src.substr(b + 14, src.find('"', b + 14) - b - 14);
    CHECK(v1 == v2 && v1 == "1.8.3" && std::string(svset::PLUGIN_VERSION) == v1, "versions agree: 1.8.2");
}

int main()
{
    char tmpl[] = "/tmp/sv181XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 1;
    const std::string profile = std::string(dir) + "/u"; setenv("USERPROFILE", profile.c_str(), 1);
    testTemplate(); testPlugin(profile); testVersion();
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::cout << "SessionVWAP 1.8.1 native settings: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
