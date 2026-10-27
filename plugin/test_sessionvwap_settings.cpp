// test_sessionvwap_settings.cpp -- lsSessionVWAP 1.5.0: the settings window (show box + colour per line, widths, overnight set,
// prior RTH VWAP), the per-market file, and that the chart follows them (Rassul 2026-10-13 21:23 "session vwap settings don't
// work. They should allow me to uncheck and remove bands as well as change colors").
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -ISessionVWAP_test_support -I. test_sessionvwap_settings.cpp
// Part 1 = SessionVWAPStyle.h alone. Part 2 = the REAL SessionVWAP.cpp on the mock SDK (SessionVWAP_test_support/irtsdk.h): the
// mock reads list / box / colour values only while `dialog` is true, as IRT does.
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

// ------------------------------------------------------------------------------------------------ part 1: SessionVWAPStyle.h
static void testDefaults()
{
    svs::Style s;
    // 1.4.2 drew: VWAP magenta 2 px, +1 green, -1 light red, +2 lime, -2 red, bands 1 px, prior RTH grey 1 px (O/N the same)
    const unsigned long COL[11] = { 0xFF80FF, 0x008000, 0xFF8080, 0x00FF00, 0xFF0000, 0xFF80FF, 0x008000, 0xFF8080, 0x00FF00, 0xFF0000, 0x6B7280 };
    const int W[11] = { 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1 };
    for (int k = 0; k < 11; ++k) {
        const svs::Pen p = svs::penFor(s, k);
        CHECK(p.show, "default: output " << k << " shown");
        CHECK(p.col == COL[k], "default colour of output " << k << " = 1.4.2's");
        CHECK(p.width == W[k], "default width of output " << k << " = 1.4.2's");
    }
    CHECK(!svs::penFor(s, -1).show && !svs::penFor(s, 11).show, "out-of-range output is never drawn");
}

static void testPens()
{
    svs::Style s; s.show[svs::K_P1] = false; s.on = false; s.prior = false; s.col[svs::K_M2] = 0x123456; s.bandW = 3; s.vwapW = 4;
    CHECK(!svs::penFor(s, 1).show && !svs::penFor(s, 6).show, "+1 SD unticked: RTH and O/N +1 hidden");
    CHECK(svs::penFor(s, 0).show && svs::penFor(s, 2).show && svs::penFor(s, 3).show && svs::penFor(s, 4).show, "RTH lines still shown with O/N off");
    for (int k = 5; k <= 9; ++k) CHECK(!svs::penFor(s, k).show, "O/N output " << k << " hidden when the overnight set is off");
    CHECK(!svs::penFor(s, 10).show, "prior RTH VWAP hidden when unticked");
    CHECK(svs::penFor(s, 4).col == 0x123456 && svs::penFor(s, 9).col == 0x123456, "-2 SD colour applies to RTH and O/N");
    CHECK(svs::penFor(s, 0).width == 4 && svs::penFor(s, 5).width == 4, "VWAP width");
    CHECK(svs::penFor(s, 1).width == 3 && svs::penFor(s, 9).width == 3 && svs::penFor(s, 10).width == 1, "band width (prior VWAP stays 1 px)");
}

