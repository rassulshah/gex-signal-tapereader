// HodLodTop.h -- lsHodLod 2.37.1 (HodLodTop 1.1.0) box top: the SIGNAL line (line 1 inside the box) and the left of the Now row (gamma pill + 0DTE expiry), SDK-free
//
// (2.37.0, Rassul 2026-11-19 19:2x CT, approved mockup hodlod-new-box-mockup.html, "ok.. do it"; 19:33 "switch positions of sweeps and Cobs")
//   SIGNAL LINE: the 2.35.0 tags (latest Cob / Sw / BkTst per market) in three groups - Cob LEFT (left-aligned), Sw CENTRE, BkTst RIGHT
//   (right-aligned); newest first in each group; green = bullish, red = bearish, black bold text on the badge.
//   Crowding (19:28 "if there are too many signals you will have to adjust font size"): each group owns a third of the line and may borrow
//   empty width from a neighbour; the steps: the normal font (11 pt) -> smaller, down to 10 pt (his readability floor) -> tighter tag spacing
//   -> only then the OLDEST tag of the most crowded group drops (repeated). Never overlapping, never past the line's ends.
//   NOW ROW LEFT: the gamma pill (+GAMMA / -GAMMA) at the box's left edge, then "0DTE Exp 3:00 PM"; the Now line stays centred and is never
//   covered: when they would touch it, the expiry text goes first, then the pill.
// (1.1.0 = lsHodLod 2.37.1, Rassul 2026-11-19 22:35 CT, his wording, mockup gamma-tag-mockup.html rows "YOUR IDEA, once the forward test
//   proves it", shown WITHOUT "?"): the gamma pill carries a label inside it - "-GAMMA \xB7 Trending \xB7 Bk / BkTst" (red) and
//   "+GAMMA \xB7 Range \xB7 Sw / Cob" (green); "0DTE Exp 3:00 PM" follows as before. Narrow pane (nowLeftTag): the expiry drops first, then
//   " \xB7 Bk / BkTst" / " \xB7 Sw / Cob", then " \xB7 Trending" / " \xB7 Range"; the +/-GAMMA word always stays (unless even it would touch the
//   Now line, as in 1.0.0); the centred Now line is never covered.
#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace htop {

static const char* const VERSION = "1.2.0";   // (1.2.0 = lsHodLod 2.42.0) the two signal rows (rowLayout)   // was "1.1.0";   // was "1.0.0" (lsHodLod 2.37.0)
static const int FS_NORMAL = 11, FS_MIN = 10;

struct Tag { std::string text; int dir = 0; int group = 0; };   // in newest-first order
// the type inside a 2.35.0 badge text: "NQ Sw PHL 1:09" -> "Sw", "ES Cob 3:18" -> "Cob", "ES BkTst PDL 11:39" -> "BkTst"
inline std::string typeOf(const std::string& label) { const size_t a = label.find(' '); if (a == std::string::npos) return std::string(); const size_t b = label.find(' ', a + 1); return label.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1); }
inline int groupOf(const std::string& type) { return type == "Cob" ? 0 : type == "Sw" ? 1 : type == "BkTst" ? 2 : -1; }
// k: 0 fill, 1 text (x = l, row t..b), 3 rect (col = border, fill)
struct It { int k = 0, l = 0, t = 0, r = 0, b = 0; unsigned col = 0, fill = 0; std::string s; int fs = 10; bool bold = true; };
struct Space { int pad, gap, sep; };
static const Space NORMAL = { 4, 6, 14 }, TIGHT = { 3, 3, 8 };

struct Sig { bool ok = false; int fs = 0; bool tight = false; int dropped = 0; int shown[3] = { 0, 0, 0 }; int gl[3] = { 0, 0, 0 }, gr[3] = { 0, 0, 0 }; std::vector<It> items; };

