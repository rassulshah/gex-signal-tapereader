#ifndef LS_DELTA_PROFILE_HOURLY_ENGINE_H
#define LS_DELTA_PROFILE_HOURLY_ENGINE_H

// DeltaProfileHourlyEngine.h 1.0.0 -- lsDeltaProfileHourly (DLT-H): the SDK-free engine.
//   in : CLOSED 1-minute bars with volume at price, in time order, each tagged with the END of the host chart bar (hourly) it
//        belongs to (IRT bar time = bar END: a minute ending 13:01 covers [13:00, 13:01) and belongs to the hour ending 14:00).
//   per closed minute (and only then): the host bars of the last 24 trading hours are rebuilt from their closed minutes, the
//        2.5.4 profile / node / zone rules run on them (DeltaProfileHourlyCore.h), and every absorption candidate is tracked
//        exactly as Delta Profile 2.5.4 does - frozen zone and bar from its first appearance, decided by the first CLOSED host
//        bar after it that closes beyond it in its own colour (red below / green above), never undone.
//   out: the snapshot to draw + append-only record lines (F first seen, D decided) for the per-session record file.
// Decisions use only minutes up to the one being processed (no look-ahead); the same minute sequence always gives the same
// lines, so a restart that re-reads the record and re-plays the minutes equals one straight pass (tests: replay == live).
// All times are the Central wall clock as epoch seconds (the chart's getLocaltime), so 17:00 is the session change.
#include "DeltaProfileHourlyCore.h"
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <deque>
#include <functional>
#include <locale>
#include <sstream>

namespace delta_profile_hourly {
static const char* const DPH_VERSION = "1.0.0";
static const int REC_FIELDS = 23;
static const std::size_t REC_MAX_BYTES = 4u * 1024u * 1024u;   // checklist #33: a record file larger than this is not read
static const std::size_t REC_MAX_LATCHES = 600;
static const int HISTORY_SESSIONS = 10;                         // record files re-read at a restart (and kept drawn)

inline long long floorDivLL(long long a, long long b) { long long q = a / b, r = a % b; return q - (r < 0 ? 1 : 0); }
inline long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}
inline long long civilSec(int y, int mo, int d, int h, int mi, int s) {
    return civilDays(y, static_cast<unsigned>(mo), static_cast<unsigned>(d)) * 86400LL + h * 3600LL + mi * 60LL + s;
}
// the Globex session a bar END stamp belongs to: at or after 17:00 = the next day's session (Delta Profile 2.5.3 edge)
inline long long sessionKey(long long localSec) {
    const long long days = floorDivLL(localSec, 86400), sec = localSec - days * 86400;
    return days + (sec >= 17 * 3600 ? 1 : 0);
}

struct MinuteIn {
    long long te = 0, bucketEnd = 0;   // the minute's END and its host bar's END (Central clock seconds)
    double o = 0, h = 0, l = 0, c = 0, volume = 0;
    std::vector<Row> rows;             // tick key, ask - bid, volume (normalised by add())
    bool missing = false;              // IRT gave no volume at price for it (counted, makes the window provisional)
};
struct Agg {                           // one host bar, from its closed minutes so far
    long long end = 0; double o = 0, h = 0, l = 0, c = 0, volume = 0; bool has = false; int missing = 0;
    long long firstTe = 0, lastTe = 0; std::map<Tick, Row> rows;
};
struct Latch {
    Tick low = 0, high = 0; std::string code; bool support = false, sell = false;
    long long peakEnd = 0;             // the host bar where the node's delta peaked (its circle)
    double price = 0, net = 0, mult = 0; bool zone = false, provisional = false, logged = false;
    long long id = -1, tFirst = 0, tDecided = 0, decidedBy = 0;
};
struct JLine { long long session, te; std::string text; };

