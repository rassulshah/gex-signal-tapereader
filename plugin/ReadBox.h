// ReadBox.h - the compact READ BOX, LEFT column (lsHodLod 2.22.0). SDK-free; unit-tested on Linux (tests_readbox).
//
// 1.0.0 (2026-11-08, Rassul's design: claude.ai/artifact/TsFBZ7j3ue4ktc1J2CM2NC; trim per research-artifacts/setups/READ-LINES-CHECK.md)
// Five lines, verdict first, one colour per line, at the top-left of the price chart; the right column's slot stays empty:
//   HTF  Rot Up/Dn, Green/Red, CLBO/CLBD, HH/LL, HEC/MEC/LEC, Sm/Nor/Big Th, Testing/Swept R/S <price>
//        the last COMPLETED 60-min bar, built from the chart's closed bars; Rot = the 1-bar High/Low ZigZag; thrust = this bar's high-to-high (low-to-low)
//        step vs the average of the last 10 positive steps (< 0.5 Sm, <= 1.5 Nor, else Big); Testing / Swept = an intact 60-min pivot
//        (3 back each side, no close beyond since) within 1/4 ATR(60m, 14). Green = Rot Up + CLBO, red = Rot Dn + CLBD, else white.
//   Day  RTH: "Day: Big 72%, 1.4x ADR, Used 74%, -> PB Revs, Hold CoB"   (grey = small, white = normal, blue = big)
//        overnight / pre-open: "Day: Overnight, ON 0.4x ADR[, Big? 35%]" - never the prior RTH's text.
//   Lvl  "Lvl: Testing S LonLO 7210 (1st) + PS0 7200, up R PDH 7460 (+0.3 ATR), Swept ONH"
//        key + volume levels (MenthorQ only as the "+" confluence within 1/4 ATR); green = testing / swept S in Rot Up, red = R in Rot Dn.
//   Mkt  verdict + Init / Trapped only (lsDeltaProfile's DECIDED I / T in the last 5 closed bars): "Buyers Pushing / B Initiating 4.4x
//        @4098.1" (green), "Sellers Failing / S Trapped" (green: the failing side takes the other side's colour), else "Mkt: Even" (white).
//   Lmt  description only (lsDeltaProfile's decided A in the last 5 closed bars): "Lmt: Buyers Absorbing 41 (4.4x) @4098.1"; none = "Lmt: -".
// Built from CLOSED bars and DECIDED signals only, so a line never changes for a bar that is already closed (no repaint).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace rbx {

static const char* const VERSION = "1.0.0";
enum Col { WHITE = 0, GREEN = 1, RED = 2, GREY = 3, BLUE = 4 };
struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0; };   // t = bar END, CT wall-clock seconds
struct Level { std::string name; double px = NAN; };
struct Sig { long long t = 0; std::string code; int bias = 0; double px = NAN, net = NAN, mult = NAN; };   // a DECIDED lsDeltaProfile event
struct DayIn { int kind = -1; double pBig = NAN, pSmall = NAN, xr = NAN; bool post = false; double adr = NAN; };
struct In {
    std::string mkt; int dec = 2; long long per = 180; int openMin = 510, closeMin = 900;
    std::vector<Bar> bars;          // closed bars, oldest first (2+ sessions for the 60-min history)
    std::vector<Level> levels;      // key + volume levels (his names)
    std::vector<Level> mq;          // MenthorQ levels (confluence only)
    std::vector<Sig> sigs;          // lsDeltaProfile decided events
    DayIn day;
};
struct Line { std::string text; int col = WHITE; };
struct Out { bool ok = false; Line l[5]; std::string why; long long at = 0; };

