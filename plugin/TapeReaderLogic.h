// TapeReaderLogic.h -- lsTapeReader 1.0.1 (2026-10-13; 1.0.0 + GAMMA feed fixes), SDK-free: every input parser, the four voters, the bias, the three lines,
// 1.1.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md, mockup orderflow-contracts-mockup.png):
//   LEVELS section REMOVED from the box (still computed and logged; levels are named inside BUYERS / SELLERS); OPTIONS section added
//   (2 lines, blue, OptionsFlow.h: "Put buyers fading (IV dn); PS 5790 wall +40% ?" / "Dealers buy 1.2K ES into the bounce (oHit 2.4x ?)",
//   from LRA-Options-<MKT>.csv as it had ARRIVED by the candle and fresh <= 20 min, '?' until the LRA-OrderFlow gates say on);
//   BUYERS / SELLERS in the panes' SPEC language (lots, ratio, pace); default Position Bottom centre; the log gains an options field.
// line 1, the box layout and the per-candle log line. Unit-tested on Linux (tests/test_tr_logic.cpp); TapeReader.cpp only feeds it
// the chart's bars and the files and executes the draw list.
//
// Rassul 2026-10-13: "add a reader box. call it tape reader ... give your read on what buyers and sellers are doing by reading the
// indicators as well as other things like levels, gamma, liquidity", "split it up by buyers and sellers", "the turn up swing and
// next stat should be centered"; 11:55 "BUYERS, SELLERS and LEVELS may each use up to 2 lines".
//
// INPUTS (all lsFlexLevels\, each optional; a missing / stale / garbled input is left out of the read and listed in the status file):
//   Delta Profile 2.7.0  DeltaProfile\<MKT>-<contract>-<spb>-signals-<session days>.csv   D lines (A), AE lines (AE? / AE), E lines (Ex?)
//   BidAskStats 1.3.0 / PullStats 1.0.0 / StackStats 1.0.0  <Name>\<MKT>-<CONTRACT>-<spb>-<YYYYMMDD>.csv   (OrderFlowFeed.h session
//                        record: header-driven, letters S / L / I (legacy R = I), spikes, implication, level, message)
//   Liquidity Profile 0.14.0  LRA-LiqZones-<MKT>.csv (Z lines) + LiquidityProfile.status-<MKT>-<spb>.txt (ZONE near / far, CONFIRM = LQ)
//   LRA-KeyLevels-<MKT>.csv, LRA-Levels-<MKT>.csv, LRA-Touch-<MKT>.csv, LRA-Dealer-<MKT>.csv (DealerLogic.h parse, sameContract)
//   HodLod.status-<MKT>-<spb>.txt  FIRST / READ_VALUES (P in) / RANGE (used %) / PROJ
//   Trend: computed natively from the chart's bars with VwapTrendLogic.h (the same rule lsSessionVWAP publishes).
//   LRA-Turns-<MKT>.csv (nightly, file VERSION 2 since 1.0.1) turn / swing odds; LRA-TapeReader-<MKT>.csv (NEW, nightly) voter weights. Formats: NOTES.md.
#ifndef LS_TAPEREADER_LOGIC_H
#define LS_TAPEREADER_LOGIC_H
#include "TapeReaderText.h"
#include "BoxPlace.h"
#include "BannerBoard.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#include "OptionsFlow.h"
#include "ClaimGates.h"          // (1.1.4) peer-review claim gates: A is not a vote / message until the external audit

namespace tr {

static const char* const VERSION = "1.3.4";   // 1.3.3 (2026-10-18 23:37 CT, Rassul "get rid of the hover over ..."): the banner detail popup off (TapeReader.cpp TR_BANNER_HOVER), logic unchanged; // 1.3.2 (2026-10-18, his decision list): Offset X / Y removed (9-spot Position only);  1.3.1 (2026-10-18 settings audit): the settings window from TapeReaderSettings.h / SettingsSpec.h (keyed TapeReader.settings-<MKT>.txt, migrated once from TapeReader.place-<MKT>.txt); same rows, drawing unchanged // 1.3.0 (2026-10-18): the Market State (MarketState.h) - every indicator's facts read, the zone story (ZoneStory.h) stepped per closed bar, odds ZONE_HOLD, SRC READ posted, lsFlexLevels\MarketState\<MKT>-<spb>.json every 2 s for Claude; nothing new drawn. // was 1.2.1;   // 1.2.1 (2026-10-14 20:07, Rassul "no AE"): AE records no longer read - no AE text, tag or vote. // 1.2.0 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section 3): line 1 shows no BULLISH / BEARISH n of 4 bias (ClaimGates.h LRA_SHOW_TAPE_BIAS) and no turn / swing % - "Turn odds learning" (LRA_SHOW_TURN_ODDS); Tgt without %; Pull lines without "support / resistance leaving"; "Added at PDL: 42 bids, 150 lots (4.4x) fast, 90% re-added after hits"; "reloading? (I 2.1x)", "iceberg?"; "(Ex?)" without "exhausted"; "Bids 2.6x normal at PDL ?"; "+GEX PDL 6,020 (model, 7m old) ?"; the OPTIONS lines keep their ? (ODEX too) and say the grid age. Logged reads unchanged except the reworded phrases.  // 1.1.4 (2026-10-14, Rassul 15:07 CT approval, peer review E "Delta Profile A ... INVALID"): the A (absorption) is no longer a FLOW vote or a BUYERS / SELLERS message (ClaimGates.h LRA_SHOW_ABSORPTION_REASON, default hidden; the A circle stays on the chart); the LEVELS line shows the gamma node without the "(dlrs buy dips / sell rips / chase)" cause (LRA_SHOW_HVL_CUSHION).   // 1.1.3 (2026-10-14 CR4800, Rassul 07:13 variant B): the ONE banner top left (bias / turn / swing / next + the Liquidity warning, 11 pt, solid background), the BUYERS / SELLERS / OPTIONS detail on its hover, Position Top left + Offset X / Y; 1.1.2 (2026-10-14): the hourly swings (HrHI / HrLO, pivot_1h) are no longer targets ("Next:"), AsiaHI / AsiaLO and CWH / CWL bucketed as asia / current_week; 1.1.1 (2026-10-13 9:40 PM): 12 / 11 pt, never over the HOD / LOD table or price; 1.1.0 (2026-10-13): LEVELS removed, OPTIONS (blue), BUYERS / SELLERS in the panes' SPEC language (lots / ratio / pace), default Bottom centre
// (6:40 PM) StackStats' one-order clause = st::ONE_ORDER_TAIL (StackStatsLogic.h); test_tr_ofcompat.cpp checks the two are equal
static const char* const STACK_ONE_ORDER = "may not reload ?";
enum { NEUTRAL = 0, BULL = 1, BEAR = -1 };
enum { V_FLOW = 0, V_LEVEL = 1, V_GAMMA = 2, V_TREND = 3, NVOTE = 4 };
static const char* const VOTE_NAME[NVOTE] = { "FLOW", "LEVEL", "GAMMA", "TREND" };

// ---- small helpers ----------------------------------------------------------------------------------------------------------
inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }
inline std::vector<std::string> split(const std::string& line, char sep) {
    std::vector<std::string> c; size_t a = 0;
    for (;;) { const size_t p = line.find(sep, a); c.push_back(line.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; }
    return c;
}
inline std::string trim(const std::string& s) { size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r"); return a == std::string::npos ? std::string() : s.substr(a, b - a + 1); }
// strict full-token numbers (checklist #4)
inline bool parseReal(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40 || t[0] == ' ' || t[0] == '\t') return false;
    errno = 0; char* e = NULL; const double v = std::strtod(t.c_str(), &e);
    if (errno != 0 || e != t.c_str() + t.size() || !finite(v)) return false;
    out = v; return true;
}
inline bool parseWhole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 24 || t[0] == ' ' || t[0] == '\t') return false;
    errno = 0; char* e = NULL; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno != 0 || e != t.c_str() + t.size() || v < lo || v > hi) return false;
    out = v; return true;
}
inline std::string clean(const std::string& s, size_t maxLen) { std::string o; for (char c : s) { if (o.size() >= maxLen) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline std::string num1(double r) { if (!finite(r) || r < 0) return "n/a"; char b[24]; std::snprintf(b, sizeof(b), "%.1f", r > 999 ? 999.0 : r); return b; }
inline std::string ratio(double r) { if (!finite(r) || r < 0) return "n/a"; char b[24]; std::snprintf(b, sizeof(b), "%.1fx", r > 999 ? 999.0 : r); return b; }
inline int decimalsFor(double tick) {
    if (!(tick > 0) || !finite(tick)) return 2;
    for (int d = 0; d <= 6; ++d) { const double s = tick * std::pow(10.0, d); if (std::fabs(s - std::floor(s + 0.5)) < 1e-6) return d; }
    return 6;
}
// a price at tick precision ("92.86", "7842.25")
inline std::string pxText(double px, double tick) {
    if (!finite(px)) return "n/a";
    if (tick > 0 && finite(tick)) px = std::floor(px / tick + 0.5) * tick;
    char f[16], b[48]; std::snprintf(f, sizeof(f), "%%.%df", decimalsFor(tick)); std::snprintf(b, sizeof(b), f, px); return b;
}
inline long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline void civilFromDays(long long z, int& y, unsigned& m, unsigned& d) {
    z += 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const long long yy = (long long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1; m = mp < 10 ? mp + 3 : mp - 9; y = (int)(yy + (m <= 2));
}
// "10:30 AM" from a Central-clock second of the day
inline std::string clock12(long long ctSec) {
    long long s = ctSec % 86400; if (s < 0) s += 86400; const int h = (int)(s / 3600), mi = (int)(s % 3600 / 60);
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, mi, h < 12 ? "AM" : "PM"); return b;
}
// the 17:00 CT session of a Central-clock epoch second: day number (Delta Profile's file name) and YYYYMMDD (order-flow files, our log)
inline long long sessionDays(long long ctSec) { const long long d = ctSec >= 0 ? ctSec / 86400 : -((-ctSec + 86399) / 86400); return d + ((ctSec - d * 86400) >= 17 * 3600 ? 1 : 0); }
inline std::string sessionYmd(long long ctSec) { int y; unsigned m, d; civilFromDays(sessionDays(ctSec), y, m, d); char b[16]; std::snprintf(b, sizeof(b), "%04d%02u%02u", y, m, d); return b; }
// file-name tokens of the other plugins: Delta Profile 2.6.3 contractToken, OrderFlowFeed / PullStats fileKey
inline std::string contractToken(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 64; ++i) { const unsigned char c = (unsigned char)s[i];
        out += ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') ? (char)c : '_'; }
    return out.empty() ? "unknown-contract" : out;
}
inline std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}

// ---- files: bounded, torn-safe ---------------------------------------------------------------------------------------------
// the file's identity: modified time at the best resolution the platform gives (100 ns FILETIME on Windows when windows.h is in,
// nanoseconds on Linux) mixed with its size; -1 = no regular file. mtime / size out for freshness and the small-file re-check.
inline long long fileSig(const std::string& path, long long* mtime = nullptr, long long* size = nullptr) {
    unsigned long long t = 0, frac = 0, sz = 0; long long sec = 0;
#if defined(_WIN32) && defined(_WINDOWS_)
#if defined(_MSC_VER)
#pragma comment(lib, "kernel32.lib")   // (1.0.1 link fix) GetFileAttributesExA
#endif
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &a) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return -1;
    t = ((unsigned long long)a.ftLastWriteTime.dwHighDateTime << 32) | a.ftLastWriteTime.dwLowDateTime;
    sz = ((unsigned long long)a.nFileSizeHigh << 32) | a.nFileSizeLow;
    sec = t >= 116444736000000000ULL ? (long long)((t - 116444736000000000ULL) / 10000000ULL) : 0; frac = t % 10000000ULL;
#elif defined(_WIN32)
    struct _stat64 st; if (_stat64(path.c_str(), &st) != 0 || (st.st_mode & S_IFMT) != S_IFREG) return -1;
    sec = (long long)st.st_mtime; sz = (unsigned long long)st.st_size;
#else
    struct stat st; if (stat(path.c_str(), &st) != 0 || (st.st_mode & S_IFMT) != S_IFREG) return -1;
    sec = (long long)st.st_mtim.tv_sec; frac = (unsigned long long)st.st_mtim.tv_nsec; sz = (unsigned long long)st.st_size;
#endif
    t = (unsigned long long)sec;
    if (mtime) *mtime = sec;
    if (size) *size = (long long)sz;
    const unsigned long long h = t * 1000000007ULL ^ (frac + 1) * 2654435761ULL ^ (sz + 7) * 0x9E3779B97F4A7C15ULL;
    return (long long)(h & 0x3FFFFFFFFFFFFFFFULL);
}
enum ReadRes { RD_OK = 0, RD_MISSING = 1, RD_TOOBIG = 2, RD_TORN = 3 };
// the whole file (<= maxBytes), or with tail = true its last maxBytes from the first complete line. The stamp must be the same
// before and after the read (a file replaced mid-read is not used: the caller keeps its last good copy).
inline int readBounded(const std::string& path, size_t maxBytes, std::string& out, long long& sig, bool tail = false) {
    out.clear();
    const long long s0 = fileSig(path); sig = s0;
    if (s0 < 0) return RD_MISSING;
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f.is_open()) return RD_MISSING;
    f.seekg(0, std::ios::end); const long long size = (long long)f.tellg();
    if (size < 0) return RD_TORN;
    long long from = 0;
    if ((unsigned long long)size > (unsigned long long)maxBytes) { if (!tail) return RD_TOOBIG; from = size - (long long)maxBytes; }
    f.seekg(from, std::ios::beg);
    out.resize((size_t)(size - from));
    if (!out.empty()) f.read(&out[0], (std::streamsize)out.size());
    if ((long long)f.gcount() != size - from) { out.clear(); return RD_TORN; }
    if (fileSig(path) != s0) { out.clear(); return RD_TORN; }
    if (from > 0) { const size_t nl = out.find('\n'); out = nl == std::string::npos ? std::string() : out.substr(nl + 1); }
    return RD_OK;
}
// lines; completeOnly drops a last line without its newline (an append-only record being written)
inline std::vector<std::string> lines(const std::string& text, bool completeOnly, size_t maxLines = 20000, size_t maxLine = 8192) {
    std::vector<std::string> v; size_t a = 0;
    while (a < text.size() && v.size() < maxLines) {
        const size_t e = text.find('\n', a);
        if (e == std::string::npos) { if (!completeOnly) { std::string l = text.substr(a); if (!l.empty() && l.back() == '\r') l.pop_back(); if (l.size() <= maxLine) v.push_back(l); } break; }
        std::string l = text.substr(a, e - a); if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.size() <= maxLine) v.push_back(l);
        a = e + 1;
    }
    return v;
}

