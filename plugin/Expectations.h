#ifndef LS_EXPECTATIONS_H
#define LS_EXPECTATIONS_H
// Expectations.h 0.1.0 (2026-10-26) -- awareness_plus item 1: every READ says what it EXPECTS next if it is right, with a deadline,
// and the coordinator (lsContext) checks each one on the bars that follow and posts EXPECTATION MET / BROKEN as DATA-derived METRIC
// facts. SDK-free, unit-tested on Linux.
//
//   kind              the read                         MET when (within K bars after the read's bar)        BROKEN when
//   LEAVE             turn / collision / absorb / ...  price leaves the read's close by N x ATR its way      the read's extreme is taken first,
//                                                                                                           or the deadline passes ("slow")
//   NO_NEW_EXTREME    stage 3 (exhaustion)             the deadline passes with no new extreme               a bar trades beyond the extreme
//   HOLD_ABOVE        HELD bid / support               the deadline passes with every bar's high >= ref      a bar's high < ref (traded wholly below)
//   HOLD_BELOW        HELD offer / resistance          the deadline passes with every bar's low <= ref       a bar's low > ref
// Decided once, no look-ahead: an expectation registered for bar t is only checked on bars with t' > t; a bar that both reaches the
// target and takes the extreme counts as BROKEN (the order inside the bar is unknown -> the cautious answer).
// The verdict is a METRIC (formula above, from D.bar_h / D.bar_l only) - never an opinion; the read it judges stays a READ.
#include <map>
#include <string>
#include <vector>
#include <cstdio>

namespace apx {

static const char* const VERSION = "0.1.0";

enum class Kind { LEAVE, NO_NEW_EXTREME, HOLD_ABOVE, HOLD_BELOW };
enum class State { OPEN, MET, BROKEN };
inline const char* kindName(Kind k) { return k == Kind::LEAVE ? "leave" : k == Kind::NO_NEW_EXTREME ? "no_new_ext" : k == Kind::HOLD_ABOVE ? "hold_above" : "hold_below"; }
inline const char* stateName(State s) { return s == State::OPEN ? "open" : s == State::MET ? "MET" : "BROKEN"; }

struct Bar { long long t = 0; double h = 0, l = 0, c = 0; };

struct Exp {
    std::string src, read, ctx;          // who expected it (DP / STAGE / HODLOD ...), the read word (absorb / stage3 / held ...), the context cell
    Kind kind = Kind::LEAVE; int side = 0;   // +1 up (bull read), -1 down
    long long t0 = 0;                    // the read's bar (decided at its close)
    double ref = 0, ext = 0, atr = 0, N = 0; int K = 0;
    double target = 0;                   // LEAVE: ref + side * N * atr
    bool news = false;                   // registered inside a news window (the nightly separates these)
    State st = State::OPEN; long long tDone = 0; int bars = 0; std::string why;
    double mfe = 0;                      // best move its way in ATR so far (the nightly learns N from it)
};

inline std::string fmt(double x) { char b[32]; std::snprintf(b, sizeof(b), "%.6g", x); return b; }

// one expectation from a read; false when it cannot be judged (no side for a LEAVE, unknown ATR, K <= 0)
inline bool make(Exp& E, const std::string& src, const std::string& read, const std::string& ctx, Kind kind, int side,
                 long long t0, double ref, double ext, double atr, int K, double N, bool news) {
    if (K <= 0 || K > 60) return false;
    if (kind == Kind::LEAVE && (side == 0 || !(atr > 0) || !(N > 0))) return false;
    if (kind == Kind::NO_NEW_EXTREME && side == 0) return false;
    E = Exp(); E.src = src; E.read = read; E.ctx = ctx; E.kind = kind; E.side = side; E.t0 = t0; E.ref = ref; E.ext = ext;
    E.atr = atr; E.K = K; E.N = N; E.news = news; E.target = ref + side * N * atr;
    return true;
}

// apply one CLOSED bar after the read; returns true if this bar resolved it
inline bool step(Exp& E, const Bar& b) {
    if (E.st != State::OPEN || b.t <= E.t0) return false;
    ++E.bars;
    if (E.atr > 0 && E.side != 0) { const double m = E.side > 0 ? (b.h - E.ref) / E.atr : (E.ref - b.l) / E.atr; if (m > E.mfe) E.mfe = m; }
    const bool last = E.bars >= E.K;
    switch (E.kind) {
    case Kind::LEAVE: {
        const bool took = E.side > 0 ? b.l < E.ext : b.h > E.ext;
        const bool hit = E.side > 0 ? b.h >= E.target : b.l <= E.target;
        if (took) { E.st = State::BROKEN; E.why = hit ? "extreme taken (same bar as target)" : "extreme taken"; }
        else if (hit) { E.st = State::MET; E.why = "left " + fmt(E.N) + " ATR in " + std::to_string(E.bars) + " bars"; }
        else if (last) { E.st = State::BROKEN; E.why = "slow: no " + fmt(E.N) + " ATR in " + std::to_string(E.K) + " bars"; }
        break; }
    case Kind::NO_NEW_EXTREME: {
        const bool took = E.side > 0 ? b.l < E.ext : b.h > E.ext;
        if (took) { E.st = State::BROKEN; E.why = "new extreme"; }
        else if (last) { E.st = State::MET; E.why = "no new extreme in " + std::to_string(E.K) + " bars"; }
        break; }
    case Kind::HOLD_ABOVE:
        if (b.h < E.ref) { E.st = State::BROKEN; E.why = "bar wholly below " + fmt(E.ref); }
        else if (last) { E.st = State::MET; E.why = "held " + std::to_string(E.K) + " bars"; }
        break;
    case Kind::HOLD_BELOW:
        if (b.l > E.ref) { E.st = State::BROKEN; E.why = "bar wholly above " + fmt(E.ref); }
        else if (last) { E.st = State::MET; E.why = "held " + std::to_string(E.K) + " bars"; }
        break;
    }
    if (E.st != State::OPEN) { E.tDone = b.t; return true; }
    return false;
}

// the book of open expectations for one chart; one per (src, t0) - a re-post of the same read never registers twice
struct Book {
    std::vector<Exp> open;
    std::vector<Exp> recent;             // the last resolved ones (newest last, capped)
    long long lastBar = 0;
    int met = 0, broken = 0, metNews = 0, brokenNews = 0;
    static constexpr size_t MAX_OPEN = 64, MAX_RECENT = 20;