// meas(s, fs, bold) -> px
template <class M> inline Sig sigLayout(const std::vector<Tag>& tags, M meas, int x0, int x1, int top, int bot, int fsNorm = FS_NORMAL, int fsMin = FS_MIN) {
    Sig S; if (x1 - x0 < 20) return S;
    if (fsNorm < fsMin) fsNorm = fsMin;
    std::vector<int> G[3]; for (size_t i = 0; i < tags.size(); i++) if (tags[i].group >= 0 && tags[i].group <= 2) G[tags[i].group].push_back((int)i);
    int n[3] = { (int)G[0].size(), (int)G[1].size(), (int)G[2].size() };
    auto gw = [&](int g, int fs, const Space& sp) { int w = 0; for (int q = 0; q < n[g]; q++) w += meas(tags[(size_t)G[g][(size_t)q]].text, fs, true) + 2 * sp.pad + (q ? sp.gap : 0); return w; };
    // the centre group's left edge, or INT_MIN when it cannot fit (borrowing: it may slide off the middle into free width)
    auto place = [&](int fs, const Space& sp, int& cl) {
        const int wL = gw(0, fs, sp), wC = gw(1, fs, sp), wR = gw(2, fs, sp);
        if (wL + wR + wC > x1 - x0) return false;
        const int lo = x0 + wL + (wL ? sp.sep : 0), hi = x1 - wR - (wR ? sp.sep : 0) - wC;
        if (wC == 0) { cl = lo; return !(wL && wR) || x0 + wL + sp.sep <= x1 - wR; }
        if (lo > hi) return false;
        cl = (std::min)((std::max)((x0 + x1) / 2 - wC / 2, lo), hi); return true;
    };
    int fs = fsNorm, cl = 0; Space sp = NORMAL; bool fit = false;
    auto best = [&]() {   // normal spacing from the normal font down to the floor, then tight spacing at the floor
        for (int f = fsNorm; f >= fsMin; --f) if (place(f, NORMAL, cl)) { fs = f; sp = NORMAL; S.tight = false; return true; }
        if (place(fsMin, TIGHT, cl)) { fs = fsMin; sp = TIGHT; S.tight = true; return true; }
        return false;
    };
    fit = best();
    while (!fit) {   // drop the oldest tag of the most crowded group (the widest at the floor, tight)
        int g = -1, w = -1; for (int k = 0; k < 3; k++) { const int x = n[k] ? gw(k, fsMin, TIGHT) : -1; if (x > w) { w = x; g = k; } }
        if (g < 0 || n[g] == 0) break;
        n[g]--; S.dropped++; fit = place(fsMin, TIGHT, cl);   // once tags drop, the line stays at the floor font with tight spacing (the order: font, spacing, then drop)
        if (fit) { fs = fsMin; sp = TIGHT; S.tight = true; }
    }
    if (!fit) return S;
    S.ok = true; S.fs = fs;
    const int xs[3] = { x0, cl, x1 - gw(2, fs, sp) };
    for (int g = 0; g < 3; g++) {
        int x = xs[g]; S.gl[g] = x; S.shown[g] = n[g];
        for (int q = 0; q < n[g]; q++) { const Tag& t = tags[(size_t)G[g][(size_t)q]]; if (q) x += sp.gap;
            const int tw = meas(t.text, fs, true);
            It F; F.k = 0; F.l = x; F.t = top + 2; F.r = x + tw + 2 * sp.pad; F.b = bot - 2; F.col = t.dir > 0 ? 0x0022C55Eu : 0x00EF4444u; S.items.push_back(F);
            It T; T.k = 1; T.l = x + sp.pad; T.t = top + 1; T.r = T.l + tw; T.b = bot - 2; T.col = 0x00000000u; T.s = t.text; T.fs = fs; T.bold = true; S.items.push_back(T);
            x += tw + 2 * sp.pad; }
        S.gr[g] = x;
    }
    return S;
}

