// t_wymarks_ar.cpp -- WyMarks.h 2.5.0: the AR badge (Rassul 2026-11-23 09:25-09:34 CT "implement the AR ... automatic rally bar that occurs after a cx",
// "the AR bar should also have a strength line", "go"). The line lsRevRead 0.23.0 writes: stamp|AR|dir||0|strength.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_wymarks_ar.cpp -o t && ./t
#include "../WyMarks.h"
#include "../LevelMarks.h"
#include <iostream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::cout << "FAIL " << m << " line " << __LINE__ << "\n"; } } while (0)
int main() {
    // the line exactly as the pane writes it (RevRead.cpp: stamp | seqText | dir, then "||0|" + st for a Rev and an AR)
    lvm::SMark s; s.code = 'A'; s.dir = 1; s.st = "R51";
    const std::string line = std::to_string(3878264460LL) + "|" + lvm::seqText(s.code) + "|" + std::to_string(s.dir) + "||0|" + s.st;
    CHECK(line == "3878264460|AR|1||0|R51", "the AR line of the file: " << line);
    const std::string txt = "WY|1|NQ|180|0.23.0\n3878263920|E|-1\n3878264100|Cx|1||0|\n" + line + "\n3878264460|A|1\n3878265000|AR|-1||0|V99+\n3878265000|Bk|-1|AHI|103780|R82\n3878265180|AR|1\n"
                            "3878265360|AR|2||0|R12\n3878265360|AR|1||0|R123456\n3878264460|AR|1||0|R60\nL|103780|AHI\n3878266000|FT|1\n";
    std::map<long long, std::vector<wyi::M> > all; const int n = wyi::parseAll(txt, all);
    CHECK(n == 6, "6 marks: Cx, AR, AR + Bk, a 3-field AR, FT (" << n << ")");
    const std::vector<wyi::M>& a = all[3878264460LL];
    CHECK(a.size() == 1 && a[0].code == 'A' && a[0].dir == 1 && a[0].lv.empty() && a[0].px == 0 && a[0].st == "R51", "the AR: green, no level, no level line, strength R51 (the absorbed 'A' letter of the same candle is not a mark; a second AR line is dropped)");
    const std::vector<wyi::M>& b = all[3878265000LL];
    CHECK(b.size() == 2 && b[0].code == 'A' && b[0].dir == -1 && b[0].st == "V99+" && b[1].code == 'B' && b[1].lv == "AHI", "an AR and a Bk on one candle: both marks (2 per candle)");
    CHECK(all[3878265180LL].size() == 1 && all[3878265180LL][0].code == 'A' && all[3878265180LL][0].st.empty(), "a 3-field AR line: the badge without a strength line");
    CHECK(!all.count(3878265360LL), "a bad direction or an over-long strength is dropped");
    CHECK(std::string(wyi::textOf('A')) == "AR" && wyi::codeOf("AR") == 'A' && wyi::codeOf("A") == 0 && wyi::codeOf("E") == 0, "codeOf / textOf: AR is 'A'; the letters A and E stay unknown here");
    CHECK(wyi::sideOf(a[0]) == 0 && !wyi::plainOf(a[0]), "an AR sits by its candle's colour (over a green candle, under a red one) and is a badge, not a plain letter");
    { std::map<long long, int> i; CHECK(wyi::parse(txt, i) == 0, "the I-only reader (lsSDMO) sees no I in it"); }
    // the layout: Cx on the candle before, the AR two candles on; a two-line AR tag over its green candle, clear of every candle and the Cx tag
    std::vector<wyi::Candle> C; for (int k = 0; k < 6; ++k) { wyi::Candle c; c.x = 100 + k * 24; c.halfW = 7; c.yHigh = 300 - k * 4; c.yLow = 340 - k * 4; c.green = k != 1; C.push_back(c); }
    std::vector<wyi::Want> W; { wyi::Want w; w.candle = 0; w.w = 20; w.h = wyi::LINE1_H; W.push_back(w); }                       // the Cx tag
    { wyi::Want w; w.candle = 2; w.w = 24; w.h = wyi::LINE1_H + wyi::LINE2_H; w.side = wyi::sideOf(a[0]); W.push_back(w); }      // the AR tag: code + strength
    const std::vector<wyi::Box> P = wyi::layout(C, W, 0, 600, 100 + 5 * 24 + 6);
    CHECK(P.size() == 2 && P[0].ok && P[1].ok, "both tags placed");
    CHECK(P[1].b <= C[2].yHigh - wyi::GAP && P[1].b - P[1].t == wyi::LINE1_H + wyi::LINE2_H, "the AR tag over its green candle, two lines high");
    bool clear = !wyi::hit(P[0], P[1]); for (const wyi::Candle& c : C) { wyi::Box q; q.l = c.x - c.halfW; q.r = c.x + c.halfW; q.t = c.yHigh; q.b = c.yLow; if (wyi::hit(P[1], q)) clear = false; }
    CHECK(clear, "the AR tag covers no candle and not the Cx tag");
    { std::vector<wyi::Candle> R = C; R[2].green = false; const std::vector<wyi::Box> Q = wyi::layout(R, W, 0, 600, 100 + 5 * 24 + 6); CHECK(Q[1].ok && Q[1].t >= R[2].yLow + wyi::GAP, "a red AR candle: the tag under it"); }
    CHECK(std::string(wyi::VERSION) == "2.6.0", "WyMarks 2.6.0");
    std::cout << (fail ? "FAILED " : "ALL PASS ") << pass << " pass, " << fail << " fail\n"; return fail;
}
