#ifndef LS_STOPRUN_H
#define LS_STOPRUN_H
// StopRun.h (1.0.0, 2026-10-13) -- SDK-free rules of the STOP RUN signal (R) of lsBidAskStats 1.4.0. Unit-tested on Linux.
// Rassul 2026-10-13 (1:30 PM): R is reserved for the stop run; (1:36 PM) build it in BidAskStats, shown "R?" until the nightly
// stop-run study proves it per market.
//
// THE RULE (native, decided once, never repaints)
//   1. A KEY LEVEL: ONH / ONL, PDH / PDL, PFH / PFL, LonHI / LonLO from LRA-KeyLevels-<MKT>.csv (fresh <= 18 h; read by NearLevels.h).
//      Highs (ONH PDH PFH LonHI) are approached from BELOW, lows (ONL PDL PFL LonLO) from ABOVE.
//   2. THE BREAK BAR: the prior closed bar closed inside (below a high / above a low) and this bar trades >= 1 tick beyond the level.
//   3. THE BURST, on the break side, within the break bar: an S sweep, or a SPIKE (the bar drawn bright: >= Spike vs average x that
//      side's average AND >= Spike vs prior biggest x the biggest bar of the prior candles - 2.0x / 1.5x / 10 by default).
//      Upside break = the buyers' (ask, right) bar; downside break = the sellers' (bid, left) bar. No burst = no R (logged nowhere).
//   4. THE RECLAIM: a close back inside (below the high / above the low) on the break bar itself or within the next N bars
//      (N from LRA-StopRun-<MKT>.csv, default 2). Still outside after N bars = the break held: no R.
//   5. THE MARK is placed when the reclaim bar CLOSES, at the bottom of the break bar's attacking-side bar:
//      failed upside break  = RED R   (buyers trapped, bearish)
//      failed downside break = GREEN R (sellers trapped, bullish)
//      "R?" until the market's file says SHOW,on; SHOW,off = not drawn (still logged). The show state is stored with the event.
//   Only bars that CLOSED while the indicator was running are judged (no look-ahead from levels that formed later, e.g. LonHI
//   before London closed); a restart brings back the events from the session record.
//
// LRA-StopRun-<MKT>.csv (written nightly by the stop-run study):  VERSION,1 / N,<0..10> / SHOW,<on|?|off>
//   Missing file, wrong VERSION, a bad N or SHOW value, or a line that is not KEY,VALUE = the defaults (N 2, SHOW ?), with a note.
//   Unknown KEY,VALUE lines (upper-case keys) are ignored so the study can add fields.
#include "NearLevels.h"
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace sr {

static const char* const VERSION = "1.0.0";
enum Show { SHOW_OFF = 0, SHOW_Q = 1, SHOW_ON = 2 };
static const int N_DEFAULT = 2, N_MAX = 10;

// ---- the per-market params file ---------------------------------------------------------------------------------------------
struct Params { int n = N_DEFAULT; int show = SHOW_Q; std::string source = "defaults (no file)"; std::string note; };
inline const char* showText(int s) { return s == SHOW_ON ? "on" : s == SHOW_OFF ? "off" : "?"; }
inline Params parseParams(const std::vector<std::string>& lines) {
    Params d; d.source = "defaults (garbled file)";
    Params p; p.source = "file";
    bool version = false, any = false;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string ln = lines[i];
        while (!ln.empty() && (ln.back() == ' ' || ln.back() == '\t')) ln.pop_back();
        if (ln.empty() || ln[0] == '#') continue;
        const size_t c = ln.find(',');
        if (c == std::string::npos || c == 0 || ln.find(',', c + 1) != std::string::npos) { d.note = "line " + std::to_string(i + 1) + " is not KEY,VALUE"; return d; }
        const std::string k = ln.substr(0, c), v = ln.substr(c + 1);
        for (char ch : k) if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')) { d.note = "line " + std::to_string(i + 1) + ": bad key"; return d; }
        if (!any) { any = true; if (k != "VERSION" || v != "1") { d.note = "first line must be VERSION,1"; return d; } version = true; continue; }
        if (k == "VERSION") { d.note = "VERSION twice"; return d; }
        if (k == "N") { long long n = 0; if (!nl::parseWhole(v, 0, N_MAX, n)) { d.note = "N: bad value '" + v.substr(0, 12) + "' (0.." + std::to_string(N_MAX) + ")"; return d; } p.n = (int)n; }
        else if (k == "SHOW") { if (v == "on") p.show = SHOW_ON; else if (v == "?") p.show = SHOW_Q; else if (v == "off") p.show = SHOW_OFF; else { d.note = "SHOW: bad value '" + v.substr(0, 12) + "' (on, ?, off)"; return d; } }
    }
    if (!version) { d.note = "empty file"; return d; }
    return p;
}
// re-read at most every 5 s and only when the file changed; a garbled file = the defaults (never the last good one: the study
// rewrites the whole file, so garbage means "not trusted tonight")
struct ParamCache { long long sig = -2, checked = -1; Params p; };
inline bool refresh(ParamCache& C, const std::string& path, long long nowMs) {
    if (C.checked >= 0 && nowMs >= C.checked && nowMs - C.checked < 5000) return false;
    C.checked = nowMs;
    const long long sig = path.empty() ? -1 : nl::fileSig(path);
    if (sig == C.sig) return false;
    C.sig = sig;
    Params n;
    if (sig < 0) { n.source = "defaults (no file)"; }
    else {
        std::vector<std::string> lines;
        if (!nl::readBoundedLines(path, lines, 16 * 1024, 64, 256)) { n.source = "defaults (garbled file)"; n.note = "unreadable or too big"; }
        else n = parseParams(lines);
    }
    C.p = n; return true;
}

