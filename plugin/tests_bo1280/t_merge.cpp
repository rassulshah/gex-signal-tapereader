// t_merge.cpp -- BreakoutLogic.h 1.28.0: a BO / BD merged into the Reversal Read Bk (same direction, its candle or the next); the Rly level
#include "../BreakoutLogic.h"
#include <cstdio>
static int f = 0, ok = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m); } }
int main() {
    bko::Engine E; E.tick = 0.25; const long long T = 3600LL * 24 * 20776 + 9 * 3600;   // 2026-11-19 9 AM local
    for (int i = 0; i < 40; ++i) { bko::In b; b.t = T + (i + 1) * 180; b.ok = true; b.o = 7300; b.h = 7302; b.l = 7298; b.c = 7301; b.vol = 100; E.bars.push_back(b); }
    bba::BoDec D; D.t0 = E.bars[20].t; D.dir = -1; D.fired = true;
    C(!E.bkMerged(D), "no letters file: the BD is drawn as before");
    E.wyOk = true; E.bkDir[E.bars[20].t] = -1; C(E.bkMerged(D), "Bk down on the BD candle: merged");
    E.bkDir.clear(); E.bkDir[E.bars[21].t] = -1; C(E.bkMerged(D), "Bk down on the next candle: merged");
    E.bkDir.clear(); E.bkDir[E.bars[22].t] = -1; C(!E.bkMerged(D), "Bk two candles later: not merged");
    E.bkDir.clear(); E.bkDir[E.bars[20].t] = 1; C(!E.bkMerged(D), "Bk of the opposite direction: not merged");
    E.bkDir[E.bars[20].t] = -1; E.wyOk = false; C(!E.bkMerged(D), "torn / missing file (wyOk false): drawn as before");
    // the Rly level: a decline ending at 7293.00 with PDL 7305.25 inside the candle -> "PDL"; nothing near -> ""
    E.wyOk = true; bko::In& z = E.bars[30]; z.h = 7310; z.l = 7293; z.o = 7308; z.c = 7300;
    wyr::Lvl L; L.px = 29221; L.name = "PFL/PDL"; E.lvKey.push_back(L);
    C(E.rlyLevel(30, -1) == "PDL", "Rly at the PDL candle: PDL");
    E.lvKey.clear(); wyr::Lvl M; M.px = 29000; M.name = "PFL/PDL"; E.lvKey.push_back(M); C(E.rlyLevel(30, -1).empty() || E.rlyLevel(30, -1).find('h') != std::string::npos || E.rlyLevel(30, -1) == "PHL" || E.rlyLevel(30, -1) == "PHH", "no key level near: only an hour level or nothing");
    wyr::Lvl Q; Q.px = 29171; Q.name = "MQ CR"; E.lvKey.push_back(Q); C(E.rlyLevel(30, -1) != "MQ CR", "a MenthorQ name never labels");
    C(E.rlyLevel(-1, 1).empty() && E.rlyLevel(999, 1).empty(), "out of range: none");
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", ok, f); return f;
}
