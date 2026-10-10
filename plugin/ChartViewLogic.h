#ifndef LS_CHARTVIEW_LOGIC_H
#define LS_CHARTVIEW_LOGIC_H
/********************************************************************************
 *  ChartViewLogic.h  --  the SDK-free half of lsChartView (1.2.0, 2026-10-10)
 *
 *  Everything ChartView decides without Investor/RT: which market a chart is, the watch-list file, when a snapshot is due,
 *  whether it changed, JSON text (escaping, numbers), size caps for the copied status files, the snapshot JSON itself, and
 *  (1.2.0) the camera's decisions: which IRT window is THIS chart (market AND bar size), whether a picture is blank, and how
 *  a watch-list label is looked up.
 *  Tested by test_chartview_logic.cpp (g++ / MinGW, no SDK) and test_chartview_mock.cpp (the real ChartView.cpp on a mock host).
 *
 *  1.2.0 (2026-10-10)
 *   - Window choice: the chart's title must show its bar size ("3 Minutes" / "(3m*)" right after the symbol); a window that
 *     only shows the symbol is never taken. Before, an empty periodicity label made every ES window match and the biggest won,
 *     so ES_180 and ES_3600 (CL, GC, HG too) got the SAME picture.
 *   - Blank detector for the picture (the price pane came out black when the chart was caught mid-paint).
 *   - Labels: "X[Y]" legend text is split; Y (the custom-indicator name, e.g. cob_bull style) and X are tried; "=value" legend
 *     tails, quotes, UTF-16 files and control bytes are cleaned. The SDK's 16-byte text label is reported as cut, not trusted.
 *   - The "Extra labels" setting is gone (task #300: the settings window is empty); the watch list is the labels file only.
 ********************************************************************************/
#include "DealerLogic.h"          // dl::marketForRoot - the one market mapping every LRA plugin uses
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cctype>
#include <cerrno>
#include <cstdlib>

namespace cvl {

static const char* const CV_VERSION_STR = "1.2.0";
static const int   MAX_LABELS      = 40;       // labels read per chart
static const int   MAX_ARRAYS      = 16;       // arrays read per label (0..15, stop at the first that fails)
static const int   FIXED_BARS      = 300;      // (task #300) always the last 300 bars - no setting
static const long long THROTTLE_MS = 10000;    // the forming bar: at most one snapshot every 10 s
static const size_t FILE_CAP       = 64 * 1024;    // each copied status / marks file
static const size_t FILES_TOTAL_CAP = 768 * 1024;  // all copied files together
static const size_t LABEL_MAX      = 120;      // one cleaned label
static const int   SNAP_MAX_TRIES  = 3;        // a blank picture is retried twice
static const long long SNAP_RETRY_MS = 1500;   // ... on a later timer tick, never in the same call
static const long long SNAP_STALE_S = 120;     // a request whose epoch is older than this is ignored
static const double BLANK_FRAC     = 0.975;    // price-pane area this uniform = blank (good charts measured 0.72 - 0.83)

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

// a labels file saved by Notepad as "Unicode" (UTF-16 LE / BE, with or without a BOM) becomes plain bytes; UTF-8 BOM dropped
inline std::string decodeText(const std::string& raw)
{
    std::string t = raw;
    if (t.size() >= 3 && (unsigned char)t[0] == 0xEF && (unsigned char)t[1] == 0xBB && (unsigned char)t[2] == 0xBF) return t.substr(3);
    bool le = t.size() >= 2 && (unsigned char)t[0] == 0xFF && (unsigned char)t[1] == 0xFE;
    bool be = t.size() >= 2 && (unsigned char)t[0] == 0xFE && (unsigned char)t[1] == 0xFF;
    if (!le && !be && t.size() >= 4) {                         // no BOM: every other byte NUL = UTF-16 anyway
        size_t z0 = 0, z1 = 0, n = t.size() / 2 * 2;
        for (size_t i = 0; i < n; i += 2) { if (t[i] == 0) z0++; if (t[i + 1] == 0) z1++; }
        if (z1 * 10 >= n / 2 * 9 && z0 == 0) le = true; else if (z0 * 10 >= n / 2 * 9 && z1 == 0) be = true;
        if (le || be) t = std::string(2, '\0') + t;            // align with the BOM path below
    }
    if (!le && !be) return t;
    std::string o;
    for (size_t i = 2; i + 1 < t.size(); i += 2) {
        unsigned char lo = (unsigned char)t[le ? i : i + 1], hi = (unsigned char)t[le ? i + 1 : i];
        o += (hi == 0 && lo != 0) ? (char)lo : '?';            // labels are ASCII; anything else is visibly '?', never garbage
    }
    return o;
}

// one watch-list line as the chart legend may give it: "updn[Distance_Ratio_updn]=0.37" -> "updn[Distance_Ratio_updn]".
// Drops the "=value" tail, surrounding quotes, control bytes and non-ASCII bytes (shown as '?'), collapses blanks, caps length.
inline std::string cleanLabel(const std::string& in)
{
    std::string s;
    for (unsigned char c : in) {
        if (c == '\t') c = ' ';
        if (c < 0x20 || c == 0x7F) continue;
        s += (c >= 0x80) ? '?' : (char)c;
    }
    s = trim(s);
    size_t rb = s.rfind(']');
    size_t eq = s.find('=', rb == std::string::npos ? 0 : rb);
    if (eq != std::string::npos) s = trim(s.substr(0, eq));
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) s = trim(s.substr(1, s.size() - 2));
    std::string o; bool sp = false;
    for (char c : s) { if (c == ' ') { if (!sp) o += c; sp = true; } else { o += c; sp = false; } }
    if (o.size() > LABEL_MAX) o.resize(LABEL_MAX);
    return o;
}