// Tape Reader's own input cache. A path is part of a source identity: at a session rollover the new
// record file must be read even if its size/time signature collides with the previous session's file.
struct InputSource {
    std::string path, seenPath;
    long long sig = -2, checked = -1, mtime = 0, size = 0, readAt = -1;
    int res = RD_MISSING; std::string note = "not read"; bool good = false;
    bool resetForPath() {
        if (path == seenPath) return false;
        seenPath = path; sig = -2; checked = -1; mtime = size = 0; readAt = -1;
        res = RD_MISSING; note = "path changed"; good = false;
        return true;
    }
};
// Stat at most every `every` ms. A changed path resets identity before throttling or comparing the
// path-independent file stamp, so a session/timeframe switch cannot reuse an older snapshot.
inline bool checkInput(InputSource& s, long long nowMs, long long every) {
    s.resetForPath();
    if (s.checked >= 0 && nowMs >= s.checked && nowMs - s.checked < every) return false;
    s.checked = nowMs;
    long long mt = 0, sz = 0; const long long sig = fileSig(s.path, &mt, &sz);
    const bool recheck = sig >= 0 && sz <= 256 * 1024 && (s.readAt < 0 || nowMs - s.readAt >= 10000 || nowMs < s.readAt);
    if (sig == s.sig && !recheck) return false;
    s.sig = sig; s.mtime = mt; s.size = sz; s.readAt = nowMs;
    return true;
}

// ---- the chart's bars ------------------------------------------------------------------------------------------------------
struct Bar { unsigned long long raw = 0; long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };   // t = Central-clock epoch s of the bar END
inline double atrOf(const std::vector<Bar>& B, int k, int n = 20) {
    double s = 0; int m = 0;
    for (int i = k; i >= 0 && m < n; --i) { const double r = B[(size_t)i].h - B[(size_t)i].l; if (finite(r) && r >= 0) { s += r; ++m; } }
    return m ? s / m : 0;
}

// ---- levels (key + MenthorQ + touch odds + gamma nodes) ----------------------------------------------------------------------
struct Level { std::string name, kind; double px = 0; std::string odds; bool q = true; };   // odds "34" (percent), q = not proven
struct GNode { double px = 0, g = 0; };
// (1.0.1, DEX audit 2026-10-13) the GAMMA voter's feed. LRA-Dealer NODE rows carry the synthetic gamma column g and MenthorQ's own
// net GEX $ (usd). In ES / NQ cash hours dealer_irt writes the SPX / QQQ index book with g = 0 on every row and only usd filled, so a
// g-only read was blind. Rule: g when any row has g != 0, else usd (same sign: + = long gamma); neither -> GAMMA abstains ("?").
// Freshness is the DATA's own time (ASOF, or the index book's SRC time when older), not the file's save time: older than 15 min in
// RTH (weekdays) -> GAMMA abstains, "stale 18h"; outside RTH the old 18 h allowance stays.
enum { GF_OK = 0, GF_NONE = 1, GF_STALE = 2 };
struct DealerNodeIn { double px = 0, g = 0, usd = 0; bool hasUsd = false; };
struct GammaFeed { int state = GF_OK; bool fromUsd = false; long long ageS = -1; std::vector<GNode> nodes; };
static const long long GAMMA_RTH_MAX_S = 15 * 60, GAMMA_OFF_MAX_S = 18 * 3600;
// the data's CT time (civil-day seconds, the bars' clock) from ASOF|<sec of day>|<yyyy-mm-dd> and SRC|..|..|..|<HH:MM>; -1 = unknown
// (1.0.1, MQG100) BOOKASOF|<CT sec-of-day>|<yyyy-mm-dd> = when the strike grid behind the file was taken (ASOF is the BUILD time:
// the bridge rebuilds every few minutes from whatever grid it has). "BOOKASOF||" = no grid at all.
// Returns the CT time, BOOK_NONE when the row is there but empty / garbled, BOOK_ABSENT when the file has no such row (older writer).
static const long long BOOK_ABSENT = -2, BOOK_NONE = -1;
inline long long parseBookAsof(const std::vector<std::string>& L) {
    long long r = BOOK_ABSENT;
    for (size_t i = 0; i < L.size(); ++i) {
        if (L[i].compare(0, 9, "BOOKASOF|") != 0) continue;
        const std::vector<std::string> c = split(L[i], '|');
        long long so = -1; int y = 0, mo = 0, d = 0; char tail = 0;
        if (c.size() >= 3 && parseWhole(c[1], 0, 86399, so) && std::sscanf(c[2].c_str(), "%d-%d-%d%c", &y, &mo, &d, &tail) == 3 &&
            y >= 2000 && y <= 2200 && mo >= 1 && mo <= 12 && d >= 1 && d <= 31) r = civilDays(y, (unsigned)mo, (unsigned)d) * 86400LL + so;
        else r = BOOK_NONE;
    }
    return r;
}
// bookCt: BOOKASOF (see above). Present -> the data time is the book's time (and the index book's SRC time when older); present but
// empty -> unknown (-1, GAMMA abstains); absent -> ASOF as before.
inline long long dealerDataCt(int y, int mo, int d, double asofSo, const std::string& srcAt, long long bookCt = BOOK_ABSENT) {
    if (bookCt == BOOK_NONE) return -1;
    if (asofSo < 0 || asofSo >= 86400 || y < 2000 || y > 2200 || mo < 1 || mo > 12 || d < 1 || d > 31) return bookCt >= 0 ? bookCt : -1;
    const long long day = civilDays(y, (unsigned)mo, (unsigned)d) * 86400LL;
    long long t = bookCt >= 0 ? bookCt : day + (long long)asofSo;
    int hh = -1, mm = -1; char tail = 0;
    if (srcAt.size() >= 4 && srcAt.size() <= 5 && std::sscanf(srcAt.c_str(), "%d:%d%c", &hh, &mm, &tail) == 2 && hh >= 0 && hh < 24 && mm >= 0 && mm < 60) {
        long long s = day + hh * 3600LL + mm * 60LL;
        if (s > day + (long long)asofSo + 600) s -= 86400;   // the book's time is just before midnight of the previous day
        if (s < t) t = s;                     // the older of the two is when the gamma values were taken
    }
    return t;
}
inline bool weekdayCt(long long ct) { const long long days = (ct >= 0 ? ct : ct - 86399) / 86400; const long long w = ((days % 7) + 7 + 4) % 7; return w >= 1 && w <= 5; }   // 1970-01-01 was a Thursday
// how old the gamma data may be at nowCt: 15 min inside RTH on a weekday, else 18 h
inline long long gammaMaxAgeS(long long nowCt, int rthOpenMin, int rthCloseMin) {
    const long long sod = ((nowCt % 86400) + 86400) % 86400;
    const bool rth = rthOpenMin >= 0 && rthCloseMin > rthOpenMin && weekdayCt(nowCt) && sod >= rthOpenMin * 60LL && sod < rthCloseMin * 60LL;
    return rth ? GAMMA_RTH_MAX_S : GAMMA_OFF_MAX_S;
}
inline std::string ageText(long long s) {
    if (s < 0) return "?";
    if (s < 3600) return std::to_string(std::max(1LL, s / 60)) + "m";
    if (s < 48 * 3600) return std::to_string(s / 3600) + "h";
    return std::to_string(s / 86400) + "d";
}
// dataCt < 0 = no data time in the file -> abstain; a data time more than 10 min ahead of the clock -> abstain
inline GammaFeed gammaFeed(bool have, const std::vector<DealerNodeIn>& rows, long long dataCt, long long nowCt, int rthOpenMin, int rthCloseMin) {
    GammaFeed F;
    if (!have || dataCt < 0 || nowCt <= 0 || dataCt - nowCt > 600) { F.state = GF_NONE; return F; }
    F.ageS = std::max(0LL, nowCt - dataCt);
    if (F.ageS > gammaMaxAgeS(nowCt, rthOpenMin, rthCloseMin)) { F.state = GF_STALE; return F; }
    bool anyG = false, anyUsd = false;
    for (const DealerNodeIn& r : rows) { if (r.px > 0 && finite(r.g) && r.g != 0) anyG = true; if (r.px > 0 && r.hasUsd && finite(r.usd) && r.usd != 0) anyUsd = true; }
    if (!anyG && !anyUsd) { F.state = GF_NONE; return F; }
    F.fromUsd = !anyG;
    for (const DealerNodeIn& r : rows) {
        if (r.px <= 0) continue;
        const double v = anyG ? r.g : (r.hasUsd ? r.usd : 0.0);
        if (!finite(v)) continue;
        GNode n; n.px = r.px; n.g = v; F.nodes.push_back(n);
    }
    return F;
}
struct Touch { std::string code; double px = 0; std::string p1; bool proven = false; };
// LRA-KeyLevels-<MKT>.csv: "KEYLV,1,<unix>" then L|name|price|pivot. false = garbled
inline bool parseKeyLevels(const std::vector<std::string>& L, long long& at, std::vector<Level>& out) {
    out.clear(); at = 0;
    if (L.empty() || L[0].compare(0, 8, "KEYLV,1,") != 0 || !parseWhole(L[0].substr(8), 1, 32503680000LL, at)) return false;
    for (size_t i = 1; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], '|'); double px;
        if (c.size() >= 3 && c[0] == "L" && !c[1].empty() && c[1].size() <= 40 && parseReal(c[2], px) && px > 0) { Level v; v.name = c[1]; v.kind = "key"; v.px = px; out.push_back(v); }
    }
    return true;
}
// LRA-Levels-<MKT>.csv (level_touch): LEVEL|code|name|price|kind|15m|30m|45m|60m|90m|CL|?|"63%/75%?"|...; STATE|STALE = not used
inline bool parseMqLevels(const std::vector<std::string>& L, std::vector<Level>& out, bool& stale, std::string& sym) {
    out.clear(); stale = false; sym.clear();
    for (size_t i = 0; i < L.size(); ++i) {
        if (L[i].empty() || L[i][0] == '#') continue;
        const std::vector<std::string> c = split(L[i], '|'); double px;
        if (c.size() >= 2 && c[0] == "STATE" && c[1] == "STALE") stale = true;
        if (c.size() >= 2 && c[0] == "SYMBOL") sym = c[1];
        if (c.size() >= 4 && c[0] == "LEVEL" && !c[2].empty() && c[2].size() <= 40 && parseReal(c[3], px) && px > 0) {
            Level v; v.name = c[2]; v.kind = c.size() >= 5 ? c[4] : "mq"; v.px = px;
            if (c.size() >= 9) { long long p; if (parseWhole(c[8], 0, 100, p)) v.odds = std::to_string(p); }
            v.q = !(c.size() >= 13 && c[12].find('?') == std::string::npos);
            out.push_back(v);
        }
    }
    return true;
}
// LRA-Touch-<MKT>.csv: TOUCH|code|price|P 1h|P by expiry|P by close|proven|text
inline void parseTouch(const std::vector<std::string>& L, std::vector<Touch>& out) {
    out.clear();
    for (size_t i = 0; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], '|'); double px; long long p;
        if (c.size() >= 7 && c[0] == "TOUCH" && parseReal(c[2], px) && px > 0 && parseWhole(c[3], 0, 100, p)) { Touch t; t.code = c[1]; t.px = px; t.p1 = std::to_string(p); t.proven = c[6] == "1"; out.push_back(t); }
    }
}
// short display name: "CR/CR0/GW0" -> "CR"
inline std::string shortName(const std::string& n) { const size_t s = n.find('/'); return s == std::string::npos ? n : n.substr(0, s); }
// the merged level list (key first, then MenthorQ; one name per price within half a tick); vwap bands are not levels here
inline std::vector<Level> mergeLevels(const std::vector<Level>& key, const std::vector<Level>& mq, const std::vector<Touch>& tp, double tick) {
    std::vector<Level> out;
    const double eps = tick > 0 ? tick * 0.5 : 1e-9;
    auto add = [&](const Level& v) {
        if (v.kind == "vwap" || v.name.compare(0, 4, "VWAP") == 0) return;
        if (v.kind == "pivot_1h" || v.kind == "pivot_1h_1x1" || v.name.compare(0, 2, "Hr") == 0) return;   // (1.1.2) hourly swings: logged, not targets
        for (size_t i = 0; i < out.size(); ++i) if (std::fabs(out[i].px - v.px) <= eps) { if (out[i].odds.empty() && !v.odds.empty()) { out[i].odds = v.odds; out[i].q = v.q; } return; }
        Level w = v; w.name = shortName(w.name); out.push_back(w);
    };
    for (const Level& v : key) add(v);
    for (const Level& v : mq) add(v);
    for (Level& v : out) for (const Touch& t : tp) if (std::fabs(t.px - v.px) <= eps) { v.odds = t.p1; v.q = !t.proven; break; }
    std::sort(out.begin(), out.end(), [](const Level& a, const Level& b) { return a.px < b.px; });
    return out;
}