// ---------------------------------------------------------------- helpers
inline std::string num(double v, int dec) {
    if (!std::isfinite(v)) return "-";
    char b[48]; std::snprintf(b, sizeof b, "%.*f", dec < 0 ? 0 : (dec > 6 ? 6 : dec), v); std::string s = b;
    if (s.find('.') != std::string::npos) { while (!s.empty() && s.back() == '0') s.pop_back(); if (!s.empty() && s.back() == '.') s.pop_back(); }
    return s;
}
inline std::string x1(double v) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", v); return b; }
inline std::string pct(double p) { char b[16]; std::snprintf(b, sizeof b, "%.0f%%", p * 100.0); return b; }
inline int modOf(long long t) { long long s = t % 86400; if (s < 0) s += 86400; return (int)(s / 60); }
inline long long dayOf(long long t) { return t >= 0 ? t / 86400 : (t - 86399) / 86400; }
// the session a bar END belongs to: bars ending after 17:00 CT belong to the next day's session (key = that day number)
inline long long sessionOf(long long tEnd) { const long long s = tEnd - 1; return dayOf(s) + (modOf(s) >= 17 * 60 ? 1 : 0); }
inline long long sessionStart(long long key) { return (key - 1) * 86400 + 17 * 3600; }   // 17:00 CT the day before

// 60-min bars from the chart's closed bars (bucket = the hour the bar STARTS in); the last one may be the hour in progress
inline std::vector<Bar> hourly(const std::vector<Bar>& B, long long per) {
    std::vector<Bar> H; long long cur = -1;
    for (const Bar& b : B) {
        const long long k = (b.t - per) / 3600;
        if (k != cur) { Bar h = b; h.t = (k + 1) * 3600; H.push_back(h); cur = k; }
        else { Bar& h = H.back(); h.h = std::max(h.h, b.h); h.l = std::min(h.l, b.l); h.c = b.c; }
    }
    return H;
}
struct Piv { char kind; int i; double px; };
// the 1-bar High/Low ZigZag (his 60-min chart setting): dir +1 / -1 now, the confirmed pivots, the current leg's extreme index
inline void zigzag(const std::vector<Bar>& B, std::vector<Piv>& piv, int& dir, int& ext) {
    piv.clear(); dir = 1; ext = 0;
    for (int i = 1; i < (int)B.size(); i++) {
        if (dir == 1) { if (B[i].h >= B[ext].h) ext = i; else if (B[i].l < B[i - 1].l) { piv.push_back({ 'H', ext, B[ext].h }); dir = -1; ext = i; } }
        else { if (B[i].l <= B[ext].l) ext = i; else if (B[i].h > B[i - 1].h) { piv.push_back({ 'L', ext, B[ext].l }); dir = 1; ext = i; } }
    }
}
inline bool isHigh(const std::string& n) {
    static const char* const H[] = { "ONH", "PDH", "PFH", "LonHI", "PhrH", "PHH", "AsiaHI", "VAH" };
    for (const char* h : H) if (n == h) return true;
    return false;
}
inline bool isLow(const std::string& n) {
    static const char* const L[] = { "ONL", "PDL", "PFL", "LonLO", "PhrL", "PHL", "AsiaLO", "VAL" };
    for (const char* h : L) if (n == h) return true;
    return false;
}