static void testText()
{
    svs::Style s; s.show[svs::K_M1] = false; s.col[svs::K_P2] = 0xABCDEF; s.vwapW = 3; s.bandW = 2; s.on = false; s.priorCol = 0x808080;
    const std::string t = svs::toText(s);
    svs::Style r; int bad = -1;
    CHECK(svs::fromText(t, r, &bad) && r == s && bad == 0, "round trip");
    std::string crlf; for (size_t i = 0; i < t.size(); ++i) { if (t[i] == '\n') crlf += '\r'; crlf += t[i]; }
    CHECK(svs::fromText(crlf, r) && r == s, "CRLF file");
    CHECK(svs::fromText("VERSION,1\nVWAP,1,abcdef,2\n", r) && r.col[0] == 0xABCDEF, "lower-case hex");
    // garbage -> defaults (whole file) or the item's default (one line)
    svs::Style d;
    CHECK(!svs::fromText("", r) && r == d, "empty file = defaults");
    CHECK(!svs::fromText("P1,0,008000\n", r) && r == d, "no VERSION line = defaults");
    CHECK(!svs::fromText("VERSION,2\nP1,0,008000\n", r) && r == d, "unknown version = defaults");
    CHECK(!svs::fromText(std::string(5000, 'x'), r) && r == d, "oversized = defaults");
    const char* badLines[] = { "VWAP,1,FF80FF,0", "VWAP,1,FF80FF,5", "VWAP,1,FF80FF,12", "VWAP,1,FF80FF", "VWAP,2,FF80FF,2", "VWAP,1,GG0000,2",
                               "VWAP,1,000000,2", "VWAP,1,0F0F0F,2", "P1,1,0080000", "P1,1,00800", "P1,,008000", "P1,1,008000,1",
                               "BANDW,0", "BANDW,9", "BANDW,-1", "BANDW,", "ON,yes", "ON,", "PRIOR,1", "PRIOR,1,101010", "PRIOR,3,808080" };
    for (size_t i = 0; i < sizeof(badLines) / sizeof(badLines[0]); ++i) {
        int nb = 0;
        CHECK(svs::fromText(std::string("VERSION,1\n") + badLines[i] + "\nM2,0,FF0000\n", r, &nb) && nb == 1, "bad line rejected: " << badLines[i]);
        svs::Style want; want.show[svs::K_M2] = false;
        CHECK(r == want, "bad line keeps its default, the good line still applies: " << badLines[i]);
    }
    CHECK(svs::fromText("VERSION,1\nFUTURE,1,2,3\nP1,0,008000\n", r) && !r.show[svs::K_P1], "unknown keys ignored");
    svs::Colour c = 0;
    CHECK(svs::parseHex("00FF00", c) && c == 0x00FF00 && !svs::parseHex("+0FF00", c) && !svs::parseHex(" 0FF00", c), "strict hex");
    CHECK(svs::hex(0xFF8080) == "FF8080" && svs::hex(0x0A0B0C) == "0A0B0C" && svs::hex(0xFF123456UL) == "123456", "hex writer (6 digits)");
}

static void testDialog()
{
    svs::Style keep; keep.col[svs::K_P1] = 0x00AA00;
    svs::Dialog d; svs::Style out;
    CHECK(!svs::fromDialog(d, keep, out), "lists reading -1 (window not open) = nothing to save");
    d.vwapW = 1; d.bandW = 4;
    CHECK(!svs::fromDialog(d, keep, out), "a list index past the choices = nothing to save");
    d.bandW = 0;
    for (int k = 0; k < svs::K_N; ++k) d.col[k] = 0x112233;
    d.col[svs::K_P1] = 0;            // a swatch read back black keeps the saved colour
    d.show[svs::K_M2] = false; d.on = false; d.priorCol = 0x050505;
    CHECK(svs::fromDialog(d, keep, out), "a real window saves");
    CHECK(out.vwapW == 2 && out.bandW == 1, "list index -> px");
    CHECK(out.col[svs::K_P1] == 0x00AA00 && out.col[svs::K_VWAP] == 0x112233, "near-black swatch keeps the saved colour");
    CHECK(!out.show[svs::K_M2] && !out.on && out.priorCol == svs::D_PRTH, "boxes + prior colour kept when its swatch reads near-black");
    CHECK(svs::pathFor("C:\\Users\\x", "ES") == "C:\\Users\\x\\InvestorRT\\rtx\\lsFlexLevels\\SessionVWAP.settings-ES.txt", "file path");
    CHECK(svs::pathFor(NULL, "ES").empty() && svs::pathFor("C:", "").empty(), "no profile / market = no file");
    CHECK(svs::marketPart("E/S..") == "E_S__", "market part sanitised");
}

