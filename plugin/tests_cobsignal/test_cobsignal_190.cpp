// lsCobSignal 1.9.0: key levels OR 3-hour levels.  ./cmp cobsignal_es.csv
#include "CobSignalLogic.h"
#include <fstream>
#include <iostream>
#include <map>
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static bool htfName(const std::string& s) { return s == "S1" || s == "S2" || s == "R1" || s == "R2"; }
int main(int argc, char** argv) {
    if (argc < 2) { std::cout << "fixture path needed\n"; return 1; }
    std::vector<cobs::Bar> B; { std::ifstream f(argv[1]); std::string l; std::getline(f, l);
        while (std::getline(f, l)) { cobs::Bar b; if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &b.o, &b.h, &b.l, &b.c, &b.v) == 6) B.push_back(b); } }
    cobs::Engine O; O.setMarket("ES", 0.25, 180); O.keySweep = false; for (const cobs::Bar& b : B) O.addBar(b);     // the 1.8.x rule: 3-hour only
    cobs::Engine E; E.setMarket("ES", 0.25, 180); for (const cobs::Bar& b : B) E.addBar(b);                          // 1.9.0: either
    std::map<std::pair<long long, int>, const cobs::Mark*> M; for (const cobs::Mark& m : E.marks) M[std::make_pair(m.t, m.side)] = &m;
    int kept = 0, both = 0, keyOnly = 0, htfOnly = 0, bad = 0, ph = 0, rthOnly = 0, hidden = 0;
    for (const cobs::Mark& o : O.marks) { const auto it = M.find(std::make_pair(o.t, o.side));
        if (it != M.end() && it->second->levels.compare(0, o.levels.size(), o.levels) == 0 && it->second->dratio == o.dratio && it->second->legStart == o.legStart) ++kept; }
    for (const cobs::Mark& m : E.marks) { std::vector<std::string> t; size_t a = 0; while (a <= m.levels.size()) { size_t q = m.levels.find('/', a); if (q == std::string::npos) q = m.levels.size(); t.push_back(m.levels.substr(a, q - a)); a = q + 1; }
        int nh = 0, nk = 0; bool order = true; for (const std::string& s : t) { if (htfName(s)) { ++nh; if (nk) order = false; } else ++nk; if (s.empty()) ++bad; }
        if (!order) ++bad; if (nh && nk) ++both; else if (nk) ++keyOnly; else ++htfOnly;
        if (!nh && nk) { const int mod = (int)((m.t % 86400 + 86400) % 86400) / 60; if (mod > 8 * 60 + 30 && mod <= 11 * 60) ++rthOnly; if (m.phOnly) ++ph; if (!m.show) ++hidden; } }
    std::cout << "3-hour only rule: " << O.marks.size() << " CoBs | either: " << E.marks.size() << " (3-hour only " << htfOnly << ", key only " << keyOnly << ", both " << both << "; prior-hour alone " << ph << ", hidden " << hidden << ")\n";
    CHECK(O.marks.size() >= 5 && kept == (int)O.marks.size(), "every 3-hour CoB of the old rule is still a CoB, same wave, its 3-hour names first (" << kept << " of " << O.marks.size() << ")");
    CHECK(keyOnly > 0 && E.marks.size() == O.marks.size() + (size_t)keyOnly, "the only new CoBs are the ones that swept a key level alone (" << keyOnly << ")");
    CHECK(bad == 0, "the level names are well formed, 3-hour names before key names");
    CHECK(rthOnly == keyOnly, "a key-level-only CoB is always in RTH by 11:00 AM (" << rthOnly << " of " << keyOnly << ")");
    {   std::string rec; bool htfLine = true; for (const std::string& l : E.out) { if (l.compare(0, 4, "HTF,") == 0) { int commas = 0; for (char ch : l) if (ch == ',') ++commas; htfLine = htfLine && commas >= 4; } }
        CHECK(htfLine, "an HTF record line always names a 3-hour level with its price"); }
    {   const size_t cut = B.size() * 2 / 3; cobs::Engine P; P.setMarket("ES", 0.25, 180); for (size_t i = 0; i < cut; ++i) P.addBar(B[i]); bool pre = true; size_t k = 0;
        for (const cobs::Mark& m : E.marks) if (m.idx < (int)cut - 5) { ++k; bool f = false; for (const cobs::Mark& p : P.marks) if (p.t == m.t && p.side == m.side && p.levels == m.levels) f = true; pre = pre && f; }
        CHECK(pre && k > 10, "no repaint: the CoBs decided on the first two thirds are the same when the rest of the bars arrive (" << k << ")"); }
    std::cout << "cmp_190: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
