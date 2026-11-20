// t_rr0210.cpp -- lsRevRead 0.21.0 / WyRead 1.8.0: T / P / X rules, decided at the close, no repaint (bar-by-bar replay with restarts,
// byte-identical), the strip's 2-per-candle limit and priority, colours (dir), showMore, run() unchanged; real ES 11-18 (MBO store) and
// 11-19 (his chart: OHLC + the pane's V buy / sell + the Book Flow records) fixtures.
// build: g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. t_rr0210.cpp -o t && ./t        dump: ./t dump  (the mockup data)
#include "WyRead.h"
#include "LevelMarks.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static wyr::Bar mk(long long t, long long o, long long h, long long l, long long c, long long buy, long long sell, bool book = true, long long pb = 100, long long pa = 100) {
    wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = buy; b.sell = sell; b.book = book; b.pull[0] = pb; b.pull[1] = pa; return b; }
// 30 quiet candles drifting down 1 tick each (buy = sell = 100, pulls 100), then the case candles appended by each test
static std::vector<wyr::Bar> base(int n = 30) { std::vector<wyr::Bar> B; long long p = 1000;
    for (int i = 0; i < n; ++i) { B.push_back(mk(3600LL * 1000 + 180LL * (i + 1), p + 2, p + 4, p, p + 1, 100, 100)); p -= 1; } return B; }
static std::string ser(const wyr::Out& o, long long upTo = -1) { std::string s;
    for (const auto& kv : o.marks) { if (upTo >= 0 && kv.first > upTo) continue; for (const wyr::Mark& m : kv.second) s += std::to_string(kv.first) + "|" + wyr::label(m) + "|" + std::to_string(m.dir) + "\n"; } return s; }
static bool has(const wyr::Out& o, long long t, char k, int dir) { const auto it = o.marks.find(t); if (it == o.marks.end()) return false; for (const wyr::Mark& m : it->second) if (m.k == k && m.dir == dir) return true; return false; }
static int count(const wyr::Out& o, char k) { int n = 0; for (const auto& kv : o.marks) for (const wyr::Mark& m : kv.second) if (m.k == k) ++n; return n; }
static long long nt(const std::vector<wyr::Bar>& B) { return B.back().t + 180; }

