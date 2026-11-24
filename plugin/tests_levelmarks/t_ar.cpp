// t_ar.cpp -- LevelMarks 1.13.0 / 1.14.0 (the ATR way, bottom of main): the AR, the bar that answers a climax (Rassul 2026-11-23 09:25 CT "i want you to implement the AR which is an automatic
// rally bar that occurs after a cx . so if there is a climax, and there is a bar that has its low below the pivot high and is within 3 bars of the pivot and has
// a BRMax of at least 10. so if the cx was in a downwave , the ARbar would have to be green", 09:26 "the AR bar should also have a strength line", his example:
// NQ 11-23 6:51 AM). Real candles + the real Cx badges of the price chart's marks file: NQ 2026-11-20 .. 11-23 and ES 2026-11-20 (the mockup's three windows).
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_ar.cpp -o t && ./t
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, ok = 0; static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string x; while (std::getline(ss, x, '|')) v.push_back(x); return v; }
static const long long E1904 = 2082844800LL;
struct Fx { std::vector<wyr::Bar> B; std::map<long long, lvm::Mark> M; std::vector<long long> T; std::vector<double> V, R; std::vector<char> G; };
static Fx load(const char* bars, const char* wy) {
    Fx x; std::ifstream in(bars); std::string line;
    while (std::getline(in, line)) { const auto v = split(line); if (v.size() < 9 || v[0] != "B") continue;
        wyr::Bar b; b.t = std::stoll(v[1]); b.o = std::stoll(v[2]); b.h = std::stoll(v[3]); b.l = std::stoll(v[4]); b.c = std::stoll(v[5]); b.buy = std::stoll(v[6]); b.sell = std::stoll(v[7]); b.when = v[8]; x.B.push_back(b); }
    std::ifstream w(wy); while (std::getline(w, line)) { const auto v = split(line); if (v.size() < 3 || v[1] != "Cx") continue; lvm::Mark m; m.code = 'C'; m.dir = std::stoi(v[2]); x.M[std::stoll(v[0])] = m; }
    for (const wyr::Bar& b : x.B) { x.T.push_back(b.t); x.V.push_back((double)(b.buy + b.sell)); x.R.push_back((double)(b.h - b.l)); x.G.push_back(b.c >= b.o ? 1 : 0); }   // as the host: range in ticks, green = close >= open
    return x;
}
static std::map<long long, lvm::SMark> ars(const Fx& x) { return lvm::arPick(lvm::arCands(x.B, x.M), x.M, x.T, x.V, x.R, x.G); }
static long long at(long long day, int h, int m) { return day + h * 3600LL + m * 60LL + E1904; }
// a hand-made candle: stamp i * 180, prices in ticks
static wyr::Bar bar(int i, long long o, long long h, long long l, long long c, long long vol = 100) { wyr::Bar b; b.t = 1000000 + i * 180LL; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = vol / 2; b.sell = vol - vol / 2; return b; }
struct Syn { std::vector<wyr::Bar> B; std::map<long long, lvm::Mark> M; std::vector<long long> T; std::vector<double> V, R; std::vector<char> G;
    void fin() { T.clear(); V.clear(); R.clear(); G.clear(); for (const wyr::Bar& b : B) { T.push_back(b.t); V.push_back((double)(b.buy + b.sell)); R.push_back((double)(b.h - b.l)); G.push_back(b.c >= b.o ? 1 : 0); } }
    std::map<long long, lvm::SMark> ar() { fin(); return lvm::arPick(lvm::arCands(B, M), M, T, V, R, G); } };
