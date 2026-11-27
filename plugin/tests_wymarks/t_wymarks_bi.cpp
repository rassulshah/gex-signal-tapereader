// WyMarks 2.9.0 - the BI badge line (lsRevRead 0.43.0): stamp|BI|dir|level|px|strength
#include "../WyMarks.h"
#include <cstdio>
static int f = 0, p = 0; static void chk(bool c, const char* w) { std::printf("%s %s\n", c ? "PASS" : "FAIL", w); c ? ++p : ++f; }
int main() {
    std::map<long long, std::vector<wyi::M> > m;
    const int n = wyi::parseAll("WY|1|ES|180|0.43.0\n3847000000|BI|1|ONL|28954|R99+\n3847000180|BI|-1|ONH|29000|V25\n3847000360|BI|1|ONL|28954|R99+XX\n", m);
    chk(n == 2, "two good BI lines (a strength longer than 5 is refused)");
    chk(m[3847000000].size() == 1 && m[3847000000][0].code == 'N' && m[3847000000][0].lv == "ONL" && m[3847000000][0].st == "R99+", "BI read: level ONL, strength R99+");
    chk(std::string(wyi::textOf('N')) == "BI", "drawn as BI");
    chk(wyi::sideOf(m[3847000000][0]) == 1 && wyi::sideOf(m[3847000180][0]) == -1, "over a bullish BI, under a bearish one");
    { int a = 0, b = 0; std::sscanf(wyi::VERSION, "%d.%d", &a, &b); chk(a > 2 || (a == 2 && b >= 9), "WyMarks 2.9.0+"); }
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", p, f); return f ? 1 : 0;
}
