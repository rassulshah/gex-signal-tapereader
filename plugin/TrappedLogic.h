#ifndef LS_TRAPPEDLOGIC_H
#define LS_TRAPPEDLOGIC_H
// TrappedLogic.h (1.0.0, 2026-10-24) -- SDK-free rules of lsTrapped (the pane) and lsTrappedMarks (the "Tr" letter on the price chart).
// Rassul 23:28 CT 2026-10-24: "istrapped is a go" (Option A mockup: trap spike + fuel-left line, trap/opt-A.png, logic trap/opts.py).
//
// THE QUESTION: are traders stuck on the wrong side of a failed sweep, how many lots, and how much covering (fuel) is still to come?
//
// SWEEP (closed RTH candles of the chart, the chart's own volume at price; market-agnostic):
//   pivots = the 1-bar pivots of the approved mockup (opts.py): a pivot high = a candle whose high is above the prior candle's high
//   and >= the next candle's high (pivot low = mirror); confirmed by that next candle; RTH candles of the session only.
//   The swept pivot = SwingRef.h's "last 3 intact swings" rule on those pivots: 1 back always; 2 back if beyond 1 back; 3 back if
//   beyond 1 and 2 back; intact = no candle after the pivot up to the one BEFORE the last closed candle traded through it (the last
//   candle before the sweep may lead in, as SwingRef); several swept = the DEEPEST (highest high / lowest low); a pivot traps once.
//   (Why not SwingRef's ZigZag swings: on the two approved test days its outside-candle rule confirms none of the 4 approved traps -
//   ES 9:03, NQ 9:12 and 9:45 sweep a pivot the ZigZag has not confirmed yet; the mockup's 1-bar pivots do.)
// TRAP (KIND_SWEEP): buyers trapped = the candle traded above the swept pivot high, closed RED and back below the pivot; the trapped
//   lots = the aggressive BUY lots (ask) at prices >= the pivot in that candle (sellers trapped = mirror: SELL lots (bid) <= the low,
//   GREEN close back above). Qualified ("Tr") when a close is >= 0.25 ATR(14 chart candles before the sweep) beyond the trapped
//   side's average fill in the reversal direction - on the sweep candle or the next candle (1 candle allowed); else dropped.
// QUANTIFY: lots, average fill, share of the candle's buying / selling, ratio vs NORMAL = the median trap lots of that market (and bar
//   size) over the last 20 sessions of the plugin's own records; with fewer than 5 sessions: the median of today's traps so far,
//   this one included ("normal: building"). Numbers are rounded at the decision (avg 1/100 tick, ratio 0.1x) so a redraw from the
//   record is identical (lesson 63).
// FUEL: starts at the trapped lots (capped by the lots still offside at the sweep close); each later closed candle burns the
//   estimated covering = (trapped lots / sweep candle volume) x max(0, net aggressive flow in their exit direction), capped by the
//   lots whose fill price the close has not moved back through. FREED = a close back beyond their average price (the line ends
//   before that candle); SPENT = fuel < 1.0x normal or < half the trapped lots, whichever is lower (a small trap is not spent at once) - the line is
//   drawn to that candle; ENDED = the RTH session closed with fuel left.
// NEVER REPAINT: decided once on closed candles, recorded (TRP line, append-only; status changes as TRS lines; TRD = session done);
//   after a restart the record wins (numbers, end and status).
// Room for a SECOND trap kind (coordinator 2026-10-24: "pullback trap", NOT implemented): Kind enum, record field 'kind' (sweep);
//   lines of an unknown kind are skipped (counted), never misread.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace trp {

static const char* const VERSION = "1.0.1";
static const int FORMAT = 1;                       // record format major (lines of another major are "old" and ignored)
enum Kind { KIND_SWEEP = 0, KIND_PULLBACK = 1 };   // KIND_PULLBACK reserved (not decided by 1.0.0)
enum Side { SELLERS = 1, BUYERS = -1 };            // SELLERS trapped = bullish (buying to come, green, spike up)
enum Status { ACTIVE = 0, FREED = 1, SPENT = 2, ENDED = 3 };

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }
inline bool toTicks(double px, double tick, long long& k) {
    if (!finite(px) || !(tick > 0)) return false;
    const double q = px / tick; if (!finite(q) || std::fabs(q) >= 9.0e15) return false;
    k = (long long)std::llround(q); return true;
}
inline const char* kindName(int k) { return k == KIND_SWEEP ? "sweep" : k == KIND_PULLBACK ? "pullback" : "?"; }
inline int kindOf(const std::string& s) { return s == "sweep" ? KIND_SWEEP : s == "pullback" ? KIND_PULLBACK : -1; }
inline const char* statusName(int s) { return s == FREED ? "freed" : s == SPENT ? "spent" : s == ENDED ? "ended" : "active"; }
inline int statusOf(const std::string& s) { return s == "freed" ? FREED : s == "spent" ? SPENT : s == "ended" ? ENDED : s == "active" ? ACTIVE : -1; }