// ---- Delta Profile 2.7.0 record --------------------------------------------------------------------------------------------
struct DeltaA { long long id = -1, decidedBy = 0; bool support = true; double lo = 0, hi = 0, px = 0, mult = 0, net = 0; };
struct DeltaAE { long long aId = -1, decT = 0; std::string label; };      // only Y with a shown label (AE? / AE)
struct DeltaEx { long long decT = 0; bool top = true; double extreme = 0; std::string mark, level; };   // printed, mark shown
struct DeltaSnap { std::vector<DeltaA> a; std::vector<DeltaAE> ae; std::vector<DeltaEx> ex; long long lines = 0, rejected = 0; };
inline void parseDelta(const std::vector<std::string>& L, const std::string& root, DeltaSnap& S) {
    S = DeltaSnap();
    for (size_t i = 0; i < L.size(); ++i) {
        const std::string& ln = L[i];
        if (ln.empty() || ln[0] == '#' || ln.compare(0, 8, "t_first|") == 0) continue;
        const std::vector<std::string> c = split(ln, '|');
        S.lines++;
        if (c.size() == 18 && c[0] == "AE") {
            long long id, dt;
            if (c[3] != root && !root.empty()) continue;
            if (!parseWhole(c[4], 0, 10000000, id) || !parseWhole(c[5], 0, 32503680000LL, dt)) { S.rejected++; continue; }
            if (false && c[1] == "Y" && (c[16] == "AE?" || c[16] == "AE")) { DeltaAE e; e.aId = id; e.decT = dt; e.label = c[16]; S.ae.push_back(e); }   // (1.2.1, Rassul 2026-10-14 "no AE") AE records are no longer read: no AE text, tag or vote
            continue;
        }
        if (c.size() == 26 && c[0] == "E") {
            if (c[3] != root && !root.empty()) continue;
            long long dt; double ext;
            if (!parseWhole(c[8], 0, 32503680000LL, dt) || !parseReal(c[10], ext)) { S.rejected++; continue; }
            if (c[1] == "P" && (c[22] == "Ex?" || c[22] == "Ex")) { DeltaEx e; e.decT = dt; e.top = c[5] == "top"; e.extreme = ext; e.mark = c[22]; e.level = c[12] == "none" ? std::string() : c[12]; S.ex.push_back(e); }
            continue;
        }
        if ((c.size() == 24 || c.size() == 22) && c[12] == "D") {
            if (c[13] != root && !root.empty()) continue;
            if (c[2] != "A") continue;
            DeltaA a; long long by, id;
            if (!parseWhole(c[10], 1, 32503680000LL, by) || !parseWhole(c[14], 0, 10000000, id) || !parseReal(c[5], a.lo) || !parseReal(c[6], a.hi) ||
                !parseReal(c[8], a.net) || !parseReal(c[9], a.mult) || !parseReal(c[17], a.px) || a.lo > a.hi || (c[3] != "support" && c[3] != "resistance")) { S.rejected++; continue; }
            a.support = c[3] == "support"; a.decidedBy = by; a.id = id;
            S.a.push_back(a);
            continue;
        }
    }
}

// ---- order-flow session records (OrderFlowFeed.h 1.0.0: BidAskStats 1.3.0, PullStats 1.0.0, StackStats 1.0.0) -----------------
// header-driven: the column names come from the file's own "bar_time|..." line, so the three indicators' different number columns
// all parse; a row must have exactly the header's field count. Side index 0 = BID side (left bar), 1 = ASK side (right bar).
enum { SRC_BAS = 0, SRC_PULL = 1, SRC_STACK = 2, NSRC = 3 };
static const char* const SRC_NAME[NSRC] = { "BidAskStats", "PullStats", "StackStats" };
struct OfRow { unsigned long long t = 0; std::string let[2]; int spike[2] = {0, 0}; int impl = NEUTRAL; std::string level, msg, nums; std::map<std::string, long long> n; };
struct OfSnap { std::map<unsigned long long, OfRow> rows; long long rejected = 0; bool header = false; };
inline long long num(const OfRow& r, const std::string& k) { std::map<std::string, long long>::const_iterator i = r.n.find(k); return i == r.n.end() ? 0 : i->second; }
inline void parseOf(const std::vector<std::string>& L, OfSnap& S) {
    S = OfSnap();
    std::vector<std::string> head;
    for (size_t i = 0; i < L.size(); ++i) {
        const std::string& ln = L[i];
        if (ln.empty() || ln[0] == '#') continue;
        if (ln.compare(0, 9, "bar_time|") == 0) { head = split(ln, '|'); S.header = true; continue; }
        if (head.empty()) { S.rejected++; continue; }
        const std::vector<std::string> c = split(ln, '|');
        if (c.size() != head.size()) { S.rejected++; continue; }
        OfRow r; bool ok = true;
        for (size_t k = 0; k < head.size() && ok; ++k) {
            const std::string& h = head[k]; const std::string& f = c[k]; long long v;
            if (h == "bar_time") { ok = parseWhole(f, 1, 9000000000000LL, v); r.t = (unsigned long long)v; }
            else if (h == "letters_left" || h == "letters_right") { const int s = h == "letters_left" ? 0 : 1; ok = f.size() <= 3; std::string o; for (char ch : f) { if (ch == 'R') ch = 'I'; if (ch != 'S' && ch != 'L' && ch != 'I') ok = false; o += ch; } r.let[s] = o; }   // (1.0.1) refill / iceberg = I; legacy R (old logs) = I
            else if (h == "spike_left" || h == "spike_right") { ok = f == "0" || f == "1"; r.spike[h == "spike_left" ? 0 : 1] = f == "1"; }
            else if (h == "implication") { if (f == "bull") r.impl = BULL; else if (f == "bear") r.impl = BEAR; else if (f == "neutral") r.impl = NEUTRAL; else ok = false; }
            else if (h == "level") { ok = f.size() <= 40; r.level = f; }
            else if (h == "message") { ok = f.size() <= 300; r.msg = f; }
            else if (h == "numbers") { ok = f.size() <= 300; r.nums = f; }
            else if (h == "ct_time" || h == "dash_left" || h == "dash_right") {}
            else if (!f.empty() && f.size() <= 15 && f.find_first_not_of("0123456789") == std::string::npos) r.n[h] = std::strtoll(f.c_str(), nullptr, 10);
        }
        if (!ok) { S.rejected++; continue; }
        S.rows[r.t] = r;
    }
}
// "AskPull 92 (4.4x)" / "BidRatio 4.1x" / "BidRefill 44 (5.2x)" -> the ratio after a key in the numbers line; -1 = none
inline double ratioAfter(const std::string& nums, const std::string& key) {
    size_t p = nums.find(key); if (p == std::string::npos) return -1;
    p += key.size();
    const size_t x = nums.find('x', p); if (x == std::string::npos || x - p > 24) return -1;
    size_t a = x; while (a > p && (std::isdigit((unsigned char)nums[a - 1]) || nums[a - 1] == '.')) --a;
    double v; return parseReal(nums.substr(a, x - a), v) ? v : -1;
}

// ---- Liquidity Profile 0.14.0 --------------------------------------------------------------------------------------------------
struct LiqZone { bool support = true; double lo = 0, hi = 0, bx = 0; long long size = 0; bool isNear = false; };
struct LiqConfirm { std::string code; double px = 0; bool support = true; double bx = 0; };
struct LiqSnap { std::vector<LiqZone> z; std::vector<LiqConfirm> lq; long long at = 0; int colRight = 0; std::string sym; };
// LRA-LiqZones-<MKT>.csv: LIQZ,1,<unix>,<tick>[,<colRight>] then Z,S|R,lo,hi,bx,size
inline bool parseLiqZones(const std::vector<std::string>& L, LiqSnap& S) {
    S.z.clear(); S.at = 0; S.colRight = 0;
    if (L.empty()) return false;
    const std::vector<std::string> h = split(L[0], ',');
    long long at; if (h.size() < 4 || h[0] != "LIQZ" || h[1] != "1" || !parseWhole(h[2], 1, 32503680000LL, at)) return false;
    S.at = at; long long cr; if (h.size() >= 5 && parseWhole(h[4], 0, 4000, cr)) S.colRight = (int)cr;
    for (size_t i = 1; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], ','); LiqZone z; long long sz;
        if (c.size() >= 6 && c[0] == "Z" && (c[1] == "S" || c[1] == "R") && parseReal(c[2], z.lo) && parseReal(c[3], z.hi) && parseReal(c[4], z.bx) && parseWhole(c[5], 0, 1000000000LL, sz) && z.lo <= z.hi && z.bx > 0) {
            z.support = c[1] == "S"; z.size = sz; S.z.push_back(z);
        }
    }
    return true;
}
// LiquidityProfile.status-<MKT>-<spb>.txt: SYMBOL, CONFIRM,<code>,<px>,support|resistance,bx <x>,shown|logged and
// ZONE,bx ..,rank ..,behind ..,shown|hidden,support|resistance,lo,hi,size,...,near|far
inline void parseLiqStatus(const std::vector<std::string>& L, LiqSnap& S) {
    S.lq.clear(); S.sym.clear();
    std::vector<std::pair<double, double> > nearZ;
    for (size_t i = 0; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], ',');
        if (c.size() >= 2 && c[0] == "SYMBOL") S.sym = c[1];
        double px, bx, lo, hi;
        if (c.size() >= 6 && c[0] == "CONFIRM" && parseReal(c[2], px) && c[4].compare(0, 3, "bx ") == 0 && parseReal(c[4].substr(3), bx) && c[5] == "shown") {
            LiqConfirm q; q.code = shortName(c[1]); q.px = px; q.support = c[3] == "support"; q.bx = bx; S.lq.push_back(q);
        }
        if (c.size() >= 8 && c[0] == "ZONE" && c.back() == "near" && parseReal(c[6], lo) && parseReal(c[7], hi)) nearZ.push_back(std::make_pair(lo, hi));
    }
    for (LiqZone& z : S.z) for (size_t k = 0; k < nearZ.size(); ++k) if (std::fabs(nearZ[k].first - z.lo) < 1e-6 && std::fabs(nearZ[k].second - z.hi) < 1e-6) z.isNear = true;
}

// ---- HOD / LOD (lsHodLod 2.0.x status) -----------------------------------------------------------------------------------------
struct HodLodSnap { bool ok = false; std::string first; double pIn = -1; int usedPct = -1; bool projOk = false, projUp = false; double projPx = 0; bool news = false;
                   double erPts = -1, rangePts = -1;      // (1.0.1) erPts = the live expected RTH range (HLE_VALUES range=), rangePts = printed
                   bool boxOk = false; int box[4] = { 0, 0, 0, 0 }; };   // (1.1.1) TABLE_BOX,l,t,r,b: where lsHodLod drew its table (price pane px)
inline void parseHodLod(const std::vector<std::string>& L, HodLodSnap& H) {
    H = HodLodSnap();
    for (size_t i = 0; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], ',');
        if (c.empty()) continue;
        if (c[0] == "FIRST" && c.size() >= 2) { H.first = c[1] == "LOD" || c[1] == "HOD" ? c[1] : std::string(); H.ok = true; }
        else if (c[0] == "READ_VALUES" && c.size() >= 2) { double p; if (parseReal(c[1], p) && p >= 0 && p <= 1) H.pIn = p; }
        else if (c[0] == "HLE_VALUES") {   // "sources=63 stage=270 range=2.26059 lo=..." (space separated inside the field)
            for (size_t k = 1; k < c.size(); ++k) { std::istringstream ss(c[k]); std::string w;
                while (ss >> w) if (w.compare(0, 6, "range=") == 0) { double r; if (parseReal(w.substr(6), r) && r > 0) H.erPts = r; } } }
        else if (c[0] == "RANGE" && c.size() >= 2) { { double r; if (parseReal(c[1], r) && r >= 0) H.rangePts = r; } const std::string& u = c.back(); long long p; if (u.size() >= 2 && u.back() == '%' && parseWhole(u.substr(0, u.size() - 1), 0, 1000, p)) H.usedPct = (int)p; }
        else if (c[0] == "PROJ" && c.size() >= 3 && (c[1] == "HOD" || c[1] == "LOD")) { double p; if (parseReal(c[2], p) && p > 0) { H.projOk = true; H.projUp = c[1] == "HOD"; H.projPx = p; } }
        else if (c[0] == "TABLE_BOX" && c.size() == 5) {             // (1.1.1) the Tape Reader never covers the HOD / LOD table
            long long v[4]; bool ok = true; for (int k = 0; k < 4 && ok; ++k) ok = parseWhole(c[1 + k], -100000, 100000, v[k]);
            if (ok && v[2] > v[0] && v[3] > v[1]) { H.boxOk = true; for (int k = 0; k < 4; ++k) H.box[k] = (int)v[k]; }
        }
        else if (c[0] == "EVENTS") { for (size_t k = 1; k < c.size(); ++k) if (c[k] == "news=1" || c[k] == "fomc=1") H.news = true; }
    }
}

