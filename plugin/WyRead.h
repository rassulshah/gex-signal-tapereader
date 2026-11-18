// WyRead.h  --  the Wyckoff turn reads of the Reversal Read pane (lsRevRead 0.17.0): E / A / Sp / Ut / I on a strip at the top of the pane,
// the plain read right of the last candle. SDK-free; the host feeds the CLOSED candles of the session (prices in ticks, lots) and draws.
// (1.0.0, 2026-11-18, Rassul 05:45-08:15 CT: "show the list of ideas ... lets try to work through them", "filter based on a wyckoff perspective",
//  "lets add the letters on a strip on the top of the pane", "remove Ts", "springs are false breakdowns and upthrusts are false breakouts ... use prior
//  highs and lows and then see the orderflow on those attempted breaks", "use Sp and Ut", "get rid of the ? marks", "dont put anything on the candle
//  chart, no lines", "the other thing to note is the lack of followthrough ... in some cases you even have a strong rejection with strength (I)",
//  "you might also want to see if there was a stop run", "ok.. lets build this"; mockup wyckoff-turn-chart.html, ES 11-17/18 overnight)
//   E   lack of demand / supply: the attackers' delta momentum (last 3 candles) faded to 30% of its best in the leg, or the thrust shortened
//   A   effort without result: two pushes in a row cost 2.5x+ the lots per tick of new ground of the leg's first push
//   Sp  spring: a false break of the recent swing / range low (the floor of the last 15 candles), closed back above within 3 candles, no
//       follow-through, confirmed by order flow - "absorbed" (sellers hit hard, bids stacked against them, next candle delta up) or "flush"
//       (sellers ran the stops, then stopped adding: offers pulled, bids back, delta up); a stop-run line when one big sell swept 3+ prices
//   Ut  upthrust: the mirror above the recent swing / range high
//   I   the other side took over: within 3 candles of a leg extreme, the other side's delta 2x+ normal and a close through the prior candle; or
//       the strong rejection after Sp / Ut (a wide candle, 1.5x+ normal range, closing back through the spring / upthrust candle)
// The thresholds are starting values from one ES night; the nightly scorer (lra.wy_turns) measures them per market. Pure function of the closed
// candles: the same candles give the same letters (no repaint); a letter decided after later candles (Sp / Ut wait for the reclaim and two candles
// without follow-through) appears then on its own candle and never changes.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace wyr {

static const char* const VERSION = "1.0.0";

// the cut-offs (starting values, ES 2026-11-18 overnight; LRA-WyRead-<MKT>.csv may override - see Params)
struct Params {
    double eFade = 0.30;      // E: momentum at or under this share of the leg's best
    double ePeakX = 2.0;      // E: the leg's best momentum must be 2x+ the median |delta| (a real push to fade from)
    double sotHalf = 0.50;    // E (SOT): the last two pushes each at or under half the leg's biggest earlier push
    double sotBigR = 0.5;     // E (SOT): that biggest push at least half a normal candle range
    double aX = 2.5;          // A: lots per tick vs the first push
    double iX = 2.0;          // I: the other side's delta vs the median |delta|
    double rejR = 1.5;        // I (rejection): range vs the median range
    double spAtt = 1.5;       // Sp / Ut: the breakers' lots vs their 10-candle mean
    double spDef = 1.5;       // Sp / Ut absorbed: the defenders' resting net vs the mean |net|
    double flushOwn = 0.30;   // Sp / Ut flush: the breakers' own resting net drops to 30% or less
    int spLook = 15;          // Sp / Ut: the reference floor / ceiling = the extreme swing of the last 15 candles
    int spReclaim = 3;        // Sp / Ut: closed back within 3 candles of the deepest one
    int spMinTicks = 2;       // Sp / Ut: broke by 2+ ticks
    int srLevels = 3;         // stop run: one order swept 3+ prices
    double srX = 3.0;         // stop run: and was 3x+ the median biggest order
};

