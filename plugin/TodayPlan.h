// TodayPlan.h 1.0.0 - lsHodLod 2.40.0 (2026-11-20 09:43 CT, Rassul "this looks good. build", mockup hodlod-box-plan-mockup.html):
// the HOD / LOD box's BUY AT · 3-HOUR SUPPORT (left) / SELL AT · 3-HOUR RESISTANCE (right) sections = the MQ Reader's TODAY PLAN for
// this chart's market, row for row. SDK-free (tests_hodlod/t_hl2400.cpp).
//
// THE SAME SOURCES THE READER USES (never the network):
//  - the 3-hour R1 / R2 / S1 / S2, standing flags, swing dir and extreme: lsFlexLevels\CobSignal.status-<MKT>-180.txt HTFNOW line
//    (written by lsCobSignal; the bridge's lra.hodlod_box 1.9.0 htf_status() reads the same line; stale = file older than 14 h).
//  - price (price_live, else price), expected move (em), the ladder (code, price, price_fmt, mq_only, mq_zone) and now_ct: the Reader's
//    own state file C:\Dev\level-reversal-analytics\data\menthorq\reader\<session>\state.json (the bridge writes it with every Reader
//    build and serves it as /api/reader "state"); only the top-level "session" / "now_ct" / "markets"."<MKT>" fields are read.
//  - the rules: menthorq-reader.user.js 3.9.0+ todayPlanHtf() line(): a level shows only when standing and NEAR (<= 0.25 EM from the
//    live price) or SPRING (buy: down swing, extreme below it) / UPTHRUST (sell: up swing, extreme above it); tags = every ladder row
//    within 0.06 EM (key code with LH / LL -> LonHI / LonLO, AH / AL -> AsiaHI / AsiaLO, '+' MenthorQ abbreviations; an MQ-only row =
//    its MenthorQ abbreviations), joined " · "; T1 = the nearest ladder price at least 0.25 EM beyond the level; S2 / R2 skipped when
//    equal to S1 / R1. Level text = the bridge's px_txt (commas, decimals per market, trailing zeros dropped); T1 = the ladder's price_fmt.
#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace tpl {

static const char* const VERSION = "1.0.0";
static const double NEAR_EM = 0.25, TAG_EM = 0.06;
static const long long HTF_STALE_S = 14LL * 3600;           // = lra.hodlod_box STALE_S
static const size_t MAX_STATUS = 64 * 1024, MAX_STATE = 16u * 1024u * 1024u;

struct Htf { bool have = false; double r1 = NAN, r2 = NAN, s1 = NAN, s2 = NAN; bool r1ok = false, r2ok = false, s1ok = false, s2ok = false; int dir = 0; double ext = NAN; };
struct Lad { std::string code, fmt; double px = NAN; bool mqOnly = false; std::vector<std::string> zone; };
struct St { bool ok = false; std::string why, session, nowCt; bool marketFound = false; double px = NAN, em = NAN; std::vector<Lad> lad; };
struct Row { std::string level, tags, chip, target; };      // "S1 7,105" | "PS0+PS" | NEAR / SPRING / UPTHRUST | "7,150.00" ("" = none)
struct Plan { bool ok = false; std::string why, time12; std::vector<Row> buy, sell; };

// ---------- number helpers (strict) ----------
inline bool fullD(const std::string& s, double& v) {
    if (s.empty()) return false;
    char* e = nullptr; const double d = std::strtod(s.c_str(), &e);
    if (!e || *e != 0 || !std::isfinite(d) || std::fabs(d) >= 1e300) return false;
    v = d; return true; }
inline int decOf(const std::string& m) {
    if (m == "GC") return 1;
    if (m == "HG") return 4;
    if (m == "NG") return 3;
    if (m == "EU" || m == "6E") return 5;
    return 2; }
// = lra.hodlod_box.px_txt: f"{v:,.{dec}f}" with trailing zeros (and a bare '.') dropped
inline std::string pxTxt(double v, const std::string& m) {
    if (!std::isfinite(v)) return "-";
    char b[64]; std::snprintf(b, sizeof b, "%.*f", decOf(m), v);
    std::string s(b), sign; if (!s.empty() && s[0] == '-') { sign = "-"; s.erase(0, 1); }
    const size_t dot = s.find('.'); std::string ip = dot == std::string::npos ? s : s.substr(0, dot), fp = dot == std::string::npos ? std::string() : s.substr(dot);
    std::string c; for (size_t i = 0; i < ip.size(); i++) { if (i && (ip.size() - i) % 3 == 0) c += ','; c += ip[i]; }
    std::string t = sign + c + fp;
    if (t.find('.') != std::string::npos) { while (!t.empty() && t.back() == '0') t.pop_back(); if (!t.empty() && t.back() == '.') t.pop_back(); }
    return t; }
