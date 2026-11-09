// t_check.cpp - the filter check: replays a recorded MBO session from stdin (export_bin records), runs TopBook once a second,
// and scores every SHOWN level and every big-enough REMOVED level by the same touch rule (held = 0.5 ATR back away before 0.25 ATR through)
#include "../TopBook.h"
#include <cstdio>
#include <cstdint>
#include <map>
#include <string>
using namespace tbk;
struct Tr { int grp; int side; long long px; long long t0; bool touched = false; long long tt = 0; };
int main(int argc, char** argv) {
    const std::string mkt = argc > 1 ? argv[1] : "ES";
    Engine E; Params p; s20For(mkt, p); E.setS20(p.s20Rth, p.s20On);
    const long long CT = -6LL * 3600 * 1000;
    const char* G[] = { "shown (all)", "shown, many orders", "shown, mostly one order ?", "shown, S20 order", "removed: inside 2 ticks", "removed: young < 30 s", "removed: quoting", "removed: pulled on approach earlier" };
    long long n[8] = {0}, held[8] = {0}, broke[8] = {0}; std::map<std::pair<int, long long>, Tr> T; std::map<std::pair<int, long long>, long long> lastDone;
    struct __attribute__((packed)) R { int8_t act, side; int32_t px, sz; uint64_t oid; int64_t t; } r;
    long long next = -1;
    auto track = [&](int grp, int side, long long px, long long now) { const auto k = std::make_pair(grp * 2 + side, px);
        if (T.count(k)) return; auto d = lastDone.find(k); if (d != lastDone.end() && now - d->second < 600000) return; Tr x; x.grp = grp; x.side = side; x.px = px; x.t0 = now; T[k] = x; };
    while (std::fread(&r, sizeof r, 1, stdin) == 1) {
        In e; e.act = r.act; e.side = r.side; e.px = r.px; e.sz = r.sz; e.oid = (long long)r.oid; e.tMs = r.t;
        if (next < 0) next = r.t;
        if (r.t >= next) { next = r.t + 1000;
            const long long s = ((r.t + CT) / 1000) % 86400; const bool rth = s >= 8 * 3600 + 30 * 60 && s < 15 * 3600; E.setRth(rth);
            const std::vector<Lvl> v = E.evaluate(r.t);
            if (rth) {
                for (const Lvl& L : v) { track(0, L.side, L.px, r.t); track(L.single ? 2 : 1, L.side, L.px, r.t); if (L.s20) track(3, L.side, L.px, r.t); }
                for (const Lvl& L : E.lastRemoved()) track(3 + L.why, L.side, L.px, r.t);
            }
            const long long lp = E.last(); const double atr = E.atr();
            for (auto it = T.begin(); it != T.end();) { Tr& x = it->second;
                const double away = x.side == 0 ? (double)(lp - x.px) : (double)(x.px - lp);
                if (!x.touched) { if (away <= 0) { x.touched = true; x.tt = r.t; } else if (r.t - x.t0 > 3600000) { it = T.erase(it); continue; } ++it; continue; }
                int out = 0; if (-away >= 0.25 * atr) out = 2; else if (away >= 0.5 * atr) out = 1; else if (r.t - x.tt > 3600000) out = 3;
                if (out) { if (out < 3) { ++n[x.grp]; (out == 1 ? held : broke)[x.grp]++; } lastDone[it->first] = r.t; it = T.erase(it); continue; }
                ++it; }
        }
        E.feed(e); }
    std::printf("%s RTH (touched levels: held = 0.5 ATR back away before 0.25 ATR through)\n", mkt.c_str());
    for (int g = 0; g < 8; ++g) std::printf("  %-38s n %5lld  held %5.1f%%\n", G[g], n[g], n[g] ? 100.0 * held[g] / n[g] : 0.0);
    std::printf("  %s\n", E.statusLine().c_str());
    return 0;
}
