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
//   Lvl  "Lvl: Testing S LonLO 7210 (1st) + PS0 7200, -> R PDH 7460 (+0.3 ATR), Swept ONH"
//        key + volume levels (MenthorQ only as the "+" confluence within 1/4 ATR); green = testing / swept S in Rot Up, red = R in Rot Dn.
//   Mkt  verdict + Init / Trapped only (lsDeltaProfile's DECIDED I / T in the last 5 closed bars): "Buyers Pushing / B Initiating 4.4x
//        @4098.1" (green), "Sellers Failing / S Trapped" (green: the failing side takes the other side's colour), else "Mkt: Even" (white).
//   Lmt  description only (lsDeltaProfile's decided A in the last 5 closed bars): "Lmt: Buyers Absorbing 41 (4.4x) @4098.1"; none = "Lmt: no absorption".
// Built from CLOSED bars and DECIDED signals only, so a line never changes for a bar that is already closed (no repaint).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace rbx {

static const char* const VERSION = "1.4.0";   // 1.4.0 (2026-11-11, Rassul "Ok build"): a second line "Book:" under "Flow:" - lsBookFlow's defend ratios (SignalBoard fact F) read for the side that matters   // was "1.3.0";   // 1.3.0 (2026-11-11 12:01-12:03 CT): the read box shows the order-flow read only (one "Flow:" line); Out carries trend / day / lvl / atr for the HOD / LOD box
// was "1.2.0";   // 1.2.0 (2026-11-11 Rassul 11:53 CT "1. ok. 2. ok 3. ok, and i dont even think it is working becasue the levels it is mentioning are passed"): Lvl = the nearest level above / below with the distance; Hrly Trend + the hourly breakout level; ONE Flow line (verdict first) from events still in play - 4 lines
// was "1.1.0";   // 1.1.0 (2026-11-11 Rassul 11:41 CT "also updat eht readt at the top to be more simple", his screenshot: the HTF line and "0.8x ADR, Used 91%" crossed out): the HTF line is "Hrly Trend: Up / Down / NA" (11:45 CT); the RTH Day line without the ADR multiple and the Used %
// was "1.0.1";
static const bool SHOW_HTF = true;    // (1.1.0) 11:45 CT "where it says HTF change it to Hrly Trend: and give it values like up or down or NA": the first line is "Hrly Trend: Up / Down / NA" (green / red / white)
static const bool BOX_FLOW_ONLY = true;   // (1.3.0, Rassul 12:01 CT "update the read at the top to just be the MKT LMT or derflow read and keep the Day in the hod lod obx", 12:03 "build")
static const bool DAY_ADR = false;    // (1.1.0) the RTH Day line: no "0.8x ADR, Used 91%"   // 1.0.1: plain ASCII only (the IRT text call is ANSI: "->" not an arrow), no bare "-" lines
enum Col { WHITE = 0, GREEN = 1, RED = 2, GREY = 3, BLUE = 4 };
struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0; };   // t = bar END, CT wall-clock seconds
struct Level { std::string name; double px = NAN; };
struct Sig { long long t = 0; std::string code; int bias = 0; double px = NAN, net = NAN, mult = NAN; };   // a DECIDED lsDeltaProfile event
struct DayIn { int kind = -1; double pBig = NAN, pSmall = NAN, xr = NAN; bool post = false; double adr = NAN; };
// (1.4.0) b / s = the bid / offer Refill Effort ratio signed (+ arriving, - leaving); bits: 1 bids defending, 2 bids pulling, 4 offers defending, 8 offers pulling
struct BookIn { bool ok = false; double b = 0, s = 0; int bits = 0; };
struct In {
    std::string mkt; int dec = 2; long long per = 180; int openMin = 510, closeMin = 900;
    std::vector<Bar> bars;          // closed bars, oldest first (2+ sessions for the 60-min history)
    std::vector<Level> levels;      // key + volume levels (his names)
    std::vector<Level> mq;          // MenthorQ levels (confluence only)
    std::vector<Sig> sigs;          // lsDeltaProfile decided events
    DayIn day;
    BookIn book;                    // (1.4.0) lsBookFlow's newest defend ratios (fact F); ok = false -> no Book line
};
struct Line { std::string text; int col = WHITE; };
struct Out { bool ok = false; Line l[5]; int n = 5; std::string why; long long at = 0;
             Line trend, day, lvl; double atr = NAN, px = NAN;
             int bkTrend = 0, bkSide = 0, bkBase = 1; bool bkPb = false; std::string bkAt; };   // (1.4.0) what the Book line needs (applyBook re-reads the book between bar closes)   // (1.3.0) the Hrly Trend / Day / Lvl reads (shown in the HOD / LOD box, not in the read box) and the 60-min ATR   // (1.1.0) n = the lines shown (4 without the HTF line)

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
struct Htf { bool ok = false; Line line; int dir = 0; double atr = NAN; int trend = 0; int stepDir = 0; double stepPx = NAN; };   // (1.2.0) stepDir / stepPx = the newest hourly breakout step (lsStepTrend's rule) and its level   // (1.1.0) trend = the hourly rotation by the 1-bar rule (lsStepTrend 1.2.0): +1 up, -1 down, 0 none yet
inline Htf htfLine(const In& I) {
    Htf o; std::vector<Bar> B = hourly(I.bars, I.per);
    if (!B.empty() && !I.bars.empty() && B.back().t > I.bars.back().t) B.pop_back();   // the hour in progress is not read: the last COMPLETED hour (changes once an hour)
    // (1.1.0) the hourly trend, lsStepTrend's rule over the COMPLETED hours: a higher high and not a lower low = up, a lower low and not a
    // higher high = down, both = the hour's own colour, neither = unchanged
    for (size_t k = 1; k < B.size(); k++) { const bool hh = B[k].h > B[k - 1].h, ll = B[k].l < B[k - 1].l;
        if (hh && !ll) o.trend = 1; else if (ll && !hh) o.trend = -1; else if (hh && ll) o.trend = B[k].c > B[k].o ? 1 : B[k].c < B[k].o ? -1 : o.trend;
        // (1.2.0) the hourly breakout step: a green hour closing above the prior hour's high while the rotation is up (the mirror for down)
        const bool su = B[k].c > B[k - 1].h && B[k].c > B[k].o && o.trend >= 0, sd = B[k].c < B[k - 1].l && B[k].c < B[k].o && o.trend <= 0;
        if (su != sd) { o.stepDir = su ? 1 : -1; o.stepPx = su ? B[k - 1].h : B[k - 1].l; } }
    if (B.size() < 16) { o.line.text = "HTF: not enough 60-min bars"; o.line.col = WHITE; return o; }
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
    std::string t = I.day.kind < 0 ? std::string("Day: no range-day read") : "Day: " + kind + (std::isfinite(p) ? " " + pct(p) : std::string());
    const double rr = rhi > rlo ? rhi - rlo : NAN;
    if (DAY_ADR && std::isfinite(rr) && std::isfinite(adr) && adr > 0) {
        t += ", " + x1(rr / adr) + " ADR";
        if (std::isfinite(I.day.xr) && I.day.xr > 0) { char b[24]; std::snprintf(b, sizeof b, ", Used %.0f%%", 100.0 * rr / (I.day.xr * adr)); t += b; }
    }
    if (afterClose) t += ", closed";
    else if (I.day.kind == 1) t += ", favours PB Revs, hold CoB";   // (1.3.0) plain words (was "-> PB Revs, Hold CoB" / "-> CoB 1.33")
    else if (I.day.kind >= 0) t += ", favours CoB";
    L.text = t; return L;
}