inline std::string t12(const std::string& hm) {   // "09:43" -> "9:43 AM" (the Reader's t12)
    if (hm.size() != 5 || hm[2] != ':' || !isdigit((unsigned char)hm[0]) || !isdigit((unsigned char)hm[1]) || !isdigit((unsigned char)hm[3]) || !isdigit((unsigned char)hm[4])) return hm;
    int h = (hm[0] - '0') * 10 + (hm[1] - '0'); const int mm = (hm[3] - '0') * 10 + (hm[4] - '0'); const char* ap = h >= 12 ? "PM" : "AM"; h %= 12; if (h == 0) h = 12;
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", h, mm, ap); return b; }

// UTF-8 (state.json strings) -> the ANSI bytes IRT draws: U+00A0..U+00FF kept as one byte (the middle dot \xB7), anything else '?'
inline std::string toAnsi(const std::string& s) {
    std::string o; for (size_t i = 0; i < s.size(); ) { const unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { o += (char)c; ++i; }
        else if ((c == 0xC2 || c == 0xC3) && i + 1 < s.size()) { o += (char)(((c & 0x1F) << 6) | ((unsigned char)s[i + 1] & 0x3F)); i += 2; }
        else { o += '?'; ++i; while (i < s.size() && ((unsigned char)s[i] & 0xC0) == 0x80) ++i; } }
    return o; }
inline std::string toUtf8(const std::string& s) { std::string o; for (char ch : s) { const unsigned char c = (unsigned char)ch; if (c < 0x80) o += ch; else { o += (char)(0xC0 | (c >> 6)); o += (char)(0x80 | (c & 0x3F)); } } return o; }
// ---------- the HTFNOW line (= lra.hodlod_box.htf_status) ----------
inline std::string trim(const std::string& s) { size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n"); return a == std::string::npos ? std::string() : s.substr(a, b - a + 1); }
inline Htf parseHtf(const std::string& text) {
    Htf H; size_t p = 0;
    while (p < text.size()) {
        size_t e = text.find('\n', p); if (e == std::string::npos) e = text.size();
        const std::string ln = text.substr(p, e - p); p = e + 1;
        std::vector<std::string> f; size_t a = 0; for (;;) { size_t c = ln.find(',', a); f.push_back(trim(ln.substr(a, c == std::string::npos ? std::string::npos : c - a))); if (c == std::string::npos) break; a = c + 1; }
        if (f.size() >= 11 && f[0] == "HTFNOW") {
            Htf h; h.have = true; double d;
            h.r1 = fullD(f[1], d) ? d : NAN; h.r1ok = f[2] == "1"; h.r2 = fullD(f[3], d) ? d : NAN; h.r2ok = f[4] == "1";
            h.s1 = fullD(f[5], d) ? d : NAN; h.s1ok = f[6] == "1"; h.s2 = fullD(f[7], d) ? d : NAN; h.s2ok = f[8] == "1";
            h.dir = fullD(f[9], d) ? (int)d : 0; h.ext = fullD(f[10], d) ? d : NAN;   // python int(_f(x) or 0): truncation toward 0
            H = h; }
    }
    return H; }

// ---------- a bounded JSON reader for the Reader's state.json (only what the plan needs) ----------
struct J {
    const char* p; const char* e; bool bad = false; int depth = 0;
    J(const char* b, const char* en) : p(b), e(en) {}
    void ws() { while (p < e && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) ++p; }
    bool eat(char c) { ws(); if (p < e && *p == c) { ++p; return true; } return false; }
    static void put8(std::string& o, unsigned cp) {
        if (cp < 0x80) o += (char)cp; else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
        else { o += (char)(0xF0 | (cp >> 18)); o += (char)(0x80 | ((cp >> 12) & 0x3F)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); } }
    bool hex4(unsigned& v) { if (e - p < 4) return false; v = 0; for (int i = 0; i < 4; i++) { const char c = p[i]; v <<= 4;
            if (c >= '0' && c <= '9') v |= (unsigned)(c - '0'); else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10); else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10); else return false; }
        p += 4; return true; }
    bool str(std::string* out) {
        ws(); if (p >= e || *p != '"') { bad = true; return false; } ++p;
        while (p < e) { const char c = *p++;
            if (c == '"') return true;
            if (c == '\\') { if (p >= e) break; const char x = *p++;
                switch (x) { case '"': case '\\': case '/': if (out) *out += x; break; case 'b': if (out) *out += '\b'; break; case 'f': if (out) *out += '\f'; break;
                    case 'n': if (out) *out += '\n'; break; case 'r': if (out) *out += '\r'; break; case 't': if (out) *out += '\t'; break;
                    case 'u': { unsigned v; if (!hex4(v)) { bad = true; return false; }
                        if (v >= 0xD800 && v <= 0xDBFF && e - p >= 6 && p[0] == '\\' && p[1] == 'u') { p += 2; unsigned lo; if (!hex4(lo)) { bad = true; return false; } v = 0x10000 + ((v - 0xD800) << 10) + (lo - 0xDC00); }
                        if (out) put8(*out, v);
                        break; }
                    default: bad = true; return false; } }
            else if (out) *out += c; }
        bad = true; return false; }
    bool num(double& v) { ws(); const char* s = p; while (p < e && (std::strchr("+-0123456789.eE", *p) != nullptr)) ++p;
        if (p == s || p - s > 60) { bad = true; return false; } std::string t(s, (size_t)(p - s)); if (!fullD(t, v)) { bad = true; return false; } return true; }
    bool lit(const char* w) { const size_t n = std::strlen(w); if ((size_t)(e - p) >= n && std::strncmp(p, w, n) == 0) { p += n; return true; } return false; }
    // a number or null (null / anything else -> NAN, skipped)
    double numOrNull() { ws(); if (p < e && (*p == '-' || (*p >= '0' && *p <= '9'))) { double v; return num(v) ? v : NAN; } skip(); return NAN; }
    bool boolOrNull() { ws(); if (lit("true")) return true; if (lit("false") || lit("null")) return false; skip(); return false; }
    void skip() {
        ws(); if (p >= e) { bad = true; return; }
        if (++depth > 256) { bad = true; return; }
        const char c = *p;
        if (c == '"') str(nullptr);
        else if (c == '{') { ++p; if (!eat('}')) { do { if (!str(nullptr) || !eat(':')) { bad = true; break; } skip(); if (bad) break; } while (eat(',')); if (!bad && !eat('}')) bad = true; } }
        else if (c == '[') { ++p; if (!eat(']')) { do { skip(); if (bad) break; } while (eat(',')); if (!bad && !eat(']')) bad = true; } }
        else if (lit("true") || lit("false") || lit("null")) {}
        else { double v; num(v); }
        --depth; }
    // iterate an object's members: f(key) must consume the value
    template <class F> bool obj(F f) { if (!eat('{')) { bad = true; return false; } if (eat('}')) return true;
        do { std::string k; if (!str(&k) || !eat(':')) { bad = true; return false; } f(k); if (bad) return false; } while (eat(','));
        if (!eat('}')) { bad = true; return false; } return true; }
    template <class F> bool arr(F f) { if (!eat('[')) { bad = true; return false; } if (eat(']')) return true;
        do { f(); if (bad) return false; } while (eat(','));
        if (!eat(']')) { bad = true; return false; } return true; }
};