// ---- NEW nightly files -------------------------------------------------------------------------------------------------------
// LRA-TapeReader-<MKT>.csv: per-market voter weights + windows (format in NOTES.md). Missing / garbled = defaults (equal weights).
struct Params {
    double w[NVOTE] = { 1, 1, 1, 1 };
    int flowBars = 4, deltaBars = 10, levelBars = 10, minAgree = 2;
    std::string src = "default", why;
};
inline Params parseParams(bool exists, const std::vector<std::string>& L, const std::string& market) {
    Params P; if (!exists) { P.src = "missing"; return P; }
    Params Q; bool sawVersion = false, sawMarket = false; size_t n = 0;
    auto bad = [&](const std::string& why) { Params D; D.src = "garbled"; D.why = why; return D; };
    for (size_t i = 0; i < L.size(); ++i) {
        std::string ln = L[i]; const size_t hash = ln.find('#'); if (hash != std::string::npos) ln = ln.substr(0, hash);
        ln = trim(ln); if (ln.empty()) continue;
        if (++n > 64) return bad("too many lines");
        const std::vector<std::string> c = split(ln, ',');
        for (size_t k = 0; k < c.size(); ++k) if (trim(c[k]) != c[k]) return bad("spaces in a field");
        if (!sawVersion) { if (c.size() != 2 || c[0] != "VERSION" || c[1] != "1") return bad("VERSION,1 must be first"); sawVersion = true; continue; }
        if (c[0] == "MARKET") { if (c.size() != 2 || c[1] != market) return bad("wrong MARKET"); sawMarket = true; continue; }
        if (c[0] == "WEIGHT") {
            double v; if (c.size() != 3 || !parseReal(c[2], v) || v < 0 || v > 10) return bad("WEIGHT " + (c.size() > 1 ? c[1] : std::string()) + " invalid");
            int k = -1; for (int j = 0; j < NVOTE; ++j) if (c[1] == VOTE_NAME[j]) k = j;
            if (k < 0) continue;                                         // unknown voter: forward compatible
            Q.w[k] = v; continue;
        }
        if (c[0] == "PARAM") {
            long long v; if (c.size() != 3 || !parseWhole(c[2], 1, 200, v)) return bad("PARAM " + (c.size() > 1 ? c[1] : std::string()) + " invalid");
            if (c[1] == "flowBars") { if (v > 20) return bad("PARAM flowBars invalid"); Q.flowBars = (int)v; }
            else if (c[1] == "deltaBars") Q.deltaBars = (int)v;
            else if (c[1] == "levelBars") { if (v < 2 || v > 100) return bad("PARAM levelBars invalid"); Q.levelBars = (int)v; }
            else if (c[1] == "minAgree") { if (v > 4) return bad("PARAM minAgree invalid"); Q.minAgree = (int)v; }
            continue;
        }
    }
    if (!sawVersion) return bad("empty");
    if (!sawMarket) return bad("no MARKET");
    Q.src = "file"; return Q;
}
// LRA-Turns-<MKT>.csv, file VERSION 2 (lra.turns, data/turns/README.md; 1.0.1 replaces the old-draft VERSION,1 TURN parser):
//   VERSION,2   MARKET,<mkt>   ASOF,<YYYY-MM-DD HH:MM>   SWING,<def>,<swing frac>,<turn frac>   UNIT,xER   ER_TICKS,<ticks>,<date>
//   X,<xER>,<ticks at ER_TICKS>   COLS,<names...>   ROW,<values in COLS order>  ('#' = comment, empty field = no value)
// ROW buckets: kind base / level / level_tod / level_sig; level LonHI .. PFL, MQ:<code>, none, all; context all / open60 / to11 /
// after11 / sweep rb abs mq dpA ex ...; side long (a low that may turn up) / short. Fields are read by their COLS name, so added
// columns never break it. q25..q90 are swing quantiles in xER (share of the day's expected RTH range); *_t = ticks at ER_TICKS.
struct TurnRow {
    std::string kind, level, ctx, dir, status;                    // dir: long -> "up", short -> "down"
    long long n = 0; double pTurn = -1, pSwing = -1, pSwingX = -1, q50 = -1, q50t = -1;
};
struct TurnsSnap { bool ok = false; double xXer = -1, xTicks = -1, erTicks = -1; std::string asof, swingDef; std::vector<TurnRow> rows; std::string why; };
static const size_t TURNS_MAX_ROWS = 5000, TURNS_MAX_LINE = 2000;
inline TurnsSnap parseTurns(bool exists, const std::vector<std::string>& L, const std::string& market) {
    TurnsSnap T; if (!exists) { T.why = "missing"; return T; }
    auto bad = [&](const std::string& why) { TurnsSnap B; B.why = "garbled: " + why; return B; };
    bool v = false, m = false, unit = false, x = false; std::vector<std::string> cols; int iKind = -1, iLvl = -1, iCtx = -1, iSide = -1, iN = -1, iPt = -1, iPs = -1, iPx = -1, iQ = -1, iQt = -1, iSt = -1;
    // an empty field = no value (-1); anything else must be a finite number in [lo, hi]
    auto num = [](const std::string& t, double lo, double hi, double& out) { out = -1; if (t.empty()) return true; double d; if (!parseReal(t, d) || d < lo || d > hi) return false; out = d; return true; };
    for (size_t i = 0; i < L.size(); ++i) {
        if (L[i].size() > TURNS_MAX_LINE) return bad("line " + std::to_string(i + 1) + " too long");
        const std::string ln = trim(L[i]); if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = split(ln, ',');
        if (!v) { if (c.size() != 2 || c[0] != "VERSION" || c[1] != "2") return bad("VERSION,2 must be first"); v = true; continue; }
        if (c[0] == "MARKET") { if (c.size() != 2 || c[1] != market) return bad("wrong MARKET"); m = true; continue; }
        if (c[0] == "ASOF") { if (c.size() != 2 || c[1].size() > 32) return bad("ASOF"); T.asof = c[1]; continue; }
        if (c[0] == "SWING") { if (c.size() >= 2) T.swingDef = c[1]; continue; }
        if (c[0] == "UNIT") { if (c.size() != 2 || c[1] != "xER") return bad("UNIT must be xER"); unit = true; continue; }
        if (c[0] == "ER_TICKS") { double d; if (c.size() < 2 || !num(c[1], 0.001, 1e7, d)) return bad("ER_TICKS"); T.erTicks = d; continue; }
        if (c[0] == "X") { double a, b; if (c.size() < 3 || c[1].empty() || !num(c[1], 0.001, 5, a) || !num(c[2], 0, 1e7, b)) return bad("X"); T.xXer = a; T.xTicks = b; x = true; continue; }
        if (c[0] == "COLS") {
            cols.assign(c.begin() + 1, c.end());
            for (size_t k = 0; k < cols.size(); ++k) { const std::string& h = cols[k]; const int j = (int)k + 1;
                if (h == "kind") iKind = j; else if (h == "level") iLvl = j; else if (h == "context") iCtx = j; else if (h == "side") iSide = j; else if (h == "n") iN = j;
                else if (h == "p_turn") iPt = j; else if (h == "p_swing") iPs = j; else if (h == "p_swing_x") iPx = j; else if (h == "q50") iQ = j; else if (h == "q50_t") iQt = j; else if (h == "status") iSt = j; }
            if (iKind < 0 || iLvl < 0 || iCtx < 0 || iSide < 0 || iN < 0 || iPt < 0 || iPx < 0 || iSt < 0) return bad("COLS lacks kind / level / context / side / n / p_turn / p_swing_x / status");
            continue;
        }
        if (c[0] == "ROW") {
            if (cols.empty()) return bad("ROW before COLS");
            if (c.size() != cols.size() + 1) return bad("ROW line " + std::to_string(i + 1) + " has " + std::to_string(c.size() - 1) + " fields, COLS " + std::to_string(cols.size()));
            if (T.rows.size() >= TURNS_MAX_ROWS) return bad("too many rows");
            TurnRow r; r.kind = c[(size_t)iKind]; r.level = c[(size_t)iLvl]; r.ctx = c[(size_t)iCtx]; r.status = c[(size_t)iSt];
            const std::string& sd = c[(size_t)iSide];
            if (sd != "long" && sd != "short") return bad("ROW line " + std::to_string(i + 1) + " side");
            r.dir = sd == "long" ? "up" : "down";
            if (r.status != "learning" && r.status != "?" && r.status != "proven") return bad("ROW line " + std::to_string(i + 1) + " status");
            if (r.kind.empty() || r.level.empty() || r.ctx.empty() || r.kind.size() > 40 || r.level.size() > 40 || r.ctx.size() > 40) return bad("ROW line " + std::to_string(i + 1) + " bucket");
            double nn;
            if (!num(c[(size_t)iN], 0, 1e9, nn) || !num(c[(size_t)iPt], 0, 1, r.pTurn) || !num(c[(size_t)iPx], 0, 1, r.pSwingX) ||
                (iPs >= 0 && !num(c[(size_t)iPs], 0, 1, r.pSwing)) || (iQ >= 0 && !num(c[(size_t)iQ], 0, 100, r.q50)) || (iQt >= 0 && !num(c[(size_t)iQt], 0, 1e7, r.q50t)))
                return bad("ROW line " + std::to_string(i + 1) + " value");
            r.n = nn < 0 ? 0 : (long long)nn;
            T.rows.push_back(r); continue;
        }
        // unknown lines: forward compatible
    }
    if (!v || !m) return bad("no VERSION / MARKET");
    if (!unit) return bad("no UNIT,xER");
    if (!x) return bad("no X");
    T.ok = true; T.why = "file, " + std::to_string(T.rows.size()) + " rows"; return T;
}
// the level-type bucket of a level name (the study's buckets)
inline std::string levelType(const std::string& name, const std::string& kind) {
    const std::string n = name;
    auto has = [&](const char* k) { return n.compare(0, std::strlen(k), k) == 0; };
    if (has("Asia")) return "asia";            // (1.1.2)
    if (has("CW")) return "current_week";      // (1.1.2) before "C..." gamma names (CR); "CW" is never a MenthorQ code here
    if (has("Lon")) return "london";
    if (has("PD")) return "prior_day";
    if (has("PW")) return "prior_week";
    if (has("PF")) return "prior_full";
    if (has("ON")) return "overnight";
    if (has("Hr")) return "pivot_1h";
    if (has("CR") || has("PS") || has("HVL") || has("GW") || has("G") || kind == "gamma") return "gamma";
    return "other";
}
// the Reader's level -> the ledger's level name: key levels by name (LonHI LonLO ONH ONL PDH PDL PFH PFL), gamma levels as MQ:<code>;
// a level the ledger does not bucket (PWH, HrLO ...) -> "" (the base row only)
inline std::string turnLevel(const std::string& name, const std::string& kind) {
    static const char* const KEY[] = { "LonHI", "LonLO", "ONH", "ONL", "PDH", "PDL", "PFH", "PFL" };
    for (const char* k : KEY) if (name == k) return name;
    if (levelType(name, kind) == "gamma" && !name.empty()) return "MQ:" + name;
    return std::string();
}
// the Reader's footprint context -> the ledger's level_sig context (README: A -> dpA, Ex -> ex; AE has no bucket yet)
inline std::string turnCtx(const std::string& ctx) { return ctx == "A" ? "dpA" : ctx == "Ex" ? "ex" : std::string(); }
// time-of-day bucket of a candle end (CT sec): open60 = the first 60 min of RTH, to11 = until 11:00, after11; "" outside RTH
inline std::string turnTod(long long t, int rthOpenMin, int rthCloseMin) {
    if (rthOpenMin < 0 || rthCloseMin <= rthOpenMin) return std::string();
    const long long sod = ((t % 86400) + 86400) % 86400;
    if (sod <= rthOpenMin * 60LL || sod > rthCloseMin * 60LL) return std::string();
    if (sod <= (rthOpenMin + 60) * 60LL) return "open60";
    return sod <= 11 * 3600LL ? "to11" : "after11";
}
// the most specific bucket that exists decides (its status may be learning): level_sig (level + footprint context) -> level_tod
// (level + time of day) -> level (all) -> base. nullptr = none
inline const TurnRow* findTurn(const TurnsSnap& T, const std::string& level, const std::string& ctx, const std::string& dir, const std::string& tod) {
    if (!T.ok) return nullptr;
    auto get = [&](const char* kind, const std::string& lv, const std::string& cx) -> const TurnRow* {
        if (lv.empty() || cx.empty()) return nullptr;
        for (const TurnRow& r : T.rows) if (r.kind == kind && r.level == lv && r.ctx == cx && r.dir == dir) return &r;
        return nullptr; };
    const TurnRow* r = get("level_sig", level, turnCtx(ctx));
    if (!r) r = get("level_tod", level, tod);
    if (!r) r = get("level", level, "all");
    if (!r) r = get("base", "all", "all");
    return r;
}
// display state of a row: 0 learning (no number), 1 '?', 2 proven. Needs n >= 20, a status past learning and both odds.
inline int oddsState(const TurnRow* r) { if (!r || r->status == "learning" || r->n < 20 || r->pTurn < 0 || r->pSwingX < 0) return 0; return r->status == "proven" ? 2 : 1; }
// "Swing X+ ticks": X (xER, the X line) x the live expected range in ticks; else X's ticks at ER_TICKS. -1 = unknown
inline long long swingXTicks(const TurnsSnap& T, double liveErTicks) {
    if (T.xXer > 0 && liveErTicks > 0) return (long long)std::floor(T.xXer * liveErTicks + 0.5);
    if (T.xTicks > 0) return (long long)std::floor(T.xTicks + 0.5);
    return -1;
}
// the row's median swing in ticks: q50 (xER) x the live expected range; else q50_t. -1 = unknown
inline long long swingMedTicks(const TurnRow& r, double liveErTicks) {
    if (r.q50 >= 0 && liveErTicks > 0) return (long long)std::floor(r.q50 * liveErTicks + 0.5);
    if (r.q50t >= 0) return (long long)std::floor(r.q50t + 0.5);
    return -1;
}
inline std::string pct(double p) { return std::to_string((int)std::floor(p * 100 + 0.5)) + "%"; }