inline Line lvlLine(const In& I, long long now, int dir, double atr, std::string* atName = nullptr, int* atSide = nullptr) {   // (1.4.0) atName / atSide: the level being tested or swept (+1 from above = support, -1 from below = resistance)
    Line L; L.text = "Lvl: no level near"; L.col = WHITE;
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
    std::string o1;
    if (have) {
        int cnt = 0; bool away = true;
        for (const Bar& b : B) { if (b.t <= s0) continue; const bool near = b.l <= best.lv.px + q && b.h >= best.lv.px - q; if (near && away) { cnt++; away = false; } if (std::fabs(b.c - best.lv.px) > 2 * q) away = true; }
        static const char* const ORD[] = { "", "1st", "2nd", "3rd" };
        const std::string o = cnt >= 1 && cnt <= 3 ? ORD[cnt] : std::to_string(cnt) + "th";
        o1 = o; if (atName) *atName = best.lv.name; if (atSide) *atSide = best.sup ? 1 : -1;
        if (best.sup && dir == 1) L.col = GREEN;
        if (!best.sup && dir == -1) L.col = RED;
    }
    // (1.2.0, Rassul 2026-11-11 11:53 CT "1. ok") always the nearest level ABOVE and BELOW the last close, with the distance in points;
    // the level being tested / swept right now says so instead of its distance. Levels price has left behind are never named.
    const Level* up = nullptr; const Level* dn = nullptr;
    for (const Level& a : lv) { if (a.px > px && (!up || a.px < up->px)) up = &a; if (a.px <= px && (!dn || a.px > dn->px)) dn = &a; }
    auto part = [&](const Level* a, bool above) { if (!a) return std::string(above ? "none above" : "none below");
        std::string w = num(std::fabs(a->px - px), I.dec) + (above ? " above" : " below");
        if (have && std::fabs(best.lv.px - a->px) < 1e-9) w = (best.what.compare(0, 5, "Swept") == 0 ? std::string("swept") : std::string("testing")) + ", " + o1;
        return a->name + " " + num(a->px, I.dec) + " (" + w + ")"; };
    if (!up && !dn) return L;
    L.text = "Lvl: " + part(up, true) + " / " + part(dn, false);
    return L;
}