inline St parseState(const std::string& text, const std::string& mkt) {
    St S; if (text.empty()) { S.why = "empty"; return S; }
    if (text.size() > MAX_STATE) { S.why = "over 16 MiB"; return S; }
    J j(text.data(), text.data() + text.size());
    double price = NAN, live = NAN;
    j.obj([&](const std::string& k) {
        if (k == "session") { j.ws(); if (j.p < j.e && *j.p == '"') j.str(&S.session); else j.skip(); }
        else if (k == "now_ct") { j.ws(); if (j.p < j.e && *j.p == '"') j.str(&S.nowCt); else j.skip(); }
        else if (k == "markets") {
            j.ws(); if (j.p >= j.e || *j.p != '{') { j.skip(); return; }
            j.obj([&](const std::string& m) {
                if (m != mkt) { j.skip(); return; }
                j.ws(); if (j.p >= j.e || *j.p != '{') { j.skip(); return; }
                S.marketFound = true;
                j.obj([&](const std::string& f) {
                    if (f == "em") S.em = j.numOrNull();
                    else if (f == "price") price = j.numOrNull();
                    else if (f == "price_live") live = j.numOrNull();
                    else if (f == "ladder") {
                        j.ws(); if (j.p >= j.e || *j.p != '[') { j.skip(); return; }
                        j.arr([&]() {
                            Lad L; j.ws(); if (j.p >= j.e || *j.p != '{') { j.skip(); return; }
                            j.obj([&](const std::string& g) {
                                if (g == "code") { j.ws(); if (j.p < j.e && *j.p == '"') j.str(&L.code); else j.skip(); }
                                else if (g == "price") L.px = j.numOrNull();
                                else if (g == "price_fmt") { j.ws(); if (j.p < j.e && *j.p == '"') j.str(&L.fmt); else j.skip(); }
                                else if (g == "mq_only") L.mqOnly = j.boolOrNull();
                                else if (g == "mq_zone") { j.ws(); if (j.p < j.e && *j.p == '[') j.arr([&]() { std::string z; j.ws(); if (j.p < j.e && *j.p == '"') { j.str(&z); L.zone.push_back(z); } else j.skip(); }); else j.skip(); }
                                else j.skip(); });
                            S.lad.push_back(L); });
                    }
                    else j.skip(); });
            });
        }
        else j.skip(); });
    if (j.bad) { S.why = "garbled / torn"; S.lad.clear(); S.marketFound = false; return S; }
    for (Lad& L : S.lad) { L.code = toAnsi(L.code); L.fmt = toAnsi(L.fmt); for (std::string& z : L.zone) z = toAnsi(z); }   // drawn with IRT's ANSI drawText
    S.px = std::isfinite(live) ? live : price;   // the Reader: x.price_live ?? x.price
    S.ok = true; S.why = S.marketFound ? "ok" : "market not in the Reader state";
    return S; }