// ---- the read -----------------------------------------------------------------------------------------------------------------
struct Inputs {
    std::string mkt, root; double tick = 0.25; int spb = 180;
    const std::vector<Bar>* B = nullptr; int k = -1;              // the candle: B[k] (closed at a candle close, the forming bar intrabar)
    const DeltaSnap* dp = nullptr;
    const OfSnap* of[NSRC] = { nullptr, nullptr, nullptr };
    const LiqSnap* liq = nullptr;
    std::vector<Level> levels;                                    // merged key + MenthorQ (with odds)
    std::vector<Level> mqRaw;                                     // MenthorQ levels as published (gamma names: PS / CR / HVL)
    std::vector<GNode> gnodes;                                    // Dealer NODE strikes (same contract only)
    int gammaState = GF_OK; long long gammaAgeS = -1;             // (1.0.1) GF_NONE -> GAMMA abstains "?", GF_STALE -> "stale <age>"
    const HodLodSnap* hl = nullptr;
    const TurnsSnap* turns = nullptr;
    int trend = 0; bool trendReady = false;                       // vtl::level (-2..+2)
    Params P;
    int rthOpen = -1, rthClose = -1;                              // minutes of the day (CT)
    bool roll = false;
    std::string inputs;                                           // "name:age_s;..." for the log
    const ofo::Snap* opt = nullptr;                               // (1.1.0) the options grid that had ARRIVED by the candle and is fresh (else none)
    int optGates[ofo::NGATE] = { 1, 1, 1, 1, 1, 1, 1 };                 // (1.1.0) the gates of LRA-OrderFlow-<MKT>.csv (on / ? / off)
    long long optAgeS = -1;                                       // (1.2.0) how old the options grid is at the candle's end (s); -1 unknown
};
struct Fact { int actor = 0; int impl = NEUTRAL; std::string text; int order = 0; long long t = 0; };   // actor 0 buyers, 1 sellers
struct Read {
    bool ok = false; unsigned long long raw = 0; long long t = 0; std::string ct;
    int bias = NEUTRAL, n = 0; int votes[NVOTE] = { 0, 0, 0, 0 };
    std::string biasText, turn, swing, next; int centreDir = 0;   // centreDir +1 up / -1 down / 0 neutral (its colour)
    std::string buyers, sellers, levels; int buyImpl = NEUTRAL, sellImpl = NEUTRAL;
    std::string why[NVOTE];                                       // one word per voter (status / log)
    bool intrabar = false; int intraUpdates = 0; std::string inputs;
    std::string turnRow;                                          // (1.0.1) the LRA-Turns bucket used + its median swing (status only)
    std::string opt[2];                                           // (1.1.0) the OPTIONS lines (blue); both empty = no fresh options data
};

struct LevelPlay { bool on = false; bool support = true; bool held = true; bool swept = false; Level lv; double ext = 0; };
inline LevelPlay levelInPlay(const Inputs& I, double band) {
    LevelPlay R; const std::vector<Bar>& B = *I.B; const int k = I.k;
    if (k < 1 || I.levels.empty()) return R;
    const int a = std::max(0, k - I.P.levelBars + 1);
    int iL = a, iH = a;
    for (int i = a; i <= k; ++i) { if (B[(size_t)i].l <= B[(size_t)iL].l) iL = i; if (B[(size_t)i].h >= B[(size_t)iH].h) iH = i; }
    const double c = B[(size_t)k].c;
    const bool sup = iL != iH ? iL > iH : c >= B[(size_t)k].o;
    const double ext = sup ? B[(size_t)iL].l : B[(size_t)iH].h;
    // the level the extreme tested: within the band of the extreme, or traded through by it (a sweep or a break) and still near the
    // close; the nearest to the extreme wins
    const Level* best = nullptr; double bd = 1e300;
    for (const Level& v : I.levels) {
        const bool tested = std::fabs(v.px - ext) <= band + 1e-9 || (sup ? (v.px > ext && v.px <= c + 4 * band) : (v.px < ext && v.px >= c - 4 * band));
        if (!tested) continue;
        const double d = std::fabs(v.px - ext); if (d < bd) { bd = d; best = &v; }
    }
    if (!best) return R;
    if (std::fabs(c - best->px) > 4 * band + 1e-9) return R;      // price has left it: no longer in play
    const double eps = I.tick * 1e-3;
    R.on = true; R.support = sup; R.lv = *best; R.ext = ext;
    R.held = sup ? c >= best->px - eps : c <= best->px + eps;
    R.swept = sup ? ext < best->px - eps : ext > best->px + eps;
    return R;
}
inline const Level* nextLevel(const Inputs& I, double from, int dir, const std::string& skip) {
    const double gap = 2 * I.tick - 1e-9; const Level* best = nullptr;
    for (const Level& v : I.levels) {
        if (v.name == skip) continue;
        const double d = (v.px - from) * dir;
        if (d >= gap && (!best || d < (best->px - from) * dir)) best = &v;
    }
    return best;
}