// one CLOSED candle (prices in ticks; lots). book = the Book Flow record exists for it (the MBO feed was on).
// Sides follow the Book Flow record: [0] = BID (resting bids; hit = lots SOLD into them), [1] = ASK (resting offers; hit = lots BOUGHT)
struct Bar {
    long long t = 0; long long o = 0, h = 0, l = 0, c = 0; long long buy = 0, sell = 0;
    bool book = false; long long addNet[2] = {0, 0}, aggrMax[2] = {0, 0}, sweepLv[2] = {0, 0}, ice[2] = {0, 0};
    std::string when;   // "1:39 AM" (the host's clock)
    long long delta() const { return buy - sell; }
    long long range() const { return h - l; }
};
struct Mark { char k = 0; int dir = 0; };   // k: 'E' 'A' 'S' (Sp) 'U' (Ut) 'I'; dir: +1 green (buyers / a low), -1 red (sellers / a top)
inline std::string label(const Mark& m) { return m.k == 'S' ? "Sp" : m.k == 'U' ? "Ut" : std::string(1, m.k); }
struct Read { std::string head; int dir = 0; std::vector<std::string> lines; };
struct Out { std::map<long long, std::vector<Mark> > marks; std::map<long long, Read> reads; };

// ---- small helpers
inline double median(std::vector<double> v) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); const size_t n = v.size(); return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0; }
inline std::string fx(double x) { char b[16]; std::snprintf(b, sizeof b, "%.1fx", x); return b; }
inline std::string num(long long v) { return std::to_string(v); }
inline std::string sgn(long long v) { return (v > 0 ? "+" : "") + std::to_string(v); }
inline std::string px(long long ticks, double tick, int dec) { char b[32]; std::snprintf(b, sizeof b, "%.*f", dec, (double)ticks * tick); return b; }
inline std::string pts(long long ticks, double tick, int dec) { char b[32]; std::snprintf(b, sizeof b, "%.*f", dec, (double)ticks * tick); return b; }

struct Ctx {
    const std::vector<Bar>& B; const Params& P; double tick; int dec;
    std::vector<double> medDl, medRg, medBig[2];   // per candle: the median |delta| / range / biggest order of the 20 candles BEFORE it
    Ctx(const std::vector<Bar>& b, const Params& p, double t, int d) : B(b), P(p), tick(t), dec(d) {
        const int n = (int)B.size(); medDl.assign((size_t)n, 0); medRg.assign((size_t)n, 0); medBig[0].assign((size_t)n, 0); medBig[1].assign((size_t)n, 0);
        for (int i = 0; i < n; ++i) { std::vector<double> d, r, g0, g1;
            for (int j = std::max(0, i - 20); j < i; ++j) { d.push_back((double)std::llabs(B[(size_t)j].delta())); r.push_back((double)B[(size_t)j].range()); if (B[(size_t)j].book) { g0.push_back((double)B[(size_t)j].aggrMax[0]); g1.push_back((double)B[(size_t)j].aggrMax[1]); } }
            medDl[(size_t)i] = median(d); medRg[(size_t)i] = median(r); medBig[0][(size_t)i] = median(g0); medBig[1][(size_t)i] = median(g1); } }
};

// ---- the 3-bar High / Low ZigZag (the chart's ZigZag, VolRangeLogic zigStep): per candle the leg it belongs to
struct Leg { int a = -1; int dir = 0; };   // a = the pivot candle the leg starts from; dir +1 up / -1 down
inline std::vector<Leg> legs(const std::vector<Bar>& B) {
    const int n = (int)B.size(); std::vector<Leg> L((size_t)n); if (n < 2) return L;
    int zd = B[1].h >= B[0].h ? 1 : -1, zext = 0, start = 0;
    L[0].a = 0; L[0].dir = zd;
    for (int i = 1; i < n; ++i) {
        long long pl = B[(size_t)i - 1].l, ph = B[(size_t)i - 1].h; for (int j = std::max(0, i - 3); j < i; ++j) { pl = std::min(pl, B[(size_t)j].l); ph = std::max(ph, B[(size_t)j].h); }
        if (zd == 1) { if (B[(size_t)i].h > B[(size_t)zext].h) zext = i; else if (B[(size_t)i].l < pl && i != zext) { start = zext; zext = i; zd = -1; } }
        else         { if (B[(size_t)i].l < B[(size_t)zext].l) zext = i; else if (B[(size_t)i].h > ph && i != zext) { start = zext; zext = i; zd = 1; } }
        L[(size_t)i].a = start; L[(size_t)i].dir = zd;
    }
    return L;
}

