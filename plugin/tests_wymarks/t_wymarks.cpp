// t_wymarks.cpp -- WyMarks.h 1.0.0. Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_wymarks.cpp -o t && ./t
#include "../WyMarks.h"
#include <fstream>
#include <iostream>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::cout << "FAIL " << m << " line " << __LINE__ << "\n"; } } while (0)
int main(int argc, char** argv) {
    std::map<long long, int> m;
    CHECK(wyi::parse("WY|1|ES|180|0.17.5\n100|I|1\n200|E|-1\n300|I|-1\r\n400|A|1\n", m) == 2, "two I");
    CHECK(m.size() == 2 && m[100] == 1 && m[300] == -1, "dirs");
    CHECK(wyi::parse("garbage\n100|I|1\n", m) == -1, "foreign file refused");
    CHECK(wyi::parse("", m) == -1, "empty = not a file");
    CHECK(wyi::parse("WY|1|EU|180|0.17.5\n", m) == 0 && m.empty(), "header only = 0 letters");
    CHECK(wyi::parse("WY|1|ES|180|x\n-5|I|1\nabc|I|1\n100|I|2\n100|I|1x\n100|I\n|I|1\n", m) == 0, "bad lines skipped");
    CHECK(wyi::parse("WY|1|ES|180|x\n100|Sp|1\n100|I|1\n100|Ut|-1", m) == 1 && m[100] == 1, "I among other letters, no final newline");
    // placement: green over, red under, Cx step, pane bounds
    wyi::Tag g = wyi::place(500, 200, 260, true, 6, false, 0, 600);
    CHECK(g.ok && g.over && g.b == 197 && g.t == 184 && g.r - g.l == 12 && g.l == 494, "green: tag over the high");
    wyi::Tag r = wyi::place(500, 200, 260, false, 6, false, 0, 600);
    CHECK(r.ok && !r.over && r.t == 263 && r.b == 276, "red: tag under the low");
    wyi::Tag gc = wyi::place(500, 200, 260, true, 6, true, 0, 600);
    CHECK(gc.b == 181 && gc.b <= 200 - 16 - 3, "Cx on top: I one row further out");
    wyi::Tag rc = wyi::place(500, 200, 260, false, 6, true, 0, 600);
    CHECK(rc.t == 279, "Cx under: I one row further down");
    CHECK(!wyi::place(500, 10, 40, true, 6, false, 0, 600).ok, "off the top: not drawn");
    CHECK(!wyi::place(500, 560, 590, false, 6, false, 0, 600).ok, "off the bottom: not drawn");
    CHECK(wyi::place(500, 200, 260, true, 0, false, 0, 600).r - wyi::place(500, 200, 260, true, 0, false, 0, 600).l == 10, "zero width text still a tag");
    // his real ES letters file of 11-18: 19 I's? count them against a plain scan
    if (argc > 1) { std::ifstream f(argv[1], std::ios::binary); std::stringstream ss; ss << f.rdbuf(); const std::string t = ss.str();
        int scan = 0; std::istringstream in(t); std::string l; while (std::getline(in, l)) if (l.find("|I|") != std::string::npos) ++scan;
        const int n = wyi::parse(t, m); CHECK(n == scan && n > 0, "real ES file: every I line read"); std::cout << "real ES file: " << n << " I\n"; }
    std::cout << (fail ? "FAILED " : "ALL PASS ") << pass << " pass, " << fail << " fail\n"; return fail ? 1 : 0;
}
