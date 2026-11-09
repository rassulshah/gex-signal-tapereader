// test_entrybadge.cpp -- EntryBadge.h 1.0.0 (lsDealerProfile / lsLimitProfile 2.56.0, Rassul 16:34 CT "Ok build"): the ENT / EXIT / 1/2 tags.
//  [1] REAL study cases (cases.txt: the step-2 study's bars, labels, key / gamma levels, book, pull flags; the mockup v2 trades):
//      ES 11-03 10:54 Pdoc long -> "ENT Pdoc" at the PB POC 7600.50 after the decision, gone once resolved
//      ES 11-04 08:33 Pdoc long -> "ENT Pdoc" at 7686.50 at the decision; the next bar filled and stopped -> gone (PULL only shows while the
//      trade is open on a closed bar: synthetic case)
//      CL 10-27 13:51 Pdoc short -> "ENT ?" at CR 100.00, after the fill "EXIT ?" on the first real book bid >= 0.5R beyond (99.60, the study rule; the mockup showed the biggest, 99.50)
//      GC 10-13 10:18 Pdoc short -> "1/2" at G2 4180 + "rest" at PFL 4178.1
//      NG 11-02 10:18 Rly Pdoc short -> "ENT Pdoc ?" at 2.915
//  [2] rules: NQ / HG / 6E nothing; ES / NG need a Pdoc (Pb / Pboc -> no ENT); entry level beyond the stop and >= 0.1 ATR from the close;
//      the newest label wins; 20 bars unfilled -> gone; target before the fill -> gone; stop -> gone; deterministic (same input, same output)
// From plugin\: g++ -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_eb/test_entrybadge.cpp -o teb && ./teb (run from tests_eb)
#include "EntryBadge.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
struct Case { long id = 0; std::string m; double tick = 0, atr = 0; ebd::Lbl L; std::vector<ebd::Bar> B; std::set<long long> pull; std::vector<ebd::Lvl> V; std::vector<ebd::Book> K; };
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::string c; for (char ch : s) { if (ch == '|') { v.push_back(c); c.clear(); } else c += ch; } v.push_back(c); return v; }
static std::vector<Case> load(const char* f) {
    std::vector<Case> out; std::ifstream in(f); std::string l; Case C;
    while (std::getline(in, l)) { const std::vector<std::string> p = split(l);
        if (p[0] == "CASE") { C = Case(); C.id = std::stol(p[1]); C.m = p[2]; C.tick = std::stod(p[3]); C.atr = std::stod(p[4]); }
        else if (p[0] == "L") { C.L.td = std::stoll(p[1]); C.L.side = std::stoi(p[2]); C.L.code = std::stoi(p[3]); C.L.pbPoc = std::stod(p[4]); C.L.stop = std::stod(p[5]); C.L.target = std::stod(p[6]); }
        else if (p[0] == "B") { ebd::Bar b; b.t = std::stoll(p[1]); b.h = std::stod(p[2]); b.l = std::stod(p[3]); b.c = std::stod(p[4]); C.B.push_back(b); if (p[5] == "1") C.pull.insert(b.t); }
        else if (p[0] == "V") C.V.push_back(ebd::Lvl{ std::stod(p[1]), p[2], p[3] == "1" });
        else if (p[0] == "K") C.K.push_back(ebd::Book{ std::stod(p[1]), std::stoll(p[2]), p[3] == "1" });
        else if (p[0] == "END") out.push_back(C); }
    return out;
}
static ebd::Out run(const Case& C, size_t nBars, ebd::Lbl L) {
    std::vector<ebd::Bar> B(C.B.begin(), C.B.begin() + (long)std::min(nBars, C.B.size()));
    return ebd::compute(C.m, B, std::vector<ebd::Lbl>{ L }, C.V, C.K, C.tick, C.atr, [&](long long t, int) { return C.pull.count(t) != 0; });
}
static size_t idxOf(const Case& C, long long t) { for (size_t k = 0; k < C.B.size(); ++k) if (C.B[k].t == t) return k; return C.B.size(); }
static const ebd::Tag* tagOf(const ebd::Out& o, int kind) { for (const ebd::Tag& t : o.tags) if (t.kind == kind) return &t; return nullptr; }
static std::string show(const ebd::Out& o) { std::string s = o.why + ":"; for (const ebd::Tag& t : o.tags) { char b[96]; std::snprintf(b, sizeof b, " [%s @ %.6g %s%s]", t.text.c_str(), t.px, t.name.c_str(), t.book ? " book" : ""); s += b; } return s; }
int main() {
    const std::vector<Case> cs = load("cases.txt"); CHECK(cs.size() == 5, "5 study cases");
    for (const Case& C : cs) {
        const size_t i = idxOf(C, C.L.td); CHECK(i < C.B.size(), "decision bar found " + std::to_string(C.id));
        const ebd::Out a = run(C, i + 2, C.L);
        std::printf("case %ld %s at decision+1: %s\n", C.id, C.m.c_str(), show(a).c_str());
        const ebd::Out z = run(C, C.B.size(), C.L);
        std::printf("case %ld %s at session end: %s\n", C.id, C.m.c_str(), show(z).c_str());
        const ebd::Out a2 = run(C, i + 2, C.L); CHECK(show(a) == show(a2), "deterministic " + std::to_string(C.id));
        if (C.id == 322) { const ebd::Tag* t = tagOf(a, 1); CHECK(t && t->text == "ENT Pdoc" && std::fabs(t->px - 7600.5) < 1e-9 && !t->book && t->name == "PbPOC", "ES 322: ENT Pdoc at the PB POC 7600.50");
            CHECK(a.tags.size() == 1, "ES 322: no exit badge on ES"); CHECK(z.tags.empty() && z.why == "resolved", "ES 322: gone once resolved (+4.67R trade)"); }
        if (C.id == 405) { const ebd::Out d = run(C, i + 1, C.L); std::printf("case 405 at the decision bar: %s\n", show(d).c_str());
            const ebd::Tag* t = tagOf(d, 1); CHECK(t && t->text == "ENT Pdoc" && std::fabs(t->px - 7686.5) < 1e-9, "ES 405: ENT Pdoc at 7686.50 at the decision");
            CHECK(a.tags.empty() && a.why == "stopped", "ES 405: the next bar filled and stopped (-1R) -> gone at its close");
            CHECK(z.tags.empty(), "ES 405: gone after the stop"); }
        if (C.id == 41) { const ebd::Out d = run(C, i + 1, C.L); std::printf("case 41 at the decision bar: %s\n", show(d).c_str());
            const ebd::Tag* t = tagOf(d, 1); CHECK(t && t->text == "ENT ?" && std::fabs(t->px - 100.0) < 1e-9 && t->name.compare(0, 2, "CR") == 0, "CL 41: ENT ? at CR 100.00");
            CHECK(!tagOf(d, 2), "CL 41: no EXIT before the fill");
            CHECK(a.filled, "CL 41: filled on the next bar");
            const ebd::Tag* x = tagOf(a, 2); CHECK(x && x->book && std::fabs(x->px - 99.6) < 1e-9 && x->text == "EXIT ?", "CL 41: EXIT ? on the first book bid >= 0.5R beyond (99.60, the study rule) after the fill"); }
        if (C.id == 698) { const ebd::Tag* h1 = tagOf(a, 3); const ebd::Tag* h2 = tagOf(a, 4);
            CHECK(h1 && std::fabs(h1->px - 4180.0) < 1e-9, "GC 698: 1/2 at G2 4180"); CHECK(h2 && std::fabs(h2->px - 4178.1) < 1e-9, "GC 698: rest at PFL 4178.1"); }
        if (C.id == 1225) { const ebd::Tag* t = tagOf(a, 1); CHECK(t && t->text == "ENT Pdoc ?" && std::fabs(t->px - 2.915) < 1e-9, "NG 1225: ENT Pdoc ? at 2.915"); }
        // [2] a Pb / Pboc on the same bars -> no ENT on ES / NG (no valid Pdoc)
        if (C.m == "ES" || C.m == "NG") { ebd::Lbl L = C.L; L.code = (L.code / 10) * 10 + 1; CHECK(run(C, i + 2, L).tags.empty(), "no ENT on a Pb " + std::to_string(C.id));
            L.code = (L.code / 10) * 10 + 2; CHECK(run(C, i + 2, L).tags.empty(), "no ENT on a Pboc " + std::to_string(C.id)); }
    }
    {   Case C = cs.front(); C.m = "NQ"; CHECK(run(C, idxOf(C, C.L.td) + 2, C.L).tags.empty(), "NQ: nothing"); C.m = "HG"; CHECK(run(C, idxOf(C, C.L.td) + 2, C.L).tags.empty(), "HG: nothing");
        C.m = "6E"; CHECK(run(C, idxOf(C, C.L.td) + 2, C.L).tags.empty(), "6E: nothing"); }
    {   // synthetic: newest label wins; 20 bars unfilled -> gone; target before fill -> gone; ENT too close to the close -> none
        std::vector<ebd::Bar> B; for (int k = 0; k < 40; ++k) B.push_back(ebd::Bar{ 1000LL + 180 * k, 105.0, 104.0, 104.5 });
        ebd::Lbl L; L.td = B[5].t; L.side = 1; L.code = 3; L.pbPoc = 103.0; L.stop = 101.0; L.target = 110.0;
        auto rn = [&](size_t n, std::vector<ebd::Lbl> ls) { std::vector<ebd::Bar> b(B.begin(), B.begin() + (long)n); return ebd::compute("ES", b, ls, {}, {}, 0.25, 2.0, [](long long, int) { return false; }); };
        CHECK(rn(10, { L }).tags.size() == 1, "synthetic: ENT while waiting");
        CHECK(rn(27, { L }).tags.empty(), "synthetic: 20 bars unfilled -> gone");
        ebd::Lbl L2 = L; L2.td = B[8].t; L2.pbPoc = 102.0; const ebd::Out o = rn(12, { L, L2 }); CHECK(o.tags.size() == 1 && std::fabs(o.tags[0].px - 102.0) < 1e-9, "synthetic: the newest label wins");
        std::vector<ebd::Bar> B2 = B; B2[7].h = 111.0; { std::vector<ebd::Bar> b(B2.begin(), B2.begin() + 10); CHECK(ebd::compute("ES", b, { L }, {}, {}, 0.25, 2.0, [](long long, int) { return false; }).tags.empty(), "synthetic: target before the fill -> gone"); }
        ebd::Lbl L3 = L; L3.pbPoc = 104.4; CHECK(rn(10, { L3 }).tags.empty(), "synthetic: ENT < 0.1 ATR from the close -> none");
        ebd::Lbl L4 = L; L4.pbPoc = 101.1; CHECK(rn(10, { L4 }).tags.empty(), "synthetic: ENT not beyond the stop by a tick -> none");
        {   std::vector<ebd::Bar> b(B.begin(), B.begin() + 12); b[8].l = 102.9; const long long pt = b[8].t;   // touched 103.00 on bar 8, bids leaving there
            const ebd::Out q = ebd::compute("ES", b, { L }, {}, {}, 0.25, 2.0, [pt](long long t, int side) { return t == pt && side > 0; });
            CHECK(q.tags.size() == 1 && q.tags[0].text == "ENT Pdoc PULL" && q.pull && q.filled, "synthetic: bids leaving at the touched ENT -> ENT Pdoc PULL");
            const ebd::Out q2 = ebd::compute("ES", b, { L }, {}, {}, 0.25, 2.0, [pt](long long t, int side) { return t == pt && side < 0; });
            CHECK(q2.tags.size() == 1 && q2.tags[0].text == "ENT Pdoc", "synthetic: offers leaving is not PULL for a long");
            std::vector<ebd::Bar> c(B.begin(), B.begin() + 12); const ebd::Out q3 = ebd::compute("ES", c, { L }, {}, {}, 0.25, 2.0, [&](long long t, int) { return t == c[9].t; });
            CHECK(q3.tags.size() == 1 && q3.tags[0].text == "ENT Pdoc", "synthetic: no PULL while the ENT level was not traded"); }
        std::vector<ebd::Bar> B3 = B; B3[8].l = 100.5; { std::vector<ebd::Bar> b(B3.begin(), B3.begin() + 12); CHECK(ebd::compute("ES", b, { L }, {}, {}, 0.25, 2.0, [](long long, int) { return false; }).tags.empty(), "synthetic: stop traded -> gone"); }
    }
    std::printf("test_entrybadge: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
