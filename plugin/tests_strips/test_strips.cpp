// tests_strips/test_strips.cpp - LetterStrips.h 1.0.0 + ReversalStageLogic.h 1.0.0 (2026-10-22). g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined
#include "../LetterStrips.h"
#include "../ReversalStageLogic.h"
#include <cstdio>
#include <map>
static int fails = 0, n = 0;
#define CHECK(c) do { ++n; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
using lst::Lt;
static std::vector<Lt> L(int pane, std::initializer_list<const char*> codes, int pivot = lst::LOW, int book = -1) { std::vector<Lt> v; for (const char* c : codes) v.push_back(lst::make(pane, c, pivot, book)); return v; }
static std::string shown(int pane, const std::vector<Lt>& v, int s) { const lst::Pick p = lst::pick(pane, v); return p.shown[s] < 0 ? std::string("-") : v[(size_t)p.shown[s]].code; }

int main() {
    // ---- strip assignment + priority
    CHECK(shown(lst::ATK, L(lst::ATK, {"L", "K", "S"}), 0) == "S");
    CHECK(shown(lst::ATK, L(lst::ATK, {"L", "K"}), 0) == "K");
    CHECK(shown(lst::ATK, L(lst::ATK, {"G", "Tr", "R"}), 1) == "R");
    CHECK(shown(lst::ATK, L(lst::ATK, {"G", "Tr"}), 1) == "Tr");
    CHECK(shown(lst::ATK, L(lst::ATK, {"G"}), 1) == "G");
    CHECK(shown(lst::ATK, L(lst::ATK, {"G"}), 0) == "-");
    CHECK(shown(lst::DEF, L(lst::DEF, {"B", "P", "W", "A"}), 0) == "A");
    CHECK(shown(lst::DEF, L(lst::DEF, {"B", "P", "W"}), 0) == "W");
    CHECK(shown(lst::DEF, L(lst::DEF, {"B", "P"}), 0) == "P");
    CHECK(shown(lst::DEF, L(lst::DEF, {"X", "L", "H"}), 1) == "H");
    CHECK(shown(lst::DEF, L(lst::DEF, {"X", "L"}), 1) == "L");
    CHECK(shown(lst::DEF, L(lst::DEF, {"X"}), 1) == "X");
    { std::vector<Lt> v = L(lst::DEF, {"W", "A"}); v.push_back(lst::make(lst::DEF, "Q", lst::LOW, -1)); CHECK(shown(lst::DEF, v, 0) == "A");
      std::vector<Lt> w = L(lst::DEF, {"W", "P"}); w.push_back(lst::make(lst::DEF, "Q", lst::LOW, -1)); CHECK(shown(lst::DEF, w, 0) == "Q"); }
    { const std::vector<Lt> v = L(lst::DEF, {"P", "H", "A", "B", "X"}); const lst::Pick p = lst::pick(lst::DEF, v);   // the others in the hover, strongest first
      CHECK(p.order[0].size() == 3 && v[(size_t)p.order[0][1]].code == "P" && v[(size_t)p.order[0][2]].code == "B");
      CHECK(p.order[1].size() == 2 && v[(size_t)p.order[1][1]].code == "X"); }
    CHECK(lst::stripOf(lst::ATK, "A") < 0);              // the shared A is not on the attacker pane
    CHECK(lst::stripOf(lst::DEF, "S") < 0 && lst::stripOf(lst::DEF, "Tr") < 0 && lst::stripOf(lst::DEF, "G") < 0);
    // ---- colour: the side that acts
    CHECK(lst::make(lst::ATK, "K", lst::HIGH, -1).colour == lst::GREEN);   // buyers attacking a high
    CHECK(lst::make(lst::ATK, "S", lst::LOW, -1).colour == lst::RED);      // sellers sweeping into a low
    CHECK(lst::make(lst::ATK, "G", lst::LOW, -1).colour == lst::RED && lst::make(lst::ATK, "Tr", lst::HIGH, -1).colour == lst::GREEN);
    CHECK(lst::make(lst::ATK, "R", lst::HIGH, -1).colour == lst::GREEN && lst::make(lst::ATK, "R", lst::LOW, -1).colour == lst::RED);
    CHECK(lst::make(lst::ATK, "L", lst::LOW, 1).colour == lst::GREEN && lst::make(lst::ATK, "L", lst::HIGH, 0).colour == lst::RED);   // L by the order's side
    CHECK(lst::make(lst::DEF, "A", lst::LOW, -1).colour == lst::GREEN && lst::make(lst::DEF, "A", lst::HIGH, -1).colour == lst::RED);
    CHECK(lst::make(lst::DEF, "H", lst::HIGH, -1).colour == lst::RED && lst::make(lst::DEF, "L", lst::LOW, -1).colour == lst::GREEN);
    CHECK(lst::make(lst::DEF, "X", lst::HIGH, -1).colour == lst::RED && lst::make(lst::DEF, "X", lst::LOW, -1).colour == lst::GREEN);   // warning: the side it hurts
    CHECK(lst::make(lst::DEF, "W", lst::LOW, -1).colour == lst::GREEN);
    CHECK(lst::make(lst::DEF, "B", lst::HIGH, 1).colour == lst::RED && lst::make(lst::DEF, "P", lst::HIGH, 0).colour == lst::GREEN);  // the book side that moved
    // ---- placement: 11 px candles, letters 7-12 px wide, never touching
    { std::vector<lst::Item> it; for (int k = 0; k < 40; ++k) { lst::Item x; x.xc = 100 + 11 * k; x.w = (k % 3 == 0) ? 12 : 7; it.push_back(x); }
      const std::vector<lst::Placed> p = lst::placeStrip(it, 80, 1000); int drawn = 0, lower = 0;
      for (size_t a = 0; a < p.size(); ++a) { drawn += p[a].drawn; lower += p[a].drawn && p[a].sub == 1; for (size_t b = a + 1; b < p.size(); ++b) CHECK(!lst::touches(p[a], p[b])); }
      CHECK(drawn >= 26); CHECK(lower == 0);   // (LetterStrips 1.1.0) ONE row per strip: the tight ones shift or are skipped, never touching
      CHECK(lst::ROW_H == 11 && lst::SUB_ROWS == 1 && lst::SUB_H == 11);
      for (size_t a = 0; a < p.size(); ++a) if (p[a].drawn) CHECK(std::abs((p[a].l + p[a].r) / 2 - it[a].xc) <= 4); }
    { std::vector<lst::Item> it; for (int k = 0; k < 30; ++k) { lst::Item x; x.xc = 100 + 4 * k; x.w = 12; it.push_back(x); }   // zoomed out: some are skipped, never touching
      const std::vector<lst::Placed> p = lst::placeStrip(it, 80, 1000); int drawn = 0;
      for (size_t a = 0; a < p.size(); ++a) { drawn += p[a].drawn; for (size_t b = a + 1; b < p.size(); ++b) CHECK(!lst::touches(p[a], p[b])); }
      CHECK(drawn > 0 && drawn < 30); }
    { std::vector<lst::Item> it(1); it[0].xc = 78; it[0].w = 10; CHECK(!lst::placeStrip(it, 80, 1000)[0].drawn);   // never over the label column
      it[0].xc = 997; CHECK(!lst::placeStrip(it, 80, 1000)[0].drawn); it[0].xc = 500; CHECK(lst::placeStrip(it, 80, 1000)[0].drawn); }
    { std::vector<lst::Item> it(2); it[0].xc = 200; it[0].w = 12; it[1].xc = 250; it[1].w = 12;   // far apart: both upper, centred
      const std::vector<lst::Placed> p = lst::placeStrip(it, 80, 1000); CHECK(p[0].sub == 0 && p[1].sub == 0 && p[0].l == 194 && p[1].l == 244); }
    // ---- the record
    { lst::Lt x = lst::make(lst::DEF, "P", lst::HIGH, 1, "120 lots | 2t"); const std::string s = lst::line(lst::DEF, 1792600000, x);
      lst::Rec r; CHECK(lst::parse(s, r)); CHECK(r.pane == lst::DEF && r.key == 1792600000 && r.l.code == "P" && r.l.pivot == lst::HIGH && r.l.book == 1 && r.l.nums == "120 lots  2t" && r.l.colour == lst::RED);
      CHECK(lst::parse(s + "\r", r));
      CHECK(!lst::parse("LT|1|D|1792600000|S|0|-1|", r));         // S is not a Defense letter
      CHECK(!lst::parse("LT|1|A|1792600000|A|0|-1|", r));         // A is not drawn on the attacker pane
      CHECK(!lst::parse("LT|1|Q|1792600000|A|0|-1|", r) && lst::parse("LT|1|Q|1792600000|Q|0|-1|q 0.4s", r) && r.pane == lst::QUE);
      CHECK(!lst::parse("LT|1|D|-5|A|0|-1|", r) && !lst::parse("LT|1|D|17x|A|0|-1|", r) && !lst::parse("LT|1|D|1|A|2|-1|", r) && !lst::parse("LT|1|D|1|A|0|3|", r));
      CHECK(!lst::parse("LT|2|D|1|A|0|-1|", r) && !lst::parse("LT|1|D|1|A|0|-1", r) && !lst::parse("LT|1|D|1|A|0|-1|x|y", r) && !lst::parse("", r)); }
    CHECK(lst::fileName(lst::ATK, "6E", 180) == "ATK-EU-180.csv" && lst::fileName(lst::QUE, "ES", 60) == "QUEUE-ES-60.csv");
    CHECK(lst::marketName("ESZ6") == "ES" && lst::marketName("@NQ") == "NQ" && lst::marketName("6EZ6") == "EU" && lst::marketName("QM") == "CL");

    // ================= the stage state machine =================
    using rsl::Bar; using rsl::Ev;
    // flat bars around 1000 (range 4 ticks) then a drop to a 5-candle low at index 10
    auto bars = [](const std::vector<std::array<long long, 3> >& hlc) { std::vector<Bar> b; long long key = 1000; for (auto& x : hlc) { Bar y; y.key = key; key += 180; y.h = x[0]; y.l = x[1]; y.c = x[2]; y.ok = true; b.push_back(y); } return b; };
    std::vector<std::array<long long, 3> > base; for (int k = 0; k < 10; ++k) base.push_back({{1004, 1000, 1002}});
    auto ev = [](std::multimap<long long, Ev>& m, long long key, int pane, const char* c, int pivot, int book = -1, const char* nums = "") { Ev e; e.key = key; e.pane = pane; e.l = lst::make(pane == lst::QUE ? lst::DEF : pane, c, pivot, book, nums); m.insert(std::make_pair(key, e)); };
    auto keyOf = [](int i) { return 1000LL + 180LL * i; };
    {   // forming: K (1) . A (2) . Tr (3) . H (4) . L (5), price stays near the low (no expansion)
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1001, 990, 992}}); h.push_back({{995, 991, 993}}); h.push_back({{995, 991, 993}}); h.push_back({{995, 992, 993}}); h.push_back({{995, 992, 993}}); h.push_back({{995, 992, 993}});
        std::multimap<long long, Ev> E; ev(E, keyOf(10), lst::ATK, "K", lst::LOW, -1, "450 lots"); ev(E, keyOf(10), lst::DEF, "A", lst::LOW, -1, "1,940 lots 2.6x");
        ev(E, keyOf(11), lst::ATK, "Tr", lst::LOW); ev(E, keyOf(12), lst::DEF, "H", lst::LOW); ev(E, keyOf(13), lst::DEF, "W", lst::LOW); ev(E, keyOf(14), lst::DEF, "L", lst::LOW);
        const std::vector<rsl::Step> s = rsl::run(bars(h), E);
        CHECK(s[9].side == -1);
        CHECK(s[10].side == rsl::LOW && s[10].stage == 2 && s[10].status == rsl::FORMING && s[10].chain == "K A");
        CHECK(s[11].stage == 3 && s[12].stage == 4 && s[13].stage == 4 && s[14].stage == 5 && s[15].stage == 5);
        CHECK(s[14].chain == "K A . Tr . H . W* . L"); CHECK(s[13].here.find("hover only") != std::string::npos);
        CHECK(s[10].here.find("1,940 lots 2.6x") != std::string::npos);
        // prefix runs identical (no repaint as candles arrive)
        for (size_t m = 1; m <= h.size(); ++m) { std::vector<std::array<long long, 3> > hp(h.begin(), h.begin() + (long)m); const std::vector<rsl::Step> sp = rsl::run(bars(hp), E); for (size_t k = 0; k < m; ++k) CHECK(sp[k] == s[k]); }
        // record round trip
        for (size_t k = 0; k < s.size(); ++k) { rsl::Step r; CHECK(rsl::parse(rsl::line(s[k]), r)); CHECK(r == s[k]); }
        const std::vector<std::string> nw = rsl::nowRows(s[12], "12:42 PM"); CHECK(nw[0] == "Now 12:42 PM" && nw[1] == "LOW attempt" && nw[2] == "Stage 4 Turn" && nw[3] == "Chain K A . Tr . H" && nw[4].find("Odds ?") == 0);
        CHECK(rsl::lookOf(s[12]).height == 4 && rsl::lookOf(s[12]).colour == lst::GREEN && !rsl::lookOf(s[12]).dim);
    }
    {   // (ReversalStage 1.2.0) a queue Q steps only at the attempt's own extreme (tk anchor within max(4, 0.3 ATR) ticks)
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1001, 990, 992}}); h.push_back({{995, 991, 993}});
        std::multimap<long long, Ev> E; ev(E, keyOf(10), lst::ATK, "K", lst::LOW); ev(E, keyOf(11), lst::QUE, "Q", lst::LOW, -1, "held 120% orig 97% q0 130 tk991");
        std::vector<rsl::Step> s = rsl::run(bars(h), E);
        CHECK(s[11].stage == 2 && s[11].chain == "K . Q"); CHECK(s[11].here.find("tk") == std::string::npos);
        std::multimap<long long, Ev> F; ev(F, keyOf(10), lst::ATK, "K", lst::LOW); ev(F, keyOf(11), lst::QUE, "Q", lst::LOW, -1, "held 120% orig 97% q0 130 tk1040");
        s = rsl::run(bars(h), F); CHECK(s[11].stage == 1 && s[11].chain == "K . Q*");          // a Q far from the extreme: hover only
        std::multimap<long long, Ev> G; ev(G, keyOf(10), lst::ATK, "K", lst::LOW); ev(G, keyOf(11), lst::QUE, "Q", lst::LOW, -1, "held 120% orig 97% q0 130");
        s = rsl::run(bars(h), G); CHECK(s[11].stage == 1);                                         // no anchor (older record): hover only
        CHECK(rsl::qAnchorOf("broke 4% q0 24 tk29115") == 29115 && rsl::qAnchorOf("q0 24") == 0 && rsl::qAnchorOf("x tk12a") == 0 && rsl::qTolTicks(40) == 12 && rsl::qTolTicks(2) == 4);
    }
    {   // complete: the close leaves the low by >= 1 ATR (ATR ~ 4-11 ticks here)
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1001, 990, 992}}); h.push_back({{1003, 991, 1002}});
        std::multimap<long long, Ev> E; ev(E, keyOf(10), lst::ATK, "S", lst::LOW); ev(E, keyOf(10), lst::DEF, "A", lst::LOW);
        const std::vector<rsl::Step> s = rsl::run(bars(h), E);
        CHECK(s[11].stage == 6 && s[11].status == rsl::COMPLETE && s[11].chain == "S A . 1 ATR");
        CHECK(rsl::lookOf(s[11]).height == 6);
    }
    {   // failed: X (re-test empty) after A; the step drops back (stub)
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1001, 990, 992}}); h.push_back({{995, 991, 994}}); h.push_back({{995, 991, 993}});
        std::multimap<long long, Ev> E; ev(E, keyOf(10), lst::ATK, "K", lst::LOW); ev(E, keyOf(10), lst::DEF, "A", lst::LOW); ev(E, keyOf(11), lst::DEF, "H", lst::LOW); ev(E, keyOf(12), lst::DEF, "X", lst::LOW);
        const std::vector<rsl::Step> s = rsl::run(bars(h), E);
        CHECK(s[11].stage == 4 && s[12].status == rsl::FAILED && s[12].stage == 0 && s[12].failedStage == 4 && rsl::lookOf(s[12]).stub);
        CHECK(rsl::nowRows(s[12], "x")[2] == "Stage 4, failed");
        // the order gate (replay ES 2026-10-22): an older turn's re-test L / X on a fresh extreme is hover only; H before an A too
        std::multimap<long long, Ev> G; ev(G, keyOf(10), lst::DEF, "L", lst::LOW); ev(G, keyOf(11), lst::DEF, "X", lst::LOW); ev(G, keyOf(12), lst::DEF, "H", lst::LOW);
        const std::vector<rsl::Step> g = rsl::run(bars(h), G);
        CHECK(g[10].stage == 0 && g[11].status != rsl::FAILED && g[12].stage == 0 && g[11].chain == "L* . X*");
        std::multimap<long long, Ev> G2; ev(G2, keyOf(10), lst::DEF, "A", lst::LOW); ev(G2, keyOf(10), lst::DEF, "H", lst::LOW); ev(G2, keyOf(10), lst::DEF, "L", lst::LOW);
        CHECK(rsl::run(bars(h), G2)[10].stage == 5);   // one candle: A then H then L in step order
        std::multimap<long long, Ev> G3; ev(G3, keyOf(10), lst::DEF, "A", lst::LOW); ev(G3, keyOf(10), lst::DEF, "H", lst::LOW); ev(G3, keyOf(10), lst::DEF, "X", lst::LOW);
        { const rsl::Step x = rsl::run(bars(h), G3)[10]; CHECK(x.status == rsl::FAILED && x.failedStage == 4 && x.stage == 0); }   // one candle: A, H, then X fails it at stage 4
    }
    {   // failed: B on the DEFENDERS' book fails; B on the attackers' book is hover only
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1001, 990, 992}}); h.push_back({{995, 991, 994}}); h.push_back({{995, 991, 993}});
        std::multimap<long long, Ev> E; ev(E, keyOf(10), lst::DEF, "A", lst::LOW); ev(E, keyOf(11), lst::DEF, "B", lst::LOW, 1); ev(E, keyOf(12), lst::DEF, "B", lst::LOW, 0);
        const std::vector<rsl::Step> s = rsl::run(bars(h), E);
        CHECK(s[11].status == rsl::FORMING && s[11].stage == 2 && s[12].status == rsl::FAILED);
        ev(E, keyOf(11), lst::DEF, "P", lst::LOW, 1); CHECK(rsl::run(bars(h), E)[11].stage == 2);   // P by the attackers' book: no step
    }
    {   // the extreme taken out at stage 2+ = failed (and the same candle starts the next attempt); at stage 1 the extreme just moves
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1001, 990, 992}}); h.push_back({{995, 991, 994}}); h.push_back({{994, 985, 986}}); h.push_back({{990, 986, 988}});
        std::multimap<long long, Ev> E; ev(E, keyOf(10), lst::DEF, "A", lst::LOW); ev(E, keyOf(12), lst::ATK, "K", lst::LOW);
        std::vector<rsl::Step> s = rsl::run(bars(h), E);
        CHECK(s[12].status == rsl::FAILED && s[12].failedStage == 2 && s[12].stage == 1 && s[12].side == rsl::LOW && s[12].why.find("taken out") != std::string::npos);
        CHECK(s[13].status == rsl::FORMING && s[13].ext == 985 && s[13].chain == "K");
        std::multimap<long long, Ev> E2; ev(E2, keyOf(10), lst::ATK, "K", lst::LOW);
        s = rsl::run(bars(h), E2);
        CHECK(s[12].status == rsl::FORMING && s[12].stage == 1 && s[12].ext == 985 && s[12].start == keyOf(10));
    }
    {   // a HIGH attempt (red), and the other side's extreme replaces a stage-1 attempt
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1010, 1003, 1008}}); h.push_back({{1009, 1005, 1006}}); h.push_back({{1006, 995, 996}});
        std::multimap<long long, Ev> E; ev(E, keyOf(10), lst::ATK, "K", lst::HIGH); ev(E, keyOf(12), lst::ATK, "S", lst::LOW);
        const std::vector<rsl::Step> s = rsl::run(bars(h), E);
        CHECK(s[10].side == rsl::HIGH && s[10].stage == 1 && rsl::lookOf(s[10]).colour == lst::RED);
        CHECK(s[12].side == rsl::LOW && s[12].stage == 1 && s[12].status == rsl::EXPIRED);
    }
    {   // expiry: stage 0 after 5 idle candles
        std::vector<std::array<long long, 3> > h = base; h.push_back({{1001, 990, 992}}); for (int k = 0; k < 6; ++k) h.push_back({{995, 991, 993}});
        const std::vector<rsl::Step> s = rsl::run(bars(h), std::multimap<long long, Ev>());
        bool exp = false; for (size_t k = 10; k < s.size(); ++k) { CHECK(s[k].stage == 0); exp = exp || s[k].status == rsl::EXPIRED; } CHECK(exp);
        for (size_t k = 0; k < s.size(); ++k) CHECK(rsl::lookOf(s[k]).height == 0 && !rsl::lookOf(s[k]).stub);
    }
    {   rsl::Step r; CHECK(!rsl::parse("RS|1|100|-1|3|1|0|0|0|||", r)); CHECK(!rsl::parse("RS|1|100|0|7|1|0|0|0|||", r)); CHECK(rsl::parse("RS|1|100|-1|0|0|0|0|0|||", r));
        CHECK(!rsl::parse("RS|1|100|0|2|1|0|0|0||", r)); CHECK(rsl::fileName("6E", 180) == "RS-EU-180.csv"); }
    std::printf("%s: %d checks, %d failed\n", fails ? "FAILED" : "PASSED", n, fails);
    return fails ? 1 : 0;
}