// ChartView.labels.txt: one chart label per line, '#' lines and blanks ignored, BOM / UTF-16 handled, duplicates once
inline std::vector<std::string> parseLabelFile(const std::string& text, int cap = MAX_LABELS)
{
    std::vector<std::string> out; std::set<std::string> seen;
    std::string t = decodeText(text);
    size_t p = 0;
    while (p <= t.size() && (int)out.size() < cap) {
        size_t e = t.find('\n', p); if (e == std::string::npos) e = t.size();
        std::string raw = trim(t.substr(p, e - p));
        p = e + 1;
        if (raw.empty() || raw[0] == '#') continue;
        std::string ln = cleanLabel(raw);
        if (!ln.empty() && seen.insert(ln).second) out.push_back(ln);
    }
    return out;
}

// the legend form "X[Y]": prefix X ("CI", "updn"), inner Y (the indicator / custom-indicator name). Inner = from the FIRST '['
// to the LAST ']' (a name may itself hold brackets).
struct LabelParts { std::string prefix, inner; bool bracket = false; };
inline LabelParts splitLabel(const std::string& label)
{
    LabelParts p;
    size_t a = label.find('['), b = label.rfind(']');
    if (a == std::string::npos || b == std::string::npos || b <= a + 1 || b != label.size() - 1) return p;
    p.bracket = true; p.prefix = trim(label.substr(0, a)); p.inner = trim(label.substr(a + 1, b - a - 1));
    return p;
}

// the names a label is tried under with getChartIndicator, in order: as written; the inner name; the prefix (unless it is just
// the "CI" tag). DealerProfile finds his custom indicators by their bare names (cob_bull), so the inner name is the likely hit.
inline std::vector<std::string> lookupNames(const std::string& label)
{
    std::vector<std::string> v; std::set<std::string> seen;
    auto add = [&](const std::string& x) { if (!x.empty() && x.size() < 80 && seen.insert(x).second) v.push_back(x); };
    add(label);
    LabelParts p = splitLabel(label);
    if (p.bracket) { add(p.inner); if (upper(p.prefix) != "CI") add(p.prefix); }
    return v;
}
// the RTL custom-indicator name for the getCustomIndicator() try: the inner name of a bracketed label, else the plain label
inline std::string customName(const std::string& label)
{
    LabelParts p = splitLabel(label);
    if (p.bracket) return p.inner.size() < 80 ? p.inner : std::string();
    return label.find_first_of("[]") == std::string::npos && label.size() < 80 ? label : std::string();
}

// an SDK fixed char field (INDICATOR_INFO name[72] / textLabel[16]) that may not be NUL-terminated: up to the first NUL;
// non-printable bytes shown as '?'; cut = the field was full with no NUL (the text is longer than the SDK keeps)
inline std::string fixedField(const char* p, size_t cap, bool* cut = nullptr)
{
    if (cut) *cut = false;
    if (!p) return std::string();
    size_t n = 0; while (n < cap && p[n]) n++;
    if (cut) *cut = (n == cap);
    std::string o;
    for (size_t i = 0; i < n; i++) { unsigned char c = (unsigned char)p[i]; o += (c < 0x20 || c >= 0x7F) ? '?' : (char)c; }
    return o;
}

