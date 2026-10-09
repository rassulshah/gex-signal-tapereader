#ifndef LS_CHARTVIEW_LOGIC_H
#define LS_CHARTVIEW_LOGIC_H
/********************************************************************************
 *  ChartViewLogic.h  --  the SDK-free half of lsChartView (1.0.0, 2026-10-09)
 *
 *  Everything ChartView decides without Investor/RT: which market a chart is, the watch-list file, when a snapshot is due,
 *  whether it changed, JSON text (escaping, numbers), size caps for the copied status files, and the snapshot JSON itself.
 *  Tested by test_chartview_logic.cpp (g++ on Linux, no SDK).
 ********************************************************************************/
#include "DealerLogic.h"          // dl::marketForRoot - the one market mapping every LRA plugin uses
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cctype>

namespace cvl {

static const char* const CV_VERSION_STR = "1.1.0";
static const int   MAX_LABELS      = 40;       // labels read per chart (file + extra)
static const int   MAX_ARRAYS      = 16;       // arrays read per label (0..15, stop at the first that fails)
static const int   MIN_BARS        = 20;
static const int   MAX_BARS        = 2000;
static const int   DEFAULT_BARS    = 300;
static const long long THROTTLE_MS = 10000;    // the forming bar: at most one snapshot every 10 s
static const size_t FILE_CAP       = 64 * 1024;    // each copied status / marks file
static const size_t FILES_TOTAL_CAP = 768 * 1024;  // all copied files together

// ---------------------------------------------------------------- market
inline std::string upper(const std::string& s) { std::string r; for (char c : s) r += (char)std::toupper((unsigned char)c); return r; }

// ES, NQ, CL, GC, HG, NG, EU (6E). Micros fall in by "contains": MES -> ES, MNQ -> NQ, MCL -> CL, MGC -> GC, M6E -> EU.
// Unknown roots keep their own (sanitised) name so a snapshot is still written, never an empty file name.
inline std::string marketOf(const std::string& root, const std::string& symbol)
{
    std::string m = dl::marketForRoot(root);
    if (m.empty()) m = dl::marketForRoot(symbol);
    if (!m.empty()) return m;
    std::string r; for (char c : upper(root.empty() ? symbol : root)) if (std::isalnum((unsigned char)c)) r += c;
    if (r.size() > 12) r.resize(12);
    return r.empty() ? std::string("UNKNOWN") : r;
}

// the bar size in seconds: the SDK's answer when it is sane, else the smallest positive gap between bar stamps
inline int periodOf(int sdkSeconds, const std::vector<long long>& stamps)
{
    if (sdkSeconds > 0 && sdkSeconds <= 7 * 86400) return sdkSeconds;
    long long best = 0;
    for (size_t i = 1; i < stamps.size(); i++) { long long d = stamps[i] - stamps[i - 1]; if (d > 0 && (best == 0 || d < best)) best = d; }
    return best > 0 && best <= 7 * 86400 ? (int)best : 0;
}

inline std::string fileStem(const std::string& mkt, int period) { return mkt + "_" + std::to_string(period > 0 ? period : 0); }

// ---------------------------------------------------------------- labels
inline std::string trim(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) b--;
    return s.substr(a, b - a);
}

// ChartView.labels.txt: one chart label per line, '#' lines and blanks ignored, a UTF-8 BOM ignored, duplicates once
inline std::vector<std::string> parseLabelFile(const std::string& text, int cap = MAX_LABELS)
{
    std::vector<std::string> out; std::set<std::string> seen;
    std::string t = text;
    if (t.size() >= 3 && (unsigned char)t[0] == 0xEF && (unsigned char)t[1] == 0xBB && (unsigned char)t[2] == 0xBF) t = t.substr(3);
    size_t p = 0;
    while (p <= t.size() && (int)out.size() < cap) {
        size_t e = t.find('\n', p); if (e == std::string::npos) e = t.size();
        std::string ln = trim(t.substr(p, e - p));
        if (!ln.empty() && ln[0] != '#' && ln.size() < 200 && seen.insert(ln).second) out.push_back(ln);
        p = e + 1;
    }
    return out;
}

