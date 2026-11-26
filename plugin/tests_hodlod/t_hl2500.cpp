// t_hl2500.cpp -- lsHodLod 2.50.0 / HodLodSweeps 1.2.0 (Rassul 2026-11-26 10:31-13:27 CT "lets use 30 min increments from rth start", "lets use
// 3.5 hrs instead", "move this box to the left of the chart and dont have any black space to the left", "go"): HOD / LOD Stats in two rows -
// 1st by the open + 30 / 60 / 90 / 120 min + after (TIME2), 2nd by the open + 3.5 h hourly + last 30 (SEC2); the box docked bottom-LEFT.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2500.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodSweeps.h"
#include <cstdio>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
static std::string file(const std::string& mkt, const std::string& rows) {
    return "VERSION,1\nMARKET," + mkt + "\nROW,HI,PHH,43,52,120\nTIME,46,15,16,23,252\n" + rows + "ONCUT,0.6273,0.9435\nLIFT,HI,PHH,1.08,42,0.80,0\n";
}
static hl::View view(const hsw::File& F, int open) {
    hl::View v; v.tmOk = hsw::timeCum(F, v.tmCum); v.tmOpenMin = open; v.tm2Ok = hsw::timeCum2(F, v.tm2Cum, v.tm2Open);
    v.secOk = hsw::secRow(F, v.secK, v.secMin, v.secPct, v.secLast, v.secOpen); return v; }
int main() {
    C(std::string(hl::HL_VERSION) == std::string(hlset::PLUGIN_VERSION) && std::string(hl::HL_VERSION) >= "2.50.0", "versions match, 2.50.0 or later");
    auto meas = [](const std::string& s, bool bold) { return (int)s.size() * (bold ? 7 : 6); };
    // ES as hodlod_levels 1.12.0 writes it (fresh store, 11-26)
    const hsw::File F = hsw::parse(file("ES", "TIME2,510,45,61,78,87,94,6,252\nSEC2,510,4,210,37,270,53,330,63,360,74,26,252\n"), "ES");
    const hl::View v = view(F, 510);
    C(F.ok && v.tm2Ok && v.secOk && v.secK == 4 && v.secLast == 26, "TIME2 + SEC2 parsed (the LIFT row is ignored)");
    hsb::Draw B = hsb::layout(v, meas, 0, 500, 0, 1400);
    C(B.ok && has(B.text, "; 1st 9:00=61% 9:30=78% 10:00=87% 10:30=94% after=6%"), "1st row: 30-min steps from the 8:30 open + after: " + B.text);
    C(has(B.text, "; 2nd 12:00=37% 1:00=53% 2:00=63% 2:30=74% last 30=26%"), "2nd row: 12:00 .. 2:30 + last 30: " + B.text);
    C(B.l == 0 && B.b == 500, "docked: left edge on the pane's left, bottom on the pane's bottom");
    for (const hsb::Item& it : B.items) C(it.l >= B.l && it.r <= B.r, "inside the box: " + it.s);
    {   // the two rows: every 1st cell on row 1, every 2nd cell on row 2, the columns aligned (same right edge for the % of a column)
        int y1 = -1, y2 = -1; int r60 = -1, r37 = -1;
        for (const hsb::Item& it : B.items) { if (it.k != 1) continue; if (it.s == "61%") { y1 = it.t; r60 = it.r; } if (it.s == "37%") { y2 = it.t; r37 = it.r; } }
        C(y1 >= 0 && y2 > y1, "1st on the Exp row, 2nd on the Act row"); C(r60 > 0 && std::abs(r60 - r37) <= 6, "(2.55.0) column 1 over both rows (the 9:00 61% cells centred in their column)"); }
    // CL: 3 steps (11:30 / 12:30 / 1:00) + last 30; GC: 2 steps (10:50 / 12:00)
    { const hsw::File G = hsw::parse(file("CL", "TIME2,480,36,49,65,78,87,13,252\nSEC2,480,3,210,40,270,59,300,74,26,252\n"), "CL");
      B = hsb::layout(view(G, 480), meas, 0, 500, 0, 1400); C(has(B.text, "; 2nd 11:30=40% 12:30=59% 1:00=74% last 30=26%"), "CL 2nd row: " + B.text); }
    { const hsw::File G = hsw::parse(file("GC", "TIME2,440,29,40,59,77,89,11,252\nSEC2,440,2,210,56,280,82,18,252\n"), "GC");
      B = hsb::layout(view(G, 440), meas, 0, 500, 0, 1400); C(has(B.text, "; 1st 7:50=40% 8:20=59% 8:50=77% 9:20=89% after=11% ; 2nd 10:50=56% 12:00=82% last 30=18%"), "GC: " + B.text); }
    // no SEC2 (an older nightly file): the 1st row only, nothing on row 2
    { const hsw::File O = hsw::parse(file("ES", "TIME2,510,45,61,78,87,94,6,252\n"), "ES"); const hl::View o = view(O, 510);
      B = hsb::layout(o, meas, 0, 500, 0, 1400); C(!o.secOk && B.ok && has(B.text, "; 2nd") && !has(B.text, "last 30"), "no SEC2: no 2nd row: " + B.text); }
    // garbled SEC2 rows are ignored
    const char* bad[] = { "SEC2,510,4,210,37,270,33,330,63,360,74,26,252\n", "SEC2,510,4,210,37,270,53,330,63\n", "SEC2,1500,1,210,37,26,252\n",
                          "SEC2,510,9,210,37,26,252\n", "SEC2,510,2,270,37,210,53,26,252\n", "SEC2,510,1,210,137,26,252\n", "SEC2,510,1,210,37,x,252\n" };
    for (const char* b : bad) { const hsw::File X = hsw::parse(file("ES", b), "ES"); int k, m[8], p[8], la, op; C(X.ok && !hsw::secRow(X, k, m, p, la, op), std::string("rejected: ") + b); }
    // no room: not drawn, says why
    B = hsb::layout(v, meas, 0, 500, 0, 300); C(!B.ok && has(B.why, "no room"), "too narrow: not drawn (" + B.why + ")");
    std::printf("t_hl2500 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, ok, f);
    return f ? 1 : 0;
}
