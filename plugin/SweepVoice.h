// SweepVoice.h -- spoken sweep alerts for lsHodLod 2.37.0, SDK-free (tests_hodlod/t_hl2370)
//
// (2.37.0, Rassul 2026-11-19 19:29 CT "ok.. do it" on the recommendation: speak when a Sw (sweep) mark is CONFIRMED at its candle close)
//   message: "<market>, sweep, <level>, <up|down>"  e.g. "NQ, sweep, PDL, up"  (a bullish sweep of a low = up; a bearish sweep of a high = down)
//   source:  lsFlexLevels\RevRead\wy-<MKT>-<spb>.txt "stamp|Sw|dir|lv|px|strength" lines (stamp = candle END, 1904 epoch, local), the same file
//            HodLod already reads for the Now line; only the Sw marks drawn on his chart: a NAMED key level (the marks' level set), strength
//            V<n> / R<n> with n >= 10 (LevelMarks 1.5.0 SW_MIN), the Sw colour rule (a green candle for up, red for down) when the candle is known
//   clips:   pre-rendered WAVs, one per market x level x side: lsFlexLevels\Voice\sw-<MKT>-<LV>-<up|down>.wav (Kokoro af_heart, the LRA voice)
//   rules:   once per market + level + side per session (5 PM to 5 PM); never for marks already there when the chart started (only candles that
//            END after this chart's start); RTH only (the market's RTH, the bar's minute inside [open, close)); not older than 5 min (a frozen
//            feed catching up stays quiet); one DLL serves every chart: ONE Hub per DLL, so several charts / timeframes of a market speak once;
//            a queue so two clips never talk over each other; a missing clip plays a short system sound instead (counted, logged).
#pragma once
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <set>
#include <string>
#include <vector>

namespace svx {

static const char* const VERSION = "1.1.0";   // (1.1.0, Rassul 2026-11-20 12:22 CT "yes cob and all market sweep voice alerts") + Cobs: offerCob, clips cob-<MKT>-<up|down>.wav "<market>, cob, <up|down>"; every chart offers every market (lsHodLod 2.41.3)
static const long long E1904 = 2082844800LL;
static const int ST_MIN = 10;          // LevelMarks 1.5.0 SW_MIN: a Sw needs its attempt's BVMax or BRMax >= 10
static const long long MAX_AGE = 300;  // s: a mark older than this when first seen is not spoken (feed catching up)
static const size_t QUEUE_MAX = 16;
static const long long GAP_MS = 350;   // between two clips
static const long long FALLBACK_MS = 1200;

struct Sw { long long t = 0; int dir = 0; std::string lv; int st = 0; };   // t = candle END, local secs (unix-naive local); lv = canonical code

// the level codes a Sw can carry (the marks' set, LevelMarks / NowLevels pri >= 0, hours 2h / 3h) -> the canonical file code
inline std::string canon(const std::string& n) {
    if (n == "LonLO") return "LLO";
    if (n == "LonHI") return "LHI";
    if (n == "AsiaLO") return "ALO";
    if (n == "AsiaHI") return "AHI";
    static const char* const K[] = { "ONL", "ONH", "LLO", "LHI", "ALO", "AHI", "PDH", "PDL", "PFH", "PFL", "VAH", "VAL", "PWH", "PWL", "CWH", "CWL",
                                     "PHH", "PHL", "2hH", "2hL", "3hH", "3hL" };
    for (const char* k : K) if (n == k) return k;
    return std::string();
}
inline const std::vector<std::string>& levels() {
    static const std::vector<std::string> v = { "ONL", "ONH", "LLO", "LHI", "ALO", "AHI", "PDH", "PDL", "PFH", "PFL", "VAH", "VAL", "PWH", "PWL", "CWH", "CWL",
                                                "PHH", "PHL", "2hH", "2hL", "3hH", "3hL" };
    return v;
}
inline const std::vector<std::string>& markets() { static const std::vector<std::string> v = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }; return v; }
// what is SAID (checked with the Kokoro phonemizer: letters spelled; a bare "A" is said as a schwa by Kokoro, so VAH / VAL use "eh")
inline std::string levelSpoken(const std::string& c) {
    if (c == "LLO") return "London low";
    if (c == "LHI") return "London high";
    if (c == "ALO") return "Asia low";
    if (c == "AHI") return "Asia high";
    if (c == "VAH") return "V eh H";
    if (c == "VAL") return "V eh L";
    if (c.size() == 3 && c[1] == 'h' && (c[0] == '2' || c[0] == '3')) return std::string(c[0] == '2' ? "two" : "three") + " hour " + (c[2] == 'H' ? "high" : "low");
    std::string o; for (size_t i = 0; i < c.size(); i++) { if (i) o += ' '; o += c[i]; } return o;   // "PDL" -> "P D L"
}
inline std::string marketSpoken(const std::string& m) { if (m == "EU") return "Euro"; return m.size() == 2 ? std::string(1, m[0]) + " " + m[1] : m; }
// what is SHOWN (status line / log): his chart's names
inline std::string levelShown(const std::string& c) { return c == "LLO" ? "LonLO" : c == "LHI" ? "LonHI" : c == "ALO" ? "AsiaLO" : c == "AHI" ? "AsiaHI" : c; }
inline std::string marketShown(const std::string& m) { return m == "EU" ? "6E" : m; }
inline std::string sideWord(int dir) { return dir > 0 ? "up" : "down"; }
inline std::string text(const std::string& m, const std::string& c, int dir) { return marketShown(m) + ", sweep, " + levelShown(c) + ", " + sideWord(dir); }
inline std::string spoken(const std::string& m, const std::string& c, int dir) { return marketSpoken(m) + ", sweep, " + levelSpoken(c) + ", " + sideWord(dir) + "."; }
inline std::string fileName(const std::string& m, const std::string& c, int dir) { return "sw-" + m + "-" + c + "-" + sideWord(dir) + ".wav"; }
// (1.1.0) the Cob clips: one per market x side
inline std::string cobText(const std::string& m, int dir) { return marketShown(m) + ", Cob, " + sideWord(dir); }
inline std::string cobSpoken(const std::string& m, int dir) { return marketSpoken(m) + ", cob, " + sideWord(dir) + "."; }
inline std::string cobFile(const std::string& m, int dir) { return "cob-" + m + "-" + sideWord(dir) + ".wav"; }

