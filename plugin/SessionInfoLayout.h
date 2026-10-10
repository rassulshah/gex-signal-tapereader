// SessionInfoLayout.h - SessionInfo 1.7.0's data parsing, content and WIDE layout as plain C++ (no SDK), so the panel can be
// tested and previewed exactly as the plugin draws it (the draw list is hl::Item, executed by SessionInfo.cpp).
// (Rassul 2026-10-09, mockup v5) four sections side by side, header + 3 lines:
//   NEWS | GAMMA / Exp <time> (<countdown>) | <MKT> RTH now: <state> + the Active / Chop windows | EXPECTED MOVE + coloured slider
// The content is the 1.6.7 content (news + countdown, gamma regime + HVL, 0DTE expiry + countdown, the GW0 pin, the RTH active /
// chop windows, the expected move with spot and range used, data age / IRT not recording); only the layout changes.
#ifndef SESSIONINFO_LAYOUT_H
#define SESSIONINFO_LAYOUT_H
#include "HodLodLogic.h"
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>

namespace sil {

static const unsigned C_INK = 0x00E5E7EB, C_MUTED = 0x009CA3AF, C_GREY = 0x0064748B, C_BOXBG = 0x000F1520, C_BORDER = 0x00374151;
static const unsigned C_AMBER = 0x00F59E0B, C_RED = 0x00EF4444, C_GREEN = 0x0022C55E, C_SLIDER_MID = 0x00374151, C_WHITE = 0x00FFFFFF;

static const size_t MAX_SESSION_LINE = 4096, MAX_SESSION_WINDOWS = 64, MAX_SESSION_NEWS = 64;
static const size_t MAX_SESSION_BYTES = 64 * 1024;     // checklist #33

struct Win { std::string tag, pct; int fromMin = -1, toMin = -1; };
struct Ev { std::string t, imp, title, cd; bool brk = false; };
struct Data {
    std::string exp;
    std::vector<Win> W;
    std::vector<Ev> news;
    std::string gam, hvl, gw0, gw0d, emLo, emHi, emPx, emPct, emUsed, dAge, dRec;
    void clear() { *this = Data(); }
};

inline int hm(const std::string& s)
{
    if (s.size() != 5 || s[2] != ':' || !isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1])
        || !isdigit((unsigned char)s[3]) || !isdigit((unsigned char)s[4])) return -1;
    int h = (s[0] - '0') * 10 + s[1] - '0', m = (s[3] - '0') * 10 + s[4] - '0';
    return h < 24 && m < 60 ? h * 60 + m : -1;
}
// strict, overflow-safe int (external input)
inline bool parseInt(const std::string& s, int& out)
{
    if (s.empty()) return false;
    size_t i = 0; bool negative = false;
    if (s[i] == '+' || s[i] == '-') { negative = s[i] == '-'; ++i; }
    if (i == s.size()) return false;
    const unsigned int limit = negative ? (unsigned int)INT_MAX + 1U : (unsigned int)INT_MAX;
    unsigned int n = 0;
    for (; i < s.size(); ++i) {
        if (!isdigit((unsigned char)s[i])) return false;
        unsigned int digit = (unsigned int)(s[i] - '0');
        if (n > (limit - digit) / 10U) return false;
        n = n * 10U + digit;
    }
    if (negative) out = n == (unsigned int)INT_MAX + 1U ? INT_MIN : -(int)n;
    else out = (int)n;
    return true;
}
// strict price "7,786.65" / "6.5500" / "-0.25" (commas allowed only as thousands separators)
inline bool parsePrice(const std::string& s0, double& out)
{
    std::string s; for (size_t i = 0; i < s0.size(); i++) if (s0[i] != ',') s += s0[i];
    if (s.empty() || s.size() > 32) return false;
    size_t i = 0; if (s[0] == '-' || s[0] == '+') i = 1;
    bool dig = false, dot = false;
    for (; i < s.size(); i++) { if (isdigit((unsigned char)s[i])) dig = true; else if (s[i] == '.' && !dot) dot = true; else return false; }
    if (!dig) return false;
    char* e = nullptr; double v = std::strtod(s.c_str(), &e);
    if (!e || *e || !std::isfinite(v)) return false;
    out = v; return true;
}
inline bool inWindow(int now, int from, int to)
{
    if (now < 0 || from < 0 || to < 0 || from == to) return false;
    return from < to ? now >= from && now < to : now >= from || now < to;
}
// times on the 12-hour clock inside free text ("Fri 15:00  FOMC" -> "Fri 3:00 PM  FOMC")
inline std::string to12(const char* in)
{
    std::string s(in ? in : ""), o; o.reserve(s.size() + 8);
    const size_t n = s.size();
    auto dig = [&](size_t k) { return k < n && isdigit((unsigned char)s[k]) != 0; };
    for (size_t i = 0; i < n; i++) {
        bool startOk = i == 0 || !(dig(i - 1) || s[i - 1] == ':' || s[i - 1] == '.' || s[i - 1] == ',');
        size_t hl = dig(i) ? (dig(i + 1) ? 2 : 1) : 0;
        if (startOk && hl && i + hl < n && s[i + hl] == ':' && dig(i + hl + 1) && dig(i + hl + 2)
            && !dig(i + hl + 3) && !(i + hl + 3 < n && s[i + hl + 3] == ':')) {
            int h = (s[i] - '0') * (hl == 2 ? 10 : 1) + (hl == 2 ? s[i + 1] - '0' : 0), m = (s[i + hl + 1] - '0') * 10 + (s[i + hl + 2] - '0');
            if (h <= 23 && m <= 59) {
                char b[32]; snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM");
                o += b; i += hl + 2; continue;
            }
        }
        o += s[i];
    }
    return o;
}