// the read of candle k (B[0..k] only; every timed input filtered to what was known by the candle's end)
inline Read compute(const Inputs& I) {
    Read R;
    if (!I.B || I.k < 0 || I.k >= (int)I.B->size() || !(I.tick > 0)) return R;
    const std::vector<Bar>& B = *I.B; const int k = I.k; const Bar& bk = B[(size_t)k];
    R.ok = true; R.raw = bk.raw; R.t = bk.t; R.ct = clock12(bk.t); R.inputs = I.inputs;
    const double tick = I.tick, c = bk.c, atr = atrOf(B, k);
    double bandT = std::floor(0.5 * atr / tick + 0.5); if (bandT < 3) bandT = 3; if (bandT > 40) bandT = 40;
    const double band = bandT * tick;
    const int dir3 = k >= 2 ? (c > B[(size_t)k - 2].c ? 1 : c < B[(size_t)k - 2].c ? -1 : 0) : 0;   // where price is going (2 candles)
    std::vector<Fact> facts; int ord = 0;
    // ---------------- FLOW: Delta A / AE / Ex + the three order-flow panes ----------------
    double flow = 0; std::string flowWhy;
    const DeltaA* A = nullptr; const DeltaAE* AE = nullptr; const DeltaEx* EX = nullptr;
    if (I.dp) {
        const long long t0 = B[(size_t)std::max(0, k - I.P.deltaBars)].t;
        for (const DeltaA& a : I.dp->a) if (a.decidedBy <= bk.t && a.decidedBy > t0 && std::fabs(c - (a.support ? a.lo : a.hi)) <= 4 * band + 1e-9 && (!A || a.decidedBy >= A->decidedBy)) A = &a;
        if (A) for (const DeltaAE& e : I.dp->ae) if (e.aId == A->id && e.decT <= bk.t) AE = &e;
        for (const DeltaEx& e : I.dp->ex) if (e.decT <= bk.t && e.decT > t0 && (!EX || e.decT >= EX->decT)) EX = &e;
    }
    if (A) {
        const int s = A->support ? BULL : BEAR;
        if (cgate::kShowAbsorptionReason) { flow += s; flowWhy += A->support ? "A+" : "A-"; }   // (1.1.4) A is not a FLOW vote (gate)
        const Level* al = nullptr; { double bd = band + 1e-9; for (const Level& v : I.levels) { const double d = std::fabs(v.px - (A->support ? A->lo : A->hi)); if (d <= bd) { bd = d; al = &v; } } }
        const std::string at = al ? al->name + " " + pxText(al->px, tick) : pxText(A->px, tick);
        Fact f; f.actor = A->support ? 0 : 1; f.impl = s; f.order = ord++; f.t = A->decidedBy;
        f.text = trt::fill("delta.a", { {"lvl", at}, {"px", ""}, {"x", num1(A->mult)}, {"ae", AE ? trt::fill("delta.ae.tag", { {"ae", AE->label} }) : std::string()} });
        if (cgate::kShowAbsorptionReason) facts.push_back(f);   // (1.1.4) no "Absorbing at ..." message while the gate is closed
        if (AE) { flow += 0.5 * s; flowWhy += "AE";
            Fact g; g.actor = A->support ? 1 : 0; g.impl = s; g.order = ord++; g.t = AE->decT; g.text = trt::fill("delta.ae.att", { {"lvl", at}, {"px", ""}, {"ae", AE->label} }); facts.push_back(g); }
    }
    if (EX) {
        const int s = EX->top ? BEAR : BULL; flow += s; flowWhy += EX->top ? "Ex-" : "Ex+";
        Fact f; f.actor = EX->top ? 0 : 1; f.impl = s; f.order = ord++; f.t = EX->decT;
        f.text = trt::fill("delta.ex", { {"px", pxText(EX->extreme, tick)}, {"flow", trt::text(EX->top ? "flow.buying" : "flow.selling")}, {"mark", cgate::markQ(EX->mark)} });   // (1.2.0) always "Ex?"
        facts.push_back(f);
    }
    const int a0 = std::max(0, k - I.P.flowBars + 1);
    for (int src = 0; src < NSRC; ++src) {
        if (!I.of[src]) continue;
        const OfRow* r = nullptr; long long rt = 0;
        for (int i = k; i >= a0 && !r; --i) { std::map<unsigned long long, OfRow>::const_iterator it = I.of[src]->rows.find(B[(size_t)i].raw); if (it != I.of[src]->rows.end() && it->second.impl != NEUTRAL) { r = &it->second; rt = B[(size_t)i].t; } }
        if (!r) continue;
        flow += r->impl; flowWhy += std::string(src == SRC_BAS ? "B" : src == SRC_PULL ? "P" : "K") + (r->impl > 0 ? "+" : "-");
        const std::string lvl = r->level;
        // (1.1.0) the panes' SPEC language, abbreviated: lots, the ratio, the pace word ("fast" / "slowly", '?' until PACE_SHOW,on)
        auto paceOf = [&](int sd) -> std::string {
            const long long note = num(*r, sd == 0 ? "pace_note_b" : "pace_note_a"), gate = r->n.count("pace_gate") ? num(*r, "pace_gate") : 0;
            if (gate == 0 || (note != 1 && note != 2)) return std::string();
            return trt::fill(note == 1 ? "pace.fast" : "pace.slow", { {"q", gate == 2 ? std::string() : std::string("?")} });
        };
        if (src == SRC_BAS) {   // who attacked: bid side hit = sellers (0), offers lifted = buyers (1); the message names the side
            int side = r->msg.compare(0, 7, "Sellers") == 0 ? 0 : r->msg.compare(0, 6, "Buyers") == 0 ? 1 : (r->impl == BULL ? (r->msg.find("held") != std::string::npos ? 0 : 1) : (r->msg.find("held") != std::string::npos ? 1 : 0));
            const bool held = r->msg.find("held") != std::string::npos;
            const std::string sfx = side == 0 ? "_b" : "_a";
            const long long lv = num(*r, "sweep_lv" + sfx), lots = num(*r, side == 0 ? "bid" : "ask");
            const bool S = r->let[side].find('S') != std::string::npos;
            const double x = ratioAfter(r->nums, side == 0 ? "BidRatio " : "AskRatio ");
            // where it happened: into the level the pane named; "off the low / high" when buyers / sellers won at the level in play
            const LevelPlay lpx = levelInPlay(I, band);
            std::string where = lvl.empty() ? std::string() : trt::fill("into", { {"lvl", lvl} });
            if (!held && lpx.on && lpx.support == (side == 1)) where = trt::text(side == 1 ? "off.low" : "off.high");
            trt::Args a = { {"lv", std::to_string(lv)}, {"n", std::to_string(lots)}, {"x", x >= 0 ? ratio(x) : std::string("high")}, {"pace", paceOf(side)},
                            {"into", lvl.empty() ? std::string() : trt::fill("into", { {"lvl", lvl} })}, {"where", where} };
            Fact f; f.actor = side == 0 ? 1 : 0; f.impl = r->impl; f.order = ord++; f.t = rt;
            f.text = S && lv > 0 ? trt::fill(held ? "bas.sweep.held" : "bas.sweep.gave", a)
                                 : trt::fill(side == 0 ? (held ? "bas.hit.held" : "bas.hit.gave") : (held ? "bas.lift.held" : "bas.lift.gave"), a);
            facts.push_back(f);
        } else if (src == SRC_PULL) {   // bids pulled = buyers leaving (bear); offers pulled = sellers leaving (bull)
            const int side = r->impl == BEAR ? 0 : 1; const std::string sfx = side == 0 ? "_b" : "_a";
            const double x = ratioAfter(r->nums, side == 0 ? "BidPull " : "AskPull ");
            const long long n = num(*r, "pull_n" + sfx), lots = num(*r, "pull_cts" + sfx);
            std::string t = trt::fill("pull", { {"n", std::to_string(n)}, {"book", side == 0 ? (n == 1 ? "bid" : "bids") : (n == 1 ? "offer" : "offers")}, {"lots", std::to_string(lots)},
                                                {"x", x >= 0 ? ratio(x) : std::string("high")}, {"pace", paceOf(side)}, {"at", lvl.empty() ? std::string() : trt::fill("at", { {"lvl", lvl} })} });
            t += trt::text(side == 0 ? "pull.bids.tail" : "pull.offers.tail");
            Fact f; f.actor = side == 0 ? 0 : 1; f.impl = r->impl; f.order = ord++; f.t = rt; f.text = t; facts.push_back(f);
        } else {                        // stacking: bids added = buyers (bull), offers added = sellers (bear)
            const int side = r->impl == BULL ? 0 : 1; const std::string sfx = side == 0 ? "_b" : "_a";
            const double x = ratioAfter(r->nums, side == 0 ? "BidStack " : "AskStack "), rx = ratioAfter(r->nums, side == 0 ? "BidRefill " : "AskRefill ");
            const bool Rl = r->let[side].find('I') != std::string::npos;
            const long long n = num(*r, "add_n" + sfx), lots = num(*r, "add_cts" + sfx), hit = num(*r, "hit_cts" + sfx), reb = num(*r, "rebuilt_cts" + sfx), ice = num(*r, "iceberg" + sfx);
            std::string t = lvl.empty() ? std::string() : trt::fill("stack.defend", { {"lvl", lvl} });
            const bool refillOnly = r->msg.find(" reloaded ") != std::string::npos || r->msg.find(" reloading ") != std::string::npos || (n == 0 && lots == 0);   // the refills alone were notable
            const std::string rxT = rx >= 0 ? ratio(rx) : std::to_string(num(*r, "refill_n" + sfx));
            if (refillOnly) t += trt::fill("stack.refillonly", { {"book", side == 0 ? "bids" : "offers"}, {"rx", rxT} }) + (ice > 0 ? trt::text("ice") : "");
            else {
                t += trt::fill(lvl.empty() ? "stack" : "stack.atlvl", { {"n", std::to_string(n)}, {"book", side == 0 ? (n == 1 ? "bid" : "bids") : (n == 1 ? "offer" : "offers")}, {"lots", std::to_string(lots)},
                                          {"x", x >= 0 ? ratio(x) : std::string("high")}, {"pace", paceOf(side)} });
                if (r->msg.find(" - rebuilt ") != std::string::npos && hit > 0) { long long p = (reb * 100 + hit / 2) / hit; if (p > 200) p = 200; t += trt::fill("stack.rebuilt", { {"p", std::to_string(p)} }); }
                if (Rl) t += trt::fill("stack.refill", { {"rx", rxT} }) + (ice > 0 ? trt::text("ice") : "");
            }
            if (r->msg.find(STACK_ONE_ORDER) != std::string::npos || r->msg.find("may be bait") != std::string::npos) t += trt::text("stack.bait");   // (6:40 PM) new words + old records
            Fact f; f.actor = side == 0 ? 0 : 1; f.impl = r->impl; f.order = ord++; f.t = rt; f.text = t;
            facts.push_back(f);
        }
    }
    std::stable_sort(facts.begin(), facts.end(), [](const Fact& x, const Fact& y) { return x.t < y.t; });   // in time order: "then ... now"
    R.votes[V_FLOW] = flow >= 1 ? BULL : flow <= -1 ? BEAR : NEUTRAL; R.why[V_FLOW] = flowWhy.empty() ? "none" : flowWhy;
    // ---------------- LEVEL: held / broken at the level of the window's latest extreme ----------------
    const LevelPlay LP = levelInPlay(I, band);
    if (LP.on) { R.votes[V_LEVEL] = LP.support == LP.held ? BULL : BEAR; R.why[V_LEVEL] = LP.lv.name + (LP.held ? (LP.support ? " held" : " rejected") : " broken"); }
    else R.why[V_LEVEL] = "no lvl in play";
    // ---------------- GAMMA: the strongest-sign node near price ----------------
    const GNode* G = nullptr; std::string gName;
    {
        double gmax = 0; for (const GNode& g : I.gnodes) gmax = std::max(gmax, std::fabs(g.g));
        double bd = 2 * band + 1e-9;
        if (I.gammaState != GF_OK) bd = -1;                          // (1.0.1) abstain: no node is read
        for (const GNode& g : I.gnodes) { if (bd < 0 || gmax <= 0 || std::fabs(g.g) < 0.25 * gmax) continue; const double d = std::fabs(g.px - c); if (d <= bd) { bd = d; G = &g; } }
        if (I.gammaState == GF_STALE) R.why[V_GAMMA] = "stale " + ageText(I.gammaAgeS);
        else if (I.gammaState == GF_NONE) R.why[V_GAMMA] = "? no gamma data";
        else if (G) {
            for (const Level& v : I.mqRaw) if (std::fabs(v.px - G->px) <= tick * 0.5 && levelType(v.name, v.kind) == "gamma") { gName = shortName(v.name); break; }
            if (gName.empty()) for (const Level& v : I.levels) if (std::fabs(v.px - G->px) <= tick * 0.5 && levelType(v.name, v.kind) == "gamma") { gName = v.name; break; }
            if (gName.empty()) gName = "G";
            if (G->g > 0) R.votes[V_GAMMA] = G->px <= c + tick * 0.5 ? BULL : BEAR;
            else R.votes[V_GAMMA] = dir3 > 0 ? BULL : dir3 < 0 ? BEAR : NEUTRAL;
            R.why[V_GAMMA] = std::string(G->g > 0 ? "+GEX " : "-GEX ") + gName + " " + pxText(G->px, tick);
        } else R.why[V_GAMMA] = "no gamma node near";
    }
    // ---------------- TREND ----------------
    R.votes[V_TREND] = !I.trendReady ? NEUTRAL : I.trend > 0 ? BULL : I.trend < 0 ? BEAR : NEUTRAL;
    R.why[V_TREND] = !I.trendReady ? "no trend yet" : I.trend >= 2 ? "strong up" : I.trend == 1 ? "up" : I.trend == -1 ? "down" : I.trend <= -2 ? "strong down" : "none";
    // ---------------- BIAS ----------------
    double S = 0; int bull = 0, bear = 0;
    for (int v = 0; v < NVOTE; ++v) { S += I.P.w[v] * R.votes[v]; if (R.votes[v] > 0) ++bull; if (R.votes[v] < 0) ++bear; }
    if (S > 1e-9 && bull > bear && bull >= I.P.minAgree) { R.bias = BULL; R.n = bull; }
    else if (S < -1e-9 && bear > bull && bear >= I.P.minAgree) { R.bias = BEAR; R.n = bear; }
    else { R.bias = NEUTRAL; R.n = std::max(bull, bear); }
    R.biasText = R.bias == BULL ? trt::fill("bias.bull", { {"n", std::to_string(R.n)} }) : R.bias == BEAR ? trt::fill("bias.bear", { {"n", std::to_string(R.n)} }) : trt::text("bias.mixed");
    // ---------------- line 1 centre: turn / swing / next ----------------
    std::vector<std::string> centre;
    if (LP.on && LP.held) {
        const std::string dir = LP.support ? "up" : "down";
        const std::string ctx = AE ? "AE" : A ? "A" : EX ? "Ex" : "none";
        const TurnRow* tr0 = I.turns ? findTurn(*I.turns, turnLevel(LP.lv.name, LP.lv.kind), ctx, dir, turnTod(bk.t, I.rthOpen, I.rthClose)) : nullptr;
        int st = oddsState(tr0);
        // the live expected range in ticks (lsHodLod's HLE, never below the range already printed, as the ledger's exp_range)
        const double erT = I.hl && I.hl->erPts > 0 ? std::max(I.hl->erPts, I.hl->rangePts) / tick : -1;
        const long long xT = I.turns ? swingXTicks(*I.turns, erT) : -1;
        if (xT <= 0) st = 0;
        if (tr0) R.turnRow = tr0->kind + "/" + tr0->level + "/" + tr0->ctx + "/" + tr0->dir + " n " + std::to_string(tr0->n) + " " + tr0->status +
                             ", median swing " + std::to_string(swingMedTicks(*tr0, erT)) + " ticks, ER " + (erT > 0 ? std::to_string((long long)std::floor(erT + 0.5)) + " live" : "file");
        R.centreDir = LP.support ? 1 : -1;
        if (st == 0) centre.push_back(trt::text("turn.learning"));
        else {
            const std::string q = st == 1 ? "?" : "";
            centre.push_back(trt::fill("turn", { {"dir", dir}, {"lvl", LP.lv.name}, {"p", pct(tr0->pTurn) + q} }));
            R.swing = trt::fill("swing", { {"x", std::to_string(xT)}, {"p", pct(tr0->pSwingX) + q} });   // P(swing >= X) at X
            centre.push_back(R.swing);
        }
        R.turn = centre[0];
    }
    const int nd = R.bias != NEUTRAL ? R.bias : (LP.on && LP.held ? (LP.support ? 1 : -1) : 0);
    const Level* nx = nd != 0 ? nextLevel(I, c, nd, LP.on ? LP.lv.name : std::string()) : nullptr;
    if (nx) { R.next = trt::fill("next", { {"px", pxText(nx->px, tick)}, {"lvl", nx->name} }); centre.push_back(R.next); if (R.centreDir == 0) R.centreDir = nd; }
    // (the three parts are joined by the layout; stored joined for the log)
    // ---------------- BUYERS / SELLERS ----------------
    for (int who = 0; who < 2; ++who) {
        std::string s; int imp = 0;
        for (const Fact& f : facts) if (f.actor == who) {
            std::string t = f.text;
            if (!s.empty() && t.size() > 1 && t[0] >= 'A' && t[0] <= 'Z' && t[1] >= 'a' && t[1] <= 'z') t[0] = (char)(t[0] - 'A' + 'a');   // "...; then swept"
            s += (s.empty() ? "" : "; ") + t; imp += f.impl;
        }
        const bool needHere = (R.bias == BULL && who == 0) || (R.bias == BEAR && who == 1);
        if (s.empty() && !needHere) { (who == 0 ? R.buyers : R.sellers) = trt::text("quiet"); (who == 0 ? R.buyImpl : R.sellImpl) = NEUTRAL; continue; }
        if (!s.empty()) s = trt::capFirst(s) + ".";
        if (needHere) s += (s.empty() ? "" : " ") + std::string(trt::text(R.bias == BULL ? "need.bull" : "need.bear")) + ".";
        (who == 0 ? R.buyers : R.sellers) = s;
        (who == 0 ? R.buyImpl : R.sellImpl) = imp > 0 ? BULL : imp < 0 ? BEAR : NEUTRAL;
    }
    // ---------------- (1.1.0) OPTIONS: the grid that had ARRIVED by the candle's end and is fresh (the host filters), blue ----------------
    if (I.opt) {
        // (1.2.0, audit list: the OPTIONS line is a vendor model, delayed about 6 to 8 min - context, not a signal) every part keeps its
        // "?" here even where the nightly switched a gate on (GC / 6E ODEX gets its ? back); the first line says the grid's age
        int og[ofo::NGATE]; for (int g = 0; g < ofo::NGATE; ++g) og[g] = I.optGates[g] == ofo::GATE_ON ? ofo::GATE_Q : I.optGates[g];
        R.opt[0] = ofo::trLine1(*I.opt, og); R.opt[1] = ofo::trLine2(*I.opt, og, I.mkt, dir3);
        if (!R.opt[0].empty() && I.optAgeS >= 0) R.opt[0] += trt::fill("opt.age", { {"m", std::to_string((I.optAgeS + 30) / 60)} });
    }
    // ---------------- LEVELS (1.1.0: computed and logged for the nightly, no longer drawn - levels are named inside BUYERS / SELLERS) ----------------
    std::vector<std::string> parts;
    {   // low trust: first / last 5 min of RTH, roll
        // the candle's span [end - spb, end) overlaps the first / last 5 min of RTH
        std::vector<std::string> lt; const long long e = ((bk.t % 86400) + 86400) % 86400, st = e - (I.spb > 0 ? I.spb : 60);
        if (I.rthOpen >= 0 && st < I.rthOpen * 60LL + 300 && e > I.rthOpen * 60LL) lt.push_back(trt::text("lowtrust.open"));
        if (I.rthClose >= 0 && e > I.rthClose * 60LL - 300 && st < I.rthClose * 60LL) lt.push_back(trt::text("lowtrust.close"));
        if (I.roll) lt.push_back(trt::text("lowtrust.roll"));
        if (!lt.empty()) { std::string w; for (size_t i = 0; i < lt.size(); ++i) w += (i ? ", " : "") + lt[i]; parts.push_back(trt::fill("lowtrust", { {"why", w} })); }
    }
    std::vector<std::string> pro;
    if (I.liq) {   // the nearest published zone on the bias side (both sides when mixed)
        for (int side = 0; side < 2; ++side) {
            const bool wantSup = side == 0;
            if ((R.bias == BULL && !wantSup) || (R.bias == BEAR && wantSup)) continue;
            const LiqZone* z = nullptr; double bd = 4 * band + 1e-9;
            for (const LiqZone& q : I.liq->z) { if (q.support != wantSup) continue; const double d = wantSup ? std::max(0.0, c - q.hi) : std::max(0.0, q.lo - c); if ((q.isNear || d <= 2 * band) && d <= bd) { bd = d; z = &q; } }
            if (!z) continue;
            std::string on; for (const LiqConfirm& q : I.liq->lq) if (q.support == wantSup && q.px >= z->lo - tick * 0.5 && q.px <= z->hi + tick * 0.5) { on = trt::fill("liq.on", { {"lvl", q.code} }); break; }
            pro.push_back(trt::fill("liq", { {"side", trt::text(wantSup ? "liq.bid" : "liq.offer")}, {"bx", num1(z->bx)}, {"on", on} }));
        }
    }
    std::string gexText;
    if (G && cgate::kShowHvlCushion) gexText = trt::fill(G->g > 0 ? (G->px <= c + tick * 0.5 ? "gex.pos.below" : "gex.pos.above") : "gex.neg", { {"lvl", gName}, {"px", pxText(G->px, tick)} });
    else if (G) gexText = trt::fill("gex.plain", { {"sign", G->g > 0 ? "+" : "-"}, {"lvl", gName}, {"px", pxText(G->px, tick)},
                                                    {"age", I.gammaAgeS >= 0 ? trt::fill("gex.age", { {"m", std::to_string((I.gammaAgeS + 30) / 60)} }) : std::string(trt::text("gex.model"))} });   // (1.1.4) no "dlrs buy dips / chase" cause; (1.2.0) "(model, 7m old) ?"
    std::vector<std::string> against;
    const int b = R.bias;
    if (G && (b == NEUTRAL || R.votes[V_GAMMA] != -b)) pro.push_back(gexText);
    std::string hlText; int hlSide = 0;
    if (I.hl && I.hl->ok && !I.hl->first.empty() && I.hl->pIn >= 0.30) { hlText = trt::fill("hl.in", { {"side", I.hl->first}, {"p", std::to_string((int)std::floor(I.hl->pIn * 100 + 0.5))} }); hlSide = I.hl->first == "LOD" ? BULL : BEAR; }
    if (!hlText.empty() && (b == NEUTRAL || hlSide == b)) pro.push_back(hlText);
    if (b != NEUTRAL) {
        if (R.votes[V_FLOW] == -b) against.push_back(trt::text(b == BULL ? "ag.flow.sells" : "ag.flow.buys"));
        if (R.votes[V_LEVEL] == -b && LP.on) against.push_back(trt::fill(LP.held ? "ag.lvl.held" : "ag.lvl.broken", { {"lvl", LP.lv.name} }));
        if (R.votes[V_GAMMA] == -b && G) against.push_back(gexText);
        if (R.votes[V_TREND] == -b) against.push_back(trt::text(I.trend >= 2 ? "ag.trend.sup" : I.trend > 0 ? "ag.trend.up" : I.trend <= -2 ? "ag.trend.sdn" : "ag.trend.dn"));
        if (!hlText.empty() && hlSide == -b) against.push_back(hlText);
    }
    if (!pro.empty()) { std::string s; for (size_t i = 0; i < pro.size(); ++i) s += (i ? ", " : "") + pro[i]; parts.push_back(s); }
    if (!against.empty()) { std::string s; for (size_t i = 0; i < against.size(); ++i) s += (i ? ", " : "") + against[i]; parts.push_back(trt::fill("against", { {"list", s} })); }
    if (b != NEUTRAL) {
        // Inval: beyond the sweep extreme / the A node edge (whichever is further), one tick past
        double inv = b == BULL ? 1e300 : -1e300;
        if (LP.on && LP.support == (b == BULL)) inv = LP.ext;
        if (A && A->support == (b == BULL)) inv = b == BULL ? std::min(inv, A->lo) : std::max(inv, A->hi);
        if (!finite(inv)) { const int a = std::max(0, k - I.P.levelBars + 1); inv = b == BULL ? 1e300 : -1e300;
            for (int i = a; i <= k; ++i) inv = b == BULL ? std::min(inv, B[(size_t)i].l) : std::max(inv, B[(size_t)i].h); }
        inv += b == BULL ? -tick : tick;
        parts.push_back(trt::fill(b == BULL ? "inval.below" : "inval.above", { {"px", pxText(inv, tick)} }));
        std::string tg;
        if (nx) tg = trt::fill("tgt", { {"px", pxText(nx->px, tick)}, {"lvl", nx->name}, {"odds", nx->odds.empty() || !cgate::kShowTurnOdds ? std::string() : trt::fill("tgt.odds", { {"p", nx->odds}, {"q", nx->q ? "?" : ""} })} });   // (1.2.0) no % (LRA_SHOW_TURN_ODDS)
        if (I.hl && I.hl->projOk && (I.hl->projUp ? 1 : -1) == b && (I.hl->projPx - c) * b >= 2 * tick && (!nx || (I.hl->projPx - nx->px) * b > tick * 0.5)) {
            const std::string pj = trt::fill("tgt.proj", { {"side", I.hl->projUp ? "HOD" : "LOD"}, {"px", pxText(I.hl->projPx, tick)} });
            tg = tg.empty() ? "Tgt " + pj : tg + ", " + pj;
        }
        if (!tg.empty()) parts.push_back(tg);
    } else {
        std::string w = trt::text("mixed.wait");
        const Level* up = nextLevel(I, c, 1, std::string()), *dn = nextLevel(I, c, -1, std::string());
        if (up && dn) w += ". " + trt::fill("watch", { {"up", pxText(up->px, tick) + " " + up->name}, {"dn", pxText(dn->px, tick) + " " + dn->name} });
        parts.push_back(w);
    }
    for (size_t i = 0; i < parts.size(); ++i) R.levels += (i ? ". " : "") + parts[i];
    if (!R.levels.empty()) R.levels += ".";
    // the centre parts, joined (the layout centres the whole string)
    std::string cs; for (size_t i = 0; i < centre.size(); ++i) cs += (i ? trt::text("centre.sep") : "") + centre[i];
    R.turn = cs;
    return R;
}