// ---------------------------------------------------------------- want / picture requests
// ChartView.want.txt and ChartView.snap.txt: blank (or "*" / "ALL") = every chart answers; otherwise one token per line /
// comma / blank: "ES" (every ES chart), "ES_180" (that chart only). A bare number of 9+ digits is the request's epoch.
struct Request { std::vector<std::string> toks; long long epoch = -1; };
inline Request parseRequest(const std::string& text)
{
    Request r;
    std::string t = decodeText(text);
    if (t.size() > 4096) t.resize(4096);
    for (char& c : t) if (c == ',' || c == ';' || c == ' ' || c == '\t' || c == '\r') c = '\n';
    size_t p = 0;
    while (p <= t.size() && r.toks.size() < 64) {
        size_t e = t.find('\n', p); if (e == std::string::npos) e = t.size();
        std::string x = trim(t.substr(p, e - p)); p = e + 1;
        if (x.empty() || x[0] == '#') continue;
        bool digits = x.size() >= 9 && x.size() <= 12; for (char c : x) if (!std::isdigit((unsigned char)c)) digits = false;
        if (digits) { errno = 0; char* end = nullptr; long long v = std::strtoll(x.c_str(), &end, 10); if (errno == 0 && end && !*end && v > 0) r.epoch = v; continue; }
        r.toks.push_back(upper(x));
    }
    return r;
}
inline bool requestMatches(const Request& r, const std::string& mkt, int period)
{
    if (r.toks.empty()) return true;
    std::string M = upper(mkt), MP = upper(fileStem(mkt, period));
    for (const auto& x : r.toks) if (x == "*" || x == "ALL" || x == M || x == MP) return true;
    return false;
}
inline bool wantMatches(const std::string& text, const std::string& mkt, int period) { return requestMatches(parseRequest(text), mkt, period); }
// a request is answered when its file changed; it is too old when its own epoch is > 120 s away from now, or (no epoch) when
// the chart only just loaded and the file is > 30 s old (a leftover from an earlier session)
inline bool requestFresh(const Request& r, long long nowEpoch, long long fileMtime, bool firstLook)
{
    if (r.epoch > 0) return nowEpoch - r.epoch <= SNAP_STALE_S && r.epoch - nowEpoch <= SNAP_STALE_S;
    return !firstLook || nowEpoch - fileMtime <= 30;
}

