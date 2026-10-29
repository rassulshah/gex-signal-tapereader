// test_oln_110.cpp -- OrderLinesLogic.h 1.1.0 (lsLiquidityProfile 0.68.0, Rassul approved 2026-10-29 09:33 / 10:23 CT): the TESTED-LINE
// lifecycle - untouched = nothing; touch -> dashed TEST from the placement; frozen rank (0 outside the top 6); HELD solid, min 2 bars,
// gone at a close through; BREAK 5 bars; pulled before a decision = gone; max 3; restart from the record = the same states (no repaint).
// g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_oln_110.cpp
#include "../OrderLinesLogic.h"
#include <cstdio>
using namespace oln;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const long long D = 20385LL * 86400000LL;
static long long T(int h, int m, int s = 0) { return D + ((h * 60LL + m) * 60LL + s) * 1000LL; }
static const long long BM = 180000;
static Engine mk() { Engine E; E.market = "ES"; E.R = ruleFor("ES"); return E; }
static void bar(Engine& E, long long end, Tick o, Tick h, Tick l, Tick c) { Bar b; b.endMs = end; b.o = o; b.h = h; b.l = l; b.c = c; E.closeBar(b); }
static int vis(const Engine& E, int li, long long last) { return visState(E.lines[(size_t)li], last, BM); }
static std::vector<std::string> recs(const Engine& E) { std::vector<std::string> r; r.push_back(Engine::header()); for (auto& x : E.rows) r.push_back(x); return r; }