// ---- levels ---------------------------------------------------------------------------------------------------------------
struct KeyLevel { std::string name; double px = 0; int dir = 0; int rank = 9; };   // dir +1 = a high, -1 = a low
inline int keyDir(const std::string& n, int& rank) {
    static const char* const H[4] = { "PFH", "PDH", "ONH", "LonHI" }, * const L[4] = { "PFL", "PDL", "ONL", "LonLO" };
    for (int i = 0; i < 4; ++i) { if (n == H[i]) { rank = i; return 1; } if (n == L[i]) { rank = i; return -1; } }
    return 0;
}
// the key levels only (not gamma / MenthorQ), from the cache NearLevels.h keeps (nl::levels() loads it); fresh <= 18 h
inline std::vector<KeyLevel> keyLevels(const nl::Cache& C, long long wall) {
    std::vector<KeyLevel> out;
    const nl::Source& S = C.key;
    if (!(S.sig >= 0 && S.at > 0 && wall - S.at <= 18 * 3600 && S.at - wall <= 600)) return out;
    for (size_t i = 0; i < S.v.size(); ++i) {
        KeyLevel k; k.name = S.v[i].name; k.px = S.v[i].px; k.dir = keyDir(k.name, k.rank);
        if (k.dir != 0 && nl::finite(k.px) && k.px > 0) out.push_back(k);
    }
    return out;
}

// ---- the detector ---------------------------------------------------------------------------------------------------------
// one CLOSED bar as the detector sees it. side 0 = bid (sellers hit, left bar), 1 = ask (buyers lifted, right bar)
struct Bar {
    long long t = 0; double h = 0, l = 0, c = 0;
    bool spike[2] = {false, false}, sweep[2] = {false, false};
    bool ratioOk[2] = {false, false}; double ratio[2] = {0, 0};
};
struct Event {
    long long breakT = 0, reclaimT = 0;
    int dir = 0;                    // +1 = failed UPSIDE break (buyers trapped, red R), -1 = failed DOWNSIDE break (sellers trapped, green R)
    std::string level; double px = 0;
    int ticks = 0;                  // how far beyond the level price went, break bar .. reclaim bar
    int bars = 0;                   // bars after the break bar to close back inside (0 = the break bar itself)
    bool spike = false, sweep = false; bool ratioOk = false; double ratio = 0;   // the burst on the attacking side, break bar
    int show = SHOW_Q, n = N_DEFAULT;
    int side() const { return dir > 0 ? 1 : 0; }
};
inline bool inside(int dir, double px, double c, double tick) { const double e = tick * 1e-6; return dir > 0 ? c < px - e : c > px + e; }
inline std::string barsText(int b) { return b <= 0 ? std::string("same bar") : b == 1 ? std::string("1 bar") : std::to_string(b) + " bars"; }
inline std::string burstText(const Event& e) {
    std::string k = e.spike && e.sweep ? "spike + sweep" : e.spike ? "spike" : "sweep";
    return (e.ratioOk ? nl::ratioText(e.ratio) + " " : std::string()) + k;
}
// the plain-English message: "Stop run above PDH - buyers trapped (spike 4.2x, back inside in 1 bar)"
inline std::string message(const Event& e) {
    const std::string q = e.show == SHOW_ON ? "" : "?";
    return "Stop run" + q + (e.dir > 0 ? " above " : " below ") + e.level + (e.dir > 0 ? " - buyers trapped" : " - sellers trapped")
         + " (" + burstText(e) + ", back inside " + (e.bars <= 0 ? std::string("on the same bar") : "in " + barsText(e.bars)) + ")";
}
inline std::string markText(const Event& e) { return e.show == SHOW_ON ? "R" : "R?"; }

