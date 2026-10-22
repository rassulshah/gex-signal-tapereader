// build: g++ -std=c++17 -O2 -Wall -Wextra -Werror -I.. replay_po260.cpp -o /tmp/replay_po260
// run:   python3 -I extract.py - nq | /tmp/replay_po260     (stdin: per session one text header line + packed 26-byte events)
// lsHodLod 2.6.0 PRE-OPEN BOOK replay on real MBO: what the line would have said (live at -20 / -10 / -1 min, frozen at the open, final at
// open + 3 min) vs where the first extreme of the RTH actually came (side and time). LEARNING only - a handful of sessions, no claim.
// Also checks the record round trip: an engine restored from the SNAP / IMB lines draws the identical line (no repaint after a restart).
#include "PreOpenLogic.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#pragma pack(push, 1)
struct RawEv { long long ms; signed char a, s; int px, sz; unsigned long long oid; };
#pragma pack(pop)
static_assert(sizeof(RawEv) == 26, "26-byte events");
static long long civil(int y, unsigned m, unsigned d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
int main() {
    std::string hdr; int n = 0, hits = 0, calls = 0, timingHits = 0, timingCalls = 0, fails = 0;
    std::printf("%-14s %-62s %-24s %s\n", "session", "line at open + 3 min (final)", "1st extreme (actual)", "verdict");
    while (std::getline(std::cin, hdr)) {
        char mkt[8] = { 0 }, date[16] = { 0 }; int openMin = 0, hodT = 0, lodT = 0; double atr = 0; long long hodPx = 0, lodPx = 0, cnt = 0;
        if (std::sscanf(hdr.c_str(), "%7s %15s %d %lf %lld %d %lld %d %lld", mkt, date, &openMin, &atr, &hodPx, &hodT, &lodPx, &lodT, &cnt) != 9) { std::fprintf(stderr, "bad header %s\n", hdr.c_str()); return 2; }
        int y = 0; unsigned m = 0, d = 0; std::sscanf(date, "%d-%u-%u", &y, &m, &d);
        const long long dayMs = civil(y, m, d) * 86400000LL;
        po::Engine E; E.mkt = mkt; E.openMin = openMin; E.s20 = po::s20Of(mkt); E.nearTicks = std::max(4, (int)std::lround(atr / 0.25)); E.setDay(dayMs);
        const long long probes[3] = { E.openMs() - 20 * 60000LL, E.openMs() - 10 * 60000LL, E.openMs() - 60000LL };
        std::string live[3]; int pi = 0; bool recSnapOk = false; std::string snapRec;
        for (long long i = 0; i < cnt; ++i) {
            RawEv e; if (!std::cin.read(reinterpret_cast<char*>(&e), sizeof e)) { std::fprintf(stderr, "short read\n"); return 2; }
            const long long ms = dayMs + e.ms;
            while (pi < 3 && ms >= probes[pi]) { live[pi] = po::lineAt(E, probes[pi], 1).s; pi++; }
            const bool hadSnap = E.snapDone;
            E.onEvent(e.a, e.s == 1 ? po::BID : e.s == 0 ? po::ASK : -1, e.px, e.sz, e.oid, ms);
            if (!hadSnap && E.snapDone) { snapRec = po::recLine(E, 180, false); recSnapOk = true; }
        }
        E.tick(E.imbEndMs() + 10000);
        const po::Line fin = po::lineAt(E, E.imbEndMs() + 60000, 1);
        // restart: a fresh engine restored from the record lines draws the same line
        po::Engine R; R.mkt = mkt; R.openMin = openMin; R.setDay(dayMs);
        { po::Snap s; po::Imb im; if (recSnapOk && po::parseRec(snapRec, po::sessionKey(dayMs), mkt, 180, dayMs, s, im) == 1) R.restoreSnap(s); }
        { po::Snap s; po::Imb im; const std::string il = po::recLine(E, 180, true); if (po::parseRec(il, po::sessionKey(dayMs), mkt, 180, dayMs, s, im) == 2) { R.restoreSnap(s); R.restoreImb(im); } }
        const po::Line fr = po::lineAt(R, R.imbEndMs() + 60000, 1);
        if (fr.s != fin.s || fr.col != fin.col) { fails++; std::printf("FAIL restart line differs: '%s' vs '%s'\n", fr.s.c_str(), fin.s.c_str()); }
        const bool lowFirst = lodT < hodT; const int firstT = lowFirst ? lodT : hodT;
        const bool early = firstT < openMin * 60 + E.P.earlyMin * 60;
        char act[64]; std::snprintf(act, sizeof act, "%s %s%s", lowFirst ? "LOW" : "HIGH", po::hm12(firstT / 60).c_str(), early ? " (early)" : "");
        std::string verdict = "no call";
        if (E.snap.call != 0) {
            calls++; const bool ok = (E.snap.call > 0) == lowFirst; hits += ok; verdict = ok ? "side RIGHT" : "side WRONG";
            if (E.snap.early) { timingCalls++; timingHits += ok && early; verdict += ok && early ? ", early RIGHT" : ", early WRONG"; }
        }
        n++;
        std::printf("%-3s %-10s %-62s %-24s %s\n", mkt, date, fin.s.c_str(), act, verdict.c_str());
        std::printf("    live -20m: %s | -10m: %s | -1m: %s\n", live[0].c_str(), live[1].c_str(), live[2].c_str());
        std::printf("    near %d ticks, bids %lld (stay %d%%, big %lld) offers %lld (stay %d%%, big %lld), open buy %lld sell %lld, events %lld\n", E.snap.nearTicks,
                    E.snap.lots[po::BID], po::stayPct(E.snap, po::BID), E.snap.big[po::BID], E.snap.lots[po::ASK], po::stayPct(E.snap, po::ASK), E.snap.big[po::ASK], E.imb.buy, E.imb.sell, E.events);
        std::fflush(stdout);
    }
    std::printf("LEARNING n=%d sessions: calls %d, side right %d of %d; 'early' right %d of %d. Restart identical: %s\n", n, calls, hits, calls, timingHits, timingCalls, fails ? "NO" : "yes");
    return fails ? 1 : 0;
}
