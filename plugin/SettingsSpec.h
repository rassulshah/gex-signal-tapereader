// SettingsSpec.h  1.0.0  (2026-10-18, Rassul 15:13 CT: "make sure every indicator always has its settings verified and tested so
// there are not bugs anymore"; 15:14 CT: "make sure it even has the right settings and you are not adding useless settings also")
//
// ONE settings layer for every IRT indicator. SDK-free (unit-tested on Linux); the SDK side is SettingsHost.h.
// Each indicator declares ONE table of rows {key, label, type, default (+ per-market defaults), range, width, same-line,
// group master, hidden}. Everything else comes from the table - registration, the per-market keyed file, reading values back,
// group masters, the colour rule, the width budget - so there is no index arithmetic anywhere.
//
// SDK facts this layer is built on (irtsdk.h 15.1.6, C:\Program Files\LinnSoft\InvestorRT\sdk\c++\include):
//  - typedef unsigned long COLOR; MAKE_COLOR(r,g,b) = r<<16 | g<<8 | b  -> COLOR is 0x00RRGGBB everywhere in the SDK (drawing,
//    setColorParameter, getParameterColor, setParameterColor). There is NO Windows COLORREF (0x00BBGGRR) in the SDK API, so no
//    byte swap is needed - the ONLY rule is: a colour row is filled with setParameterColor and read with getParameterColor.
//    setIntegerValue / getIntegerValue on a colour row leave the swatch BLACK and read back 0 (KingTracker v0.15 found it; HodLod
//    2.4, LiquidityProfile 0.4x and BigBidAsk 1.1x still did it - BigBidAsk.settings-ES.txt held 0 for every colour on 10-18).
//  - IRT keeps the dialog's values BY POSITION (setParameterVersion resets them when rows change); outside the settings window the
//    chart copy reads every box unchecked / list -1 / colour 0 (BUILD-LESSONS 42 / 44). So the chart reads ONLY the per-market
//    file this layer writes, and the file is keyed (key=value), never positional.
//  - enableParameter(i, bool) exists and parmsUpdt(i) is called for each change in the window: group masters are wired with them.
//  - kParmAppendSameLine (1 << 0) puts a row on the previous row's line; the SDK gives no dialog width, so the width budget is
//    measured (BigBidAsk's window cut controls off at about 415 px of content on his PC, 10-18): BUDGET_PX = 400.
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace ss {

static const char* const VERSION = "1.0.0";
static const char* const MAGIC = "SPEC1";      // first token of every keyed settings file
static const int BUDGET_PX = 400;              // widest row the IRT settings window shows without cutting it off

enum Type { T_BOOL = 1, T_INT, T_FLOAT, T_COLOR, T_LIST, T_LABEL };
enum Market { M_ES, M_NQ, M_CL, M_GC, M_HG, M_NG, M_EU, M_N };
static const char* const MARKETS[M_N] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
static const double NO_MKT = -1e300;           // "no per-market default" marker inside Row::mkt

inline int marketIndex(const std::string& m) {
    for (int i = 0; i < M_N; ++i) if (m == MARKETS[i]) return i;
    if (m == "6E") return M_EU;
    return -1;
}

// One row of the settings window (or a hidden value kept in the file only).
struct Row {
    const char* key;          // file key, unique, [a-z0-9_]
    const char* label;        // the text in the window (controls only - no how-to text)
    Type type;
    double def;               // default for every market without its own
    double lo, hi;            // INT / FLOAT range; LIST: 0 .. choices-1 (filled by the table helpers); COLOR 0 .. 0xFFFFFF
    const char* choices;      // LIST: "a;b;c"
    short width;              // control width px (0 = the IRT default for the type)
    bool same;                // on the previous row's line (kParmAppendSameLine)
    const char* group;        // key of this row's group master, or nullptr
    bool master;              // this BOOL row switches its group's items
    bool hidden;              // kept in the file (and read by the chart), not shown in the window
    double mkt[M_N];          // per-market defaults (NO_MKT = use def)
};

struct Spec {
    const char* plugin;       // "HodLod" -> file <flex>\HodLod.settings-<MKT>.txt
    const char* version;      // the indicator's version (= its setVersion literal; the contract test checks it)
    unsigned paramVersion;    // setParameterVersion (bump whenever the visible rows change)
    const Row* rows;
    int n;
};

// ---- table helpers (keep a table one line per row) ---------------------------------------------------------------------------------
inline Row base(const char* k, const char* l, Type t, double d, double lo, double hi) {
    Row r; r.key = k; r.label = l; r.type = t; r.def = d; r.lo = lo; r.hi = hi; r.choices = nullptr; r.width = 0; r.same = false;
    r.group = nullptr; r.master = false; r.hidden = false; for (int i = 0; i < M_N; ++i) r.mkt[i] = NO_MKT; return r;
}
inline int countChoices(const char* c) { if (!c || !*c) return 0; int n = 1; for (; *c; ++c) if (*c == ';') ++n; return n; }
inline Row boolRow(const char* k, const char* l, bool d) { return base(k, l, T_BOOL, d ? 1 : 0, 0, 1); }
inline Row intRow(const char* k, const char* l, int d, int lo, int hi, short w = 50) { Row r = base(k, l, T_INT, d, lo, hi); r.width = w; return r; }
inline Row floatRow(const char* k, const char* l, double d, double lo, double hi, short w = 50) { Row r = base(k, l, T_FLOAT, d, lo, hi); r.width = w; return r; }
inline Row colorRow(const char* k, const char* l, unsigned long d, short w = 40) { Row r = base(k, l, T_COLOR, (double)(d & 0xFFFFFFul), 0, 0xFFFFFF); r.width = w; return r; }
inline Row listRow(const char* k, const char* l, int d, const char* ch, short w = 110) { Row r = base(k, l, T_LIST, d, 0, countChoices(ch) - 1); r.choices = ch; r.width = w; return r; }
inline Row labelRow(const char* k, const char* l, short w = 400) { Row r = base(k, l, T_LABEL, 0, 0, 0); r.width = w; return r; }
inline Row same(Row r) { r.same = true; return r; }
inline Row in(Row r, const char* masterKey) { r.group = masterKey; return r; }
inline Row masterOf(Row r) { r.master = true; return r; }
inline Row hide(Row r) { r.hidden = true; return r; }
inline Row perMarket(Row r, double es, double nq, double cl, double gc, double hg, double ng, double eu) {
    const double v[M_N] = { es, nq, cl, gc, hg, ng, eu }; for (int i = 0; i < M_N; ++i) r.mkt[i] = v[i]; return r;
}

inline int find(const Spec& S, const char* key) { for (int i = 0; i < S.n; ++i) if (std::strcmp(S.rows[i].key, key) == 0) return i; return -1; }
inline double defaultFor(const Row& r, int mkt) { return (mkt >= 0 && mkt < M_N && r.mkt[mkt] != NO_MKT) ? r.mkt[mkt] : r.def; }

// A value is valid for its row (finite, in range, integral where it must be).
inline bool valid(const Row& r, double x) {
    if (!std::isfinite(x)) return false;
    switch (r.type) {
        case T_BOOL:  return x == 0 || x == 1;
        case T_INT:   return x == std::floor(x) && x >= r.lo && x <= r.hi;
        case T_FLOAT: return x >= r.lo - 1e-9 && x <= r.hi + 1e-9;
        case T_COLOR: return x == std::floor(x) && x >= 0 && x <= 0xFFFFFF;
        case T_LIST:  return x == std::floor(x) && x >= 0 && x <= r.hi;
        case T_LABEL: return true;
    }
    return false;
}

// ---- the values of one chart (one per table row, in table order) --------------------------------------------------------------------
struct Values {
    std::vector<double> v;
    int market = -1;
    std::string why;                      // where the values came from ("file", "defaults: no file", "migrated from ...")
    bool b(int k) const { return k >= 0 && k < (int)v.size() && v[(size_t)k] != 0; }
    int i(int k) const { return (k >= 0 && k < (int)v.size()) ? (int)std::llround(v[(size_t)k]) : 0; }
    double f(int k) const { return (k >= 0 && k < (int)v.size()) ? v[(size_t)k] : 0.0; }
    unsigned long c(int k) const { return (unsigned long)i(k) & 0xFFFFFFul; }   // COLOR, 0x00RRGGBB, passed to the SDK unchanged
};

inline Values defaults(const Spec& S, int mkt) {
    Values V; V.market = mkt; V.why = "defaults"; V.v.resize((size_t)S.n);
    for (int k = 0; k < S.n; ++k) V.v[(size_t)k] = defaultFor(S.rows[k], mkt);
    return V;
}

// Group masters: an item of a group is ON only while its master is on. (The window also unchecks + disables the items.)
inline bool effective(const Spec& S, const Values& V, int k) {
    if (k < 0 || k >= S.n) return false;
    const Row& r = S.rows[k];
    if (r.type == T_BOOL && !V.b(k)) return false;
    if (r.group) { const int m = find(S, r.group); if (m >= 0 && !V.b(m)) return false; }
    return true;
}
inline std::vector<int> itemsOf(const Spec& S, int master) {
    std::vector<int> out; if (master < 0) return out;
    for (int k = 0; k < S.n; ++k) if (S.rows[k].group && std::strcmp(S.rows[k].group, S.rows[master].key) == 0) out.push_back(k);
    return out;
}
// The click rule (BigBidAsk 1.11.0, Rassul 14:59 "clicking selects all ... unselecting unselects all"): a master click sets every
// BOOL item of its group to the master's state; non-BOOL items (colours) keep their value but are disabled while it is off.
inline void masterClick(const Spec& S, Values& V, int master) {
    const bool on = V.b(master);
    for (int k : itemsOf(S, master)) if (S.rows[k].type == T_BOOL) V.v[(size_t)k] = on ? 1 : 0;
}

// ---- number text (locale-free, full-token) -----------------------------------------------------------------------------------------
inline std::string fmt(const Row& r, double x) {
    char b[64];
    if (r.type == T_COLOR) std::snprintf(b, sizeof b, "%06lX", (unsigned long)std::llround(x) & 0xFFFFFFul);
    else if (r.type == T_FLOAT) { std::snprintf(b, sizeof b, "%.4f", x); std::string s = b; while (s.size() > 1 && s.back() == '0') s.pop_back(); if (!s.empty() && s.back() == '.') s.pop_back(); return s; }
    else std::snprintf(b, sizeof b, "%lld", (long long)std::llround(x));
    return b;
}
inline bool parseNum(const Row& r, const std::string& s, double& out) {
    if (s.empty() || s.size() > 32) return false;
    char* e = nullptr;
    if (r.type == T_COLOR) {
        for (char ch : s) if (!std::isxdigit((unsigned char)ch)) return false;
        const unsigned long long x = std::strtoull(s.c_str(), &e, 16); if (!e || *e) return false; out = (double)x; return true;
    }
    const double x = std::strtod(s.c_str(), &e); if (!e || *e) return false; out = x; return true;
}

// ---- the keyed file: "SPEC1|<plugin>|<version>|<MKT>" then key=value lines ------------------------------------------------------------
inline std::string format(const Spec& S, const Values& V) {
    std::string o = std::string(MAGIC) + "|" + S.plugin + "|" + S.version + "|" + (V.market >= 0 && V.market < M_N ? MARKETS[V.market] : "?") + "\n";
    for (int k = 0; k < S.n; ++k) { const Row& r = S.rows[k]; if (r.type == T_LABEL) continue; o += std::string(r.key) + "=" + fmt(r, V.v[(size_t)k]) + "\n"; }
    return o;
}
inline bool isKeyed(const std::string& text) { return text.compare(0, std::strlen(MAGIC) + 1, std::string(MAGIC) + "|") == 0; }

// Parse a keyed file: unknown keys ignored, missing / invalid keys = the market's default (counted in why), never garbage.
// Returns false (and defaults) when the file is not a keyed file of this plugin or names another market.
inline bool parse(const Spec& S, int mkt, const std::string& text, Values& V) {
    V = defaults(S, mkt);
    if (text.size() > 64 * 1024 || !isKeyed(text)) { V.why = "defaults: not a SPEC1 file"; return false; }
    size_t p = 0; int line = 0, bad = 0, unknown = 0, got = 0; bool head = true;
    while (p <= text.size() && line < 1024) {
        size_t e = text.find('\n', p); if (e == std::string::npos) e = text.size();
        std::string ln = text.substr(p, e - p); p = e + 1; ++line;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (head) {
            head = false;
            std::vector<std::string> t; size_t q = 0; for (;;) { size_t z = ln.find('|', q); t.push_back(ln.substr(q, z == std::string::npos ? std::string::npos : z - q)); if (z == std::string::npos) break; q = z + 1; }
            if (t.size() < 4 || t[1] != S.plugin) { V = defaults(S, mkt); V.why = "defaults: file of another plugin"; return false; }
            if (mkt >= 0 && marketIndex(t[3]) != mkt) { V = defaults(S, mkt); V.why = "defaults: file of another market"; return false; }
            continue;
        }
        if (ln.empty()) { if (e >= text.size()) break; continue; }
        const size_t eq = ln.find('='); if (eq == std::string::npos) { ++bad; continue; }
        const int k = find(S, ln.substr(0, eq).c_str()); if (k < 0) { ++unknown; continue; }
        double x = 0; if (!parseNum(S.rows[k], ln.substr(eq + 1), x) || !valid(S.rows[k], x)) { ++bad; continue; }
        V.v[(size_t)k] = x; ++got;
        if (e >= text.size()) break;
    }
    V.why = "file: " + std::to_string(got) + " keys" + (bad ? ", " + std::to_string(bad) + " invalid -> default" : "") + (unknown ? ", " + std::to_string(unknown) + " unknown ignored" : "");
    return true;
}

// ---- legacy (positional) files: one value per row in a fixed order; anything invalid -> default --------------------------------------
// order = the keys in the old file's positional order ("" = a field the old file had that no longer exists: skipped).
inline void fromPositional(const Spec& S, const std::vector<const char*>& order, const std::vector<std::string>& fields, Values& V) {
    for (size_t j = 0; j < order.size() && j < fields.size(); ++j) {
        if (!order[j] || !*order[j]) continue;
        const int k = find(S, order[j]); if (k < 0) continue;
        const Row& r = S.rows[k]; double x = 0;
        std::string f = fields[j]; while (!f.empty() && (f.back() == ' ' || f.back() == '\r')) f.pop_back();
        if (r.type == T_COLOR) { char* e = nullptr; const long long d = std::strtoll(f.c_str(), &e, 10); if (!e || *e || f.empty()) continue; x = (double)d; }   // old files kept colours as decimal ints
        else { char* e = nullptr; x = std::strtod(f.c_str(), &e); if (!e || *e || f.empty()) continue; }
        if (valid(r, x)) V.v[(size_t)k] = x;
    }
}
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> t; size_t q = 0;
    for (;;) { size_t z = s.find(d, q); t.push_back(s.substr(q, z == std::string::npos ? std::string::npos : z - q)); if (z == std::string::npos) break; q = z + 1; }
    return t;
}