inline bool decidedCode(const std::string& c) { return !c.empty() && c.back() != '?'; }
inline bool sellNode(const std::string& c, bool sup) {
    return c[0] == 'A' && c.compare(0, 3, "Acc") != 0 ? sup : c[0] == 'I' ? !sup : c.compare(0, 3, "Dst") == 0;
}
inline std::string num(double v, const char* format) { char b[64]; std::snprintf(b, sizeof(b), format, v); return b; }
inline const char* recordHeader() {
    return "t_first|t_decided|code|side|bias|zone_lo|zone_hi|tick|net_delta|multiple|decided_by_t|version|event|root|id|peak_t|spb|price|lo_ticks|hi_ticks|node_side|source|provisional";
}
inline std::string recordName(const std::string& market, int spb, long long session) {
    return market + "-" + std::to_string(spb) + "-signals-" + std::to_string(session) + ".csv";
}
inline std::string recordLine(const Latch& L, char event, double tick, const std::string& root, int spb) {
    const bool d = event == 'D';
    std::string code = L.code;
    if (!d && (code.empty() || code.back() != '?')) code += '?';   // the F line is always the candidate ("A?")
    std::ostringstream o; o.imbue(std::locale::classic());
    o << (L.tFirst > 0 ? L.tFirst : 0) << '|' << (d ? L.tDecided : 0) << '|' << code << '|'
      << (L.support ? "support" : "resistance") << '|' << (L.support ? "bullish" : "bearish") << '|'
      << num(static_cast<double>(L.low) * tick, "%.10g") << '|' << num(static_cast<double>(L.high) * tick, "%.10g") << '|'
      << num(tick, "%.10g") << '|' << num(L.net, "%.0f") << '|' << num(L.mult, "%.2f") << '|' << (d ? L.decidedBy : 0) << '|'
      << DPH_VERSION << '|' << event << '|' << root << '|' << L.id << '|' << L.peakEnd << '|' << spb << '|'
      << num(L.price, "%.10g") << '|' << L.low << '|' << L.high << '|' << (L.sell ? "sell" : "buy") << '|'
      << (L.zone ? "zone" : "node") << '|' << (L.provisional ? 1 : 0);
    return o.str();
}
// checklist #4: strict full-token parsing - no blanks, no trailing junk, in range, finite
inline bool parseWhole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 24 || std::isspace(static_cast<unsigned char>(t[0]))) return false;
    errno = 0; char* e = NULL; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno != 0 || e != t.c_str() + t.size() || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool parseReal(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40 || std::isspace(static_cast<unsigned char>(t[0]))) return false;
    errno = 0; char* e = NULL; const double v = std::strtod(t.c_str(), &e);
    if (errno != 0 || e != t.c_str() + t.size() || !finite(v)) return false;
    out = v; return true;
}
inline std::vector<std::string> fields(const std::string& line) {
    std::vector<std::string> c; std::size_t a = 0;
    for (;;) {
        const std::size_t p = line.find('|', a);
        c.push_back(line.substr(a, p == std::string::npos ? std::string::npos : p - a));
        if (p == std::string::npos) break;
        a = p + 1;
    }
    return c;
}
struct RecordEvent { char event = 0; Latch latch; std::string root; };
inline bool parseRecordLine(const std::string& line, double chartTick, int spb, RecordEvent& r) {
    const std::vector<std::string> c = fields(line);
    if (static_cast<int>(c.size()) != REC_FIELDS || c[12].size() != 1 || (c[12][0] != 'F' && c[12][0] != 'D')) return false;
    Latch& L = r.latch; r.event = c[12][0]; r.root = c[13];
    if (r.root.empty() || r.root.size() > 16) return false;
    long long tf, td, by, id, pt, sp, lo, hi, pv;
    const long long T_MAX = 32503680000LL;                                  // year 3000
    if (!parseWhole(c[0], 0, T_MAX, tf) || !parseWhole(c[1], 0, T_MAX, td) || !parseWhole(c[10], 0, T_MAX, by) ||
        !parseWhole(c[14], 0, 100000000, id) || !parseWhole(c[15], 1, T_MAX, pt) || !parseWhole(c[16], 1, 86400 * 31, sp) ||
        !parseWhole(c[18], -9000000000000000LL, 9000000000000000LL, lo) || !parseWhole(c[19], -9000000000000000LL, 9000000000000000LL, hi) ||
        lo > hi || !parseWhole(c[22], 0, 1, pv))
        return false;
    if (sp != spb) return false;
    double zl, zh, tk, net, mult, price;
    if (!parseReal(c[5], zl) || !parseReal(c[6], zh) || !parseReal(c[7], tk) || !parseReal(c[8], net) || !parseReal(c[9], mult) ||
        !parseReal(c[17], price) || tk <= 0 || tk >= 1000 || mult < 0 || zl > zh) return false;
    const std::string& code = c[2];
    if (r.event == 'F' ? code != "A?" : (code != "A" && code != "I" && code != "Acc" && code != "Dst")) return false;
    if (r.event == 'D' ? (td == 0 || by == 0) : (td != 0 || by != 0)) return false;
    if (tf == 0) return false;                                               // checklist #34: no time = not trusted
    const bool support = c[3] == "support";
    if ((!support && c[3] != "resistance") || c[4] != (support ? "bullish" : "bearish")) return false;
    if ((c[20] != "buy" && c[20] != "sell") || (c[21] != "node" && c[21] != "zone")) return false;
    L.code = code; L.support = support; L.tFirst = tf; L.tDecided = td; L.decidedBy = by; L.id = id; L.peakEnd = pt;
    L.price = price; L.net = net; L.mult = mult; L.sell = c[20] == "sell"; L.zone = c[21] == "zone"; L.provisional = pv == 1;
    L.low = lo; L.high = hi;
    if (std::fabs(tk - chartTick) > 1e-9 * std::max(tk, chartTick)) {      // written on another tick size: the prices decide
        try { L.low = priceKey(zl, chartTick); L.high = priceKey(zh, chartTick); } catch (...) { return false; }
        if (L.low > L.high) return false;
    }
    return true;
}