// ------------------------------------------------------------------------------------------------ part 2: the real plugin
static RTDATE at(int y, int mo, int d, int h, int mi)
{
    struct tm t = {}; t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; t.tm_min = mi;
    return (RTDATE)timegm(&t);
}
// 3-min ES bars: Mon 12 Oct RTH, the overnight from 17:03, and (if `rthToo`) Tue 13 Oct RTH up to 10:00
static void bars(mock::Host& H, bool rthToo)
{
    std::vector<RTDATE> T;
    for (RTDATE t = at(2026, 10, 12, 8, 33); t <= at(2026, 10, 12, 15, 0); t += 180) T.push_back(t);
    for (RTDATE t = at(2026, 10, 12, 17, 3); t <= at(2026, 10, 13, 8, 30); t += 180) T.push_back(t);
    if (rthToo) for (RTDATE t = at(2026, 10, 13, 8, 33); t <= at(2026, 10, 13, 10, 0); t += 180) T.push_back(t);
    const size_t n = T.size();
    for (int a = 0; a < 14; ++a) H.f[a].assign(n, 0.0f);
    for (int a = 0; a < 2; ++a) H.i[a].assign(n, 0UL);
    for (size_t i = 0; i < n; ++i) {
        const float tp = 5800.0f + 12.0f * (float)std::sin((double)i / 9.0) + 0.05f * (float)i;
        H.i[(int)barDateTime][i] = T[i]; H.i[(int)barVolume][i] = 100 + (unsigned long)(i % 37) * 20;
        H.f[(int)barHigh][i] = tp + 1.5f; H.f[(int)barLow][i] = tp - 1.25f; H.f[(int)barClose][i] = tp + 0.25f;
    }
}
// what drawLines must draw: per output, the joined pairs of positive values in the visible range (mock: the last 300 bars)
static int segments(mock::Host& H, int k)
{
    const int n = (int)H.f[(int)barClose].size();
    int a = n > 300 ? n - 300 : 0, b = n - 1; if (a > 0) a--;
    int c = 0;
    for (int i = a + 1; i <= b; ++i) { const float y0 = H.f[k][(size_t)i - 1], y1 = H.f[k][(size_t)i]; if (y0 > 0 && y1 > 0) ++c; }
    return c;
}
static std::map<std::pair<unsigned long, int>, int> drawn(mock::Host& H)
{
    std::map<std::pair<unsigned long, int>, int> m;
    for (size_t i = 0; i < H.lines.size(); ++i) m[std::make_pair((unsigned long)H.lines[i].c, (int)H.lines[i].w)]++;
    return m;
}
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }

static void testPlugin(const std::string& profile)
{
    mock::declared.clear();
    SessionVWAP x;
    mock::Host H; H.root = "ES"; mock::current = &H;
    CHECK(x.setup() == RTX_OK, "setup");
    // the window: 15 rows, controls only, the right kinds, defaults = 1.4.2's look
    CHECK(mock::declared.size() == (size_t)svs::R_COUNT, "16 settings rows (1.8.0 + pivot; the 11 outputs are not rows in the mock)");
    const int kind[svs::R_COUNT] = { 1, 2, 3, 1, 2, 1, 2, 1, 2, 1, 2, 3, 1, 1, 2, 1 };
    for (int r = 0; r < svs::R_COUNT; ++r) CHECK(mock::declared[(size_t)x.P[r]].type == kind[r], "row " << r << " is a " << (kind[r] == 1 ? "checkbox" : kind[r] == 2 ? "colour" : "list"));
    CHECK(mock::declared[(size_t)x.P[svs::R_VWAP_W]].list == 1 && mock::declared[(size_t)x.P[svs::R_VWAP_W]].listCount == 4, "VWAP width list 1-4 px, default 2 px");
    CHECK(mock::declared[(size_t)x.P[svs::R_BAND_W]].list == 0, "band width default 1 px");
    CHECK(mock::declared[(size_t)x.P[svs::R_P1_COL]].c == svs::D_P1 && mock::declared[(size_t)x.P[svs::R_PRIOR_COL]].c == svs::D_PRTH, "colour defaults");
    for (size_t r = 0; r < mock::declared.size(); ++r) {
        const std::string& nm = mock::declared[r].name;
        CHECK(!nm.empty() && nm.size() <= 30 && nm.find(". ") == std::string::npos, "row name is a short control label, not how-to text: " << nm);
    }

    // RTH day: no file -> every line in its default pen, exactly the visible segments
    bars(H, true); x.fill(0);
    const std::string esFile = svs::pathFor(profile.c_str(), "ES");
    std::remove(esFile.c_str());
    H.lines.clear(); x.draw();
    std::map<std::pair<unsigned long, int>, int> m = drawn(H);
    int want[11]; for (int k = 0; k < 11; ++k) want[k] = segments(H, k);
    CHECK(want[0] > 20 && want[1] > 20 && want[5] > 20 && want[10] > 0, "the scenario has RTH, O/N and prior-RTH segments on screen");
    CHECK(m[std::make_pair(0xFF80FFUL, 2)] == want[5], "(1.8.0) VWAP magenta 2 px only overnight: the RTH line is the 3-bar pivot VWAP");
    CHECK(m[std::make_pair(0x22C55EUL, 2)] + m[std::make_pair(0xEF4444UL, 2)] > 0, "(1.8.0) the 3-bar pivot VWAP is drawn green / red at the VWAP width");
    CHECK(m[std::make_pair(0x008000UL, 1)] == want[1] + want[6], "+1 SD green 1 px");
    CHECK(m[std::make_pair(0xFF0000UL, 1)] == want[4] + want[9], "-2 SD red 1 px");
    CHECK(m[std::make_pair(0x6B7280UL, 1)] == want[10], "prior RTH VWAP grey");
    int total = 0; for (int k = 0; k < 11; ++k) total += want[k];
    CHECK((int)H.lines.size() == total, "nothing else drawn");

    // a closed window never saves (IRT: lists -1, boxes unchecked) - the chart keeps every line
    x.parmsApply(); x.parmsUpdt(0);
    CHECK(svs::fileStamp(esFile) < 0, "Apply without a real window writes nothing");

    // the window: untick +1 SD -> saved at once (parmsUpdt) -> the next draw has no +1 SD line, everything else unchanged
    H.dialog = true; x.parmsLoad();
    CHECK(svs::fileStamp(esFile) < 0, "opening the window never writes (lesson 29)");
    x.checkBox(x.P[svs::R_P1], false); x.parmsUpdt((unsigned)x.P[svs::R_P1]); H.dialog = false;
    CHECK(svs::fileStamp(esFile) >= 0, "a change in the window is saved for the market");
    H.lines.clear(); x.draw(); m = drawn(H);
    CHECK(m[std::make_pair(0x008000UL, 1)] == 0, "+1 SD unticked: its line is gone (RTH and O/N)");
    CHECK(m[std::make_pair(0xFF80FFUL, 2)] == want[5] && m[std::make_pair(0xFF8080UL, 1)] == want[2] + want[7], "the other lines stay");
    const int pv0 = m[std::make_pair(0x22C55EUL, 2)] + m[std::make_pair(0xEF4444UL, 2)];
    CHECK((int)H.lines.size() == total - want[0] + pv0 - want[1] - want[6], "exactly the +1 SD segments removed (1.8.0: RTH VWAP segments = pivot segments)");

    // every line, one at a time: ticked -> drawn, unticked -> gone
    const int showRow[5] = { svs::R_VWAP, svs::R_P1, svs::R_M1, svs::R_P2, svs::R_M2 };
    for (int k = 0; k < 5; ++k) {
        H.dialog = true; for (int j = 0; j < 5; ++j) x.checkBox(x.P[showRow[j]], j != k); x.parmsApply(); H.dialog = false;
        H.lines.clear(); x.draw();
        CHECK((int)H.lines.size() == total - want[k] - want[k + 5], "line " << k << " unticked removes only its RTH + O/N segments");
    }
    H.dialog = true; for (int j = 0; j < 5; ++j) x.checkBox(x.P[showRow[j]], true);
    x.checkBox(x.P[svs::R_PRIOR], false); x.parmsApply(); H.dialog = false;
    H.lines.clear(); x.draw();
    CHECK((int)H.lines.size() == total - want[10] && drawn(H)[std::make_pair(0x6B7280UL, 1)] == 0, "prior RTH VWAP unticked");
    H.dialog = true; x.checkBox(x.P[svs::R_PRIOR], true); x.checkBox(x.P[svs::R_ON], false); x.parmsApply(); H.dialog = false;
    H.lines.clear(); x.draw();
    CHECK((int)H.lines.size() == total - want[5] - want[6] - want[7] - want[8] - want[9], "overnight set unticked: the five O/N lines go, RTH + prior stay");

    // colours and widths are applied
    H.dialog = true; x.checkBox(x.P[svs::R_ON], true);
    x.setParameterColor(x.P[svs::R_M2_COL], 0x00FFFF00); x.setListIndex(x.P[svs::R_VWAP_W], 3); x.setListIndex(x.P[svs::R_BAND_W], 1);
    x.parmsApply(); H.dialog = false;
    H.lines.clear(); x.draw(); m = drawn(H);
    CHECK(m[std::make_pair(0xFFFF00UL, 2)] == want[4] + want[9], "-2 SD in the new colour, at the band width");
    CHECK(m[std::make_pair(0xFF0000UL, 1)] == 0 && m[std::make_pair(0xFF0000UL, 2)] == 0, "the old -2 SD colour is gone");
    CHECK(m[std::make_pair(0xFF80FFUL, 4)] == want[5] && m[std::make_pair(0x22C55EUL, 4)] + m[std::make_pair(0xEF4444UL, 4)] > 0, "VWAP at 4 px (and the pivot VWAP)");
    CHECK(m[std::make_pair(0x008000UL, 2)] == want[1] + want[6], "bands at 2 px");

    // a black swatch read back from the window keeps the saved colour (no invisible lines on his black chart)
    H.dialog = true; x.setParameterColor(x.P[svs::R_P2_COL], 0); x.parmsApply(); H.dialog = false;
    H.lines.clear(); x.draw();
    CHECK(drawn(H)[std::make_pair(0x00FF00UL, 2)] == want[3] + want[8], "+2 SD keeps lime when its swatch reads black");
    const std::string saved = readAll(esFile);
    CHECK(saved.find("M2,1,FFFF00\n") != std::string::npos && saved.find("VWAP,1,FF80FF,4\n") != std::string::npos && saved.find("BANDW,2\n") != std::string::npos, "the file holds the choices");

    // a second chart of the same market: its window opens on the saved choices; another market keeps the defaults
    mock::Host H2; H2.root = "ES"; mock::current = &H2; bars(H2, true); x.fill(0);
    H2.dialog = true; x.parmsLoad();
    CHECK(x.getListIndex(x.P[svs::R_VWAP_W]) == 3 && x.getParameterColor(x.P[svs::R_M2_COL]) == 0xFFFF00, "the second ES chart's window shows the saved choices");
    H2.dialog = false; H2.lines.clear(); x.draw();
    CHECK(drawn(H2)[std::make_pair(0xFFFF00UL, 2)] == want[4] + want[9], "the second ES chart draws the same style");
    mock::Host H3; H3.root = "NQ"; mock::current = &H3; bars(H3, true); x.fill(0); H3.lines.clear(); x.draw();
    CHECK(drawn(H3)[std::make_pair(0xFF0000UL, 1)] == want[4] + want[9] && drawn(H3)[std::make_pair(0xFF80FFUL, 2)] == want[5], "NQ keeps the defaults (settings are per market)");
    // (1.6.2, msglog 2026-10-21 09:51 "lsSessionVWAP Ver 1.6.1 in chart "???" calling parmsLoad()") a window copy with NO chart:
    // it used to open blank (VWAP / bands unticked, colour black). Now it opens on this plugin's last market (ES) - filled - and OK on
    // the real chart saves only what he changed; an Apply that still has no chart saves nothing.
    {
        mock::Host H5; H5.root = ""; H5.dialog = true; mock::current = &H5;
        x.parmsLoad();
        CHECK(x.isBoxChecked(x.P[svs::R_VWAP]) && x.getListIndex(x.P[svs::R_VWAP_W]) == 3 && x.getParameterColor(x.P[svs::R_M2_COL]) == 0xFFFF00,
              "a chart-less window copy opens FILLED from the last market's file (VWAP ticked, width 4 px, -2 SD yellow)");
        x.checkBox(x.P[svs::R_P1], false); x.parmsUpdt((unsigned)x.P[svs::R_P1]);
        const std::string before = readAll(esFile);
        x.parmsApply();
        CHECK(readAll(esFile) == before, "an Apply that still has no chart saves nothing (never into a guessed market)");
        H5.root = "ES"; x.parmsApply();                                   // IRT attaches the copy to its chart for OK
        const std::string after = readAll(esFile);
        CHECK(after.find("P1,0,") != std::string::npos && after.find("M2,1,FFFF00\n") != std::string::npos && after.find("VWAP,1,FF80FF,4\n") != std::string::npos,
              "OK on the real chart saves the one change (+1 SD off) and keeps every other choice");
        mock::Host H6; H6.root = "ES"; H6.dialog = true; mock::current = &H6; x.parmsLoad();
        CHECK(!x.isBoxChecked(x.P[svs::R_P1]) && x.isBoxChecked(x.P[svs::R_M1]), "re-opened: the window shows what was saved");
        x.checkBox(x.P[svs::R_P1], true); x.parmsApply();
        mock::current = &H3;                                              // as before: the next done() frees H3's chart
    }
    x.done(); mock::current = &H2; x.done(); mock::current = &H;

    // a damaged file: defaults, no crash; then a good file again is picked up without a restart (stamp change)
    { std::ofstream f(esFile.c_str(), std::ios::trunc); f << "garbage\x01\x02"; }
    mock::Host H4; H4.root = "ES"; mock::current = &H4; bars(H4, true); x.fill(0); H4.lines.clear(); x.draw();
    CHECK(drawn(H4)[std::make_pair(0xFF80FFUL, 2)] == want[5], "garbled file = the defaults");
    svs::Style s; s.show[svs::K_VWAP] = false;
    sleep(1);                                                            // a new mtime second (the stamp) and past the 1 s check
    { std::ofstream f(esFile.c_str(), std::ios::trunc); f << svs::toText(s); }
    H4.lines.clear(); x.draw();
    CHECK(drawn(H4)[std::make_pair(0xFF80FFUL, 2)] == 0 && !H4.lines.empty(), "a changed file is followed live (VWAP off)");
    x.done(); mock::current = &H; x.done();
    std::remove(esFile.c_str());
}