// ---------------------------------------------------------------- the lines
struct Htf { bool ok = false; Line line; int dir = 0; double atr = NAN; };
inline Htf htfLine(const In& I) {
    Htf o; std::vector<Bar> B = hourly(I.bars, I.per);
    if (!B.empty() && !I.bars.empty() && B.back().t > I.bars.back().t) B.pop_back();   // the hour in progress is not read: the last COMPLETED hour (changes once an hour)
    if (B.size() < 16) { o.line.text = "HTF: -"; o.line.col = WHITE; return o; }
    std::vector<Piv> piv; int dir = 1, ext = 0; zigzag(B, piv, dir, ext);
    const int i = (int)B.size() - 1; const Bar& cur = B[i]; const Bar& prv = B[i - 1];
    std::vector<std::string> P;
    P.push_back(dir == 1 ? "Rot Up" : "Rot Dn");
    P.push_back(cur.c >= cur.o ? "Green" : "Red");
    const bool clbo = cur.c > prv.h, clbd = cur.c < prv.l;
    if (clbo) P.push_back("CLBO");
    if (clbd) P.push_back("CLBD");
    const bool hh = cur.h > prv.h, ll = cur.l < prv.l;
    if (dir == 1 && hh) P.push_back("HH"); else if (dir == -1 && ll) P.push_back("LL"); else if (hh) P.push_back("HH"); else if (ll) P.push_back("LL");
    const double rg = cur.h - cur.l, pos = rg > 0 ? (cur.c - cur.l) / rg : 0.5;
    P.push_back(pos >= 2.0 / 3 ? "HEC" : (pos <= 1.0 / 3 ? "LEC" : "MEC"));
    std::vector<double> steps;
    for (int j = 1; j < i; j++) { const double s = dir == 1 ? B[j].h - B[j - 1].h : B[j - 1].l - B[j].l; if (s > 0) steps.push_back(s); }
    const double cs = dir == 1 ? cur.h - prv.h : prv.l - cur.l;
    if (cs > 0 && !steps.empty()) {
        const size_t n = std::min<size_t>(10, steps.size()); double a = 0; for (size_t k = steps.size() - n; k < steps.size(); k++) a += steps[k]; a /= (double)n;
        const double x = a > 0 ? cs / a : 0; P.push_back(x < 0.5 ? "Sm Th" : (x <= 1.5 ? "Nor Th" : "Big Th"));
    }
    double atr = 0; int na = 0;
    for (int j = std::max(1, i - 13); j <= i; j++) { atr += std::max(B[j].h, B[j - 1].c) - std::min(B[j].l, B[j - 1].c); na++; }
    atr = na ? atr / na : NAN; o.atr = atr;
    // an intact pivot (no CLOSE beyond it since it formed), up to 3 back each side, within 1/4 ATR -> Testing; traded through -> Swept
    auto intact = [&](const Piv& p, bool r) { for (int j = p.i + 1; j < i; j++) if (r ? B[j].c > p.px : B[j].c < p.px) return false; return true; };
    std::string best;   // the research order: the last 3 up pivots (R) first, then the last 3 down pivots (S); the first that qualifies
    for (int side = 0; side < 2 && best.empty(); side++) {
        int n = 0;
        for (int k = (int)piv.size() - 1; k >= 0 && n < 3 && best.empty(); k--) {
            const Piv& p = piv[k]; const bool r = p.kind == 'H'; if (r != (side == 0)) continue;
            n++;
            if (!intact(p, r)) continue;
            if (r) { if (cur.h > p.px) best = "Swept R " + num(p.px, I.dec); else if (p.px - cur.h <= atr / 4) best = "Testing R " + num(p.px, I.dec); }
            else { if (cur.l < p.px) best = "Swept S " + num(p.px, I.dec); else if (cur.l - p.px <= atr / 4) best = "Testing S " + num(p.px, I.dec); }
        }
    }
    if (!best.empty()) P.push_back(best);
    std::string t = "HTF: "; for (size_t k = 0; k < P.size(); k++) { if (k) t += ", "; t += P[k]; }
    o.line.text = t; o.line.col = (dir == 1 && clbo) ? GREEN : ((dir == -1 && clbd) ? RED : WHITE); o.dir = dir; o.ok = true;
    return o;
}

