// CandleAbsorbLogic.h - lsCandleAbsorb 1.0.0 (2026-11-01) - SDK-free logic: candle absorption marks at IRT ZigZag pivots + the delta POC
// dash on every candle. Rassul 2026-10-31 (VS ES chart):
//   "apply the absorbtion candle stick rules now that we disucessed"
//   "instead of R , just put D for Demand ( S for supply in opposite case). get rid of F. i just want A, D,S. if there is exhasutionstion
//    just put Ae"
//   "i also want a small dash that represents the delta poc on everybar . this will allow me to see the location of the delta poc"
//   "the dash is supposed to be only on the candles"
// RULES (decided on CLOSED bars, at the bar that CONFIRMS the pivot - never repainted):
//   candidates  = each IRT ZigZag pivot bar (Min Price Change = N BARS = 3, Use High/Low) + the NEXT bar when it re-tests the extreme
//                 within 4 ticks (low: its low <= pivot low + 4 ticks; high: its high >= pivot high - 4 ticks)
//   bar delta   = same-price delta per row (IRT VAP buy - sell volume); delta POC = the row with the biggest |delta|
//   location    = at a LOW: the delta POC at or below BOTH open and close AND in the lower half of the bar; at a HIGH the mirror
//   size        = |delta POC| >= 2.0 x the median |delta| of the bar's rows
//   code        = A  attackers absorbed: sellers' POC at a low / buyers' POC at a high
//                 Ae A + exhaustion: the next row PAST the POC (toward the extreme) traded <= 20% of the POC's |delta|
//                 D  demand: buyers' POC at a low          S  supply: sellers' POC at a high
//                 one code per bar; green at lows, red at highs; no "?" (Rassul 2026-11-01 09:17 CT "get rid of the question mark")
//   dash        = every candle (closed + the forming one): a short bar ON the candle at its delta POC, a little wider than the body,
//                 black edge, green when buyers made the POC (delta > 0), red when sellers did
#pragma once
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace cab {

static const int ROW_OUT = 16;   // (1.1.1) px between the extreme row (his ratio label) and the code
static const char* const VERSION = "1.1.1";   // (1.1.1, 2026-11-03) the code one text row further from the high / low   // was "1.1.0";   // (1.1.0, Rassul 2026-11-02 13:10 CT "you must draw it on the 12:12 bar closed and repaint as necessary") the LIVE mark   // was "1.0.1":   // (1.0.1) host crash fix only
static const int N_BARS = 3;          // IRT ZigZag Min Price Change = 3 bars, Use High/Low
static const int RETEST_TICKS = 4;    // the next bar re-tests the extreme within 4 ticks
static const double SIZE_X = 2.0;     // delta POC >= 2.0 x the bar's median |delta| row
static const double EXH_X = 0.2;      // Ae: the next row past the POC <= 20% of the POC
static const int MAX_BARS = 20000;    // closed bars kept

enum Code { NONE = 0, A = 1, AE = 2, D = 3, S = 4 };
inline const char* codeName(int c) { return c == A ? "A" : c == AE ? "Ae" : c == D ? "D" : c == S ? "S" : "-"; }

struct Bar { long long t = 0, o = 0, h = 0, l = 0, c = 0; };   // whole ticks; t = IRT bar END (local seconds)
struct DRow { long long tick = 0; long long dlt = 0; };

inline bool toTicks(double px, double tick, long long& out) {
    if (!std::isfinite(px) || !(tick > 0)) return false;
    const double q = px / tick;
    if (!std::isfinite(q) || std::fabs(q) > 9e15) return false;
    out = (long long)std::llround(q); return true;
}
inline std::string hm(long long t) { if (t <= 0) return "--:--"; const long long s = ((t % 86400) + 86400) % 86400; char b[16]; std::snprintf(b, sizeof b, "%02lld:%02lld", s / 3600, (s / 60) % 60); return b; }

// one bar's delta read: the delta POC and, for a low / a high, the code it would carry
struct BarRead {
    bool have = false; long long poc = 0, pocD = 0; double x = 0;   // delta POC row, its delta, |delta| / median
    int codeLow = NONE, codeHigh = NONE;
};
inline BarRead readBar(const Bar& b, const std::vector<DRow>& rowsIn) {
    BarRead r; if (rowsIn.empty()) return r;
    std::vector<DRow> rows(rowsIn); std::sort(rows.begin(), rows.end(), [](const DRow& a, const DRow& c) { return a.tick < c.tick; });
    {   std::vector<DRow> m; for (const DRow& x : rows) { if (!m.empty() && m.back().tick == x.tick) m.back().dlt += x.dlt; else m.push_back(x); } rows.swap(m); }
    int pk = 0; for (int j = 1; j < (int)rows.size(); ++j) if (std::llabs(rows[(size_t)j].dlt) > std::llabs(rows[(size_t)pk].dlt)) pk = j;   // ties: the lowest row
    r.have = true; r.poc = rows[(size_t)pk].tick; r.pocD = rows[(size_t)pk].dlt;
    std::vector<long long> a; a.reserve(rows.size()); for (const DRow& x : rows) a.push_back(std::llabs(x.dlt));
    std::sort(a.begin(), a.end()); const size_t n = a.size();
    double med = n % 2 ? (double)a[n / 2] : ((double)a[n / 2 - 1] + (double)a[n / 2]) / 2.0; if (!(med > 0)) med = 1.0;
    r.x = (double)std::llabs(r.pocD) / med;
    if (r.pocD == 0 || r.x < SIZE_X - 1e-9) return r;
    auto nextPast = [&](int dir) -> long long {   // |delta| of the row one tick past the POC toward the extreme (-1 = none traded there)
        const long long t = r.poc + dir; for (const DRow& x : rows) if (x.tick == t) return std::llabs(x.dlt); return -1; };
    const bool exLow = [&] { const long long nx = nextPast(-1); return nx >= 0 && (double)nx <= EXH_X * (double)std::llabs(r.pocD) + 1e-9; }();
    const bool exHigh = [&] { const long long nx = nextPast(+1); return nx >= 0 && (double)nx <= EXH_X * (double)std::llabs(r.pocD) + 1e-9; }();
    if (r.poc <= std::min(b.o, b.c) && 2 * r.poc <= b.h + b.l) r.codeLow = r.pocD < 0 ? (exLow ? AE : A) : D;    // sellers absorbed / demand
    if (r.poc >= std::max(b.o, b.c) && 2 * r.poc >= b.h + b.l) r.codeHigh = r.pocD > 0 ? (exHigh ? AE : A) : S;  // buyers absorbed / supply
    return r;
}

struct Mark { int bar = -1; bool low = true; int code = NONE; long long poc = 0, pocD = 0; double x = 0; int pivot = -1, conf = -1; long long t = 0; };

class Engine {
public:
    std::vector<Bar> B; std::vector<BarRead> R; std::vector<Mark> marks;
    std::vector<std::string> out;   // record lines to append (host flushes)
    int zDir = 0, zI = 0, zHi = 0, zLo = 0;   // IRT zigzag: live leg (+1 / -1 / 0 none), its extreme; first-leg high / low
    long long pivots = 0, cands = 0;
    void reset() { *this = Engine(); }
    int lastBar() const { return (int)B.size() - 1; }
    // a closed bar and its delta rows (each bar once, in order)
    void addBar(const Bar& b, const std::vector<DRow>& rows) {
        B.push_back(b); R.push_back(readBar(b, rows)); const int k = lastBar();
        if (k >= N_BARS) {
            long long ph = LLONG_MIN, pl = LLONG_MAX; for (int j = k - N_BARS; j < k; ++j) { ph = std::max(ph, B[(size_t)j].h); pl = std::min(pl, B[(size_t)j].l); }
            const Bar& c = B[(size_t)k];
            if (zDir == 0) {
                if (c.h > ph) { pivot(zLo, true, k); zDir = 1; zI = k; }
                else if (c.l < pl) { pivot(zHi, false, k); zDir = -1; zI = k; }
            } else if (zDir > 0) {
                if (c.h > B[(size_t)zI].h) zI = k;
                else if (c.l < pl) { pivot(zI, false, k); zDir = -1; zI = k; }
            } else {
                if (c.l < B[(size_t)zI].l) zI = k;
                else if (c.h > ph) { pivot(zI, true, k); zDir = 1; zI = k; }
            }
        }
        if (zDir == 0) { if (b.h > B[(size_t)zHi].h) zHi = k; if (b.l < B[(size_t)zLo].l) zLo = k; }
        if ((int)B.size() > MAX_BARS + 2000) trim(2000);
    }
    // the bar k is a confirmed pivot (low or high), confirmed at bar conf (closed): the pivot bar + the re-test bar
    void pivot(int k, bool low, int conf) {
        if (k < 0 || k >= (int)B.size()) return;
        ++pivots;
        consider(k, low, k, conf);
        const int k1 = k + 1;
        if (k1 <= conf && k1 < (int)B.size()) {
            const bool re = low ? B[(size_t)k1].l <= B[(size_t)k].l + RETEST_TICKS : B[(size_t)k1].h >= B[(size_t)k].h - RETEST_TICKS;
            if (re) consider(k1, low, k, conf);
        }
    }
    void consider(int k, bool low, int pv, int conf) {
        ++cands;
        for (const Mark& m : marks) if (m.bar == k) return;   // one code per bar
        const BarRead& r = R[(size_t)k]; const int code = low ? r.codeLow : r.codeHigh;
        if (!r.have || code == NONE) return;
        Mark m; m.bar = k; m.low = low; m.code = code; m.poc = r.poc; m.pocD = r.pocD; m.x = r.x; m.pivot = pv; m.conf = conf; m.t = B[(size_t)k].t;
        marks.push_back(m);
        std::ostringstream o; char xb[32]; std::snprintf(xb, sizeof xb, "%.1f", m.x);
        o << "MARK," << m.t << "," << hm(m.t) << "," << (low ? "LOW" : "HIGH") << "," << codeName(code) << ",poc_tick " << m.poc << ",delta " << m.pocD << ",x " << xb
          << ",pivot " << hm(B[(size_t)pv].t) << (k == pv ? ",pivot bar" : ",retest bar") << ",confirmed " << hm(B[(size_t)conf].t);
        out.push_back(o.str());
    }
    void trim(int n) {   // drop the oldest n bars (indices shift)
        if (n <= 0 || n >= (int)B.size()) return;
        B.erase(B.begin(), B.begin() + n); R.erase(R.begin(), R.begin() + n);
        std::vector<Mark> keep; for (Mark m : marks) { m.bar -= n; m.pivot -= n; m.conf -= n; if (m.bar >= 0) keep.push_back(m); } marks.swap(keep);
        zI = std::max(0, zI - n); zHi = std::max(0, zHi - n); zLo = std::max(0, zLo - n); trimmed += n;
    }
    long long trimmed = 0;
    // (1.1.0, Rassul 2026-11-02 13:10 CT "you must draw it on the 12:12 bar closed and repaint as necessary") the LIVE mark: the running
    // ZigZag leg's extreme bar (and its re-test bar) is judged as soon as it CLOSES, as if the pivot were confirmed; it is drawn the same
    // way, moves / disappears when a later bar makes a new extreme (repaint allowed: the pivot rule), and becomes a recorded mark once
    // the ZigZag confirms the pivot. Not recorded while live.
    std::vector<Mark> liveMarks() const {
        std::vector<Mark> o; if (zDir == 0 || zI < 0 || zI >= (int)B.size()) return o;
        const bool low = zDir < 0;   // a down leg's extreme = a candidate LOW pivot; an up leg's = a candidate HIGH
        auto add = [&](int k) {
            if (k < 0 || k >= (int)B.size()) return;
            for (const Mark& m : marks) if (m.bar == k) return;
            const BarRead& r = R[(size_t)k]; const int code = low ? r.codeLow : r.codeHigh; if (!r.have || code == NONE) return;
            Mark m; m.bar = k; m.low = low; m.code = code; m.poc = r.poc; m.pocD = r.pocD; m.x = r.x; m.pivot = zI; m.conf = -1; m.t = B[(size_t)k].t; o.push_back(m); };
        add(zI);
        const int k1 = zI + 1;
        if (k1 < (int)B.size()) { const bool re = low ? B[(size_t)k1].l <= B[(size_t)zI].l + RETEST_TICKS : B[(size_t)k1].h >= B[(size_t)zI].h - RETEST_TICKS; if (re) add(k1); }
        return o;
    }
};

// ---------- layout (pixels) ----------
struct Prim { enum K { FILL, TEXT } k = FILL; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned long col = 0; std::string text; bool bold = true; };
struct Colours { unsigned long up = 0x22C55E, down = 0xEF4444, edge = 0x000000; };
struct View {
    int l = 0, t = 0, r = 0, b = 0, first = 0, last = 0; double bw = 8;    // bw = pixels per bar
    std::function<double(int)> X; std::function<double(double)> Y;         // engine bar -> centre x; price in ticks -> y
    std::function<int(const std::string&)> textW;
    bool haveForming = false; Bar forming; BarRead formRead; int formIdx = -1;   // the forming bar (engine index = lastBar + 1)
    int clipL = -30000, clipR = 30000;
};
struct Scene { std::vector<Prim> p; int dashes = 0, marksDrawn = 0, liveDrawn = 0; };
inline int ci(double v) { return (int)std::lround(std::max(-30000.0, std::min(30000.0, v))); }

// the dash: ON the candle at the delta POC row, a little wider than the body (body = 60% of the bar spacing, dash = body + 2 px a side),
// 2 px tall (3 px when the bars are wide), with a 1 px black edge all round
inline void dash(Scene& s, const View& v, const Colours& C, double xc, long long poc, long long d) {
    if (d == 0) return;
    const double body = std::max(1.0, v.bw * 0.6), half = body / 2.0 + 2.0;
    const double y = v.Y((double)poc); const int h = v.bw >= 10 ? 3 : 2;
    const int x0 = ci(xc - half), x1 = ci(xc + half), y0 = ci(y - h / 2.0), y1 = y0 + h - 1;
    if (x1 < v.clipL || x0 > v.clipR || y1 < v.t || y0 > v.b) return;
    Prim e; e.k = Prim::FILL; e.x0 = x0 - 1; e.x1 = x1 + 1; e.y0 = y0 - 1; e.y1 = y1 + 1; e.col = C.edge; s.p.push_back(e);
    Prim f; f.k = Prim::FILL; f.x0 = x0; f.x1 = x1; f.y0 = y0; f.y1 = y1; f.col = d > 0 ? C.up : C.down; s.p.push_back(f);
    ++s.dashes;
}
inline Scene layout(const Engine& E, const View& v, const Colours& C = Colours()) {
    Scene s;
    const int a = std::max(0, v.first), z = std::min(E.lastBar(), v.last);
    for (int k = a; k <= z; ++k) { const BarRead& r = E.R[(size_t)k]; if (r.have) dash(s, v, C, v.X(k), r.poc, r.pocD); }
    if (v.haveForming && v.formIdx >= a && v.formIdx <= v.last && v.formRead.have) dash(s, v, C, v.X(v.formIdx), v.formRead.poc, v.formRead.pocD);
    std::vector<Mark> all = E.marks; { const std::vector<Mark> lv = E.liveMarks(); all.insert(all.end(), lv.begin(), lv.end()); s.liveDrawn = (int)lv.size(); }   // (1.1.0)
    for (const Mark& m : all) {
        if (m.bar < a || m.bar > z) continue;
        const Bar& b = E.B[(size_t)m.bar];
        const std::string txt = codeName(m.code);   // (09:17 CT) no "?"
        const int tw = v.textW ? v.textW(txt) : (int)txt.size() * 7; const double xc = v.X(m.bar);
        // (1.1.1, Rassul 2026-11-03 11:05 / 12:24 CT "the labels like A S etc on the candles seem to conflict with other labels") one text row
        // (16 px) further out: his updn ratio / time label keeps the row at the extreme; lsWavePOC's Pb sits one more row out (PB_GAP 46)
        const double y = m.low ? v.Y((double)b.l - 0.5) + 3 + ROW_OUT : v.Y((double)b.h + 0.5) - 3 - 14 - ROW_OUT;
        Prim t; t.k = Prim::TEXT; t.x0 = ci(xc - tw / 2.0); t.x1 = ci(xc + tw / 2.0); t.y0 = ci(y); t.y1 = t.y0 + 14; t.col = m.low ? C.up : C.down; t.text = txt;
        if (t.x0 < v.clipL || t.x1 > v.clipR || t.y0 < v.t || t.y1 > v.b) continue;
        s.p.push_back(t); ++s.marksDrawn;
    }
    return s;
}

// status text (one line per fact; written on change / every 30 s by the host)
inline std::string statusText(const Engine& E, const std::string& mkt, int spb, const std::string& vap, const std::string& rec) {
    std::ostringstream o; long long n[5] = {0, 0, 0, 0, 0}; for (const Mark& m : E.marks) ++n[m.code];
    o << "VERSION," << VERSION << "\nMARKET," << mkt << ",SECONDS_PER_BAR," << spb << "\nRULES,ZigZag " << N_BARS << " bars High/Low,retest " << RETEST_TICKS
      << " ticks,delta POC >= " << SIZE_X << "x median in the extreme end,Ae next row <= " << (int)(EXH_X * 100) << "%\n"
      << "BARS," << E.B.size() << ",pivots " << E.pivots << ",candidates " << E.cands << "\n"
      << "MARKS,A " << n[A] << ",Ae " << n[AE] << ",D " << n[D] << ",S " << n[S] << "\nVAP," << vap << "\nRECORD," << rec << "\n";
    const size_t m = E.marks.size();
    { const std::vector<Mark> lv = E.liveMarks(); o << "LIVE," << lv.size(); for (const Mark& k : lv) o << "," << hm(k.t) << " " << (k.low ? "low " : "high ") << codeName(k.code) << " (repaints until the ZigZag confirms)"; o << "\n"; }   // (1.1.0)
    for (size_t i = m > 8 ? m - 8 : 0; i < m; ++i) { const Mark& k = E.marks[i]; o << "LAST," << hm(k.t) << "," << (k.low ? "low " : "high ") << codeName(k.code) << ",delta " << k.pocD << "\n"; }
    return o.str();
}

} // namespace cab
