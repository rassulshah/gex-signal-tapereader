// t_hl2510.cpp -- lsHodLod 2.51.0 / HodLodSweeps 1.3.0 (Rassul 2026-11-26 13:43-13:46 CT "a line above the box ... stats for levels that Hod swept and
// Lod swept", "two lines, hod is top lod below", "ok"; HodLod-Swept-mockup.html): HOD swept / LOD swept lines on top of the box, each level
// "name pct% (random%)" in the nightly's row order; the box grows by the two rows; the Level line still sits on the box's top edge.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2510.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodSweeps.h"
#include <cstdio>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
static const char* ES = "VERSION,1\nMARKET,ES\n"
    "ROW,HI,PHH,45,52,120\nROW,HI,ON3,40,46,120\nROW,HI,DPoc,26,16,60\nROW,HI,VAH,21,18,86\nROW,HI,PDH,14,14,97\nROW,HI,PFH,12,15,105\n"
    "ROW,LO,PHL,40,53,131\nROW,LO,ON3,35,47,131\nROW,LO,DPoc,26,18,70\nROW,LO,VAL,10,10,89\nROW,LO,PDL,8,8,104\nROW,LO,PFL,5,6,109\n"
    "TIME,46,15,16,23,252\nTIME2,510,45,61,78,87,94,6,252\nSEC2,510,4,210,37,270,53,330,63,360,74,26,252\n"
    "LIFT,HI,PHH,1.08,42,0.80,0\nLIFT,HI,ON3,0.99,40,0.90,0\nLIFT,HI,DPoc,1.24,21,0.63,0\nLIFT,HI,VAH,1.05,20,0.83,0\nLIFT,HI,PDH,0.85,17,0.96,0\nLIFT,HI,PFH,0.85,15,0.96,0\n"
    "LIFT,LO,PHL,1.04,38,0.80,0\nLIFT,LO,ON3,1.06,33,0.80,0\nLIFT,LO,DPoc,1.30,20,0.55,0\nLIFT,LO,VAL,0.78,13,0.97,0\nLIFT,LO,PDL,0.76,10,0.97,0\nLIFT,LO,PFL,0.72,8,0.97,0\n";
static hl::View view(const hsw::File& F) {
    hl::View v; v.tmOk = hsw::timeCum(F, v.tmCum); v.tmOpenMin = 510; v.tm2Ok = hsw::timeCum2(F, v.tm2Cum, v.tm2Open);
    v.secOk = hsw::secRow(F, v.secK, v.secMin, v.secPct, v.secLast, v.secOpen);
    for (const std::string& k : F.hiOrd) { hl::SwCell c; c.name = k; c.pct = F.hi.at(k).pct; c.rnd = F.hi.at(k).rnd; v.sxHi.push_back(c); }   // as the host fills it
    for (const std::string& k : F.loOrd) { hl::SwCell c; c.name = k; c.pct = F.lo.at(k).pct; c.rnd = F.lo.at(k).rnd; v.sxLo.push_back(c); }
    return v; }
