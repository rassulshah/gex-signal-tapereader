// t_hl2490.cpp -- lsHodLod 2.49.0 / HodLodSweeps 1.1.0 (Rassul 2026-11-26 09:37-09:40 CT "lets increase 1st extreme with 9:30 and 10 so it includes
// 2 hours ... and then a probability for after the 2nd hour, which should be the remainder", "build"): TIME2 = the 1st extreme cumulative by
// +15 / 30 / 60 / 90 / 120 min + after (the remainder), labelled from the open the nightly counted from (HG 7:10, was labelled 7:20).
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2490.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodSweeps.h"
#include <cstdio>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
static std::string file(const std::string& mkt, const std::string& time2) {
    return "VERSION,1\nMARKET," + mkt + "\nROW,HI,PHH,43,52,120\nTIME,46,15,16,23,252\n" + time2 + "ONCUT,0.6273,0.9435\n";
}
int main() {
    C(std::string(hl::HL_VERSION) == std::string(hlset::PLUGIN_VERSION) && std::string(hl::HL_VERSION) >= "2.49.0", "versions match, 2.49.0 or later (2.50.0: t_hl2500 covers the 2-row stats)");
    auto meas = [](const std::string& s, bool bold) { return (int)s.size() * (bold ? 7 : 6); };
    // ES as the nightly writes it today (hodlod_levels 1.9.0 on the 252-session store)
    hsw::File F = hsw::parse(file("ES", "TIME2,510,46,60,77,87,93,7,252\n"), "ES");
    int c[6], open = -1; C(F.ok && hsw::timeCum2(F, c, open) && open == 510 && c[0] == 46 && c[4] == 93 && c[5] == 7, "TIME2 parsed (ES)");
    hl::View v; v.tmOk = hsw::timeCum(F, v.tmCum); v.tmOpenMin = 510; v.tm2Ok = hsw::timeCum2(F, v.tm2Cum, v.tm2Open);
    hsb::Draw B = hsb::layout(v, meas, 400, 500, 0, 900);
    C(B.ok && has(B.text, "9:00=60% 9:30=77% 10:00=87% 10:30=93% after=7%"), "ES 2-hour table + after: " + B.text);
    for (const hsb::Item& it : B.items) C(it.l >= B.l && it.r <= B.r, "inside the box: " + it.s);
    // HG: counted from 7:10 by the nightly -> labelled 7:25 / 7:40 / 8:10 / 8:40 / 9:10 (the box's own open is 7:20)
    hsw::File G = hsw::parse(file("HG", "TIME2,430,30,46,67,79,91,9,252\n"), "HG");
    hl::View h; h.tmOk = hsw::timeCum(G, h.tmCum); h.tmOpenMin = 440; h.tm2Ok = hsw::timeCum2(G, h.tm2Cum, h.tm2Open);
    B = hsb::layout(h, meas, 400, 500, 0, 900);
    C(has(B.text, "7:40=46% 8:10=67% 8:40=79% 9:10=91% after=9%"), "HG labelled from the nightly's 7:10: " + B.text);
    // no TIME2 (an old nightly file): the 4-column table as before
    hsw::File O = hsw::parse(file("ES", ""), "ES");
    hl::View o; o.tmOk = hsw::timeCum(O, o.tmCum); o.tmOpenMin = 510; o.tm2Ok = hsw::timeCum2(O, o.tm2Cum, o.tm2Open);
    B = hsb::layout(o, meas, 400, 500, 0, 900);
    C(!o.tm2Ok && has(B.text, "8:45=46% 9:00=61% 9:30=77% 9:30+=23%") && !has(B.text, "after"), "old file: 4 columns: " + B.text);
    // garbled TIME2 rows are ignored (falling numbers, > 100, a bad open, too few fields, text)
    const char* bad[] = { "TIME2,510,46,60,55,87,93,7,252\n", "TIME2,510,46,60,77,87,193,7,252\n", "TIME2,1500,46,60,77,87,93,7,252\n",
                          "TIME2,510,46,60,77,87,93\n", "TIME2,510,46,x,77,87,93,7,252\n", "TIME2,-5,46,60,77,87,93,7,252\n" };
    for (const char* b : bad) { hsw::File X = hsw::parse(file("ES", b), "ES"); int cc[6], op; C(X.ok && !hsw::timeCum2(X, cc, op), std::string("rejected: ") + b); }
    // the box grows to fit 6 columns, still docked and centred
    hl::View w = v; const hsb::Draw W6 = hsb::layout(w, meas, 400, 500, 0, 900); w.tm2Ok = false; const hsb::Draw W4 = hsb::layout(w, meas, 400, 500, 0, 900);
    C(W6.r - W6.l > W4.r - W4.l && W6.b == 500 && std::abs((W6.l + W6.r) / 2 - 400) <= 1, "6 columns: wider, docked, centred");
    std::printf("t_hl2490 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, ok, f);
    return f ? 1 : 0;
}
