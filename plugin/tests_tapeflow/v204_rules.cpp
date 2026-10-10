// (2.0.4) the node-absorption rules added for 2.0.4, on REAL data where it exists:
//  1. the native 1-bar ZigZag == Rassul's IRT zigzag (zzPrice pivots in <MKT>cobTestData.txt), all 7 markets, >= 98% required
//  2. ES Friday 2026-10-09 (ChartView ES_180 bars, real per-second tape ES-sec-20735): the zigzag pivots at the 10:36 low (7,842.75,
//     known at the 10:39 close) and the 13:54 high (7,870.50, known at 13:57); a bullish A? at 10:36 confirmed A at 10:39, a bearish
//     A? at 13:54 confirmed A at 13:57 (node rows synthetic at the absorbed price - no per-price tape on file yet; the minute
//     volumes and the normal minute are real)
//  3. close beyond the node: inside / at the edge / wrong side -> rejected and logged ("close not beyond node"); one tick beyond -> A?
//  4. the pivot rule: pivot at the confirmation bar -> confirmed one bar later (lateness measured); pivot elsewhere -> "not at pivot"
//  5. a restart after every bar (record reloaded, bars re-fed) == one straight pass (no repaint)
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <cmath>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include "../TapeFlowLogic.h"
using namespace tfl;
static int checks = 0, fails = 0;
#define CHECK(x, msg) do { ++checks; if (!(x)) { ++fails; std::printf("FAIL %d: %s\n", __LINE__, msg); } } while (0)
static long long civil(int y, int mo, int d, int h, int mi, int s)
{
    y -= mo <= 2; long long era = (y >= 0 ? y : y - 399) / 400; unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (unsigned)(mo + (mo > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1; unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60 + s;
}
static long long parseT(const std::string& s)   // "2026-10-09 10:36" or with ":00"
{
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, se = 0; const int r = std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &se); if (r != 3 && r < 5) return 0; return civil(y, mo, d, h, mi, se);   // IRT writes the midnight bar as the date alone
}
static int T(double p, double tick) { return (int)std::llround(p / tick); }
static std::string hm(long long t) { char b[8]; snprintf(b, sizeof(b), "%02d:%02d", (int)(t % 86400 / 3600), (int)(t % 3600 / 60)); return b; }

// ---- 1. the zigzag against the IRT export
static void zigzagVsExport()
{
    const char* mk[] = {"ES", "NQ", "CL", "GC", "HG", "NG", "EU"};
    const double tk[] = {0.25, 0.25, 0.01, 0.1, 0.0005, 0.001, 0.00005};
    std::printf("1. native zigzag vs Rassul's IRT zigzag (zzPrice pivots):\n   mkt  sessions  bars     his pivots  ours    matched  match rate\n");
    int done = 0;
    for (int m = 0; m < 7; ++m) {
        std::ifstream f(std::string("/mnt/user-data/uploads/level-reversal-analytics/data/raw-exports/") + mk[m] + "cobTestData.txt");
        if (!f.good()) continue;
        std::string ln; std::getline(f, ln);
        std::vector<BarIn> bars; std::map<long long, std::pair<int, int> > ref; std::set<long long> sess;
        while (std::getline(f, ln)) {
            std::vector<std::string> c; size_t a = 0; while (true) { size_t p = ln.find(',', a); c.push_back(ln.substr(a, p == std::string::npos ? p : p - a)); if (p == std::string::npos) break; a = p + 1; }
            if (c.size() < 20) continue;
            std::string ds = c[1]; if (ds.size() > 2 && ds[0] == '"') ds = ds.substr(1, ds.size() - 2);
            BarIn b; b.te = parseT(ds); if (!b.te) continue;
            const size_t n = c.size();
            b.o = T(std::atof(c[n - 5].c_str()), tk[m]); b.h = T(std::atof(c[n - 4].c_str()), tk[m]); b.l = T(std::atof(c[n - 3].c_str()), tk[m]); b.c = T(std::atof(c[n - 2].c_str()), tk[m]);
            b.ts = b.te - 180; bars.push_back(b); sess.insert(sessionOf(b.te));
            const double zp = std::atof(c[8].c_str()), zc = std::atof(c[9].c_str());
            if (zp != 0) ref[b.te] = std::make_pair(zc > 0 ? 1 : -1, T(zp, tk[m]));
        }
        if (bars.empty()) continue;
        if (!ref.empty()) ref.erase(std::prev(ref.end()));                // his last pivot is still forming (repaints): not scored
        ZigZag z; size_t ours = 0, matched = 0; const long long lastRef = ref.empty() ? 0 : ref.rbegin()->first;
        for (const BarIn& b : bars) { z.add(b);
            if (!z.piv.empty() && z.piv.back().confT == b.te) {
                const ZigZag::Piv& p = z.piv.back(); if (p.t > lastRef) continue; ours++;
                auto it = ref.find(p.t); if (it != ref.end() && it->second.first == p.side && it->second.second == p.px) matched++;
                else if (getenv("ZZMISS") && m == 0) { static int sh = 0; if (sh++ < 6) std::printf("   miss %lld side %d px %d ref %s\n", p.t, p.side, p.px, it == ref.end() ? "none" : (std::to_string(it->second.first) + " " + std::to_string(it->second.second)).c_str()); } } }
        const double rate = ref.empty() ? 0 : (double)matched / (double)ref.size();
        std::printf("   %s   %5zu   %7zu  %7zu   %7zu  %7zu   %.4f\n", mk[m], sess.size(), bars.size(), ref.size(), ours, matched, rate);
        CHECK(rate >= 0.98 && sess.size() >= 250, "(1) native zigzag matches his IRT zigzag on >= 98% of pivots, 300+ sessions");
        done++;
    }
    if (!done) std::printf("   SKIPPED - the exports are not here\n");
}

// ---- ES Friday: the chart's bars and the per-second tape
struct Friday { std::vector<BarIn> bars; std::vector<SecRec> H; bool ok = false; };
static std::vector<std::string> jsonArr(const std::string& s, const char* key)
{
    std::vector<std::string> v; size_t a = s.find(std::string("\"") + key + "\":["); if (a == std::string::npos) return v;
    a = s.find('[', a) + 1; const size_t e = s.find(']', a); std::string body = s.substr(a, e - a); std::string cur;
    for (char ch : body) { if (ch == ',') { v.push_back(cur); cur.clear(); } else if (ch != '"') cur += ch; } if (!cur.empty()) v.push_back(cur);
    return v;
}
static Friday loadFriday()
{
    Friday F; std::ifstream j("/mnt/user-data/uploads/lsFlexLevels/ChartView/ES_180.json"); std::ifstream s("/mnt/user-data/uploads/lsFlexLevels/TapeFlow/ES-sec-20735.csv");
    if (!j.good() || !s.good()) return F;
    std::stringstream ss; ss << j.rdbuf(); const std::string js = ss.str();
    auto t = jsonArr(js, "t"), o = jsonArr(js, "o"), h = jsonArr(js, "h"), l = jsonArr(js, "l"), c = jsonArr(js, "c");
    for (size_t i = 0; i < t.size() && i < c.size(); ++i) { BarIn b; b.te = parseT(t[i]); b.ts = b.te - 180;
        b.o = T(std::atof(o[i].c_str()), 0.25); b.h = T(std::atof(h[i].c_str()), 0.25); b.l = T(std::atof(l[i].c_str()), 0.25); b.c = T(std::atof(c[i].c_str()), 0.25); F.bars.push_back(b); }
    std::string ln;
    while (std::getline(s, ln)) { if (ln.empty() || !isdigit((unsigned char)ln[0])) continue; std::vector<std::string> x; size_t a = 0;
        while (true) { size_t p = ln.find('|', a); x.push_back(ln.substr(a, p == std::string::npos ? p : p - a)); if (p == std::string::npos) break; a = p + 1; }
        if (x.size() < 10) continue; SecRec r; long long v = 0; if (!parseStoreLong(x[0], &r.t)) continue; r.b = parseStoreLong(x[6], &v) ? v : 0; r.s = parseStoreLong(x[7], &v) ? v : 0; r.u = parseStoreLong(x[8], &v) ? v : 0; r.flags = SR_SIDES;
        if (!F.H.empty() && r.t <= F.H.back().t) continue; F.H.push_back(r); }
    F.ok = !F.bars.empty() && !F.H.empty();
    return F;
}
static const BarIn* barAt(const Friday& F, int h, int m) { const long long t = civil(2026, 10, 9, h, m, 0); for (auto& b : F.bars) if (b.te == t) return &b; return nullptr; }
// a node row at px for bar b: 'side' +1 = buyers lifted it (bearish), -1 = sellers hit it (bullish); placed in the bar's minute with
// the most aggressor contracts of that side in the REAL tape
static void nodeRows(BarRows& R, const Friday& F, const BarIn& b, double px, int side)
{
    int best = 0; long long bv = -1; for (int k = 0; k < 3; ++k) { long long B, S; sideSums(F.H, b.ts + 60 * k, b.ts + 60 * (k + 1), &B, &S); if ((side > 0 ? B : S) > bv) { bv = side > 0 ? B : S; best = k; } }
    auto add = [&](long long t, double p, int sd, long long q) { Tick k; k.t = t; k.px = T(p, 0.25); k.q = q; if (sd > 0) { k.bid = k.px - 1; k.ask = k.px; } else { k.bid = k.px; k.ask = k.px + 1; } R.add(k); };
    add(b.ts + 60 * best + 10, px, side, 600); add(b.ts + 60 * best + 30, px, side, 140);
    add(b.ts + 100, (b.h + b.l) / 2 * 0.25, -side, 40); add(b.ts + 120, b.l * 0.25 + (side > 0 ? 0.0 : 1.0), side, 30); add(b.ts + 150, b.h * 0.25 - 0.5, -side, 25);
}
struct Run { SignalBook bk; std::vector<std::string> lines; };
// feed a day: node bars get their BarFlow from nodeFill + nodeMinuteFill (real minute volumes, normal minute = the last 60 min)
static void feedDay(SignalBook& bk, const Friday& F, const std::vector<BarIn>& bars, const BarRows& R, const std::set<long long>& nodeBars, int nodeN, size_t from = 0, size_t to = SIZE_MAX)
{
    for (size_t i = from; i < bars.size() && i < to; ++i) {
        const BarIn& b = bars[i]; BarFlow fl; fl.secs = 180;
        if (nodeBars.count(b.te)) { nodeFill(fl, R.at(b.te), b, nodeN); nodeMinuteFill(fl, R, b, F.H, recentMinuteNorm(F.H, b.ts)); }
        bk.feed(b, fl);
    }
}
static const Mark* aAt(const SignalBook& bk, long long t) { for (auto& m : bk.marks) if (m.kind == 'A' && m.barT == t) return &m; return nullptr; }
static long rejectedClose(const SignalBook& bk) { auto it = bk.rejected.find("close not beyond node"); return it == bk.rejected.end() ? 0 : it->second; }

static void dbgNode();
int main()
{
    if (getenv("ZZDBG")) { dbgNode(); return 0; }
    zigzagVsExport();
    const Friday F = loadFriday();
    if (!F.ok) { std::printf("2-5. SKIPPED - ES_180.json / ES-sec-20735.csv not here\n%d checks, %d failed\n", checks, fails); return fails ? 1 : 0; }
    const BarIn* b1036 = barAt(F, 10, 36); const BarIn* b1039 = barAt(F, 10, 39); const BarIn* b1354 = barAt(F, 13, 54); const BarIn* b1357 = barAt(F, 13, 57);
    CHECK(b1036 && b1039 && b1354 && b1357, "Friday's bars 10:36 / 10:39 / 13:54 / 13:57 on file");
    if (!b1036 || !b1039 || !b1354 || !b1357) { std::printf("%d checks, %d failed\n", checks, fails); return 1; }
    // ---- 2a. the zigzag pivots on Friday
    {
        ZigZag z; for (auto& b : F.bars) z.add(b);
        const ZigZag::Piv* lo = z.pivotAt(b1036->te, -1); const ZigZag::Piv* hi = z.pivotAt(b1354->te, 1);
        std::printf("2. Friday zigzag: low pivot at 10:36 %s (known at %s), high pivot at 13:54 %s (known at %s)\n",
            lo ? std::to_string(lo->px * 0.25).c_str() : "-", lo ? hm(lo->confT).c_str() : "-", hi ? std::to_string(hi->px * 0.25).c_str() : "-", hi ? hm(hi->confT).c_str() : "-");
        CHECK(lo && lo->px == T(7842.75, 0.25) && lo->confT == b1039->te, "(2) the 10:36 low 7,842.75 is the zigzag pivot, known at the 10:39 close");
        CHECK(hi && hi->px == T(7870.5, 0.25) && hi->confT == b1357->te, "(2) the 13:54 high 7,870.50 is the zigzag pivot, known at the 13:57 close");
    }
    // ---- 2b. the day through the signal book: bullish A at 10:36 / bearish A at 13:54, both confirmed one bar later
    BarRows R; R.spb = 180; nodeRows(R, F, *b1036, 7843.0, -1); nodeRows(R, F, *b1354, 7869.0, 1);
    const std::set<long long> nodes = {b1036->te, b1354->te};
    {
        SignalBook bk; bk.cfg.absorbMethod = 1; bk.version = "2.0.4";
        feedDay(bk, F, F.bars, R, nodes, 1);
        const Mark* a = aAt(bk, b1036->te); const Mark* c = aAt(bk, b1354->te);
        if (a) std::printf("   10:36: %s %s at %.2f, %lld sold in %s-%s / normal minute %.0f = %.1fx, %s at %s\n", a->dir > 0 ? "bullish" : "bearish", markLabel(*a).c_str(), a->px * 0.25,
            a->minuteVol, hm(a->minuteT).c_str(), hm(a->minuteT + 60).c_str(), a->norm, a->mult, a->state == MK_CONFIRMED ? "confirmed" : "NOT confirmed", hm(a->confT).c_str());
        if (c) std::printf("   13:54: %s %s at %.2f, %lld bought in %s-%s / normal minute %.0f = %.1fx, %s at %s\n", c->dir > 0 ? "bullish" : "bearish", markLabel(*c).c_str(), c->px * 0.25,
            c->minuteVol, hm(c->minuteT).c_str(), hm(c->minuteT + 60).c_str(), c->norm, c->mult, c->state == MK_CONFIRMED ? "confirmed" : "NOT confirmed", hm(c->confT).c_str());
        CHECK(a && a->dir > 0 && a->px == T(7843.0, 0.25) && a->state == MK_CONFIRMED && a->confT == b1039->te, "(2) ES 10:36: bullish A? at 7,843 (close 7,844 above it), confirmed A at the 10:39 close (the pivot low)");
        CHECK(c && c->dir < 0 && c->px == T(7869.0, 0.25) && c->state == MK_CONFIRMED && c->confT == b1357->te && c->minuteVol == 2934, "(2) ES 13:54: bearish A? at 7,869 (close 7,865 below it), 2,934 bought 13:51-13:52, confirmed A at 13:57 (the pivot high)");
    }
    // ---- 3. close beyond the node: one-tick node (7843) and two-tick node rows (7843-7843.25) at 10:36
    {
        struct Case { const char* name; double close; int nodeN; bool want; };
        const Case cs[] = { {"close inside the node (= 7843.00, 1-tick node)", 7843.0, 1, false}, {"close on the wrong side (7842.75, below)", 7842.75, 1, false},
                            {"close exactly at the node edge (7843.25, 2-tick node 7843-7843.25)", 7843.25, 2, false}, {"one tick beyond the edge (7843.50, 2-tick node)", 7843.5, 2, true},
                            {"one tick beyond (7843.25, 1-tick node)", 7843.25, 1, true} };
        for (const Case& k : cs) {
            std::vector<BarIn> bars = F.bars; for (auto& b : bars) if (b.te == b1036->te) b.c = T(k.close, 0.25);
            SignalBook bk; bk.cfg.absorbMethod = 1; bk.version = "2.0.4";
            feedDay(bk, F, bars, R, {b1036->te}, k.nodeN);
            const Mark* a = aAt(bk, b1036->te); bool rline = false; for (auto& l : bk.journal) if (l.find("|R|") != std::string::npos && l.find("close not beyond node") != std::string::npos) rline = true;
            std::printf("3. %-66s -> %s\n", k.name, a ? "A?" : (rline ? "rejected, logged 'close not beyond node'" : "nothing"));
            CHECK(k.want ? (a != nullptr && !rline) : (a == nullptr && rline && rejectedClose(bk) == 1), "(3) close beyond the node: inside / at the edge / wrong side rejected and logged; beyond -> A?");
        }
    }
    // ---- 4. the pivot rule
    {
        // (a) the confirmation bar makes the pivot: 10:39 trades 7842.50 (a lower low) but closes green above the node; 10:42 confirms the pivot
        std::vector<BarIn> bars = F.bars; for (auto& b : bars) if (b.te == b1039->te) b.l = T(7842.5, 0.25);
        SignalBook bk; bk.cfg.absorbMethod = 1; bk.version = "2.0.4"; feedDay(bk, F, bars, R, {b1036->te}, 1);
        const Mark* a = aAt(bk, b1036->te);
        std::printf("4a. pivot at the confirmation bar (10:39 low 7842.50): %s at %s (confirmation bar 10:39 -> lateness %d bar)\n", a && a->state == MK_CONFIRMED ? "confirmed" : "NOT confirmed", a ? hm(a->confT).c_str() : "-", a ? (int)((a->confT - b1039->te) / 180) : -1);
        CHECK(a && a->state == MK_CONFIRMED && a->confT == b1039->te + 180, "(4a) the pivot is the confirmation bar: confirmed when the zigzag turns (one bar later)");
        // (b) the pivot is another bar: 10:39 no higher high, then 10:42 trades a lower low (7842.00) -> not at pivot, the A? stays A?
        std::vector<BarIn> b2 = F.bars; for (auto& b : b2) { if (b.te == b1039->te) b.h = T(7846.0, 0.25); if (b.te == b1039->te + 180) { b.l = T(7842.0, 0.25); } }
        SignalBook bk2; bk2.cfg.absorbMethod = 1; bk2.version = "2.0.4"; feedDay(bk2, F, b2, R, {b1036->te}, 1);
        const Mark* a2 = aAt(bk2, b1036->te);
        std::printf("4b. pivot elsewhere (10:42 low 7842.00): %s, why '%s'\n", a2 ? (a2->state == MK_EXPIRED ? "never confirmed (stays A?)" : "CONFIRMED") : "-", a2 ? a2->why.c_str() : "-");
        CHECK(a2 && a2->state == MK_EXPIRED && a2->why.find("not at pivot") != std::string::npos && markLabel(*a2).compare(0, 2, "A?") == 0, "(4b) the pivot is another bar: 'not at pivot', the A? is never confirmed (and never removed)");
        // (c) the rule off = the 2.0.3 behaviour (study variant)
        SignalBook bk3; bk3.cfg.absorbMethod = 1; bk3.cfg.pivotRule = false; feedDay(bk3, F, b2, R, {b1036->te}, 1);
        const Mark* a3 = aAt(bk3, b1036->te);
        CHECK(a3 && a3->state == MK_CONFIRMED, "(4c) with the pivot rule off (study variant) the same A confirms (2.0.3 rule)");
    }
    // ---- 5. no repaint: a restart after EVERY bar (the record reloaded, the session's bars re-fed) == one straight pass
    {
        SignalBook straight; straight.cfg.absorbMethod = 1; straight.version = "2.0.4"; feedDay(straight, F, F.bars, R, nodes, 1);
        std::string text = std::string(SignalBook::header()) + "\n"; size_t restarts = 0;
        for (size_t k = 1; k <= F.bars.size(); ++k) {
            SignalBook bk; bk.cfg.absorbMethod = 1; bk.version = "2.0.4"; bk.nodeOnly = true; bk.loadText(text);
            feedDay(bk, F, F.bars, R, nodes, 1, 0, k);                      // the plugin re-feeds the session from its start
            for (auto& l : bk.journal) text += l + "\n";
            char d[64]; snprintf(d, sizeof(d), "%lld|D|0|-|0|0|0|0|0||2.0.4\n", F.bars[k - 1].te); text += d; restarts++;
        }
        SignalBook fin; fin.nodeOnly = true; fin.loadText(text);
        auto key = [](const SignalBook& b) { std::vector<std::string> v; for (auto& m : b.marks) if (m.kind == 'A') { char x[160]; snprintf(x, sizeof(x), "%lld %d %d %d %lld %.2f", m.barT, m.dir, m.px, m.state, m.confT, m.mult); v.push_back(x); } return v; };
        const auto a = key(straight), b = key(fin);
        std::printf("5. restart after every bar: %zu restarts, A's straight %zu / restarted %zu, identical: %s\n", restarts, a.size(), b.size(), a == b ? "yes" : "NO");
        CHECK(a == b && !a.empty(), "(5) a restart after every bar gives exactly the straight pass (zigzag + pivot rule, no repaint)");
    }
    std::printf("v204 rules: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}

static void dbgNode()
{
    const Friday F = loadFriday(); const BarIn* b = barAt(F, 10, 36);
    BarRows R; R.spb = 180; nodeRows(R, F, *b, 7843.0, -1);
    BarFlow fl; fl.secs = 180; nodeFill(fl, R.at(b->te), *b, 1);
    std::printf("dir %d lo %d hi %d mult %.2f pos %.2f closeBeyond %d close %d low %d\n", fl.ndDir, fl.ndLo, fl.ndHi, fl.ndMult, fl.ndPos, fl.ndCloseBeyond, b->c, b->l);
    int lo = INT_MAX; for (auto& x : F.bars) if (x.te < b->te && x.te >= b->te - 19 * 180) lo = std::min(lo, x.l); std::printf("19-bar low before %d\n", lo);
}
