// build (plugin\tests_readbox): g++ -std=c++17 -Wall -Wextra -I.. t_rb.cpp -o t_rb && ./t_rb   (bar files: CT END stamps from data\irt\bars 3m)
// t_rb.cpp - ReadBox.h 1.0.0: parity with the research read (readbox/compute.py, the mockup moments), no repaint, session-aware, layout
#include "../ReadBox.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <ctime>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
static std::vector<rbx::Bar> load(const char* p) { std::vector<rbx::Bar> v; std::ifstream f(p); rbx::Bar b; while (f >> b.t >> b.o >> b.h >> b.l >> b.c) v.push_back(b); return v; }
static long long T(int y, int mo, int d, int h, int mi) { struct tm t = {}; t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; t.tm_min = mi; return (long long)timegm(&t); }
static rbx::In upto(const std::vector<rbx::Bar>& all, long long end, const char* mkt, int dec) { rbx::In I; I.mkt = mkt; I.dec = dec; for (auto& b : all) if (b.t <= end) I.bars.push_back(b); return I; }
int main() {
    const auto ES = load("ES_3m.txt"), NQ = load("NQ_3m.txt"), GC = load("GC_3m.txt");
    CHECK(ES.size() > 5000 && NQ.size() > 5000, "bar files");
    // ---- 1 PARITY: the HTF line with the research (60-min bars; the research's hour = the bar that STARTS at M, so bars to M + 60 min)
    struct P { const std::vector<rbx::Bar>* B; const char* m; int dec; long long end; const char* want; };
    const P ps[] = { { &ES, "ES", 2, T(2026, 11, 6, 9, 3), "HTF: Rot Up, Green, CLBO, HH, HEC, Big Th, Swept R 7427.25" },
                     { &ES, "ES", 2, T(2026, 11, 6, 14, 45), "HTF: Rot Dn, Red, MEC" },
                     { &NQ, "NQ", 2, T(2026, 11, 6, 13, 3), "HTF: Rot Dn, Green, HEC, Swept S 27037.75" },
                     { &ES, "ES", 2, T(2026, 11, 5, 7, 3), "HTF: Rot Up, Red, CLBD, HH, MEC, Sm Th, Swept R 7737.25" } };
    int same = 0;
    for (const P& p : ps) { const rbx::In I = upto(*p.B, p.end, p.m, p.dec); const rbx::Htf h = rbx::htfLine(I); const bool ok = h.line.text == p.want; same += ok;
        std::printf("  %s %s\n    plugin   %s\n    research %s\n", p.m, ok ? "SAME" : "DIFF", h.line.text.c_str(), p.want); }
    CHECK(same >= 3, "HTF parity with the research read on " + std::to_string(same) + " / 4 moments");
    // ---- 2 NO REPAINT: a read for a closed bar never changes when later bars / later-decided signals arrive
    {   rbx::In base = upto(GC, T(2026, 11, 6, 9, 0), "GC", 1); base.day.kind = 1; base.day.pBig = 0.7; base.day.adr = 60; base.day.xr = 1.3;
        base.levels = { { "PDL", 4035.1 }, { "ONH", 4078.0 }, { "WPoc", 4055 } }; base.mq = { { "PS0", 4050 } };
        rbx::Sig s1; s1.t = T(2026, 11, 6, 8, 57); s1.code = "I"; s1.bias = 1; s1.px = 4054.1; s1.mult = 4.4; base.sigs.push_back(s1);
        const rbx::Out a = rbx::build(base);
        rbx::In more = upto(GC, T(2026, 11, 6, 12, 0), "GC", 1); more.day = base.day; more.levels = base.levels; more.mq = base.mq; more.sigs = base.sigs;
        rbx::Sig s2 = s1; s2.t = T(2026, 11, 6, 9, 30); s2.code = "T"; s2.bias = -1; more.sigs.push_back(s2);
        rbx::In cut = more; cut.bars.clear(); for (auto& b : more.bars) if (b.t <= a.at) cut.bars.push_back(b);
        const rbx::Out b = rbx::build(cut); bool eq = a.ok && b.ok; for (int k = 0; k < 5; k++) eq = eq && a.l[k].text == b.l[k].text && a.l[k].col == b.l[k].col;
        CHECK(eq, "no repaint: the 9:00 read is the same with later bars / a later-decided T present");
        CHECK(a.l[3].text == "Mkt: Buyers Pushing / B Initiating 4.4x @4054.1" && a.l[3].col == rbx::GREEN, "Mkt: " + a.l[3].text);
        for (int k = 0; k < 5; k++) std::printf("  GC 9:00  %s\n", a.l[k].text.c_str()); }
    // ---- 3 SESSION-AWARE: Sunday/overnight shows overnight context, never the prior RTH's read
    {   rbx::In I = upto(ES, T(2026, 11, 6, 15, 57), "ES", 2); I.day.kind = 1; I.day.pBig = 0.82; I.day.adr = 85.3; I.day.xr = 1.35;
        rbx::Bar x = I.bars.back(); for (int k = 1; k <= 60; k++) { x.t = T(2026, 11, 8, 17, 0) + 180 * k; I.bars.push_back(x); }   // Sunday evening bars
        const rbx::Out o = rbx::build(I);
        CHECK(o.l[1].text.find("Day: Overnight") == 0 && o.l[1].text.find("Big 82%") == std::string::npos, "overnight Day line: " + o.l[1].text);
        rbx::In R = upto(ES, T(2026, 11, 6, 9, 30), "ES", 2); R.day = I.day; const rbx::Out r = rbx::build(R);
        CHECK(r.l[1].text.find("Day: Big 82%") == 0 && r.l[1].col == rbx::BLUE && r.l[1].text.find("-> PB Revs, Hold CoB") != std::string::npos, "RTH Day line: " + r.l[1].text);
        rbx::In C = upto(ES, T(2026, 11, 6, 15, 57), "ES", 2); C.day = I.day; const rbx::Out c = rbx::build(C);
        CHECK(c.l[1].text.find("closed") != std::string::npos, "after the close: " + c.l[1].text); }
    // ---- 4 LAYOUT (ChartView widths of ES / GC / CL charts): 5 lines inside the left half, no overlap, the right slot empty
    {   const int W[3] = { 1180, 860, 1420 }; bool ok = true;
        for (int w : W) { const int paneTop = 40; const rbx::Layout L = rbx::layout(60, paneTop + 2, 60 + w, 700); ok = ok && L.ok && L.box.t == paneTop + 2 && L.line[0].t > L.box.t && L.line[4].b <= L.box.b && L.box.r < L.slot.l && L.box.r <= 60 + w / 2;   // (2.22.1) top edge = pane top + 2
            for (int k = 0; k < 4; k++) ok = ok && L.line[k].b <= L.line[k + 1].t && L.line[k].r <= L.box.r; }
        CHECK(ok, "layout: 5 rows inside the left half on 3 chart widths, no overlap, the right column slot free");
        auto meas = [](const std::string& s) { return (int)s.size() * 6; };
        const std::string f = rbx::fit("Lvl: Testing S LonLO 7210 (1st) + PS0 7200, -> R PDH 7460 (+0.3 ATR), Swept ONH", 200, meas);
        CHECK(f.rfind("Lvl: Testing S", 0) == 0 && meas(f) <= 200, "fit keeps the verdict, cuts parts from the end: " + f); }
    // ---- 6 (1.0.1) every drawn string is 7-bit ASCII (IRT draws ANSI: a UTF-8 arrow showed as mojibake), no bare "-" line
    {   bool ascii = true, dash = false; int n = 0;
        auto chk = [&](const rbx::Out& o) { for (int k = 0; k < 5; k++) { for (unsigned char ch : o.l[k].text) if (ch > 127) ascii = false; const size_t c = o.l[k].text.find(": "); if (c != std::string::npos && o.l[k].text.substr(c + 2) == "-") dash = true; n++; } };
        for (const auto* B : { &ES, &NQ, &GC }) for (size_t e = 200; e < B->size(); e += 37) {
            rbx::In I; I.mkt = "ES"; I.dec = 2; for (size_t k = 0; k <= e; k++) I.bars.push_back((*B)[k]);
            I.day.kind = (int)(e % 4) - 1; I.day.pBig = 0.5; I.day.pSmall = 0.1; I.day.xr = 1.2; I.day.adr = 50;
            I.levels = { { "PDL", (*B)[e].c - 3 }, { "ONH", (*B)[e].c + 4 }, { "WPoc", (*B)[e].c + 9 } }; I.mq = { { "PS0", (*B)[e].c - 2 } };
            chk(rbx::build(I)); }
        rbx::In E; E.bars = { ES[0], ES[1], ES[2] }; chk(rbx::build(E));
        CHECK(ascii && !dash && n > 1000, "every drawn line is 7-bit ASCII and none is a bare '-' (" + std::to_string(n) + " lines)"); }
    // ---- 5 the lsDeltaProfile record parser (decided rows only)
    {   const std::string t = "# x\nt_first|t_decided|code\n1793899080|1793899080|I|support|bullish|1|2|0.1|41|4.37|1|2.32.1|D|GCZ6|12|1|1|4098.1|1|2|buy|node|1|1|conf=same\n"
                              "1793899080|0|I?|support|bullish|1|2|0.1|41|4.37|1|2.32.1|F|GCZ6|12|1|1|4098.1|1|2|buy|node|1|1\nRW|2.34.0|ESZ6|1\n";
        const auto v = rbx::parseSigs(t); CHECK(v.size() == 1 && v[0].code == "I" && v[0].bias == 1 && std::fabs(v[0].px - 4098.1) < 1e-9, "parseSigs: decided rows only"); }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