// ---- the per-candle log line (lsFlexLevels\TapeReader\<MKT>-<CONTRACT>-<spb>-<YYYYMMDD>.csv) ----------------------------------
// (1.1.0) + "options" before "version" (23 fields); a 1.0.x log line (22 fields) still loads with no options
static const char* const LOG_HEADER = "bar_raw|bar_t|ct|bias|n|v_flow|v_level|v_gamma|v_trend|weights|centre|centre_dir|buy_impl|sell_impl|buyers|sellers|levels|why|intrabar_updates|inputs|params|options|version";
static const int LOG_FIELDS = 23, LOG_FIELDS_V10 = 22;
inline std::string logLine(const Read& R, const Params& P) {
    char w[96]; std::snprintf(w, sizeof(w), "%.3g;%.3g;%.3g;%.3g", P.w[0], P.w[1], P.w[2], P.w[3]);
    std::string why; for (int v = 0; v < NVOTE; ++v) why += (v ? ";" : "") + std::string(VOTE_NAME[v]) + "=" + clean(R.why[v], 60);
    char pp[96]; std::snprintf(pp, sizeof(pp), "flowBars=%d;deltaBars=%d;levelBars=%d;minAgree=%d;src=%s", P.flowBars, P.deltaBars, P.levelBars, P.minAgree, P.src.c_str());
    return std::to_string(R.raw) + "|" + std::to_string(R.t) + "|" + clean(R.ct, 12) + "|" + (R.bias == BULL ? "bull" : R.bias == BEAR ? "bear" : "mixed") + "|" + std::to_string(R.n) + "|" +
        std::to_string(R.votes[0]) + "|" + std::to_string(R.votes[1]) + "|" + std::to_string(R.votes[2]) + "|" + std::to_string(R.votes[3]) + "|" + w + "|" +
        clean(R.turn, 300) + "|" + std::to_string(R.centreDir) + "|" + std::to_string(R.buyImpl) + "|" + std::to_string(R.sellImpl) + "|" +
        clean(R.buyers, 600) + "|" + clean(R.sellers, 600) + "|" + clean(R.levels, 600) + "|" + why + "|" + std::to_string(R.intraUpdates) + "|" + clean(R.inputs, 600) + "|" + pp + "|" +
        clean(R.opt[0], 300) + " // " + clean(R.opt[1], 300) + "|" + VERSION;
}
inline bool parseLogLine(const std::string& ln, Read& R) {
    const std::vector<std::string> c = split(ln, '|');
    if ((int)c.size() != LOG_FIELDS && (int)c.size() != LOG_FIELDS_V10) return false;
    long long raw, t, n, v[4], cd, bi, si, iu;
    if (!parseWhole(c[0], 1, 9000000000000LL, raw) || !parseWhole(c[1], 0, 32503680000LL, t) || !parseWhole(c[4], 0, 4, n)) return false;
    for (int i = 0; i < 4; ++i) if (!parseWhole(c[5 + i], -1, 1, v[i])) return false;
    if (!parseWhole(c[11], -1, 1, cd) || !parseWhole(c[12], -1, 1, bi) || !parseWhole(c[13], -1, 1, si) || !parseWhole(c[18], 0, 1000000, iu)) return false;
    if (c[3] != "bull" && c[3] != "bear" && c[3] != "mixed") return false;
    Read r; r.ok = true; r.raw = (unsigned long long)raw; r.t = t; r.ct = c[2]; r.bias = c[3] == "bull" ? BULL : c[3] == "bear" ? BEAR : NEUTRAL; r.n = (int)n;
    for (int i = 0; i < 4; ++i) r.votes[i] = (int)v[i];
    r.biasText = r.bias == BULL ? trt::fill("bias.bull", { {"n", std::to_string(r.n)} }) : r.bias == BEAR ? trt::fill("bias.bear", { {"n", std::to_string(r.n)} }) : trt::text("bias.mixed");
    r.turn = c[10]; r.centreDir = (int)cd; r.buyImpl = (int)bi; r.sellImpl = (int)si; r.buyers = c[14]; r.sellers = c[15]; r.levels = c[16]; r.intraUpdates = (int)iu; r.inputs = c[19];
    if ((int)c.size() == LOG_FIELDS) { const size_t k = c[21].find(" // "); if (k != std::string::npos) { r.opt[0] = c[21].substr(0, k); r.opt[1] = c[21].substr(k + 4); } }   // (1.1.0)
    R = r; return true;
}

