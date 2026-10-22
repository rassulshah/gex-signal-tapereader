// test_levelpct_logic.cpp -- LevelPctLogic 1.0.0 (lsDealerProfile 2.19.0, the level % badges, Option B, Rassul 2026-10-21)
// g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_levelpct_logic.cpp -o tlp && ./tlp
#include "LevelPctLogic.h"
#include <cstdio>
#include <cmath>
static int n = 0, f = 0;
#define CHECK(c, m) do { ++n; if (!(c)) { ++f; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, std::string(m).c_str()); } } while (0)
static bool any(const lpct::Hover& h, const std::string& k) { for (const auto& s : h.lines) if (s.find(k) != std::string::npos) return true; return h.title.find(k) != std::string::npos; }
static const char* GOOD =
    "VERSION,1\nMARKET,NQ\nBUILT,2026-10-21\nSOURCE,hodlod_levels 1.2.0\nZONE_ADR,0.25\nMIN_N,20\nDAYS,2025-05-27,2026-10-20\n"
    "# L,name,side,n1,pct1,rnd1,n2,pct2,rnd2,beats\n"
    "L,PDL,LOD,150,10,5,320,38,20,3\n"
    "L,PDH,HOD,140,12,11,300,30,29,0\n"
    "L,PFH,HOD,10,50,40,300,33,20,2\n"          // n1 < 20 -> the first number becomes a dash
    "L,HVL,LOD,12,40,30,15,30,25,1\n"           // both n < 20 -> dropped
    "L,VAH,HOD,90,8,7,200,40,27,1\n"
    "L,1DMin,LOD,30,20,10,60,40,30,0\n";