class Engine {
public:
    Params P;
    std::string root, market; int spb = 0; double tick = 0;
    long long coverageFrom = LLONG_MIN;   // no minute before this was asked for: a host bar starting earlier is partial
    long long resumeAfter = LLONG_MIN;    // minutes at or before this were processed before a restart: aggregated only
    // state
    std::map<long long, Agg> aggs;        // host bars by END
    std::deque<MinuteIn> pending;
    long long lastAdded = LLONG_MIN, cursor = LLONG_MIN;
    std::vector<Latch> latches; long long nextId = 0;
    std::vector<JLine> journal;
    // the last snapshot (what is drawn)
    Snapshot snap; std::vector<long long> winEnds; long long snapTe = LLONG_MIN;
    bool ready = false, complete = false, provisional = false; std::string state = "no 1-minute data yet";
    // counters (status)
    long long builds = 0, added = 0, rejected = 0, missingMinutes = 0, linesF = 0, linesD = 0, faults = 0,
              seedLoaded = 0, seedRejected = 0;
    int consecutiveFaults = 0; bool quarantined = false; std::string fault, lastReject;

    Engine() {}
    Engine(const std::string& r, const std::string& m, int secondsPerBar, double tickSize, const Params& p = Params())
        : P(p), root(r), market(m), spb(secondsPerBar), tick(tickSize) {}

