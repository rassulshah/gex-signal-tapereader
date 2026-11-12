// t_revread.cpp -- lsRevRead 0.1.0 logic tests on his own ES order recording (2026-11-11 20:00-21:00 CT, revread_es1111.txt).
// Build (in plugin\tests_revread):  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_revread.cpp -o t && ./t
#include "../RevReadLogic.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
struct Ev { long long t; int act, side; long long px, sz, id, aggr; };
static const long long DAY = 1794355200LL;   // any whole day (seconds), a multiple of 180
static std::vector<Ev> load(const char* f) { std::vector<Ev> v; std::ifstream in(f); std::string l;
    while (std::getline(in, l)) { if (l.empty() || l[0] == '#') continue; std::vector<std::string> c; std::stringstream ss(l); std::string x; while (std::getline(ss, x, '|')) c.push_back(x); if (c.size() < 7) continue;
        Ev e; e.t = DAY + std::atoll(c[0].c_str()); e.act = std::atoi(c[1].c_str()); e.px = (long long)std::llround(std::atof(c[2].c_str()) * 4); e.sz = std::atoll(c[3].c_str()); e.id = std::atoll(c[4].c_str()); e.aggr = std::atoll(c[5].c_str());
        e.side = c[6] == "B" ? rvs::BID : c[6] == "S" ? rvs::ASK : -1; v.push_back(e); }
    return v; }
