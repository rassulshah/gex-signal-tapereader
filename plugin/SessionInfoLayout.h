// SessionInfoLayout.h - SessionInfo 1.7.1's session-file parsing, content and layout as plain C++ (no SDK), so the box can be tested
// and previewed exactly as the plugin draws it (the draw list is hl::Item, executed by SessionInfo.cpp).
// (Rassul 2026-10-12, mockup 11 / 13) ONE cell, flush top-left, ending before the last price bar:
//   "Regime: Positive"  (+ "  .  data 6h40 old" when the bridge is stale)
//   "Expiry 12:30 PM (in 13h 45m) . pin 4,200 (GW 0DTE)"
// The news / events, the RTH active-chop windows and the expected move with its slider are no longer in the box.
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
static const unsigned C_AMBER = 0x00F59E0B, C_RED = 0x00EF4444, C_GREEN = 0x0022C55E;

static const size_t MAX_SESSION_LINE = 4096;
static const size_t MAX_SESSION_BYTES = 64 * 1024;     // checklist #33

struct Data {
    std::string exp, gam, hvl, gw0, gw0d, dAge, dRec;
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
// the bridge's LRA-Session-<MKT>.csv (format unchanged since 1.6.0): returns false for an oversized file
inline bool parseSession(const std::string& text, Data& D)
{
    D.clear();
    if (text.size() > MAX_SESSION_BYTES) return false;
    std::stringstream in(text); std::string ln;
    while (std::getline(in, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.size() > MAX_SESSION_LINE) continue;
        // (1.7.1) split keeping empty fields: "EMR|lo|hi|px|pct|" has 6 fields (the trailing one empty). std::getline drops a
        // trailing empty field, which made GC's EMR line (used% empty) 5 fields -> rejected -> EXPECTED MOVE missing (camera 22:30)
        std::vector<std::string> c; size_t p0 = 0;
        while (c.size() < 16) { size_t q = ln.find('|', p0); c.push_back(ln.substr(p0, q == std::string::npos ? std::string::npos : q - p0)); if (q == std::string::npos) break; p0 = q + 1; }
        if (c.empty()) continue;
        if (c[0] == "EXP" && c.size() >= 2 && hm(c[1]) >= 0) D.exp = c[1];
        // (HL105) WIN / EMR / CAL / NEWS rows are not read: the box no longer shows windows, the expected move or events
        else if (c[0] == "GAM" && c.size() >= 3) { D.gam = c[1]; D.hvl = c[2]; }
        else if (c[0] == "GW0" && c.size() >= 2) { D.gw0 = c[1]; D.gw0d = c.size() >= 3 ? c[2] : ""; }
        else if (c[0] == "DAGE" && c.size() >= 3) { D.dAge = c[1]; D.dRec = c[2]; }
    }
    return true;
}

struct Seg { std::string s; unsigned col; bool bold; };
struct Row { std::vector<Seg> segs; };
struct Section { std::vector<Row> rows; int minW = 0; };
struct Slider { bool on = false; };     // (HL105) the expected-move slider is gone; kept as the build() out-parameter (always off)

inline Row row(const std::string& s, unsigned c, bool b) { Row r; r.segs.push_back(Seg{ s, c, b }); return r; }

struct Inputs {
    std::string mkt; Data D; bool sourceAvailable = false;
    int now = 0, dow = 5, show = 0, fs = 8;
};

inline std::string t12(int m, bool suffix) { char b[16]; int h = (m / 60) % 24; snprintf(b, sizeof b, suffix ? "%d:%02d %s" : "%d:%02d", h % 12 == 0 ? 12 : h % 12, m % 60, h < 12 ? "AM" : "PM"); return b; }

// drop all-zero decimals from a bridge price: "4,195.0" -> "4,195" ("4,105.1" stays)
inline std::string trimPx(const std::string& p) { size_t d = p.find('.'); if (d != std::string::npos && p.find_first_not_of('0', d + 1) == std::string::npos) return p.substr(0, d); return p; }
inline std::vector<Section> build(const Inputs& I, Slider& SL)
{
    std::vector<Section> out;
    const Data& D = I.D; const int now = I.now; const bool all = I.show == 0;
    SL = Slider();
    if (!I.sourceAvailable) { Section s; s.rows.push_back(row(I.mkt + " session data unavailable", C_GREY, true)); out.push_back(s); return out; }
    // (HL105, mockup 11) the NEWS / EVENTS cell is gone from the box (the calendar rows stay in the session file for the Reader)
    // ---- (HL105, mockup 8) ONE cell, two lines: "Regime: Positive" (+ "  \xB7  data 6h40 old" when stale) over
    //      "Expiry 12:30 PM (in 13h 45m) \xB7 pin 4,200 (GW 0DTE)"
    if (all || I.show == 1) {
        Section s; bool neg = D.gam == "NEG";
        Row r1;   // (HL105, mockup 11) the box is this one cell
        if (D.gam.empty()) r1.segs.push_back(Seg{ "Regime: n/a", C_GREY, true });
        else r1.segs.push_back(Seg{ neg ? "Regime: Negative" : "Regime: Positive", neg ? C_RED : C_GREEN, true });
        int age = -1; if (!D.dAge.empty()) parseInt(D.dAge, age);
        bool rec = D.dRec.empty() || D.dRec == "OK";
        if (age > 10 || !rec) {
            std::string t = age > 10 ? (age >= 60 ? "data " + std::to_string(age / 60) + "h" + (age % 60 < 10 ? "0" : "") + std::to_string(age % 60) + " old" : "data " + std::to_string(age) + " min old") : "";
            if (!rec) t += std::string(t.empty() ? "" : " \xB7 ") + "IRT not recording";
            r1.segs.push_back(Seg{ "  \xB7  " + t, rec ? C_GREY : C_AMBER, false });
        }
        s.rows.push_back(r1);
        Row r2; int e = hm(D.exp);
        if (e < 0) r2.segs.push_back(Seg{ "no 0DTE expiry today", C_GREY, false });
        else {
            int left = e - now; if (now >= 17 * 60) left = e + 1440 - now;
            char b[64];
            if (left > 0) {
                snprintf(b, sizeof b, "Expiry %s (in %dh %02dm)", t12(e, true).c_str(), left / 60, left % 60);
                r2.segs.push_back(Seg{ b, left <= 15 ? C_RED : left <= 60 ? C_AMBER : C_MUTED, left <= 60 });
                r2.segs.push_back(Seg{ D.gw0.empty() ? std::string(" \xB7 no pin today") : " \xB7 pin " + trimPx(D.gw0) + " (GW 0DTE)", D.gw0.empty() ? C_GREY : C_INK, false });
            } else { snprintf(b, sizeof b, "Expiry %s (expired)", t12(e, true).c_str()); r2.segs.push_back(Seg{ b, C_GREY, false }); }
        }
        s.rows.push_back(r2);
        s.minW = 150;
        out.push_back(s);
    }
    return out;
}

// (1.7.1) shorten at a WORD boundary: whole words + "..." (never cut mid-word); a single word too long for w is the only exception
inline std::string fit(const std::string& s, int w, int fs, bool bold, const hl::Measure& M)
{
    if (M(s, fs, bold) <= w) return s;
    std::vector<size_t> cuts;                                          // the ends of whole words
    for (size_t i = 1; i < s.size(); i++) if (s[i] == ' ' && s[i - 1] != ' ') cuts.push_back(i);
    size_t lo = 0, hi = cuts.size();                                  // the largest k with words[0..k) + "..." fitting
    while (lo < hi) { size_t mid = lo + (hi - lo + 1) / 2; if (M(s.substr(0, cuts[mid - 1]) + "...", fs, bold) <= w) lo = mid; else hi = mid - 1; }
    if (lo > 0) { std::string t = s.substr(0, cuts[lo - 1]); while (!t.empty() && (t[t.size() - 1] == ' ' || t[t.size() - 1] == '-' || t[t.size() - 1] == ':')) { if (t[t.size() - 1] == ':') break; t.erase(t.size() - 1); } return t + "..."; }
    size_t a = 0, b = s.size();                                        // one word wider than w: characters (rare, very narrow)
    while (a < b) { size_t mid = a + (b - a + 1) / 2; if (M(s.substr(0, mid) + "...", fs, bold) <= w) a = mid; else b = mid - 1; }
    return s.substr(0, a) + "...";
}

inline int rowW(const Row& R, int fs, const hl::Measure& M) { int x = 0; for (size_t k = 0; k < R.segs.size(); k++) x += M(R.segs[k].s, fs, R.segs[k].bold); return x; }

// (HL105, mockups 11 / 13) the cell(s) in one row, flush at the top-left of the region, ending before the last price bar (and the
// profiles). Too wide: a smaller font (down to 8 pt), then every line shortened at a word. Never a mid-word cut, never a dropped cell.
static const int MIN_FS = 8;
inline hl::Layout layout(const std::vector<Section>& S, const Slider& SL, const hl::Box& region, int pos, int fs0, const hl::Measure& M)
{
    (void)SL;
    hl::Layout L;
    if (S.empty()) return L;
    const int avail = region.r - region.l, n = (int)S.size();
    int fs = fs0, pad = 0, lh = 0, gap = 0, W = 0, H = 0;
    std::vector<int> w;
    auto natural = [&](int f) {
        w.assign((size_t)n, 0);
        for (int i = 0; i < n; i++) {
            int m = (int)(S[(size_t)i].minW * f / 8.0f + 0.5f);
            for (size_t r = 0; r < S[(size_t)i].rows.size(); r++) {
                int x = rowW(S[(size_t)i].rows[r], f, M);
                m = (std::max)(m, x);
            }
            w[(size_t)i] = m;
        }
        pad = (int)(f * 0.8f + 0.5f); lh = (int)(f * 1.6f + 0.5f); gap = pad * 2;
        int rows = 1; for (int i = 0; i < n; i++) rows = (std::max)(rows, (int)S[(size_t)i].rows.size());
        W = pad; for (int i = 0; i < n; i++) W += w[(size_t)i] + (i + 1 < n ? gap : pad);
        H = pad + rows * lh;
    };
    bool ok = false;
    for (int f = fs0; f >= MIN_FS && !ok; f--) { natural(f); if (W <= avail) { ok = true; fs = f; } }
    if (!ok) {                                                       // squeeze at 8 pt: the cells share the room
        fs = MIN_FS; natural(fs);
        int over = W - avail;
        if (over > 0) {
            int tot = 0;
            for (int i = 0; i < n; i++) tot += w[(size_t)i];
            int room = tot - over;
            for (int i = 0; i < n; i++) w[(size_t)i] = tot > 0 ? w[(size_t)i] * room / tot : 0;
        }
        W = pad; for (int i = 0; i < n; i++) W += w[(size_t)i] + (i + 1 < n ? gap : pad);
        for (int i = 0; i < n; i++) if (w[(size_t)i] < 80) L.note = "pane too narrow";
    }
    if (!L.note.empty() || H > region.b - region.t) {
        if (L.note.empty()) L.note = "pane too short";
        std::string s = "Session Info: " + L.note; int tw = M(s, MIN_FS, false), lh8 = (int)(MIN_FS * 1.6f + 0.5f);
        if (tw + 8 <= avail && lh8 + 4 <= region.b - region.t) {
            hl::addRect(L, region.l, region.t, region.l + tw + 8, region.t + lh8 + 4, C_BORDER, C_BOXBG, 1, hl::R_NOTE);
            hl::addText(L, region.l + 4, region.t + 2, region.t + 2 + lh8, s, C_GREY, MIN_FS, false, tw, hl::R_NOTE, 0);
            L.blocks.push_back(hl::Box{ region.l, region.t, region.l + tw + 8, region.t + lh8 + 4 });
        }
        return L;
    }
    if (pos < 0 || pos > 8) pos = 0;
    int col = pos % 3, rw = pos / 3, x0, y0;
    x0 = col == 0 ? region.l : col == 1 ? (region.l + region.r) / 2 - W / 2 : region.r - W;
    y0 = rw == 0 ? region.t : rw == 1 ? (region.t + region.b) / 2 - H / 2 : region.b - H;
    x0 = (std::max)(region.l, (std::min)(x0, region.r - W)); y0 = (std::max)(region.t, (std::min)(y0, region.b - H));
    hl::Box box = { x0, y0, x0 + W, y0 + H };
    L.table = box; L.tableOk = true; L.blocks.push_back(box);
    hl::addRect(L, box.l, box.t, box.r, box.b, C_BORDER, C_BOXBG, 1, hl::R_TABLE);
    int x = x0 + pad, yRow = y0 + pad / 2;
    for (int i = 0; i < n; i++) {
        const Section& Sec = S[(size_t)i]; const int cw = w[(size_t)i];
        if (i > 0) hl::addLine(L, x - gap / 2, y0 + 2, x - gap / 2, y0 + H - 2, C_BORDER, 1, hl::R_TABLE);
        for (size_t ri = 0; ri < Sec.rows.size(); ri++) {
            int t = yRow + (int)ri * lh, b = t + lh;
            const Row& R = Sec.rows[ri];
            int xx = x;
            for (size_t sg = 0; sg < R.segs.size(); sg++) {
                std::string s = fit(R.segs[sg].s, x + cw - xx, fs, R.segs[sg].bold, M);
                int tw = M(s, fs, R.segs[sg].bold);
                hl::addText(L, xx, t, b, s, R.segs[sg].col, fs, R.segs[sg].bold, tw, hl::R_TABLE, i);
                xx += tw;
                if (s.size() >= 3 && s.compare(s.size() - 3, 3, "...") == 0) break;    // the rest of a shortened line is dropped
            }
        }
        x += cw + gap;
    }
    return L;
}

}  // namespace sil
#endif