// (1.2.0, Rassul 11:53 CT "3. ok, and i dont even think it is working becasue the levels it is mentioning are passed") ONE "Flow:" line with
// the verdict first, from lsDeltaProfile's decided events of the last 5 candles that are STILL IN PLAY: the last close is on the event's
// side of its price (a buyers' event: at / above it; a sellers' event: at / below it) and within 1 ATR of it. An event price has gone
// through, or left behind by more than 1 ATR, is not mentioned (1.0.x named it for 5 candles whatever price did).
inline Line flowLine(const In& I, long long now, double atr) {
    Line L; L.text = "Flow: Even"; L.col = WHITE; if (I.bars.empty()) return L;
    double px = NAN; for (const Bar& b : I.bars) if (b.t <= now) px = b.c;
    const long long from = now - 5 * I.per; const Sig* init = nullptr; const Sig* trap = nullptr; const Sig* ab = nullptr;
    auto live = [&](const Sig& s) { if (!std::isfinite(s.px) || !std::isfinite(px)) return false;
        if (s.bias > 0 ? px < s.px : px > s.px) return false;                           // price has gone through it
        return !(std::isfinite(atr) && atr > 0) || std::fabs(px - s.px) <= atr; };      // or has left it behind
    for (const Sig& s : I.sigs) { if (s.t <= from || s.t > now || !live(s)) continue; if (s.code == "I") init = &s; if (s.code == "T") trap = &s; if (s.code == "A" || s.code == "AL") ab = &s; }
    auto side = [](const Sig* s) { return s->bias > 0 ? std::string("Buyers") : std::string("Sellers"); };
    const std::string push = init ? std::string("pushing") + (std::isfinite(init->mult) && init->mult > 0 ? " " + x1(init->mult) : std::string()) : std::string();
    const std::string absb = ab ? std::string("absorbing") + (std::isfinite(ab->net) && ab->net != 0 ? " " + num(std::fabs(ab->net), 0) : std::string())
                                  + (std::isfinite(ab->mult) && ab->mult > 0 ? " (" + x1(ab->mult) + ")" : std::string()) + " @" + num(ab->px, I.dec) : std::string();
    if (init && ab && (init->bias > 0) == (ab->bias > 0)) { L.text = "Flow: " + side(ab) + " in control: " + push + ", " + absb; L.col = ab->bias > 0 ? GREEN : RED; }
    else if (init && ab) { L.text = "Flow: Mixed: " + side(init) + " " + push + ", " + side(ab) + " " + absb; L.col = WHITE; }
    else if (ab) { L.text = "Flow: " + side(ab) + " " + absb; L.col = ab->bias > 0 ? GREEN : RED; }
    else if (init) { L.text = "Flow: " + side(init) + " " + push + " @" + num(init->px, I.dec); L.col = init->bias > 0 ? GREEN : RED; }
    else if (trap) { L.text = std::string("Flow: ") + (trap->bias > 0 ? "Sellers trapped" : "Buyers trapped") + " @" + num(trap->px, I.dec); L.col = trap->bias > 0 ? GREEN : RED; return L; }
    else return L;
    if (trap) L.text += std::string(", ") + (trap->bias > 0 ? "sellers trapped" : "buyers trapped");
    return L;
}

