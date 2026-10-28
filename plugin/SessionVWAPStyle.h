// SessionVWAPStyle.h -- lsSessionVWAP 1.5.0 (2026-10-13): which lines are drawn, in which colour and width. No SDK.
//
// Rassul 2026-10-13 21:23 CT: "session vwap settings don't work. They should allow me to uncheck and remove bands as well as
// change colors." Until 1.4.2 the plugin had NO settings at all (1.3.3 / 1.3.4 took them away at his request, 2026-10-09) and
// drew every line in fixed colours. 1.5.0 gives every line a show box and a colour, the VWAP and the bands a width; the defaults
// are exactly what 1.4.2 drew (VWAP magenta 2 px, +1 SD green, -1 SD light red, +2 SD lime, -2 SD red, bands 1 px, the prior
// RTH VWAP grey overnight), so nothing changes until he changes it.
//
// WHERE THE CHOICES LIVE. IRT only fills list / check-box values while the settings window is open (DealerProfile 2.2.5,
// DealerRead 4.5.2, TradeManager, HodLod 1.0.1 - our own plugins' verified behaviour), and one DLL object serves every chart.
// So, as lsHodLod / lsTapeReader / the order-flow panes do: the window's OK / Apply / each change writes the choices for that
// chart's market to %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\SessionVWAP.settings-<MKT>.txt (atomic), and every draw reads
// that file back (by its stamp, at most once a second). Opening the window never writes (lesson 29). No file = the defaults.
//
// File (one line per item, strict; a bad line keeps that item's default, unknown lines are ignored, at most 4 KiB):
//   VERSION,1
//   VWAP,<0|1>,<RRGGBB>,<width 1-4>      the VWAP line (RTH and overnight)
//   P1,<0|1>,<RRGGBB>                    +1 SD band
//   M1,<0|1>,<RRGGBB>                    -1 SD band
//   P2,<0|1>,<RRGGBB>                    +2 SD band
//   M2,<0|1>,<RRGGBB>                    -2 SD band
//   BANDW,<width 1-4>                    every band's width
//   ON,<0|1>                             the overnight VWAP + bands (17:00 -> the RTH open)
//   PRIOR,<0|1>,<RRGGBB>                 the prior RTH session's VWAP, drawn overnight
//   PIVOT,<0|1>[,<above RRGGBB>,<below RRGGBB>,<width 1-4>]  (1.8.0) the 3-bar pivot VWAP on / off; (1.8.1) its own colours + width
//                                        (absent = on, green 22C55E / red EF4444, 2 px). 1.8.1: drawn on its own - it no longer hides
//                                        the session VWAP line, and the VWAP box no longer hides it.
#ifndef SESSION_VWAP_STYLE_H
#define SESSION_VWAP_STYLE_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