struct Detector {
    struct Pending { KeyLevel lv; long long breakT = 0; int k = 0; double ext = 0; bool burst = false; Bar b; int show = SHOW_Q, n = N_DEFAULT; };
    std::map<std::string, Pending> pending;     // per level (name), at most one break being watched
    long long lastT = 0; bool havePrev = false; double prevC = 0;
    long long breaks = 0, noBurst = 0, held = 0;
    // feed the CLOSED bars strictly in time order, each once. prevClose = the close of the bar before b (any closed bar, known data).
    // Returns the events DECIDED by this bar's close (the reclaim bar), at most one per break bar and direction.
    std::vector<Event> onClose(const Bar& b, bool havePrevClose, double prevClose, const std::vector<KeyLevel>& levels, double tick, const Params& P) {
        std::vector<Event> out;
        if (b.t <= lastT || !(tick > 0) || !nl::finite(b.h) || !nl::finite(b.l) || !nl::finite(b.c)) return out;
        lastT = b.t;
        auto excursion = [&](int dir, double px, const Bar& x) { return dir > 0 ? x.h - px : px - x.l; };
        auto fire = [&](const Pending& p, int k, long long reclaimT) {
            Event e; e.breakT = p.breakT; e.reclaimT = reclaimT; e.dir = p.lv.dir; e.level = p.lv.name; e.px = p.lv.px;
            e.ticks = (int)std::llround(p.ext / tick); e.bars = k; e.show = p.show; e.n = p.n;
            const int s = e.side(); e.spike = p.b.spike[s]; e.sweep = p.b.sweep[s]; e.ratioOk = p.b.ratioOk[s]; e.ratio = p.b.ratio[s];
            out.push_back(e);
        };
        // 1. breaks being watched: this bar is bar k after the break
        for (std::map<std::string, Pending>::iterator it = pending.begin(); it != pending.end();) {
            Pending& p = it->second; ++p.k;
            const double x = excursion(p.lv.dir, p.lv.px, b); if (x > p.ext) p.ext = x;
            if (inside(p.lv.dir, p.lv.px, b.c, tick)) { if (p.burst) fire(p, p.k, b.t); pending.erase(it++); continue; }
            if (p.k >= p.n) { ++held; pending.erase(it++); continue; }
            ++it;
        }
        // 2. new breaks on this bar
        if (havePrevClose && nl::finite(prevClose)) for (size_t i = 0; i < levels.size(); ++i) {
            const KeyLevel& L = levels[i];
            if (pending.count(L.name)) continue;
            if (!inside(L.dir, L.px, prevClose, tick)) continue;                         // approached from the right side
            const double x = excursion(L.dir, L.px, b);
            if (x < tick * (1 - 1e-6)) continue;                                          // >= 1 tick beyond
            ++breaks;
            Pending p; p.lv = L; p.breakT = b.t; p.k = 0; p.ext = x; p.b = b; p.show = P.show; p.n = P.n;
            const int s = L.dir > 0 ? 1 : 0; p.burst = b.spike[s] || b.sweep[s];
            if (!p.burst) ++noBurst;
            if (inside(L.dir, L.px, b.c, tick)) { if (p.burst) fire(p, 0, b.t); continue; }   // back inside on the break bar itself
            if (p.n <= 0) { ++held; continue; }
            pending[L.name] = p;
        }
        // one event per break bar and direction: the highest-ranked level (PF > PD > ON > Lon), the others named after it
        std::vector<Event> merged;
        for (size_t i = 0; i < out.size(); ++i) {
            bool done = false;
            for (size_t j = 0; j < merged.size() && !done; ++j) if (merged[j].breakT == out[i].breakT && merged[j].dir == out[i].dir) {
                int ri = 9, rj = 9; keyDir(out[i].level, ri); keyDir(merged[j].level.substr(0, merged[j].level.find('/')), rj);
                if (ri < rj) { const std::string other = merged[j].level; merged[j] = out[i]; merged[j].level = out[i].level + "/" + other.substr(0, other.find('/')); }
                else if (merged[j].level.find('/') == std::string::npos) merged[j].level += "/" + out[i].level;
                done = true;
            }
            if (!done) merged.push_back(out[i]);
        }
        return merged;
    }
};