    // one CLOSED minute, strictly after the previous one; false (and counted) when out of order or malformed
    bool add(MinuteIn m) {
        std::string why;
        if (spb <= 0 || !(tick > 0)) why = "engine not configured";
        else if (m.te <= lastAdded) why = "not after the previous minute";
        else if (!(m.bucketEnd >= m.te && m.bucketEnd - m.te < spb)) why = "minute outside its host bar";
        else if (!m.missing) {
            if (!finite(m.o) || !finite(m.h) || !finite(m.l) || !finite(m.c) || m.h < m.l || m.o < m.l || m.o > m.h ||
                m.c < m.l || m.c > m.h) why = "invalid OHLC";
            else if (!finite(m.volume) || m.volume < 0) why = "invalid volume";
            else {
                Bar b; b.rows.swap(m.rows);
                try { normalize(b); } catch (const std::exception& e) { why = e.what(); }
                m.rows.swap(b.rows);
                double sum = 0; for (const Row& r : m.rows) sum += r.volume;
                if (why.empty() && sum > m.volume) m.volume = sum;
            }
        }
        if (!why.empty()) { ++rejected; lastReject = why; return false; }
        if (m.missing) { m.rows.clear(); ++missingMinutes; }
        lastAdded = m.te; ++added;
        pending.push_back(std::move(m));
        return true;
    }
    // process waiting minutes in order: at most maxSteps, and only while timeLeft() (checklist #18 / #40: bounded slices)
    int run(int maxSteps, const std::function<bool()>& timeLeft) {
        int n = 0;
        while (!pending.empty() && n < maxSteps && !quarantined && (n == 0 || timeLeft())) {
            MinuteIn m = std::move(pending.front()); pending.pop_front(); ++n;
            try { step(m); consecutiveFaults = 0; }
            catch (const std::exception& e) {
                ++faults; fault = e.what(); ready = false; state = std::string("calculation error: ") + e.what();
                if (++consecutiveFaults >= 3) { quarantined = true; state = "quarantined after 3 faults: " + fault; }   // #30
            }
        }
        return n;
    }
    // seed the book from one session's record text (only complete lines; first decided line of a key wins)
    void seedText(const std::string& text) {
        const std::size_t end = text.rfind('\n');
        if (end == std::string::npos) return;
        std::istringstream in(text.substr(0, end + 1)); std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#' || line.compare(0, 8, "t_first|") == 0) continue;
            RecordEvent r;
            if (!parseRecordLine(line, tick, spb, r)) { ++seedRejected; continue; }
            if (r.root != root) continue;
            Latch* at = find(r.latch);
            if (r.event == 'F') {
                if (at || latches.size() >= REC_MAX_LATCHES) continue;
                r.latch.tDecided = 0; r.latch.decidedBy = 0; r.latch.logged = false;
                latches.push_back(r.latch); ++seedLoaded; nextId = std::max(nextId, r.latch.id + 1);
            } else {
                if (!at) { ++seedRejected; continue; }
                if (at->logged) continue;
                at->code = r.latch.code; at->support = r.latch.support; at->tDecided = r.latch.tDecided;
                at->decidedBy = r.latch.decidedBy; at->logged = true;
            }
        }
    }
    // the record lines are written: drop them (checklist #6 - only after a flushed, good append)
    void journalWritten(std::size_t count) { journal.erase(journal.begin(), journal.begin() + static_cast<long>(std::min(count, journal.size()))); }
    // the newest minute whose lines are all written (the progress mark a restart resumes after)
    long long progressTe() const {
        long long p = cursor;
        for (const JLine& j : journal) p = std::min(p, j.te - 1);
        return p;
    }
    bool live(const Latch& L) const { return !winEnds.empty() && L.peakEnd >= winEnds.front(); }