namespace svs {

typedef unsigned long Colour;   // IRT COLOR, 0x00RRGGBB

// 1.4.2's fixed colours - the defaults (Rassul 2026-10-09 13:41 his settings window colours; 14:40 -1 SD light red)
static const Colour D_VWAP = 0x00FF80FFUL;   // magenta
static const Colour D_P1   = 0x00008000UL;   // +1 SD green
static const Colour D_M1   = 0x00FF8080UL;   // -1 SD light red
static const Colour D_P2   = 0x0000FF00UL;   // +2 SD lime
static const Colour D_M2   = 0x00FF0000UL;   // -2 SD red
static const Colour D_PRTH = 0x006B7280UL;   // prior RTH VWAP, dim grey
static const Colour D_PIV_UP = 0x0022C55EUL; // (1.8.1) pivot VWAP while the bar closed above it - green
static const Colour D_PIV_DN = 0x00EF4444UL; // (1.8.1) pivot VWAP while the bar closed below it - red
static const int W_MIN = 1, W_MAX = 4, D_VWAP_W = 2, D_BAND_W = 1, D_PIV_W = 2;
static const size_t FILE_MAX = 4096;

// the five line kinds, in the plugin's output order (VWAP, +1, -1, +2, -2)
enum { K_VWAP = 0, K_P1, K_M1, K_P2, K_M2, K_N };

// (1.8.3) SVS_DEF_ON = the default of every line except the pivot VWAP: false in the plugin. The pre-1.8.3 test suites (181 / 182 /
// settings / 160), which exercise the unchanged mechanisms from the old all-on look, compile with -DSVS_TEST_ALL_ON_DEFAULTS.
#ifndef SVS_DEF_ON
#ifdef SVS_TEST_ALL_ON_DEFAULTS
#define SVS_DEF_ON true
#else
#define SVS_DEF_ON false
#endif
#endif
// (1.8.4, 2026-10-28 16:26 CT Rassul "the chart is extremely messy" + "also remove the Pivot VWAP line") the pivot VWAP is OFF by default too:
// a fresh chart draws NO line (he ticks any line back in the settings). The pre-1.8.3 suites keep it on with SVS_TEST_ALL_ON_DEFAULTS.
#ifndef SVS_PIV_DEF
#ifdef SVS_TEST_ALL_ON_DEFAULTS
#define SVS_PIV_DEF true
#else
#define SVS_PIV_DEF false
#endif
#endif
struct Style {
    bool show[K_N];
    Colour col[K_N];
    int vwapW, bandW;
    bool on;            // the overnight VWAP + bands
    bool prior;         // the prior RTH VWAP (overnight)
    bool pivot;         // (1.8.0) the 3-bar pivot VWAP (green / red by the close); (1.8.1) drawn independently of the VWAP line
    Colour priorCol;
    Colour pivUp, pivDn; // (1.8.1) the pivot VWAP's two colours
    int pivW;           // (1.8.1) the pivot VWAP's width
    // (1.8.3, 2026-10-28 14:08 CT Rassul "only 1 vwap level should be displayed" - the 3-bar anchored pivot VWAP) defaults: ONLY the
    // pivot VWAP is drawn (green above / red below, 2 px); session VWAP, the +-1 / +-2 SD bands, the overnight set and the prior RTH
    // VWAP are off (their colours / widths keep their values, so ticking a box brings the line back in its usual style)
    Style() : vwapW(D_VWAP_W), bandW(D_BAND_W), on(SVS_DEF_ON), prior(SVS_DEF_ON), pivot(SVS_PIV_DEF), priorCol(D_PRTH), pivUp(D_PIV_UP), pivDn(D_PIV_DN), pivW(D_PIV_W) {
        const Colour d[K_N] = { D_VWAP, D_P1, D_M1, D_P2, D_M2 };
        for (int k = 0; k < K_N; ++k) { show[k] = SVS_DEF_ON; col[k] = d[k]; }
    }
    bool operator==(const Style& o) const {
        for (int k = 0; k < K_N; ++k) if (show[k] != o.show[k] || col[k] != o.col[k]) return false;
        return vwapW == o.vwapW && bandW == o.bandW && on == o.on && prior == o.prior && pivot == o.pivot && priorCol == o.priorCol
            && pivUp == o.pivUp && pivDn == o.pivDn && pivW == o.pivW;
    }
    bool operator!=(const Style& o) const { return !(*this == o); }
};

// One drawn series (the plugin's 11 outputs): 0-4 RTH VWAP / +1 / -1 / +2 / -2, 5-9 the same overnight, 10 the prior RTH VWAP.
struct Pen { bool show; Colour col; int width; };
inline Pen penFor(const Style& s, int output) {
    Pen p; p.show = false; p.col = 0; p.width = 1;
    if (output < 0 || output > 10) return p;
    if (output == 10) { p.show = s.prior; p.col = s.priorCol; p.width = 1; return p; }
    const int k = output % 5;
    p.show = s.show[k] && (output < 5 || s.on);
    p.col = s.col[k];
    p.width = k == K_VWAP ? s.vwapW : s.bandW;
    return p;
}

// ---- strict text --------------------------------------------------------------------------------------------------------------
inline bool parseBit(const std::string& t, bool& out) { if (t == "1") { out = true; return true; } if (t == "0") { out = false; return true; } return false; }
inline bool parseWidth(const std::string& t, int& out) {
    if (t.size() != 1 || t[0] < '0' + W_MIN || t[0] > '0' + W_MAX) return false;
    out = t[0] - '0'; return true;
}
inline bool parseHex(const std::string& t, Colour& out) {
    if (t.size() != 6) return false;
    Colour v = 0;
    for (size_t i = 0; i < 6; ++i) {
        const char c = t[i]; int d;
        if (c >= '0' && c <= '9') d = c - '0'; else if (c >= 'A' && c <= 'F') d = c - 'A' + 10; else if (c >= 'a' && c <= 'f') d = c - 'a' + 10; else return false;
        v = (v << 4) | (Colour)d;
    }
    out = v; return true;
}
inline std::string hex(Colour c) { char b[8]; std::snprintf(b, sizeof(b), "%06lX", (unsigned long)(c & 0xFFFFFFUL)); return b; }
inline std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = s.find(sep, a); f.push_back(s.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    return f;
}
// black or near-black is never a line colour on his black chart (KingTracker 0.15): a swatch that reads near-black is ignored
inline bool nearBlack(Colour c) {
    const unsigned r = (unsigned)((c >> 16) & 0xFF), g = (unsigned)((c >> 8) & 0xFF), b = (unsigned)(c & 0xFF);
    unsigned m = r > g ? r : g; if (b > m) m = b;
    return m < 0x30;
}

inline std::string toText(const Style& s) {
    static const char* KEY[K_N] = { "VWAP", "P1", "M1", "P2", "M2" };
    std::string o = "VERSION,1\n";
    for (int k = 0; k < K_N; ++k) {
        o += KEY[k]; o += ','; o += s.show[k] ? '1' : '0'; o += ','; o += hex(s.col[k]);
        if (k == K_VWAP) { o += ','; o += (char)('0' + s.vwapW); }
        o += '\n';
    }
    o += "BANDW,"; o += (char)('0' + s.bandW); o += '\n';
    o += "ON,"; o += s.on ? '1' : '0'; o += '\n';
    o += "PRIOR,"; o += s.prior ? '1' : '0'; o += ','; o += hex(s.priorCol); o += '\n';
    o += "PIVOT,"; o += s.pivot ? '1' : '0'; o += ','; o += hex(s.pivUp); o += ','; o += hex(s.pivDn); o += ','; o += (char)('0' + s.pivW); o += '\n';   // (1.8.0; 1.8.1 + colours, width)
    return o;
}

// Parse a settings file's text. Returns false (out = defaults) when the text is not a version-1 settings file at all; a single
// bad line keeps only that item's default. `bad` counts the lines that were rejected (for the status file).
inline bool fromText(const std::string& text, Style& out, int* bad = NULL) {
    Style s; int nb = 0;
    out = Style();
    if (text.empty() || text.size() > FILE_MAX) { if (bad) *bad = 1; return false; }
    std::vector<std::string> L = split(text, '\n');
    bool versionOk = false;
    for (size_t i = 0; i < L.size(); ++i) {
        std::string ln = L[i];
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.empty()) continue;
        std::vector<std::string> f = split(ln, ',');
        if (!versionOk) { if (f.size() == 2 && f[0] == "VERSION" && f[1] == "1") { versionOk = true; continue; } if (bad) *bad = 1; return false; }
        int k = -1;
        if (f[0] == "VWAP") k = K_VWAP; else if (f[0] == "P1") k = K_P1; else if (f[0] == "M1") k = K_M1;
        else if (f[0] == "P2") k = K_P2; else if (f[0] == "M2") k = K_M2;
        if (k >= 0) {
            bool b; Colour c; int w = 0;
            const size_t want = k == K_VWAP ? 4 : 3;
            if (f.size() != want || !parseBit(f[1], b) || !parseHex(f[2], c) || nearBlack(c) || (k == K_VWAP && !parseWidth(f[3], w))) { ++nb; continue; }
            s.show[k] = b; s.col[k] = c; if (k == K_VWAP) s.vwapW = w;
        } else if (f[0] == "BANDW") {
            int w = 0; if (f.size() != 2 || !parseWidth(f[1], w)) { ++nb; continue; } s.bandW = w;
        } else if (f[0] == "ON") {
            bool b; if (f.size() != 2 || !parseBit(f[1], b)) { ++nb; continue; } s.on = b;
        } else if (f[0] == "PRIOR") {
            bool b; Colour c; if (f.size() != 3 || !parseBit(f[1], b) || !parseHex(f[2], c) || nearBlack(c)) { ++nb; continue; }
            s.prior = b; s.priorCol = c;
        } else if (f[0] == "PIVOT") {   // (1.8.0) absent in an older file = the default (on)
            bool b; if ((f.size() != 2 && f.size() != 5) || !parseBit(f[1], b)) { ++nb; continue; }
            if (f.size() == 5) {   // (1.8.1) PIVOT,<b>,<above>,<below>,<width>
                Colour u, d; int w = 0;
                if (!parseHex(f[2], u) || nearBlack(u) || !parseHex(f[3], d) || nearBlack(d) || !parseWidth(f[4], w)) { ++nb; continue; }
                s.pivUp = u; s.pivDn = d; s.pivW = w;
            }
            s.pivot = b;
        }   // unknown keys: ignored (a newer file read by an older plugin)
    }
    if (!versionOk) { if (bad) *bad = 1; return false; }
    out = s; if (bad) *bad = nb;
    return true;
}