// ---- the session record line: SR|break_time|reclaim_time|ct|dir|level|level_px|ticks|bars|burst_x100|burst|show|n|message --
static const char* const SR_HEAD = "# SR|break_time|reclaim_time|ct_time|dir|level|level_px|break_ticks|bars_to_reclaim|burst_x100|burst|show|n|message";
inline std::string clean(const std::string& s, size_t n) { std::string o; for (char c : s) { if (o.size() >= n) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline std::string eventLine(const Event& e, const std::string& ct) {
    char px[32]; std::snprintf(px, sizeof(px), "%.6f", e.px);
    const long long bx = e.ratioOk && nl::finite(e.ratio) && e.ratio < 1e6 ? (long long)std::llround(e.ratio * 100) : -1;
    return "SR|" + std::to_string(e.breakT) + "|" + std::to_string(e.reclaimT) + "|" + clean(ct, 12) + "|" + (e.dir > 0 ? "up" : "down") + "|" + clean(e.level, 40)
         + "|" + px + "|" + std::to_string(e.ticks) + "|" + std::to_string(e.bars) + "|" + (bx < 0 ? std::string("-") : std::to_string(bx))
         + "|" + (e.spike && e.sweep ? "both" : e.spike ? "spike" : "sweep") + "|" + showText(e.show) + "|" + std::to_string(e.n) + "|" + clean(message(e), 300);
}
inline bool parseEvent(const std::string& ln, Event& out, std::string* ct = nullptr) {
    if (ln.compare(0, 3, "SR|") != 0 || ln.size() > 1024) return false;
    const std::vector<std::string> c = nl::fields(ln);
    if (c.size() != 14) return false;
    Event e; long long v = 0; double d = 0;
    if (!nl::parseWhole(c[1], 1, 32503680000LL, v)) return false;
    e.breakT = v;
    if (!nl::parseWhole(c[2], 1, 32503680000LL, v) || v < e.breakT) return false;
    e.reclaimT = v;
    if (c[3].size() > 12) return false;
    if (c[4] == "up") e.dir = 1; else if (c[4] == "down") e.dir = -1; else return false;
    if (c[5].empty() || c[5].size() > 40) return false;
    e.level = c[5];
    if (!nl::parseReal(c[6], d) || d <= 0) return false;
    e.px = d;
    if (!nl::parseWhole(c[7], 0, 1000000, v)) return false;
    e.ticks = (int)v;
    if (!nl::parseWhole(c[8], 0, N_MAX, v)) return false;
    e.bars = (int)v;
    if (c[9] == "-") e.ratioOk = false; else { if (!nl::parseWhole(c[9], 0, 100000000, v)) return false; e.ratioOk = true; e.ratio = (double)v / 100.0; }
    if (c[10] == "both") e.spike = e.sweep = true; else if (c[10] == "spike") e.spike = true; else if (c[10] == "sweep") e.sweep = true; else return false;
    if (c[11] == "on") e.show = SHOW_ON; else if (c[11] == "?") e.show = SHOW_Q; else if (c[11] == "off") e.show = SHOW_OFF; else return false;
    if (!nl::parseWhole(c[12], 0, N_MAX, v)) return false;
    e.n = (int)v;
    if (ct) *ct = c[3];
    out = e; return true;
}

} // namespace sr
#endif