// (1.2.0 = lsHodLod 2.42.0, Rassul 2026-11-20 13:22 CT "two signal rows 1 for cobs and the other for sweeps .. sweep signal row above the cob
// row and make the values look nice for both rows. sweep row should have strength like 99", mockup hodlod-signal-rows-mockup.html, 13:36
// "perfect", 14:13 build): ONE row = a grey head ("SW" / "COB") then chips, newest first: a 3 px bar in the side's colour, a dark tint of it,
// the market (bold white), the time, the level(s), an arrow drawn with lines (no glyph) and the number (sweep strength / Cob ZZO count) in
// black on the side's colour. Crowding: 11 pt -> 10 pt (his floor) -> tight spacing -> the OLDEST chip drops (the right end), repeated.
// Items: k 0 fill, k 1 text, k 4 line (l,t)->(r,b) width fs (2).
struct Chip { std::string mkt, when, lv, num; int dir = 0; };
struct Row { bool ok = false; int fs = 0; bool tight = false; int shown = 0, dropped = 0, r = 0; std::vector<It> items; };
static const unsigned CH_TINT_DN = 0x002A0F12u, CH_TINT_UP = 0x000D2617u, CH_WHITE = 0x00FFFFFFu, CH_TIME = 0x00CBD5E1u, CH_LV = 0x00E5E7EBu, CH_HEAD = 0x008A93A3u;
inline std::string chipLv(const std::string& lv) { std::string o; for (char c : lv) { if (c == '/') o += " \xB7 "; else o += c; } return o; }
inline int arrowW(int fs) { return fs / 2 + 4; }
template <class M> inline int chipW(const Chip& c, M meas, int fs, int pad) {
    const int g = fs >= 11 ? 4 : 3; int w = 3 + pad + meas(c.mkt, fs, true) + g + meas(c.when, fs, false);
    if (!c.lv.empty()) w += g + meas(chipLv(c.lv), fs, false);
    w += g + arrowW(fs);
    if (!c.num.empty()) w += g + meas(c.num, fs, true) + 6;
    return w + pad;
}
template <class M> inline Row rowLayout(const std::vector<Chip>& chips, const std::string& head, M meas, int x0, int x1, int top, int bot, int fsNorm = FS_NORMAL, int fsMin = FS_MIN) {
    Row R; if (x1 - x0 < 40 || bot - top < 8) return R;
    if (fsNorm < fsMin) fsNorm = fsMin;
    const int hfs = fsMin;   // the head: small, grey
    const int headW = head.empty() ? 0 : meas(head, hfs, true) + 8;
    const int c0 = x0 + (headW < 34 ? 34 : headW);
    int n = (int)chips.size();
    auto total = [&](int k, int fs, int pad, int gap) { int w = 0; for (int i = 0; i < k; i++) w += chipW(chips[(size_t)i], meas, fs, pad) + (i ? gap : 0); return w; };
    int fs = fsNorm, pad = 5, gap = 6; bool fit = false;
    for (int f = fsNorm; f >= fsMin && !fit; --f) if (c0 + total(n, f, 5, 6) <= x1) { fs = f; pad = 5; gap = 6; fit = true; }
    if (!fit && c0 + total(n, fsMin, 3, 3) <= x1) { fs = fsMin; pad = 3; gap = 3; fit = true; R.tight = true; }
    while (!fit && n > 0) { --n; R.dropped++; if (c0 + total(n, fsMin, 3, 3) <= x1) { fs = fsMin; pad = 3; gap = 3; fit = true; R.tight = true; } }
    R.ok = true; R.fs = fs; R.shown = n;
    if (headW) { It T; T.k = 1; T.l = x0; T.t = top + 1; T.r = x0 + headW - 8; T.b = bot - 1; T.col = CH_HEAD; T.s = head; T.fs = hfs; T.bold = true; R.items.push_back(T); }
    const int ct = top + 2, cb = bot - 2, g = fs >= 11 ? 4 : 3; int x = c0;
    auto text = [&](int l, const std::string& s, unsigned col, bool b) { const int w = meas(s, fs, b); It T; T.k = 1; T.l = l; T.t = top + 1; T.r = l + w; T.b = bot - 1; T.col = col; T.s = s; T.fs = fs; T.bold = b; R.items.push_back(T); return w; };
    for (int i = 0; i < n; i++) { const Chip& c = chips[(size_t)i]; if (i) x += gap;
        const int w = chipW(c, meas, fs, pad); const unsigned side = c.dir > 0 ? 0x0022C55Eu : 0x00EF4444u;
        It F; F.k = 0; F.l = x; F.t = ct; F.r = x + w; F.b = cb; F.col = c.dir > 0 ? CH_TINT_UP : CH_TINT_DN; R.items.push_back(F);
        It Bar; Bar.k = 0; Bar.l = x; Bar.t = ct; Bar.r = x + 3; Bar.b = cb; Bar.col = side; R.items.push_back(Bar);
        int xx = x + 3 + pad;
        xx += text(xx, c.mkt, CH_WHITE, true) + g;
        xx += text(xx, c.when, CH_TIME, false);
        if (!c.lv.empty()) { xx += g; xx += text(xx, chipLv(c.lv), CH_LV, false); }
        xx += g; { const int aw = arrowW(fs), ax = xx + aw / 2, h = fs - 2, yc = (ct + cb) / 2, ya = yc - h / 2, yb = yc + h / 2;
            const int tip = c.dir > 0 ? ya : yb, d = c.dir > 0 ? 3 : -3;
            It a; a.k = 4; a.l = ax; a.t = ya; a.r = ax; a.b = yb; a.col = side; a.fs = 2; R.items.push_back(a);
            It b1; b1.k = 4; b1.l = ax - 3; b1.t = tip + d; b1.r = ax; b1.b = tip; b1.col = side; b1.fs = 2; R.items.push_back(b1);
            It b2; b2.k = 4; b2.l = ax + 3; b2.t = tip + d; b2.r = ax; b2.b = tip; b2.col = side; b2.fs = 2; R.items.push_back(b2);
            xx += aw; }
        if (!c.num.empty()) { xx += g; const int nw = meas(c.num, fs, true) + 6; It N; N.k = 0; N.l = xx; N.t = ct + 1; N.r = xx + nw; N.b = cb - 1; N.col = side; R.items.push_back(N);
            text(xx + 3, c.num, 0x00000000u, true); xx += nw; }
        x += w; }
    R.r = x; return R;
}