static void testT() {
    // a new low on 2x selling with negative delta, then the next candle closes above its high with positive delta -> T (green) on that candle
    std::vector<wyr::Bar> B = base(); const long long lo = B.back().l;
    B.push_back(mk(nt(B), lo + 2, lo + 3, lo - 4, lo - 3, 80, 200));          // k: new low, sellers 2.0x, delta -120
    B.push_back(mk(nt(B), lo - 3, lo + 6, lo - 3, lo + 5, 220, 90));          // j: no lower low, close above k's high (lo+3), delta +130
    const wyr::Out o = wyr::more(B, 0.25, 2); CHECK(has(o, B.back().t, 'T', 1)); CHECK(count(o, 'T') == 1);
    CHECK(o.reads.count(B.back().t) && o.reads.at(B.back().t).head == "Sellers trapped");
    { auto C = B; C[C.size() - 2].sell = 140; CHECK(count(wyr::more(C, 0.25, 2), 'T') == 0); }        // sellers 1.4x: no trap
    { auto C = B; C[C.size() - 2].buy = 250; CHECK(count(wyr::more(C, 0.25, 2), 'T') == 0); }        // delta positive on k: no trap
    { auto C = B; C.back().c = lo + 3; CHECK(count(wyr::more(C, 0.25, 2), 'T') == 0); }              // closes AT k's high, not above
    { auto C = B; C.back().buy = 80; CHECK(count(wyr::more(C, 0.25, 2), 'T') == 0); }                // reclaim with negative delta
    { auto C = B; C.back().l = lo - 5; CHECK(count(wyr::more(C, 0.25, 2), 'T') == 0); }              // a lower low first: no trap
    { auto C = B; wyr::Bar x = C.back(); C.pop_back();                                               // the reclaim on the 3rd candle after k: still T; 4th: none
      C.push_back(mk(nt(C), lo - 3, lo, lo - 3, lo - 1, 100, 100)); C.push_back(mk(nt(C), lo - 1, lo + 1, lo - 2, lo, 100, 100)); x.t = nt(C); C.push_back(x);
      CHECK(has(wyr::more(C, 0.25, 2), C.back().t, 'T', 1));
      auto D = C; D.pop_back(); D.push_back(mk(nt(D), lo, lo + 1, lo - 1, lo, 100, 100)); x.t = nt(D); D.push_back(x); CHECK(count(wyr::more(D, 0.25, 2), 'T') == 0); }
    // mirror at a high: rising drift, a new high on 2x buying with negative-for-buyers ... (delta > 0 = buyers), then a close below k's low with delta < 0 -> T red
    std::vector<wyr::Bar> U; long long p = 1000; for (int i = 0; i < 30; ++i) { U.push_back(mk(3600LL * 1000 + 180LL * (i + 1), p - 1, p, p - 4, p - 2, 100, 100)); p += 1; }
    const long long hi = U.back().h;
    U.push_back(mk(nt(U), hi - 2, hi + 4, hi - 3, hi + 3, 200, 80)); U.push_back(mk(nt(U), hi + 3, hi + 3, hi - 6, hi - 5, 90, 220));
    const wyr::Out ou = wyr::more(U, 0.25, 2); CHECK(has(ou, U.back().t, 'T', -1));
    // T needs no Book Flow record (buy / sell only)
    { auto C = B; for (auto& b : C) b.book = false; CHECK(has(wyr::more(C, 0.25, 2), C.back().t, 'T', 1)); }
}
static void testP() {
    std::vector<wyr::Bar> B = base(); const long long lo = B.back().l;
    B.push_back(mk(nt(B), lo + 1, lo + 2, lo, lo, 90, 130, true, 210, 100));   // tests the low (= the 10 candles' low), sellers 1.3x, bids pulled 2.1x
    wyr::Out o = wyr::more(B, 0.25, 2); CHECK(has(o, B.back().t, 'P', -1)); CHECK(o.reads.at(B.back().t).head == "Support leaving");
    { auto C = B; C.back().pull[0] = 190; CHECK(count(wyr::more(C, 0.25, 2), 'P') == 0); }   // 1.9x pulled
    { auto C = B; C.back().sell = 110; CHECK(count(wyr::more(C, 0.25, 2), 'P') == 0); }      // hit 1.1x
    { auto C = B; C.back().book = false; CHECK(count(wyr::more(C, 0.25, 2), 'P') == 0); }    // no Book Flow record: no P
    { auto C = B; C.back().l = lo + 1; CHECK(count(wyr::more(C, 0.25, 2), 'P') == 0); }      // not at the low
    { auto C = B; for (size_t i = 0; i + 1 < C.size(); ++i) C[i].book = false; CHECK(count(wyr::more(C, 0.25, 2), 'P') == 0); }   // no pull norm (under 10 records)
}
static void testX() {
    std::vector<wyr::Bar> B = base(); const long long lo = B.back().l;
    B[B.size() - 1].sell = 150;                                                   // the attack (1.5x) on the candle before (an X may land on the base's own candles - checked per candle)
    B.push_back(mk(nt(B), lo + 1, lo + 2, lo - 2, lo - 1, 80, 90, true, 90, 100));  // a new low on 90 lots (0.6x of 150), bids pulled 0.9x
    wyr::Out o = wyr::more(B, 0.25, 2); CHECK(has(o, B.back().t, 'X', 1));
    { auto C = B; C.back().sell = 110; CHECK(!has(wyr::more(C, 0.25, 2), C.back().t, 'X', 1)); }   // 0.73x: not faded enough
    { auto C = B; C.back().pull[0] = 120; CHECK(!has(wyr::more(C, 0.25, 2), C.back().t, 'X', 1)); } // bids pulled 1.2x: defenders left
    { auto C = B; C[C.size() - 2].sell = 110; CHECK(!has(wyr::more(C, 0.25, 2), C.back().t, 'X', 1)); }   // no real attack before (1.1x)
}
static void testStrip() {
    using V = std::vector<wyr::SMk>; auto S = [](const char* l, int d) { wyr::SMk x; x.lab = l; x.dir = d; return x; };
    auto labs = [](const V& v) { std::string s; for (const auto& m : v) s += m.lab + (m.dir > 0 ? "+" : "-") + " "; return s; };
    CHECK(labs(wyr::stripPick({ S("C", 1), S("E", -1), S("Sp", 1) })) == "Sp+ E- ");
    CHECK(labs(wyr::stripPick({ S("B", 1), S("A", -1), S("T", 1), S("P", -1) })) == "T+ P- ");
    CHECK(labs(wyr::stripPick({ S("X", 1), S("A", -1) })) == "A- X+ ");
    CHECK(labs(wyr::stripPick({ S("E", 1), S("X", 1) })) == "X+ E+ ");
    CHECK(labs(wyr::stripPick({ S("B", 1), S("C", -1), S("E", 1) })) == "E+ B+ ");
    CHECK(labs(wyr::stripPick({ S("Ut", -1), S("T", -1), S("I", -1) })) == "Ut- I- ");
    CHECK(labs(wyr::stripPick({ S("A", -1), S("A", -1) })) == "A- ");               // once per candle
    CHECK(labs(wyr::stripPick({ S("Q", 1), S("A", 0) })) == "");                    // unknown code / no direction: dropped
    CHECK(wyr::stripPick({ S("C", 1), S("B", 1), S("E", 1), S("A", 1), S("T", 1) }).size() == 2);
    CHECK(wyr::prio("Sp") == wyr::prio("Ut") && wyr::prio("Sp") < wyr::prio("I") && wyr::prio("I") < wyr::prio("T") && wyr::prio("T") < wyr::prio("P") && wyr::prio("P") < wyr::prio("A")
          && wyr::prio("A") < wyr::prio("X") && wyr::prio("X") < wyr::prio("E") && wyr::prio("E") < wyr::prio("B") && wyr::prio("B") < wyr::prio("C"));
    // colours: green = dir +1, red = dir -1 - the pick keeps each letter's own direction
    const V v = wyr::stripPick({ S("T", 1), S("B", -1) }); CHECK(v.size() == 2 && v[0].dir == 1 && v[1].dir == -1);
    CHECK(wyr::showMore('T', "ES") && wyr::showMore('T', "NQ") && wyr::showMore('T', "CL") && !wyr::showMore('T', "GC") && !wyr::showMore('T', "HG"));
    CHECK(!wyr::showMore('P', "ES") && !wyr::showMore('X', "ES") && !wyr::showMore('P', "NQ") && !wyr::showMore('X', "CL"));
}
// ---- real data
static std::vector<std::string> lines(const char* p) { std::vector<std::string> v; std::ifstream f(p); std::string l; while (std::getline(f, l)) if (!l.empty()) v.push_back(l); return v; }
static std::vector<long long> nums(const std::string& l, char sep) { std::vector<long long> v; std::stringstream ss(l); std::string f; while (std::getline(ss, f, sep)) v.push_back(std::llround(std::atof(f.c_str()))); return v; }
static std::vector<wyr::Bar> es1118() {   // MBO store bars (every candle has the order book): t,o,h,l,c (ticks),buy,sell,addB,addA,pullB,pullA,maxBuy,maxSell
    std::vector<wyr::Bar> B; for (const std::string& l : lines("es1118_store.csv")) { const auto v = nums(l, ','); if (v.size() < 13) continue;
        wyr::Bar b = mk(v[0], v[1], v[2], v[3], v[4], v[5], v[6], true, v[9], v[10]); b.addNet[0] = v[7] - v[9]; b.addNet[1] = v[8] - v[10]; b.aggrMax[0] = v[12]; b.aggrMax[1] = v[11]; B.push_back(b); }
    return B; }