struct Cell { long long px = 0, ask = 0, bid = 0; };   // ticks; ask = aggressive buying (bought), bid = aggressive selling (sold)
struct Bar {
    long long t = 0;          // the candle END (IRT bar time)
    long long sess = 0;       // the 17:00 CT trading day
    bool rth = false;
    long long o = 0, h = 0, l = 0, c = 0;   // ticks
    bool cellsOk = false; std::vector<Cell> cells;
};
struct Params { double qualAtr = 0.25; double spentX = 1.0; double smallSpentFrac = 0.5; int maxBack = 3; int histSessions = 20; int minSessions = 5; };
struct FuelPt { long long t = 0; double fuel = 0; long long still = 0; };

struct Trap {
    int kind = KIND_SWEEP; long long sess = 0, sweepT = 0, trT = 0, pivT = 0; int side = 0; long long pivPx = 0; int back = 0;
    long long lots = 0; double avgT = 0; int sharePct = 0; long long candleVol = 0, sideVol = 0;
    double ratio = 0; long long normal = 0; bool building = false; double atrT = 0;
    std::vector<std::pair<long long, long long> > fills;     // (px ticks, lots) of the trapped side in the sweep candle
    // live (derived from closed candles; the record's end wins)
    std::vector<FuelPt> fuel; int status = ACTIVE; long long endT = 0; double fuelNow = 0; long long stillNow = 0; long long lastC = 0, lastT = 0;
    bool fromRecord = false, recEnd = false; int recStatus = ACTIVE; long long recEndT = 0;
    std::string key() const { return std::to_string(sweepT) + (side == SELLERS ? "S" : "B") + std::to_string(kind); }
    int sg() const { return side == SELLERS ? 1 : -1; }      // the reversal direction (sellers trapped -> up)
};

// ---- small helpers --------------------------------------------------------------------------------------------------------------
inline long long medianOf(std::vector<long long> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end()); const size_t n = v.size();
    if (n % 2) return v[n / 2];
    return (v[n / 2 - 1] + v[n / 2] + 1) / 2;                  // the two middles' mean, half up (the mockup's 445)
}
inline double round1(double x) { return std::floor(x * 10.0 + 0.5) / 10.0; }
inline double round2(double x) { return std::floor(x * 100.0 + 0.5) / 100.0; }
inline std::string commas(long long v) {
    const bool neg = v < 0; unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ull : (unsigned long long)v;
    std::string s = std::to_string(u), o; int c = 0;
    for (size_t i = s.size(); i > 0; --i) { o.insert(o.begin(), s[i - 1]); if (++c % 3 == 0 && i > 1) o.insert(o.begin(), ','); }
    return neg ? "-" + o : o;
}
inline int tickDecimals(double tick) {
    for (int d = 0; d <= 8; ++d) { const double m = tick * std::pow(10.0, d); if (std::fabs(m - std::floor(m + 0.5)) < 1e-6) return d; }
    return 8;
}
// a price in ticks (may be fractional, e.g. an average) as text: the tick's decimals (+2 for fractions), thousands separated
inline std::string pxText(double ticks, double tick, bool frac = false) {
    const int d = tickDecimals(tick) + (frac ? 2 : 0);
    char b[64]; std::snprintf(b, sizeof b, "%.*f", d, ticks * tick);
    std::string s = b; const size_t dot = s.find('.'); std::string ip = dot == std::string::npos ? s : s.substr(0, dot), fp = dot == std::string::npos ? "" : s.substr(dot);
    bool neg = !ip.empty() && ip[0] == '-'; if (neg) ip.erase(0, 1);
    std::string o; int c = 0; for (size_t i = ip.size(); i > 0; --i) { o.insert(o.begin(), ip[i - 1]); if (++c % 3 == 0 && i > 1) o.insert(o.begin(), ','); }
    return (neg ? "-" : "") + o + fp;
}
inline bool fullLL(const std::string& s, long long& v) { if (s.empty()) return false; char* e = nullptr; const long long r = std::strtoll(s.c_str(), &e, 10); if (!e || *e) return false; v = r; return true; }
inline bool fullD(const std::string& s, double& v) { if (s.empty()) return false; char* e = nullptr; const double r = std::strtod(s.c_str(), &e); if (!e || *e || !finite(r)) return false; v = r; return true; }
inline std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> o; size_t a = 0; for (;;) { const size_t p = s.find(d, a); o.push_back(s.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; } return o; }
inline bool versionOk(const std::string& v) {     // "1.x.y" (format major 1)
    std::vector<std::string> p = split(v, '.'); if (p.size() != 3) return false;
    long long a = 0, b = 0, c = 0; return fullLL(p[0], a) && fullLL(p[1], b) && fullLL(p[2], c) && a == FORMAT && b >= 0 && c >= 0;
}