inline void add(Out& o, int i, const std::vector<Bar>& B, char k, int dir, const Read& r) {
    const long long t = B[(size_t)i].t; std::vector<Mark>& v = o.marks[t];
    for (const Mark& m : v) if (m.k == k) return;
    Mark m; m.k = k; m.dir = dir; v.push_back(m);
    Read& rr = o.reads[t]; if (rr.head.empty()) rr = r; else { rr.lines.push_back(""); rr.lines.push_back(r.head); for (const std::string& s : r.lines) rr.lines.push_back(s); }
}

// ---- E and A: the pushes of each leg (a push = a candle that makes new ground in the leg's direction)
inline void pushes(const Ctx& X, const std::vector<Leg>& L, Out& o) {
    const std::vector<Bar>& B = X.B; const int n = (int)B.size(); const Params& P = X.P;
    int curA = -2, cnt = 0, run = 0, lastA = -10; long long ext = 0, pk = 0; double base = 0; bool eDone = false; std::vector<long long> ground; std::vector<double> xs; std::vector<long long> lpts;
    for (int i = 1; i < n; ++i) {
        const Leg& g = L[(size_t)i]; const int d = g.dir;
        if (g.a != curA) { curA = g.a; cnt = 0; run = 0; pk = 0; base = 0; eDone = false; ground.clear(); xs.clear(); lpts.clear(); const Bar& s = B[(size_t)std::max(0, g.a)]; ext = d > 0 ? s.h : s.l; }
        const Bar& b = B[(size_t)i]; const long long nw = d > 0 ? b.h - ext : ext - b.l; if (nw <= 0) continue;
        ext = d > 0 ? b.h : b.l; ++cnt; ground.push_back(nw);
        long long mom = 0; for (int j = std::max(0, i - 2); j <= i; ++j) mom += B[(size_t)j].delta(); mom *= d;
        pk = std::max(pk, mom);
        // E: momentum faded, or the thrust shortened
        if (!eDone && cnt >= 3) {
            const bool fade = (double)pk >= P.ePeakX * X.medDl[(size_t)i] && pk > 0 && (double)mom <= P.eFade * (double)pk;
            long long big = 0; for (size_t q = 0; q + 2 < ground.size(); ++q) big = std::max(big, ground[q]);
            const bool sot = ground.size() >= 3 && (double)big >= P.sotBigR * X.medRg[(size_t)i] && big >= 2 && (double)ground[ground.size() - 1] <= P.sotHalf * (double)big && (double)ground[ground.size() - 2] <= P.sotHalf * (double)big;
            if (fade || sot) { eDone = true; Read r; r.dir = -d; r.head = std::string(d > 0 ? "Buyers" : "Sellers") + " running out";
                if (fade) r.lines.push_back("Delta momentum " + num(pk) + " -> " + num(mom) + " (last 3 candles).");
                if (sot) { std::string s = "Pushes getting shorter: "; const size_t a0 = ground.size() > 4 ? ground.size() - 4 : 0; for (size_t q = a0; q < ground.size(); ++q) s += (q > a0 ? " -> " : "") + num(ground[q]); r.lines.push_back(s + " ticks."); }
                r.lines.push_back("Watch: A or the turn bar (I).");
                add(o, i, B, 'E', -d, r); } }
        // A: effort without result
        const long long eff = d > 0 ? b.buy : b.sell; const double lpt = (double)eff / (double)nw;
        if (cnt == 1) { base = lpt; continue; }
        if (base <= 0) continue;
        const double x = lpt / base; xs.push_back(x); lpts.push_back((long long)std::llround(lpt));
        run = x >= P.aX ? run + 1 : 0;
        if (run >= 2 && i - lastA >= 3) { lastA = i; Read r; r.dir = -d; r.head = std::string(d > 0 ? "Buyers" : "Sellers") + " being absorbed";
            r.lines.push_back(std::string(d > 0 ? "Buyers" : "Sellers") + " still hitting, price stuck:");
            r.lines.push_back(fx(xs[xs.size() - 2]) + ", " + fx(x) + " the lots per tick of the first push.");
            r.lines.push_back(std::string("Watch: ") + (d > 0 ? "sellers" : "buyers") + " taking over (I).");
            add(o, i, B, 'A', -d, r); run = 0; }
    }
}

