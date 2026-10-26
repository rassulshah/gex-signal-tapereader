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
    CHECK(lst::stripOf(lst::DEF, "F?") == 1 && lst::rankOf(lst::DEF, "F?") == 0 && lst::rankOf(lst::DEF, "H") == 1 && lst::rankOf(lst::DEF, "X") == 3);   // (1.4.0) F? first on the Release strip
    { std::vector<lst::Lt> v; v.push_back(lst::make(lst::DEF, "H", lst::LOW, -1, "")); v.push_back(lst::make(lst::DEF, "F?", lst::LOW, -1, "")); const lst::Pick pk = lst::pick(lst::DEF, v); CHECK(pk.shown[1] == 1); }   // F? beats H
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


    // Typed stage semantics, causal provenance, frozen checkpoints, and RS2
    // records live in test_stage_core.cpp.  This file deliberately retains the
    // strip priority/colour/layout and legacy LT parser assertions above.
    std::printf("%s: %d checks, %d failed\n", fails ? "FAILED" : "PASSED", n, fails);
    return fails ? 1 : 0;
}
