// test_liqtouch_grade.cpp -- LiqTouch.h 1.4.0 THE REAL-LIQUIDITY GRADE (R S notP F) + its outcome (HELD / BROKE), on hand-made books
// and on the six REAL ES MBO touches of 22 Oct 2026 (the parity fixtures).
// Build (from plugin/): g++ -std=c++17 -O1 -Wall -Wextra -fsanitize=address,undefined -I. tests_liqtouch/test_liqtouch_grade.cpp -o tltg && ./tltg tests_liqtouch
#include "LiqTouch.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
static int passes = 0, fails = 0;
static void check(bool c, const std::string& m) { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", m.c_str()); } }
using namespace ltc;
static const long long T0 = 1792692000000LL;   // a whole second
// a followed bid level at 996 (300 lots) under last 1000 (ATR 20 ticks -> ES hw 6: zone 990..1002), touched at T0 + 31 s
struct Book {
    Engine E; long long id = 100, t = T0;
    explicit Book(const std::string& m = "ES") : E(defaults(m)) { E.atrOverride.push_back(std::make_pair(0LL, 20.0)); E.begin(T0, 1000); }
    void ev(int a, int sd, long long px, long long sz, long long oid) { E.onEvent(a, sd, px, sz, oid, t); }
    void at(long long ms) { t = ms; E.advance(ms); }
};
static Grade decided(Book& b) { const std::vector<Engine::GRec> r = b.E.drainGrades(); for (const auto& x : r) if (x.kind == 'G') return x.g; for (const auto& g : b.E.gradesDone()) return g; Grade z; return z; }
int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "tests_liqtouch";
    check(zoneHw("ES", 20) == 6 && zoneHw("ES", 5) == 4 && zoneHw("NQ", 100) == 11 && zoneHw("EU", 1) == 2 && zoneHw("GC", 200) == 15, "zone half-widths = the study's medium zones (clamped)");
    check(gradeLetters(G_R | G_NP) == "RsPf", "letters: upper = passed");
    // ---------------------------------------------------------------- 1. REAL: rebuilt, stacked + stayed, not pulled, attack faded
    {   Book b; b.at(T0); b.ev(NEW, BID, 996, 300, 1); b.ev(NEW, ASK, 1001, 50, 2);
        b.t = T0 + 5000; b.ev(NEW, BID, 995, 40, 3);                 // a durable add before the touch (stays)
        b.at(T0 + 31000);                                            // followed (300 >= tier 72 for 30 s)
        b.t = T0 + 31500; b.ev(FILL, BID, 996, 60, 1);               // the touch: sellers hit 60 of the 300
        check(b.E.gradesLive().size() == 1 && b.E.gradesLive()[0].shown == 340 && b.E.gradesLive()[0].lo == 990 && b.E.gradesLive()[0].hi == 1002, "grade starts at the touch: zone 990-1002, shown 340 (both bids)");
        b.t = T0 + 40000; b.ev(FILL, BID, 996, 40, 1);               // more selling in the 1st 90 s (100 total)
        b.t = T0 + 50000; b.ev(NEW, BID, 997, 120, 4);               // the zone rebuilt with size that stays
        b.t = T0 + 140000; b.ev(FILL, BID, 996, 10, 1);              // 2nd 90 s: 10 lots only (faded)
        b.at(T0 + 31500 + 180000 + 1000);
        const Grade g = decided(b);
        check(g.done && g.mask == (G_R | G_S | G_NP | G_F) && g.checks() == 4, "REAL: R S notP F all passed (" + gradeLetters(g.mask) + ")");
        check(g.pulled == 0 && g.v1 == 100 && g.v2 == 10 && g.res >= 0.84 && g.dur >= 0.18, "REAL: pulled 0, attack 100 then 10, rebuilt " + std::to_string(g.res) + ", durable " + std::to_string(g.dur));
        // the outcome: price 1 ATR (20 ticks) above the level -> HELD
        b.t = T0 + 220000; b.ev(NEW, ASK, 1016, 5, 9); b.ev(FILL, ASK, 1016, 5, 9);
        b.at(T0 + 222000); bool held = false; for (const auto& x : b.E.drainGrades()) if (x.kind == 'H') held = x.g.outcome == O_HELD;
        check(held && b.E.gradesLive().empty() && b.E.gradesDone().size() == 1 && b.E.gradesDone()[0].outcome == O_HELD, "REAL: HELD once price is 1 ATR away (final, moved to done)");
    }
    // ---------------------------------------------------------------- 2. FAKE: pulled, churn only, not rebuilt, attack speeding up
    {   Book b; b.at(T0); b.ev(NEW, BID, 996, 300, 1);
        b.at(T0 + 31000); b.t = T0 + 31500; b.ev(FILL, BID, 996, 20, 1);    // touch
        b.t = T0 + 40000; b.ev(DEL, BID, 996, 0, 1);                        // the defender pulls the rest (280 of 300)
        for (int k = 0; k < 30; ++k) { b.t = T0 + 45000 + k * 3000; b.ev(NEW, BID, 995, 50, 50 + k); b.t += 2000; b.ev(DEL, BID, 995, 0, 50 + k); }   // 1,500 lots of churn vs 310 that stayed (the wall + a filled 10) = 0.17   // churn: added and gone in 2 s
        b.t = T0 + 160000; b.ev(NEW, BID, 994, 10, 70); b.ev(FILL, BID, 994, 10, 70);   // 2nd 90 s: attack 10 > 0.5 x 20? -> 10 / 20 = 0.5 = not faded
        b.at(T0 + 31500 + 181000);
        const Grade g = decided(b);
        check(g.done && g.mask == 0, "FAKE: nothing passed (" + gradeLetters(g.mask) + ", pull " + std::to_string(g.pull) + ", dur " + std::to_string(g.dur) + ", exh " + std::to_string(g.exh) + ")");
        check(g.pulled == 280 && g.shown == 300, "FAKE: 280 of 300 pulled");
        check(!g.through, "FAKE: price never went through: not flagged broken");
    }
    // ---------------------------------------------------------------- 3. BROKE: a 3-min block closes below the zone after the decision
    {   Book b; b.at(T0); b.ev(NEW, BID, 996, 300, 1);
        b.at(T0 + 31000); b.t = T0 + 31500; b.ev(FILL, BID, 996, 20, 1);
        b.at(T0 + 213000); decided(b);
        { Book c; c.at(T0); c.ev(NEW, BID, 996, 300, 1); c.at(T0 + 31000); c.t = T0 + 31500; c.ev(FILL, BID, 996, 20, 1);
          c.t = T0 + 100000; c.ev(NEW, BID, 985, 5, 7); c.ev(FILL, BID, 985, 5, 7);   // through the zone before the decision
          c.at(T0 + 213000); const Grade h = decided(c); check(h.done && h.through, "through the zone at the decision: flagged broken"); }
        b.t = T0 + 214000; b.ev(NEW, BID, 980, 5, 8); b.ev(FILL, BID, 980, 5, 8);   // trades at 980 (below 990) and stays there into the next block
        b.at(T0 + 214000 + 200000);
        bool broke = false; for (const auto& x : b.E.drainGrades()) if (x.kind == 'H') broke = x.g.outcome == O_BROKE;
        check(broke, "BROKE: a 3-min block closed below the far edge");
    }
    // ---------------------------------------------------------------- 4. a move inside the zone is not a pull; a part-cut is
    {   Book b; b.at(T0); b.ev(NEW, BID, 996, 300, 1); b.ev(NEW, BID, 993, 100, 2);
        b.at(T0 + 31000); b.t = T0 + 31500; b.ev(FILL, BID, 996, 10, 1);
        b.t = T0 + 40000; b.ev(MOD, BID, 994, 100, 2);    // moved 993 -> 994 (inside 990-1002): not pulled
        b.t = T0 + 41000; b.ev(MOD, BID, 996, 190, 1);    // 290 -> 190: 100 cut = pulled
        b.at(T0 + 31500 + 181000); const Grade g = decided(b);
        check(g.pulled == 100 && g.shown == 400, "move inside the zone not pulled, a 100-lot cut pulled (" + std::to_string(g.pulled) + " / " + std::to_string(g.shown) + ")");
    }
    // ---------------------------------------------------------------- 5. REAL ES MBO, 22 Oct (the six parity touches): every touch graded, numbers sane, deterministic
    {   std::ifstream in((dir + "/cases.csv").c_str()); std::string line; std::getline(in, line);
        std::vector<std::string> head; { std::stringstream ss(line); std::string c; while (std::getline(ss, c, ',')) head.push_back(c); }
        auto col = [&](const char* n) { for (size_t i = 0; i < head.size(); ++i) if (head[i] == n) return i; return (size_t)0; };
        int n = 0;
        while (std::getline(in, line)) {
            std::vector<std::string> f; { std::stringstream ss(line); std::string c; while (std::getline(ss, c, ',')) f.push_back(c); }
            const std::string name = f[col("name")];
            std::string sig[2];
            for (int run = 0; run < 2; ++run) {
                Engine E(defaults("ES"));
                { std::stringstream ss(f[col("atr")]); std::string cp; while (std::getline(ss, cp, ';')) { const size_t k = cp.find(':'); E.atrOverride.push_back(std::make_pair(std::atoll(cp.substr(0, k).c_str()), std::atof(cp.substr(k + 1).c_str()))); } }
                E.begin(std::atoll(f[col("W0")].c_str()), std::atoll(f[col("last")].c_str()));
                std::ifstream b((dir + "/" + name + ".bin").c_str(), std::ios::binary); char rec[34]; long long lastMs = 0;
                while (b.read(rec, 34)) { long long ms, p, s, id; signed char a, sd; std::memcpy(&ms, rec, 8); a = rec[8]; sd = rec[9]; std::memcpy(&p, rec + 10, 8); std::memcpy(&s, rec + 18, 8); std::memcpy(&id, rec + 26, 8); E.onEvent(a, sd, p, s, id, ms); lastMs = ms; }
                E.advance(lastMs + 2000);
                const long long px = std::atoll(f[col("px")].c_str()); const int side = std::atoi(f[col("side")].c_str());
                const Grade* G = nullptr; std::vector<Grade> all = E.gradesDone(); for (const Grade& g : E.gradesLive()) all.push_back(g);
                for (const Grade& g : all) if (g.px == px && g.side == side && g.done) { G = &g; break; }
                if (!G) { check(false, name + ": graded"); break; }
                char bb[300]; std::snprintf(bb, sizeof bb, "%s mask %s (%d/4) shown %lld pulled %lld res %.2f added %lld quick %lld dur %.2f attack %lld -> %lld exh %.2f outcome %s (study hold %s)",
                    name.c_str(), gradeLetters(G->mask).c_str(), G->checks(), G->shown, G->pulled, G->res, G->added, G->quick, G->dur, G->v1, G->v2, G->exh, outName(G->outcome), f[col("hold")].c_str());
                sig[run] = bb;
                if (run == 0) { std::printf("  %s\n", bb); ++n;
                    check(G->shown > 0 && G->res >= 0 && G->pull >= 0 && G->pull <= 1.0 + 1e-9 && G->added >= G->quick, name + ": finite, consistent numbers"); }
            }
            check(!sig[0].empty() && sig[0] == sig[1], name + ": the same replay gives the same grade (no repaint)");
        }
        check(n == 6, "six real touches graded (" + std::to_string(n) + ")");
    }
    std::printf("LiqTouch %s grade: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