// ---- Sp / Ut (+ the rejection I)
inline bool fractal(const std::vector<Bar>& B, int k, int side) {   // side +1 = a low (for Sp), -1 = a high (for Ut)
    const int n = (int)B.size(); if (k < 2 || k > n - 3) return false;
    for (int q : { k - 2, k - 1, k + 1, k + 2 }) { if (side > 0 ? !(B[(size_t)k].l < B[(size_t)q].l) : !(B[(size_t)k].h > B[(size_t)q].h)) return false; }
    return true;
}
inline void springs(const Ctx& X, Out& o, std::vector<int>& rejI) {
    const std::vector<Bar>& B = X.B; const int n = (int)B.size(); const Params& P = X.P;
    for (int side : { 1, -1 }) {   // +1 spring (a false break DOWN), -1 upthrust (a false break UP)
        const int att = side > 0 ? 0 : 1;   // the breakers hit the resting side att: a spring = sellers hitting the bids [0]; an upthrust = buyers lifting the offers [1]
        const int def = att, own = 1 - att;  // defenders rest on side att (bids for a spring); the breakers' own resting orders on the other side
        auto low = [&](int k) { return side > 0 ? B[(size_t)k].l : -B[(size_t)k].h; };
        auto cls = [&](int k) { return side > 0 ? B[(size_t)k].c : -B[(size_t)k].c; };
        auto lots = [&](int k) { return side > 0 ? B[(size_t)k].sell : B[(size_t)k].buy; };
        int lastMark = -10; std::vector<int> usedRef;
        for (int d = 12; d < n; ++d) {
            int rk = -1; for (int k = std::max(0, d - P.spLook); k <= d - 3; ++k) if (fractal(B, k, side) && (rk < 0 || low(k) < low(rk))) rk = k;
            if (rk < 0) continue;
            const long long ref = low(rk);
            if (std::find(usedRef.begin(), usedRef.end(), rk) != usedRef.end()) continue;
            if (low(d) > ref - P.spMinTicks) continue;                                        // broke it by 2+ ticks
            bool deeper = true; for (int k = std::max(rk + 1, d - 3); k < d; ++k) if (low(k) <= low(d)) deeper = false;   // the deepest candle of the attempt
            if (!deeper) continue;
            bool closedBelow = false; for (int k = rk + 1; k < d; ++k) if (cls(k) < ref) closedBelow = true;   // the floor already gave way on a close: not a fresh break
            if (closedBelow) continue;
            if (d + 2 >= n) continue;                                                           // two candles after it are needed
            if (low(d + 1) < low(d) || low(d + 2) < low(d)) continue;                          // follow-through: a real break
            int rc = -1; for (int k = d; k <= std::min(n - 1, d + P.spReclaim); ++k) if (cls(k) > ref) { rc = k; break; }
            if (rc < 0) continue;
            const Bar& b = B[(size_t)d];
            if (!b.book) continue;                                                              // the order-flow check needs the Book Flow record
            double mA = 0; int nA = 0; double mD = 0; int nD = 0;
            for (int k = std::max(0, d - 10); k < d; ++k) { mA += (double)lots(k); ++nA; if (B[(size_t)k].book) { mD += (double)std::llabs(B[(size_t)k].addNet[def]); ++nD; } }
            const double ax = nA && mA > 0 ? (double)lots(d) / (mA / nA) : 0, dx = nD && mD > 0 ? (double)b.addNet[def] / (mD / nD) : 0;
            if (ax < P.spAtt) continue;
            const Bar& n1 = B[(size_t)d + 1]; const long long dl1 = n1.delta() * side;
            const bool absorbed = dx >= P.spDef && dl1 > 0;
            int fk = -1; if (!absorbed && b.addNet[own] > 0) for (int k = d + 1; k <= d + 2; ++k) { const Bar& q = B[(size_t)k]; if (q.book && (double)q.addNet[own] <= P.flushOwn * (double)b.addNet[own] && q.addNet[def] > 0 && q.delta() * side > 0) { fk = k; break; } }
            if (!absorbed && fk < 0) continue;
            if (d - lastMark < 4) continue;
            usedRef.push_back(rk); lastMark = d;
            const bool lo = side > 0; Read r; r.dir = side; r.head = std::string(lo ? "Spring" : "Upthrust") + (absorbed ? " (absorbed)" : " (flush)");
            r.lines.push_back("Broke the " + B[(size_t)rk].when + (lo ? " low " : " high ") + px(lo ? B[(size_t)rk].l : B[(size_t)rk].h, X.tick, X.dec) + " by " + num(lo ? B[(size_t)rk].l - b.l : b.h - B[(size_t)rk].h) + " ticks,");
            r.lines.push_back("closed back " + std::string(lo ? "above" : "below") + " it at " + B[(size_t)rc].when + ". No follow-through.");
            const std::string brk = lo ? "Sellers" : "Buyers", dside = lo ? "bids" : "offers", oside = lo ? "offers" : "bids";
            if (absorbed) { r.lines.push_back(brk + " hit hard (" + fx(ax) + "), " + dside + " stacked " + fx(dx) + ".");
                r.lines.push_back("Next candle delta " + sgn(n1.delta()) + "."); }
            else { const Bar& q = B[(size_t)fk]; r.lines.push_back(brk + " flushed it (" + fx(ax) + "), then stopped:");
                r.lines.push_back(oside + " " + sgn(b.addNet[own]) + " -> " + sgn(q.addNet[own]) + ", " + dside + " back " + sgn(q.addNet[def]) + "."); }
            const double mb = X.medBig[att][(size_t)d];
            if (b.sweepLv[att] >= P.srLevels && mb > 0 && (double)b.aggrMax[att] >= P.srX * mb)
                r.lines.push_back("Stop run: one " + num(b.aggrMax[att]) + "-lot " + (lo ? "sell" : "buy") + " swept " + num(b.sweepLv[att]) + " prices (" + fx((double)b.aggrMax[att] / mb) + ").");
            r.lines.push_back(brk + " trapped " + (lo ? "below." : "above."));
            add(o, d, B, side > 0 ? 'S' : 'U', side, r);
            // the strong rejection: a wide candle right after, closing back through the spring / upthrust candle
            for (int k = d + 1; k <= d + 2 && k < n; ++k) { const Bar& q = B[(size_t)k]; const double mr = X.medRg[(size_t)k];
                const bool wide = mr > 0 && (double)q.range() >= P.rejR * mr, through = lo ? (q.c > b.h && q.c > q.o) : (q.c < b.l && q.c < q.o);
                if (wide && through) { Read ri; ri.dir = side; ri.head = std::string(lo ? "Buyers" : "Sellers") + " took over";
                    ri.lines.push_back("Strong rejection: " + std::string(lo ? "+" : "-") + pts(std::llabs(q.c - q.o), X.tick, X.dec) + " pts, " + fx((double)q.range() / mr) + " normal range,");
                    ri.lines.push_back("closed through the " + std::string(lo ? "spring" : "upthrust") + " candle.");
                    ri.lines.push_back("Watch: entry at a key level (Rev, PB, Cob).");
                    add(o, k, B, 'I', side, ri); rejI.push_back(k); break; } }
        }
    }
}

