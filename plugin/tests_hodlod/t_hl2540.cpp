// t_hl2540.cpp -- lsHodLod 2.54.0 / HodLodSweeps 1.4.0 / HodLodLast5 1.0.0 (Rassul 2026-11-26 14:59-15:18 CT, LastFive-Vertical-mockup.html,
// "build"): the LAST 5 DAYS panel - L5 rows parsed (bad rows skipped), day above the date, HOD / LOD time + <= 2 levels + the 1st tag,
// flush left, centred vertically in the band between the Event Calendar and the Level line, oldest days dropped when short; the
// HOD / LOD box centred on the candles again (cx).
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2540.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodSweeps.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
static int M(const std::string& s, bool b) { return (int)s.size() * 6 + (b ? (int)s.size() : 0); }
int main() {
    C(std::string(hl::HL_VERSION) == "2.54.0" && std::string(hlset::PLUGIN_VERSION) == "2.54.0", "versions 2.54.0");
    C(std::string(hsw::VERSION) == "1.4.0" && std::string(l5::VERSION) == "1.0.0", "sweeps 1.4.0, last5 1.0.0");
    // the real nightly file (hodlod_levels 1.15.0, ES built 2026-11-26)
    std::ifstream in("es_sweeps_1126.csv"); std::stringstream ss; ss << in.rdbuf(); const std::string txt = ss.str();
    const hsw::File F = hsw::parse(txt, "ES");
    C(F.ok && F.l5.size() == 5, "real ES file: 5 days");
    C(F.l5.size() == 5 && F.l5[0].date == "2026-11-25" && F.l5[0].first == "LOD" && F.l5[0].hodT == 897 && F.l5[0].hodLv == "AsiaHI PS0" && F.l5[0].lodLv == "CWL PS", "11/25 row");
    C(F.l5.size() == 5 && F.l5[2].hodLv.empty() && F.l5[2].lodLv.empty(), "11/23: none swept = empty");
    C(F.l5.size() == 5 && F.l5[4].date == "2026-11-19" && F.l5[4].first == "HOD", "oldest last");
    C(!F.hi.empty() && F.secK == 4, "the other rows still read");
    // bad rows skipped, never a crash
    const std::string bad = "VERSION,1\nMARKET,ES\nROW,HI,PHH,45,52,120\n"
        "L5,2026-11-25,LOD,897,A B C,528,-\nL5,2026-13-01,HOD,500,-,600,-\nL5,2026-11-24,MID,500,-,600,-\nL5,2026-11-24,HOD,1500,-,600,-\n"
        "L5,2026-11-24,HOD,500,ONH;rm,600,-\nL5,2026-11-24,HOD,500,ONH  PDH,600,-\nL5,2026-11-24,HOD,500\nL5,2026-11-24,HOD,500,,600,-\n"
        "L5,2026-11-23,HOD,543,ONH AsiaHI,897,-\n";
    const hsw::File B = hsw::parse(bad, "ES");
    C(B.ok && B.l5.size() == 1 && B.l5[0].date == "2026-11-23", "8 bad L5 rows skipped, the good one kept");
    std::string many = "VERSION,1\nMARKET,ES\nROW,HI,PHH,45,52,120\n"; for (int i = 1; i <= 9; i++) many += "L5,2026-11-0" + std::to_string(i) + ",HOD,500,-,600,-\n";
    C(hsw::parse(many, "ES").l5.size() == 5, "at most 5 days kept");
    C(hsw::parse("VERSION,1\nMARKET,ES\nROW,HI,PHH,45,52,120\n", "ES").l5.empty(), "an older file: no days");
    // day / date / time
    C(l5::dow("2026-11-25") == "Wed" && l5::dow("2026-11-19") == "Thu" && l5::dow("2026-11-23") == "Mon" && l5::dow("2024-02-29") == "Thu" && l5::dow("2027-01-01") == "Fri", "day of the week");
    C(l5::md("2026-11-25") == "11/25" && l5::md("2026-01-05") == "1/05", "date");
    C(l5::clk(897) == "2:57" && l5::clk(510) == "8:30" && l5::clk(720) == "12:00", "12-hour CT time");
    // the calendar's height from its status file
    C(l5::calHeight("lsEventCal 1.1.0\nVERSION,1.1.0\nMARKET,ES\nSOURCE,file\nPLACE,top left 0,0 212x34\nROW,none\n") == 34, "calendar 34 px");
    C(l5::calHeight("VERSION,1.1.0\nPLACE,nothing to show\n") == -1 && l5::calHeight("PLACE,top left 0,0 212x\n") == -1 && l5::calHeight("") == -1 && l5::calHeight("PLACE,top left 0,0 2x9999\n") == -1, "calendar not drawn / garbled = -1");
    C(l5::calHeight("PLACE,top left 0,0 212x30\r\n") == 30, "CRLF ok");
    // layout: band 70 .. 470, pane left 0, last bar 800
    const l5::Draw P = l5::layout(F.l5, M, 0, 70, 470, 800);
    C(P.ok && P.shown == 5, "drawn, 5 days");
    C(P.l == -1, "flush left: the left border just off the pane edge (no black space)");
    C(std::abs((P.t - 70) - (470 - P.b)) <= 1, "centred vertically in the band");
    C(P.b - P.t == l5::height(5), "height");
    int tags = 0, days = 0, dates = 0; bool inBox = true;
    for (const l5::Item& it : P.items) { if (it.k == 1 && it.s == "1st") tags++; if (it.k == 1 && (it.s == "Wed" || it.s == "Tue" || it.s == "Mon" || it.s == "Fri" || it.s == "Thu")) days++;
        if (it.k == 1 && it.s.size() == 5 && it.s[2] == '/') dates++;
        if (it.k != 0 && (it.l < P.l || it.r > P.r || it.t < P.t || it.b > P.b)) inBox = false; }
    C(tags == 5 && days == 5 && dates == 5, "a 1st tag, a day and a date per day");
    C(inBox, "everything inside the panel");
    // the day above the date: the "Wed" row is above the "11/25" row
    int yW = -1, yD = -1; for (const l5::Item& it : P.items) { if (it.k == 1 && it.s == "Wed") yW = it.t; if (it.k == 1 && it.s == "11/25") yD = it.t; }
    C(yW >= 0 && yD == yW + l5::ROW, "day on the HOD row, date under it");
    // text never overlaps on a row
    bool ov = false; for (size_t a = 0; a < P.items.size(); a++) for (size_t b = a + 1; b < P.items.size(); b++) { const l5::Item& x = P.items[a]; const l5::Item& y = P.items[b];
        if (x.k == 1 && y.k == 1 && x.t == y.t && x.s != "1st" && y.s != "1st" && x.l < y.r && y.l < x.r) ov = true; }
    C(!ov, "no overlapping text");
    C(has(P.text, "Wed 11/25 HOD 2:57 AsiaHI PS0 LOD 8:48 CWL PS [1st]") && has(P.text, "Thu 11/19 HOD 8:36 PhrH AsiaHI [1st]"), "status text");
    // short band: the oldest days drop; none fit = not drawn
    const l5::Draw S3 = l5::layout(F.l5, M, 0, 100, 100 + l5::height(3) + 5, 800);
    C(S3.ok && S3.shown == 3 && has(S3.text, "Wed 11/25") && !has(S3.text, "Thu 11/19"), "short band: the newest 3 days");
    C(!l5::layout(F.l5, M, 0, 100, 140, 800).ok, "no room: not drawn");
    C(!l5::layout(F.l5, M, 0, 70, 470, 120).ok, "too narrow: not drawn");
    C(!l5::layout(std::vector<l5::Day>(), M, 0, 70, 470, 800).ok, "no rows: not drawn");
    // the box centred on the candles (cx = the middle of the room)
    hl::View v; v.tmOk = hsw::timeCum(F, v.tmCum); v.tmOpenMin = 510; v.tm2Ok = hsw::timeCum2(F, v.tm2Cum, v.tm2Open); v.secOk = hsw::secRow(F, v.secK, v.secMin, v.secPct, v.secLast, v.secOpen);
    const hsb::Draw H = hsb::layout(v, M, 600, 900, 0, 1200);
    C(H.ok && std::abs((H.l + H.r) / 2 - 600) <= 1, "HOD / LOD box centred");
    std::printf("t_hl2540: %d pass, %d fail\n", ok, f);
    return f ? 1 : 0;
}