static std::vector<wyr::Bar> es1119() {   // his chart 11-19: OHLC (unix end), the pane's V buy / sell, the Book Flow records (193 of the candles)
    std::map<long long, std::pair<long long, long long> > V; for (const std::string& l : lines("es1119_v.psv")) { const auto v = nums(l, '|'); if (v.size() == 3) V[v[0]] = std::make_pair(v[1], v[2]); }
    std::map<long long, std::vector<long long> > R; for (const std::string& l : lines("es1119_book.psv")) { const auto v = nums(l, '|'); if (v.size() == 7) R[v[0]] = v; }
    std::vector<wyr::Bar> B;
    for (const std::string& l : lines("es1119_3m.csv")) { std::stringstream ss(l); std::string f; std::vector<double> d; while (std::getline(ss, f, ',')) d.push_back(std::atof(f.c_str())); if (d.size() < 6) continue;
        const long long t = (long long)d[0] + 2082844800LL; const auto it = V.find(t); if (it == V.end()) continue;
        wyr::Bar b = mk(t, std::llround(d[1] / 0.25), std::llround(d[2] / 0.25), std::llround(d[3] / 0.25), std::llround(d[4] / 0.25), it->second.first, it->second.second, false, 0, 0);
        const auto r = R.find(t); if (r != R.end()) { b.book = true; b.pull[0] = r->second[1]; b.pull[1] = r->second[2]; b.addNet[0] = r->second[3] - r->second[1]; b.addNet[1] = r->second[4] - r->second[2]; b.aggrMax[0] = r->second[5]; b.aggrMax[1] = r->second[6]; }
        B.push_back(b); }
    return B; }
static std::vector<wyr::Lvl> keys1119() { std::vector<wyr::Lvl> v; for (const std::string& l : lines("es1119_keys.csv")) { if (l.compare(0, 2, "L|") != 0) continue;
        const size_t a = l.find('|', 2), b = l.find('|', a + 1); wyr::Lvl z; z.name = l.substr(2, a - 2); z.px = std::llround(std::atof(l.substr(a + 1, b - a - 1).c_str()) / 0.25);
        if (lvm::pri(z.name, 0) >= 0) v.push_back(z); } return v; }