// (1.4.0, Rassul 2026-11-11 "Can you include 4 because I want to use it to enter after a pullback when I am looking for a reversal", "Ok build")
// ONE "Book:" line, the verdict first, for the side that matters: hourly trend Up -> the bids (Down -> the offers); price testing / sweeping a
// key level against the trend -> the side that would make the reversal, with the level named; no trend -> the level's side, else the stronger
// side. "defending" = that side's Refill Effort ratio >= 2.0x with size arriving; "pulling" = size leaving >= 1.5x (lsBookFlow decides, fact F).
inline Line bookLine(const BookIn& k, int trend, const std::string& atName, int atSide, bool pb) {
    Line L; L.text.clear(); L.col = WHITE; if (!k.ok) return L;
    const std::string raw = "(B " + x1(std::fabs(k.b)) + ", S " + x1(std::fabs(k.s)) + ")";
    int side = 0; std::string pre;                                          // side +1 = read the bids, -1 = the offers
    if (atSide != 0 && !atName.empty() && (trend == 0 || atSide == -trend)) { side = atSide; pre = (trend == 0 ? "At " : "Reversal at ") + atName + ": "; }
    else if (trend != 0) { side = trend;
        pre = std::string(pb ? "Pullback in " : "") + (trend > 0 ? (pb ? "uptrend: " : "Uptrend: ") : (pb ? "downtrend: " : "Downtrend: ")); }
    else { const bool bd = (k.bits & 1) != 0, sd = (k.bits & 4) != 0;
        if (bd && (!sd || k.b >= k.s)) side = 1; else if (sd) side = -1;
        else { L.text = "Book: Even " + raw; return L; } }
    const bool def = (k.bits & (side > 0 ? 1 : 4)) != 0, pull = (k.bits & (side > 0 ? 2 : 8)) != 0;
    const char* who = side > 0 ? "Bids" : "Offers";
    if (pull) { L.text = "Book: " + pre + who + " pulling " + x1(std::fabs(side > 0 ? k.b : k.s)); L.col = side > 0 ? RED : GREEN; }
    else if (def) { L.text = "Book: " + pre + who + " defending " + x1(std::fabs(side > 0 ? k.b : k.s)) + " (2.0x)"; L.col = side > 0 ? GREEN : RED; }
    else L.text = "Book: " + pre + who + " not defending " + raw;
    return L;
}

