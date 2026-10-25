// replay_lmz.cpp -- LmtZonesLogic.h (1.2.0: + the FILLING / HELD tags and the log-only FAST / DRIFT / EATEN / STEPPED) replayed on ONE recorded order-by-order session (Databento MBO converted by conv.py:
// "E,t_ct_ms,act,side,tick,size,oid" with NEW 0 / MOD 1 / DEL 2 / FILL 3, and "T,t_ct_ms,tick,lots" trades for the 3-min bars).
// Usage: replay_lmz <events.csv> <MKT> <s20> [gamma ticks comma list]
// Prints zones per kind / side, how many are drawn, how they ended, drawn zones per RTH hour, and checks: one E line per zone, never
// two live zones on one order, a restart after every bar gives the SAME zones as one straight pass (no repaint).
#include "LmtZonesLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace lmz;
struct Ev { int kind; long long t; int act, side; Tick px; long long sz, oid; };
static bool loadEv(const char* p, std::vector<Ev>& v) {
    std::ifstream f(p); std::string ln; while (std::getline(f, ln)) { const std::vector<std::string> c = split(ln, ',');
        Ev e{}; if (c[0] == "E" && c.size() == 7) { e.kind = 0; e.t = std::stoll(c[1]); e.act = std::stoi(c[2]); e.side = std::stoi(c[3]); e.px = std::stoll(c[4]); e.sz = std::stoll(c[5]); e.oid = std::stoll(c[6]); }
        else if (c[0] == "T" && c.size() == 4) { e.kind = 1; e.t = std::stoll(c[1]); e.px = std::stoll(c[2]); e.sz = std::stoll(c[3]); }
        else continue;
        v.push_back(e); }
    return !v.empty();
}
// run; restartEvery: re-create the engine from its own record after every closed bar (the IRT restart path)
static Engine run(const std::vector<Ev>& ev, const std::string& m, long long s20, const std::vector<Tick>& g, bool restartEach, std::vector<std::string>& rec) {
    Engine E; E.market = m; E.R = ruleFor(m); E.setGamma(g);
    Bar cur; bool have = false; long long end = 0;
    auto close = [&]() {
        E.closeBar(cur, s20); for (auto& r : E.rows) rec.push_back(r); E.rows.clear();
        if (restartEach) {   // a new engine: the record + the same book (the live feed carries on) + the same bars
            Engine N; N.market = m; N.R = E.R; N.setGamma(g); N.book = E.book; for (auto& kv : N.book) kv.second.zone = -1;
            N.W = E.W; N.feedStartMs = E.feedStartMs; N.lastT = E.lastT; N.lastBarMs = E.lastBarMs; N.prints = E.prints; N.restore(rec); E = N;
        }
    };
    for (const Ev& e : ev) {
        if (have && e.t >= end) { close(); have = false; }
        if (e.kind == 1) { if (!have) { have = true; end = (e.t / 180000) * 180000 + 180000; cur = Bar(); cur.endMs = end; cur.o = cur.h = cur.l = cur.c = e.px; }
            cur.h = std::max(cur.h, e.px); cur.l = std::min(cur.l, e.px); cur.c = e.px; continue; }
        E.onEvent(e.act, e.side, e.px, e.sz, e.oid, e.t);
    }
    if (have) close();
    E.endSession(end + 1); for (auto& r : E.rows) rec.push_back(r); E.rows.clear();
    return E;
}
int main(int argc, char** argv) {
    if (argc < 4) { std::printf("usage: replay_lmz events.csv MKT s20 [gamma ticks]\n"); return 2; }
    std::vector<Ev> ev; if (!loadEv(argv[1], ev)) { std::printf("no events\n"); return 2; }
    std::vector<Tick> g; if (argc > 4) for (auto& s : split(argv[4], ',')) if (!s.empty()) g.push_back(std::stoll(s));
    std::vector<std::string> rec1, rec2;
    const Engine A = run(ev, argv[2], std::stoll(argv[3]), g, false, rec1);
    int fails = 0;
    std::printf("%s: %zu events, %zu bars, %zu zones\n", argv[2], ev.size(), A.W.bars.size(), A.zones.size());
    for (int s = 0; s < 2; ++s) { std::printf("  %s:", s ? "offers" : "bids  "); for (int k = 0; k < K_N; ++k) std::printf(" %s %lld", kindCode(k), A.tally.made[s][k]); std::printf("\n"); }
    std::printf("  drawn %lld | ends:", A.tally.drawn); for (int e = 1; e <= NEARED; ++e) std::printf(" %s %lld", endCode(e), A.tally.ends[e]); std::printf("\n");
    int conf = 0, filledAny = 0, dFar = 0, dNear = 0; std::map<int, int> perHour; for (const Zone& z : A.zones) { if (z.conf) ++conf; if (z.traded > 0) ++filledAny; if (z.drawn) { perHour[(int)((z.t0 / 3600000) % 24)]++; if (z.kind == K_FAR) ++dFar; else ++dNear; } }
    std::printf("  drawn: %d zones + %d far lines\n", dNear, dFar);
    std::printf("  gamma diamonds %d, zones with LMT F fills %d | drawn per CT hour:", conf, filledAny); int mx = 0; for (auto& kv : perHour) { std::printf(" %02d:%d", kv.first, kv.second); mx = std::max(mx, kv.second); } std::printf("\n");
    for (const Zone& z : A.zones) if (z.drawn && z.kind != K_FAR) std::printf("    %s %s %lld @ %lld  %s -> %s %s  traded %lld%s\n", z.side ? "offer" : "bid", kindCode(z.kind), z.size0, (long long)z.segs[0].px, clock12(z.t0).c_str(), endCode(z.end), clock12(z.t1).c_str(), z.traded, z.conf ? "  <>" : "");
    // (1.2.0) the state tags: per session and per RTH, drawn zones only and all (non-far) zones
    {   long long nz = 0, nzD = 0, cont = 0, contD = 0, fill = 0, fillD = 0, held = 0, heldD = 0, pulled = 0, pulledD = 0, pulledBefore = 0, rth = 0, rthHeld = 0, rthFill = 0;
        for (const Zone& z : A.zones) { if (z.kind == K_FAR) continue; const bool d = z.drawn; ++nz; if (d) ++nzD;
            if (z.contactBarMs) { ++cont; if (d) ++contD; } if (z.tagFillMs) { ++fill; if (d) ++fillD; } if (z.tagHeldMs) { ++held; if (d) ++heldD; }
            if (z.end == PULLED) { ++pulled; if (d) ++pulledD; if (!z.contactBarMs) ++pulledBefore; }
            const int mod = (int)((z.t0 / 60000) % 1440); if (mod > 510 && mod <= 900) { ++rth; if (z.tagHeldMs) ++rthHeld; if (z.tagFillMs) ++rthFill; } }
        std::printf("  TAGS (non-far zones %lld, drawn %lld): contacted %lld (drawn %lld) | FILLING %lld (drawn %lld) | HELD %lld (drawn %lld) | PULLED %lld (drawn %lld; before contact %lld)\n",
                    nz, nzD, cont, contD, fill, fillD, held, heldD, pulled, pulledD, pulledBefore);
        std::printf("  RTH zones %lld: FILLING %lld HELD %lld | of contacted: FILLING %.0f%% HELD %.0f%%\n", rth, rthFill, rthHeld, cont ? 100.0 * fill / cont : 0.0, cont ? 100.0 * held / cont : 0.0);
        std::printf("  LOG ONLY: arrivals FAST %lld MID %lld DRIFT %lld | EATEN %lld | STEPPED %lld\n", A.tally.fast, A.tally.mid, A.tally.drift, A.tally.eaten, A.tally.stepped);
        for (const Zone& z : A.zones) if (z.drawn && z.kind != K_FAR && (z.tagHeldMs || z.tagFillMs)) std::printf("    %s %s %lld @ %lld  %s -> %s %s  FILLING %s HELD %s traded %lld\n", z.side ? "offer" : "bid", kindCode(z.kind), z.size0, (long long)z.segs[0].px, clock12(z.t0).c_str(), endCode(z.end), clock12(z.t1).c_str(), z.tagFillMs ? clock12(z.tagFillMs).c_str() : "-", z.tagHeldMs ? clock12(z.tagHeldMs).c_str() : "-", z.traded);
    }
    int eLines = 0; for (auto& r : rec1) if (r[0] == 'E') ++eLines;
    if (eLines != (int)A.zones.size()) { ++fails; std::printf("FAIL: %d E lines for %zu zones\n", eLines, A.zones.size()); }
    // no repaint: restart after every bar
    const Engine B = run(ev, argv[2], std::stoll(argv[3]), g, true, rec2);
    bool same = A.zones.size() == B.zones.size();
    for (size_t i = 0; same && i < A.zones.size(); ++i) { const Zone& a = A.zones[i]; const Zone& b = B.zones[i];
        same = a.oid == b.oid && a.kind == b.kind && a.t0 == b.t0 && a.t1 == b.t1 && a.end == b.end && a.segs.size() == b.segs.size() && a.traded == b.traded && a.drawn == b.drawn
            && a.tagFillMs == b.tagFillMs && a.tagHeldMs == b.tagHeldMs; if (!same) std::printf("  differ at zone %zu\n", i); }
    if (!same) { ++fails; std::printf("FAIL: restart-after-every-bar differs (%zu vs %zu zones)\n", A.zones.size(), B.zones.size()); }
    else std::printf("  restart after every bar: identical zones and tags (no repaint)\n");
    // (1.2.0) the IRT restart path: a FRESH engine reads the whole record, then every bar comes again -> no tag / end / B line decided again
    {   Engine R; R.market = argv[2]; R.R = ruleFor(argv[2]); R.setGamma(g); long long bad = -1; R.restore(rec1, &bad);
        for (const Bar& b : A.W.bars) R.closeBar(b, std::stoll(argv[3]));
        int newT = 0, newE = 0, newB = 0; for (auto& r : R.rows) { if (r[0] == 'T') ++newT; if (r[0] == 'E') ++newE; if (r[0] == 'B') ++newB; }
        bool tagsSame = R.zones.size() == A.zones.size();
        for (size_t i = 0; tagsSame && i < A.zones.size(); ++i) tagsSame = R.zones[i].tagFillMs == A.zones[i].tagFillMs && R.zones[i].tagHeldMs == A.zones[i].tagHeldMs && R.zones[i].end == A.zones[i].end;
        if (bad != 0 || newT || newE || newB || !tagsSame) { ++fails; std::printf("FAIL: IRT restart path - bad %lld, new T %d E %d B %d, tags same %d\n", bad, newT, newE, newB, tagsSame ? 1 : 0); }
        else std::printf("  IRT restart path (fresh engine, record read, all %zu bars again): 0 new tags / ends, tags identical\n", A.W.bars.size());
    }
    std::printf("replay %s: %s\n", argv[2], fails ? "FAILED" : "ok"); return fails ? 1 : 0;
}