// the per-chart "Extra labels" setting: comma or semicolon separated ("none" = nothing)
inline std::vector<std::string> parseExtra(const std::string& s, int cap = MAX_LABELS)
{
    std::vector<std::string> out; std::string cur;
    std::string all = s + ",";
    for (char c : all) {
        if (c == ',' || c == ';') {
            std::string x = trim(cur); cur.clear();
            if (!x.empty() && upper(x) != "NONE" && (int)out.size() < cap) out.push_back(x);
        } else cur += c;
    }
    return out;
}

inline std::vector<std::string> mergeLabels(const std::vector<std::string>& a, const std::vector<std::string>& b, int cap = MAX_LABELS)
{
    std::vector<std::string> out; std::set<std::string> seen;
    for (const auto& x : a) if ((int)out.size() < cap && seen.insert(x).second) out.push_back(x);
    for (const auto& x : b) if ((int)out.size() < cap && seen.insert(x).second) out.push_back(x);
    return out;
}

// the names a label is tried under: as written; "CI[name]" also as "name" (the chart legend may show either)
inline std::vector<std::string> labelVariants(const std::string& label)
{
    std::vector<std::string> v; v.push_back(label);
    if (label.size() > 4 && label.compare(0, 3, "CI[") == 0 && label.back() == ']') v.push_back(label.substr(3, label.size() - 4));
    return v;
}
// the RTL custom-indicator name for the last-resort getCustomIndicator() try ("" = do not try)
inline std::string customName(const std::string& label)
{
    if (label.size() > 4 && label.compare(0, 3, "CI[") == 0 && label.back() == ']') return label.substr(3, label.size() - 4);
    return std::string();
}

// ChartView.want.txt: blank (or "*" / "ALL") = every chart answers; otherwise one token per line or comma: "ES" (every ES
// chart), "ES_180" (that chart only)
inline bool wantMatches(const std::string& text, const std::string& mkt, int period)
{
    std::string t = text; for (char& c : t) if (c == ',' || c == ';' || c == ' ' || c == '\t' || c == '\r') c = '\n';
    std::vector<std::string> toks = parseLabelFile(t, 64);
    if (toks.empty()) return true;
    std::string M = upper(mkt), MP = upper(fileStem(mkt, period));
    for (auto x : toks) { x = upper(x); if (x == "*" || x == "ALL" || x == M || x == MP) return true; }
    return false;
}

// ---------------------------------------------------------------- when a snapshot is due
enum Why { NONE = 0, FIRST, NEWBAR, TIMER, WANT };
inline const char* whyName(int w) { return w == FIRST ? "first" : w == NEWBAR ? "new bar" : w == TIMER ? "forming bar" : w == WANT ? "request" : "none"; }

struct Sched {
    long barCount = -1;            // bars on the chart at the last build
    long long lastBarT = -1;       // the newest bar's stamp at the last build
    long long lastBuildMs = -1;    // the last build (written or skipped as unchanged)
};
// The cheap test run on every calc / draw: no SDK array is touched unless this says so.
inline int due(const Sched& s, long barCount, long long lastBarT, long long nowMs, bool want, long long throttleMs = THROTTLE_MS)
{
    if (barCount < 2) return NONE;
    if (want) return WANT;
    if (s.lastBuildMs < 0) return FIRST;
    if (barCount != s.barCount || lastBarT != s.lastBarT) return NEWBAR;
    if (nowMs - s.lastBuildMs >= throttleMs || nowMs < s.lastBuildMs) return TIMER;   // (a clock step back also rebuilds)
    return NONE;
}
inline void built(Sched& s, long barCount, long long lastBarT, long long nowMs) { s.barCount = barCount; s.lastBarT = lastBarT; s.lastBuildMs = nowMs; }
// a forming-bar rebuild that produced the same body is not written again; a new bar, a request or the first always is
inline bool shouldWrite(int why, unsigned long long hash, unsigned long long lastHash, bool everWritten)
{
    if (why == NONE) return false;
    if (why == WANT || why == FIRST || !everWritten) return true;
    return hash != lastHash;
}

