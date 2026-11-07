// ChartRead.h - the 3-line CHART READ at the top of the price chart (lsHodLod 2.20.0). SDK-free; unit-tested on Linux.
//
// 1.0.0 (2026-11-07, Rassul 11:52 CT "there is a read on top of each chart, that is where i need your guidance explained to me
// depending on the situation the market is in, you will provide it to me there and can grade yourself"; 12:50 "it cant be more than 3
// lines"; 12:51 "build it" - mockup v2, GC 2026-11-06 8:03 AM):
//   L1  "<MKT> <time>  ·  <situation>:  <what happened>"                        (white)
//   L2  the action: entry (the Cob leg's 50%) -> target (1.33) · cancel · size  (green buy / red sell), or "Wait: <why>" (grey)
//   L3  "Odds: <MKT> Cob X% (n N)  ·  big day Y%  ·  my <MKT> read grade: ..."   (grey)
// Situations: Pre-open, Opening (first 30 min), LOD / HOD candidate (a Cob from the day's extreme after a key-level sweep), Cob live
// (a Cob off another swing), Cob done (target hit / cancelled), Trend day? (first 30 min + big-day odds >= 40%), 1st-extreme candidate
// without a Cob, No setup. After 11:00 AM CT no new trades (his rule). Built from CLOSED bars only (no repaint); the host logs each new
// read (append-only) and the nightly lra.read_grade grades it.
// Inputs: the chart's closed bars, HodLod's day view (the 1st extreme side / price / time, the swept levels, the overnight high / low,
// the ADR), the drawn CoBs from lsCobSignal (lsFlexLevels\CobSignal-<MKT>-<spb>-live.csv + the ZZO lines of its record) and the nightly
// odds lsFlexLevels\LRA-ReadParams-<MKT>.csv (missing -> the line says "-", never an invented number).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace crd {

static const char* const VERSION = "1.0.0";
static const int CUTOFF_MIN = 11 * 60;          // his rule: no new trades after 11:00 AM CT
static const double TARGET_EXT = 1.33;          // his CoB win rule (1.33 extension of the leg)
static const double HOLD_BIG = 0.40;            // hold for 1.618 only when the big-day odds are >= 40%
enum Tone { GREY = 0, GREEN = 1, RED = 2 };

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0; };   // t = bar END, local CT wall-clock seconds since 1970-01-01

struct Cob {
    long long t = 0;            // the CoB bar (END)
    int side = 0;               // +1 bull, -1 bear
    std::string label = "Cob", levels;   // levels = lsCobSignal's "LonLO/AsiaLO/PHL"
    double origin = NAN;        // the swept extreme (the leg's start)
    long long legStartT = 0, legEndT = 0;
    double top = NAN;           // the leg's end pivot (high for bull, low for bear), from the chart's bars
    int zzo = -1;               // the ZZO wave count (-1 = unknown)
};

struct Params {
    bool ok = false; std::string why = "no file";
    double cobWin = NAN; int cobN = 0;          // CoB win % on unseen sessions (lra.skill_jumpstart hold-out)
    double cut1 = NAN, cut2 = NAN;              // overnight range / ADR tercile cuts
    double big[3] = { NAN, NAN, NAN };          // P(big day > 1.2 ADR) after a bottom / middle / top tercile overnight range
    double agree30 = NAN;                       // P(the close agrees with the first 30 minutes' direction)
    std::string grade;                          // "65% right · +0.31R · last 20" (empty = not graded yet)
};

struct In {
    std::string mkt; int openMin = 510; int dec = 2; long long per = 180;
    std::vector<Bar> bars;                      // closed bars, oldest first (today's session at least)
    double adr = NAN; bool onOk = false; double onHi = NAN, onLo = NAN;
    int side = 0;                               // HodLod view: 0 not clear, 1 LOD first, 2 HOD first
    double lod = NAN, hod = NAN; int lodMod = -1, hodMod = -1;
    std::string sweptLod, sweptHod;
    std::vector<Cob> cobs; Params P;
};