// ---- layout (pure: the plugin executes the draw list; the render check draws the same list) ---------------------------------
enum { K_RECT = 1, K_TEXT = 3 };
struct Item { int k = 0, l = 0, t = 0, r = 0, b = 0; unsigned c1 = 0, c2 = 0; std::string s; int fs = 9; bool bold = false; };
struct Box { int l = 0, t = 0, r = 0, b = 0; };
inline bool overlap(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
typedef std::function<int(const std::string&, int, bool)> Measure;
struct Layout { std::vector<Item> items; Box box; bool ok = false; std::string note; int fs = 9, lines = 0, spot = -1; };   // spot: (1.1.1) the 9-spot place used
static const unsigned C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_QUIET = 0x00D8DDE3, C_INK = 0x00D8DDE3, C_HEAD = 0x00F3F4F6, C_BG = 0x000F1520, C_BORDER = 0x00C8CDD3;
static const unsigned C_BLUE = 0x0058A6FF;   // (1.1.0) OPTIONS (the same blue as the panes' options line)
static const int POS_DEFAULT = 0;     // (1.1.3, Rassul 07:13 variant B) Top left - the banner (1.1.0-1.1.2: Bottom centre box; 1.0.x: Bottom left)
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
static const char* const ELLIPSIS = "\x85";   // "…" in the Windows ANSI code page the chart draws with
inline unsigned implCol(int i) { return i > 0 ? C_GREEN : i < 0 ? C_RED : C_QUIET; }
inline Box placeIn(const Box& area, int W, int H, int pos, int m = 4) {
    if (pos < 0 || pos > 8) pos = POS_DEFAULT;
    const int col = pos % 3, row = pos / 3;
    int x = col == 0 ? area.l + m : col == 1 ? (area.l + area.r) / 2 - W / 2 : area.r - m - W;
    int y = row == 0 ? area.t + m : row == 1 ? (area.t + area.b) / 2 - H / 2 : area.b - m - H;
    if (x + W > area.r) x = area.r - W;
    if (x < area.l) x = area.l;
    if (y + H > area.b) y = area.b - H;
    if (y < area.t) y = area.t;
    Box b; b.l = x; b.t = y; b.r = x + W; b.b = y + H; return b;
}
// cut s to fit w px, ending in "…" at a word boundary when one fits (a single over-long word: characters)
inline std::string fitEllipsis(const std::string& s, int w, int fs, bool bold, const Measure& M) {
    if (M(s, fs, bold) <= w) return s;
    if (M(ELLIPSIS, fs, bold) > w) return std::string();
    std::vector<size_t> cuts; for (size_t i = 1; i < s.size(); ++i) if (s[i] == ' ' && s[i - 1] != ' ') cuts.push_back(i);
    size_t lo = 0, hi = cuts.size();
    while (lo < hi) { const size_t mid = lo + (hi - lo + 1) / 2; if (M(s.substr(0, cuts[mid - 1]) + ELLIPSIS, fs, bold) <= w) lo = mid; else hi = mid - 1; }
    if (lo > 0) { std::string t = s.substr(0, cuts[lo - 1]); while (!t.empty() && (t.back() == ',' || t.back() == ';' || t.back() == '-' || t.back() == ' ')) t.pop_back(); return t + ELLIPSIS; }
    size_t a = 0, b = s.size();
    while (a < b) { const size_t mid = a + (b - a + 1) / 2; if (M(s.substr(0, mid) + ELLIPSIS, fs, bold) <= w) a = mid; else b = mid - 1; }
    return s.substr(0, a) + ELLIPSIS;
}
// up to maxLines lines at word boundaries; the last cut with "..." only when they are not enough (1.1.3: BUYERS / SELLERS up to 3)
inline std::vector<std::string> wrapN(const std::string& s, int w, int fs, int maxLines, const Measure& M) {
    std::vector<std::string> out; std::string rest = s;
    while (!rest.empty() && (int)out.size() < maxLines) {
        if (M(rest, fs, false) <= w) { out.push_back(rest); return out; }
        if ((int)out.size() == maxLines - 1) { out.push_back(fitEllipsis(rest, w, fs, false, M)); return out; }
        size_t best = 0;
        for (size_t i = 1; i < rest.size(); ++i) if (rest[i] == ' ' && rest[i - 1] != ' ' && M(rest.substr(0, i), fs, false) <= w) best = i;
        if (best == 0) { out.push_back(fitEllipsis(rest, w, fs, false, M)); return out; }
        out.push_back(rest.substr(0, best));
        size_t r = best; while (r < rest.size() && rest[r] == ' ') ++r;
        rest = rest.substr(r);
    }
    return out;
}
inline std::vector<std::string> wrap2(const std::string& s, int w, int fs, const Measure& M) { return wrapN(s, w, fs, 2, M); }

// ================================================================================================================================
// (1.1.3) THE BANNER (Rassul 2026-10-14 07:12 "you already have messaging at the top"; 07:13 variant B "one banner"; 07:17 solid dark
// background): top left, below the chart's title row, 11 pt (10% under 1.1.2's 12 pt line 1):
//   line 1   Tape Reader v1.1.3  .  7:09 AM   BULLISH 2 of 4  .  Turn up at PFH 83%  .  Swing 52+ ticks 34%  .  Next: 7874.25 ONH
//   line 2   [ABSORPTION WEAKENING] buyers exhausted (0.4x), but sellers are pulling their offers (92%)   (the Liquidity warning, as it
//            colours it today: the name in its outlined tag, green / red / grey; the detail grey) - only while there is one
// Hover over the banner: the BUYERS / SELLERS / OPTIONS detail (10 pt, was 11) in a box right under it. Solid dark fill + 1 px solid
// border (Settings > Banner background, default on). The width never shakes: it grows at once and shrinks only after the shorter
// text held for 60 s (bp::StableW, kept by the host); the height is 1 or 2 lines (the warning there or not).
// ================================================================================================================================
static const int BAN_PT = 11, DET_PT = 10, BAN_PAD = 5, BAN_LH = 19, DET_LH = 17, DET_W = 470, TITLE_ROW = 22;
// (1.1.3) the hover dead band: the detail opens only with the mouse inside the banner and closes only HOVER_BAND px outside it / its detail
static const int HOVER_BAND = 6;
inline bool inBox(const Box& b, int x, int y, int grow = 0) { return x >= b.l - grow && x <= b.r + grow && y >= b.t - grow && y <= b.b + grow; }
static const unsigned C_TAG_GREY = 0x009CA3AF, C_DETAIL = 0x00BEC2C8;
struct Seg { std::string s; unsigned col = C_HEAD; bool bold = true; };
// line 1 as coloured parts: the header (version + time) white, the bias in its colour, the turn / swing / next in the turn's colour
// (1.2.0, 2026-10-14 18:25 CT audit display list) what line 1 SHOWS of the centre (the log keeps the full read): with the turn-odds gate
// closed (LRA_SHOW_TURN_ODDS 0) a "Turn up at PDL 62%" part reads "Turn odds learning" and the "Swing 40+ ticks 55%" part is not shown;
// "Next: 6,030 PDH" stays. Works on a live read and on one reloaded from the log (it reads the stored text).
inline std::string shownCentre(const std::string& turn) {
    if (cgate::kShowTurnOdds || turn.empty()) return turn;
    const std::string sep = trt::text("centre.sep"), learn = trt::text("turn.learning");
    std::vector<std::string> parts; std::string t = turn;
    for (;;) { const size_t k = t.find(sep); parts.push_back(t.substr(0, k)); if (k == std::string::npos) break; t = t.substr(k + sep.size()); }
    std::string o; bool learned = false;
    for (const std::string& p : parts) {
        std::string q = p;
        if (q.compare(0, 5, "Turn ") == 0) { if (learned) continue; q = learn; learned = true; }
        else if (q.compare(0, 6, "Swing ") == 0) continue;
        o += (o.empty() ? "" : sep) + q;
    }
    return o;
}
// (1.2.0) the bias word ("BULLISH 2 of 4" / "MIXED") is not shown while LRA_SHOW_TAPE_BIAS is 0 (computed and logged as before)
inline bool showBias() { return cgate::kShowTapeBias; }
inline std::vector<Seg> line1Segs(const Read& R, const std::string& header) {
    std::vector<Seg> v; Seg h; h.s = header; h.col = C_HEAD; v.push_back(h);
    if (showBias()) { Seg b; b.s = "   " + R.biasText; b.col = R.bias == BULL ? C_GREEN : R.bias == BEAR ? C_RED : C_QUIET; v.push_back(b); }
    const std::string shown = shownCentre(R.turn);
    if (!shown.empty()) {
        const std::string sep = trt::text("centre.sep"); std::string t = shown, o;
        for (;;) { const size_t k = t.find(sep); o += "  \xB7  " + t.substr(0, k); if (k == std::string::npos) break; t = t.substr(k + sep.size()); }
        if (!showBias() && o.compare(0, 5, "  \xB7  ") == 0) o = "   " + o.substr(5);   // no bias before it: no leading dot
        const unsigned dc = R.centreDir > 0 ? C_GREEN : R.centreDir < 0 ? C_RED : C_QUIET;
        const std::string learn = trt::text("turn.learning"); const size_t lp = o.find(learn);
        if (lp != std::string::npos && lp < 6) {   // (1.2.0) "Turn odds learning" neutral, the rest ("Next: ...") in the direction's colour
            Seg c; c.s = o.substr(0, lp + learn.size()); c.col = C_QUIET; v.push_back(c);
            if (lp + learn.size() < o.size()) { Seg d; d.s = o.substr(lp + learn.size()); d.col = dc; v.push_back(d); }
        } else { Seg c; c.s = o; c.col = shown == learn ? C_QUIET : dc; v.push_back(c); }
    }
    return v;
}
inline unsigned warnCol(int favour) { return favour > 0 ? C_GREEN : favour < 0 ? C_RED : C_TAG_GREY; }
// the width the banner's text needs (the host keeps a stable width from it)
inline int bannerNeed(const Read& R, const std::string& header, const bb::Warn& W, const Measure& M) {
    int w1 = 0; for (const Seg& g : line1Segs(R, header)) w1 += M(g.s, BAN_PT, g.bold);
    int w2 = 0; if (W.on) w2 = M(W.name, BAN_PT, true) + 8 + 10 + M(W.detail, BAN_PT, false);
    return 2 * BAN_PAD + 10 + std::max(w1, w2);                          // + a margin: IRT's font may run a few px wider than the measure
}
inline int bannerH(bool warn) { return BAN_PAD + BAN_LH * (warn ? 2 : 1) + BAN_PAD - 2; }
struct Banner { Layout L; Box box, detail; bool detailOn = false; };
// area = the pane up to the last bar, below the title row; W = the stable width (>= the need, the host's StableW); pl = Position + Offset
inline Banner layoutBanner(const Read& R, const std::string& header, const bb::Warn& W, const Box& area, const bp::Place& pl, int stableW,
                           bool bg, bool hover, const Measure& M, const std::vector<Box>& hard = std::vector<Box>(), bp::Hyst* hyst = nullptr) {
    Banner B; Layout& L = B.L;
    const int aw = area.r - area.l, ah = area.b - area.t;
    const int H = bannerH(W.on);
    if (aw < 160 || ah < H + 4) { L.note = trt::text(aw < 160 ? "note.narrow" : "note.short"); return B; }
    const int Wd = std::min(aw, std::max(stableW, bannerNeed(R, header, W, M)));
    const bp::Box A = bp::mk(area.l, area.t, area.r, area.b);
    bp::Box b = bp::placeBox(A, Wd, H, pl);
    // the HOD / LOD table (hard): just below it, else just above it - never over it (after the offset); the hysteresis keeps it still
    for (const Box& o : hard) { const bp::Box ob = bp::mk(o.l, o.t, o.r, o.b); if (bp::overlap(b, ob)) {
        bp::Box below = bp::clampIn(bp::mk(b.l, ob.b + 4, b.r, ob.b + 4 + H), A), above = bp::clampIn(bp::mk(b.l, ob.t - 4 - H, b.r, ob.t - 4), A);
        b = !bp::overlap(below, ob) ? below : !bp::overlap(above, ob) ? above : b; } }
    // (1.1.3 fix) the hysteresis holds the spot's ANCHOR point only (left / centre / right edge x, top / middle / bottom edge y, by the
    // Position), never the whole box: the box is rebuilt from the anchor with the CURRENT width / height, so a 1 -> 2 line change or a
    // wider text shows on the same draw (it waited 2 draws when the whole box went through Hyst), and grows away from the anchor
    if (hyst) {
        const int col = pl.pos % 3, row = pl.pos / 3;
        auto anchorOf = [&](const bp::Box& x) { const int ax = col == 0 ? x.l : col == 2 ? x.r : (x.l + x.r) / 2, ay = row == 0 ? x.t : row == 2 ? x.b : (x.t + x.b) / 2; return bp::mk(ax, ay, ax + 1, ay + 1); };
        auto fromAnchor = [&](const bp::Box& a) { const int l = col == 0 ? a.l : col == 2 ? a.l - Wd : a.l - Wd / 2, t = row == 0 ? a.t : row == 2 ? a.t - H : a.t - H / 2; return bp::clampIn(bp::mk(l, t, l + Wd, t + H), A); };
        bool curBlocked = false;
        if (hyst->have) { const bp::Box cb = fromAnchor(hyst->cur); for (const Box& o : hard) if (bp::overlap(cb, bp::mk(o.l, o.t, o.r, o.b))) curBlocked = true; }
        b = fromAnchor(hyst->step(anchorOf(b), curBlocked));
    }
    B.box.l = b.l; B.box.t = b.t; B.box.r = b.r; B.box.b = b.b; L.box = B.box; L.ok = true; L.fs = BAN_PT; L.lines = W.on ? 2 : 1; L.spot = pl.pos;
    if (bg) { Item r; r.k = K_RECT; r.l = b.l; r.t = b.t; r.r = b.r; r.b = b.b; r.c1 = C_BORDER; r.c2 = C_BG; L.items.push_back(r); }
    auto text = [&](int x, int t, int bt, const std::string& str, unsigned col, int f, bool bold) {
        if (str.empty()) return;
        Item it; it.k = K_TEXT; it.l = x; it.t = t; it.r = x + M(str, f, bold); it.b = bt; it.c1 = col; it.s = str; it.fs = f; it.bold = bold; L.items.push_back(it); };
    // line 1: the parts left to right, the last cut with "..." only if the pane is narrower than the text
    int x = b.l + BAN_PAD + 2; const int maxR = b.r - BAN_PAD; int y = b.t + BAN_PAD - 1;
    for (const Seg& g : line1Segs(R, header)) {
        if (x >= maxR) break;
        std::string t = g.s; if (x + M(t, BAN_PT, g.bold) > maxR) t = fitEllipsis(t, maxR - x, BAN_PT, g.bold, M);
        text(x, y, y + BAN_LH, t, g.col, BAN_PT, g.bold); x += M(t, BAN_PT, g.bold);
    }
    if (W.on) {                                                            // line 2: the Liquidity warning, coloured as today
        y += BAN_LH; x = b.l + BAN_PAD + 2;
        const int nw = M(W.name, BAN_PT, true);
        Item tag; tag.k = K_RECT; tag.l = x - 2; tag.t = y + 1; tag.r = x + nw + 4; tag.b = y + BAN_LH - 2; tag.c1 = warnCol(W.favour); tag.c2 = 0xFFFFFFFFu; L.items.push_back(tag);   // c2 = hollow: the outline only
        text(x + 1, y, y + BAN_LH, W.name, warnCol(W.favour), BAN_PT, true);
        const int dx0 = x + nw + 14; if (dx0 < maxR) text(dx0, y, y + BAN_LH, fitEllipsis(W.detail, maxR - dx0, BAN_PT, false, M), C_DETAIL, BAN_PT, false);
    }
    if (!hover) return B;
    // the hover: BUYERS / SELLERS (up to 3 lines each) / OPTIONS (blue, 2 lines) right under the banner, 10 pt
    const int labW = std::max(M(trt::text("label.sellers"), DET_PT, true), std::max(M(trt::text("label.buyers"), DET_PT, true), M(trt::text("label.options"), DET_PT, true))) + 12;
    const int DW = std::min(aw, std::max(DET_W, b.r - b.l)), tw = DW - 2 * BAN_PAD - labW;
    const std::string side[2] = { R.buyers.empty() ? std::string(trt::text("quiet")) : R.buyers, R.sellers.empty() ? std::string(trt::text("quiet")) : R.sellers };
    std::vector<std::string> wr[3];
    for (int i = 0; i < 2; ++i) wr[i] = wrapN(side[i], tw, DET_PT, 3, M);
    const bool optAny = !R.opt[0].empty() || !R.opt[1].empty();
    if (optAny) { for (int j = 0; j < 2; ++j) if (!R.opt[j].empty()) wr[2].push_back(fitEllipsis(R.opt[j], tw, DET_PT, false, M)); } else wr[2].push_back(trt::text("options.none"));
    const int nl = (int)(wr[0].size() + wr[1].size() + wr[2].size()), DH = BAN_PAD + nl * DET_LH + BAN_PAD - 2;
    bp::Box d = bp::clampIn(bp::mk(b.l, b.b + 2, b.l + DW, b.b + 2 + DH), A);
    if (d.t < b.b && b.t - 2 - DH >= A.t) d = bp::mk(b.l, b.t - 2 - DH, b.l + DW, b.t - 2);          // no room under it: above it
    B.detail.l = d.l; B.detail.t = d.t; B.detail.r = d.r; B.detail.b = d.b; B.detailOn = true;
    { Item r; r.k = K_RECT; r.l = d.l; r.t = d.t; r.r = d.r; r.b = d.b; r.c1 = C_BORDER; r.c2 = C_BG; L.items.push_back(r); }
    const char* lab[3] = { trt::text("label.buyers"), trt::text("label.sellers"), trt::text("label.options") };
    const unsigned lc[3] = { implCol(side[0] == trt::text("quiet") ? 0 : R.buyImpl), implCol(side[1] == trt::text("quiet") ? 0 : R.sellImpl), C_BLUE };
    int yy = d.t + BAN_PAD - 1;
    for (int i = 0; i < 3; ++i) {
        text(d.l + BAN_PAD, yy, yy + DET_LH, lab[i], lc[i], DET_PT, true);
        const unsigned ink = i < 2 ? C_INK : optAny ? C_BLUE : C_QUIET;
        for (size_t j = 0; j < wr[i].size(); ++j) { text(d.l + BAN_PAD + labW, yy, yy + DET_LH, wr[i][j], ink, DET_PT, false); yy += DET_LH; }
    }
    return B;
}

} // namespace tr
#endif