inline bool toLL(const std::string& s, long long& v) { if (s.empty() || s.size() > 20) return false; errno = 0; char* e = nullptr; const long long x = std::strtoll(s.c_str(), &e, 10); if (errno || !e || *e) return false; v = x; return true; }
// "V45" / "R99+" -> 45 / 99; "" or garbage -> 0
inline int strengthOf(const std::string& s) {
    if (s.size() < 2 || (s[0] != 'V' && s[0] != 'R')) return 0;
    std::string d = s.substr(1); if (!d.empty() && d.back() == '+') d.pop_back();
    long long n = 0; if (!toLL(d, n) || n < 0 || n > 999) return 0; return (int)n;
}
// the drawn Sw marks of a wy file whose candle ends in [from, to]
inline std::vector<Sw> parseSw(const std::string& text, long long from, long long to, size_t maxBytes = 512 * 1024) {
    std::vector<Sw> out; if (text.size() > maxBytes) return out;
    size_t a = 0; bool head = false;
    while (a < text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string l = text.substr(a, e - a); a = e + 1;
        if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.empty()) continue;
        if (!head) { if (l.compare(0, 5, "WY|1|") != 0) return std::vector<Sw>(); head = true; continue; }
        std::vector<std::string> f; size_t p = 0; while (true) { const size_t q = l.find('|', p); f.push_back(l.substr(p, q == std::string::npos ? std::string::npos : q - p)); if (q == std::string::npos) break; p = q + 1; }
        if (f.size() != 6 || f[1] != "Sw") continue;
        long long st = 0, d = 0; if (!toLL(f[0], st) || !toLL(f[2], d) || (d != 1 && d != -1)) continue;
        const long long t = st - E1904; if (t < from || t > to) continue;
        const std::string c = canon(f[3]); if (c.empty()) continue;
        const int n = strengthOf(f[5]); if (n < ST_MIN) continue;
        Sw s; s.t = t; s.dir = (int)d; s.lv = c; s.st = n; out.push_back(s);
    }
    return out;
}
// 5 PM local starts a session
inline long long sessionOf(long long t) { const long long day = t >= 0 ? t / 86400 : (t - 86399) / 86400; const long long five = day * 86400 + 17 * 3600; return t >= five ? five : five - 86400; }
// the bar ending at t is inside RTH [openMin, closeMin) (minutes of the day, local)
inline bool inRth(long long tEnd, int openMin, int closeMin) {
    long long s = (tEnd - 1) % 86400; if (s < 0) s += 86400; const long long m = s / 60; return m >= openMin && m < closeMin;
}
// a WAV file's play time (ms) from its first bytes (RIFF / fmt / data); -1 = not a PCM WAV
inline long long wavMs(const std::string& b, long long fileSize) {
    if (b.size() < 12 || b.compare(0, 4, "RIFF") != 0 || b.compare(8, 4, "WAVE") != 0) return -1;
    auto u32 = [&](size_t i) { return (unsigned long long)(unsigned char)b[i] | ((unsigned long long)(unsigned char)b[i + 1] << 8) | ((unsigned long long)(unsigned char)b[i + 2] << 16) | ((unsigned long long)(unsigned char)b[i + 3] << 24); };
    size_t i = 12; unsigned long long rate = 0;
    while (i + 8 <= b.size()) {
        const unsigned long long n = u32(i + 4);
        if (b.compare(i, 4, "fmt ") == 0 && i + 16 <= b.size()) rate = u32(i + 8 + 8);   // byte rate
        if (b.compare(i, 4, "data") == 0) { if (!rate) return -1; unsigned long long data = n; const long long rest = fileSize - (long long)(i + 8); if (rest >= 0 && (unsigned long long)rest < data) data = (unsigned long long)rest; return (long long)(data * 1000ULL / rate); }
        i += 8 + (size_t)n + (n & 1); if (n > (1ULL << 31)) return -1;
    }
    return -1;
}