struct Read {
    bool ok = false; std::string sit, l1, l2, l3; int tone = GREY; int side = 0;
    double entry = NAN, target = NAN, cancel = NAN; long long at = 0;
};

// ---- small helpers
inline std::string num(double v, int dec) { if (!std::isfinite(v)) return "-"; char b[48]; std::snprintf(b, sizeof b, "%.*f", dec < 0 ? 0 : (dec > 6 ? 6 : dec), v); return b; }
inline std::string pct(double p) { if (!std::isfinite(p)) return "-"; char b[16]; std::snprintf(b, sizeof b, "%.0f%%", p * 100.0); return b; }
inline int modOf(long long t) { long long s = t % 86400; if (s < 0) s += 86400; return (int)(s / 60); }
inline long long dayOf(long long t) { return t >= 0 ? t / 86400 : (t - 86399) / 86400; }
inline std::string hm12(long long t)
{
    int m = modOf(t), h = m / 60, mi = m % 60; const char* ap = h >= 12 ? "PM" : "AM"; int h12 = h % 12; if (h12 == 0) h12 = 12;
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", h12, mi, ap); return b;
}
inline std::string hm12short(long long t) { std::string s = hm12(t); return s.substr(0, s.size() - 3); }   // "7:33"
inline std::string levelName(const std::string& code)
{
    static const char* const M[][2] = { { "PHL", "PhrL" }, { "PHH", "PhrH" }, { "LLO", "LonLO" }, { "LHI", "LonHI" }, { "ALO", "AsiaLO" }, { "AHI", "AsiaHI" } };
    for (const auto& m : M) if (code == m[0]) return m[1];
    return code;
}
inline std::vector<std::string> splitLevels(const std::string& s)
{
    std::vector<std::string> out; std::string cur;
    for (char ch : s) { if (ch == '/' || ch == '|' || ch == '+' || ch == ';' || ch == ' ') { if (!cur.empty()) out.push_back(levelName(cur)); cur.clear(); } else cur += ch; }
    if (!cur.empty()) out.push_back(levelName(cur));
    return out;
}
inline std::string joinLevels(const std::vector<std::string>& v) { std::string s; for (size_t i = 0; i < v.size(); i++) { if (i) s += " / "; s += v[i]; } return s; }
inline std::vector<std::string> splitCsv(const std::string& line)
{
    std::vector<std::string> f; std::string cur; for (char ch : line) { if (ch == ',') { f.push_back(cur); cur.clear(); } else if (ch != '\r') cur += ch; } f.push_back(cur); return f;
}
inline bool toD(const std::string& s, double& v) { if (s.empty() || s.size() > 40) return false; char* e = nullptr; v = std::strtod(s.c_str(), &e); return e && *e == 0 && std::isfinite(v); }
inline bool toLL(const std::string& s, long long& v) { if (s.empty() || s.size() > 20) return false; char* e = nullptr; v = std::strtoll(s.c_str(), &e, 10); return e && *e == 0; }

// ---- files
// LRA-ReadParams-<MKT>.csv (written by lra.read_grade):
//   VERSION,<x> / MARKET,GC / COB,<win 0-1>,<n> / ON_CUTS,<c1>,<c2> / BIG_DAY,<bottom>,<middle>,<top> / AGREE30,<p> / GRADE,<text>
inline Params parseParams(const std::string& text, const std::string& mkt)
{
    Params P; if (text.empty()) { P.why = "no file"; return P; }
    if (text.size() > 64 * 1024) { P.why = "file too big"; return P; }
    bool ver = false, mk = false; size_t a = 0;
    while (a < text.size()) {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        const std::vector<std::string> f = splitCsv(text.substr(a, b - a)); a = b + 1;
        if (f.empty() || f[0].empty() || f[0][0] == '#') continue;
        double x, y, z; long long n;
        if (f[0] == "VERSION") ver = true;
        else if (f[0] == "MARKET") { if (f.size() < 2 || f[1] != mkt) { Params Q; Q.why = "wrong market"; return Q; } mk = true; }
        else if (f[0] == "COB" && f.size() >= 3 && toD(f[1], x) && toLL(f[2], n) && x >= 0 && x <= 1 && n >= 0) { P.cobWin = x; P.cobN = (int)n; }
        else if (f[0] == "ON_CUTS" && f.size() >= 3 && toD(f[1], x) && toD(f[2], y) && x > 0 && y >= x) { P.cut1 = x; P.cut2 = y; }
        else if (f[0] == "BIG_DAY" && f.size() >= 4 && toD(f[1], x) && toD(f[2], y) && toD(f[3], z) && x >= 0 && x <= 1 && y >= 0 && y <= 1 && z >= 0 && z <= 1) { P.big[0] = x; P.big[1] = y; P.big[2] = z; }
        else if (f[0] == "AGREE30" && f.size() >= 2 && toD(f[1], x) && x >= 0 && x <= 1) P.agree30 = x;
        else if (f[0] == "GRADE" && f.size() >= 2) { std::string g; for (size_t i = 1; i < f.size(); i++) { if (i > 1) g += ","; g += f[i]; } if (g.size() <= 80) P.grade = g; }
    }
    if (!ver || !mk) { Params Q; Q.why = !ver ? "no VERSION" : "no MARKET"; return Q; }
    P.ok = true; P.why = "ok"; return P;
}