// ---------------------------------------------------------------- which IRT window is this chart (1.2.0)
// IRT chart titles look like  "ES LTF: EPZ26 (3m*), EPZ26 (1m) 3 Minutes*, Full Session 17:00-16:00"
//                             "CL LTF: CLEX26 (3m*) Crude Light (Globex): November 2026 3 Minutes*, Full Session 17:00-16:00"
// The bar size shows twice: a code right after the chart's FIRST symbol "(3m*)" and a phrase "3 Minutes". A second symbol
// with its own code ("EPZ26 (1m)") is an overlay, not this chart's bar size, so only the code after the first symbol counts.
inline std::vector<std::string> periodPhrases(int spb)
{
    std::vector<std::string> v;
    if (spb <= 0) return v;
    if (spb % 86400 == 0) { int d = spb / 86400; if (d == 1) { v.push_back("Daily"); v.push_back("1 Day"); } else if (d == 7) { v.push_back("Weekly"); v.push_back("7 Day"); } else v.push_back(std::to_string(d) + " Day"); return v; }
    if (spb % 60 == 0) {
        int m = spb / 60; v.push_back(std::to_string(m) + " Minute");
        if (m % 60 == 0) { int h = m / 60; v.push_back(std::to_string(h) + " Hour"); if (h == 1) v.push_back("Hourly"); }
        return v;
    }
    v.push_back(std::to_string(spb) + " Second");
    return v;
}
inline std::vector<std::string> periodCodes(int spb)
{
    std::vector<std::string> v;
    if (spb <= 0) return v;
    if (spb % 86400 == 0) { v.push_back(std::to_string(spb / 86400) + "d"); if (spb == 86400) v.push_back("D"); return v; }
    if (spb % 60 == 0) { int m = spb / 60; v.push_back(std::to_string(m) + "m"); if (m % 60 == 0) v.push_back(std::to_string(m / 60) + "h"); return v; }
    v.push_back(std::to_string(spb) + "s");
    return v;
}
inline bool icaseAt(const std::string& s, size_t at, const std::string& w)
{
    if (at + w.size() > s.size()) return false;
    for (size_t i = 0; i < w.size(); i++) if (std::tolower((unsigned char)s[at + i]) != std::tolower((unsigned char)w[i])) return false;
    return true;
}
// the phrase as a whole word: not preceded by a digit / letter ("13 Minutes" is not "3 Minute"), not followed by a digit
inline bool hasPhrase(const std::string& title, const std::string& ph)
{
    for (size_t at = 0; at + ph.size() <= title.size(); at++) {
        if (!icaseAt(title, at, ph)) continue;
        if (at > 0 && std::isalnum((unsigned char)title[at - 1])) continue;
        size_t e = at + ph.size();
        if (e < title.size() && std::isdigit((unsigned char)title[e])) continue;
        return true;
    }
    return false;
}
// > 0 = this window shows this chart (higher = surer); 0 = has the symbol but NOT this bar size; -1 = not this symbol at all
inline int titleScore(const std::string& title, const std::string& sym, int spb, const std::string& perLabel)
{
    if (sym.empty()) return -1;
    size_t at = title.find(sym);
    if (at == std::string::npos) return -1;
    int score = 0;
    std::vector<std::string> ph = periodPhrases(spb);
    for (const auto& p : ph) if (hasPhrase(title, p)) { score += 4; break; }
    size_t q = at + sym.size();
    while (q < title.size() && title[q] == ' ') q++;
    if (q < title.size() && title[q] == '(') {
        std::vector<std::string> codes = periodCodes(spb);
        for (const auto& c : codes) {
            if (!icaseAt(title, q + 1, c)) continue;
            size_t e = q + 1 + c.size();
            if (e < title.size() && (title[e] == ')' || title[e] == '*' || title[e] == ' ')) { score += 4; break; }
        }
    }
    std::string pl = trim(perLabel);
    while (!pl.empty() && pl.back() == '*') pl.pop_back();
    if (!pl.empty() && hasPhrase(title, pl)) score += 2;
    // a phrase match alone from a SECOND symbol's code is impossible (codes are only read after the first symbol); a phrase
    // match is the chart's own bar size because IRT writes one periodicity phrase per chart title
    return score;
}
struct WinCand { std::string title; long long area = 0; int w = 0, h = 0; bool visible = true, iconic = false; };
struct Pick { int index = -1; int score = 0; int matches = 0; int symbolOnly = 0; int seen = 0; std::string why; };
// the best window for (symbol, bar size): score > 0 only; ties go to the window whose size is nearest the chart pane's size
// (paneW / paneH from draw, 0 = unknown), then the larger one. Never a window that only shows the symbol.
inline Pick pickChart(const std::vector<WinCand>& c, const std::string& sym, int spb, const std::string& perLabel, int paneW, int paneH)
{
    Pick p; p.seen = (int)c.size();
    long long bestFit = -1;
    std::string shown;                                       // up to 3 titles that show the symbol but not the bar size
    for (size_t i = 0; i < c.size(); i++) {
        if (!c[i].visible || c[i].iconic) continue;
        int s = titleScore(c[i].title, sym, spb, perLabel);
        if (s == 0) { p.symbolOnly++; if (p.symbolOnly <= 3) shown += (shown.empty() ? "" : " | ") + c[i].title.substr(0, 120); }
        if (s <= 0) continue;
        p.matches++;
        long long fit = (paneW > 0 && paneH > 0) ? (long long)std::abs(c[i].w - paneW) + std::abs(c[i].h - paneH) : 0;
        bool better = p.index < 0 || s > p.score || (s == p.score && (fit < bestFit || (fit == bestFit && c[i].area > c[(size_t)p.index].area)));
        if (better) { p.index = (int)i; p.score = s; bestFit = fit; }
    }
    if (p.index < 0) {
        if (p.symbolOnly > 0) p.why = std::to_string(p.symbolOnly) + " window(s) show " + sym + " but none shows its bar size (" + std::to_string(spb) + " s): " + shown;
        else p.why = "no visible IRT window shows " + sym;
    }
    return p;
}