// ---------------------------------------------------------------- JSON text
inline unsigned long long fnv1a(const std::string& s)
{
    unsigned long long h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

// JSON string body. Valid UTF-8 passes through; any other byte >= 0x80 (Windows-1252 text in a status file) is written as
// \u00XX so the file always parses.
inline std::string jsonEscape(const std::string& s)
{
    std::string o; o.reserve(s.size() + 8);
    const size_t n = s.size();
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if (c == '\t') o += "\\t";
        else if (c < 0x20 || c == 0x7F) { char b[8]; std::snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
        else if (c < 0x80) o += (char)c;
        else {
            int len = (c >= 0xC2 && c <= 0xDF) ? 2 : (c >= 0xE0 && c <= 0xEF) ? 3 : (c >= 0xF0 && c <= 0xF4) ? 4 : 0;
            bool ok = len > 0 && i + len <= n;
            for (int k = 1; ok && k < len; k++) if (((unsigned char)s[i + k] & 0xC0) != 0x80) ok = false;
            if (ok && len == 3) { unsigned char c1 = (unsigned char)s[i + 1]; if ((c == 0xE0 && c1 < 0xA0) || (c == 0xED && c1 > 0x9F)) ok = false; }
            if (ok && len == 4) { unsigned char c1 = (unsigned char)s[i + 1]; if ((c == 0xF0 && c1 < 0x90) || (c == 0xF4 && c1 > 0x8F)) ok = false; }
            if (ok) { o.append(s, i, (size_t)len); i += (size_t)len - 1; }
            else { char b[8]; std::snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
        }
    }
    return o;
}
inline std::string jstr(const std::string& s) { return "\"" + jsonEscape(s) + "\""; }

// numbers: non-finite = null; 8 significant digits (a float has ~7) with no trailing zeros; -0 = 0
inline std::string num(double x)
{
    if (!std::isfinite(x)) return "null";
    if (x == 0) return "0";
    char b[40]; std::snprintf(b, sizeof(b), "%.8g", x);
    return b;
}

// ---------------------------------------------------------------- time
inline long long daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline void civilFromDays(long long z, int& y, int& m, int& d)
{
    z += 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long yy = (long long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1); m = (int)(mp < 10 ? mp + 3 : mp - 9); y = (int)(yy + (m <= 2));
}
struct Civil { int y = 1970, mo = 1, d = 1, h = 0, mi = 0, s = 0; };
inline long long secsOf(const Civil& c) { return daysFromCivil(c.y, (unsigned)c.mo, (unsigned)c.d) * 86400LL + c.h * 3600LL + c.mi * 60LL + c.s; }
// local - UTC in minutes, from the same instant broken down both ways (Central: -300 in CDT, -360 in CST)
inline int utcOffsetMin(const Civil& local, const Civil& utc) { return (int)((secsOf(local) - secsOf(utc)) / 60); }
inline std::string stamp(const Civil& c)
{
    char b[32]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d", c.y, c.mo, c.d, c.h, c.mi, c.s); return b;
}
inline std::string iso(const Civil& c, int offMin)
{
    char b[48]; int a = offMin < 0 ? -offMin : offMin;
    std::snprintf(b, sizeof(b), "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d", c.y, c.mo, c.d, c.h, c.mi, c.s, offMin < 0 ? '-' : '+', a / 60, a % 60);
    return b;
}
inline std::string dateOfDays(long long days) { int y, m, d; civilFromDays(days, y, m, d); char b[16]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d", y, m, d); return b; }

// ---------------------------------------------------------------- the status / marks files our plugins write
enum Cut { HEAD = 0, TAIL = 1, HEADTAIL = 2 };
struct Src { std::string rel; int cut; };
// TapeFlow names its events file by contract (non-alphanumerics -> '_') - same rule as TapeFlow::evPath
inline std::string contractTag(const std::string& sym) { std::string c = sym; for (char& ch : c) if (!std::isalnum((unsigned char)ch) && ch != '-' && ch != '_') ch = '_'; return c; }
// Every file (relative to lsFlexLevels) worth copying for market M. Plugin status files first, then the data files the
// plugins DRAW from (levels, signals, summary), then TapeFlow's committed events (its on-chart marks) for the nearby
// session dates (TapeFlow's session date = the calendar day after 17:00 CT).
inline std::vector<Src> sourcesFor(const std::string& M, const std::string& sym, long long todayDays)
{
    std::vector<Src> v;
    auto add = [&](const std::string& r, int c) { v.push_back(Src{ r, c }); };
    add("DealerProfile.status.txt", HEAD); add("DealerRead.status.txt", HEAD); add("DealerSig.status.txt", HEAD);
    add("DealerSummary.status.txt", HEAD); add("DeltaProfile.status.txt", HEAD); add("SessionVWAP.status.txt", HEAD);
    add("SessionVWAP-" + M + ".odds.txt", HEAD); add("TapeFlow.status-" + M + ".txt", HEAD); add("IRTReader.status-" + M + ".txt", HEAD);
    add("KingTracker.status.txt", HEAD);
    static const char* BK[5] = { "Auto", "SPX", "SPY", "IF", "Both" };
    for (int b = 0; b < 5; b++) { add(std::string("GammaProfile.status-") + BK[b] + "-Left.txt", HEAD); add(std::string("GammaProfile.status-") + BK[b] + "-Right.txt", HEAD); }
    add("LRA-Dealer-" + M + ".csv", HEADTAIL); add("LRA-Summary-" + M + ".txt", HEAD); add("LRA-Session-" + M + ".csv", HEAD);
    add("LRA-Levels-" + M + ".csv", HEAD); add("LRA-Touch-" + M + ".csv", HEAD);
    const std::string tag = contractTag(sym);
    if (!tag.empty()) for (int k = 1; k >= -3; k--) add("TapeFlow\\" + M + "-events-v110-" + tag + "-" + dateOfDays(todayDays + k) + ".csv", TAIL);
    return v;
}
// Which byte ranges to read from a file of `size` bytes so at most `cap` are kept: [0, headLen) and [tailFrom, size)
struct Plan { size_t headLen = 0; size_t tailFrom = 0; bool truncated = false; };
inline Plan planRead(size_t size, int cut, size_t cap = FILE_CAP)
{
    Plan p;
    if (size <= cap) { p.headLen = size; p.tailFrom = size; return p; }
    p.truncated = true;
    if (cut == HEAD) { p.headLen = cap; p.tailFrom = size; }
    else if (cut == TAIL) { p.headLen = 0; p.tailFrom = size - cap; }
    else { p.headLen = cap / 4; p.tailFrom = size - (cap - cap / 4); }
    return p;
}
// joins what was read: a cut tail starts at its first full line; a cut head ends at its last full line
inline std::string joinCut(const std::string& head, const std::string& tail, const Plan& p, size_t size)
{
    if (!p.truncated) return head;
    std::string h = head, t = tail;
    if (!h.empty()) { size_t e = h.rfind('\n'); if (e != std::string::npos) h.resize(e + 1); }
    if (!t.empty()) { size_t s = t.find('\n'); if (s != std::string::npos && s + 1 < t.size()) t = t.substr(s + 1); }
    std::string mid = "...[ChartView: " + std::to_string((unsigned long long)size) + " bytes, cut to " + std::to_string((unsigned long long)(h.size() + t.size())) + "]...\n";
    return h + mid + t;
}

// ---------------------------------------------------------------- the snapshot
struct Series {
    std::string label, name, textLabel, via;              // via: "chart", "chart:<name>", "custom"
    std::vector<int> arrayNo;                             // which arrays were read (0, 1, ...)
    std::vector<std::vector<double>> values;              // values[k][i] for bar i of the snapshot (NaN = no value)
};
struct FileBlob { std::string name; unsigned long long bytes = 0; std::string mtime; bool truncated = false; std::string text; };
struct Snap {
    std::string market, symbol, root, chart, periodicity;
    int period = 0; long chartBars = 0; int barsWanted = DEFAULT_BARS;
    std::vector<std::string> t; std::vector<double> o, h, l, c, v;
    std::vector<Series> series; std::vector<std::string> missing;
    std::vector<FileBlob> files; std::vector<std::string> filesSkipped;
    std::string watchFile; int watchFromFile = 0; int watchExtra = 0;
    std::vector<std::string> notes;
};
inline std::string arr(const std::vector<double>& x)
{
    std::string o = "["; for (size_t i = 0; i < x.size(); i++) { if (i) o += ','; o += num(x[i]); } return o + "]";
}
inline std::string arrS(const std::vector<std::string>& x)
{
    std::string o = "["; for (size_t i = 0; i < x.size(); i++) { if (i) o += ','; o += jstr(x[i]); } return o + "]";
}
// The body: everything except the header (version / written / why). The change test hashes this, so a snapshot whose
// content did not move is not written again.
inline std::string body(const Snap& s)
{
    std::string o;
    o.reserve(64 * 1024);
    o += "\"market\":" + jstr(s.market) + ",\"symbol\":" + jstr(s.symbol) + ",\"root\":" + jstr(s.root) + ",\"chart\":" + jstr(s.chart);
    o += ",\"period\":" + std::to_string(s.period) + ",\"periodicity\":" + jstr(s.periodicity);
    o += ",\"chart_bars\":" + std::to_string(s.chartBars) + ",\"bars_wanted\":" + std::to_string(s.barsWanted);
    o += ",\"bar_time\":\"IRT bar stamp, local (Central) time\"";
    o += ",\"bars\":{\"n\":" + std::to_string(s.t.size()) + ",\"t\":" + arrS(s.t) + ",\"o\":" + arr(s.o) + ",\"h\":" + arr(s.h) + ",\"l\":" + arr(s.l)
       + ",\"c\":" + arr(s.c) + ",\"v\":" + arr(s.v) + "}";
    o += ",\"series\":{";
    for (size_t k = 0; k < s.series.size(); k++) {
        const Series& x = s.series[k];
        if (k) o += ',';
        o += jstr(x.label) + ":{\"name\":" + jstr(x.name) + ",\"textLabel\":" + jstr(x.textLabel) + ",\"via\":" + jstr(x.via) + ",\"arrays\":{";
        for (size_t a = 0; a < x.values.size() && a < x.arrayNo.size(); a++) { if (a) o += ','; o += "\"" + std::to_string(x.arrayNo[a]) + "\":" + arr(x.values[a]); }
        o += "}}";
    }
    o += "},\"missing\":" + arrS(s.missing);
    o += ",\"watch\":{\"file\":" + jstr(s.watchFile) + ",\"from_file\":" + std::to_string(s.watchFromFile) + ",\"extra\":" + std::to_string(s.watchExtra) + "}";
    o += ",\"status_files\":{";
    for (size_t k = 0; k < s.files.size(); k++) {
        const FileBlob& f = s.files[k];
        if (k) o += ',';
        o += jstr(f.name) + ":{\"bytes\":" + std::to_string(f.bytes) + ",\"mtime\":" + jstr(f.mtime) + ",\"truncated\":" + (f.truncated ? "true" : "false")
           + ",\"text\":" + jstr(f.text) + "}";
    }
    o += "},\"status_skipped\":" + arrS(s.filesSkipped);
    o += ",\"notes\":" + arrS(s.notes);
    return o;
}
inline std::string document(const std::string& bodyText, const std::string& writtenIso, long long writtenEpoch, int why)
{
    return "{\"version\":" + jstr(CV_VERSION_STR) + ",\"written\":" + jstr(writtenIso) + ",\"written_epoch\":" + std::to_string(writtenEpoch)
         + ",\"why\":" + jstr(whyName(why)) + "," + bodyText + "}\n";
}

} // namespace cvl
#endif