// lsCobSignal's live file: COB,<t>,<BULL|BEAR>,<label>,<levels>,<dratio>,<ext>,<adr>,<legStartT>,<legEndT>,<rN>,<vN>,<SHOW|LOG>
// and its record's ZZO lines: ZZO,<t>,<BULL|BEAR>,<count>,<OK|NO>,<at>
inline std::vector<Cob> parseCobs(const std::string& live, const std::string& recordTail)
{
    std::vector<Cob> out; size_t a = 0;
    while (a < live.size()) {
        size_t b = live.find('\n', a); if (b == std::string::npos) b = live.size();
        const std::vector<std::string> f = splitCsv(live.substr(a, b - a)); a = b + 1;
        if (f.size() < 13 || f[0] != "COB") continue;
        Cob c; long long t, s0, s1; double ext;
        if (!toLL(f[1], t) || !toD(f[6], ext) || !toLL(f[8], s0) || !toLL(f[9], s1)) continue;
        if (f[2] != "BULL" && f[2] != "BEAR") continue;
        if (f[12] != "SHOW") continue;
        c.t = t; c.side = f[2] == "BULL" ? 1 : -1; c.label = f[3].empty() ? "Cob" : f[3]; c.levels = f[4]; c.origin = ext; c.legStartT = s0; c.legEndT = s1;
        out.push_back(c);
    }
    a = 0;
    while (a < recordTail.size()) {
        size_t b = recordTail.find('\n', a); if (b == std::string::npos) b = recordTail.size();
        const std::vector<std::string> f = splitCsv(recordTail.substr(a, b - a)); a = b + 1;
        if (f.size() < 5 || f[0] != "ZZO") continue;
        long long t, n; if (!toLL(f[1], t) || !toLL(f[3], n)) continue;
        const int side = f[2] == "BULL" ? 1 : (f[2] == "BEAR" ? -1 : 0);
        for (Cob& c : out) if (c.t == t && c.side == side) c.zzo = (int)n;
    }
    std::sort(out.begin(), out.end(), [](const Cob& x, const Cob& y) { return x.t < y.t; });
    return out;
}

// the last <= maxB bytes of a file (whole lines only); "" when missing
inline std::string readTail(const std::string& path, size_t maxB = 64 * 1024)
{
    std::ifstream f(path.c_str(), std::ios::binary); if (!f) return std::string();
    f.seekg(0, std::ios::end); const std::streamoff sz = f.tellg(); if (sz <= 0) return std::string();
    const std::streamoff from = sz > (std::streamoff)maxB ? sz - (std::streamoff)maxB : 0;
    f.seekg(from, std::ios::beg); std::string s((size_t)(sz - from), '\0'); f.read(&s[0], (std::streamsize)s.size());
    s.resize((size_t)f.gcount());
    if (from > 0) { size_t nl = s.find('\n'); s = nl == std::string::npos ? std::string() : s.substr(nl + 1); }
    return s;
}

