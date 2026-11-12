// t_revstory.cpp -- lsRevStory 0.1.0 logic tests on his own ES order recording (2026-11-11 20:00-21:00 CT, revstory_es1111.txt).
// Build (in plugin\tests_revstory):  g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_revstory.cpp -o t && ./t
#include "../RevStoryLogic.h"
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
    const std::vector<Ev> V = load(argc > 1 ? argv[1] : "revstory_es1111.txt");
    CHECK(V.size() > 40000, "fixture loaded (" << V.size() << " events)");
    CHECK(std::string(rvs::VERSION) == "0.1.0", "version 0.1.0");
    Run R; replay(R, V); const rvs::Engine& E = R.E;
    CHECK(E.sideMismatch == 0 && E.fills == 7307 && E.garbage == 0, "every fill's aggressor side agrees with the resting order's side (" << E.fills << " fills, " << E.sideMismatch << " mismatches)");
    // ---- [1] tonight's reversal: the 8:43 PM selling climax, absorbed, fading, flushed, buyers ----
    const rvs::Story* s = storyAt(E, T(20, 43, 20));
    CHECK(s && s->bottom && s->shown && s->end == rvs::E_COMPLETE, "[1] the 8:43:10-8:43:20 PM block starts a bottom story that completes");
    if (s) {
        CHECK(s->burst == 119 && s->opp == 40 && s->bigOrder == 50, "[1] climax: 119 lots sold in 10 s vs 40 bought, the biggest market order 50 lots (" << s->burst << " / " << s->opp << " / " << s->bigOrder << ")");
        CHECK(s->anchor == 30460 && s->shownSz >= 30 && s->shownSz <= 38, "[1] anchor 7615.00 with 30-38 lots showing on the bid before the first hit (" << s->anchor * 0.25 << ", " << s->shownSz << ")");
        CHECK(s->tA == T(20, 44, 10) * 1000 && s->absR >= 1.5, "[1] absorbed at 8:44:10 PM (63 lots into ~32 showing)");
        CHECK(s->fading && s->fadeList.size() >= 2 && s->fadeList.back() < s->fadeList[s->fadeList.size() - 2], "[1] fading: each return sells less (" << readOf(E, *s) << ")");
        CHECK(s->tE > s->tA && s->flushLots > 0 && (double)s->flushLots <= 0.6 * 119 && s->extreme == 30455, "[1] exhausted: the new low 7613.75 on little volume (" << s->flushLots << " lots)");
        CHECK(s->tI == T(20, 47, 10) * 1000 && s->initLots == 47 && s->initOpp == 3, "[1] initiation: 47 lots bought vs 3 sold in the 8:47:00 block");
        const std::string r = readOf(E, *s);
        CHECK(r.find("Sell climax 119 (50-lot) > Absorbed ") == 0 && r.find("at 7615 > Fading ") != std::string::npos && r.find("Flush on ") != std::string::npos && r.find("Buyers 47") != std::string::npos, "[1] the read line: " << r);
    }
    // ---- [2] "i dont understand how you marked the 63 as a climax": the 8:29 PM burst ran through prices -> no letters ----
    const rvs::Story* q = storyAt(E, T(20, 29, 10));
    CHECK(q && q->burst == 120 && q->tA == 0 && q->end != rvs::E_COMPLETE, "[2] the 8:29 PM burst (120 lots in 10 s) is NOT absorbed: " << (q ? rvs::endName(q->end) : "missing"));
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
      Run N; replay(N, V, false, false, T(20, 44, 30)); const rvs::Story* n = N.E.newest(T(20, 44, 30) * 1000);
      CHECK(n && n->step == rvs::S_ABSORBED && readOf(N.E, *n).find("> Absorbed ") != std::string::npos && readOf(N.E, *n).find("buyers?") != std::string::npos && N.letters.empty(), "[6] at 8:44:30 PM absorbed, no letter yet (the candle is still open): " << (n ? readOf(N.E, *n) : "none")); }
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
          && rvs::hourlyText(1, 1, 30467, 30485, false, 30490, 30486, 30492, 4, 0.25) == "Hrly up, going with it" && rvs::hourlyText(-1, 1, 30467, 30440, true, 30450, 30448, 30452, 4, 0.25) == "Hrly dn (rotation up), trend broken" && rvs::hourlyText(0, 0, 0, 0, false, 1, 1, 1, 4, 0.25) == "Hrly none",
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
    std::cout << "t_revstory (lsRevStory " << rvs::VERSION << "): " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