inline Line dayLine(const In& I, long long now) {
    Line L; const long long key = sessionOf(now), s0 = sessionStart(key);
    const long long nd = dayOf(now - 1); const int m = modOf(now - 1);
    const bool rth = nd == key && m >= I.openMin && m < I.closeMin;
    const bool afterClose = nd == key && m >= I.closeMin;
    double hi = -1e300, lo = 1e300, rhi = -1e300, rlo = 1e300;
    for (const Bar& b : I.bars) {
        if (b.t <= s0 || b.t > now) continue;
        hi = std::max(hi, b.h); lo = std::min(lo, b.l);
        const int bm = modOf(b.t - I.per);
        if (dayOf(b.t - 1) == key && bm >= I.openMin && bm < I.closeMin) { rhi = std::max(rhi, b.h); rlo = std::min(rlo, b.l); }
    }
    const double adr = I.day.adr;
    if (!rth && !afterClose) {   // overnight / pre-open: the overnight range only (never the prior RTH's read)
        std::string t = "Day: Overnight";
        if (hi > lo && std::isfinite(adr) && adr > 0) t += ", ON " + x1((hi - lo) / adr) + " ADR";
        if (std::isfinite(I.day.pBig)) t += ", Big? " + pct(I.day.pBig);
        L.text = t; L.col = WHITE; return L;
    }
    std::string kind = "Nor"; double p = NAN; L.col = WHITE;
    if (I.day.kind == 1) { kind = "Big"; p = I.day.pBig; L.col = BLUE; }
    else if (I.day.kind == 2) { kind = "Sm"; p = I.day.pSmall; L.col = GREY; }
    else if (I.day.kind == 0 && std::isfinite(I.day.pBig) && std::isfinite(I.day.pSmall)) p = 1.0 - I.day.pBig - I.day.pSmall;
    std::string t = I.day.kind < 0 ? std::string("Day: -") : "Day: " + kind + (std::isfinite(p) ? " " + pct(p) : std::string());
    const double rr = rhi > rlo ? rhi - rlo : NAN;
    if (std::isfinite(rr) && std::isfinite(adr) && adr > 0) {
        t += ", " + x1(rr / adr) + " ADR";
        if (std::isfinite(I.day.xr) && I.day.xr > 0) { char b[24]; std::snprintf(b, sizeof b, ", Used %.0f%%", 100.0 * rr / (I.day.xr * adr)); t += b; }
    }
    if (afterClose) t += ", closed";
    else if (I.day.kind == 1) t += ", \xE2\x86\x92 PB Revs, Hold CoB";
    else if (I.day.kind >= 0) t += ", \xE2\x86\x92 CoB 1.33";
    L.text = t; return L;
}