// ---- the window layout: rows, lines, width budget -------------------------------------------------------------------------------------
// Estimated pixels (IRT dialog font ~ MS Sans Serif 8 pt: ~6 px a character + the control). The contract test keeps every line under
// BUDGET_PX; the real-dialog check after a build confirms it (tools\settings-check).
inline int labelPx(const char* s) { return s ? (int)std::strlen(s) * 6 + 8 : 0; }
inline int controlPx(const Row& r) {
    switch (r.type) {
        case T_BOOL:  return 18 + labelPx(r.label);                      // box + its text
        case T_LABEL: return r.width > 0 ? std::min<int>(r.width, (int)std::strlen(r.label) * 6 + 8) : labelPx(r.label);
        case T_COLOR: return labelPx(r.label) + (r.width > 0 ? r.width : 40);
        case T_LIST:  return labelPx(r.label) + (r.width > 0 ? r.width : 110);
        default:      return labelPx(r.label) + (r.width > 0 ? r.width : 60);
    }
}
struct Line { std::vector<int> rows; int px = 0; };
inline std::vector<Line> lines(const Spec& S) {
    std::vector<Line> L;
    for (int k = 0; k < S.n; ++k) {
        const Row& r = S.rows[k]; if (r.hidden) continue;
        if (!r.same || L.empty()) L.push_back(Line());
        L.back().rows.push_back(k); L.back().px += controlPx(r) + (L.back().rows.size() > 1 ? 10 : 0);
    }
    return L;
}
inline int visibleLines(const Spec& S) { return (int)lines(S).size(); }

