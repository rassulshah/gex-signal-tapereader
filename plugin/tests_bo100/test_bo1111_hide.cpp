// test_bo1111_hide.cpp -- lsBreakout 1.11.1 (Rassul HG 10:13): no Rly letter where lsCobSignal draws a Cob on the same extreme, no Pb where
// it draws an Lvp on the same pivot - DISPLAY only (the decisions / records unchanged). SDK-free.
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bo1111_hide.cpp -o t1111 && ./t1111
#include "BreakoutHide.h"
#include <fstream>
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
static std::vector<bko::In> load(const std::string& f) {
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
static bko::Scene draw(const bko::Engine& E, int n) {
    bko::View v; v.first = 0; v.last = n - 1; v.area.l = -100000; v.area.t = -100000; v.area.r = 100000; v.area.b = 100000;
    v.X = [](int i) { return 10 + i * 12; }; v.Y = [](double p) { return (int)std::lround(100000 - p * 8); }; v.textW = [](const char* s, int) { return (int)std::strlen(s) * 6; };
    return bko::layout(E, v);
}
int main() {
    const std::vector<bko::In> B = load("es_2026-10-28_30_3m.txt");
    CHECK(B.size() > 300, "ES fixture loaded (" + std::to_string(B.size()) + " bars)");
    int om = 0, cm = 0; svl::rthOf("ES", om, cm);
    bko::Engine E; E.update(B, (long)B.size(), B[0].t, 0.25, 180, om, cm);
    const bko::Scene before = draw(E, (int)B.size());
    const size_t rlyRec = E.rly.size(), pbRec = E.pb.size(), revRec = E.rev.size();
    bkh::Cob C; bkh::hide(E, C, "ES", 0.25, 180, B, [](long long t) { return t - OFF; });
    const bko::Scene after = draw(E, (int)B.size());
    std::printf("  ES Oct 28-30: Cobs %zu (drawn %zu Rly-hide keys / 3), Lvps %zu; Rly drawn %d -> %d (hidden %d), Pb drawn %d -> %d (hidden %d)\n",
                C.CB.marks.size(), E.hideRly.size() / 3, C.CB.lvp.marks.size(), before.rlyShown, after.rlyShown, after.rlyHidden, before.pbShown, after.pbShown, after.pbHidden);
    CHECK(E.rly.size() == rlyRec && E.pb.size() == pbRec && E.rev.size() == revRec, "decisions unchanged (Rly / Pb / Rev kept, recorded)");
    CHECK(after.rlyShown + after.rlyHidden == before.rlyShown && after.pbShown + after.pbHidden <= before.pbShown, "every hidden letter was a drawn one; nothing else changes");
    int bad = 0;
    for (const bko::Txt& t : after.txt) { if (t.kind == 1 && E.hideRly.count(std::make_pair(B[(size_t)t.bar].t, t.dir))) ++bad; if (t.kind == 2 && E.hidePb.count(std::make_pair(B[(size_t)t.bar].t, t.dir))) ++bad; }
    CHECK(bad == 0, "no Rly drawn on a drawn Cob's extreme, no Pb on a drawn Lvp's pivot (" + std::to_string(bad) + ")");
    // forced: a drawn Cob at a Rly's end (opposite side, same candle side) hides it; a drawn Lvp at a Pb pivot hides it; other sides stay
    {   bko::Engine F = E; F.hideRly.clear(); F.hidePb.clear(); int rlyN = 0, pbN = 0;
        for (const auto& kv : F.rly) if (kv.second.fired) { F.hideRly.insert(std::make_pair(kv.second.te, kv.second.dir)); ++rlyN; break; }
        for (const auto& kv : F.pb) { F.hidePb.insert(std::make_pair(kv.second.tp, kv.second.dir)); ++pbN; break; }
        const bko::Scene s0 = draw(E, (int)B.size()); bko::Engine G = F; G.hideRly.clear(); G.hidePb.clear(); const bko::Scene s1 = draw(G, (int)B.size()); const bko::Scene s2 = draw(F, (int)B.size());
        CHECK(s2.rlyHidden == rlyN && s2.pbHidden <= pbN && s2.rlyShown == s1.rlyShown - rlyN, "a forced Cob / Lvp key hides exactly that Rly / Pb"); (void)s0; }
    CHECK(std::string(bko::VERSION) == "1.11.1", "version 1.11.1");
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