// ---- the record ------------------------------------------------------------------------------------------------------------------
//   TRP|ver|kind|session|sweep_t|tr_t|side|pivot_t|pivot_px|back|lots|avg_px|share_pct|candle_vol|side_vol|ratio|normal|normal_state|atr
//   TRS|ver|kind|session|sweep_t|side|t|status|fuel|still
//   TRD|ver|session|traps
static const char* const REC_HEAD = "# lsTrapped record: TRP|ver|kind|session|sweep_t|tr_t|side|pivot_t|pivot_px|back|lots|avg_px|share_pct|candle_vol|side_vol|ratio|normal|normal_state|atr ; TRS|ver|kind|session|sweep_t|side|t|status|fuel|still ; TRD|ver|session|traps";
inline std::string g12(double v) { char b[48]; std::snprintf(b, sizeof b, "%.12g", v); return b; }
inline std::string trpLine(const Trap& T, double tick) {
    std::string o = std::string("TRP|") + VERSION + "|" + kindName(T.kind) + "|" + std::to_string(T.sess) + "|" + std::to_string(T.sweepT) + "|" + std::to_string(T.trT) + "|"
        + (T.side == SELLERS ? "S" : "B") + "|" + std::to_string(T.pivT) + "|" + g12((double)T.pivPx * tick) + "|" + std::to_string(T.back) + "|" + std::to_string(T.lots) + "|"
        + g12(T.avgT * tick) + "|" + std::to_string(T.sharePct) + "|" + std::to_string(T.candleVol) + "|" + std::to_string(T.sideVol) + "|";
    char r[32]; std::snprintf(r, sizeof r, "%.1f", T.ratio);
    return o + r + "|" + std::to_string(T.normal) + "|" + (T.building ? "building" : "hist") + "|" + g12(T.atrT * tick);
}
inline std::string trsLine(const Trap& T) {
    return std::string("TRS|") + VERSION + "|" + kindName(T.kind) + "|" + std::to_string(T.sess) + "|" + std::to_string(T.sweepT) + "|" + (T.side == SELLERS ? "S" : "B") + "|"
        + std::to_string(T.endT) + "|" + statusName(T.status) + "|" + std::to_string((long long)std::llround(T.fuelNow)) + "|" + std::to_string(T.stillNow);
}
inline std::string trdLine(long long sess, int traps) { return std::string("TRD|") + VERSION + "|" + std::to_string(sess) + "|" + std::to_string(traps); }