int main() {
    C(std::string(hl::HL_VERSION) == std::string(hlset::PLUGIN_VERSION) && std::string(hl::HL_VERSION) >= "2.51.0" && std::string(hsw::VERSION) == "1.3.0", "versions match, 2.51.0 or later / 1.3.0");
    auto meas = [](const std::string& s, bool bold) { return (int)s.size() * (bold ? 7 : 6); };
    const hsw::File F = hsw::parse(ES, "ES");
    C(F.ok && F.hiOrd.size() == 6 && F.hiOrd[0] == "PHH" && F.hiOrd[5] == "PFH" && F.loOrd[3] == "VAL", "ROW order kept (not alphabetical)");
    C(F.hi.at("PHH").pct == 45 && F.hi.at("PHH").rnd == 42 && F.lo.at("PFL").rnd == 8, "LIFT random % read into the cells");
    const hl::View v = view(F); hsb::Draw B = hsb::layout(v, meas, 0, 600, 0, 1600);
    C(B.ok && has(B.text, "; HOD swept PhrH=45%(42%) ON=40%(40%) DPoc=26%(21%) VAH=21%(20%) PDH=14%(17%) PFH=12%(15%)"), "HOD line: " + B.text);
    C(has(B.text, "; LOD swept PhrL=40%(38%) ON=35%(33%) DPoc=26%(20%) VAL=10%(13%) PDL=8%(10%) PFL=5%(8%)"), "LOD line: " + B.text);
    C(B.b == 600 && B.b - B.t == 3 * 18 + 4 + 3 * 18 + 3, "(2.52.0) the box grows by the head row + the two swept rows (+ the grey line), still docked on the bottom");
    for (const hsb::Item& it : B.items) C(it.l >= B.l && it.r <= B.r && it.t >= B.t && it.b <= B.b, "inside the box: " + it.s);
    {   // HOD line on top, LOD below, both above the Exp / Act rows; labels in red / green
        int yH = -1, yL = -1, yE = -1; unsigned cH = 0, cL = 0;
        for (const hsb::Item& it : B.items) { if (it.k != 1) continue; if (it.s == "HOD swept") { yH = it.t; cH = it.col; } if (it.s == "LOD swept") { yL = it.t; cL = it.col; } if (it.s == "Exp") yE = it.t; }
        C(yH >= 0 && yL > yH && yE > yL, "HOD swept on top, LOD swept below, then Exp / Act"); C(cH == hl::C_RED && cL == hl::C_GREEN, "labels red / green"); }
    {   // no LIFT rows (an old file): the % without brackets; no ROW rows: no swept lines, the box as 2.50.0
        std::string old = ES; old = old.substr(0, old.find("LIFT,"));
        const hl::View o = view(hsw::parse(old, "ES")); B = hsb::layout(o, meas, 0, 600, 0, 1600);
        C(has(B.text, "HOD swept PhrH=45% ON=40%") && !has(B.text, "(42%)"), "no LIFT: % only: " + B.text);
        hl::View n = o; n.sxHi.clear(); n.sxLo.clear(); B = hsb::layout(n, meas, 0, 600, 0, 1600);
        C(B.ok && B.b - B.t == 3 * 18 + 4 && !has(B.text, "swept"), "no swept data: the 2.50.0 box"); }
    {   // garbled LIFT values are ignored
        std::string bad = std::string(ES) + "LIFT,HI,PHH,1.08,x,0.80,0\nLIFT,HI,PDH,0.85,150,0.96,0\n"; const hsw::File G = hsw::parse(bad, "ES");
        C(G.hi.at("PHH").rnd == 42 && G.hi.at("PDH").rnd == 17, "a bad LIFT random % keeps the good one"); }
    {   // (2.52.0) the table: column heads, HOD / LOD aligned per column, % white above random / grey at or below
        int xHead = -1, x21 = -1, x10 = -1, yHead = -1, yHod = -1; unsigned c21 = 0, c10 = 0;
        const hsb::Draw T = hsb::layout(v, meas, 0, 600, 0, 1600);
        for (const hsb::Item& it : T.items) { if (it.k != 1) continue;
            if (it.s == "VA") { xHead = (it.l + it.r) / 2; yHead = it.t; } if (it.s == "HOD swept") yHod = it.t;
            if (it.s == "21%") { x21 = it.l; c21 = it.col; } if (it.s == "10%") { x10 = it.l; c10 = it.col; } }
        C(yHead >= 0 && yHod > yHead, "column heads above the HOD row");
        C(x21 > 0 && x10 > 0 && std::abs(x21 - x10) <= 8 && std::abs(x21 - xHead) < 40, "VAH 21% and VAL 10% in the same VA column");
        C(c21 == hsb::C_TXT && c10 == hsb::C_HEAD, "VAH 21% (random 20) white, VAL 10% (random 13) grey"); }
    std::printf("t_hl2510 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, ok, f);
    return f ? 1 : 0;
}
