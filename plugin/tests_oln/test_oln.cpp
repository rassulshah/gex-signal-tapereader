// test_oln.cpp -- OrderLinesLogic.h 1.0.0 logic tests (lsLiquidityProfile 0.55.0). g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_oln.cpp
#include "../OrderLinesLogic.h"
#include <cstdio>
using namespace oln;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const long long D = 20385LL * 86400000LL;   // a CT day (ms)
static long long T(int h, int m, int s = 0, int ms = 0) { return D + ((h * 60LL + m) * 60LL + s) * 1000LL + ms; }
static Engine mk(const char* mkt = "ES") { Engine E; E.market = mkt; E.R = ruleFor(mkt); return E; }
static Bar bar(long long end, Tick o, Tick h, Tick l, Tick c) { Bar b; b.endMs = end; b.o = o; b.h = h; b.l = l; b.c = c; return b; }
static void bars(Engine& E, int h0, int m0, int n, Tick mid) { for (int k = 0; k < n; ++k) E.closeBar(bar(T(h0, m0) + k * 180000LL, mid, mid + 12, mid - 12, mid)); }   // ATR 24 ticks
static const Line* drawnAt(const Engine& E, Tick px) { for (auto& L : E.lines) if (E.visible(L) && L.px() == px) return &L; return nullptr; }
int main() {
    {   // 1. the 9-min rest: an order drawn only once it rested 9 min, then back to placement; a 4-min order never
        Engine E = mk(); bars(E, 11, 0, 20, 30800);
        E.onEvent(NEW, BID, 30720, 100, 1, T(12, 0)); E.onEvent(NEW, BID, 30710, 100, 2, T(12, 0, 1));
        E.onEvent(DEL, BID, 0, 0, 2, T(12, 4)); E.onEvent(MOD, BID, 30720, 100, 1, T(12, 8, 59));
        CHECK(!drawnAt(E, 30720), "not drawn before 9 min");
        E.onEvent(MOD, BID, 30720, 100, 1, T(12, 9, 1));
        const Line* L = drawnAt(E, 30720); CHECK(L && L->t0 == T(12, 0), "drawn after 9 min, from its placement");
        CHECK(!drawnAt(E, 30710), "a 4-min order (pulled) never drawn");
        CHECK(L && L->label() == "100", "label = size only");
    }
    {   // 2. roll: pulled at 7,675, the same size re-placed 0.45 s BEFORE at 7,680 (the 1:04 PM case) -> one line that steps, no tag
        Engine E = mk(); bars(E, 11, 0, 20, 30800);
        E.onEvent(NEW, BID, 30700, 100, 10, T(11, 41, 25));
        E.onEvent(NEW, BID, 30720, 100, 11, T(13, 2, 16, 49));
        E.onEvent(DEL, BID, 0, 0, 10, T(13, 2, 16, 499));
        E.onEvent(NEW, BID, 30600, 7, 99, T(13, 2, 40));   // time passes beyond the 15 s window
        const Line* L = drawnAt(E, 30720); CHECK(L && L->segs.size() == 2 && L->segs[0].px == 30700 && L->rolls == 1, "rolled = one stepping line");
        CHECK(L && L->label() == "100", "no roll tag");
        CHECK(E.tally.rolls == 1, "one roll counted");
    }
    {   // 3. roll ambiguity: two same-size candidates -> no link (both separate)
        Engine E = mk(); bars(E, 11, 0, 20, 30800);
        E.onEvent(NEW, BID, 30700, 100, 20, T(11, 0));
        E.onEvent(DEL, BID, 0, 0, 20, T(12, 0));
        E.onEvent(NEW, BID, 30705, 100, 21, T(12, 0, 3)); E.onEvent(NEW, BID, 30706, 100, 22, T(12, 0, 4));
        E.onEvent(NEW, BID, 30600, 7, 99, T(12, 0, 30));
        const Line* L = drawnAt(E, 30700); CHECK(L && L->end == PULLED && L->segs.size() == 1, "ambiguous re-place: not joined, the old line ends PULLED");
        CHECK(E.tally.rollAmbig == 1, "ambiguity counted");
    }
    {   // 4. a FILL is never joined: filled at 7,688.75, a 50-lot offer placed 73 s before at 7,695.75 = two lines
        Engine E = mk(); bars(E, 11, 0, 20, 30770);
        E.onEvent(NEW, ASK, 30755, 50, 30, T(13, 47, 40)); E.onEvent(NEW, ASK, 30783, 50, 31, T(14, 11, 29));
        E.onEvent(FILL, BID, 30755, 50, 30, T(14, 12, 42)); E.onEvent(DEL, ASK, 0, 0, 30, T(14, 12, 42));
        E.onEvent(MOD, ASK, 30783, 50, 31, T(14, 21, 0));
        const Line* A = drawnAt(E, 30755); const Line* B = drawnAt(E, 30783);
        CHECK(A && A->end == FILLED && A->segs.size() == 1, "filled order ends FILLED");
        CHECK(B && B != A && B->segs.size() == 1 && B->rolls == 0, "the new order is its own line");
    }
    {   // 5. same id moved = steps (no roll needed); an order that grew 10 -> 50 keeps its earlier prices
        Engine E = mk(); bars(E, 11, 0, 20, 30900);
        E.onEvent(NEW, ASK, 30896, 10, 40, T(11, 26, 17)); E.onEvent(MOD, ASK, 30889, 50, 40, T(11, 27, 3)); E.onEvent(MOD, ASK, 30869, 50, 40, T(11, 32, 36));
        E.onEvent(MOD, ASK, 30869, 50, 40, T(11, 40));
        const Line* L = drawnAt(E, 30869); CHECK(L && L->segs.size() == 3 && L->segs[0].px == 30896, "moved order steps; the steps from before it reached the floor kept");
    }
    {   // 6. late-day cut-off: an ES order placed at 2:30 PM never starts a line
        Engine E = mk(); bars(E, 13, 0, 20, 30800);
        E.onEvent(NEW, BID, 30700, 100, 50, T(14, 30)); E.onEvent(MOD, BID, 30700, 100, 50, T(14, 45));
        CHECK(!drawnAt(E, 30700), "no line after the cut-off");
        Engine G = mk("GC"); bars(G, 11, 0, 20, 30800); G.onEvent(NEW, BID, 30700, 20, 51, T(12, 0)); G.onEvent(MOD, BID, 30700, 20, 51, T(12, 15));
        CHECK(!drawnAt(G, 30700), "GC cut-off 12:00");
    }
    {   // 7. iceberg: shows 30, refilled 15 times, 500 lots -> "I R15 500"
        Engine E = mk(); bars(E, 12, 0, 20, 30780);
        E.onEvent(NEW, BID, 30760, 30, 60, T(12, 42, 45));
        for (int k = 0; k < 16; ++k) { E.onEvent(FILL, ASK, 30760, k < 15 ? 30 : 50, 60, T(13, 0, 20 + k)); if (k < 15) E.onEvent(MOD, BID, 30760, 30, 60, T(13, 0, 20 + k, 500)); }
        E.onEvent(DEL, BID, 0, 0, 60, T(13, 0, 57));
        const Line* L = drawnAt(E, 30760); CHECK(L && L->refills == 15 && L->label() == "I R15 500" && L->end == FILLED, "iceberg label I R15 500");
    }
    {   // 8. ladder: 4 bids of 20-24 lots in one second, 3 ticks apart -> "4o 87"
        Engine E = mk(); bars(E, 13, 0, 20, 30780);
        E.onEvent(NEW, BID, 30719, 21, 70, T(13, 31, 20, 0)); E.onEvent(NEW, BID, 30720, 22, 71, T(13, 31, 20, 200));
        E.onEvent(NEW, BID, 30721, 20, 72, T(13, 31, 20, 400)); E.onEvent(NEW, BID, 30722, 24, 73, T(13, 31, 20, 600));
        E.onEvent(NEW, BID, 30600, 7, 99, T(13, 31, 23)); E.onEvent(NEW, BID, 30600, 7, 98, T(13, 41));
        const Line* L = nullptr; for (auto& x : E.lines) if (E.visible(x) && x.kind == K_LADDER) L = &x;
        CHECK(L && L->label() == "4o 87" && L->px() == 30721, "ladder label 4o 87 at the size-weighted price (30720.54 -> 30721)");
        Engine F = mk(); bars(F, 13, 0, 20, 30780);   // spread too wide: no ladder
        F.onEvent(NEW, BID, 30700, 21, 80, T(13, 31, 20, 0)); F.onEvent(NEW, BID, 30720, 22, 81, T(13, 31, 20, 200)); F.onEvent(NEW, BID, 30740, 24, 82, T(13, 31, 20, 400));
        F.onEvent(NEW, BID, 30600, 7, 99, T(13, 41)); bool any = false; for (auto& x : F.lines) if (x.kind == K_LADDER) any = true;
        CHECK(!any, "a ladder must span <= the market's span");
    }
    {   // 9. HELD: bid filled, the fill bar closes green above it -> HELD; THRU when a bar's HIGH is below the bid
        Engine E = mk(); bars(E, 12, 0, 20, 30800);   // last bar ends 12:57
        E.onEvent(NEW, BID, 30720, 100, 90, T(12, 40)); E.onEvent(NEW, BID, 30760, 100, 91, T(12, 40));
        E.onEvent(MOD, BID, 30720, 100, 90, T(12, 50)); E.onEvent(MOD, BID, 30760, 100, 91, T(12, 50));
        E.onEvent(FILL, ASK, 30760, 100, 91, T(12, 58)); E.onEvent(DEL, BID, 0, 0, 91, T(12, 58));
        E.closeBar(bar(T(13, 0), 30765, 30766, 30740, 30745));   // fill bar: red, high above
        E.closeBar(bar(T(13, 3), 30745, 30755, 30735, 30750));   // high 30755 < 30760 -> THRU
        CHECK(drawnAt(E, 30760) && drawnAt(E, 30760)->word == W_THRU, "a bar wholly below the bid = THRU");
        E.onEvent(FILL, ASK, 30720, 100, 90, T(13, 4)); E.onEvent(DEL, BID, 0, 0, 90, T(13, 4));
        E.closeBar(bar(T(13, 6), 30725, 30726, 30716, 30718));   // red, below
        CHECK(drawnAt(E, 30720)->word == W_NONE, "no favourable close yet");
        E.closeBar(bar(T(13, 9), 30718, 30730, 30717, 30728));   // green, close above 30720 -> HELD
        CHECK(drawnAt(E, 30720)->word == W_HELD && drawnAt(E, 30720)->wordShown, "green close above the bid = HELD");
        Engine G = mk(); bars(G, 12, 0, 20, 30800);
        G.onEvent(NEW, BID, 30720, 100, 92, T(12, 40)); G.onEvent(MOD, BID, 30720, 100, 92, T(12, 50)); G.onEvent(FILL, ASK, 30720, 100, 92, T(12, 58)); G.onEvent(DEL, BID, 0, 0, 92, T(12, 58));
        for (int k = 0; k < 4; ++k) G.closeBar(bar(T(13, 0) + k * 180000LL, 30725, 30730, 30716, 30721 - 0));   // never green
        CHECK(drawnAt(G, 30720)->word == W_THRU, "no favourable close within the fill bar + 3 = THRU");
    }
    {   // 10. one word per zone: two bids filled into the same low, decided at the same close -> the one nearest the low is shown
        Engine E = mk(); bars(E, 12, 0, 20, 30800);
        E.onEvent(NEW, BID, 30729, 100, 100, T(12, 30)); E.onEvent(NEW, BID, 30720, 50, 101, T(12, 30));
        E.onEvent(MOD, BID, 30729, 100, 100, T(12, 45)); E.onEvent(MOD, BID, 30720, 50, 101, T(12, 45));
        E.onEvent(FILL, ASK, 30729, 100, 100, T(12, 58)); E.onEvent(DEL, BID, 0, 0, 100, T(12, 58));
        E.onEvent(FILL, ASK, 30720, 50, 101, T(12, 59)); E.onEvent(DEL, BID, 0, 0, 101, T(12, 59));
        E.closeBar(bar(T(13, 0), 30720, 30740, 30716, 30736));   // green, closes above both
        const Line* A = drawnAt(E, 30729); const Line* B = drawnAt(E, 30720);
        CHECK(A && B && A->word == W_HELD && B->word == W_HELD, "both HELD");
        CHECK(B->wordShown && !A->wordShown, "only the one nearest the low drawn");
    }
    {   // 11. record + restart: drawn lines, steps, ends and words come back identical, never decided again
        Engine E = mk(); bars(E, 11, 0, 20, 30800);
        E.onEvent(NEW, BID, 30700, 100, 10, T(11, 41, 25)); E.onEvent(NEW, BID, 30720, 100, 11, T(13, 2, 16, 49)); E.onEvent(DEL, BID, 0, 0, 10, T(13, 2, 16, 499));
        E.onEvent(NEW, BID, 30600, 7, 99, T(13, 2, 40));
        E.onEvent(FILL, ASK, 30720, 100, 11, T(13, 4, 20)); E.onEvent(DEL, BID, 0, 0, 11, T(13, 4, 20));
        E.closeBar(bar(T(13, 6), 30716, 30737, 30716, 30737));
        std::vector<std::string> rec = E.rows;
        Engine R = mk(); R.restore(rec);
        const Line* A = drawnAt(E, 30720); const Line* B = drawnAt(R, 30720);
        CHECK(A && B && B->segs.size() == A->segs.size() && B->end == FILLED && B->word == A->word && B->wordShown == A->wordShown && B->t0 == A->t0, "restored identically");
        R.closeBar(bar(T(13, 9), 30700, 30701, 30690, 30690));
        CHECK(B->word == W_HELD, "a restored word never changes");
    }
    {   // 12. params file: PARAM lines + plain lines, out of range ignored
        std::vector<std::string> L = { "VERSION,1", "MARKET,ES", "PARAM,FLOOR,60", "LADDER_PIECE_MIN,12", "PARAM,REST_MIN,9999", "garbage" };
        Rule r = parseParams(L, "ES");
        CHECK(r.floorLots == 60 && r.ladPieceMin == 12 && r.restMin == 9, "params parsed, out-of-range kept the default");
        CHECK(ruleFor("NQ").cutMin == 14 * 60 + 30 && ruleFor("CL").cutMin == 13 * 60 && ruleFor("EU").cutMin == 13 * 60 + 30, "cut-offs per market");
    }
    {   // 13. hover rows follow his format: question / answer / how to use / example
        Engine E = mk(); bars(E, 11, 0, 20, 30800);
        E.onEvent(NEW, BID, 30720, 100, 1, T(12, 0)); E.onEvent(MOD, BID, 30720, 100, 1, T(12, 10));
        auto rows = hoverRows(*drawnAt(E, 30720), 0.25, T(12, 10));
        bool q = false, u = false, x = false; for (auto& r : rows) { if (r.text.rfind("Question:", 0) == 0) q = true; if (r.text.rfind("How to use:", 0) == 0) u = true; if (r.text.rfind("Example:", 0) == 0) x = true; }
        CHECK(q && u && x && rows.size() <= 10, "hover: question / how to use / example, <= 10 rows");
    }
    std::printf("OrderLinesLogic %s: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