static void replay(const std::vector<wyr::Bar>& B, const char* nm) {
    // bar by bar, the indicator restarted before every bar (a fresh run on the closed candles so far): every letter it shows at that point is
    // byte-identical to what the full session shows for those candles, and a letter never appears on a candle older than the newest one
    const wyr::Out full = wyr::more(B, 0.25, 2); const wyr::Out fullRun = wyr::run(B, 0.25, 2);
    int bad = 0, late = 0; std::string prev;
    for (size_t n = 12; n <= B.size(); ++n) { const std::vector<wyr::Bar> P(B.begin(), B.begin() + (long)n); const long long tn = P.back().t;
        const wyr::Out o = wyr::more(P, 0.25, 2); const std::string s = ser(o);
        if (s != ser(full, tn)) ++bad;
        if (s.compare(0, prev.size(), prev) != 0 && !prev.empty()) { // the older text must be a prefix in time order: anything new is on the newest candle
            for (const auto& kv : o.marks) if (kv.first < tn && ser(full, kv.first) != ser(o, kv.first)) { ++late; break; } }
        prev = s; }
    std::printf("  %s: %zu candles, T %d P %d X %d (A %d E %d Sp %d Ut %d) - replay mismatches %d, back-dated %d\n", nm, B.size(), count(full, 'T'), count(full, 'P'), count(full, 'X'),
        count(fullRun, 'A'), count(fullRun, 'E'), count(fullRun, 'S'), count(fullRun, 'U'), bad, late);
    CHECK(bad == 0); CHECK(late == 0);
    const std::string a = ser(full), b = ser(wyr::more(B, 0.25, 2)); CHECK(a == b);   // the same candles, the same letters
}
static void dump() {   // the mockup data: per candle o h l c buy sell | the strip letters (the host's merge + stripPick)
    const std::vector<wyr::Bar> B = es1119(); const auto K = keys1119();
    const wyr::Out a = wyr::run(B, 0.25, 2, wyr::Params(), K), m = wyr::more(B, 0.25, 2); const auto lm = lvm::run(B, K);
    for (const wyr::Bar& b : B) { std::vector<wyr::SMk> c; auto put = [&](const std::string& l, int d) { wyr::SMk x; x.lab = l; x.dir = d; c.push_back(x); };
        { auto it = a.marks.find(b.t); if (it != a.marks.end()) for (const auto& x : it->second) if (x.k != 'I') put(wyr::label(x), x.dir); }
        { auto it = m.marks.find(b.t); if (it != m.marks.end()) for (const auto& x : it->second) if (wyr::showMore(x.k, "ES")) put(wyr::label(x), x.dir); }
        { auto it = lm.find(b.t); if (it != lm.end() && (it->second.code == 'B' || it->second.code == 'C')) put(it->second.code == 'B' ? "B" : "C", it->second.dir); }
        std::string all; for (const auto& x : c) all += x.lab + (x.dir > 0 ? "+" : "-") + " ";
        std::string pk; for (const auto& x : wyr::stripPick(c)) pk += x.lab + (x.dir > 0 ? "+" : "-") + " ";
        std::string hid; { auto it = m.marks.find(b.t); if (it != m.marks.end()) for (const auto& x : it->second) if (!wyr::showMore(x.k, "ES")) hid += wyr::label(x) + (x.dir > 0 ? "+" : "-") + " "; }
        std::printf("%lld,%lld,%lld,%lld,%lld,%lld,%lld,%s,%s,%s\n", b.t - 2082844800LL, b.o, b.h, b.l, b.c, b.buy, b.sell, pk.c_str(), all.c_str(), hid.c_str()); }
}
int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "dump") == 0) { dump(); return 0; }
    testT(); testP(); testX(); testStrip();
    const auto E18 = es1118(), E19 = es1119(); CHECK(E18.size() > 400); CHECK(E19.size() > 400);
    replay(E18, "ES 11-18 (MBO store)"); replay(E19, "ES 11-19 (his chart)");
    { const wyr::Out m = wyr::more(E19, 0.25, 2); int pxBook = 0; for (const auto& kv : m.marks) for (const auto& x : kv.second) if (x.k == 'P' || x.k == 'X') { for (const auto& b : E19) if (b.t == kv.first && !b.book) ++pxBook; }
      CHECK(pxBook == 0); }   // P / X only on candles with a Book Flow record
    std::printf("%d checks, %d failed\n", checks, fails); return fails ? 1 : 0;
}
