// replay_oh.cpp -- DealerHedgeLogic.h replayed on recorded order-by-order sessions (Databento MBO -> conv_oh.py: "E,t_ct_ms,act,side,
// tick,size,oid" + "T,t_ct_ms,tick,lots"), with the MenthorQ level set in force before the open (levels_oh.py: name,px,gexM,sign,zero,prio).
// Clock = whole seconds (IRT's DEPTH_ORDER stamp), 3-min bars from the trades, ATR(14) of the closed bars.
// usage: replay_oh <MKT> <tick> <events1.csv> <levels1.txt> [<events2.csv> <levels2.txt> ...]   (sessions in order: slot norms carry over)
// For each session: zones lit at the levels (RTH 8:30-15:00 CT and all), each one listed, and the NULL - the same detector at level
// +2 ATR and -2 ATR (average); a restart from the record after every bar must give byte-identical Z lines (no repaint).
#include "DealerHedgeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace dh;
struct Ev { int kind; long long t; int act, side; Tick px; long long sz, oid; };
static bool loadEv(const char* p, std::vector<Ev>& v) {
    std::ifstream f(p); std::string ln;
    while (std::getline(f, ln)) {
        const std::vector<std::string> c = split(ln, ','); Ev e{};
        if (c[0] == "E" && c.size() == 7) { e.kind = 0; e.t = std::stoll(c[1]); e.act = std::stoi(c[2]); e.side = std::stoi(c[3]); e.px = std::stoll(c[4]); e.sz = std::stoll(c[5]); e.oid = std::stoll(c[6]); }
        else if (c[0] == "T" && c.size() == 4) { e.kind = 1; e.t = std::stoll(c[1]); e.px = std::stoll(c[2]); e.sz = std::stoll(c[3]); }
        else continue;
        e.t = (e.t / 1000) * 1000;                       // IRT stamps whole seconds
        v.push_back(e);
    }
    // the file starts with the exchange's book snapshot, stamped with each order's ORIGINAL time (days earlier): those orders are
    // placed at the first trade's time, so the session / clock start at the replay window
    long long t0 = -1; for (const Ev& e : v) if (e.kind == 1) { t0 = e.t; break; }
    if (t0 > 0) for (Ev& e : v) if (e.t < t0 - 6 * 3600000LL) e.t = t0;
    return !v.empty();
}
static std::vector<Level> loadLv(const char* p) {
    std::vector<Level> v; std::ifstream f(p); std::string ln;
    while (std::getline(f, ln)) { const std::vector<std::string> c = split(ln, ','); if (c.size() < 6) continue;
        Level L; L.name = c[0]; L.px = std::stod(c[1]); L.gexM = std::stod(c[2]) > 0 ? std::stod(c[2]) : -1; L.sign = c[3] == "-" ? 0 : c[3][0]; L.zero = c[4] == "1"; L.prio = std::stoi(c[5]); v.push_back(L); }
    return v;
}
struct Out { Engine E; std::vector<std::string> rec; int barsN = 0; bool sameRestart = true; };
static Out run(const std::vector<Ev>& ev, const std::string& m, double tick, const std::vector<Level>& lv, double shift, const std::map<int, SlotNorm>& norms, bool restartEach) {
    Out o; Engine& E = o.E; E.market = m; E.tick = tick; E.P = defaultsFor(marketIdx(m)); E.P.nullShiftAtr = shift; E.slotNorm = norms;
    E.newSession(sessionOf(ev.front().t)); E.setLevels(lv);
    std::vector<double> H, L, C; double h = 0, l = 0, c = 0; long long end = 0; bool have = false;
    for (const Ev& e : ev) {
        if (have && e.t >= end) {                       // the 3-min bar closes
            H.push_back(h); L.push_back(l); C.push_back(c); have = false; ++o.barsN;
            const double a = atrOf(H, L, C, E.P.atrLen); if (a > 0) E.setAtr(a);
            for (auto& r : E.rows) o.rec.push_back(r);
            E.rows.clear();
            if (restartEach) {                          // IRT restart: the zones come back from the record only
                Engine R = E; R.zones.clear(); R.restore(o.rec); R.setLevels(lv);
                for (size_t i = 0; i < E.zones.size(); ++i) if (E.zones[i].state != HIDDEN) {
                    bool f = false; for (const Zone& z : R.zones) if (z.key == E.zones[i].key) { f = true; if (R.zLine(z) != E.zLine(E.zones[i])) o.sameRestart = false; }
                    if (!f) o.sameRestart = false;
                }
                // carry the live state (a restart mid-burst keeps deciding from the record + the live feed)
                for (Zone& z : R.zones) for (const Zone& x : E.zones) if (x.key == z.key && x.state != HIDDEN) { z.state = x.state; z.liveSide = x.liveSide; z.lastActMs = x.lastActMs; z.liveFromMs = x.liveFromMs; z.episodes = x.episodes; }
                E = R;
            }
        }
        if (e.kind == 1) {
            const double px = e.px * tick;
            if (!have) { have = true; end = (e.t / 180000) * 180000 + 180000; h = l = c = px; }
            h = std::max(h, px); l = std::min(l, px); c = px; E.trade(e.px);
        } else E.onEvent(e.act, e.side, e.px, e.sz, e.oid, e.t);
        E.evaluate(e.t, have ? end : (e.t / 180000) * 180000 + 180000);
    }
    for (auto& r : E.rows) o.rec.push_back(r);
    E.rows.clear();
    return o;
}
static bool rth(long long ms) { const int m = minuteOfDay(ms); return m >= 510 && m < 900; }
int main(int argc, char** argv) {
    if (argc < 5 || (argc - 3) % 2) { std::printf("usage: replay_oh MKT tick ev1 lv1 [ev2 lv2 ...]\n"); return 2; }
    const std::string m = argv[1]; const double tick = std::stod(argv[2]);
    std::map<int, SlotNorm> norms; std::vector<std::string> normHist; int fails = 0;
    long long totL = 0, totLR = 0; double totN = 0, totNR = 0;
    for (int a = 3; a + 1 < argc; a += 2) {
        std::vector<Ev> ev; if (!loadEv(argv[a], ev)) { std::printf("no events %s\n", argv[a]); return 2; }
        const std::vector<Level> lv = loadLv(argv[a + 1]);
        Engine probe; probe.P = defaultsFor(marketIdx(m)); probe.loadNormHistory(normHist, sessionOf(ev.front().t)); norms = probe.slotNorm;
        Out A = run(ev, m, tick, lv, 0, norms, false);
        Out Up = run(ev, m, tick, lv, 2.0, norms, false), Dn = run(ev, m, tick, lv, -2.0, norms, false);
        Out B = run(ev, m, tick, lv, 0, norms, true);
        int lit = 0, litR = 0, nUp = 0, nDn = 0, nUpR = 0, nDnR = 0, watched = 0;
        for (const Zone& z : A.E.zones) if (z.state != HIDDEN) { ++lit; if (rth(z.firstMs)) ++litR; }
        for (const Zone& z : Up.E.zones) if (z.state != HIDDEN) { ++nUp; if (rth(z.firstMs)) ++nUpR; }
        for (const Zone& z : Dn.E.zones) if (z.state != HIDDEN) { ++nDn; if (rth(z.firstMs)) ++nDnR; }
        watched = (int)A.E.zones.size();
        std::printf("\n== %s %s: %zu events, %d bars, %zu levels -> %d zones, normal %s (%zu slot norms)\n", m.c_str(), argv[a], ev.size(), A.barsN, lv.size(), watched, A.E.normSrc.c_str(), norms.size());
        std::printf("   lit at the levels: %d (RTH %d) | NULL +2 ATR: %d (RTH %d), -2 ATR: %d (RTH %d) | paths a %lld b %lld c %lld | buy %lld sell %lld | episodes %lld\n",
            lit, litR, nUp, nUpR, nDn, nDnR, A.E.tally.pathA, A.E.tally.pathB, A.E.tally.pathC, A.E.tally.decided[0], A.E.tally.decided[1], A.E.tally.episodes);
        for (const Zone& z : A.E.zones) if (z.state != HIDDEN)
            std::printf("     %s %-22s %s  %s  flow %s  lots %lld (adds %lld fills %lld)  paths %s%s  rep %lldx%d  ref %d  gamma %s exp %s  lit %d\n",
                clock12(z.firstMs).c_str(), z.name.c_str(), priceText(z.lvl * tick, tick).c_str(), z.side == BUY ? "BUY " : "SELL", ratioText(z.flowX).c_str(), z.lots, z.adds, z.fills,
                z.paths.c_str(), z.zero ? " 0DTE" : "", z.repSize, z.repCount, z.refCount, z.pos > 0 ? "long" : z.pos < 0 ? "short" : "?", z.expect < 0 ? "?" : (z.expect == z.side ? "agree" : "DISAGREE"), z.episodes);
        for (const Zone& z : Up.E.zones) if (z.state != HIDDEN) std::printf("     null+ %s %s %s flow %s paths %s\n", clock12(z.firstMs).c_str(), priceText(z.lvl * tick, tick).c_str(), z.side == BUY ? "BUY" : "SELL", ratioText(z.flowX).c_str(), z.paths.c_str());
        for (const Zone& z : Dn.E.zones) if (z.state != HIDDEN) std::printf("     null- %s %s %s flow %s paths %s\n", clock12(z.firstMs).c_str(), priceText(z.lvl * tick, tick).c_str(), z.side == BUY ? "BUY" : "SELL", ratioText(z.flowX).c_str(), z.paths.c_str());
        // the level zones that price actually reached (in reach) vs lit, for the hit-rate comparison
        int zl = 0; for (auto& r : A.rec) if (r[0] == 'Z') ++zl;
        if (zl != lit) { ++fails; std::printf("FAIL: %d Z lines for %d lit zones\n", zl, lit); }
        int zb = 0; for (auto& r : B.rec) if (r[0] == 'Z') ++zb;
        if (!B.sameRestart || zb != zl) { ++fails; std::printf("FAIL: restart after every bar changed the record (%d vs %d)\n", zb, zl); }
        else std::printf("   restart from the record after every bar: identical Z lines (no repaint)\n");
        for (auto& r : A.E.normRows) normHist.push_back(r);
        std::printf("   slot rows written %zu (history %zu)\n", A.E.normRows.size(), normHist.size());
        totL += lit; totLR += litR; totN += (nUp + nDn) / 2.0; totNR += (nUpR + nDnR) / 2.0;
    }
    std::printf("\nTOTAL %s: lit at the levels %lld (RTH %lld) vs NULL (+/-2 ATR average) %.1f (RTH %.1f)\n", m.c_str(), totL, totLR, totN, totNR);
    std::printf("replay %s: %s\n", m.c_str(), fails ? "FAILED" : "ok");
    return fails ? 1 : 0;
}
