// t_wymarks.cpp -- WyMarks.h 1.0.0 / 2.0.0 / 2.2.0. Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_wymarks.cpp -o t && ./t
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
    // (2.0.0) the 5-field lines: stamp|letter|dir|level|px; Bk / Sw / Cx; the I first on its candle; bad fields skipped
    { std::map<long long, std::vector<wyi::M> > all;
      const int n = wyi::parseAll("WY|1|ES|180|0.19.0\n100|Sw|1|PDL|29221\n100|I|1|PDL|29221\n200|Bk|-1|PDH|29555\n300|Cx|1||0\n400|I|1\n500|Bk|2|PDH|1\n600|Zz|1|X|1\n700|Bk|1|TOOLONGNAME|5\n", all);
      CHECK(n == 5, "5-field file: 5 marks");
      CHECK(all[100].size() == 2 && all[100][0].code == 'I' && all[100][0].lv == "PDL" && all[100][1].code == 'S', "the I first, then Sw, levels kept");
      CHECK(all[200][0].code == 'B' && all[200][0].dir == -1 && all[200][0].px == 29555, "Bk PDH bearish");
      CHECK(all[300][0].code == 'C' && all[300][0].lv.empty(), "Cx with no level");
      CHECK(all[400][0].code == 'I' && all[400][0].lv.empty(), "an old 3-field I line still read");
      CHECK(!all.count(500) && !all.count(600) && !all.count(700), "bad dir / unknown letter / too-long level skipped");
      std::map<long long, int> im; CHECK(wyi::parse("WY|1|ES|180|0.19.0\n100|Sw|1|PDL|29221\n100|I|1|PDL|29221\n400|I|-1\n", im) == 2 && im[100] == 1 && im[400] == -1, "the 1.0.0 parse: only the I's"); }
    // (2.0.0) the layout: 400 random crowded candles with 1-2 tags each - no tag covers a candle or another tag, all inside the pane
    { unsigned s = 7; auto rnd = [&]() { s = s * 1103515245u + 12345u; return (int)((s >> 8) % 1000); };
      std::vector<wyi::Candle> C; std::vector<wyi::Want> W;
      for (int i = 0; i < 400; ++i) { wyi::Candle c; c.x = 20 + i * 9; int a = 100 + rnd() % 400, b = a + 5 + rnd() % 120; c.yHigh = a; c.yLow = b; c.halfW = 3; c.green = rnd() % 2; C.push_back(c);
        if (rnd() % 3 == 0) { wyi::Want w; w.candle = i; w.w = 12; w.h = 24; W.push_back(w); if (rnd() % 2) { wyi::Want w2 = w; w2.w = 26; W.push_back(w2); } } }
      const std::vector<wyi::Box> P = wyi::layout(C, W, 0, 800, 20 + 400 * 9);
      int bad = 0, drawn = 0; for (size_t k = 0; k < P.size(); ++k) { if (!P[k].ok) continue; ++drawn; if (P[k].t < 1 || P[k].b > 799) ++bad;
          for (const wyi::Candle& c : C) { wyi::Box cb; cb.l = c.x - c.halfW; cb.r = c.x + c.halfW; cb.t = c.yHigh; cb.b = c.yLow; if (wyi::hit(P[k], cb)) ++bad; }
          for (size_t j = 0; j < k; ++j) if (P[j].ok && wyi::hit(P[k], P[j])) ++bad; }
      CHECK(bad == 0 && drawn > (int)W.size() * 8 / 10, "layout: " + std::to_string(drawn) + "/" + std::to_string(W.size()) + " tags placed, no overlaps (" + std::to_string(bad) + ")"); }
    { std::vector<wyi::Candle> C(1); C[0].x = 100; C[0].yHigh = 300; C[0].yLow = 340; C[0].green = true; std::vector<wyi::Want> W(2); W[0].w = 10; W[0].h = 13; W[1].w = 20; W[1].h = 24;
      const auto P = wyi::layout(C, W, 0, 800, 1000); CHECK(P[0].ok && P[0].b < 300 && P[1].ok && P[1].b <= P[0].t, "green candle: the I over it, the level mark further out");
      C[0].green = false; const auto Q = wyi::layout(C, W, 0, 800, 1000); CHECK(Q[0].t > 340 && Q[1].t >= Q[0].b, "red candle: under it");
      const auto R = wyi::layout(C, W, 0, 800, 90); CHECK(!R[0].ok, "nothing right of the last bar"); }
    // (2.1.0) the level lines, the board key, taken boxes
    { std::vector<std::pair<long long, std::string> > L; const int n = wyi::parseLevels("WY|1|ES|180|0.19.0\nL|29221|PFL/PDL\nL|x|bad\nL|29555|PDH\n100|I|1\n", L);
      CHECK(n == 2 && L[0].first == 29221 && L[0].second == "PFL/PDL" && L[1].second == "PDH", "level lines read, bad skipped");
      std::map<long long, std::vector<wyi::M> > all; CHECK(wyi::parseAll("WY|1|ES|180|0.19.0\nL|29221|PFL/PDL\n100|I|1\n", all) == 1, "level lines are not marks");
      CHECK(wyi::civilDays(2026, 11, 19) == 20776 && wyi::civilDays(1970, 1, 1) == 0, "civil days");
      std::vector<wyi::Candle> C(1); C[0].x = 100; C[0].yHigh = 300; C[0].yLow = 340; C[0].green = true; std::vector<wyi::Want> W(1); W[0].w = 10; W[0].h = 13;
      std::vector<wyi::Box> T(1); T[0].l = 86; T[0].r = 114; T[0].b = 300; T[0].t = 268;   // the BO indicator's letter over the candle (32 px)
      const auto P = wyi::layout(C, W, 0, 800, 1000, T); CHECK(P[0].ok && (P[0].b < 268 || P[0].t > 340 || P[0].r < 86 || P[0].l > 114), "a tag never covers another indicator's letter"); }
    {   // (2.2.0) the strength field: 6-field lines parse (Bk / Sw / I), 5- and 3-field lines still parse, an oversized strength is rejected
        const std::string txt = "WY|1|ES|180|0.19.2\n100|Bk|-1|ONL|29373|V99+\n200|Sw|1|PDL|29221|\n300|I|1|PHL|29634|R45\n400|Cx|1|Op|29644\n500|I|1\n600|Bk|1|PDH|29555|V123456\n";
        std::map<long long, std::vector<wyi::M> > m; const int n = wyi::parseAll(txt, m);
        CHECK(n == 5, "6-field lines read (5 good, 1 rejected)");
        CHECK(m[100].size() == 1 && m[100][0].st == "V99+" && m[100][0].lv == "ONL", "Bk strength V99+");
        CHECK(m[200].size() == 1 && m[200][0].st.empty(), "an empty strength = none");
        CHECK(m[300].size() == 1 && m[300][0].code == 'I' && m[300][0].st == "R45", "the I strength");
        CHECK(m[400].size() == 1 && m[400][0].st.empty() && m[400][0].lv == "Op", "5 fields still read");
        CHECK(m[500].size() == 1, "3 fields still read");
        CHECK(m.count(600) == 0, "an oversized strength is rejected"); }
    std::cout << (fail ? "FAILED " : "ALL PASS ") << pass << " pass, " << fail << " fail\n"; return fail ? 1 : 0;
}