// ---- table checks (the contract test runs these for every indicator) ----------------------------------------------------------------
inline std::vector<std::string> check(const Spec& S) {
    std::vector<std::string> err;
    auto E = [&](const std::string& m) { err.push_back(std::string(S.plugin) + ": " + m); };
    if (!S.plugin || !*S.plugin) E("no plugin name");
    if (!S.version || !*S.version) E("no version");
    for (int k = 0; k < S.n; ++k) {
        const Row& r = S.rows[k]; const std::string key = r.key ? r.key : "";
        if (key.empty()) { E("row " + std::to_string(k) + " has no key"); continue; }
        for (char ch : key) if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')) { E("key '" + key + "' must be [a-z0-9_]"); break; }
        if (find(S, r.key) != k) E("duplicate key '" + key + "'");
        if (r.type != T_LABEL && !r.hidden && (!r.label || !*r.label)) E("'" + key + "' has no label");
        if (r.type == T_LIST && countChoices(r.choices) < 2) E("list '" + key + "' needs 2+ choices");
        if ((r.type == T_INT || r.type == T_FLOAT) && !(r.lo < r.hi)) E("'" + key + "' range lo < hi");
        for (int m = -1; m < M_N; ++m) { const double d = m < 0 ? r.def : defaultFor(r, m);
            if (r.type != T_LABEL && !valid(r, d)) E("'" + key + "' default " + fmt(r, d) + " invalid for " + (m < 0 ? std::string("all") : std::string(MARKETS[m]))); }
        if (r.group) { const int g = find(S, r.group); if (g < 0) E("'" + key + "' group '" + r.group + "' does not exist"); else if (!S.rows[g].master || S.rows[g].type != T_BOOL) E("'" + key + "' group '" + r.group + "' is not a BOOL master"); }
        if (r.master && r.type != T_BOOL) E("master '" + key + "' must be BOOL");
        if (r.master && itemsOf(S, k).empty()) E("master '" + key + "' has no items");
        if (r.same && k > 0 && S.rows[k - 1].hidden && !r.hidden) {
            int p = k - 1; while (p >= 0 && S.rows[p].hidden) --p; if (p < 0) E("'" + key + "' is same-line but nothing visible precedes it");
        }
        if (r.label) { const std::string l = r.label; if (l.find('?') != std::string::npos) E("label '" + l + "' contains '?'"); }
    }
    for (const Line& L : lines(S)) if (L.px > BUDGET_PX) {
        std::string names; for (int k : L.rows) names += std::string(names.empty() ? "" : ", ") + S.rows[k].key;
        E("line [" + names + "] is ~" + std::to_string(L.px) + " px > " + std::to_string(BUDGET_PX) + " px (cut off on the right)");
    }
    return err;
}

