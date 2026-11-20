// test_levelliq_100.cpp -- LevelLiq.h 1.0.0 (lsLimitProfile 2.68.0, 2026-11-20): each state's rule, no repaint (restore = straight pass),
// the line ends after a break (and after P only when P is drawn), two charts / markets independent, the Now body changes only on change.
// Build (Linux, from plugin/): g++ -std=c++17 -g -fsanitize=address,undefined -Werror -Wall -Wextra -I. test_levelliq_100.cpp -o t && ./t
#include "LevelLiq.h"
#include <cstdio>
#include <map>
#include <string>
#include <vector>
static int pass = 0, fail = 0;
static void check(bool ok, const std::string& what) { std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str()); (ok ? pass : fail)++; }
// a fake book: resting / cumulative cancelled / filled per (side, px)
struct Book { std::map<std::pair<int, long long>, llq::Probe> m;
    llq::Probe at(int s, long long p) const { auto it = m.find({s, p}); return it == m.end() ? llq::Probe() : it->second; }
    void rest(int s, long long p, long long q) { m[{s, p}].resting = q; }
    void cancel(int s, long long p, long long q) { auto& x = m[{s, p}]; x.resting -= q; x.cxl += q; }
    void fill(int s, long long p, long long q) { auto& x = m[{s, p}]; x.resting -= q; x.fill += q; } };
struct Sim { llq::Tracker T; Book B; long long now = 36000; std::vector<std::string> rec;
    explicit Sim(const llq::Params& p = llq::Params()) : T(p) {}
    std::vector<llq::SegIn> seg(long long lvl, bool bid, long long node, const char* nm = "PDL", bool conf = false) { llq::SegIn s; s.lvl = lvl; s.bid = bid; s.node = node; s.name = nm; s.conf = conf; s.t0 = 36000; return {s}; }
    void step(long long last, const std::vector<llq::SegIn>& a) { T.step(now, last, 8.0, a, [this](int s, long long p) { return B.at(s, p); }); drain(); }
    void bar(long long o, long long h, long long l, long long c) { now += 180; llq::Bar b; b.endS = now; b.o = o; b.h = h; b.l = l; b.c = c; T.close(b, [this](int s, long long p) { return B.at(s, p); }); drain(); }
    void drain() { for (auto& r : T.take()) rec.push_back(r.line); }
    std::string all() const { std::string s; for (auto& l : rec) s += l + "\n"; return s; }
    int count(char k) const { int n = 0; for (auto& l : rec) if (!l.empty() && l[0] == k) ++n; return n; }
    std::string lastC() const { for (size_t i = rec.size(); i-- > 0;) if (rec[i][0] == 'C') return rec[i]; return ""; } };
// one bid level at 1000 (ticks), node at 999 with 100 lots; price comes down from 1010
static void approach(Sim& S, long long node0 = 100) {
    S.B.rest(0, 999, node0); auto a = S.seg(1000, true, 999);
    S.step(1010, a); S.bar(1012, 1014, 1006, 1008);   // reached (within 8 ticks) -> S0 = node0; this candle wholly above the level
    S.step(1004, a); }
