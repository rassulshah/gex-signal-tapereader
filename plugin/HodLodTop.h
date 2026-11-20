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

static const char* const VERSION = "1.1.0";   // was "1.0.0" (lsHodLod 2.37.0)
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