int main() {
    int rank = 2;
    {   // HELD: untouched -> TEST at the touch -> HELD (solid) -> close through after 1 bar -> still shown (min 2 bars) -> gone
        Engine E = mk(); E.rankFn = [&](int, Tick) { return rank; };
        bar(E, T(8, 57), 1010, 1014, 1006, 1012);
        E.onEvent(NEW, BID, 1000, 100, 11, T(8, 58));
        bar(E, T(9, 0), 1012, 1016, 1008, 1010); bar(E, T(9, 3), 1010, 1015, 1004, 1008);
        CHECK(E.lines.size() == 1 && vis(E, 0, T(9, 3)) == V_NONE, "untouched big order: nothing on the candles");
        E.onEvent(FILL, BID, 1001, 5, 99, T(9, 4));   // a trade above it: not a touch
        CHECK(E.lines[0].touchMs == 0, "a trade above a bid is not a touch");
        E.onEvent(FILL, BID, 1000, 30, 11, T(9, 4, 30));
        CHECK(E.lines[0].touchMs == T(9, 4, 30) && E.lines[0].touchRank == 2 && E.lines[0].drawn, "touch at the first trade at its price, rank 2 recorded");
        CHECK(vis(E, 0, T(9, 3)) == V_TEST && E.lines[0].t0 == T(8, 58), "TEST (dashed) from the PLACEMENT time");
        rank = 5; E.onEvent(FILL, BID, 1000, 70, 11, T(9, 5)); E.onEvent(DEL, BID, 1000, 0, 11, T(9, 5, 1));
        CHECK(E.lines[0].touchRank == 2, "rank frozen at the touch");
        bar(E, T(9, 6), 1004, 1008, 999, 1006);   // fill bar: green close above -> HELD
        CHECK(E.lines[0].word == W_HELD && E.lines[0].wordMs == T(9, 6) && vis(E, 0, T(9, 6)) == V_HELD, "HELD decided at the 9:06 close");
        bar(E, T(9, 9), 1006, 1007, 990, 995);    // closes through 1 bar later
        CHECK(E.lines[0].throughMs == T(9, 9) && vis(E, 0, T(9, 9)) == V_HELD, "closed through after 1 bar: still shown (min 2 bars)");
        bar(E, T(9, 12), 995, 1000, 990, 996);
        CHECK(vis(E, 0, T(9, 12)) == V_NONE, "removed at the 2nd bar after the decision");
        bool hasT = false, hasX = false, hasW = false; for (auto& r : E.rows) { if (r == "T|1|" + std::to_string(T(9, 4, 30)) + "|2|100") hasT = true; if (r.rfind("X|1|", 0) == 0) hasX = true; if (r.find("|HELD") != std::string::npos && r[0] == 'W') hasW = true; }
        CHECK(hasT && hasX && hasW, "record: T (touch + rank + displayed lots), W, X");
        CHECK(labelText(E.lines[0]) == "HELD 100", "label text: the word + the displayed size at the touch (\"[2] HELD 100\")");
        Engine R = mk(); R.restore(recs(E));
        bool same = true; for (long long t = T(9, 0); t <= T(9, 15); t += BM) if (visState(R.lines[0], t, BM) != vis(E, 0, t)) same = false;
        CHECK(same && R.lines[0].touchRank == 2 && R.lines[0].touchLots == 100, "restart: the same states from the record (no repaint)");
    }
    {   // HELD that is NOT closed through stays (solid, never dimmed) to the right edge; a later close through removes it at that close
        Engine E = mk(); rank = 9; E.rankFn = [&](int, Tick) { return rank; };
        bar(E, T(10, 0), 1110, 1112, 1104, 1106);
        E.onEvent(NEW, ASK, 1100, 80, 21, T(10, 1));
        E.onEvent(FILL, ASK, 1100, 80, 21, T(10, 2)); E.onEvent(DEL, ASK, 1100, 0, 21, T(10, 2, 1));
        CHECK(E.lines[0].touchRank == 0, "rank 9 (outside the top 6) -> no rank box (0)");
        bar(E, T(10, 3), 1098, 1101, 1092, 1094);   // red close below the offer -> HELD
        CHECK(E.lines[0].word == W_HELD, "offer HELD");
        for (int k = 1; k <= 8; ++k) bar(E, T(10, 3) + k * BM, 1094, 1099, 1090, 1095);
        CHECK(vis(E, 0, T(10, 3) + 8 * BM) == V_HELD, "HELD stays (8 bars, no close through, never dimmed)");
        bar(E, T(10, 3) + 9 * BM, 1095, 1104, 1094, 1102);
        CHECK(vis(E, 0, T(10, 3) + 9 * BM) == V_NONE, "a close through 9 bars later removes it at that close");
    }
    {   // BREAK: 5 bars, then gone; drawn word BREAK, record keeps THRU + BREAK
        Engine E = mk(); E.rankFn = [&](int, Tick) { return 1; };
        bar(E, T(11, 0), 1010, 1012, 1004, 1006);
        E.onEvent(NEW, BID, 1000, 60, 31, T(11, 1));
        E.onEvent(FILL, BID, 1000, 60, 31, T(11, 2)); E.onEvent(DEL, BID, 1000, 0, 31, T(11, 2, 1));
        bar(E, T(11, 3), 1000, 1001, 990, 992); bar(E, T(11, 6), 992, 998, 985, 990);   // high below the bid -> BREAK at 11:06
        CHECK(E.lines[0].word == W_THRU && std::string(wordText(E.lines[0].word)) == "BREAK" && labelText(E.lines[0]) == "BREAK 60", "BREAK drawn as \"BREAK 60\"");
        CHECK(vis(E, 0, T(11, 6) + 4 * BM) == V_BREAK && vis(E, 0, T(11, 6) + 5 * BM) == V_NONE, "BREAK shown 5 bars after the decision, then removed");
        bool rec = false; for (auto& r : E.rows) if (r.rfind("W|", 0) == 0 && r.find("|THRU|") != std::string::npos && r.find("|BREAK") != std::string::npos) rec = true;
        CHECK(rec, "record keeps the old code THRU and adds BREAK");
    }
    {   // pulled before a decision: gone; touched by a closed bar's range (fallback)
        Engine E = mk(); E.rankFn = [&](int, Tick) { return 3; };
        bar(E, T(12, 0), 1010, 1012, 1004, 1006);
        E.onEvent(NEW, BID, 1000, 100, 41, T(12, 1));
        bar(E, T(12, 3), 1006, 1008, 1000, 1004);
        CHECK(E.lines[0].touchMs == T(12, 3) && vis(E, 0, T(12, 3)) == V_TEST, "touched by the closed bar's low (fallback), stamped at the bar end");
        E.onEvent(DEL, BID, 1000, 0, 41, T(12, 4)); E.onEvent(NEW, ASK, 2000, 1, 42, T(12, 4, 30));   // the roll window passes
        CHECK(E.lines[0].end == PULLED && vis(E, 0, T(12, 6)) == V_NONE, "pulled before a decision = it disappears");
    }
    {   // at most 3 tested lines: the newest
        Engine E = mk(); E.rankFn = [&](int, Tick) { return 1; };
        bar(E, T(13, 0), 1050, 1052, 1044, 1046);
        for (int k = 0; k < 5; ++k) E.onEvent(NEW, BID, 1000 + k, 100, 50 + k, T(13, 1));
        for (int k = 0; k < 5; ++k) E.onEvent(FILL, BID, 1004 - k, 1, 99, T(13, 2, k * 5));   // touches 1004 first ... 1000 last
        const std::vector<int> sh = shownLines(E, T(13, 0), BM);
        for (int i : sh) std::printf("  shown %lld touch %lld\n", (long long)E.lines[(size_t)i].px(), E.lines[(size_t)i].touchMs - T(13,0));
        CHECK(sh.size() == 3 && E.lines[(size_t)sh[0]].px() == 1000 && E.lines[(size_t)sh[2]].px() == 1002, "max 3 tested lines, the newest touches");
    }
    std::printf("OrderLinesLogic %s lifecycle: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