static long long T(int h, int m, int s = 0) { return DAY + h * 3600 + m * 60 + s; }
struct Run { rvs::Engine E; std::map<long long, std::string> letters; int changed = 0; std::map<long long, rvs::Poc> pocs; };
static std::string lettersOf(const std::vector<rvs::Mark>& v) { std::string s; for (const rvs::Mark& m : v) { s += m.letter; s += m.col > 0 ? '+' : '-'; } return s; }
// replay: the candle POC from the fills of that candle (what IRT's volume at price holds); mirror = a top (prices and sides flipped)
static void replay(Run& R, const std::vector<Ev>& V, bool mirror = false, bool midPoc = false, long long stopAt = 0) {
    R.E.P = rvs::forMarket("ES"); R.E.tick = 0.25; R.E.spb = 180;
    long long cur = 0, lo = 0, hi = 0, cl = 0; std::map<long long, std::pair<long long, long long> > vap;   // tick -> (vol, delta)
    auto close = [&]() { if (!cur) return; std::vector<rvs::VRow> rows; for (const auto& kv : vap) { rvs::VRow r; r.tick = kv.first; r.vol = kv.second.first; r.dlt = kv.second.second; rows.push_back(r); }
        rvs::Poc p = rvs::pocOf(cur, rows, lo, hi, cl); if (midPoc && p.ok) { p.tick = (lo + hi) / 2; p.pos = 0.5; }
        R.pocs[cur] = p; R.E.onCandle(cur, p, cl);
        for (const auto& kv : R.E.marks) { const std::string s = lettersOf(kv.second); auto it = R.letters.find(kv.first); if (it == R.letters.end()) R.letters[kv.first] = s; else if (it->second != s && s.find(it->second) == std::string::npos) ++R.changed; else it->second = s; } };
    for (const Ev& e0 : V) { Ev e = e0; if (stopAt && e.t >= stopAt) break;
        if (mirror) { e.px = 80000 - e.px; if (e.side >= 0) e.side = 1 - e.side; }
        const long long cd = R.E.candleOf(e.t * 1000);
        if (cd != cur) { close(); cur = cd; vap.clear(); lo = hi = cl = 0; }
        R.E.onEvent(e.t * 1000, e.act, e.side, e.px, e.sz, e.id, e.aggr);
        if (e.act == rvs::FILL) { if (!lo) lo = hi = e.px; lo = std::min(lo, e.px); hi = std::max(hi, e.px); cl = e.px; auto& r = vap[e.px]; r.first += e.sz; r.second += e.side == rvs::BID ? e.sz : -e.sz; } }
    if (!stopAt) close();   // a replay cut short leaves its last candle open (not decided)
}
static const rvs::Story* storyAt(const rvs::Engine& E, long long t0) { for (const rvs::Story& s : E.ended) if (s.t0 == t0 * 1000) return &s; for (int b = 0; b < 2; ++b) if (E.live[b] && E.st[b].t0 == t0 * 1000) return &E.st[b]; return nullptr; }
static std::string readOf(const rvs::Engine& E, const rvs::Story& s) { std::string r; const std::vector<rvs::Seg> L = E.readLine(s); for (size_t i = 0; i < L.size(); ++i) r += (i ? " > " : "") + L[i].text; return r; }
int main(int argc, char** argv) {
    const std::vector<Ev> V = load(argc > 1 ? argv[1] : "revread_es1111.txt");
    CHECK(V.size() > 40000, "fixture loaded (" << V.size() << " events)");
    CHECK(std::string(rvs::VERSION) == "0.4.1", "version 0.4.1");
    Run R; replay(R, V); const rvs::Engine& E = R.E;
    CHECK(E.sideMismatch == 0 && E.fills == 7307 && E.garbage == 0, "every fill's aggressor side agrees with the resting order's side (" << E.fills << " fills, " << E.sideMismatch << " mismatches)");
    // ---- [1] tonight's reversal: the 8:43 PM selling climax, absorbed, fading, flushed, buyers ----
    const rvs::Story* s = storyAt(E, T(20, 43, 20));
    CHECK(s && s->bottom && s->shown && s->end == rvs::E_COMPLETE, "[1] the 8:43:10-8:43:20 PM block starts a bottom story that completes");
    if (s) {
        CHECK(s->burst == 119 && s->opp == 40 && s->bigOrder == 50, "[1] climax: 119 lots sold in 10 s vs 40 bought, the biggest market order 50 lots (" << s->burst << " / " << s->opp << " / " << s->bigOrder << ")");
        CHECK(s->anchor == 30460 && s->shownSz >= 60 && s->shownSz <= 100, "[1] anchor 7615.00; 60-100 lots showing on the bids of the zone (7614.75-7615.50) before they were first hit (" << s->anchor * 0.25 << ", " << s->shownSz << ")");
        std::cout << "  absorbed at second-of-day " << (s->tA / 1000 - DAY) << "\n";
        CHECK(s->tA >= T(20, 43, 40) * 1000 && s->tA <= T(20, 45, 0) * 1000 && s->absR >= 1.5, "[1] absorbed between 8:43:40 and 8:45:00 PM (inside the climax candle): the lots sold into the zone reached 1.5x what was showing (" << s->absR << "x at the end)");
        CHECK(s->fading && s->fadeList.size() >= 2 && s->fadeList.back() < s->fadeList[s->fadeList.size() - 2], "[1] fading: each return sells less (" << readOf(E, *s) << ")");
        CHECK(s->tE > s->tA && s->flushLots > 0 && (double)s->flushLots <= 0.6 * 119 && s->extreme == 30455, "[1] exhausted: the new low 7613.75 on little volume (" << s->flushLots << " lots)");
        CHECK(s->tI == T(20, 47, 10) * 1000 && s->initLots == 47 && s->initOpp == 3, "[1] initiation: 47 lots bought vs 3 sold in the 8:47:00 block");
        const std::string r = readOf(E, *s);
        CHECK(r.find("Sell climax 119 (50-lot) > Absorbed ") == 0 && r.find("at 7615 > Fading ") != std::string::npos && r.find("Flush on ") != std::string::npos && r.find("Buyers 47") != std::string::npos, "[1] the read line: " << r);
    }
    // ---- [2] "i dont understand how you marked the 63 as a climax": the 8:29 PM burst ran through prices -> no letters ----
    const rvs::Story* q = storyAt(E, T(20, 29, 10));
    CHECK(q && q->burst == 120 && q->end != rvs::E_COMPLETE && q->pocGate == 0, "[2] the 8:29 PM burst (120 lots in 10 s) never completes and its candle's POC is not at the low: " << (q ? rvs::endName(q->end) : "missing"));
    CHECK(!R.letters.count(T(20, 30)), "[2] the 8:30 PM candle (POC 63) gets no letter");
    // ---- [3] letters: decided at the candle close, C red + A green on 8:45, E + I green on 8:48, nothing else in the hour ----
    CHECK(R.letters.size() == 2 && R.letters[T(20, 45)] == "C-A+" && R.letters[T(20, 48)] == "E+I+", "[3] letters: 8:45 PM = red C + green A, 8:48 PM = green E + green I, no others (" << R.letters.size() << ": " << R.letters[T(20, 45)] << " " << R.letters[T(20, 48)] << ")");
    { const rvs::Poc& p = R.pocs[T(20, 45)]; CHECK(p.ok && p.tick == 30461 && p.vol == 121 && p.pos < 0.34 && p.dlt > 0, "[3] the 8:45 PM candle's POC: 121 lots at 7615.25, in the lower third, more bought than sold there (pos " << p.pos << ", delta " << p.dlt << ")"); }
    CHECK(R.changed == 0, "[3] no repaint: a decided letter never changed during the replay (" << R.changed << ")");
    // ---- [4] the POC gate: the same orders with the climax candle's POC in the middle of the candle -> no letters, the read says why ----
    { Run M; replay(M, V, false, true); const rvs::Story* m = storyAt(M.E, T(20, 43, 20));
      CHECK(M.letters.empty() && m && m->pocGate == 0 && readOf(M.E, *m).find("POC not at the extreme") != std::string::npos, "[4] POC mid-candle -> no letters; the read: " << (m ? readOf(M.E, *m) : "")); }
    // ---- [5] the mirror: the same flow upside down is a TOP: green C (buying climax), red A / E / I ----
    { Run M; replay(M, V, true); const rvs::Story* m = storyAt(M.E, T(20, 43, 20));
      CHECK(m && !m->bottom && m->end == rvs::E_COMPLETE && m->burst == 119 && m->initLots == 47, "[5] mirrored: a top story with the same numbers");
      CHECK(M.letters.size() == 2 && M.letters[T(20, 45)] == "C+A-" && M.letters[T(20, 48)] == "E-I-", "[5] mirrored letters: green C, red A / E / I (" << M.letters[T(20, 45)] << " " << M.letters[T(20, 48)] << ")");
      CHECK(m && readOf(M.E, *m).find("Buy climax 119 (50-lot) > Absorbed") == 0 && readOf(M.E, *m).find("Sellers 47") != std::string::npos, "[5] mirrored read: " << (m ? readOf(M.E, *m) : "")); }
    // ---- [6] live: the read fills in step by step ----
    { Run M; replay(M, V, false, false, T(20, 43, 40)); const rvs::Story* m = M.E.newest(T(20, 43, 40) * 1000);
      CHECK(m && m->step == rvs::S_CLIMAX && readOf(M.E, *m).find("Sell climax 119 (50-lot) > absorbed? > fading? > flush? > buyers?") == 0, "[6] at 8:43:40 PM: " << (m ? readOf(M.E, *m) : "none"));
      Run N; replay(N, V, false, false, T(20, 44, 50)); const rvs::Story* n = N.E.newest(T(20, 44, 50) * 1000);
      CHECK(n && n->step == rvs::S_ABSORBED && readOf(N.E, *n).find("> Absorbed ") != std::string::npos && readOf(N.E, *n).find("buyers?") != std::string::npos && N.letters.empty(), "[6] at 8:44:50 PM absorbed, no letter yet (the candle is still open): " << (n ? readOf(N.E, *n) : "none")); }
    // ---- [7] a restart: the letters are reloaded from the record, identical ----
    { rvs::Engine F; F.spb = 180; const int n = F.loadMarks(E.out); CHECK(n == 2 && lettersOf(F.marks[T(20, 45)]) == "C-A+" && lettersOf(F.marks[T(20, 48)]) == "E+I+", "[7] reloaded " << n << " candles of letters from the record lines");
      std::vector<std::string> bad = { "L|abc|C-", "L|123", "L|1794429900|Z+", "garbage", "" }; rvs::Engine G; CHECK(G.loadMarks(bad) == 0, "[7] garbled record lines load nothing"); }
    // ---- [8] context: location (a level at the anchor), a stop run of a level, a volatility window, Pb ----
    { Run M; M.E.levels.push_back(std::make_pair(30461LL, std::string("VAL"))); M.E.levels.push_back(std::make_pair(30457LL, std::string("PDL"))); M.E.ctx.hourly = "Hrly up, testing 7616.75"; M.E.ctx.pb = 1; M.E.ctx.session = "ON";
      replay(M, V); const rvs::Story* m = storyAt(M.E, T(20, 43, 20)); const std::string r = m ? readOf(M.E, *m) : "";
      CHECK(m && m->ctx.level == "VAL" && m->sweep == "PDL" && m->ctx.pb == 1, "[8] the story is AT VAL, and its flush took out PDL (7614.25) and came back");
      CHECK(r.find("Stop run of PDL on ") != std::string::npos && r.find("Hrly up, testing 7616.75 | at VAL | Pb") != std::string::npos, "[8] the read with its context: " << r);
      bool rec = false; for (const std::string& l : M.E.out) if (l.find("|END|") != std::string::npos && l.find("|sess ON|") != std::string::npos && l.find("|level VAL|") != std::string::npos && l.find("|sweep PDL|") != std::string::npos && l.find("|end complete") != std::string::npos) rec = true;
      CHECK(rec, "[8] the record line carries session, level, sweep and the result for the nightly");
      Run W; W.E.ctx.volWin = 1; replay(W, V); const rvs::Story* w = storyAt(W.E, T(20, 43, 20));
      CHECK(w && !w->shown && W.letters.empty(), "[8] inside a volatility window the climax needs 1.5x more: the same 119 is recorded but not shown"); }
    // ---- [9] pieces ----
    CHECK(rvs::sessionOf(20 * 3600, 510, 900) == "ON" && rvs::sessionOf(3 * 3600, 510, 900) == "LON" && rvs::sessionOf(9 * 3600, 510, 900) == "RTH" && rvs::sessionOf(16 * 3600, 510, 900) == "ON", "[9] sessions: overnight / London / RTH");
    CHECK(rvs::volWindow(510 * 60 + 100, 510, 900) == 1 && rvs::volWindow(900 * 60 - 100, 510, 900) == 2 && rvs::volWindow(7 * 3600 + 1800 + 60, 510, 900) == 3 && rvs::volWindow(11 * 3600, 510, 900) == 0, "[9] volatility windows: the open, into the close, release times, none");
    CHECK(rvs::hourlyText(1, 1, 30467, 30485, false, 30462, 30458, 30470, 4, 0.25) == "Hrly up, testing 7616.75" && rvs::hourlyText(1, 1, 30467, 30485, false, 30480, 30478, 30482, 4, 0.25) == "Hrly up, pulling back to 7616.75"
          && rvs::hourlyText(1, 1, 30467, 30485, false, 30490, 30486, 30492, 4, 0.25) == "Hrly up, going with it" && rvs::hourlyText(-1, 1, 30467, 30440, true, 30450, 30448, 30452, 4, 0.25) == "Hrly dn (rotation up), trend broken" && rvs::hourlyText(0, 0, 0, 0, false, 1, 1, 1, 4, 0.25) == "Hrly none" && rvs::hourlyText(1, 1, 30467, 30485, false, 30445, 30442, 30450, 4, 0.25) == "Hrly up, below the test line 7616.75",
          "[9] the hourly read: testing / pulling back / going with it / broken / none");
    { std::vector<rvs::VRow> rows = { { 100, 5, 1 }, { 101, 9, -3 }, { 102, 9, 2 }, { 103, 0, 0 } }; const rvs::Poc p = rvs::pocOf(7, rows, 100, 110, 104);
      CHECK(p.ok && p.tick == 101 && p.vol == 9 && p.total == 23 && std::fabs(p.pos - 0.1) < 1e-9 && rvs::pocGate(p, true, 104) && !rvs::pocGate(p, true, 100) && !rvs::pocGate(p, false, 100), "[9] POC: the biggest row (the lower one on a tie), its place in the candle, the gate"); }
    { rvs::Params P = rvs::forMarket("NQ"); std::vector<std::string> L = { "MARKET|NQ", "CX_X|2.5", "ABS_X|99", "PROVEN|1", "JUNK|3", "FADE_X|abc", "INIT_X|nan" }; const int n = rvs::parseParams(L, "NQ", P);
      CHECK(n == 3 && P.cxX == 2.5 && P.absX == 10 && P.proven == 1 && P.fadeX == 0.5, "[9] the nightly's params: applied, clamped, garbage ignored (" << n << ")");
      rvs::Params Q = rvs::forMarket("ES"); CHECK(rvs::parseParams(L, "ES", Q) == 0 && Q.cxX == 2.0 && Q.proven == 0, "[9] a params file of another market is not used"); }
    { std::vector<std::pair<int, rvs::Poc> > c; for (int i = 0; i < 5; ++i) { rvs::Poc p; p.ok = true; p.t = 1000 + i * 180; p.vol = 20 * (i + 1); p.dlt = i % 2 ? 5 : -5; c.push_back(std::make_pair(100 + i * 40, p)); }
      std::map<long long, std::vector<rvs::Mark> > mk; rvs::Mark m; m.candle = 1000 + 4 * 180; m.letter = 'C'; m.col = -1; mk[m.candle].push_back(m);
      const std::vector<rvs::PBar> B = rvs::layoutBars(c, mk, 500, 700, 12, 40, 16); bool in = B.size() == 5; for (const rvs::PBar& b : B) in = in && b.top >= 500 + 16 && b.bottom <= 700 && b.letY >= 500 + 16 && !b.num.empty();
      CHECK(in && B[4].top < B[0].top && B[4].marks.size() == 1 && B[0].col == -1 && B[1].col == 1 && B[4].num == "100", "[9] the pane: 5 bars inside the pane under the read line, taller = more lots, numbers on top, the letter on its candle");
      CHECK(!rvs::layoutBars(c, mk, 500, 700, 12, 6, 16).empty() && rvs::layoutBars(c, mk, 500, 700, 12, 6, 16)[0].num.empty(), "[9] narrow candles: bars without numbers (no overlap)"); }
    // ---- [10] robustness: garbage, a feed reset, the timeout ----
    { rvs::Engine G; G.P = rvs::forMarket("ES"); G.onEvent(1000, 7, 0, 100, 1, 1, 0); G.onEvent(1000, 3, 0, -5, 1, 1, 0); G.onEvent(1000, 0, 0, 100, -1, 1, 0); CHECK(G.garbage == 3 && G.fills == 0 && G.book.empty(), "[10] garbage events are counted and ignored");
      Run M; replay(M, V, false, false, T(20, 44, 30)); M.E.reset(); bool ended = false; for (const rvs::Story& x : M.E.ended) if (x.end == rvs::E_RESET) ended = true; CHECK(ended && !M.E.live[0] && M.E.book.empty(), "[10] a feed reset ends the open story (\"feed reset\") and clears the book"); }
    // ---- [10] 0.2.0: the zone bottom variants, pullback absorbed (log-only), the new params keys
    { std::vector<std::string> L; L.push_back("MARKET|ES"); L.push_back("RET_TICKS|2"); L.push_back("MERGE_TICKS|4"); L.push_back("ABS_WAIT|18"); L.push_back("RET_TICKS|99");
      rvs::Params Q = rvs::forMarket("ES"); const int n = rvs::parseParams(L, "ES", Q); CHECK(n == 4 && Q.retTicks == 12 && Q.mergeTicks == 4 && Q.absWait == 18, "[10] RET_TICKS / MERGE_TICKS / ABS_WAIT are read and clamped");
      rvs::Params D = rvs::forMarket("ES"); CHECK(D.retTicks == 0 && D.mergeTicks == 0, "[10] the zone variants are off by default (0.1.1 behaviour)"); }
    { // a second climax 2 ticks under the zone: restarts the story by default, joins it (a zone bottom) with MERGE_TICKS
      for (int pass = 0; pass < 2; ++pass) { rvs::Engine G; G.P = rvs::forMarket("ES"); G.P.mergeTicks = pass ? 4 : 0; long long id = 1, t = 100000;
          for (int k = 0; k < 40; ++k, t += 10000) { G.onEvent(t, rvs::FILL, rvs::ASK, 1000, 5, id++, 0); G.onEvent(t + 1, rvs::FILL, rvs::BID, 1001, 5, id++, 0); }
          G.onEvent(t, rvs::FILL, rvs::ASK, 992, 60, id++, 0); G.onEvent(t + 1, rvs::FILL, rvs::ASK, 991, 80, id++, 0); G.onEvent(t + 2, rvs::FILL, rvs::ASK, 990, 100, id++, 0); t += 10000;      // climax: 240 sold down to 990
          G.onEvent(t, rvs::FILL, rvs::ASK, 988, 120, id++, 0); G.onEvent(t + 1, rvs::FILL, rvs::ASK, 987, 180, id++, 0); t += 10000;                                                       // a bigger one, 2 ticks under the zone
          G.onEvent(t, rvs::FILL, rvs::BID, 989, 1, id++, 0);                                                                                                                                    // closes that block
          if (!pass) CHECK(G.live[0] && G.st[0].id == 2 && G.st[0].merged == 0 && G.ended.size() == 1 && G.ended[0].end == rvs::E_DEEP, "[10] default: the deeper climax ends the story and starts a new one");
          else CHECK(G.live[0] && G.st[0].id == 1 && G.st[0].merged == 1 && G.ended.empty() && G.st[0].zoneLo == 986 && G.st[0].extreme == 987 && G.st[0].burstAll == 540 && G.st[0].absLots >= 300, "[10] MERGE_TICKS 4: it joins the story, the zone widens to the new low, its lots count as traded into the zone"); } }
    { // pullback absorbed: buyers lift into a new high, the POC is at the top, the candle closes under it; the next candle follows through
      rvs::Engine G; G.P = rvs::forMarket("ES"); G.spb = 180; long long id = 1;
      auto cand = [&](long long end, long long lo, long long hi, long long close, long long poc, long long cdlt, bool trade) { if (trade) G.onEvent((end - 10) * 1000, rvs::FILL, rvs::BID, close, 1, id++, 0);
          rvs::Poc p; p.ok = true; p.t = end; p.lo = lo; p.hi = hi; p.close = close; p.tick = poc; p.vol = 10; p.total = 50; p.cdlt = cdlt; p.pos = (double)(poc - lo) / (double)(hi - lo); p.closePos = (double)(close - lo) / (double)(hi - lo); G.onCandle(end, p, close); };
      long long e = 180000; for (int i = 0; i < 12; ++i, e += 180) cand(e, 90, 100 + i % 2, 95, 95, i % 2 ? 10 : -10, true);
      CHECK(G.pulls.empty(), "[10] ordinary candles: no pullback candidate");
      cand(e, 100, 110, 104, 109, 40, true); e += 180;
      CHECK(G.pulls.size() == 1 && G.pulls[0].top && G.pulls[0].conf == -1 && std::fabs(G.pulls[0].dx - 4.0) < 1e-9 && G.pulls[0].ext == 110, "[10] buyers stopped at the high of a pullback: a top candidate, delta 4.0x the normal, waiting for the next candle");
      cand(e, 95, 108, 96, 100, -30, true); e += 180;
      bool f = false; for (const std::string& o : G.out) if (o.find("P|0.4.1|1|top|F|") == 0 && o.find("|conf 1|") != std::string::npos) f = true;
      CHECK(G.pulls.size() == 1 && G.pulls[0].conf == 1 && f && G.marks.empty(), "[10] the next candle makes a lower low and a lower close: follow-through recorded (F, conf 1); log-only, no letter");
      cand(e, 96, 120, 119, 118, 60, true); e += 180;   // closes ABOVE its POC: buyers were not stopped
      cand(e, 100, 125, 110, 124, -50, true); e += 180;  // sellers' delta at a new high: not buyers stopped
      CHECK(G.pulls.size() == 1, "[10] a close above the POC, or a negative delta at the high, is not a candidate");
      cand(e + 3600, 110, 140, 120, 139, 80, false);     // a candle the engine never saw trade (the chart's history after a restart)
      CHECK(G.pulls.size() == 1 && G.cnd.back().t == e + 3600, "[10] a candle fed from the chart's history only builds the normal: no candidate, nothing recorded twice"); }
    // ---- [11] 0.3.0: the hold read (log-only Q lines)
    CHECK(rvs::holdScore(0.8, 2.5, 2.0, 6.0, false) == 5 && rvs::holdScore(0.05, 1.0, 1.0, 2.0, false) == -2 && rvs::holdScore(0.5, 1.0, 1.0, 2.0, false) == 1 && rvs::holdScore(0.2, 2.0, 1.0, 5.0, false) == 1 && rvs::holdScore(0.5, 1.0, 1.0, 2.0, true) == 2, "[11] the hold score: the close back, volume, the move, the range, the failed breakout");
    CHECK(rvs::holdTier(5) == 3 && rvs::holdTier(4) == 3 && rvs::holdTier(3) == 2 && rvs::holdTier(2) == 1 && rvs::holdTier(0) == 1 && rvs::holdTier(-1) == 1 && rvs::holdTier(-2) == 0 && std::string(rvs::holdName(3)) == "strong" && std::string(rvs::holdName(0)) == "weak", "[11] tiers: strong / some / neutral / weak");
    { rvs::Engine G; G.P = rvs::forMarket("ES"); G.spb = 180; long long id = 1;
      auto cand = [&](long long end, long long lo, long long hi, long long close, long long total, bool trade) { if (trade) G.onEvent((end - 10) * 1000, rvs::FILL, rvs::BID, close, 1, id++, 0);
          rvs::Poc p; p.ok = true; p.t = end; p.lo = lo; p.hi = hi; p.close = close; p.tick = lo; p.vol = 10; p.total = total; p.cdlt = 0; p.pos = 0; G.onCandle(end, p, close); };
      long long e = 180000; for (int i = 0; i < 22; ++i, e += 180) cand(e, 90, 100, 95, 100, true);
      size_t q = 0; for (const std::string& o : G.out) if (o[0] == 'Q') ++q; CHECK(q == 0, "[11] candles inside the range: no hold read");
      cand(e, 100, 120, 102, 300, true); e += 180;       // a new high, closed near its low on 3x volume, a 2x range, a run of 3 ranges (it closed above the prior high 100: not a failed breakout): +2 +1 +1 = 4, strong
      std::string l; for (const std::string& o : G.out) if (o[0] == 'Q') l = o;
      CHECK(l.find("Q|0.4.1|1|top|") == 0 && l.find("|score 4|tier strong|") != std::string::npos && l.find("|failed 0|") != std::string::npos && G.lastHoldTier == 3 && G.lastHoldSide == 1 && G.marks.empty(), "[11] a high rejected on heavy volume: strong, recorded, no letter (log-only)");
      cand(e, 101, 130, 130, 100, true); e += 180;       // a new high that closed AT the high on normal volume: weak (-2 + range 2.9x +1 + move +0 = -1 -> neutral? no: move (130-90)/10 = 4 -> -2 +1 = -1)
      l.clear(); for (const std::string& o : G.out) if (o[0] == 'Q') l = o;
      CHECK(l.find("|top|") != std::string::npos && l.find("|score -1|tier neutral|") != std::string::npos && l.find("|failed 0|") != std::string::npos, "[11] a high that closed at the high (a breakout that held at the close): no hold claimed");
      { std::vector<rvs::Cnd> c; for (int i = 0; i < 20; ++i) { rvs::Cnd x; x.t = i; x.lo = 90; x.hi = 100; x.close = 95; x.total = 100; c.push_back(x); }
        rvs::Cnd f; f.t = 20; f.lo = 96; f.hi = 102; f.close = 99; f.total = 100; c.push_back(f);       // pokes 2 ticks over the prior high, closes back under it: cp 0.5 (+1), failed (+1) = 2
        const rvs::Hold h = rvs::holdOf(c, 20); rvs::Cnd g = f; g.close = 101; c[20] = g; const rvs::Hold h2 = rvs::holdOf(c, 20);
        CHECK(h.side == 1 && h.failed && h.score == 2 && h.tier == 1 && h.ext == 102 && h2.side == 1 && !h2.failed && h2.score == -1, "[11] holdOf: a failed breakout of the prior high scores +1; closing through it does not");
        c[20] = f; c[20].hi = 100; CHECK(rvs::holdOf(c, 20).side == 0 && rvs::holdOf(c, 5).side == 0, "[11] holdOf: not a new extreme, or fewer than 20 candles before: no read"); }
      const size_t before = G.out.size(); cand(e + 7200, 60, 131, 61, 900, false);
      CHECK(G.out.size() == before, "[11] a candle from the chart's history is not read"); }
    std::cout << "t_revread (lsRevRead " << rvs::VERSION << "): " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