// (1.4.0) puts / refreshes the Book line under the Flow line (the host calls it again each second with the newest fact F): both lines in one
// colour when they agree, the Book line white when they disagree; no book = no line.
inline void applyBook(Out& o, const BookIn& k) {
    if (!o.ok || o.bkBase < 1 || o.bkBase > 4) return;
    Line bk = bookLine(k, o.bkTrend, o.bkAt, o.bkSide, o.bkPb); o.n = o.bkBase;
    if (bk.text.empty()) return;
    const int fc = o.l[o.bkBase - 1].col; if (fc != WHITE && bk.col != WHITE && bk.col != fc) bk.col = WHITE;
    o.l[o.bkBase] = bk; o.n = o.bkBase + 1;
}

inline Out build(const In& I) {
    Out o; if (I.bars.size() < 3) { o.why = "no bars"; return o; }
    const long long now = I.bars.back().t; o.at = now;
    const Htf h = htfLine(I);
    { Line t; t.text = std::string("Hrly Trend: ") + (h.trend > 0 ? "Up" : h.trend < 0 ? "Down" : "NA"); t.col = h.trend > 0 ? GREEN : h.trend < 0 ? RED : WHITE;
        // (1.2.0, Rassul 11:53 CT "2. ok") + the newest hourly breakout level when it goes the trend's way: "Up, above 7607" / "Up, back below 7607"
        if (h.trend != 0 && h.stepDir == h.trend && std::isfinite(h.stepPx)) { const double px = I.bars.back().c; const bool beyond = h.trend > 0 ? px >= h.stepPx : px <= h.stepPx;
            t.text += std::string(", ") + (beyond ? (h.trend > 0 ? "above " : "below ") : (h.trend > 0 ? "back below " : "back above ")) + num(h.stepPx, I.dec); }
        o.trend = t; }
    std::string atName; int atSide = 0;
    o.day = dayLine(I, now); o.lvl = lvlLine(I, now, h.dir, h.atr, &atName, &atSide); o.atr = h.atr; o.px = I.bars.back().c;
    const Line flow = flowLine(I, now, h.atr);
    int k = 0;
    if (BOX_FLOW_ONLY) o.l[k++] = flow;   // (1.3.0) the read box = the order-flow read only
    else { if (SHOW_HTF) o.l[k++] = o.trend;
           o.l[k++] = o.day; o.l[k++] = o.lvl; o.l[k++] = flow; }
    o.n = k;
    o.ok = true; o.why = "ok";
    { const size_t nb = I.bars.size(); o.bkTrend = h.trend; o.bkAt = atName; o.bkSide = atSide; o.bkBase = k;
      o.bkPb = h.trend != 0 && nb >= 2 && (h.trend > 0 ? I.bars[nb - 1].c < I.bars[nb - 2].c : I.bars[nb - 1].c > I.bars[nb - 2].c); }
    if (BOX_FLOW_ONLY) applyBook(o, I.book);
    return o;
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

// the box (1.0.1, Rassul "do not take up extra space"): as wide as the longest line + 6 px each side, 5 lines + 2 px top and bottom,
// at most half the price area wide (longer lines are cut by fit()); no reserved right-column area (the right column will be its own box)
struct Rect { int l = 0, t = 0, r = 0, b = 0; };
struct Layout { bool ok = false; Rect box; Rect line[5]; int fs = 10, lh = 15; };
inline Layout layout(int left, int top, int right, int bottom, int textW, int fs = 10, int rows = 5) {   // (1.1.0) rows = Out::n
    if (rows < 1) rows = 1;
    if (rows > 5) rows = 5;
    Layout L; L.fs = fs; L.lh = fs + 5; const int padX = 6, padY = 2, h = rows * L.lh + 2 * padY, w = right - left;
    if (w < 200 || top + h > bottom - 20) return L;
    const int maxW = w / 2, bw = std::min(maxW, std::max(80, textW + 2 * padX));
    L.box = Rect{ left, top, left + bw, top + h };
    for (int k = 0; k < 5; k++) L.line[k] = Rect{ left + padX, top + padY + k * L.lh, left + bw - padX, top + padY + (k + 1) * L.lh };
    L.ok = true; return L;
}

} // namespace rbx