// ---------------------------------------------------------------- blank picture (1.2.0)
// The price pane sits in the upper half of every IRT chart picture. Measured on his pictures (2026-10-09): a good chart's
// rows 6% - 55% are at most 83% one colour; the blank ES picture was 100%. Colours are compared on 5 bits per channel.
struct Blank { double frac = 1.0; bool blank = true; };
inline Blank blankCheck(const unsigned char* rgb, int w, int h)
{
    Blank b;
    if (!rgb || w < 20 || h < 20) return b;
    int y0 = (int)(h * 0.06), y1 = (int)(h * 0.55), x0 = (int)(w * 0.02), x1 = (int)(w * 0.90);
    std::vector<unsigned> cnt(32768, 0); unsigned n = 0, best = 0;
    for (int y = y0; y < y1; y += 2)
        for (int x = x0; x < x1; x += 2) {
            const unsigned char* p = rgb + ((size_t)y * (size_t)w + (size_t)x) * 3;
            unsigned k = ((unsigned)(p[0] >> 3) << 10) | ((unsigned)(p[1] >> 3) << 5) | (unsigned)(p[2] >> 3);
            unsigned v = ++cnt[k]; if (v > best) best = v; n++;
        }
    if (n == 0) return b;
    b.frac = (double)best / (double)n;
    b.blank = b.frac >= BLANK_FRAC;
    return b;
}
// what to do after a capture: SAVE (good), RETRY (blank, tries left - next timer tick), GIVE_UP (blank after the last try:
// the last good picture is kept, the blank one is saved beside it as <stem>.blank.png and the status says BLANK)
enum SnapNext { SAVE = 0, RETRY = 1, GIVE_UP = 2 };
inline int afterCapture(bool captured, bool blank, int attempt)
{
    if (captured && !blank) return SAVE;
    return attempt + 1 < SNAP_MAX_TRIES ? RETRY : GIVE_UP;
}
// PrintWindow flags per attempt: PW_RENDERFULLCONTENT (2) twice (the 2nd after an InvalidateRect and a tick), then classic 0
inline unsigned printFlags(int attempt) { return attempt >= 2 ? 0u : 2u; }

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

