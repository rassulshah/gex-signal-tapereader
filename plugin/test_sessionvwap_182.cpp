// test_sessionvwap_182.cpp -- lsSessionVWAP 1.8.2 (2026-10-28, Rassul 11:27 CT "did you fix the session vwap. as you can see it isnt
// working properly. can you fix this too and test it").
//   g++ -std=c++17 -Wall -Wextra -Werror -DSVS_TEST_ALL_ON_DEFAULTS -fsanitize=address,undefined -ISessionVWAP_test_support -I. test_sessionvwap_182.cpp && ./a.out
// What he saw (screenshot 11:27, Settings-dialog.log 11:25:52-11:26:00): the window of his ES 3m chart opened on VWAP unticked, VWAP
// colour BLACK, 1 px - the set 1.8.1's parmsInit had carried over from the 1.8.0 record - and the chart drew no session VWAP.
// Covers: defaults load = the intended 1.8.0 look; his exact record (a) read as the defaults when IRT keeps it, (b) replaced by the
// defaults through parmsInit(8) / parmsInit(7); the window ALWAYS shows exactly what the chart draws (incl. a black swatch, greyed rows);
// every box toggles its line, every colour / width changes its draw ops; save -> new DLL object (restart) round trip; status STYLE line.
// The mock (SessionVWAP_test_support/irtsdk.h): one mock::Host = one indicator on one chart; the window's controls ARE its saved set
// (IRT stores the window on OK), getParameters hands that set to any copy at natural struct alignment.
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
static void colour(SessionVWAP& x, mock::Host& H, int row, COLOR c) { openWin(x, H); x.setParameterColor(x.P[row], c); x.parmsUpdt((unsigned)x.P[row]); okWin(x, H); }
static void list(SessionVWAP& x, mock::Host& H, int row, int i) { openWin(x, H); x.setListIndex(x.P[row], i); x.parmsUpdt((unsigned)x.P[row]); okWin(x, H); }
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