// ---- I: the other side took over right after a leg extreme
inline void initiation(const Ctx& X, const std::vector<Leg>& L, Out& o, const std::vector<int>& rejI) {
    const std::vector<Bar>& B = X.B; const int n = (int)B.size(); const Params& P = X.P; int last[2] = { -10, -10 };
    for (int i = 4; i < n; ++i) {
        const Bar& b = B[(size_t)i]; const Bar& p = B[(size_t)i - 1]; const double md = X.medDl[(size_t)i]; if (md <= 0) continue;
        for (int side : { 1, -1 }) {   // +1 buyers take over after a LOW, -1 sellers after a HIGH
            const long long dl = b.delta() * side; if ((double)dl < P.iX * md) continue;
            if (side > 0 ? !(b.c > p.h) : !(b.c < p.l)) continue;
            int ex = -1; for (int k = i - 3; k < i; ++k) { if (k < 0) continue; bool isExt = true;
                for (int q = std::max(0, k - 10); q < i; ++q) if (q != k && (side > 0 ? B[(size_t)q].l < B[(size_t)k].l : B[(size_t)q].h > B[(size_t)k].h)) { isExt = false; break; }
                if (isExt) { ex = k; break; } }
            if (ex < 0) continue;
            const int si = side > 0 ? 0 : 1; if (i - last[si] < 4) continue;
            if (std::find(rejI.begin(), rejI.end(), i) != rejI.end()) continue;
            last[si] = i; (void)L;
            Read r; r.dir = side; r.head = std::string(side > 0 ? "Buyers" : "Sellers") + " took over";
            r.lines.push_back(std::string(side > 0 ? "Buyers" : "Sellers") + " hit hard (" + fx((double)dl / md) + " normal delta),");
            r.lines.push_back("closed through the prior candle, " + std::string(side > 0 ? "off the " + B[(size_t)ex].when + " low." : "off the " + B[(size_t)ex].when + " high."));
            r.lines.push_back("Watch: entry at a key level (Rev, PB, Cob).");
            add(o, i, B, 'I', side, r);
        }
    }
}