// ---- small file helpers (portable C; the host passes the full path) ---------------------------------------------------------------
inline bool readSmall(const std::string& path, std::string& out, size_t cap = 64 * 1024) {
    out.clear(); FILE* f = std::fopen(path.c_str(), "rb"); if (!f) return false;
    char buf[4096]; size_t n; bool ok = true;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) { out.append(buf, n); if (out.size() > cap) { ok = false; break; } }
    std::fclose(f); return ok;
}
// .tmp -> old to .bak -> rename -> restore .bak on failure (BUILD-LESSONS 5)
inline bool writeAtomic(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    FILE* f = std::fopen(tmp.c_str(), "wb"); if (!f) return false;
    const bool w = std::fwrite(text.data(), 1, text.size(), f) == text.size(); const bool fl = std::fflush(f) == 0; std::fclose(f);
    if (!w || !fl) { std::remove(tmp.c_str()); return false; }
    std::remove(bak.c_str());
    const bool had = std::rename(path.c_str(), bak.c_str()) == 0;
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { if (had) std::rename(bak.c_str(), path.c_str()); std::remove(tmp.c_str()); return false; }
    return true;
}

// ---- the per-chart store: the chart reads ONLY this (re-read at most once a second, compared by content - lesson 84) --------------
// legacy: optional reader for the indicator's previous file (positional); it fills V and returns true when it recognised the text.
typedef bool (*LegacyFn)(const Spec& S, int mkt, const std::string& text, Values& V);
struct Store {
    const Spec* S = nullptr;
    std::string path;            // <flex>\<plugin>.settings-<MKT>.txt
    std::vector<std::string> legacyPaths;
    LegacyFn legacy = nullptr;
    long long lastReadMs = -1000000;
    std::string lastText; bool haveText = false;
    Values cur;
    // load: keyed file -> values; else a legacy file (same path or an older name) -> values, written back keyed ONCE (log line)
    Values load(int mkt, long long nowMs, bool force = false, std::string* migrated = nullptr) {
        if (!S) return Values();
        if (!force && haveText && nowMs - lastReadMs < 1000 && cur.market == mkt) return cur;
        lastReadMs = nowMs;
        std::string t;
        if (!path.empty() && readSmall(path, t) && isKeyed(t)) {
            if (haveText && t == lastText && cur.market == mkt) return cur;
            Values V; parse(*S, mkt, t, V); lastText = t; haveText = true; cur = V; return cur;
        }
        // not keyed yet: the same path (positional) first, then the older names
        std::vector<std::string> tries; if (!t.empty()) tries.push_back(path); for (const auto& p : legacyPaths) tries.push_back(p);
        for (const auto& p : tries) {
            std::string lt; if (p == path) lt = t; else if (!readSmall(p, lt)) continue;
            Values V = defaults(*S, mkt);
            if (legacy && !lt.empty() && legacy(*S, mkt, lt, V)) {
                V.market = mkt; V.why = "migrated from " + p;
                if (!path.empty() && writeAtomic(path, format(*S, V)) && migrated) *migrated = p;
                cur = V; haveText = true; lastText = format(*S, V); return cur;
            }
        }
        cur = defaults(*S, mkt); cur.why = "defaults: no settings file"; haveText = true; lastText.clear(); return cur;
    }
    bool save(const Values& V) { if (!S || path.empty()) return false; const std::string t = format(*S, V); if (!writeAtomic(path, t)) return false; lastText = t; cur = V; haveText = true; return true; }
};

// The dialog snapshot rule (BoxPlace.h DlgSnap, lesson 44): a control he did not touch keeps the FILE's value (IRT may ignore a fill).
struct Snap {
    std::vector<double> v; bool ok = false;
    void take(const Values& V) { v = V.v; ok = true; }
    double pick(size_t k, double now, double file) const { if (!ok || k >= v.size()) return now; return now == v[k] ? file : now; }
};

} // namespace ss