struct Record {
    std::vector<Trap> traps;                       // TRP lines (kind sweep), first line per key wins; TRS ends applied
    std::set<long long> doneSess;                  // TRD lines
    long long bad = 0, old = 0, otherKind = 0, ends = 0;
};
// one file's lines; tick = the chart's tick (prices back to ticks); a garbled / old / other-kind line is skipped and counted
inline void parseLines(const std::vector<std::string>& lines, double tick, Record& R) {
    std::map<std::string, size_t> at; for (size_t i = 0; i < R.traps.size(); ++i) at[R.traps[i].key()] = i;
    std::vector<std::vector<std::string> > trs;
    for (size_t li = 0; li < lines.size(); ++li) {
        std::string s = lines[li]; while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
        if (s.empty() || s[0] == '#') continue;
        const std::vector<std::string> f = split(s, '|');
        if (f[0] != "TRP" && f[0] != "TRS" && f[0] != "TRD") { ++R.bad; continue; }
        if (f.size() < 2 || !versionOk(f[1])) { ++R.old; continue; }
        if (f[0] == "TRD") { long long ss = 0, n = 0; if (f.size() != 4 || !fullLL(f[2], ss) || !fullLL(f[3], n) || ss <= 0 || n < 0) { ++R.bad; continue; } R.doneSess.insert(ss); continue; }
        if (f.size() < 3) { ++R.bad; continue; }
        const int kind = kindOf(f[2]);
        if (kind < 0) { ++R.bad; continue; }
        if (kind != KIND_SWEEP) { ++R.otherKind; continue; }          // room for later kinds: never misread
        if (f[0] == "TRS") { if (f.size() != 10) { ++R.bad; continue; } trs.push_back(f); continue; }
        if (f.size() != 19) { ++R.bad; continue; }
        Trap T; T.kind = kind; T.fromRecord = true;
        long long back = 0, share = 0; double piv = 0, avg = 0, ratio = 0, atr = 0;
        bool ok = fullLL(f[3], T.sess) && fullLL(f[4], T.sweepT) && fullLL(f[5], T.trT) && (f[6] == "S" || f[6] == "B") && fullLL(f[7], T.pivT) && fullD(f[8], piv)
               && fullLL(f[9], back) && fullLL(f[10], T.lots) && fullD(f[11], avg) && fullLL(f[12], share) && fullLL(f[13], T.candleVol) && fullLL(f[14], T.sideVol)
               && fullD(f[15], ratio) && fullLL(f[16], T.normal) && (f[17] == "hist" || f[17] == "building") && fullD(f[18], atr);
        ok = ok && T.sess > 0 && T.sweepT > 0 && T.trT >= T.sweepT && T.lots > 0 && T.normal > 0 && back >= 1 && back <= 3 && share >= 0 && share <= 100 && ratio >= 0 && atr > 0 && T.candleVol >= T.lots;
        long long pk = 0; ok = ok && toTicks(piv, tick, pk) && avg > 0 && finite(avg / tick);
        if (!ok) { ++R.bad; continue; }
        T.side = f[6] == "S" ? SELLERS : BUYERS; T.pivPx = pk; T.back = (int)back; T.sharePct = (int)share; T.building = f[17] == "building";
        T.avgT = round2(avg / tick); T.ratio = round1(ratio); T.atrT = round2(atr / tick);
        if (at.count(T.key())) continue;                                        // the first line of a key wins
        at[T.key()] = R.traps.size(); R.traps.push_back(T);
    }
    for (size_t i = 0; i < trs.size(); ++i) {
        const std::vector<std::string>& f = trs[i]; long long ss = 0, sw = 0, t = 0, fu = 0, st = 0;
        const int status = statusOf(f[7]);
        if (!fullLL(f[3], ss) || !fullLL(f[4], sw) || (f[5] != "S" && f[5] != "B") || !fullLL(f[6], t) || status <= ACTIVE || !fullLL(f[8], fu) || !fullLL(f[9], st) || t < sw || fu < 0 || st < 0) { ++R.bad; continue; }
        Trap probe; probe.sweepT = sw; probe.side = f[5] == "S" ? SELLERS : BUYERS; probe.kind = KIND_SWEEP;
        std::map<std::string, size_t>::iterator it = at.find(probe.key());
        if (it == at.end()) { ++R.bad; continue; }
        Trap& T = R.traps[it->second]; if (T.recEnd) continue;                  // the first end wins
        T.recEnd = true; T.recStatus = status; T.recEndT = t; ++R.ends;
    }
}

// ---- normal: the median trap lots of the last 20 sessions with a record (>= 5), else today's traps so far -------------------------
struct Hist {
    std::map<long long, std::vector<long long> > sess;      // session -> its trap lots (a session with a record, traps or none)
    void setSession(long long s, const std::vector<long long>& lots) { sess[s] = lots; }
    // (normal, building) for a trap of session s; today = today's trap lots so far including this one
    std::pair<long long, bool> normalFor(long long s, const std::vector<long long>& today, const Params& P) const {
        std::vector<long long> pool; int n = 0;
        for (std::map<long long, std::vector<long long> >::const_reverse_iterator it = sess.rbegin(); it != sess.rend() && n < P.histSessions; ++it) {
            if (it->first >= s) continue;
            ++n; pool.insert(pool.end(), it->second.begin(), it->second.end());
        }
        if (n >= P.minSessions && !pool.empty()) return std::make_pair(std::max(1LL, medianOf(pool)), false);
        return std::make_pair(std::max(1LL, medianOf(today)), true);
    }
    int sessionsBefore(long long s, const Params& P) const { int n = 0; for (std::map<long long, std::vector<long long> >::const_reverse_iterator it = sess.rbegin(); it != sess.rend() && n < P.histSessions; ++it) if (it->first < s) ++n; return n; }
};