// the bridge's LRA-Session-<MKT>.csv (format unchanged since 1.6.0): returns false for an oversized file
inline bool parseSession(const std::string& text, Data& D)
{
    D.clear();
    if (text.size() > MAX_SESSION_BYTES) return false;
    std::stringstream in(text); std::string ln;
    while (std::getline(in, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.size() > MAX_SESSION_LINE) continue;
        std::vector<std::string> c; std::stringstream ss(ln); std::string x;
        while (std::getline(ss, x, '|')) c.push_back(x);
        if (c.empty()) continue;
        if (c[0] == "EXP" && c.size() >= 2 && hm(c[1]) >= 0) D.exp = c[1];
        else if (c[0] == "WIN" && c.size() >= 4) {
            int from = hm(c[2]), to = hm(c[3]);
            if (D.W.size() < MAX_SESSION_WINDOWS && (c[1] == "ACTIVE" || c[1] == "CHOP") && from >= 0 && to >= 0 && from != to) {
                Win w; w.tag = c[1]; w.fromMin = from; w.toMin = to; if (c.size() >= 6) w.pct = c[5]; D.W.push_back(w);
            }
        }
        else if ((c[0] == "CAL" || c[0] == "NEWS") && c.size() >= 4 && D.news.size() < MAX_SESSION_NEWS) {
            Ev e; e.t = c[1]; e.imp = c[2]; e.title = c[3]; e.brk = c[0] == "NEWS"; if (c.size() >= 5) e.cd = c[4]; D.news.push_back(e);
        }
        else if (c[0] == "GAM" && c.size() >= 3) { D.gam = c[1]; D.hvl = c[2]; }
        else if (c[0] == "GW0" && c.size() >= 2) { D.gw0 = c[1]; D.gw0d = c.size() >= 3 ? c[2] : ""; }
        else if (c[0] == "EMR" && c.size() >= 6) { D.emLo = c[1]; D.emHi = c[2]; D.emPx = c[3]; D.emPct = c[4]; D.emUsed = c[5]; }
        else if (c[0] == "DAGE" && c.size() >= 3) { D.dAge = c[1]; D.dRec = c[2]; }
    }
    return true;
}