// numbers: non-finite = null; whole numbers (volumes, counts - also above 2^31) exactly; others 8 significant digits (a float
// has ~7) with no trailing zeros; -0 = 0
inline std::string num(double x)
{
    if (!std::isfinite(x)) return "null";
    if (x == 0) return "0";
    char b[40];
    if (std::fabs(x) < 1e15 && x == std::floor(x)) std::snprintf(b, sizeof(b), "%.0f", x);
    else std::snprintf(b, sizeof(b), "%.8g", x);
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
inline std::string dateOfDays(long long days) { int y, m, d; civilFromDays(days, y, m, d); char b[40]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d", y, m, d); return b; }

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
    static const char* const BK[5] = { "Auto", "SPX", "SPY", "IF", "Both" };
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
    std::string label, foundAs, via;                      // label as listed; the name that worked; "chart" / "custom" / "+cached"
    std::string sdkName, sdkTextLabel; bool sdkTextCut = false;
    std::vector<int> arrayNo;                             // which arrays were read (0, 1, ...)
    std::vector<std::vector<double>> values;              // values[k][i] for bar i of the snapshot (NaN = no value)
};
struct FileBlob { std::string name; unsigned long long bytes = 0; std::string mtime; bool truncated = false; std::string text; };
struct Snap {
    std::string market, symbol, root, chart, periodicity;
    int period = 0; long chartBars = 0; int barsWanted = FIXED_BARS;
    std::vector<std::string> t; std::vector<double> o, h, l, c, v;
    std::vector<Series> series; std::vector<std::string> missing; std::vector<std::string> lookup;   // lookup: why a label is missing
    std::vector<FileBlob> files; std::vector<std::string> filesSkipped;
    std::string watchFile; int watchFromFile = 0;
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
        LabelParts lp = splitLabel(x.label);
        o += jstr(x.label) + ":{\"label\":" + jstr(x.label) + ",\"legend_prefix\":" + jstr(lp.prefix) + ",\"legend_name\":" + jstr(lp.inner)
           + ",\"found_as\":" + jstr(x.foundAs) + ",\"via\":" + jstr(x.via) + ",\"sdk_name\":" + jstr(x.sdkName)
           + ",\"sdk_text_label\":" + jstr(x.sdkTextLabel) + ",\"sdk_text_label_cut\":" + (x.sdkTextCut ? "true" : "false") + ",\"arrays\":{";
        for (size_t a = 0; a < x.values.size() && a < x.arrayNo.size(); a++) { if (a) o += ','; o += "\"" + std::to_string(x.arrayNo[a]) + "\":" + arr(x.values[a]); }
        o += "}}";
    }
    o += "},\"missing\":" + arrS(s.missing) + ",\"lookup\":" + arrS(s.lookup);
    o += ",\"watch\":{\"file\":" + jstr(s.watchFile) + ",\"from_file\":" + std::to_string(s.watchFromFile) + "}";
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

// the picture's status file ChartView\img\<stem>.status.txt - one line per field, read by Claude after a request
struct SnapStatus { std::string result, window, request, reason, when; int attempts = 0, score = 0, w = 0, h = 0, matches = 0; double blankFrac = -1; long long ms = 0; };
inline std::string snapStatusText(const SnapStatus& s)
{
    char bf[32]; std::snprintf(bf, sizeof(bf), "%.4f", s.blankFrac);
    return std::string("VERSION|") + CV_VERSION_STR + "\nRESULT|" + s.result + "\nWHEN|" + s.when + "\nREQUEST|" + s.request
         + "\nWINDOW|" + s.window + "\nSCORE|" + std::to_string(s.score) + "\nMATCHES|" + std::to_string(s.matches)
         + "\nSIZE|" + std::to_string(s.w) + "x" + std::to_string(s.h) + "\nATTEMPTS|" + std::to_string(s.attempts)
         + "\nBLANK_FRAC|" + (s.blankFrac < 0 ? std::string("") : std::string(bf)) + "\nMS|" + std::to_string(s.ms) + "\nREASON|" + s.reason + "\n";
}

// (1.1.0) a dependency-free PNG encoder for the chart camera: RGB rows, filter 0, zlib "stored" blocks (no compression - a
// 1000 x 450 chart is ~1.4 MB, fine for a file read once). Pure C++, tested on Linux (test_chartview_logic.cpp).
inline const unsigned long* pngCrcTable()
{
    struct T { unsigned long v[256]; T() { for (unsigned long i = 0; i < 256; i++) { unsigned long k = i; for (int j = 0; j < 8; j++) k = (k & 1) ? 0xedb88320UL ^ (k >> 1) : k >> 1; v[i] = k; } } };
    static const T t;                                      // immutable after its (thread-safe) construction
    return t.v;
}
inline unsigned long pngCrc(const unsigned char* p, size_t n, unsigned long c = 0xffffffffUL)
{
    const unsigned long* T = pngCrcTable();
    for (size_t i = 0; i < n; i++) c = T[(c ^ p[i]) & 0xff] ^ (c >> 8);
    return c;
}
inline void pngPut32(std::string& o, unsigned long v) { o += (char)((v >> 24) & 255); o += (char)((v >> 16) & 255); o += (char)((v >> 8) & 255); o += (char)(v & 255); }
inline void pngChunk(std::string& o, const char* type, const std::string& data)
{
    pngPut32(o, (unsigned long)data.size());
    std::string td = std::string(type, 4) + data;
    o += td;
    pngPut32(o, pngCrc((const unsigned char*)td.data(), td.size()) ^ 0xffffffffUL);
}
// rgb = w * h * 3 bytes, top row first; returns the PNG file bytes ("" for bad sizes)
inline std::string pngEncode(const unsigned char* rgb, int w, int h)
{
    if (!rgb || w <= 0 || h <= 0 || w > 20000 || h > 20000) return std::string();
    std::string raw; raw.reserve((size_t)h * ((size_t)w * 3 + 1));
    for (int y = 0; y < h; y++) { raw += (char)0; raw.append((const char*)rgb + (size_t)y * w * 3, (size_t)w * 3); }
    std::string z; z += (char)0x78; z += (char)0x01;
    unsigned long a = 1, b = 0;
    for (size_t i = 0; i < raw.size(); i++) { a = (a + (unsigned char)raw[i]) % 65521UL; b = (b + a) % 65521UL; }
    size_t pos = 0;
    do {
        size_t n = raw.size() - pos; if (n > 65535) n = 65535;
        bool last = pos + n >= raw.size();
        z += (char)(last ? 1 : 0);
        z += (char)(n & 255); z += (char)((n >> 8) & 255); z += (char)(~n & 255); z += (char)((~n >> 8) & 255);
        z.append(raw, pos, n); pos += n;
    } while (pos < raw.size());
    pngPut32(z, (b << 16) | a);
    std::string o("\x89PNG\r\n\x1a\n", 8);
    std::string ih; pngPut32(ih, (unsigned long)w); pngPut32(ih, (unsigned long)h); ih += (char)8; ih += (char)2; ih += (char)0; ih += (char)0; ih += (char)0;
    pngChunk(o, "IHDR", ih); pngChunk(o, "IDAT", z); pngChunk(o, "IEND", std::string());
    return o;
}

} // namespace cvl
#endif