// ---- the engine: closed candles in time order, each once --------------------------------------------------------------------------
struct Engine {
    Params P; Hist H;
    std::map<std::string, Trap> known;            // the record (key -> trap): the record wins
    std::vector<Trap> traps;                       // every decided trap (all sessions fed), in decision order
    std::vector<std::string> newLines;             // TRP / TRS / TRD lines to append (not yet in the record)
    long long candidates = 0, unqualified = 0, noCells = 0, lastT = 0; std::string lastReason = "no candle yet";
    // session state
    struct RB { long long t, o, h, l, c; bool cellsOk; long long ask, bid; std::vector<Cell> cells; };
    struct Piv { int idx; long long px; int dir; int conf; };
    struct Cand { Trap T; int idx; };
    long long sess = -1; bool rthOpen = false; std::vector<RB> rth; std::vector<Piv> piv; std::set<long long> used; std::vector<Cand> pend;
    std::vector<long long> todayLots; std::set<long long> doneSess; int sessTraps = 0;
    std::vector<Bar> win;                          // the last 14+ candles (ATR), any session

    void loadRecord(const Record& R) {
        for (size_t i = 0; i < R.traps.size(); ++i) known[R.traps[i].key()] = R.traps[i];
        doneSess.insert(R.doneSess.begin(), R.doneSess.end());
    }
    double atrBefore() const {                     // mean true range of the last 14 candles fed (before the candle being judged)
        double s = 0; int m = 0;
        for (int i = (int)win.size() - 1; i >= 0 && m < 14; --i, ++m) {
            const Bar& b = win[(size_t)i]; double tr = (double)(b.h - b.l);
            if (i > 0) { const double pc = (double)win[(size_t)i - 1].c; tr = std::max(tr, std::max(std::fabs((double)b.h - pc), std::fabs((double)b.l - pc))); }
            s += tr;
        }
        return m ? s / m : 0;
    }
    void endSession() {
        if (sess < 0) return;
        for (size_t i = 0; i < traps.size(); ++i) {
            Trap& T = traps[i]; if (T.sess != sess || T.status != ACTIVE) continue;
            if (T.recEnd) { applyRecEnd(T); continue; }
            T.status = ENDED; T.endT = T.lastT; newLines.push_back(trsLine(T));
        }
        pend.clear();
        if (rthOpen) {
            H.setSession(sess, todayLots);
            if (!doneSess.count(sess)) { newLines.push_back(trdLine(sess, sessTraps)); doneSess.insert(sess); }
        }
        rthOpen = false;
    }
    void applyRecEnd(Trap& T) { T.status = T.recStatus; T.endT = T.recEndT; }
    // still offside at a close: lots whose fill the close has not moved back through
    static long long stillAt(const Trap& T, long long c) { long long s = 0; for (size_t i = 0; i < T.fills.size(); ++i) if (T.sg() * (c - T.fills[i].first) > 0) s += T.fills[i].second; return s; }
    void burn(Trap& T, const RB& b) {
        if (T.status != ACTIVE) return;
        if (T.recEnd && b.t > T.recEndT) { applyRecEnd(T); return; }
        if (T.recEnd && b.t == T.recEndT && T.recStatus == FREED) { applyRecEnd(T); return; }
        T.lastC = b.c; T.lastT = b.t;
        if ((double)T.sg() * ((double)b.c - T.avgT) < 0) {
            T.status = FREED; T.endT = b.t; T.stillNow = stillAt(T, b.c);
            if (T.recEnd) applyRecEnd(T); else newLines.push_back(trsLine(T));
            return;
        }
        const long long still = stillAt(T, b.c);
        const long long flow = b.cellsOk ? (T.side == BUYERS ? b.bid - b.ask : b.ask - b.bid) : 0;
        const double cover = T.candleVol > 0 ? (double)T.lots / (double)T.candleVol * (double)std::max(0LL, flow) : 0;
        T.fuelNow = std::max(0.0, std::min((double)still, T.fuelNow - cover)); T.stillNow = still;
        FuelPt p; p.t = b.t; p.fuel = T.fuelNow; p.still = still; T.fuel.push_back(p);
        const double floorLots = std::min(P.spentX * (double)T.normal, P.smallSpentFrac * (double)T.lots);   // 1.0x normal, or half the trap if lower
        if (T.recEnd && b.t == T.recEndT) { applyRecEnd(T); return; }
        if (T.fuelNow < floorLots - 1e-9) {
            T.status = SPENT; T.endT = b.t;
            if (T.recEnd) applyRecEnd(T); else newLines.push_back(trsLine(T));
        }
    }
    void decide(Cand& C, const RB& qual) {
        Trap T = C.T; T.trT = qual.t;
        std::map<std::string, Trap>::const_iterator k = known.find(T.key());
        if (k != known.end()) {                                    // the record wins: its numbers, its end
            const Trap& R = k->second;
            T.trT = R.trT; T.pivT = R.pivT; T.pivPx = R.pivPx; T.back = R.back; T.lots = R.lots; T.avgT = R.avgT; T.sharePct = R.sharePct; T.candleVol = R.candleVol;
            T.sideVol = R.sideVol; T.ratio = R.ratio; T.normal = R.normal; T.building = R.building; T.atrT = R.atrT; T.fromRecord = true;
            T.recEnd = R.recEnd; T.recStatus = R.recStatus; T.recEndT = R.recEndT;
        } else {
            std::vector<long long> today = todayLots; today.push_back(T.lots);
            const std::pair<long long, bool> nm = H.normalFor(T.sess, today, P);
            T.normal = nm.first; T.building = nm.second; T.ratio = round1((double)T.lots / (double)T.normal);
            newLines.push_back(trpLine(T, tickForLines));
        }
        todayLots.push_back(T.lots); ++sessTraps;
        // fuel at the sweep close
        const RB& sb = rth[(size_t)C.idx];
        const long long still0 = stillAt(T, sb.c);
        T.fuelNow = (double)std::min(still0, T.lots); T.stillNow = still0; T.lastC = sb.c; T.lastT = sb.t;
        FuelPt p; p.t = sb.t; p.fuel = T.fuelNow; p.still = still0; T.fuel.push_back(p);
        traps.push_back(T);
        lastReason = std::string(T.side == SELLERS ? "sellers" : "buyers") + " trapped " + commas(T.lots) + " lots (" + g12(T.ratio) + "x)";
    }
    double tickForLines = 0.25;                    // the chart's tick (record prices); set by the host