static std::string field(const std::string& l, int k) { size_t p = 0; for (int i = 0; i < k; ++i) { p = l.find('|', p); if (p == std::string::npos) return ""; ++p; } return l.substr(p, l.find('|', p) - p); }
int main() {
    {   Sim S; approach(S); S.B.rest(0, 999, 115); S.bar(1008, 1009, 1003, 1004);   // grew 15% at a closed candle
        const llq::Watch* w = S.T.find(1000, true); check(w && w->st == llq::ST_BUILDING, "BUILDING: node >= 1.10 x S0 at a candle close");
        S.B.rest(0, 999, 85); S.bar(1004, 1005, 1002, 1003); w = S.T.find(1000, true); check(w && w->st == llq::ST_LEAVING, "LEAVING: node <= 0.90 x S0");
        S.B.rest(0, 999, 100); S.bar(1003, 1005, 1002, 1004); w = S.T.find(1000, true); check(w && w->st == llq::ST_STEADY, "STEADY between");
        check(S.count('T') == 3, "each state change recorded once (T lines " + std::to_string(S.count('T')) + ")"); }
    {   Sim S; approach(S); auto a = S.seg(1000, true, 999);
        S.step(1000, a);                                  // contact: price at the level, the candle before wholly above
        S.B.fill(0, 1000, 30); S.now += 50; S.step(1000, a); S.B.fill(0, 1000, 10); S.now += 70; S.step(1000, a); S.now -= 120;   // attack: 30 lots in min 1, 10 in min 2
        S.B.fill(0, 1001, 4); S.bar(1004, 1005, 1000, 1003);   // last minute 4 (< half of 30), node 100 kept, closed back above, not through
        const std::string c = S.lastC(); check(field(c, 5) == "XH", "X + H at the contact close (attack slowing, size stays, closed back): " + field(c, 5));
        check(S.T.marks().size() == 2 && S.T.drawLetter('H') && !S.T.drawLetter('X') && !S.T.drawLetter('P'), "only H is drawn (X recorded, not drawn)");
        S.bar(1003, 1006, 1001, 1005); S.bar(1005, 1008, 1003, 1007); S.bar(1007, 1009, 1004, 1008);
        check(S.count('O') == 1 && S.all().find("|XH|held") != std::string::npos, "outcome recorded once after 3 candles: held");
        check(S.T.ends().empty(), "a held level: the line goes on"); }
    {   Sim S; approach(S); auto a = S.seg(1000, true, 999); S.step(1000, a);
        S.B.cancel(0, 999, 80); S.B.fill(0, 999, 5); S.bar(1004, 1005, 999, 1001);   // 80 of 100 cancelled while hit, little filled
        check(field(S.lastC(), 5) == "P", "P: node < half at the close, cancelled > half and > filled");
        check(S.T.ends().empty() && S.count('E') == 1, "P recorded as an end (E|..|P) but P is not drawn, so the drawn line does not end");
        llq::Params p; p.drawP = true; Sim D(p); approach(D); D.step(1000, D.seg(1000, true, 999)); D.B.cancel(0, 999, 80); D.bar(1004, 1005, 999, 1001);
        check(D.T.ends().size() == 1 && D.T.ends()[0].letter == 'P', "with P drawn, the line ends at the P candle"); }
    {   Sim S; approach(S); S.step(1000, S.seg(1000, true, 999)); S.bar(1004, 1005, 996, 997);   // 4 ticks through, closed through
        const std::string c = S.lastC(); check(c.find("|H|") == std::string::npos && S.T.ends().size() == 1 && S.T.ends()[0].letter == 'B', "a break: no H, the line ends at that candle");
        check(S.T.endIn(1000, true, 36000, S.now) == S.now, "endIn finds the end inside the stretch"); }
    {   Sim S; approach(S); S.step(1000, S.seg(1000, true, 999)); S.bar(1004, 1005, 998, 1001);   // 2 ticks through, closed back: H (3+ is through)
        check(field(S.lastC(), 5).find('H') != std::string::npos, "H tolerates 1-2 ticks through (levels are zones)"); }
    {   // contact needs the candle before wholly on the defending side
        Sim S; S.B.rest(0, 999, 100); auto a = S.seg(1000, true, 999); S.step(1003, a); S.bar(1004, 1006, 999, 1003); S.step(1000, a); S.bar(1003, 1004, 999, 1002);
        check(S.count('C') == 0, "no contact when the candle before already traded at the level"); }
    {   // offers mirror: level 1000 above, red side
        Sim S; S.B.rest(1, 1001, 50); auto a = S.seg(1000, false, 1001, "PDH"); S.step(992, a); S.bar(990, 994, 988, 992); S.step(1000, a); S.bar(992, 1000, 991, 996);
        const std::string c = S.lastC(); check(field(c, 4) == "-1" && field(c, 5).find('H') != std::string::npos && !S.T.marks().empty() && !S.T.marks()[0].bid, "offers: H on an offer line (drawn red by the host)"); }
    {   // NO REPAINT: a restart mid-session restores the decided letters / ends from the record identically
        Sim S; approach(S); auto a = S.seg(1000, true, 999); S.step(1000, a); S.bar(1004, 1005, 1000, 1003); S.bar(1003, 1004, 1001, 1003);
        Sim S2; approach(S2); S2.step(1000, S2.seg(1000, true, 999)); S2.bar(1004, 1005, 996, 997);
        llq::Tracker R; R.restore(S.all()); llq::Tracker R2; R2.restore(S2.all());
        bool same = R.marks().size() == S.T.marks().size(); for (size_t i = 0; same && i < R.marks().size(); ++i) same = R.marks()[i].endS == S.T.marks()[i].endS && R.marks()[i].letter == S.T.marks()[i].letter && R.marks()[i].lvl == S.T.marks()[i].lvl;
        check(same, "restart: the letters come back identical from the record");
        check(R2.ends().size() == 1 && R2.ends()[0].endS == S2.T.ends()[0].endS, "restart: the line end comes back from the record");
        R.restore(S.all()); check(R.marks().size() == S.T.marks().size(), "restoring twice adds nothing");
        // a later candle never changes a decided letter
        const size_t n0 = S.T.marks().size(); const long long e0 = S.T.marks()[0].endS; S.B.cancel(0, 999, 100); S.bar(1003, 1004, 990, 991);
        check(S.T.marks().size() == n0 && S.T.marks()[0].endS == e0 && S.T.marks()[0].letter == 'H', "a later break never changes the decided H");
        llq::Bar old; old.endS = 1; S.T.close(old, [](int, long long) { return llq::Probe(); }); check(S.T.marks().size() == n0, "an old / repeated candle is ignored"); }
    {   // two charts / markets: separate trackers never share state
        Sim A, B; approach(A); A.step(1000, A.seg(1000, true, 999)); A.bar(1004, 1005, 1000, 1003);
        check(B.T.marks().empty() && B.T.watches().empty() && !A.T.marks().empty(), "two trackers (charts / markets) are independent"); }
    {   // the Now body: stable while nothing changes, changes when the state does; garbled restore input is ignored
        Sim S; approach(S); auto a = S.seg(1000, true, 999); const std::string b1 = S.T.nowBody(S.now); S.now += 5; S.step(1004, a); const std::string b2 = S.T.nowBody(S.now);
        check(!b1.empty() && b1 == b2 && b1.find("L|PDL|1000|B|steady|0|") == 0, "Now body: one row per live line, unchanged while nothing changes: " + b1.substr(0, b1.size() - 1));
        S.step(1000, a); S.bar(1004, 1005, 1000, 1003); const std::string b3 = S.T.nowBody(S.now);
        check(b3 != b2 && b3.find("|B|H|1|") != std::string::npos, "after the contact close: H, shown 1 (drawn)");
        S.now += 3600; check(S.T.nowBody(S.now).empty(), "a stretch gone > 3 min: no row");
        llq::Tracker G; G.restore("C|x|PDL|1|1|H\nC|5|a|b|1|H\nE|7|n|10|2|B\n" + std::string(100, '|')); check(G.marks().empty() && G.ends().empty(), "garbled record lines are ignored"); }
    {   Sim S; S.B.rest(0, 999, 100); auto a = S.seg(1000, true, 999); S.step(1050, a); S.bar(1050, 1052, 1048, 1051);
        check(S.T.find(1000, true)->phase == 0, "not reached while price is beyond 1 ATR"); }
    std::printf("RESULT %d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