// the whole read of a session's closed candles (oldest first). tick > 0; dec = the price decimals.
inline Out run(const std::vector<Bar>& B, double tick, int dec, const Params& P = Params()) {
    Out o; if (B.size() < 15 || !(tick > 0)) return o;
    const Ctx X(B, P, tick, dec); const std::vector<Leg> L = legs(B); std::vector<int> rejI;
    springs(X, o, rejI); pushes(X, L, o); initiation(X, L, o, rejI);
    for (auto& kv : o.marks) {   // the strip order, top to bottom: Sp / Ut, A, E, I (the turn bar last, as it reads)
        auto rank = [](char k) { return k == 'S' || k == 'U' ? 0 : k == 'A' ? 1 : k == 'E' ? 2 : 3; };
        std::stable_sort(kv.second.begin(), kv.second.end(), [&](const Mark& a, const Mark& b) { return rank(a.k) < rank(b.k); }); }
    return o;
}

// the strip record line (the nightly scorer reads lsFlexLevels\RevRead\wy-<MKT>-<spb>-<session>.csv): candle end|letters|dirs
inline std::string recLine(long long t, const std::vector<Mark>& v) { std::string a, d; for (const Mark& m : v) { a += (a.empty() ? "" : ",") + label(m); d += (d.empty() ? "" : ",") + std::to_string(m.dir); } return std::to_string(t) + "|" + a + "|" + d; }

} // namespace wyr