int main()
{
    lpct::Table T; std::string why;
    CHECK(lpct::parse(GOOD, "NQ", T, why) && T.ok && why.empty(), "good file parses: " + why);
    CHECK(T.rows.size() == 5, "HVL with n < 20 on both is dropped");
    CHECK(T.first == "2025-05-27" && T.last == "2026-10-20" && std::fabs(T.zone - 0.25) < 1e-12, "DAYS / ZONE read");
    std::string used;
    const lpct::Row* r = lpct::lookup(T, "PDL", "LOD", used);
    CHECK(r && used == "PDL" && lpct::text(*r) == "10%  /  38%", "exact format: two spaces each side of the slash");
    CHECK(r && lpct::green(*r) && lpct::outline(*r) == lpct::C_GREEN, "beats random -> green outline");
    r = lpct::lookup(T, "PDH", "HOD", used);
    CHECK(r && !lpct::green(*r) && lpct::outline(*r) == lpct::C_GREY && lpct::text(*r) == "12%  /  30%", "no better than random -> grey");
    CHECK(lpct::lookup(T, "PDL", "HOD", used) == nullptr, "a side with no row: no badge (never the other side's numbers)");
    r = lpct::lookup(T, "PFH/PDH", "HOD", used);
    CHECK(r && used == "PFH" && lpct::text(*r) == "-  /  33%", "merged PFH/PDH: the first part with a number; n1 < 20 = dash");
    CHECK(r && r->beats == 2, "a dashed number cannot carry the green flag (beats 2 kept, 1 masked)");
    r = lpct::lookup(T, "ONH/PDH", "HOD", used);
    CHECK(r && used == "PDH", "merged: ONH has no row, PDH used");
    CHECK(lpct::lookup(T, "HVL", "LOD", used) == nullptr, "dropped row not found");
    // no '?' anywhere on any text
    for (const auto& x : T.rows) { CHECK(lpct::text(x).find('?') == std::string::npos, "no ? in the badge"); }
    // side
    CHECK(lpct::sideOf(100, 105, NAN) == "LOD" && lpct::sideOf(110, 105, 90) == "HOD", "side from the RTH open");
    CHECK(lpct::sideOf(100, NAN, 104) == "LOD" && lpct::sideOf(100, 0, 98) == "HOD", "no open yet: the last price");
    CHECK(lpct::sideOf(105, 105, 1) == "" && lpct::sideOf(100, NAN, NAN) == "", "on the open / nothing known: no side");
    // hover: what, both numbers with n, random, means, how to use, example
    r = lpct::lookup(T, "PDL", "LOD", used);
    lpct::Hover h = lpct::hover("NQ", "PDL", used, "29771.00", *r, T);
    CHECK(any(h, "What:") && any(h, "within 0.25 ADR of PDL") && any(h, "below the RTH open"), "hover: what it is");
    CHECK(any(h, "of 150 days whose FIRST extreme was the LOD") && any(h, "of 320 days with PDL below the open, the day's FINAL LOD"), "hover: both numbers with n");
    CHECK(any(h, "A random price the same distance away: 5%  /  20%."), "hover: the random comparison");
    CHECK(any(h, "Means: GREEN") && any(h, "1st and final LOD") && any(h, "How to use:") && any(h, "Example: \"10%  /  38%\""), "hover: means / use / example");
    r = lpct::lookup(T, "PDH", "HOD", used); h = lpct::hover("NQ", "PDH", used, "30113.00", *r, T);
    CHECK(any(h, "Means: GREY = no better than a random price") && any(h, "above the RTH open") && any(h, "HOD"), "grey hover");
    for (const auto& s : h.lines) CHECK(s.find('?') == std::string::npos, "no ? in hover lines");
    r = lpct::lookup(T, "PFH", "HOD", used); h = lpct::hover("NQ", "PFH \xB7 PDH", used, "30113.00", *r, T);
    CHECK(any(h, "only 10 days") && any(h, "too few (20 needed)"), "dash explained in the hover");
    // wrong / garbled files: no table, a reason
    struct Bad { const char* t; const char* m; const char* k; } bad[] = {
        { "", "NQ", "empty" },
        { "MARKET,NQ\nVERSION,1\n", "NQ", "VERSION" },
        { "VERSION,2\nMARKET,NQ\n", "NQ", "VERSION" },
        { "VERSION,1\nMARKET,ES\nL,PDL,LOD,150,10,5,320,38,20,3\n", "NQ", "MARKET ES is not NQ" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,LOD,150,10,5,320,38,20\n", "NQ", "10 fields" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,LOD,150,101,5,320,38,20,3\n", "NQ", "bad number" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,LOD,150,1e1,5,320,38,20,3\n", "NQ", "bad number" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,LOD,-5,10,5,320,38,20,3\n", "NQ", "bad number" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,MID,150,10,5,320,38,20,3\n", "NQ", "side" },
        { "VERSION,1\nMARKET,NQ\nL,P D L,LOD,150,10,5,320,38,20,3\n", "NQ", "name" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,LOD,150,10,,320,38,20,3\n", "NQ", "random control" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,LOD,150,10,5,320,38,20,7\n", "NQ", "bad number" },
        { "VERSION,1\nMARKET,NQ\nL,PDL,LOD,150,10,5,320,38,20,3", "NQ", "torn" },
        { "VERSION,1\nMARKET,NQ\nZONE_ADR,nan\n", "NQ", "ZONE_ADR" },
    };
    for (const Bad& b : bad) {
        lpct::Table B; std::string w;
        const bool ok = lpct::parse(b.t, b.m, B, w);
        CHECK(!ok && !B.ok && w.find(b.k) != std::string::npos, std::string("garbled refused: ") + b.k + " got '" + w + "'");
        CHECK(lpct::lookup(B, "PDL", "LOD", used) == nullptr, "a refused file shows no badge");
    }
    { std::string big(70000, '#'); big = "VERSION,1\nMARKET,NQ\n" + big + "\n"; lpct::Table B; std::string w; CHECK(!lpct::parse(big, "NQ", B, w) && w.find("too big") != std::string::npos, "oversized refused"); }
    { lpct::Table B; std::string w; CHECK(lpct::parse("VERSION,1\r\nMARKET,NQ\r\nL,PDL,LOD,150,10,5,320,38,20,3\r\n", "NQ", B, w) && B.rows.size() == 1, "CRLF accepted"); }
    std::printf("%d checks, %d failed\n", n, f);
    return f ? 1 : 0;
}