// ---------- the Reader's tag helpers ----------
inline std::string replWord(const std::string& s, const std::string& w, const std::string& to) {   // JS replace(/\bW\b/g, to)
    auto wc = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; };
    std::string o; size_t i = 0;
    while (i < s.size()) { if (s.compare(i, w.size(), w) == 0 && (i == 0 || !wc(s[i - 1])) && (i + w.size() >= s.size() || !wc(s[i + w.size()]))) { o += to; i += w.size(); } else o += s[i++]; }
    return o; }
inline std::string stripTrailNum(const std::string& t) {   // JS .replace(/ [\d,.]+$/, '')
    size_t i = t.size(); while (i > 0 && (isdigit((unsigned char)t[i - 1]) || t[i - 1] == ',' || t[i - 1] == '.')) --i;
    if (i < t.size() && i > 0 && t[i - 1] == ' ') return t.substr(0, i - 1);
    return t; }
// mqAb: the MenthorQ zone names -> "PS0+PS" (order of MQAB per name, set order kept)
inline std::string mqAb(const std::vector<std::string>& z) {
    static const char* const PRE[][2] = { { "0DTE Call Resistance", "CR0" }, { "Call Resistance", "CR" }, { "0DTE Put Support", "PS0" }, { "Put Support", "PS" },
        { "0DTE HVL", "HVL0" }, { "HVL", "HVL" }, { "0DTE Gamma Wall", "GW0" }, { "Gamma Wall", "GW" }, { "1D Min", "1DL" }, { "1D Max", "1DH" } };
    std::vector<std::string> out;
    for (const std::string& t0 : z) {
        std::string r;
        for (const auto& pr : PRE) if (t0.compare(0, std::strlen(pr[0]), pr[0]) == 0) { const std::string t = stripTrailNum(t0); r = t.compare(0, std::strlen(pr[0]), pr[0]) == 0 ? pr[1] + t.substr(std::strlen(pr[0])) : t; break; }
        if (r.empty() && t0.compare(0, 3, "GEX") == 0) {   // /^GEX ?(\d+)/ -> G$1
            size_t k = 3; if (k < t0.size() && t0[k] == ' ') ++k; const size_t d0 = k; while (k < t0.size() && isdigit((unsigned char)t0[k])) ++k;
            if (k > d0) { const std::string t = stripTrailNum(t0); size_t q = 3; if (q < t.size() && t[q] == ' ') ++q; size_t q0 = q; while (q < t.size() && isdigit((unsigned char)t[q])) ++q;
                r = q > q0 ? "G" + t.substr(q0, q - q0) + t.substr(q) : t; } }
        if (!r.empty() && std::find(out.begin(), out.end(), r) == out.end()) out.push_back(r); }
    std::string s; for (size_t i = 0; i < out.size(); i++) s += (i ? "+" : "") + out[i]; return s; }
