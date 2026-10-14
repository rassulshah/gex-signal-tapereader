#ifndef LS_BANNERBOARD_H
#define LS_BANNERBOARD_H
// BannerBoard.h (1.0.1, 2026-10-14, CR4800 / CR5000) -- the ONE top banner of a chart, shared by lsTapeReader and lsLiquidityProfile.
// Rassul 07:12 / 07:13 (variant B, "one banner"): the Tape Reader draws a 1-2 line banner at the top left where the Liquidity warning
// sits today - line 1 its read (bias / turn / swing / next), line 2 the Liquidity warning. The Liquidity Profile stops drawing its own
// warning while a Tape Reader is on the same chart and hands the warning over; with no Tape Reader it draws it as before.
//   LRA_LQW_<symbol>_<spb>_<chart>  (Liquidity -> Tape Reader)  "<ms>|<favour>|<name>|<detail>"   name "" = no warning now
//   LRA_TRB_<symbol>_<spb>_<chart>  (Tape Reader -> Liquidity)  "<ms>|<l>|<t>|<r>|<b>"            the banner's reserved rectangle
// The process environment block (as HoverBoard.h): shared by every plugin DLL in the IRT process, no files. <chart> = a hash of the
// chart's label (getChartLabel: the same for every indicator on one chart, different for another chart of the same symbol), so the
// match never depends on the draw order or the visible range. <ms> = a steady clock; an entry older than FRESH_MS is ignored (the
// other plugin was removed / its chart closed); the Tape Reader clears its entry when it is removed.
// (1.0.1, CR5000 heartbeat) "is the other plugin still on this chart" is decided by bb::Beat, not by the 60 s age alone: present while
// its stamp keeps moving between this plugin's draws, or it is under BEAT_MS (5 s) old, or fewer than BEAT_MISS (3) of this plugin's
// draws went by without a new stamp. Both plugins draw on every repaint of the chart, so a removed one is noticed within ~5 s / 3
// draws, and an idle chart (no repaint for minutes) never flickers on its first repaint (whichever plugin draws first). The Liquidity
// Profile clears its own entry when it is removed, as the Tape Reader does.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#if defined(_WIN32) && !defined(HB_FAKE)
#include <windows.h>
#endif
namespace bb {
static const char* const VERSION = "1.0.1";
static const long long FRESH_MS = 60000;
static const long long BEAT_MS = 5000; static const int BEAT_MISS = 3;
struct Warn { bool on = false; int favour = 0; std::string name, detail; };
struct Rect { int l = 0, t = 0, r = 0, b = 0; };
inline unsigned chartHash(const std::string& label) { unsigned h = 2166136261u; for (unsigned char c : label) { h ^= c; h *= 16777619u; } return h; }
inline std::string key(const char* pre, const std::string& sym, long spb, const std::string& chart) {
    std::string o = pre;
    for (size_t i = 0; i < sym.size() && i < 24; ++i) { const char c = sym[i]; if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) o += c; }
    return o + "_" + std::to_string(spb > 0 ? spb : 0) + "_" + std::to_string(chartHash(chart));
}
inline std::vector<std::string> split(const std::string& s, char d, size_t maxParts) {
    std::vector<std::string> f; size_t a = 0;
    while (f.size() + 1 < maxParts) { const size_t c = s.find(d, a); if (c == std::string::npos) break; f.push_back(s.substr(a, c - a)); a = c + 1; }
    f.push_back(s.substr(a)); return f;
}
inline bool num(const std::string& s, long long& v) {
    if (s.empty() || s.size() > 20) return false;
    size_t i = s[0] == '-' ? 1 : 0; if (i == s.size()) return false;
    for (size_t k = i; k < s.size(); ++k) if (s[k] < '0' || s[k] > '9') return false;
    v = std::strtoll(s.c_str(), nullptr, 10); return true;
}
inline std::string clean(const std::string& s, size_t cap) { std::string o; for (char c : s) { if (o.size() >= cap) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline std::string encWarn(long long ms, const Warn& w) {
    return std::to_string(ms) + "|" + std::to_string(w.on ? (w.favour > 0 ? 1 : w.favour < 0 ? -1 : 0) : 0) + "|" + (w.on ? clean(w.name, 60) : "") + "|" + (w.on ? clean(w.detail, 200) : "");
}
// the warning for this chart, fresh; false = no Liquidity Profile on the chart (or a stale entry)
inline bool decWarn(const std::string& s, long long nowMs, Warn& out) {
    out = Warn(); if (s.empty() || s.size() > 400) return false;
    const std::vector<std::string> f = split(s, '|', 4); if (f.size() != 4) return false;
    long long ms = 0, fav = 0;
    if (!num(f[0], ms) || !num(f[1], fav) || fav < -1 || fav > 1) return false;
    if (nowMs >= 0 && (nowMs < ms || nowMs - ms > FRESH_MS)) return false;   // nowMs < 0 = any age (the heartbeat decides)
    out.on = !f[2].empty(); out.favour = (int)fav; out.name = f[2]; out.detail = f[3]; return true;
}
// the stamp of an encoded entry (-1 = none / malformed)
inline long long stampOf(const std::string& s) { const size_t c = s.find('|'); long long ms = -1; if (c == std::string::npos || !num(s.substr(0, c), ms) || ms < 0) return -1; return ms; }
inline std::string encBanner(long long ms, const Rect& r) {
    return std::to_string(ms) + "|" + std::to_string(r.l) + "|" + std::to_string(r.t) + "|" + std::to_string(r.r) + "|" + std::to_string(r.b);
}
inline bool decBanner(const std::string& s, long long nowMs, Rect& out) {
    out = Rect(); if (s.empty() || s.size() > 120) return false;
    const std::vector<std::string> f = split(s, '|', 5); if (f.size() != 5) return false;
    long long v[5]; for (int k = 0; k < 5; ++k) if (!num(f[(size_t)k], v[k])) return false;
    if (nowMs >= 0 && (nowMs < v[0] || nowMs - v[0] > FRESH_MS)) return false;
    if (v[3] <= v[1] || v[4] <= v[2] || v[3] - v[1] > 8000 || v[4] - v[2] > 4000) return false;
    out.l = (int)v[1]; out.t = (int)v[2]; out.r = (int)v[3]; out.b = (int)v[4]; return true;
}
// ---- the process-wide calls (Win32), or the Linux test fake (setenv) --------------------------------------------------------
#if defined(_WIN32) && !defined(HB_FAKE)
inline void bbSet(const std::string& n, const std::string& v) { SetEnvironmentVariableA(n.c_str(), v.empty() ? NULL : v.c_str()); }
inline bool bbGet(const std::string& n, std::string& v) {
    char b[512]; const DWORD k = GetEnvironmentVariableA(n.c_str(), b, (DWORD)sizeof(b));
    if (k == 0 || k >= sizeof(b)) return false;
    v.assign(b, (size_t)k); return true;
}
#else
inline void bbSet(const std::string& n, const std::string& v) { if (v.empty()) unsetenv(n.c_str()); else setenv(n.c_str(), v.c_str(), 1); }
inline bool bbGet(const std::string& n, std::string& v) { const char* s = std::getenv(n.c_str()); if (!s) return false; v = s; return true; }
#endif
inline void publishWarn(const std::string& sym, long spb, const std::string& chart, long long ms, const Warn& w) { if (!sym.empty()) bbSet(key("LRA_LQW_", sym, spb, chart), encWarn(ms, w)); }
inline bool readWarn(const std::string& sym, long spb, const std::string& chart, long long ms, Warn& w) { std::string v; return !sym.empty() && bbGet(key("LRA_LQW_", sym, spb, chart), v) && decWarn(v, ms, w); }
inline void publishBanner(const std::string& sym, long spb, const std::string& chart, long long ms, const Rect& r) { if (!sym.empty()) bbSet(key("LRA_TRB_", sym, spb, chart), encBanner(ms, r)); }
inline bool readBanner(const std::string& sym, long spb, const std::string& chart, long long ms, Rect& r) { std::string v; return !sym.empty() && bbGet(key("LRA_TRB_", sym, spb, chart), v) && decBanner(v, ms, r); }
// (1.0.1) the entry at any age + its stamp: the caller's Beat decides whether the other plugin is still there
inline bool peekWarn(const std::string& sym, long spb, const std::string& chart, Warn& w, long long& ms) { std::string v; ms = -1; if (sym.empty() || !bbGet(key("LRA_LQW_", sym, spb, chart), v) || !decWarn(v, -1, w)) return false; ms = stampOf(v); return ms >= 0; }
inline bool peekBanner(const std::string& sym, long spb, const std::string& chart, Rect& r, long long& ms) { std::string v; ms = -1; if (sym.empty() || !bbGet(key("LRA_TRB_", sym, spb, chart), v) || !decBanner(v, -1, r)) return false; ms = stampOf(v); return ms >= 0; }
inline void clearWarn(const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) bbSet(key("LRA_LQW_", sym, spb, chart), std::string()); }
// (1.0.1) the heartbeat of the OTHER plugin on this chart, kept per chart by the reader; step() once per own draw
struct Beat {
    long long seen = -1; int same = 0;
    bool step(bool have, long long stamp, long long nowMs) {
        if (!have || stamp < 0) { seen = -1; same = 0; return false; }
        if (stamp != seen) { seen = stamp; same = 0; return true; }        // it drew since my last draw: present
        ++same;
        return same < BEAT_MISS || (nowMs >= stamp && nowMs - stamp < BEAT_MS);
    }
    void reset() { seen = -1; same = 0; }
};
inline void clearBanner(const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) bbSet(key("LRA_TRB_", sym, spb, chart), std::string()); }
} // namespace bb
#endif