// ---- the read
inline int tercile(const Params& P, double onR) { if (!std::isfinite(onR) || !std::isfinite(P.cut1) || !std::isfinite(P.cut2)) return -1; return onR < P.cut1 ? 0 : (onR <= P.cut2 ? 1 : 2); }

inline Read build(const In& I)
{
    Read R; if (I.bars.empty()) return R;
    const Bar& last = I.bars.back(); const long long now = last.t, day = dayOf(now); const int nowMod = modOf(now);
    R.at = now; R.ok = true;
    const std::string head = I.mkt + " " + hm12(now) + "  \xB7  ";
    const std::string dot = "  \xB7  ";
    // today's RTH bars (bar END after the open, same calendar day)
    std::vector<const Bar*> rth;
    for (const Bar& b : I.bars) if (dayOf(b.t) == day && modOf(b.t) > I.openMin) rth.push_back(&b);
    // the first 30 minutes: the open of the first RTH bar vs the close of the bar ending at open + 30
    int dir30 = 0; bool have30 = false; double open = NAN;
    if (!rth.empty()) {
        open = rth.front()->o; const Bar* b30 = nullptr;
        for (const Bar* b : rth) if (modOf(b->t) >= I.openMin + 30) { b30 = b; break; }   // the first bar ending at / after open + 30 min
        if (b30) { have30 = true; dir30 = b30->c > open ? 1 : (b30->c < open ? -1 : 0); }
    }
    // the overnight range -> big-day odds
    const double onR = (I.onOk && std::isfinite(I.adr) && I.adr > 0) ? (I.onHi - I.onLo) / I.adr : NAN;
    const int ter = tercile(I.P, onR); const double big = ter >= 0 ? I.P.big[ter] : NAN;
    R.l3 = "Odds: " + I.mkt + " Cob " + pct(I.P.cobWin) + (I.P.cobN > 0 ? " (n " + std::to_string(I.P.cobN) + ")" : std::string()) + dot + "big day " + pct(big) + dot +
           "my " + I.mkt + " read grade: " + (I.P.grade.empty() ? std::string("starts after the first graded session") : I.P.grade);
    if (rth.empty()) {
        R.sit = "Pre-open";
        R.l1 = head + "Pre-open:  overnight range " + (std::isfinite(onR) ? num(onR, 2) + " ADR" : std::string("-")) + " -> big day " + pct(big) + ".";
        R.l2 = "Wait for the open (" + hm12(day * 86400 + (long long)I.openMin * 60) + "): the first 30 min sets the day's direction.";
        return R;
    }
    // the latest drawn CoB of today's RTH, at or before now
    const Cob* cob = nullptr;
    for (const Cob& c : I.cobs) if (dayOf(c.t) == day && modOf(c.t) > I.openMin && c.t <= now) cob = &c;
    if (cob) {
        const Cob& c = *cob; double top = c.top;
        if (!std::isfinite(top)) for (const Bar& b : I.bars) if (b.t == c.legEndT) top = c.side > 0 ? b.h : b.l;
        if (std::isfinite(top) && std::isfinite(c.origin) && (c.side > 0 ? top > c.origin : top < c.origin)) {
            const double leg = top - c.origin, entry = c.origin + 0.5 * leg, target = c.origin + TARGET_EXT * leg;
            // after the CoB bar: cancelled (a close beyond the origin) or done (the target reached)?
            bool dead = false, hit = false;
            for (const Bar& b : I.bars) if (b.t > c.t) {
                if (c.side > 0 ? b.c < c.origin : b.c > c.origin) { dead = true; break; }
                if (c.side > 0 ? b.h >= target : b.l <= target) { hit = true; break; }
            }
            const std::vector<std::string> lv = splitLevels(c.levels);
            const bool atExtreme = c.side > 0 ? (std::isfinite(I.lod) && c.origin <= I.lod + 1e-9) : (std::isfinite(I.hod) && c.origin >= I.hod - 1e-9);
            const std::string sit = atExtreme ? (c.side > 0 ? "LOD candidate" : "HOD candidate") : "Cob live";
            const std::string cobTxt = c.label + (c.zzo >= 0 ? " (" + std::to_string(c.zzo) + ")" : std::string());
            const std::string what = std::string(c.side > 0 ? "sellers" : "buyers") + " swept " + (lv.empty() ? std::string("a key level") : joinLevels(lv)) + " to " +
                                     num(c.origin, I.dec) + " at " + hm12short(c.legStartT) + ", " + cobTxt + " at " + hm12short(c.t) + " - " +
                                     (c.side > 0 ? "buyers back above " : "sellers back below ") + (lv.empty() ? std::string("the level") : lv.back()) + ".";
            if (dead || hit) {
                R.sit = "Cob done";
                R.l1 = head + (dead ? "Cob cancelled" : "Cob target hit") + ":  " + what;
                R.l2 = dead ? "Wait: the Cob closed back " + std::string(c.side > 0 ? "below " : "above ") + num(c.origin, I.dec) + " - wait for the next sweep + Cob."
                            : "Done: target " + num(target, I.dec) + " reached - wait for the next sweep + Cob.";
                return R;
            }
            R.sit = sit; R.l1 = head + sit + ":  " + what;
            if (nowMod > CUTOFF_MIN) { R.l2 = "Wait: after 11:00 AM - no new trades (manage what you hold: target " + num(target, I.dec) + ", cancel " + num(c.origin, I.dec) + ")."; return R; }
            R.side = c.side; R.entry = entry; R.target = target; R.cancel = c.origin; R.tone = c.side > 0 ? GREEN : RED;
            std::vector<std::string> why;
            if (have30 && dir30 != 0 && dir30 != c.side) why.push_back(std::string("first 30 min closed ") + (dir30 > 0 ? "up" : "down"));
            const bool holdBig = std::isfinite(big) && big >= HOLD_BIG;
            if (std::isfinite(big) && !holdBig) why.push_back("normal-range day");
            std::string size;
            if (!why.empty()) { size = "size down: "; for (size_t i = 0; i < why.size(); i++) { if (i) size += ", "; size += why[i]; } size += "."; }
            else size = "full size.";
            if (holdBig) size += "  hold for 1.618 (big day " + pct(big) + ").";
            R.l2 = std::string(c.side > 0 ? "Buy" : "Sell") + " the 50% pullback ~" + num(entry, I.dec) + "  ->  target " + num(target, I.dec) + " (1.33)" + dot +
                   "cancel on a close " + (c.side > 0 ? "below " : "above ") + num(c.origin, I.dec) + dot + size;
            return R;
        }
    }
    const int sinceOpen = nowMod - I.openMin;
    if (sinceOpen <= 30) {
        R.sit = "Opening";
        const double mv = last.c - open;
        R.l1 = head + "Opening:  first " + std::to_string(sinceOpen) + " min " + (mv > 0 ? "up " : (mv < 0 ? "down " : "flat ")) + num(std::fabs(mv), I.dec) +
               " from " + num(open, I.dec) + dot + "overnight range " + (std::isfinite(onR) ? num(onR, 2) + " ADR" : std::string("-")) + " -> big day " + pct(big) + ".";
        R.l2 = "Wait: the first 30 min sets the day (the close agrees " + pct(I.P.agree30) + "); the 15-min push is often the fake-out.";
        return R;
    }
    if (nowMod > CUTOFF_MIN) {
        R.sit = "After 11:00";
        R.l1 = head + "After 11:00:  " + (I.side == 1 ? "LOD " + num(I.lod, I.dec) + " came first" : (I.side == 2 ? "HOD " + num(I.hod, I.dec) + " came first" : std::string("no clear 1st extreme"))) +
               (have30 ? std::string(", first 30 min closed ") + (dir30 > 0 ? "up" : (dir30 < 0 ? "down" : "flat")) : std::string()) + ".";
        R.l2 = "Wait: after 11:00 AM - no new trades.";
        return R;
    }
    if (have30 && dir30 != 0 && std::isfinite(big) && big >= HOLD_BIG) {
        R.sit = "Trend day?";
        R.l1 = head + "Trend day?:  first 30 min closed " + (dir30 > 0 ? "up" : "down") + " (the close agrees " + pct(I.P.agree30) + "), big day " + pct(big) + ".";
        R.l2 = std::string("Wait for a pullback Cob on the ") + (dir30 > 0 ? "long" : "short") + " side - buy pullbacks, hold for 1.618.";
        return R;
    }
    if (I.side == 1 || I.side == 2) {
        const bool lodS = I.side == 1; R.sit = lodS ? "LOD candidate" : "HOD candidate";
        const std::string sw = lodS ? I.sweptLod : I.sweptHod; const int m = lodS ? I.lodMod : I.hodMod;
        char tb[16]; tb[0] = 0; if (m >= 0) { int h = m / 60 % 12; if (h == 0) h = 12; std::snprintf(tb, sizeof tb, "%d:%02d", h, m % 60); }
        R.l1 = head + R.sit + ":  " + (lodS ? "low " + num(I.lod, I.dec) : "high " + num(I.hod, I.dec)) + (tb[0] ? std::string(" at ") + tb : std::string()) +
               (sw.empty() ? std::string(", no key level swept") : ", swept " + sw) + ".";
        R.l2 = std::string("Wait for a Cob off the ") + (lodS ? "low" : "high") + " (ZZO 10+) before " + (lodS ? "buying." : "selling.");
        return R;
    }
    R.sit = "No setup";
    R.l1 = head + "No setup:  no key-level sweep with a reaction yet" + (have30 ? std::string(", first 30 min closed ") + (dir30 > 0 ? "up" : (dir30 < 0 ? "down" : "flat")) : std::string()) + ".";
    R.l2 = "Wait: no key-level sweep + Cob yet.";
    return R;
}