inline Line lvlLine(const In& I, long long now, int dir, double atr) {
    Line L; L.text = "Lvl: -"; L.col = WHITE;
    std::vector<Bar> B; for (const Bar& b : I.bars) if (b.t <= now) B.push_back(b);
    if (B.size() < 3 || !std::isfinite(atr) || atr <= 0) return L;
    const Bar& cur = B.back(); const Bar& prv = B[B.size() - 2]; const double q = atr / 4, px = cur.c;
    std::vector<Level> lv;   // same-price levels merged ("PFL/PDL")
    for (const Level& a : I.levels) {
        if (!std::isfinite(a.px)) continue;
        bool m = false;
        for (Level& x : lv) if (std::fabs(x.px - a.px) < 1e-9) { if (x.name.find(a.name) == std::string::npos) x.name += "/" + a.name; m = true; }
        if (!m) lv.push_back(a);
    }
    std::sort(lv.begin(), lv.end(), [](const Level& a, const Level& b) { return a.px < b.px; });
    struct C { std::string what; Level lv; double d = 0; bool sup = false; } best; bool have = false;
    for (const Level& a : lv) {
        if (prv.c > a.px && cur.l <= a.px + q) { C c; c.what = cur.l < a.px ? "Swept S" : "Testing S"; c.lv = a; c.d = std::fabs(px - a.px); c.sup = true; if (!have || c.d < best.d) { best = c; have = true; } }
        if (prv.c < a.px && cur.h >= a.px - q) { C c; c.what = cur.h > a.px ? "Swept R" : "Testing R"; c.lv = a; c.d = std::fabs(px - a.px); c.sup = false; if (!have || c.d < best.d) { best = c; have = true; } }
    }
    // "today": from the RTH open once it has passed, else from the 17:00 session start (overnight)
    const long long key = sessionOf(now); long long s0 = sessionStart(key);
    if (dayOf(now - 1) == key && modOf(now - 1) >= I.openMin) s0 = key * 86400 + (long long)I.openMin * 60;
    std::vector<std::string> P;
    if (have) {
        int cnt = 0; bool away = true;
        for (const Bar& b : B) { if (b.t <= s0) continue; const bool near = b.l <= best.lv.px + q && b.h >= best.lv.px - q; if (near && away) { cnt++; away = false; } if (std::fabs(b.c - best.lv.px) > 2 * q) away = true; }
        static const char* const ORD[] = { "", "1st", "2nd", "3rd" };
        const std::string o = cnt >= 1 && cnt <= 3 ? ORD[cnt] : std::to_string(cnt) + "th";
        std::string s = best.what + " " + best.lv.name + " " + num(best.lv.px, I.dec) + " (" + o + ")";
        const Level* mq = nullptr;
        for (const Level& m : I.mq) if (std::isfinite(m.px) && std::fabs(m.px - best.lv.px) <= q && (!mq || std::fabs(m.px - best.lv.px) < std::fabs(mq->px - best.lv.px))) mq = &m;
        if (mq) s += " + " + mq->name + " " + num(mq->px, I.dec);
        P.push_back(s);
        if (best.sup && dir == 1) L.col = GREEN;
        if (!best.sup && dir == -1) L.col = RED;
    }
    double ref30 = NAN; for (const Bar& b : B) if (b.t <= now - 1800) ref30 = b.c;   // the direction of travel: the close vs 30 min ago
    const bool up = std::isfinite(ref30) ? px >= ref30 : dir == 1; const double ref = have ? best.lv.px : px;
    const Level* nx = nullptr;
    for (const Level& a : lv) if (up ? a.px > ref + q : a.px < ref - q) { if (!nx || (up ? a.px < nx->px : a.px > nx->px)) nx = &a; }
    if (nx) { char b[40]; std::snprintf(b, sizeof b, " (%+.1f ATR)", (nx->px - px) / atr); P.push_back(std::string(up ? "\xE2\x86\x91 R " : "\xE2\x86\x93 S ") + nx->name + " " + num(nx->px, I.dec) + b); }
    std::vector<std::string> sw; std::vector<const Bar*> seg; for (const Bar& b : B) if (b.t > s0) seg.push_back(&b);
    if (!seg.empty()) {
        double mx = -1e300, mn = 1e300; for (const Bar* b : seg) { mx = std::max(mx, b->h); mn = std::min(mn, b->l); }
        for (const Level& a : lv) {
            if ((have && a.name == best.lv.name) || (nx && a.name == nx->name)) continue;
            bool hiT = true, loT = true; size_t p0 = 0;
            while (p0 <= a.name.size()) { size_t p1 = a.name.find('/', p0); if (p1 == std::string::npos) p1 = a.name.size(); const std::string n = a.name.substr(p0, p1 - p0); if (!isHigh(n)) hiT = false; if (!isLow(n)) loT = false; p0 = p1 + 1; }
            if (hiT && mx > a.px && seg.front()->o < a.px) sw.push_back(a.name);
            if (loT && mn < a.px && seg.front()->o > a.px) sw.push_back(a.name);
        }
    }
    if (!sw.empty()) { std::string s = "Swept"; for (const std::string& x : sw) s += " " + x; P.push_back(s); }
    if (P.empty()) return L;
    std::string t = "Lvl: "; for (size_t k = 0; k < P.size(); k++) { if (k) t += ", "; t += P[k]; }
    L.text = t; return L;
}

inline Line mktLine(const In& I, long long now) {
    Line L; L.text = "Mkt: Even"; L.col = WHITE;
    const long long from = now - 5 * I.per; const Sig* init = nullptr; const Sig* trap = nullptr;
    for (const Sig& s : I.sigs) { if (s.t <= from || s.t > now) continue; if (s.code == "I") init = &s; if (s.code == "T") trap = &s; }
    if (init) {
        const bool b = init->bias > 0;
        L.text = std::string("Mkt: ") + (b ? "Buyers Pushing / B Initiating" : "Sellers Pushing / S Initiating") + (std::isfinite(init->mult) && init->mult > 0 ? " " + x1(init->mult) : std::string()) + " @" + num(init->px, I.dec);
        if (trap) L.text += std::string(", ") + (trap->bias > 0 ? "S Trapped" : "B Trapped");
        L.col = b ? GREEN : RED;
    } else if (trap) {
        const bool b = trap->bias > 0;   // a bullish T = sellers trapped -> sellers failing, drawn in the buyers' colour
        L.text = std::string("Mkt: ") + (b ? "Sellers Failing / S Trapped" : "Buyers Failing / B Trapped") + " @" + num(trap->px, I.dec);
        L.col = b ? GREEN : RED;
    }
    return L;
}