// "Exp 3:00 PM" (the header's short expiry) -> "0DTE Exp 3:00 PM"; empty stays empty
inline std::string zeroDte(const std::string& ex) { return ex.empty() ? ex : "0DTE " + ex; }

// the Now row's left part. xL = the box's left edge; nowL = the Now line's left edge (a large value when there is no Now line); keep: 2 pill +
// expiry, 1 pill only, 0 nothing (it would touch the Now line). pillCol = border / text colour, pillFill = its fill.
struct Left { int keep = 0; int r = 0; std::vector<It> items; int lvl = -1; std::string tag; };   // (1.1.0) lvl = which gammaTag form was drawn (0 full .. 2 word only), -1 none; tag = its text
template <class M> inline Left nowLeft(const std::string& pill, unsigned pillCol, unsigned pillFill, const std::string& exp, unsigned expCol, M meas, int xL, int top, int bot, int nowL, int xMax, int fs = 10) {
    Left o; const int gap = 10;
    const int pillW = pill.empty() ? 0 : meas(pill, fs, true) + 12, expW = exp.empty() ? 0 : meas(exp, fs, false);
    const int r2 = xL + pillW + (pillW && expW ? 8 : 0) + expW, r1 = xL + pillW;
    const int lim = (std::min)(nowL - gap, xMax);
    if (expW && r2 <= lim) o.keep = 2; else if (pillW && r1 <= lim) o.keep = 1; else if (!pillW && expW && r2 <= lim) o.keep = 2; else return o;
    int x = xL;
    if (pillW) { It R; R.k = 3; R.l = x; R.t = top + 1; R.r = x + pillW; R.b = bot - 1; R.col = pillCol; R.fill = pillFill; o.items.push_back(R);
                 It T; T.k = 1; T.l = x + 6; T.t = top + 1; T.r = x + pillW - 6; T.b = bot - 1; T.col = pillCol; T.s = pill; T.fs = fs; T.bold = true; o.items.push_back(T); x += pillW + 8; }
    if (o.keep == 2 && expW) { It T; T.k = 1; T.l = x; T.t = top + 1; T.r = x + expW; T.b = bot - 1; T.col = expCol; T.s = exp; T.fs = fs; T.bold = false; o.items.push_back(T); x += expW; }
    else if (pillW) x -= 8;
    o.r = x; return o;
}

// (1.1.0) the gamma pill's label forms, longest first: full, without the setups, the word only. Any other pill ("GAMMA STALE", "") as is.
inline std::vector<std::string> gammaTag(const std::string& pill) {
    if (pill == "-GAMMA") return { "-GAMMA \xB7 Trending \xB7 Bk / BkTst", "-GAMMA \xB7 Trending", "-GAMMA" };
    if (pill == "+GAMMA") return { "+GAMMA \xB7 Range \xB7 Sw / Cob", "+GAMMA \xB7 Range", "+GAMMA" };
    if (pill.empty()) return {};
    return { pill };
}

// (1.1.0) the Now row's left part with the labelled pill. Drop order when room runs out: the expiry, then the setups part, then the
// regime word part; the +/-GAMMA word stays (it goes only when even it would touch the Now line). forms = gammaTag(pill).
template <class M> inline Left nowLeftTag(const std::vector<std::string>& forms, unsigned pillCol, unsigned pillFill, const std::string& exp, unsigned expCol, M meas, int xL, int top, int bot, int nowL, int xMax, int fs = 10) {
    if (forms.empty()) { Left o = nowLeft(std::string(), pillCol, pillFill, exp, expCol, meas, xL, top, bot, nowL, xMax, fs); return o; }
    if (!exp.empty()) { Left o = nowLeft(forms[0], pillCol, pillFill, exp, expCol, meas, xL, top, bot, nowL, xMax, fs); if (o.keep == 2) { o.lvl = 0; o.tag = forms[0]; return o; } }
    for (size_t i = 0; i < forms.size(); i++) {
        Left o = nowLeft(forms[i], pillCol, pillFill, std::string(), expCol, meas, xL, top, bot, nowL, xMax, fs);
        if (o.keep == 1) { o.lvl = (int)i; o.tag = forms[i]; return o; }
    }
    return Left();
}

} // namespace htop