    bool add(const Exp& E) {
        for (const auto& o : open) if (o.src == E.src && o.t0 == E.t0) return false;
        for (const auto& o : recent) if (o.src == E.src && o.t0 == E.t0) return false;
        if (E.t0 < lastBar) return false;                 // a read for a bar already checked: too late to be fair
        if (open.size() >= MAX_OPEN) open.erase(open.begin());
        open.push_back(E); return true;
    }
    // every newly CLOSED bar exactly once (bars at or before lastBar are ignored); returns what resolved on it
    std::vector<Exp> onBar(const Bar& b) {
        std::vector<Exp> done;
        if (b.t <= lastBar) return done;
        lastBar = b.t;
        for (size_t i = 0; i < open.size();) {
            if (step(open[i], b)) {
                Exp E = open[i]; done.push_back(E);
                if (E.st == State::MET) { ++met; if (E.news) ++metNews; } else { ++broken; if (E.news) ++brokenNews; }
                recent.push_back(E); if (recent.size() > MAX_RECENT) recent.erase(recent.begin());
                open.erase(open.begin() + (long)i);
            } else ++i;
        }
        return done;
    }
    // the board facts (class METRIC): counts + the newest verdict; ids are src:t0
    std::map<std::string, std::string> kv() const {
        std::map<std::string, std::string> o;
        o["class"] = "METRIC"; o["exp_open"] = std::to_string(open.size()); o["exp_met"] = std::to_string(met); o["exp_broken"] = std::to_string(broken);
        o["exp_met_news"] = std::to_string(metNews); o["exp_broken_news"] = std::to_string(brokenNews);
        if (!recent.empty()) {
            const Exp& E = recent.back();
            o["last"] = std::string(stateName(E.st)) + " " + E.src + " " + E.read + " (" + E.why + ")";
            o["last_t0"] = std::to_string(E.t0); o["last_done"] = std::to_string(E.tDone); o["last_state"] = stateName(E.st);
        } else o["last"] = "?";
        std::string ids; for (const auto& E : open) { if (!ids.empty()) ids += ","; ids += E.src + ":" + E.read; }
        o["open_ids"] = ids.empty() ? "-" : ids;
        return o;
    }
};

// record line per verdict, for the nightly:  EXP|1|<src>|<read>|<ctx>|<kind>|<side>|<t0>|<tDone>|<state>|<K>|<N>|<ref>|<ext>|<atr>|<mfe atr>|<news>|<why>
inline std::string recordLine(const Exp& E) {
    return "EXP|1|" + E.src + "|" + E.read + "|" + E.ctx + "|" + kindName(E.kind) + "|" + std::to_string(E.side) + "|" + std::to_string(E.t0) + "|" +
           std::to_string(E.tDone) + "|" + stateName(E.st) + "|" + std::to_string(E.K) + "|" + fmt(E.N) + "|" + fmt(E.ref) + "|" + fmt(E.ext) + "|" +
           fmt(E.atr) + "|" + fmt(E.mfe) + "|" + (E.news ? "1" : "0") + "|" + E.why;
}

}  // namespace apx
#endif