// 30 quiet candles drifting down (range 4), a pivot low at candle 30 (the Cx there), then the candles given; the last candle confirms the pivot when it takes out the 3-candle high
static Syn down(const std::vector<wyr::Bar>& after, int cxAt = 30) {
    Syn s; long long p = 1000; for (int i = 0; i < 30; ++i) { s.B.push_back(bar(i, p, p + 1, p - 3, p - 2)); p -= 2; }
    s.B.push_back(bar(30, p, p + 1, p - 9, p - 6, 900));                       // the pivot candle: low p - 9, high p + 1
    for (const wyr::Bar& b : after) s.B.push_back(b);
    lvm::Mark m; m.code = 'C'; m.dir = 1; s.M[s.B[(size_t)cxAt].t] = m; return s;
}
int main() {
    const long long d23 = 1795392000LL, d20 = 1795132800LL, d22 = d23 - 86400;
    // ---- NQ, the real candles and the real Cx badges
    { const Fx x = load("nq1123ar.psv", "wy_NQ_1123.txt"); C(x.B.size() > 500 && x.M.size() >= 20, "NQ fixture loaded");
      const auto A = ars(x);
      const auto a651 = A.find(at(d23, 6, 51));
      C(a651 != A.end() && a651->second.code == 'A' && a651->second.dir == 1, "NQ 11-23 6:51 AM = AR, green (his circled candle)");
      C(a651 != A.end() && a651->second.st == "R51" && a651->second.bkT == at(d23, 6, 45), "its strength line R51; it belongs to the 6:45 AM Cx");
      C(!A.count(at(d23, 6, 48)) && !A.count(at(d23, 6, 45)) && !A.count(at(d23, 6, 54)), "the pivot candle 6:48, the Cx candle 6:45 and 6:54 are not AR");
      const auto a833 = A.find(at(d20, 8, 33)), a848 = A.find(at(d20, 8, 48));
      C(a833 != A.end() && a833->second.dir == -1 && a833->second.st == "V99+" && a833->second.bkT == at(d20, 8, 30), "NQ 11-20 8:33 AM = AR, red, after the 8:30 Cx at the high (V99+)");
      C(a848 != A.end() && a848->second.dir == 1 && a848->second.st == "V99+" && a848->second.bkT == at(d20, 8, 39), "NQ 11-20 8:48 AM = AR, green, after the 8:39 Cx (pivot 8:42): the 2nd candle after the pivot");
      C(!A.count(at(d20, 8, 45)), "8:45 AM (green, BRMax 0) is passed over");
      C(A.count(at(d23, 2, 12)) && A.find(at(d23, 2, 12))->second.dir == -1, "NQ 11-23 2:12 AM = AR, red");
      for (const auto& kv : A) { const auto it = std::lower_bound(x.T.begin(), x.T.end(), kv.first); const int i = (int)(it - x.T.begin()); const wyr::Bar& b = x.B[(size_t)i];
          C(kv.second.dir > 0 ? b.c > b.o : b.c < b.o, "every AR has the colour: " + b.when); { const std::vector<double> atr = wyr::atr13(x.B); const auto ci = std::lower_bound(x.T.begin(), x.T.end(), kv.second.bkT); const wyr::Bar& cx = x.B[(size_t)(ci - x.T.begin())];   // (1.14.0) BRMax 10+ OR the ATR way
            const bool brm = lvm::stCount(x.R, x.G, i) >= lvm::AR_MIN, atrWay = i > 0 && (double)(b.h - b.l) > atr[(size_t)i - 1] && (kv.second.dir > 0 ? b.h > cx.h : b.l < cx.l);
            C(brm || atrWay, "every AR has BRMax 10+ or is bigger than the ATR and beyond its Cx candle: " + b.when);
            const std::string& st = kv.second.st; C(!st.empty() && (st[0] == 'V' || st[0] == 'R' || (st.back() == 'x' && st.find('.') != std::string::npos)), "every AR has its strength line: " + b.when);
            if (!st.empty() && st.back() == 'x') { C(atrWay && lvm::stCount(x.R, x.G, i) < lvm::ST_MIN && lvm::stCount(x.V, x.G, i) < lvm::ST_MIN, "an 'x' strength line only without a V / R count: " + b.when);
                char w[16]; std::snprintf(w, sizeof w, "%.1fx", (double)(b.h - b.l) / atr[(size_t)i - 1]); C(st == w, "the 'x' line = range / ATR(13) of the candle before: " + b.when + " " + st + " " + w); } } C(x.M.count(kv.second.bkT) == 1, "every AR belongs to a Cx badge: " + b.when); }
      { std::set<long long> cx; for (const auto& kv : A) cx.insert(kv.second.bkT); C(cx.size() == A.size(), "one AR per Cx"); }
      // the candidates: 6:45 Cx -> pivot 6:48; candles with colour + overlap in order
      { const auto K = lvm::arCands(x.B, x.M); bool got = false; for (const auto& a : K) if (a.cxT == at(d23, 6, 45)) { got = true; C(a.pivT == at(d23, 6, 48) && a.dir == 1 && !a.c.empty() && a.c[0] == at(d23, 6, 51), "the 6:45 Cx sits on the candle before its pivot (6:48); first candidate 6:51"); }
        C(got, "the 6:45 Cx has its candidates"); }
      // no repaint: candle by candle from Sunday 5 PM, an AR appears no earlier than its candle's close and never changes or leaves
      { int bad = 0, early = 0, seen = 0; std::map<long long, lvm::SMark> prev; const long long t0 = at(d22, 17, 3);
        for (size_t n = 60; n <= x.B.size(); ++n) { if (x.B[n - 1].t < t0) continue;
            Fx p; p.B.assign(x.B.begin(), x.B.begin() + (long)n); p.T.assign(x.T.begin(), x.T.begin() + (long)n); p.V.assign(x.V.begin(), x.V.begin() + (long)n); p.R.assign(x.R.begin(), x.R.begin() + (long)n); p.G.assign(x.G.begin(), x.G.begin() + (long)n);
            for (const auto& kv : x.M) if (kv.first <= p.B.back().t) p.M[kv.first] = kv.second;
            const auto Q = ars(p);
            for (const auto& kv : Q) { if (kv.first > p.B.back().t) ++early; const auto it = A.find(kv.first); if (it == A.end() || it->second.dir != kv.second.dir || it->second.st != kv.second.st || it->second.bkT != kv.second.bkT) ++bad; }
            for (const auto& kv : prev) if (!Q.count(kv.first)) ++bad;
            seen = (int)Q.size(); prev = Q; }
        C(bad == 0 && early == 0, "replay candle by candle: an AR never changes, never leaves, never shows before its candle closed (" + std::to_string(bad) + " / " + std::to_string(early) + ")"); C(seen == (int)A.size(), "the replay ends with the same ARs"); }
      // a Cx that is not drawn any more gives no AR
      { Fx y = x; y.M.erase(at(d23, 6, 45)); C(!ars(y).count(at(d23, 6, 51)), "no Cx badge = no AR"); }
      { Fx y = x; y.M[at(d23, 6, 45)].code = 'S'; C(!ars(y).count(at(d23, 6, 51)), "a Sw on that candle is not a Cx: no AR"); }
      { Fx y = x; y.R.pop_back(); C(ars(y).empty(), "vectors of different lengths: nothing (never a crash)"); } }
    // ---- ES Friday afternoon: two Cx at highs, no AR
    { const Fx x = load("es1120ar.psv", "wy_ES_1123.txt"); C(x.B.size() > 600, "ES fixture loaded"); const auto A = ars(x);
      C(x.M.count(at(d20, 12, 21)) && x.M.count(at(d20, 12, 57)), "ES 12:21 PM and 12:57 PM are Cx badges");
      int n = 0; for (const auto& kv : A) if (kv.first >= at(d20, 12, 6) && kv.first <= at(d20, 13, 45)) ++n;
      C(n == 0, "ES 11-20 12:06-1:45 PM: no AR (the wide red 1:00 PM candle is the pivot candle itself)");
      for (const auto& a : lvm::arCands(x.B, x.M)) if (a.cxT == at(d20, 12, 57)) C(a.pivT == at(d20, 13, 0), "the 12:57 PM Cx: pivot 1:00 PM"); }
    // ---- each condition, on hand-made candles (pivot candle 30: high p + 1 = 941, low 931; p = 940)
    {
      // the plain case: candle 31 green, low 934 < 941, range 12 (the biggest in 30+), candle 32 takes out the 3-candle high and confirms the pivot
      { Syn s = down({ bar(31, 934, 946, 934, 945), bar(32, 945, 950, 944, 949) }); const auto A = s.ar();
        C(A.size() == 1 && A.count(s.B[31].t) && A.begin()->second.dir == 1 && A.begin()->second.st == "R31" && A.begin()->second.bkT == s.B[30].t, "plain case: the first candle after the pivot is the AR, strength R31 (the widest of the 31 candles before it)"); }
      // BRMax 9 against 10: nine quiet candles before a candle of range 5, then the same with ten
      for (int quiet : { 9, 10 }) { Syn s; long long q = 2000; for (int i = 0; i < 12; ++i) { s.B.push_back(bar(i, q, q + 1, q - 9, q - 8)); q -= 8; }    // big candles down
          for (int i = 12; i < 12 + quiet - 1; ++i) { s.B.push_back(bar(i, q, q + 1, q - 2, q - 1)); q -= 1; }                                    // quiet ones (range 3)
          const int pv = 12 + quiet - 1; s.B.push_back(bar(pv, q, q + 1, q - 3, q - 2)); q -= 2;                                                   // the pivot candle (range 4: quiet too)
          s.B.push_back(bar(pv + 1, q, q + 5, q, q + 4));                                                                                          // green, range 5: bigger than the `quiet` candles before it only
          s.B.push_back(bar(pv + 2, q + 4, q + 12, q + 3, q + 11));
          lvm::Mark m; m.code = 'C'; m.dir = 1; s.M[s.B[(size_t)pv].t] = m; s.fin();
          const int n = lvm::stCount(s.R, s.G, pv + 1); const auto A = s.ar();
          C(n == quiet, "hand-made: the candle's BRMax is " + std::to_string(quiet) + " (" + std::to_string(n) + ")");
          C(quiet == 9 ? !A.count(s.B[(size_t)pv + 1].t) : A.count(s.B[(size_t)pv + 1].t) == 1, quiet == 9 ? "BRMax 9: not an AR" : "BRMax 10: the AR"); }
      { Syn s = down({ bar(31, 934, 946, 934, 933), bar(32, 934, 950, 934, 949), bar(33, 949, 952, 948, 951) });                                // 31 closes under its open (red): skipped; 32 green
        const auto A = s.ar(); C(A.size() == 1 && A.count(s.B[32].t), "a red candle is passed over; the next green one is the AR"); }
      { Syn s = down({ bar(31, 934, 946, 934, 934), bar(32, 934, 950, 934, 949), bar(33, 949, 952, 948, 951) });                                // 31 a doji (close = open)
        C(!s.ar().count(s.B[31].t), "close = open is not green"); }
      { Syn s = down({ bar(31, 942, 954, 941, 953), bar(32, 953, 958, 952, 957) });                                                             // low 941 = the pivot candle's high: no overlap
        C(s.ar().empty(), "its low AT the pivot candle's high is not below it: no AR"); Syn s2 = down({ bar(31, 941, 953, 940, 952), bar(32, 952, 957, 951, 956) }); C(s2.ar().size() == 1, "one tick below: the AR"); }
      { Syn s = down({ bar(31, 934, 936, 933, 935), bar(32, 935, 937, 934, 936), bar(33, 936, 938, 935, 937), bar(34, 936, 950, 935, 949), bar(35, 949, 953, 948, 952) });
        C(s.ar().empty(), "the 4th candle after the pivot is too late"); }
      { Syn s = down({ bar(31, 934, 936, 933, 935), bar(32, 935, 937, 934, 936), bar(33, 936, 950, 935, 949), bar(34, 949, 953, 948, 952) });
        const auto A = s.ar(); C(A.size() == 1 && A.count(s.B[33].t), "the 3rd candle after the pivot still counts"); }
      { Syn s = down({ bar(31, 934, 946, 934, 945), bar(32, 945, 950, 944, 949) }, 29); const auto A = s.ar();                                     // the Cx on the candle BEFORE the pivot
        C(A.size() == 1 && A.count(s.B[31].t) && A.begin()->second.bkT == s.B[29].t, "the Cx on the candle before the pivot: the 3 candles are counted from the pivot"); }
      { Syn s = down({ bar(31, 934, 946, 934, 945), bar(32, 945, 950, 944, 949) }, 27); C(s.ar().empty(), "a Cx that is neither the pivot nor the candle before it: nothing"); }
      { Syn s = down({ bar(31, 934, 946, 934, 945), bar(32, 945, 950, 944, 949) }); s.M.begin()->second.dir = -1; C(s.ar().empty(), "a Cx of the other side at a pivot low: nothing"); }
      { Syn s = down({ bar(31, 934, 937, 933, 936) }); C(lvm::arCands(s.B, s.M).empty() && s.ar().empty(), "the pivot not confirmed yet: no candidate, no AR"); }
      // the mirror: an up wave, the Cx at the pivot high, the AR red with its high above the pivot candle's low
      { Syn s; long long q = 1000; for (int i = 0; i < 30; ++i) { s.B.push_back(bar(i, q, q + 3, q - 1, q + 2)); q += 2; }
        s.B.push_back(bar(30, q, q + 9, q - 1, q + 6, 900)); s.B.push_back(bar(31, q + 6, q + 6, q - 6, q - 5)); s.B.push_back(bar(32, q - 5, q - 4, q - 10, q - 9));
        lvm::Mark m; m.code = 'C'; m.dir = -1; s.M[s.B[30].t] = m; const auto A = s.ar();
        C(A.size() == 1 && A.count(s.B[31].t) && A.begin()->second.dir == -1 && A.begin()->second.st == "R31", "mirror: a red AR after a climax at a high (R31)"); }
      // two Cx whose first passing candle is the same one: it is marked once
      { Syn s = down({ bar(31, 934, 946, 934, 945), bar(32, 945, 950, 944, 949) }); lvm::Mark m; m.code = 'C'; m.dir = 1; s.M[s.B[29].t] = m; C(s.ar().size() == 1, "one AR per candle"); } }
    C(std::string(lvm::seqText('A')) == "AR" && std::string(lvm::seqText('R')) == "Rev" && std::string(lvm::seqText('F')) == "FT" && std::string(lvm::seqText('T')) == "Tst", "seqText: AR, the others as before");
    // ---- (1.14.0, Rassul 2026-11-23 14:00 CT "if the Green bar is bigger than the ATR and its high is above the cmx bar with the hevy volume , it is also valid AR")
    { const Fx x = load("nq1123ar.psv", "wy_NQ_1123.txt"); const auto A = ars(x); const long long d20 = 1795132800LL;
      const auto a745 = A.find(at(d20, 7, 45));
      C(a745 != A.end() && a745->second.dir == 1 && a745->second.bkT == at(d20, 7, 36), "NQ 11-20 7:45 AM = AR by the ATR way, green, after the 7:36 Cx (the mockup's candle)");
      C(a745 != A.end() && a745->second.st == "1.2x", "its strength line reads 1.2x (no BRMax 10)");
      C(!A.count(at(d20, 7, 39)) && !A.count(at(d20, 7, 42)), "7:39 and 7:42 (green, not above the Cx candle's high) are passed over");
      C(A.count(at(1795392000LL, 6, 51)) && A.find(at(1795392000LL, 6, 51))->second.st == "R51", "his 6:51 AM example is unchanged (R51)"); }
    {   // hand-made: quiet candles of both colours (range 4, so ATR is about 4 and no candle is a BRMax 10), a pivot low with range 10 (high 941)
        auto mk = [](const std::vector<wyr::Bar>& after) { Syn s; long long p = 1000; for (int i = 0; i < 30; ++i) { s.B.push_back(i == 21 ? bar(i, p - 2, p + 6, p - 3, p) /* one green candle of range 9: later greens under it have no BRMax 10 */ : i % 2 ? bar(i, p - 2, p + 1, p - 3, p) : bar(i, p, p + 1, p - 3, p - 2)); p -= 2; }
            s.B.push_back(bar(30, p, p + 1, p - 9, p - 6, 900)); for (const wyr::Bar& b : after) s.B.push_back(b); lvm::Mark m; m.code = 'C'; m.dir = 1; s.M[s.B[30].t] = m; return s; };
        { Syn s = mk({ bar(31, 935, 943, 935, 942), bar(32, 942, 947, 941, 946) }); const auto A = s.ar();
          C(A.size() == 1 && A.begin()->first == s.B[31].t && !A.begin()->second.st.empty() && A.begin()->second.st.back() == 'x', "range 8 > ATR and high 943 above the Cx candle's 941: the AR, strength in x"); }
        { Syn s = mk({ bar(31, 934, 941, 933, 940), bar(32, 941, 946, 939, 940), bar(33, 940, 942, 939, 941) });
          C(s.ar().empty(), "range 8 > ATR but the high only AT the Cx candle's high: no AR (and the small green after it is under the ATR)"); }
        { Syn s = mk({ bar(31, 938, 942, 938, 941), bar(32, 941, 946, 940, 940), bar(33, 940, 942, 939, 941) });
          C(s.ar().empty(), "above the Cx candle's high but range 4, not bigger than the ATR: no AR"); }
        { Syn s = mk({ bar(31, 941, 950, 941, 949), bar(32, 949, 952, 948, 950) });
          C(s.ar().empty(), "bigger than the ATR and above the Cx candle, but its low is not below the pivot candle's high: no AR (rule 4 stays)"); }
        { Syn s = mk({ bar(31, 934, 936, 933, 935), bar(32, 935, 937, 934, 936), bar(33, 936, 938, 935, 937), bar(34, 936, 946, 935, 945), bar(35, 945, 948, 944, 947) });
          C(s.ar().empty(), "the 4th candle after the pivot never counts, ATR way included (rule 2 stays)"); }
        { Syn s = mk({ bar(31, 942, 943, 935, 936), bar(32, 936, 944, 935, 943), bar(33, 943, 946, 942, 945) }); const auto A = s.ar();
          C(A.size() == 1 && A.begin()->first == s.B[32].t, "a red candle is never the AR of a low; the next green one that passes is"); }
    }
    {   // mirror: a climax at a high, a red candle bigger than the ATR with its low below the Cx candle's low
        Syn s; long long p = 1000; for (int i = 0; i < 30; ++i) { s.B.push_back(i % 2 ? bar(i, p + 2, p + 3, p - 1, p) : bar(i, p, p + 3, p - 1, p + 2)); p += 2; }
        s.B.push_back(bar(30, p, p + 9, p - 1, p + 6, 900));   // pivot high: 1060 .. low 1059, high 1069
        s.B.push_back(bar(31, p + 5, p + 5, p - 3, p - 2)); s.B.push_back(bar(32, p - 2, p - 1, p - 5, p - 4));
        lvm::Mark m; m.code = 'C'; m.dir = -1; s.M[s.B[30].t] = m; const auto A = s.ar();
        C(A.size() == 1 && A.begin()->first == s.B[31].t && A.begin()->second.dir == -1, "mirror: a red candle bigger than the ATR, low below the Cx candle's low = AR of a high");
    }
    C(lvm::AR_MIN == 10 && lvm::AR_BARS == 3 && std::string(lvm::VERSION) == "1.20.0", "AR_MIN 10, AR_BARS 3, LevelMarks 1.14.0");
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