// ---- the settings window ----------------------------------------------------------------------------------------------------
// Parameter rows, in order (IRT keeps saved values BY POSITION: rows may only be appended below the last one).
enum Row {
    R_VWAP = 0, R_VWAP_COL, R_VWAP_W,
    R_P1, R_P1_COL, R_M1, R_M1_COL,
    R_P2, R_P2_COL, R_M2, R_M2_COL,
    R_BAND_W, R_ON, R_PRIOR, R_PRIOR_COL,
    R_PIVOT,            // (1.8.0) appended: the 3-bar pivot VWAP on / off
    R_PIV_UP, R_PIV_DN, R_PIV_W,   // (1.8.1) appended: the pivot VWAP's colours + width
    R_COUNT
};
static const char* WIDTH_CHOICES = "1 px;2 px;3 px;4 px";

// What the window holds, as read by the host (lists as indexes, -1 = not readable, colours raw).
struct Dialog {
    int vwapW = -1, bandW = -1;
    bool show[K_N] = { true, true, true, true, true };
    Colour col[K_N] = { 0, 0, 0, 0, 0 };
    bool on = true, prior = true, pivot = true;
    Colour priorCol = 0;
};
// The window's values -> a Style. False = the window is not really open (its width lists do not read a real choice), so
// nothing may be saved. A colour that reads near-black keeps `keep`'s colour (IRT has read colour rows back as black).
inline bool fromDialog(const Dialog& d, const Style& keep, Style& out) {
    if (d.vwapW < 0 || d.vwapW > W_MAX - W_MIN || d.bandW < 0 || d.bandW > W_MAX - W_MIN) return false;
    Style s = keep;
    s.vwapW = W_MIN + d.vwapW; s.bandW = W_MIN + d.bandW;
    for (int k = 0; k < K_N; ++k) { s.show[k] = d.show[k]; const Colour c = d.col[k] & 0xFFFFFFUL; if (!nearBlack(c)) s.col[k] = c; }
    s.on = d.on; s.prior = d.prior; s.pivot = d.pivot;
    { const Colour c = d.priorCol & 0xFFFFFFUL; if (!nearBlack(c)) s.priorCol = c; }
    out = s; return true;
}