    void feed(const Bar& b) {
        if (b.t <= lastT) return;                  // each candle once, in order
        lastT = b.t;
        if (b.sess != sess) { endSession(); sess = b.sess; rth.clear(); piv.clear(); used.clear(); pend.clear(); todayLots.clear(); sessTraps = 0; }
        if (!b.rth) {
            if (rthOpen) endSession();             // the RTH close: active traps end, the session counts for the normal
            win.push_back(b); if (win.size() > 40) win.erase(win.begin());
            return;
        }
        rthOpen = true;
        RB r; r.t = b.t; r.o = b.o; r.h = b.h; r.l = b.l; r.c = b.c; r.cellsOk = b.cellsOk; r.ask = r.bid = 0; r.cells = b.cells;
        for (size_t k = 0; k < b.cells.size(); ++k) { r.ask += b.cells[k].ask; r.bid += b.cells[k].bid; }
        const int i = (int)rth.size();
        rth.push_back(r);
        // 1. candidates from the candle before: qualify on this (next) candle, else drop
        std::vector<Cand> keep;
        for (size_t k = 0; k < pend.size(); ++k) {
            Cand& C = pend[k];
            if ((double)C.T.sg() * ((double)r.c - C.T.avgT) >= P.qualAtr * C.T.atrT - 1e-9) decide(C, r);
            else { ++unqualified; lastReason = "a sweep did not close 0.25 ATR beyond the trapped average (2 candles)"; }
        }
        pend.swap(keep);
        // 2. the fuel of every active trap of this session (this candle burns)
        for (size_t k = 0; k < traps.size(); ++k) if (traps[k].sess == sess && traps[k].trT < r.t) burn(traps[k], r);
        // the traps decided on THIS candle (qualified on the next candle) burn it too
        for (size_t k = 0; k < traps.size(); ++k) if (traps[k].sess == sess && traps[k].trT == r.t && traps[k].sweepT < r.t && traps[k].fuel.size() == 1) burn(traps[k], r);
        // 3. this candle as a sweep
        const double atr = atrBefore();
        for (int dir = 1; dir >= -1; dir -= 2) {
            std::vector<const Piv*> conf; for (size_t k = 0; k < piv.size(); ++k) if (piv[k].dir == dir && piv[k].conf < i) conf.push_back(&piv[k]);
            const int n = (int)conf.size(); const Piv* best = nullptr; int bestBack = 0;
            for (int k = 0; k < P.maxBack && k < 3 && k < n; ++k) {
                const Piv& p = *conf[(size_t)(n - 1 - k)];
                bool elig = true; for (int q = 0; q < k && elig; ++q) elig = dir > 0 ? p.px > conf[(size_t)(n - 1 - q)]->px : p.px < conf[(size_t)(n - 1 - q)]->px;
                if (!elig) continue;
                if (dir > 0 ? !(r.h > p.px) : !(r.l < p.px)) continue;
                bool intact = true; for (int q = p.idx + 1; q <= i - 2 && intact; ++q) intact = dir > 0 ? rth[(size_t)q].h <= p.px : rth[(size_t)q].l >= p.px;
                if (!intact) continue;
                if (used.count((long long)p.idx * 2 + (dir > 0 ? 1 : 0))) continue;
                if (!best || (dir > 0 ? p.px > best->px : p.px < best->px)) { best = &p; bestBack = k + 1; }
            }
            if (!best) continue;
            const bool buyers = dir > 0;
            if (buyers ? !(r.c < r.o && r.c < best->px) : !(r.c > r.o && r.c > best->px)) continue;   // red back below the high / green back above the low
            if (!r.cellsOk) { ++noCells; lastReason = "a sweep candle had no volume at price"; continue; }
            Cand C; Trap& T = C.T; C.idx = i;
            T.kind = KIND_SWEEP; T.sess = sess; T.sweepT = r.t; T.side = buyers ? BUYERS : SELLERS; T.pivT = rth[(size_t)best->idx].t; T.pivPx = best->px; T.back = bestBack;
            long long lots = 0; double pq = 0;
            for (size_t k = 0; k < r.cells.size(); ++k) {
                const Cell& c = r.cells[k]; const long long q = buyers ? c.ask : c.bid;
                if (q <= 0 || (buyers ? c.px < best->px : c.px > best->px)) continue;
                T.fills.push_back(std::make_pair(c.px, q)); lots += q; pq += (double)c.px * (double)q;
            }
            if (lots <= 0) continue;
            T.lots = lots; T.avgT = round2(pq / (double)lots); T.candleVol = r.ask + r.bid; T.sideVol = buyers ? r.ask : r.bid;
            T.sharePct = T.sideVol > 0 ? (int)std::llround(100.0 * (double)lots / (double)T.sideVol) : 0;
            T.atrT = round2(atr);
            if (!(T.atrT > 0)) continue;
            used.insert((long long)best->idx * 2 + (dir > 0 ? 1 : 0)); ++candidates;
            if ((double)T.sg() * ((double)r.c - T.avgT) >= P.qualAtr * T.atrT - 1e-9) decide(C, r);
            else pend.push_back(C);
        }
        // 4. the pivot this candle confirms (the candle before it)
        if (i >= 2) {
            const RB& a = rth[(size_t)i - 2]; const RB& m = rth[(size_t)i - 1];
            if (m.h > a.h && m.h >= r.h) { Piv p; p.idx = i - 1; p.px = m.h; p.dir = 1; p.conf = i; piv.push_back(p); }
            if (m.l < a.l && m.l <= r.l) { Piv p; p.idx = i - 1; p.px = m.l; p.dir = -1; p.conf = i; piv.push_back(p); }
        }
        win.push_back(b); win.back().cells.clear(); if (win.size() > 40) win.erase(win.begin());
    }
    // traps in the record that the candles did not reproduce (e.g. the chart's volume at price changed): drawn as recorded, no fuel line
    std::vector<Trap> recordOnly() const {
        std::set<std::string> have; for (size_t i = 0; i < traps.size(); ++i) have.insert(traps[i].key());
        std::vector<Trap> o;
        for (std::map<std::string, Trap>::const_iterator it = known.begin(); it != known.end(); ++it)
            if (!have.count(it->first) && it->second.trT <= lastT) { Trap T = it->second; if (T.recEnd) { T.status = T.recStatus; T.endT = T.recEndT; } o.push_back(T); }
        return o;
    }
};