// a news time: "HH:MM" (today) or "Ddd HH:MM" (the bridge's calendar rows). dayOk = the item is today's.
inline int newsMin(const std::string& t, int todayDow, bool& today)
{
    static const char* D[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    today = true;
    if (t.size() == 5) return hm(t);
    if (t.size() == 9 && t[3] == ' ') {
        int dow = -1; for (int i = 0; i < 7; i++) if (t.compare(0, 3, D[i]) == 0) dow = i;
        if (dow < 0) return -1;
        today = dow == todayDow;
        return hm(t.substr(4));
    }
    return -1;
}

struct Seg { std::string s; unsigned col; bool bold; };
struct Row { std::vector<Seg> segs; int kind = 0; };   // kind 1 = the expected-move slider
struct Section { std::vector<Row> rows; int minW = 0; };
struct Slider { bool on = false; int pct = 0; bool hasE = false; double eLodF = 0, eHodF = 0; };

inline Row row(const std::string& s, unsigned c, bool b) { Row r; r.segs.push_back(Seg{ s, c, b }); return r; }

struct Inputs {
    std::string mkt; Data D; bool sourceAvailable = false;
    int now = 0, dow = 5, show = 0, fs = 8;
    bool haveE = false; double eLod = NAN, eHod = NAN;   // E-LOD / E-HOD from the HOD/LOD model (chart bars, native)
};

inline std::string t12(int m, bool suffix) { char b[16]; int h = (m / 60) % 24; snprintf(b, sizeof b, suffix ? "%d:%02d %s" : "%d:%02d", h % 12 == 0 ? 12 : h % 12, m % 60, h < 12 ? "AM" : "PM"); return b; }
inline std::string range12(int a, int b) { bool same = (a / 60 < 12) == (b / 60 < 12); return same ? t12(a, false) + "-" + t12(b, true) : t12(a, true) + "-" + t12(b, true); }

// the four sections (only the one chosen in Show when Show is not All)
inline std::vector<Section> build(const Inputs& I, Slider& SL)
{
    std::vector<Section> out;
    const Data& D = I.D; const int now = I.now; const bool all = I.show == 0;
    SL = Slider();
    if (!I.sourceAvailable) { Section s; s.rows.push_back(row(I.mkt + " session data unavailable", C_GREY, true)); out.push_back(s); return out; }
    // ---- NEWS: breaking first, then today's / coming items, then the past ones (grey); 3 lines
    if (all || I.show == 3) {
        Section s; s.rows.push_back(row("NEWS", C_MUTED, true));
        std::vector<Row> up, past;
        for (size_t i = 0; i < D.news.size(); i++) {
            const Ev& e = D.news[i];
            if (e.brk) { up.insert(up.begin(), row(to12((e.t + "  " + e.title).c_str()), C_AMBER, true)); continue; }
            bool today = true; int m = newsMin(e.t, I.dow, today);
            bool isPast = today && m >= 0 && m < now && !(now >= 17 * 60 && m < 17 * 60);
            bool soon = today && m >= now && m - now <= 30;
            std::string t = e.t; if (today && e.t.size() == 9) t = e.t.substr(4);      // today's: no weekday
            std::string ln = to12((t + "  " + e.title).c_str());
            if (!e.cd.empty() && !isPast) ln += " - " + e.cd;
            unsigned c = isPast ? C_GREY : soon ? C_RED : e.imp == "High" ? C_AMBER : C_INK;
            (isPast ? past : up).push_back(row(ln, c, soon || (!e.cd.empty() && !isPast)));
        }
        for (int i = (int)past.size() - 1; i >= 0; i--) up.push_back(past[(size_t)i]);   // most recent past first
        if (up.empty()) s.rows.push_back(row("none in 24h", C_GREY, false));
        for (size_t i = 0; i < up.size() && i < 3; i++) {
            Row r = up[i];
            if (i == 2 && up.size() > 3) r.segs[0].s += " (+" + std::to_string(up.size() - 3) + ")";
            s.rows.push_back(r);
        }
        if (up.size() == 1) s.rows.push_back(row("none else in 24h", C_GREY, false));
        s.minW = 150;
        out.push_back(s);
    }
    // ---- GAMMA / Exp <time> (<countdown>)
    if (all || I.show == 1) {
        Section s; int e = hm(D.exp); int left = 0;
        std::string head = "GAMMA  /  "; unsigned hc = C_MUTED;
        if (e < 0) head += "no 0DTE today";
        else {
            left = e - now; if (now >= 17 * 60) left = e + 1440 - now;
            char b[64];
            if (left > 0) { snprintf(b, sizeof b, "Exp %s (%dh%02dm)", t12(e, true).c_str(), left / 60, left % 60); hc = left <= 15 ? C_RED : left <= 60 ? C_AMBER : C_MUTED; }
            else snprintf(b, sizeof b, "0DTE expired %s", t12(e, true).c_str());
            head += b;
        }
        s.rows.push_back(row(head, hc, true));
        if (!D.gam.empty()) {
            bool neg = D.gam == "NEG";
            s.rows.push_back(row(neg ? "NEGATIVE - levels can break" : "POSITIVE - levels tend to hold", neg ? C_AMBER : C_GREEN, false));
        } else s.rows.push_back(row("gamma regime n/a", C_GREY, false));
        std::string l3;
        if (!D.hvl.empty()) l3 = "HVL " + D.hvl;
        if (e >= 0 && left > 0) {
            std::string pin;
            if (!D.gw0.empty()) {
                pin = "pin " + D.gw0;
                if (!D.gw0d.empty()) {
                    bool above = D.gw0d[0] != '-';
                    std::string pts = D.gw0d; if (pts[0] == '+' || pts[0] == '-') pts = pts.substr(1);
                    pin += " (" + pts + (above ? " above)" : " below)");
                }
            } else pin = "no pin today";
            l3 += (l3.empty() ? "" : "  \xB7  ") + pin;
        }
        if (!l3.empty()) s.rows.push_back(row(l3, C_INK, false));
        int age = -1; if (!D.dAge.empty()) parseInt(D.dAge, age);
        bool rec = D.dRec.empty() || D.dRec == "OK";
        if (age > 10 || !rec) {
            std::string t = age > 10 ? "data " + std::to_string(age) + " min old" : "";
            if (!rec) t += std::string(t.empty() ? "" : " - ") + "IRT not recording " + I.mkt;
            s.rows.push_back(row(t, rec ? C_GREY : C_AMBER, !rec));
        }
        s.minW = 200;
        out.push_back(s);
    }
    // ---- <MKT> RTH now: <state> + the windows (active green, chop amber)
    if ((all || I.show == 2) && !D.W.empty()) {
        Section s; std::string nowTag;
        for (size_t i = 0; i < D.W.size(); i++) if (inWindow(now, D.W[i].fromMin, D.W[i].toMin)) nowTag = D.W[i].tag;
        s.rows.push_back(row(I.mkt + (nowTag.empty() ? " RTH now: normal" : nowTag == "ACTIVE" ? " RTH now: ACTIVE" : " RTH now: CHOP - wait"),
                             nowTag == "ACTIVE" ? C_GREEN : nowTag == "CHOP" ? C_AMBER : C_MUTED, true));
        // up to 3 windows: the current and coming ones first, then the earliest of the rest
        std::vector<size_t> order;
        for (size_t i = 0; i < D.W.size(); i++) if (inWindow(now, D.W[i].fromMin, D.W[i].toMin) || D.W[i].fromMin >= now) order.push_back(i);
        if (order.size() < 3) for (size_t i = 0; i < D.W.size() && order.size() < 3; i++) { bool in = false; for (size_t k = 0; k < order.size(); k++) if (order[k] == i) in = true; if (!in) order.push_back(i); }
        std::sort(order.begin(), order.end());
        if (order.size() > 3) order.resize(3);
        for (size_t k = 0; k < order.size(); k++) {
            const Win& w = D.W[order[k]];
            std::string ln = std::string(w.tag == "ACTIVE" ? "Active " : "Chop ") + range12(w.fromMin, w.toMin);
            if (!w.pct.empty()) ln += " (" + w.pct + "%)";
            s.rows.push_back(row(ln, w.tag == "ACTIVE" ? C_GREEN : C_AMBER, inWindow(now, w.fromMin, w.toMin)));
        }
        s.minW = 170;
        out.push_back(s);
    }
    // ---- EXPECTED MOVE: range, the coloured slider (spot marker + the model's E-LOD / E-HOD ticks), spot line
    if ((all || I.show == 4) && !D.emLo.empty()) {
        Section s;
        s.rows.push_back(row(std::string("EXPECTED MOVE") + (D.emUsed.empty() ? "" : "  \xB7  used " + D.emUsed + "%"), C_MUTED, true));
        s.rows.push_back(row(D.emLo + " - " + D.emHi, C_INK, false));
        Row r; r.kind = 1; s.rows.push_back(r);
        int pct = 0; parseInt(D.emPct, pct); if (pct < 0) pct = 0; if (pct > 100) pct = 100;
        SL.on = true; SL.pct = pct;
        double lo = 0, hi = 0;
        if (I.haveE && std::isfinite(I.eLod) && std::isfinite(I.eHod) && parsePrice(D.emLo, lo) && parsePrice(D.emHi, hi) && hi > lo) {
            SL.hasE = true; SL.eLodF = (I.eLod - lo) / (hi - lo); SL.eHodF = (I.eHod - lo) / (hi - lo);
        }
        s.rows.push_back(row("spot " + D.emPx + " \xB7 " + D.emPct + "% up the range", C_INK, false));
        s.minW = 190;
        out.push_back(s);
    }
    return out;
}

// red -> grey -> green
inline unsigned gradient(double f)
{
    if (f < 0) f = 0;
    if (f > 1) f = 1;
    unsigned a = f < 0.5 ? C_RED : C_SLIDER_MID, b = f < 0.5 ? C_SLIDER_MID : C_GREEN; double t = f < 0.5 ? f * 2 : (f - 0.5) * 2;
    auto ch = [&](int sh) { int x = (int)((a >> sh) & 255), y = (int)((b >> sh) & 255); return (unsigned)std::lround(x + (y - x) * t) << sh; };
    return ch(16) | ch(8) | ch(0);
}

// longest prefix + "..." that fits w (binary search: log2 width calls, checklist #33)
inline std::string fit(const std::string& s, int w, int fs, bool bold, const hl::Measure& M)
{
    if (M(s, fs, bold) <= w) return s;
    size_t lo = 0, hi = s.size();
    while (lo < hi) { size_t mid = lo + (hi - lo + 1) / 2; if (M(s.substr(0, mid) + "...", fs, bold) <= w) lo = mid; else hi = mid - 1; }
    std::string t = s.substr(0, lo); while (!t.empty() && t[t.size() - 1] == ' ') t.erase(t.size() - 1);
    return t + "...";
}

// the WIDE panel. region = where the box may go (the pane left of the profiles and of the latest bars); pos = the 9-spot setting
inline hl::Layout layout(const std::vector<Section>& S, const Slider& SL, const hl::Box& region, int pos, int fs, const hl::Measure& M)
{
    hl::Layout L;
    if (S.empty()) return L;
    const int pad = (int)(fs * 0.9f + 0.5f), lh = (int)(fs * 1.75f + 0.5f), gap = pad * 2;
    int nRows = 0; for (size_t i = 0; i < S.size(); i++) nRows = (std::max)(nRows, (int)S[i].rows.size());
    if (nRows < 4) nRows = 4;
    std::vector<int> w(S.size());
    int W = pad;
    for (size_t i = 0; i < S.size(); i++) {
        int m = (int)(S[i].minW * fs / 8.0f + 0.5f);
        for (size_t r = 0; r < S[i].rows.size(); r++) { int x = 0; for (size_t k = 0; k < S[i].rows[r].segs.size(); k++) x += M(S[i].rows[r].segs[k].s, fs, S[i].rows[r].segs[k].bold); m = (std::max)(m, x); }
        m = (std::min)(m, (int)(330 * fs / 8.0f));                      // a long headline ends in "..." rather than widening the panel
        w[i] = m; W += m + (i + 1 < S.size() ? gap : pad);
    }
    int avail = region.r - region.l;
    if (W > avail) {                                                   // narrow pane: shrink every section proportionally
        double k = (double)(avail - pad * (int)S.size() * 3) / (double)(W - pad * (int)S.size() * 3);
        W = pad;
        for (size_t i = 0; i < S.size(); i++) { w[i] = (int)(w[i] * k); W += w[i] + (i + 1 < S.size() ? gap : pad); }
        for (size_t i = 0; i < S.size(); i++) if (w[i] < 60) { L.note = "pane too narrow"; }
    }
    int H = pad + nRows * lh;
    if (!L.note.empty() || H > region.b - region.t) {
        if (L.note.empty()) L.note = "pane too short";
        std::string s = "Session Info: " + L.note; int tw = M(s, fs, false);
        if (tw + 8 <= avail && lh + 4 <= region.b - region.t) {
            hl::addRect(L, region.l, region.t, region.l + tw + 8, region.t + lh + 4, C_BORDER, C_BOXBG, 1, hl::R_NOTE);
            hl::addText(L, region.l + 4, region.t + 2, region.t + 2 + lh, s, C_GREY, fs, false, tw, hl::R_NOTE, 0);
            L.blocks.push_back(hl::Box{ region.l, region.t, region.l + tw + 8, region.t + lh + 4 });
        }
        return L;
    }
    if (pos < 0 || pos > 8) pos = 1;
    int col = pos % 3, rw = pos / 3, x0, y0;
    x0 = col == 0 ? region.l + 8 : col == 1 ? (region.l + region.r) / 2 - W / 2 : region.r - 8 - W;
    y0 = rw == 0 ? region.t + 8 : rw == 1 ? (region.t + region.b) / 2 - H / 2 : region.b - 8 - H;
    x0 = (std::max)(region.l, (std::min)(x0, region.r - W)); y0 = (std::max)(region.t, (std::min)(y0, region.b - H));
    hl::Box box = { x0, y0, x0 + W, y0 + H };
    L.table = box; L.tableOk = true; L.blocks.push_back(box);
    hl::addRect(L, box.l, box.t, box.r, box.b, C_BORDER, C_BOXBG, 1, hl::R_TABLE);
    int x = x0 + pad;
    for (size_t i = 0; i < S.size(); i++) {
        if (i > 0) hl::addLine(L, x - gap / 2, y0 + pad / 2, x - gap / 2, y0 + H - pad / 2, C_BORDER, 1, hl::R_TABLE);
        for (size_t r = 0; r < S[i].rows.size(); r++) {
            int t = y0 + pad / 2 + (int)r * lh, b = t + lh;
            const Row& R = S[i].rows[r];
            if (R.kind == 1 && SL.on) {                               // the slider
                int bl = x, br = x + w[i], bt = (t + b) / 2 - fs / 2 + 1, bb = (t + b) / 2 + fs / 2 - 1;
                for (int px = bl; px < br; px += 2) hl::addFill(L, px, bt, (std::min)(px + 2, br), bb, gradient((double)(px - bl) / (double)(std::max)(1, br - bl - 1)), hl::R_TABLE);
                auto tick = [&](double f, unsigned c) {              // the model's E-LOD / E-HOD: a tick under the bar, an arrow at the end when outside
                    if (f >= 0 && f <= 1) { int xx = bl + (int)std::lround(f * (br - bl)); hl::addFill(L, xx - 1, bb, xx + 1, bb + 4, c, hl::R_TABLE); }
                    else if (f < 0) { hl::addLine(L, bl + 4, bb + 1, bl, bb + 3, c, 1, hl::R_TABLE); hl::addLine(L, bl, bb + 3, bl + 4, bb + 5, c, 1, hl::R_TABLE); }
                    else { hl::addLine(L, br - 4, bb + 1, br, bb + 3, c, 1, hl::R_TABLE); hl::addLine(L, br, bb + 3, br - 4, bb + 5, c, 1, hl::R_TABLE); }
                };
                if (SL.hasE) { tick(SL.eLodF, C_RED); tick(SL.eHodF, C_GREEN); }
                int mx = bl + (br - bl) * SL.pct / 100;
                hl::addFill(L, mx - 2, bt - 3, mx + 2, bb + 1, C_WHITE, hl::R_TABLE);
                continue;
            }
            int xx = x;
            for (size_t k = 0; k < R.segs.size(); k++) {
                std::string s = fit(R.segs[k].s, x + w[i] - xx, fs, R.segs[k].bold, M);
                int tw = M(s, fs, R.segs[k].bold);
                hl::addText(L, xx, t, b, s, R.segs[k].col, fs, R.segs[k].bold, tw, hl::R_TABLE, (int)i);
                xx += tw;
            }
        }
        x += w[i] + gap;
    }
    return L;
}

}  // namespace sil
#endif