// ---- the per-market file ----------------------------------------------------------------------------------------------------
inline std::string marketPart(const std::string& m) {
    std::string r;
    for (size_t i = 0; i < m.size() && r.size() < 8; ++i) { const char c = m[i]; r += ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) ? c : '_'; }
    return r;
}
inline std::string pathFor(const char* profile, const std::string& market) {
    if (!profile || !profile[0] || market.empty()) return std::string();
    return std::string(profile) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionVWAP.settings-" + marketPart(market) + ".txt";
}
inline long long fileStamp(const std::string& p) {
    if (p.empty()) return -1;
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return -1;       // as lsHodLod / lsTapeReader (MSVC)
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return -1;
#endif
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}
// Bounded read with the stamp checked before and after (a torn read is retried on the next pass). 0 ok, 1 missing, 2 bad.
inline int readFile(const std::string& p, std::string& text, long long& stamp) {
    text.clear(); stamp = fileStamp(p);
    if (stamp < 0) return 1;
    std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) return 1;
    std::vector<char> buf(FILE_MAX + 1);
    f.read(&buf[0], (std::streamsize)buf.size());
    const std::streamsize got = f.gcount();
    if (got <= 0 || (size_t)got > FILE_MAX) return 2;
    text.assign(&buf[0], (size_t)got);
    if (fileStamp(p) != stamp) return 2;
    return 0;
}

} // namespace svs

#endif