static void testVersion()
{
    std::ifstream f("SessionVWAP.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string src = ss.str();
    const size_t a = src.find("setVersion(\""), b = src.find("SV_VERSION = \"");
    CHECK(a != std::string::npos && b != std::string::npos, "both version literals present");
    if (a != std::string::npos && b != std::string::npos) {
        const std::string v1 = src.substr(a + 12, src.find('"', a + 12) - a - 12), v2 = src.substr(b + 14, src.find('"', b + 14) - b - 14);
        CHECK(v1 == v2 && v1.size() >= 5 && v1 >= "1.5.0", "setVersion literal = SV_VERSION (compared, never typed - lesson 79; got " << v1 << " / " << v2 << ")");
    }
}

int main()
{
    char tmpl[] = "/tmp/svsetXXXXXX"; char* dir = mkdtemp(tmpl);
    if (!dir) { std::cerr << "no temp dir\n"; return 1; }
    const std::string profile = std::string(dir) + "/u";      // files land in `dir` as "u\InvestorRT\rtx\lsFlexLevels\..."
    setenv("USERPROFILE", profile.c_str(), 1);
    testDefaults(); testPens(); testText(); testDialog(); testPlugin(profile); testVersion();
    std::string cmd = std::string("rm -rf ") + dir; if (std::system(cmd.c_str())) {}
    std::cout << "SessionVWAP settings: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
