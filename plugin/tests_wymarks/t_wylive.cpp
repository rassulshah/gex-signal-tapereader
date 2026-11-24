// t_wylive.cpp -- WyMarks 2.6.0 parseLive (RevRead\wylive-<MKT>-<spb>.txt, lsRevRead 0.25.0): the live Cx? line.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_wylive.cpp -o t && ./t
#include "../WyMarks.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m); } }
int main() {
    long long t = 0; int d = 0;
    C(wyi::parseLive("WYL|1|HG|180|0.25.0\n3878319780|Cx?|1\n", "HG", t, d) && t == 3878319780LL && d == 1, "one line: stamp + green");
    C(wyi::parseLive("WYL|1|HG|180|0.25.0\r\n3878319780|Cx?|-1\r\n", "HG", t, d) && d == -1, "CRLF, red");
    C(!wyi::parseLive("WYL|1|HG|180|0.25.0\n", "HG", t, d) && t == 0, "no line = none");
    C(!wyi::parseLive("WYL|1|GC|180|0.25.0\n3878319780|Cx?|1\n", "HG", t, d), "another market = none");
    C(!wyi::parseLive("WY|1|HG|180|0.25.0\n3878319780|Cx?|1\n", "HG", t, d), "the wy file is not the live file");
    C(!wyi::parseLive("WYL|1|HG|180|0.25.0\n3878319780|Cx|1\n", "HG", t, d), "a decided Cx line is not a live one");
    C(!wyi::parseLive("WYL|1|HG|180|0.25.0\nabc|Cx?|1\n", "HG", t, d), "garbled stamp");
    C(!wyi::parseLive("WYL|1|HG|180|0.25.0\n3878319780|Cx?|2\n", "HG", t, d), "garbled side");
    C(!wyi::parseLive("", "HG", t, d) && !wyi::parseLive("WYL|1|HG|180\n1|Cx?|1\n", "", t, d), "empty / no market");
    std::map<long long, std::vector<wyi::M> > m; C(wyi::parseAll("WY|1|HG|180|x\n3878319780|Cx?|1\n", m) == 0 && m.empty(), "the wy readers never see a Cx? (unknown code)");
    std::printf("%s %d pass, %d fail\n", f ? "FAIL" : "ALL PASS", ok, f); return f ? 1 : 0;
}