inline Line lmtLine(const In& I, long long now) {
    Line L; L.text = "Lmt: -"; L.col = WHITE;
    const long long from = now - 5 * I.per; const Sig* a = nullptr;
    for (const Sig& s : I.sigs) { if (s.t <= from || s.t > now) continue; if (s.code == "A" || s.code == "AL") a = &s; }
    if (!a) return L;
    const bool b = a->bias > 0;
    L.text = std::string("Lmt: ") + (b ? "Buyers Absorbing" : "Sellers Absorbing") + (std::isfinite(a->net) && a->net != 0 ? " " + num(std::fabs(a->net), 0) : std::string())
           + (std::isfinite(a->mult) && a->mult > 0 ? " (" + x1(a->mult) + ")" : std::string()) + " @" + num(a->px, I.dec);
    L.col = b ? GREEN : RED;
    return L;
}

inline Out build(const In& I) {
    Out o; if (I.bars.size() < 3) { o.why = "no bars"; return o; }
    const long long now = I.bars.back().t; o.at = now;
    const Htf h = htfLine(I);
    o.l[0] = h.line; o.l[1] = dayLine(I, now); o.l[2] = lvlLine(I, now, h.dir, h.atr); o.l[3] = mktLine(I, now); o.l[4] = lmtLine(I, now);
    o.ok = true; o.why = "ok"; return o;
}

// lsDeltaProfile's signals record (lsFlexLevels\DeltaProfile\<MKT>-<SYM>-<spb>-signals-<day>.csv), '|' fields, DECIDED rows (event D):
// t_first|t_decided|code|side|bias|zone_lo|zone_hi|tick|net_delta|multiple|decided_by_t|version|event|root|id|peak_t|peak_raw|price|...
inline std::vector<Sig> parseSigs(const std::string& text) {
    std::vector<Sig> v; size_t a = 0;
    while (a < text.size()) {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        std::string ln = text.substr(a, b - a); a = b + 1; if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] < '0' || ln[0] > '9') continue;
        std::vector<std::string> f; size_t p0 = 0;
        for (;;) { size_t p1 = ln.find('|', p0); f.push_back(ln.substr(p0, p1 == std::string::npos ? std::string::npos : p1 - p0)); if (p1 == std::string::npos) break; p0 = p1 + 1; }
        if (f.size() < 18 || f[12] != "D") continue;
        Sig s; s.t = std::atoll(f[1].c_str()); s.code = f[2]; s.bias = f[4] == "bullish" ? 1 : (f[4] == "bearish" ? -1 : 0);
        s.net = std::atof(f[8].c_str()); s.mult = std::atof(f[9].c_str()); s.px = std::atof(f[17].c_str());
        if (s.t > 0 && s.bias != 0 && std::isfinite(s.px)) v.push_back(s);
    }
    return v;
}

// fit a line to a width: cut whole ", " parts from the END first (the verdict stays first), then "..."
template <class Meas> std::string fit(const std::string& s, int w, Meas meas) {
    if (meas(s) <= w) return s;
    std::string t = s;
    for (;;) { const size_t p = t.rfind(", "); if (p == std::string::npos) break; t = t.substr(0, p); if (meas(t) <= w) return t; }
    while (t.size() > 4 && meas(t + "...") > w) t.pop_back();
    return t + "...";
}

// the box: 5 lines in the LEFT half of the price area (the right half is the right column's slot, left empty), at the top
struct Rect { int l = 0, t = 0, r = 0, b = 0; };
struct Layout { bool ok = false; Rect box, slot; Rect line[5]; int fs = 10, lh = 14; };
inline Layout layout(int left, int top, int right, int bottom, int fs = 10) {
    Layout L; L.fs = fs; L.lh = fs + 4; const int pad = 4, h = 5 * L.lh + 2 * pad, w = right - left;
    if (w < 300 || top + h > bottom - 40) return L;
    const int mid = left + w / 2;
    L.box = Rect{ left, top, mid - 4, top + h }; L.slot = Rect{ mid + 4, top, right, top + h };
    for (int k = 0; k < 5; k++) L.line[k] = Rect{ left + 6, top + pad + k * L.lh, mid - 10, top + pad + (k + 1) * L.lh };
    L.ok = true; return L;
}

} // namespace rbx
