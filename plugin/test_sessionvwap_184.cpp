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
    const svs::Style D;
    CHECK(svset::spec().paramVersion == 11, "(1.8.4) parameter version 11 (IRT hands every 1.8.3 / 1.8.2 / 1.8.1 / 1.8.0 set to parmsInit)");
    CHECK(!D.show[0] && !D.show[1] && !D.show[2] && !D.show[3] && !D.show[4] && !D.on && !D.prior && !D.pivot && D.pivW == 2
          && D.pivUp == 0x22C55E && D.pivDn == 0xEF4444 && D.col[svs::K_VWAP] == 0xFF80FF && D.vwapW == 2,
          "(1.8.4) defaults: NO line - pivot VWAP, session VWAP, 4 bands, overnight, prior all off (colours / widths kept)");

    mock::declared.clear();
    SessionVWAP x;
    mock::Host H; H.root = "ES"; mock::current = &H;
    CHECK(x.setup() == RTX_OK && mock::declared.size() == (size_t)svs::R_COUNT, "setup: 19 rows");
    { int on = 0; for (size_t i = 0; i < mock::declared.size(); ++i) if (mock::declared[i].type == 1 && mock::declared[i].b) ++on;
      CHECK(on == 0 && !mock::declared[(size_t)x.P[svs::R_PIVOT]].b, "(1.8.4) declared rows: every box unticked"); }
    bars(H); x.fill(0);
    int want[11]; for (int k = 0; k < 11; ++k) want[k] = segments(H, k);

    // ---- 1. (1.8.4) fresh: NOTHING is drawn, the window shows every box unticked
    Drawn m = redraw(x, H);
    CHECK(H.lines.empty() && want[0] > 0, "(1.8.4) fresh: no line drawn at all (" << H.lines.size() << ")");
    CHECK(svp::describe(shown(x, H)) == svp::describe(D), "fresh: the window shows the defaults (every box unticked)");
    CHECK(agree(x, H), "fresh: window == chart");
    CHECK(drawnStyle(x, H).find("VWAP 0 ") == 0 && drawnStyle(x, H).find("PIVOT 0 ") != std::string::npos && drawnStyle(x, H).find("ON 0|PRIOR 0") != std::string::npos, "(1.8.4) status STYLE: VWAP 0 ... ON 0 PRIOR 0 ... PIVOT 0");
    // the pivot VWAP when he ticks it (its segment count, used below)
    box(x, H, svs::R_PIVOT, true); m = redraw(x, H);
    const int pvUp = at2(m, 0x22C55E, 2), pvDn = at2(m, 0xEF4444, 2), pv = pvUp + pvDn;
    CHECK(pv > 0 && (int)H.lines.size() == pv && agree(x, H), "(1.8.4) pivot ticked: only the pivot VWAP (green / red 2 px) - " << pv << " segments");
    box(x, H, svs::R_PIVOT, false); redraw(x, H);
    CHECK(H.lines.empty() && agree(x, H), "(1.8.4) pivot unticked: nothing again");

    // ---- 2. older saved sets -> parmsInit -> the new defaults, once
    for (unsigned ver = 7; ver <= 10; ++ver) {
        if (ver == 9) allOn(x, H); else if (ver == 10) { H.val = mock::declared; box(x, H, svs::R_PIVOT, true); } else hisRecord(x, H);   // (1.8.4) v10 = the 1.8.3 pivot-only set
        CHECK(x.parmsInit(ver) == RTX_OK && isDeclared(H), "parmsInit(" << ver << "): the saved set = the new defaults, row by row");
        m = redraw(x, H);
        CHECK(H.lines.empty() && why(x, H) == "ok" && agree(x, H), "(1.8.4) parmsInit(" << ver << "): reset once - nothing drawn, window == chart");
    }
    { allOn(x, H); CHECK(x.parmsInit(11) == RTX_OK && H.val[(size_t)x.P[svs::R_VWAP]].b, "parmsInit(11): a current set (his own choices) is left alone");
      m = redraw(x, H); CHECK(at2(m, 0xFF80FF, 2) == want[0] + want[5] && at2(m, 0x008000, 1) == want[1] + want[6], "... and draws his choices (VWAP + bands)"); }
    { hisRecord(x, H); m = redraw(x, H); CHECK(H.lines.empty() && why(x, H).find("legacy") == 0, "legacy backstop: the 1.8.1 record kept by IRT draws the new defaults (nothing)"); }
    H.val = mock::declared;

    // ---- 3. every box adds / removes only its own line
    struct BoxCase { int row; int lines; const char* name; };
    const BoxCase B[] = { { svs::R_VWAP, want[0], "VWAP" }, { svs::R_P1, want[1], "+1 SD" }, { svs::R_M1, want[2], "-1 SD" },
                          { svs::R_P2, want[3], "+2 SD" }, { svs::R_M2, want[4], "-2 SD" }, { svs::R_PRIOR, want[10], "prior RTH VWAP" } };
    for (const BoxCase& c : B) {
        box(x, H, c.row, true); redraw(x, H);
        CHECK((int)H.lines.size() == c.lines && c.lines > 0, c.name << " ticked: exactly its " << c.lines << " RTH segments added");
        CHECK(agree(x, H), c.name << " ticked: window == chart");
        box(x, H, c.row, false); redraw(x, H);
        CHECK(H.lines.empty() && agree(x, H), c.name << " unticked: nothing drawn again");
    }
    box(x, H, svs::R_ON, true); redraw(x, H);
    CHECK(H.lines.empty() && agree(x, H), "overnight ticked with every line off: nothing added (it switches the O/N copies of the ticked lines)");
    box(x, H, svs::R_VWAP, true); redraw(x, H);
    CHECK((int)H.lines.size() == want[0] + want[5] && agree(x, H), "overnight + VWAP ticked: RTH + O/N VWAP added, no bands");
    box(x, H, svs::R_ON, false); redraw(x, H);
    CHECK((int)H.lines.size() == want[0] && agree(x, H), "overnight unticked: only the O/N VWAP goes");
    box(x, H, svs::R_VWAP, false); redraw(x, H);
    CHECK(H.lines.empty() && agree(x, H), "VWAP unticked: nothing drawn");
    box(x, H, svs::R_PIVOT, true); redraw(x, H);
    CHECK((int)H.lines.size() == pv && agree(x, H), "pivot ticked: back");

    // ---- 4. restart round trip (a new DLL object) keeps his change
    box(x, H, svs::R_P2, true); redraw(x, H); const std::string before = drawnStyle(x, H); const size_t nBefore = H.lines.size();
    mock::current = &H; x.done();
    {
        SessionVWAP y; mock::declared.clear(); mock::current = &H; y.setup(); y.fill(0);
        redraw(y, H);
        CHECK(drawnStyle(y, H) == before && H.lines.size() == nBefore && (int)nBefore == pv + want[3], "restart: same settings drawn (pivot + his +2 SD)");
        CHECK(agree(y, H), "restart: window == chart");
        mock::current = &H; y.fill(0);
        const std::string st = readStatus(dir);
        CHECK(st.find("VERSION,1.8.5\n") != std::string::npos && st.find("PARMS,native,ok\n") != std::string::npos, "status: VERSION 1.8.4, PARMS native ok");
        mock::current = &H; y.done();
    }

    // ---- 5. the pivot VWAP is ONE connected line per RTH session
    {
        SessionVWAP z; mock::declared.clear(); mock::Host Z; Z.root = "ES"; mock::current = &Z; z.setup(); zigzag(Z); z.fill(0);
        redraw(z, Z); CHECK(Z.lines.empty(), "(1.8.4) zig-zag session, defaults: nothing drawn");
        Z.val = mock::declared; box(z, Z, svs::R_PIVOT, true); redraw(z, Z);   // (1.8.4) he ticks the pivot VWAP: ONE connected line
        const int n = (int)Z.f[(int)barClose].size();
        std::vector<double> h, l, c, v;
        for (int i = 0; i < n; ++i) { h.push_back(Z.f[(int)barHigh][(size_t)i]); l.push_back(Z.f[(int)barLow][(size_t)i]); c.push_back(Z.f[(int)barClose][(size_t)i]); v.push_back((double)Z.i[(int)barVolume][(size_t)i]); }
        const pvw::Out o = pvw::compute(h, l, c, v);
        int first = -1, changes = 0; for (int i = 0; i < n; ++i) { if (first < 0 && std::isfinite(o.line[(size_t)i])) first = i; if (i > 0 && first >= 0 && i > first && o.anchor[(size_t)i] != o.anchor[(size_t)i - 1]) ++changes; }
        bool contig = first >= 0; for (int i = first; contig && i < n; ++i) contig = std::isfinite(o.line[(size_t)i]);
        CHECK(first >= 0 && contig && changes >= 3, "zig-zag session: a value from bar " << first << " on, " << changes << " anchor changes");
        CHECK((int)Z.lines.size() == n - 1 - first, "segments = bars with a value - 1 (" << (n - 1 - first) << "), got " << Z.lines.size());
        std::vector<int> seen((size_t)n, 0); bool unit = true, onLine = true;
        for (const mock::Line& L : Z.lines) { unit = unit && L.b1 == L.b0 + 1 && L.b0 >= first; if (L.b1 >= 0 && L.b1 < n) seen[(size_t)L.b1]++;
            onLine = onLine && std::fabs(L.p0 - (float)o.line[(size_t)L.b0]) < 1e-3f && std::fabs(L.p1 - (float)o.line[(size_t)L.b1]) < 1e-3f; }
        CHECK(unit, "every segment joins bar k-1 to bar k");
        CHECK(onLine, "segment ends = the pivot VWAP values (unchanged, no repaint)");
        int across = 0, colourOk = 1;
        for (int k = first + 1; k < n; ++k) {
            CHECK(seen[(size_t)k] == 1, "bar " << k << " is joined to bar " << (k - 1) << " exactly once");
            if (o.anchor[(size_t)k] != o.anchor[(size_t)k - 1]) { ++across;
                for (const mock::Line& L : Z.lines) if (L.b1 == k) { const int s = o.state[(size_t)k] != 0 ? o.state[(size_t)k] : o.state[(size_t)k - 1];
                    colourOk &= (s < 0 ? L.c == (COLOR)0xEF4444 : L.c == (COLOR)0x22C55E) ? 1 : 0; } }
        }
        CHECK(across == changes && across >= 3, "a segment crosses each of the " << changes << " anchor changes (1.8.2 left a gap there)");
        CHECK(colourOk == 1, "the crossing segment has the new bar's colour");
        mock::current = &Z; z.done();
    }
}

static void testVersion()
{
    std::ifstream f("SessionVWAP.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
    const size_t a = src.find("setVersion(\""), b = src.find("SV_VERSION = \"");
    const std::string v1 = src.substr(a + 12, src.find('"', a + 12) - a - 12), v2 = src.substr(b + 14, src.find('"', b + 14) - b - 14);
    CHECK(v1 == v2 && v1 == "1.8.5" && std::string(svset::PLUGIN_VERSION) == v1, "versions agree: 1.8.4 (setVersion literal = SV_VERSION = PLUGIN_VERSION)");
}

int main()
{
    char tmpl[] = "/tmp/sv184XXXXXX"; char* dir = mkdtemp(tmpl); if (!dir) return 1;
    const std::string profile = std::string(dir) + "/u"; setenv("USERPROFILE", profile.c_str(), 1);
    testAll(dir); testVersion();
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::cout << "SessionVWAP 1.8.4 no-line defaults + connected pivot VWAP when ticked: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
