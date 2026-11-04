// grid_2179.h - one line per scenario: table box, sweep tier, sorted text multiset hash (texts only, version normalised) - lsHodLod 2.17.10 test
#include <set>
static std::string gridLine(const hl::Layout& L) {
    std::multiset<std::string> m; for (const hl::Item& it : L.items) if (it.role == hl::R_TABLE && it.k == hl::K_TEXT && !it.s.empty()) { std::string s = it.s; if (s == "PhrH") s = "PHH"; if (s == "PhrL") s = "PHL"; if (s.size() > 2 && s[0] == 'v' && s.compare(1, 2, "2.") == 0) s = "vVER"; m.insert(s + "|" + std::to_string(it.fs) + std::to_string((int)it.bold) + "|" + std::to_string(it.c1)); }
    std::string all; for (auto& s : m) { all += s; all += '\n'; }
    char b[200]; std::snprintf(b, sizeof b, "%d %d %d %d %d %d %d %llu", L.tableOk, L.swOk, L.swTier, L.table.l, L.table.t, L.table.r, L.table.b, fnv(all)); return b;
}