// ---- the read log (lsFlexLevels\Reads\<MKT>-<yyyymmdd>.csv): one row per NEW read, append-only
inline std::string csvQ(const std::string& s) { std::string o = "\""; for (char ch : s) { if (ch == '"') o += "\"\""; else if (ch != '\n' && ch != '\r') o += ch; } return o + "\""; }
inline std::string logHeader() { return "time,situation,side,entry,target,cancel,l1,l2,l3,version"; }
inline std::string logRow(const Read& R, int dec, const std::string& plugin)
{
    return hm12(R.at) + "," + csvQ(R.sit) + "," + std::to_string(R.side) + "," + (std::isfinite(R.entry) ? num(R.entry, dec) : std::string()) + "," +
           (std::isfinite(R.target) ? num(R.target, dec) : std::string()) + "," + (std::isfinite(R.cancel) ? num(R.cancel, dec) : std::string()) + "," +
           csvQ(R.l1) + "," + csvQ(R.l2) + "," + csvQ(R.l3) + "," + plugin + "/" + VERSION;
}
inline std::string ymd(long long t)
{   // civil date from days since 1970-01-01 (Howard Hinnant)
    long long z = dayOf(t) + 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = (long long)yoe + era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153, d = doy - (153 * mp + 2) / 5 + 1, m = mp < 10 ? mp + 3 : mp - 9; if (m <= 2) y++;
    char b[40]; std::snprintf(b, sizeof b, "%04lld%02u%02u", y, m, d); return b;
}
// the key that decides "a new read": the action text, not the clock in L1
inline std::string key(const Read& R) { size_t p = R.l1.find("  \xB7  "); return R.sit + "|" + (p == std::string::npos ? R.l1 : R.l1.substr(p)) + "|" + R.l2; }

// fit a line to a pixel width: cut at a word and add "..." (measure = the host's text width)
template <class M> inline std::string fit(const std::string& s, int maxW, M measure)
{
    if (maxW <= 0) return std::string();
    if (measure(s) <= maxW) return s;
    std::string t = s; while (!t.empty() && measure(t + "...") > maxW) { size_t sp = t.find_last_of(' '); t = sp == std::string::npos || sp < t.size() / 2 ? t.substr(0, t.size() - 1) : t.substr(0, sp); }
    while (!t.empty() && (t.back() == ' ' || t.back() == ',')) t.pop_back();
    return t + "...";
}

} // namespace crd
