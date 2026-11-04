// test_blb_replay.cpp -- BookLevelBox 1.0.0 (lsBookFlow 1.34.0) PARITY with the nightly lra.book_level_combo 1.1.1 on the real ES
// 2026-10-30 order feed (06:25 - 13:20 CT, 10.6 M events, Rassul's IRT record), the six prototype cases (G 7630 held, PWL 7648.50 broke
// +84 s, PS 7600 broke +217 s, VWAP 7651 resistance broke +18 s, VWAP -1 band support, Pb test of the 12:33 BD).
//   1. the per-second inputs (durable defender adds / rested pulls in the zone, attacker adds / pulls, behind) == the replay's arrays
//   2. the smoothed reads at EVERY second -270 .. +300 (incremental walk) == book_level_combo.smooth(), word + colour + ratio
//   3. the live path (away / arrival rule on the trades, pick(), evaluate() 5 s late) on the same day: arrival times and the box at
//      the prototype's snapshots (+60 s etc.)
//   4. CPU: ns per event of the engine
// Build: g++ -std=c++17 -O2 -Wall -Wextra -Werror -I.. test_blb_replay.cpp -o t && ./t events.bin ref.txt
#include "../BookLevelBox.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>
#include <string>
#include <vector>
#pragma pack(push, 1)
struct EvRec { long long t; signed char act, side; int pt, sz; long long oid; int bid, ask; double px; };
#pragma pack(pop)
static int P = 0, F = 0;
static void check(bool c, const std::string& m) { if (c) ++P; else { ++F; if (F < 60) std::printf("FAIL %s\n", m.c_str()); } }
struct RefCase { std::string key; long long tArr = 0, lo = 0, hi = 0; int ds = 0; std::vector<double> arr[5]; std::vector<blb::Read> sm[3]; };
int main(int argc, char** argv) {
    if (argc < 3) { std::printf("usage: t events.bin ref.txt\n"); return 2; }
    std::vector<RefCase> R;
    {   std::ifstream f(argv[2]); int n = 0; f >> n;
        for (int i = 0; i < n; ++i) {
            RefCase c; f >> c.key >> c.tArr >> c.lo >> c.hi >> c.ds;
            for (int k = 0; k < 5; ++k) { c.arr[k].resize(601); for (int j = 0; j < 601; ++j) f >> c.arr[k][(size_t)j]; }
            for (int k = 0; k < 571; ++k) for (int q = 0; q < 3; ++q) { blb::Read r; f >> r.word >> r.x >> r.col; c.sm[q].push_back(r); }
            R.push_back(c);
        } }
    check(R.size() == 6, "six reference cases");
    FILE* fp = std::fopen(argv[1], "rb"); if (!fp) { std::printf("no events\n"); return 2; }
    std::vector<EvRec> E; { EvRec e; while (std::fread(&e, sizeof e, 1, fp) == 1) E.push_back(e); } std::fclose(fp);
    std::printf("events %zu\n", E.size());
    // ---- pass 1: the engine's arrays + the smoothing walk per case, taken when the replay reaches anchor + 306 s ----
    blb::Engine G; G.market = "ES";
    std::vector<bool> done(R.size(), false);
    long long maxAbsD = 0; int arrMismatch = 0, smMismatch = 0;
    const auto t0 = std::chrono::steady_clock::now();
    // the nightly replays on a price grid (book_level_combo.replay: the cases' lowest zone - 1000 ticks .. highest + 1000) and SKIPS every
    // event priced off it - incl. a MOD that moves an order off the grid, so that order keeps counting at its old price (one ES order
    // on 10-30). The engine has no grid; to compare like for like the parity pass feeds it the nightly's input (gridSkip counts them).
    long long gLo = LLONG_MAX, gHi = LLONG_MIN; for (const RefCase& c : R) { gLo = std::min(gLo, c.lo - 1000); gHi = std::max(gHi, c.hi + 1000); }
    long long gridSkip = 0;
    for (size_t i = 0; i < E.size(); ++i) {
        const EvRec& e = E[i];
        if (e.pt < gLo || e.pt >= gHi) { ++gridSkip; if (e.act == 3) { /* the nightly skips the print too */ } continue; }
        G.onEvent(e.t, e.act, e.side, e.pt, e.sz, e.oid, e.bid, e.ask);
        for (size_t c = 0; c < R.size(); ++c) {
            if (done[c] || G.cur < R[c].tArr + 300 + 6) continue;
            done[c] = true;
            blb::Engine::Case k; k.li = (int)c; k.lo = R[c].lo; k.hi = R[c].hi; k.ds = R[c].ds; k.anchor = R[c].tArr; k.arrived = true;
            blb::Engine::Series S; G.series(k, k.anchor - 300, k.anchor + 300, S);
            const std::vector<double>* mine[5] = { &S.dA, &S.dP, &S.aA, &S.aP, &S.bh };
            static const char* const NM[5] = { "d_add", "d_pull", "a_add", "a_pull", "behind" };
            for (int a = 0; a < 5; ++a) { int bad = 0; long long first = -999; for (int j = 0; j < 601; ++j) { const double d = (*mine[a])[(size_t)j] - R[c].arr[a][(size_t)j]; if (d != 0) { ++bad; std::printf("   diff %s %s k %d mine %.0f ref %.0f\n", R[c].key.c_str(), NM[a], j - 300, (*mine[a])[(size_t)j], R[c].arr[a][(size_t)j]); if (first == -999) first = j - 300; if (std::llabs((long long)d) > maxAbsD) maxAbsD = std::llabs((long long)d); } }
                check(bad == 0, R[c].key + " " + NM[a] + " equals the replay (" + std::to_string(bad) + " seconds differ, first k " + std::to_string(first) + ")"); arrMismatch += bad; }
            // the smoothed reads, incrementally from -270 (rebuild at -270, then one second at a time)
            G.sm = blb::Engine::Smooth(); G.cache.clear();
            int bad = 0; std::string firstBad;
            for (long long kk = -270; kk <= 300; ++kk) {
                G.advance(k, kk);
                for (int q = 0; q < 3; ++q) {
                    const blb::Read& a = G.sm.cur[q]; const blb::Read& b = R[c].sm[q][(size_t)(kk + 270)];
                    const bool ok = a.word == b.word && a.col == b.col && std::fabs(a.x - b.x) <= 1e-6 * std::max(1.0, std::fabs(b.x));
                    if (!ok) { ++bad; if (firstBad.empty()) { char t[160]; std::snprintf(t, sizeof t, "k %lld line %d mine %d %.4f %d ref %d %.4f %d", kk, q, a.word, a.x, a.col, b.word, b.x, b.col); firstBad = t; } }
                }
            }
            check(bad == 0, R[c].key + " smoothed reads -270..+300 equal smooth() (" + std::to_string(bad) + " differ; " + firstBad + ")"); smMismatch += bad;
            G.sm = blb::Engine::Smooth(); G.cache.clear();
        }
    }
    const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / (double)E.size();
    for (size_t c = 0; c < R.size(); ++c) check(done[c], R[c].key + " reached");
    std::printf("arrays: %d seconds differ (max |diff| %lld); smoothed: %d reads differ; off-grid events skipped as the nightly %lld\n", arrMismatch, maxAbsD, smMismatch, gridSkip);
    // ---- pass 2: the LIVE path - the levels, the away / arrival rule, evaluate() each second (5 s late) ----
    blb::Engine L; L.market = "ES";
    std::vector<blb::Level> lv;
    auto sym = [&](const std::string& n, int type, double px) { blb::Level l; l.name = n; l.type = type; const long long c = (long long)std::llround(px / 0.25); l.lo = c - 4; l.hi = c + 4; lv.push_back(l); };
    sym("PWL", blb::L_KEY, 7648.50); sym("G 7630", blb::L_MQ, 7630.0); sym("PS 7600", blb::L_MQ, 7600.0); sym("VWAP", blb::L_VWAP, 7651.0); sym("VWAP -1", blb::L_VWAP, 7641.25);
    { blb::Level l; l.name = "Pb BD 12:33"; l.type = blb::L_PB; l.fixedZone = true; l.lo = 30433; l.hi = 30447; l.tFrom = R[5].tArr - 600; l.tTo = 0; lv.push_back(l); }
    L.setLevels(lv);
    long long lastE = -1; long long shownSecs = 0, evalCalls = 0, liveCmp = 0, liveBad = 0, outsideBand = 0; std::set<std::string> liveKeys;
    auto shownPx = [](const blb::Engine& G2, long long x, const blb::Engine::Shown& sh) { const blb::Engine::Sec* q = G2.get(x); if (!q || !q->pxOk) return 999LL; const long long p = q->px; return p < sh.c.lo ? sh.c.lo - p : (p > sh.c.hi ? p - sh.c.hi : 0LL); };
    std::vector<std::pair<long long, blb::Engine::Shown> > snaps;
    const long long rthOpen = R[0].tArr - (R[0].tArr % 86400) + 8 * 3600 + 30 * 60; bool resetDone = false;
    double evalNs = 0;
    for (size_t i = 0; i < E.size(); ++i) {
        const EvRec& e = E[i];
        if (!resetDone && e.t >= rthOpen - 3600) { L.resetAway(); resetDone = true; }
        L.onEvent(e.t, e.act, e.side, e.pt, e.sz, e.oid, e.bid, e.ask);
        const long long Ev = L.cur - blb::MINREST;
        if (Ev > lastE && L.cur > 0) {
            for (long long x = std::max(lastE + 1, Ev - 3); x <= Ev; ++x) {
                const auto a = std::chrono::steady_clock::now();
                const blb::Engine::Shown sh = L.evaluate(x); ++evalCalls;
                evalNs += std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - a).count();
                if (sh.on) ++shownSecs;
                if (sh.on && sh.c.arrived) for (const RefCase& c : R) {
                    if (sh.c.anchor != c.tArr || sh.c.lo != c.lo || sh.c.hi != c.hi || sh.k < -270 || sh.k > 300) continue;
                    ++liveCmp;
                    for (int q = 0; q < 3; ++q) { const blb::Read& a = sh.r[q]; const blb::Read& b = c.sm[q][(size_t)(sh.k + 270)];
                        if (!(a.word == b.word && a.col == b.col && std::fabs(a.x - b.x) <= 1e-6 * std::max(1.0, std::fabs(b.x)))) { ++liveBad; if (liveBad < 5) std::printf("   live diff %s k %lld line %d\n", c.key.c_str(), sh.k, q); } }
                    liveKeys.insert(c.key);
                }
                if (sh.on) { const long long d = shownPx(L, x, sh); if (d > blb::BAND) ++outsideBand; }
                for (const RefCase& c : R) for (long long kk : { -30LL, 60LL, 150LL }) if (x == c.tArr + kk) snaps.push_back(std::make_pair(x, sh));
            }
            lastE = Ev;
        }
    }
    std::printf("pending overflow %lld\n", G.pendOver); std::printf("live compared with the nightly: %lld seconds, %lld differ\n", liveCmp, liveBad); std::printf("live: %lld evaluations, %lld s shown, %.1f us per evaluation, %lld arrivals, smoothing rebuilds %lld\n", evalCalls, shownSecs, evalNs / std::max(1LL, evalCalls) / 1000.0, L.arrivals, L.smoothRebuilds);
    check(liveCmp > 300 && liveBad == 0, "live box (5 s late) == the nightly's smoothed read at every shown second of a case with the same arrival: " + std::to_string(liveCmp) + " seconds, " + std::to_string(liveBad) + " differ");
    check(liveKeys.count("held") && liveKeys.count("ambig") && liveKeys.count("band") && liveKeys.count("pb"), "live arrivals = the prototype's for G 7630, PS 7600, VWAP -1, Pb (" + std::to_string(liveKeys.size()) + ")");
    check(outsideBand == 0, "the box never shows with price more than 4 ticks from the zone (" + std::to_string(outsideBand) + ")");
    for (const auto& s : snaps) {
        const blb::BoxText t = blb::boxText(s.second, L.P);
        std::printf("  %lld: %s | arrived %d anchor %lld k %lld | %s | %s | %s\n", s.first, t.level.c_str(), s.second.c.arrived ? 1 : 0, s.second.c.anchor, s.second.k, t.part[0].c_str(), t.part[1].c_str(), t.part[2].c_str());
    }
    // every live arrival vs the prototype's t_arr
    for (const blb::Engine::LState& l : L.L) std::printf("  level %s arrived %d tArr %lld ds %d\n", l.lv.name.c_str(), l.arrived ? 1 : 0, l.tArr, l.ds);
    std::printf("CPU: %.0f ns per event (engine only, -O2)\n", ns);
    std::printf("blb replay: %d passed, %d failed\n", P, F); return F ? 1 : 0;
}