static void testAll(const std::string& dir)
{
    const svs::Style D;
    CHECK(svset::spec().paramVersion == 11, "parameter version 9 (IRT hands every 1.8.1 / 1.8.0 set to parmsInit)");
    CHECK(D.show[svs::K_VWAP] && D.col[svs::K_VWAP] == 0xFF80FF && D.vwapW == 2 && D.bandW == 1 && D.on && D.prior && D.pivot && D.pivW == 2,
          "intended defaults: VWAP on magenta 2 px, bands 1 px, overnight on, prior on, pivot on 2 px");

    mock::declared.clear();
    SessionVWAP x;
    mock::Host H; H.root = "ES"; mock::current = &H;
    CHECK(x.setup() == RTX_OK && mock::declared.size() == (size_t)svs::R_COUNT, "setup: 19 rows");
    bars(H); x.fill(0);
    int want[11]; for (int k = 0; k < 11; ++k) want[k] = segments(H, k);
    int total = 0; for (int k = 0; k < 11; ++k) total += want[k];

    // ---- 1. a fresh indicator: the defaults draw and the window shows them
    Drawn m = redraw(x, H);
    const int pvUp = at2(m, 0x22C55E, 2), pvDn = at2(m, 0xEF4444, 2), pv = pvUp + pvDn;
    CHECK(at2(m, 0xFF80FF, 2) == want[0] + want[5] && want[0] > 0, "fresh: session VWAP magenta 2 px (RTH + overnight)");
    CHECK(at2(m, 0x008000, 1) == want[1] + want[6] && at2(m, 0xFF8080, 1) == want[2] + want[7] && at2(m, 0x00FF00, 1) == want[3] + want[8] && at2(m, 0xFF0000, 1) == want[4] + want[9],
          "fresh: the four bands in his colours, 1 px");
    CHECK(at2(m, 0x6B7280, 1) == want[10] && pvUp > 0 && pvDn > 0 && (int)H.lines.size() == total + pv, "fresh: prior RTH VWAP + pivot VWAP, nothing else");
    CHECK(svp::describe(shown(x, H)) == svp::describe(D), "fresh: the window shows the defaults (VWAP ticked, magenta, 2 px ...)");
    CHECK(agree(x, H), "fresh: window == chart");

    // ---- 2a. HIS record, IRT keeps it (parmsInit not called): drawn as the defaults, the window shows the defaults, OK fixes the set
    hisRecord(x, H);
    m = redraw(x, H);
    CHECK(at2(m, 0xFF80FF, 2) == want[0] + want[5] && at2(m, 0x008000, 1) == want[1] + want[6] && (int)H.lines.size() == total + pv,
          "his 11:26 record: the chart draws the defaults (VWAP + bands back)");
    CHECK(why(x, H).find("legacy") == 0, "... and the status reason says so (PARMS,native,legacy ...)");
    CHECK(svp::describe(shown(x, H)) == svp::describe(D), "his record: the window opens on the defaults, never VWAP unticked / black / 1 px");
    okWin(x, H);
    { bool same = true; for (size_t i = 0; i < H.val.size(); ++i) same = same && H.val[i].b == mock::declared[i].b && H.val[i].c == mock::declared[i].c && H.val[i].list == mock::declared[i].list;
      CHECK(same, "OK on that window stores the defaults in the indicator's own set"); }
    CHECK(why(x, H) == "ok" || (redraw(x, H), why(x, H) == "ok"), "after OK the set is a normal one (no legacy reading)");

    // ---- 2b. HIS record handed to parmsInit (version 8 = 1.8.1, version 7 = 1.8.0): replaced by the defaults
    for (unsigned ver = 7; ver <= 8; ++ver) {
        hisRecord(x, H);
        CHECK(x.parmsInit(ver) == RTX_OK, "parmsInit(" << ver << ") OK");
        bool same = true; for (size_t i = 0; i < H.val.size(); ++i) same = same && H.val[i].b == mock::declared[i].b && H.val[i].c == mock::declared[i].c && H.val[i].list == mock::declared[i].list;
        CHECK(same, "parmsInit(" << ver << "): the saved set = the declared defaults, row by row");
        m = redraw(x, H);
        CHECK((int)H.lines.size() == total + pv && at2(m, 0xFF80FF, 2) == want[0] + want[5] && why(x, H) == "ok", "parmsInit(" << ver << "): drawn = defaults");
    }
    { hisRecord(x, H); H.val[(size_t)x.P[svs::R_P1]].b = true; CHECK(x.parmsInit(11) == RTX_OK && H.val[(size_t)x.P[svs::R_VWAP]].b == false, "parmsInit(9): a current set is left alone"); }
    H.val = mock::declared;

    // ---- 3. every box: off -> its lines gone (only them), the window shows it unticked; on -> back
    struct BoxCase { int row; int lines; const char* name; };
    const BoxCase B[] = { { svs::R_VWAP, want[0] + want[5], "VWAP" }, { svs::R_P1, want[1] + want[6], "+1 SD" }, { svs::R_M1, want[2] + want[7], "-1 SD" },
                          { svs::R_P2, want[3] + want[8], "+2 SD" }, { svs::R_M2, want[4] + want[9], "-2 SD" },
                          { svs::R_ON, want[5] + want[6] + want[7] + want[8] + want[9], "overnight set" }, { svs::R_PRIOR, want[10], "prior RTH VWAP" },
                          { svs::R_PIVOT, pv, "3-bar pivot VWAP" } };
    for (const BoxCase& c : B) {
        box(x, H, c.row, false); m = redraw(x, H);
        CHECK((int)H.lines.size() == total + pv - c.lines && c.lines > 0, c.name << " off: exactly its " << c.lines << " segments gone");
        CHECK(!x.isBoxChecked(x.P[c.row]) || (shown(x, H), !x.isBoxChecked(x.P[c.row])), c.name << " off: the window shows it unticked");
        CHECK(agree(x, H), c.name << " off: window == chart");
        box(x, H, c.row, true); m = redraw(x, H);
        CHECK((int)H.lines.size() == total + pv, c.name << " back on: every line again");
    }

    // ---- 4. every colour / width
    struct ColCase { int row; COLOR c; int w; int lines; const char* name; };
    const ColCase C[] = { { svs::R_VWAP_COL, 0x00FFFF, 2, want[0] + want[5], "VWAP colour" }, { svs::R_P1_COL, 0x3366FF, 1, want[1] + want[6], "+1 SD colour" },
                          { svs::R_M1_COL, 0xFFAA00, 1, want[2] + want[7], "-1 SD colour" }, { svs::R_P2_COL, 0x99FF99, 1, want[3] + want[8], "+2 SD colour" },
                          { svs::R_M2_COL, 0xAA00AA, 1, want[4] + want[9], "-2 SD colour" }, { svs::R_PRIOR_COL, 0xAAAAAA, 1, want[10], "prior colour" },
                          { svs::R_PIV_UP, 0x0000FF, 2, pvUp, "pivot above colour" }, { svs::R_PIV_DN, 0xFFFF00, 2, pvDn, "pivot below colour" } };
    for (const ColCase& c : C) { colour(x, H, c.row, c.c); m = redraw(x, H);
        CHECK(at2(m, c.c, c.w) == c.lines, c.name << " applied to its " << c.lines << " segments"); CHECK(agree(x, H), c.name << ": window == chart"); }
    list(x, H, svs::R_VWAP_W, 3); m = redraw(x, H);
    CHECK(at2(m, 0x00FFFF, 4) == want[0] + want[5] && agree(x, H), "VWAP width 4 px");
    list(x, H, svs::R_BAND_W, 2); m = redraw(x, H);
    CHECK(at2(m, 0x3366FF, 3) == want[1] + want[6] && at2(m, 0xAA00AA, 3) == want[4] + want[9] && at2(m, 0x00FFFF, 4) == want[0] + want[5] && agree(x, H), "band width 3 px (VWAP keeps 4 px)");
    list(x, H, svs::R_PIV_W, 0); m = redraw(x, H);
    CHECK(at2(m, 0x0000FF, 1) == pvUp && at2(m, 0xFFFF00, 1) == pvDn && agree(x, H), "pivot width 1 px");

    // ---- 5. a black swatch draws the default colour AND the window shows that colour (1.8.1: window black, chart lime)
    colour(x, H, svs::R_P2_COL, 0x000000); m = redraw(x, H);
    CHECK(at2(m, 0x00FF00, 3) == want[3] + want[8], "black +2 SD swatch: drawn in the default lime (never invisible on his black chart)");
    CHECK(shown(x, H).col[svs::K_P2] == 0x00FF00 && agree(x, H), "... and the window shows lime, not black");

    // ---- 6. save -> restart (a new DLL object) round trip: same drawing, same window
    redraw(x, H); const std::string before = drawnStyle(x, H); const size_t nBefore = H.lines.size();
    mock::current = &H; x.done();
    {
        SessionVWAP y; mock::declared.clear(); mock::current = &H; y.setup(); y.fill(0);
        Drawn m2 = redraw(y, H);
        CHECK(drawnStyle(y, H) == before && H.lines.size() == nBefore && at2(m2, 0x00FFFF, 4) == want[0] + want[5], "restart: the chart draws the same settings");
        CHECK(agree(y, H), "restart: window == chart");
        // ---- 7. the status file says what was drawn
        mock::current = &H; y.fill(0);
        const std::string st = readStatus(dir);
        CHECK(st.find("VERSION,1.8.4\n") != std::string::npos && st.find("PARMS,native,ok\n") != std::string::npos, "status: VERSION 1.8.2, PARMS native ok");
        CHECK(st.find("STYLE," + before + "\n") != std::string::npos, "status: STYLE line = the drawn style (" << before << ")");
        mock::current = &H; y.done();
    }

    // ---- 8. two ES charts keep their own sets
    {
        SessionVWAP z; mock::declared.clear(); mock::Host A, Bh; A.root = Bh.root = "ES";
        mock::current = &A; z.setup(); bars(A); z.fill(0); mock::current = &Bh; bars(Bh); z.fill(0);
        box(z, A, svs::R_P1, false);
        Drawn ma = redraw(z, A), mb = redraw(z, Bh);
        CHECK(at2(ma, 0x008000, 1) == 0 && at2(mb, 0x008000, 1) == want[1] + want[6], "chart A's +1 SD off leaves chart B's on");
        CHECK(agree(z, A) && agree(z, Bh), "each window == its own chart");
        mock::current = &A; z.done(); mock::current = &Bh; z.done();
    }
}

static void testVersion()
{
    std::ifstream f("SessionVWAP.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
    const size_t a = src.find("setVersion(\""), b = src.find("SV_VERSION = \"");
    const std::string v1 = src.substr(a + 12, src.find('"', a + 12) - a - 12), v2 = src.substr(b + 14, src.find('"', b + 14) - b - 14);
    CHECK(v1 == v2 && v1 == "1.8.4" && std::string(svset::PLUGIN_VERSION) == v1, "versions agree: 1.8.2 (setVersion literal = SV_VERSION = PLUGIN_VERSION)");
}

int main()
{
    char tmpl[] = "/tmp/sv182XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 1;
    const std::string profile = std::string(dir) + "/u"; setenv("USERPROFILE", profile.c_str(), 1);
    testAll(dir); testVersion();
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::cout << "SessionVWAP 1.8.2 settings fix: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