private:
    Latch* find(const Latch& k) {
        for (Latch& L : latches)
            if (L.peakEnd == k.peakEnd && L.low == k.low && L.high == k.high && L.sell == k.sell) return &L;
        return nullptr;
    }
    void aggregate(const MinuteIn& m) {
        Agg& a = aggs[m.bucketEnd];
        a.end = m.bucketEnd; a.lastTe = m.te; if (!a.firstTe) a.firstTe = m.te;
        if (m.missing) { ++a.missing; return; }
        if (!a.has) { a.o = m.o; a.h = m.h; a.l = m.l; a.has = true; }
        a.h = std::max(a.h, m.h); a.l = std::min(a.l, m.l); a.c = m.c; a.volume += m.volume;
        for (const Row& r : m.rows) {
            Row& x = a.rows[r.key]; x.key = r.key; x.delta += r.delta; x.volume += r.volume;
            if (!finite(x.delta) || !finite(x.volume)) throw std::overflow_error("bar aggregation overflow");
        }
    }
    void step(const MinuteIn& m) {
        aggregate(m);
        cursor = m.te;
        // the window: walk back from this minute's host bar until the spans reach WINDOW_HOURS (trading time, bar by bar)
        std::map<long long, Agg>::iterator it = aggs.find(m.bucketEnd);
        if (it == aggs.end()) throw std::logic_error("minute's host bar not aggregated");
        const long long need = static_cast<long long>(P.windowHours) * 3600LL, zoneNeed = static_cast<long long>(P.zoneHours) * 3600LL;
        std::vector<const Agg*> win; long long sum = 0, oldestSpan = spb; std::size_t zoneCount = 0; long long zsum = 0;
        for (;;) {
            const bool first = it == aggs.begin();
            const long long span = first ? spb : std::min<long long>(spb, it->first - std::prev(it)->first);
            win.push_back(&it->second); sum += span; oldestSpan = span;
            if (zsum < zoneNeed) { zsum += span; ++zoneCount; }
            if (sum >= need || first) break;
            --it;
        }
        std::reverse(win.begin(), win.end());
        const bool partial = coverageFrom != LLONG_MIN && win.front()->end - oldestSpan < coverageFrom;
        complete = sum >= need && !partial;
        provisional = false;
        std::vector<Bar> bars; winEnds.clear();
        for (std::size_t i = 0; i < win.size(); ++i) {
            const Agg& a = *win[i];
            if (a.missing) provisional = true;
            if (!a.has) continue;                                   // a host bar with no readable minute yet
            Bar b; b.index = static_cast<int>(bars.size()); b.time = a.end; b.closed = a.end <= m.te;
            b.open = a.o; b.high = a.h; b.low = a.l; b.close = a.c;
            b.rows.reserve(a.rows.size());
            for (std::map<Tick, Row>::const_iterator r = a.rows.begin(); r != a.rows.end(); ++r) b.rows.push_back(r->second);
            bars.push_back(std::move(b)); winEnds.push_back(a.end);
        }
        const bool allowed = complete && !provisional;
        const bool record = complete && m.te > resumeAfter;
        // catch-up (a restart re-playing what was already recorded, or the warm-up before 24 h of minutes exist): only the
        // newest queued minute needs a picture - the others just add to their bars (cheap, and the result is identical)
        if (!record && !pending.empty() && (pending.front().te <= resumeAfter || !complete)) {
            if (!winEnds.empty()) aggs.erase(aggs.begin(), aggs.lower_bound(win.front()->end));
            return;
        }
        Snapshot candidate = build(bars, tick, allowed, std::min(zoneCount, bars.size()), P);
        ++builds; snapTe = m.te;
        // drop host bars older than this window (the window only moves forward)
        if (!winEnds.empty()) aggs.erase(aggs.begin(), aggs.lower_bound(win.front()->end));
        if (record) track(candidate, m.te, allowed);
        else if (complete) adoptCodes(candidate);                  // re-played minute (already recorded): show the record
        snap = std::move(candidate);
        ready = !snap.buckets.empty();
        state = !ready ? "no volume at price in the window" : !complete ? "short 1-minute history: no signals yet" :
                provisional ? "missing 1-minute volume at price: provisional" : "ready";
        pruneHistory(m.te);
    }
    long long endOf(int idx) const { return idx >= 0 && idx < static_cast<int>(winEnds.size()) ? winEnds[static_cast<std::size_t>(idx)] : 0; }
    void adoptCodes(Snapshot& c) {
        const Tick tpr = c.ticksPerRow;
        for (Node& n : c.nodes)
            for (const Latch& L : latches) {
                const Tick lo = n.bucket * tpr, hi = n.bucket * tpr + tpr - 1;
                if (live(L) && L.low <= hi && lo <= L.high && !n.state.code.empty() && L.sell == sellNode(n.state.code, n.state.support)) {
                    n.state.code = L.code; n.state.support = L.support; break;
                }
            }
    }
    // Delta Profile 2.5.2 / 2.5.4 tracking, unchanged: a candidate's zone and bar are frozen when it first appears
    void track(Snapshot& c, long long nowT, bool allowedNow) {
        auto trackOne = [&](Tick lo, Tick hi, std::string& code, bool& support, int peakBar, double price, int decidedBar,
                            double net, double mult, bool zone) {
            if (code.empty()) return;
            const bool mySell = sellNode(code, support);
            for (Latch& L : latches)
                if (live(L) && L.low <= hi && lo <= L.high && L.sell == mySell) {               // already tracked
                    if (!decidedCode(L.code) && decidedCode(code) && allowedNow) { L.code = code; L.support = support; L.decidedBy = endOf(decidedBar); }
                    code = L.code; support = L.support; return;
                }
            if (code[0] != 'A' || code.compare(0, 3, "Acc") == 0 || latches.size() >= REC_MAX_LATCHES || peakBar < 0 ||
                peakBar >= static_cast<int>(winEnds.size())) return;                              // only absorption starts a record
            Latch L; L.low = lo; L.high = hi; L.code = allowedNow ? code : (decidedCode(code) ? code + "?" : code); L.support = support;
            L.peakEnd = endOf(peakBar); L.price = price; L.sell = mySell; L.provisional = !allowedNow;
            L.id = nextId++; L.tFirst = nowT; L.net = net; L.mult = mult; L.zone = zone;
            if (decidedCode(L.code)) L.decidedBy = endOf(decidedBar);
            latches.push_back(L);
            journal.push_back(JLine{sessionKey(nowT), nowT, recordLine(L, 'F', tick, root, spb)}); ++linesF;
        };
        const Tick tpr = c.ticksPerRow;
        for (Node& n : c.nodes) {
            double net = 0;
            for (const Bucket& b : c.buckets) if (b.key == n.bucket) { net = b.delta; break; }
            trackOne(n.bucket * tpr, n.bucket * tpr + tpr - 1, n.state.code, n.state.support, n.state.peakBar, n.price,
                     n.state.decidedBar, net, n.ratio, false);
        }
        for (Zone& z : c.zones) {
            const double mult = c.meanAbsDelta > 0 ? std::fabs(z.net) / c.meanAbsDelta : 0;
            trackOne(z.low, z.high, z.code, z.support, z.peakBar, (static_cast<double>(z.low) + static_cast<double>(z.high)) * 0.5 * tick,
                     z.decidedBar, z.net, mult, true);
        }
        // decide the undecided live candidates from the CLOSED host bars after their bar (no bar after this minute is known)
        const double eps = tick * 1e-6;
        for (Latch& L : latches) {
            if (decidedCode(L.code) || !live(L)) continue;
            const double zl = static_cast<double>(L.low) * tick, zh = static_cast<double>(L.high) * tick;
            for (std::map<long long, Agg>::const_iterator a = aggs.upper_bound(L.peakEnd); a != aggs.end(); ++a) {
                if (a->first > nowT || !a->second.has) break;                                       // not closed yet
                const double o = a->second.o, cl = a->second.c;
                const bool redBelow = cl < zl - eps && cl < o, greenAbove = cl > zh + eps && cl > o;
                if (!redBelow && !greenAbove) continue;
                if (L.sell) { L.code = greenAbove ? "A" : "I"; L.support = greenAbove; }
                else        { L.code = redBelow ? "A" : "I"; L.support = !redBelow; }
                L.decidedBy = a->first;
                break;
            }
        }
        for (Latch& L : latches) {
            if (!decidedCode(L.code) || L.logged) continue;
            L.tDecided = nowT; L.logged = true;
            journal.push_back(JLine{sessionKey(nowT), nowT, recordLine(L, 'D', tick, root, spb)}); ++linesD;
        }
        adoptCodes(c);
    }
    void pruneHistory(long long nowT) {   // only what a restart would also re-read stays in memory (drawing only)
        const long long oldest = sessionKey(nowT) - HISTORY_SESSIONS + 1;
        latches.erase(std::remove_if(latches.begin(), latches.end(), [&](const Latch& L) {
            return !live(L) && sessionKey(L.tFirst) < oldest; }), latches.end());
    }
};
} // namespace delta_profile_hourly
#endif