inline std::string tagOf(const Lad& l) {
    if (l.mqOnly) return mqAb(l.zone);
    std::string c = l.code; for (size_t i; (i = c.find(" \xB7 ")) != std::string::npos;) c.replace(i, 3, "/");   // ' · ' -> '/' (ANSI after toAnsi)
    c = replWord(replWord(c, "LH", "LonHI"), "LL", "LonLO");                                                          // KN()
    c = replWord(replWord(c, "AH", "AsiaHI"), "AL", "AsiaLO");
    return c + (l.zone.empty() ? std::string() : "+" + mqAb(l.zone)); }

// ---------- the plan (todayPlanHtf line() for one market) ----------
inline Plan build(const Htf& H, bool htfStale, const St& st, const std::string& mkt) {
    Plan P; P.time12 = t12(st.nowCt);
    if (!st.ok) { P.why = "Reader state " + st.why; return P; }
    if (!st.marketFound) { P.why = "Reader state has no " + mkt; return P; }
    if (!H.have) { P.why = "no 3-hour levels (CobSignal HTFNOW)"; return P; }
    if (htfStale) { P.why = "3-hour levels stale (CobSignal status older than 14 h)"; return P; }
    const double px = st.px, em = st.em;
    if (!std::isfinite(px) || !std::isfinite(em) || em == 0) { P.why = "Reader has no price / expected move"; return P; }
    std::vector<const Lad*> lad; for (const Lad& l : st.lad) if (std::isfinite(l.px)) lad.push_back(&l);
    auto line = [&](const char* nm, bool buy, double v, bool ok, std::vector<Row>& out) {
        if (!std::isfinite(v) || !ok) return;
        const double d = std::fabs(px - v) / em;
        const bool beyond = buy ? (H.dir < 0 && std::isfinite(H.ext) && H.ext < v) : (H.dir > 0 && std::isfinite(H.ext) && H.ext > v);
        const std::string chip = beyond ? (buy ? "SPRING" : "UPTHRUST") : (d <= NEAR_EM ? "NEAR" : "");
        if (chip.empty()) return;
        std::vector<std::string> tg;
        for (const Lad* l : lad) if (std::fabs(l->px - v) <= TAG_EM * em) { const std::string t = tagOf(*l); if (!t.empty() && std::find(tg.begin(), tg.end(), t) == tg.end()) tg.push_back(t); }
        std::string tags; for (size_t i = 0; i < tg.size(); i++) tags += (i ? " \xB7 " : "") + tg[i];   // ANSI (IRT drawText); statusLine writes UTF-8
        std::vector<const Lad*> far; for (const Lad* l : lad) if (buy ? l->px >= v + NEAR_EM * em : l->px <= v - NEAR_EM * em) far.push_back(l);
        std::stable_sort(far.begin(), far.end(), [buy](const Lad* a, const Lad* b) { return buy ? a->px < b->px : a->px > b->px; });
        Row r; r.level = std::string(nm) + " " + pxTxt(v, mkt); r.tags = tags; r.chip = chip; r.target = far.empty() ? std::string() : far[0]->fmt;
        out.push_back(r); };
    auto same = [](double a, double b) { return std::isfinite(a) && std::isfinite(b) && std::fabs(a - b) <= std::fabs(a) * 1e-9; };
    line("S1", true, H.s1, H.s1ok, P.buy); if (!same(H.s1, H.s2)) line("S2", true, H.s2, H.s2ok, P.buy);
    line("R1", false, H.r1, H.r1ok, P.sell); if (!same(H.r1, H.r2)) line("R2", false, H.r2, H.r2ok, P.sell);
    P.ok = true; P.why = "ok"; return P; }

// one status line
inline std::string statusLine(const Plan& P, const std::string& src) {
    std::string s = std::string("TODAY_PLAN,") + VERSION + "," + (P.ok ? "ok" : P.why) + ",time " + (P.time12.empty() ? "-" : P.time12) + ",buy ";
    if (P.buy.empty()) s += "-";
    for (size_t i = 0; i < P.buy.size(); i++) s += (i ? " ; " : "") + P.buy[i].level + " " + P.buy[i].tags + " " + P.buy[i].chip + (P.buy[i].target.empty() ? "" : " T1 " + P.buy[i].target);
    s += ",sell ";
    if (P.sell.empty()) s += "-";
    for (size_t i = 0; i < P.sell.size(); i++) s += (i ? " ; " : "") + P.sell[i].level + " " + P.sell[i].tags + " " + P.sell[i].chip + (P.sell[i].target.empty() ? "" : " T1 " + P.sell[i].target);
    return toUtf8(s + ",src " + src) + "\n"; }

}   // namespace tpl
