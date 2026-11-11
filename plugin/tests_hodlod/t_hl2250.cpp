// t_hl2250.cpp -- lsHodLod 2.25.0 (Rassul 2026-11-11 12:02 CT "will the hod lod box highlight the level that is upcoming or going to be in play ?",
// 12:03 "ok.. lets do it. build"): the sweep tables' UPCOMING level (hl::markSweeps / hl::swPrices).
// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2250.cpp -o t2250 && ./t2250   (from plugin\tests_hodlod)
#include "../HodLodLogic.h"
#include <cstdio>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s\n", m); } } while (0)
static hl::View::SwRow row(const char* n, const char* px, const char* pct, bool elig = true) { hl::View::SwRow r; r.name = n; r.px = px; r.pct = pct; r.elig = elig; return r; }
static int liveOf(const std::vector<hl::View::SwRow>& R) { int k = -1; for (size_t i = 0; i < R.size(); i++) if (R[i].st > 0) k = (int)i; return k; }
static int dims(const std::vector<hl::View::SwRow>& R) { int n = 0; for (const auto& r : R) if (r.dim) ++n; return n; }
int main() {
    { const std::vector<double> p = hl::swPrices("7,597.50 - 7,620.00"); CHECK(p.size() == 2 && p[0] == 7597.5 && p[1] == 7620.0, "a price range gives both prices"); }
    CHECK(hl::swPrices("-").empty() && hl::swPrices("below open").empty() && hl::swPrices("").empty(), "no price text: none");
    { const std::vector<double> p = hl::swPrices("1.08455"); CHECK(p.size() == 1 && p[0] == 1.08455, "a 5-decimal price"); }
    hl::View v;
    v.swHi = { row("PhrH", "-", "46%"), row("ONH / LonHI / AsiaHI", "7,597.50 - 7,620.00", "39%"), row("DPoc", "7,528.75", "26%"), row("PDH", "7,566.25", "16%", false), row("PFH", "7,598.75", "15%") };
    v.swLo = { row("PhrL", "-", "41%"), row("ONL / LonLO / AsiaLO", "7,557.00 - 7,587.25", "36%"), row("DPoc", "7,528.75", "26%"), row("PDL", "7,493.00", "7%") };
    hl::markSweeps(v, 7612.0, 3.75);   // before the first extreme: both tables have their next level
    CHECK(liveOf(v.swHi) == 1 && v.swHi[1].st == 1, "HOD table: the next level above 7,612 is LonHI 7,620 (next)");
    CHECK(liveOf(v.swLo) == 1 && v.swLo[1].st == 1, "LOD table: the next level below is LonLO 7,587.25");
    CHECK(dims(v.swHi) == 4 && dims(v.swLo) == 3 && !v.swHi[1].dim && !v.swLo[1].dim, "every other row is dim");
    hl::markSweeps(v, 7617.0, 3.75); CHECK(v.swHi[1].st == 2, "within a quarter ATR: testing");
    hl::markSweeps(v, 7625.0, 3.75); CHECK(liveOf(v.swHi) == -1 && dims(v.swHi) == 0, "price above every HOD level: no bright row, nothing dimmed");
    hl::markSweeps(v, 7598.0, 3.75); CHECK(liveOf(v.swHi) == 4, "a level price has gone through is never the next one: PFH 7,598.75, then LonHI");
    hl::markSweeps(v, 7560.0, 3.75); CHECK(liveOf(v.swHi) == 1 && v.swHi[3].st == 0, "a level that can no longer be swept (PDH) is skipped: the next is 7,597.50");
    v.firstCalledIn = true; v.side = 2;   // HOD first is in: only the LOD table is watched
    hl::markSweeps(v, 7600.0, 3.75); CHECK(liveOf(v.swHi) == -1 && dims(v.swHi) == 5 && liveOf(v.swLo) == 1, "HOD first in: the HOD table is dim, the LOD table has the next level");
    v.side = 1; hl::markSweeps(v, 7600.0, 3.75); CHECK(liveOf(v.swLo) == -1 && dims(v.swLo) == 4 && liveOf(v.swHi) >= 0, "LOD first in: the mirror");
    hl::markSweeps(v, NAN, 3.75); CHECK(liveOf(v.swHi) == -1 && dims(v.swHi) == 0 && dims(v.swLo) == 0, "no price: nothing marked");
    std::printf("t_hl2250 (lsHodLod %s): %d passed, %d failed\n", hl::HL_VERSION, pass, fail); return fail ? 1 : 0;
}