struct Clip { std::string key, mkt, file, text; long long t = 0; };
// ONE per DLL (all charts): what was said this session, the queue, the play clock
struct Hub {
    std::set<std::string> said, bad; std::deque<Clip> q; long long busyUntilMs = 0;
    int spoken = 0, fallbacks = 0, skippedOld = 0, skippedRth = 0, skippedColour = 0, dropped = 0; std::string last, lastMissing;
    // offer the Sw marks one chart sees. chartStart = when this chart started (local secs); colour(t, dir): 1 matches, 0 wrong colour, -1 unknown
    template <class C> int offer(const std::string& mkt, long long chartStart, long long nowLocal, const std::vector<Sw>& marks, int openMin, int closeMin, C colour) {
        int n = 0;
        for (const Sw& m : marks) {
            if (m.t <= chartStart) continue;                                   // already there when the chart (re)started
            const std::string key = mkt + "|" + m.lv + "|" + std::to_string(m.dir) + "|" + std::to_string(sessionOf(m.t));
            if (said.count(key)) continue;
            const std::string once = key + "|" + std::to_string(m.t);   // a quiet mark is counted once and does NOT use up its level's alert
            if (!inRth(m.t, openMin, closeMin)) { if (bad.insert(once).second) ++skippedRth; continue; }   // (an overnight PHL sweep must not silence the RTH one)
            if (nowLocal - m.t > MAX_AGE) { if (bad.insert(once).second) ++skippedOld; continue; }
            if (colour(m.t, m.dir) == 0) { if (bad.insert(once).second) ++skippedColour; continue; }   // a Sw candle of the wrong colour is not drawn (LevelMarks 1.3.0); a later right one may still speak
            said.insert(key);
            if (q.size() >= QUEUE_MAX) { ++dropped; continue; }
            Clip c; c.key = key; c.mkt = mkt; c.file = fileName(mkt, m.lv, m.dir); c.text = text(mkt, m.lv, m.dir); c.t = m.t; q.push_back(c); ++n;
        }
        if (said.size() > 4096) said.clear();
        if (bad.size() > 4096) bad.clear();   // bounded (a session has a few dozen)
        return n;
    }
    // (1.1.0) one Cob (its Rev candle END t, local secs): each Cob once (market + side + candle), the same start / RTH / age rules as a sweep
    int offerCob(const std::string& mkt, long long chartStart, long long nowLocal, long long t, int dir, int openMin, int closeMin) {
        if (t <= chartStart || (dir != 1 && dir != -1)) return 0;
        const std::string key = mkt + "|COB|" + std::to_string(dir) + "|" + std::to_string(t);
        if (said.count(key)) return 0;
        if (!inRth(t, openMin, closeMin)) { if (bad.insert(key).second) ++skippedRth; return 0; }
        if (nowLocal - t > MAX_AGE) { if (bad.insert(key).second) ++skippedOld; return 0; }
        said.insert(key);
        if (q.size() >= QUEUE_MAX) { ++dropped; return 0; }
        Clip c; c.key = key; c.mkt = mkt; c.file = cobFile(mkt, dir); c.text = cobText(mkt, dir); c.t = t; q.push_back(c); return 1;
    }
    // play the next clip when the last one is done. play(file) -> its play time in ms, or -1 = missing / failed; fallback() -> ms
    template <class P, class F> bool pump(long long nowMs, P play, F fallback) {
        if (q.empty() || nowMs < busyUntilMs) return false;
        const Clip c = q.front(); q.pop_front();
        long long ms = play(c.file);
        if (ms < 0) { ms = fallback(); if (ms < 0) ms = FALLBACK_MS; ++fallbacks; lastMissing = c.file; }
        busyUntilMs = nowMs + ms + GAP_MS; ++spoken; last = c.text;
        return true;
    }
};

} // namespace svx