// ---- text -----------------------------------------------------------------------------------------------------------------------
typedef std::function<std::string(long long)> TimeFn;   // a bar END -> "9:03" (12-hour, Central)
inline std::string sideWord(const Trap& T) { return T.side == SELLERS ? "Sellers" : "Buyers"; }
inline std::string spikeLabel(const Trap& T) { char r[32]; std::snprintf(r, sizeof r, "%.1fx", T.ratio); return commas(T.lots) + " \xB7 " + r; }
inline std::string endText(const Trap& T, const TimeFn& hm) {
    if (T.status == FREED) return "freed " + hm(T.endT);
    if (T.status == SPENT) return "fuel spent " + hm(T.endT);
    if (T.status == ENDED) return "session closed, fuel left " + commas((long long)std::llround(T.fuelNow)) + " (est.)";
    return "fuel left " + commas((long long)std::llround(T.fuelNow)) + " (est.)";
}
// the one message line (top-left of the pane): the most recent big trap of the session
inline std::string message(const Trap& T, const TimeFn& hm) {
    char r[32]; std::snprintf(r, sizeof r, "%.1fx", T.ratio);
    std::string s = hm(T.trT) + " " + sideWord(T) + " trapped " + commas(T.lots) + " lots (" + r + ") " + (T.side == SELLERS ? "below the " : "above the ") + hm(T.pivT) + (T.side == SELLERS ? " low" : " high") + " - " + endText(T, hm);
    if (T.status == ACTIVE) s += T.side == SELLERS ? " - buying to come." : " - selling to come.";
    else s += ".";
    return s;
}
inline double tickValueOf(const std::string& m) {   // $ per tick, full-size contracts (the host prefers the chart's SYM_VALTICK)
    return m == "ES" ? 12.5 : m == "NQ" ? 5.0 : m == "CL" ? 10.0 : m == "GC" ? 10.0 : m == "HG" ? 12.5 : m == "NG" ? 10.0 : (m == "EU" || m == "6E") ? 6.25 : 0.0;
}
// the hover (his style: the numbers first, then question / what it is / how it works / how to use / example), <= 12 rows of <= 125 chars
inline std::vector<std::string> hoverRows(const Trap& T, double tick, double tickValue, const TimeFn& hm, double bigX) {
    std::vector<std::string> v; char b[200];
    const bool sel = T.side == SELLERS;
    std::snprintf(b, sizeof b, "%s trapped%s - %s lots at avg %s (%.1fx normal)", sideWord(T).c_str(), T.ratio >= bigX ? "" : " (small)", commas(T.lots).c_str(), pxText(T.avgT, tick, true).c_str(), T.ratio); v.push_back(b);
    std::snprintf(b, sizeof b, "Share: %d%% of the candle's %s  |  swept the %s %s %s (%d swing%s back)", T.sharePct, sel ? "selling" : "buying", hm(T.pivT).c_str(), sel ? "low" : "high",
                  pxText((double)T.pivPx, tick).c_str(), T.back, T.back == 1 ? "" : "s"); v.push_back(b);
    const double uw = (double)T.sg() * ((double)T.lastC - T.avgT);
    const long long uwT = (long long)std::floor(uw + 0.5);
    const double atrX = T.atrT > 0 ? uw / T.atrT : 0;
    const long long pct = T.lots > 0 ? (long long)std::llround(100.0 * (double)T.stillNow / (double)T.lots) : 0;
    const long long pain = tickValue > 0 && uwT > 0 ? (long long)std::llround((double)T.stillNow * (double)uwT * tickValue) : 0;
    if (T.status == ACTIVE) std::snprintf(b, sizeof b, "Underwater now: %lld ticks (%.1f ATR)  |  still trapped %lld%%  |  pain $%s", uwT, atrX, pct, commas(pain).c_str());
    else std::snprintf(b, sizeof b, "At the end (%s): %lld ticks (%.1f ATR) underwater  |  still trapped %lld%%  |  pain $%s", hm(T.endT).c_str(), uwT, atrX, pct, commas(pain).c_str());
    v.push_back(b);
    std::snprintf(b, sizeof b, "Fuel left: %s lots (est.)  |  status: %s", commas((long long)std::llround(T.status == FREED ? 0.0 : T.fuelNow)).c_str(),
                  T.status == ACTIVE ? "active" : T.status == FREED ? ("freed " + hm(T.endT)).c_str() : T.status == SPENT ? ("spent " + hm(T.endT)).c_str() : "session closed"); v.push_back(b);
    if (T.building) std::snprintf(b, sizeof b, "Normal: building - %s lots = the median of today's traps so far (needs 5 sessions of records)", commas(T.normal).c_str());
    else std::snprintf(b, sizeof b, "Normal: %s lots = the median trap of the last 20 sessions  |  big = %.1fx+", commas(T.normal).c_str(), bigX);
    v.push_back(b);
    v.push_back(std::string("Question: who is stuck on the wrong side of this failed sweep, and how much ") + (sel ? "buying" : "selling") + " must they still do?");
    v.push_back(sel ? "What: aggressive SELL lots at / below the swept low in a candle that closed green, back above it - the breakdown failed."
                    : "What: aggressive BUY lots at / above the swept high in a candle that closed red, back below it - the breakout failed.");
    v.push_back("How: their lots and average fill from the candle's volume at price; fuel = lots still offside, burned by their est. covering.");
    v.push_back(sel ? "Use: a big trap (bright) with fuel left = their covering (buying) is likely still to come; freed = they got out."
                    : "Use: a big trap (bright) with fuel left = their covering (selling) is likely still to come; freed = they got out.");
    v.push_back("Example: ES Oct 5 9:03 sellers trapped 3,140 lots below the 8:48 low (normal 445 = 7.1x); price rallied as they covered.");
    return v;
}

} // namespace trp
#endif
