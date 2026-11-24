/********************************************************************************
 *  Bo3h.cpp  --  Investor/RT RTX extension  ls3hBO  "3h BO"  1.0.0  (2026-11-23)
 *
 *  Rassul 2026-11-23 CT, on his GC 180-minute chart: 19:23 "the 3hr bo just looks for breaks of pivots and then their test . so the idea is
 *  to look for an expanding range break through a pivot and then its test and label"; 19:27 "the close should take out the pivot"; 19:30 "it
 *  should have been drawn"; 19:33 "the test needs to be .33 at of the pivot break"; 19:34 "yes both", "build"; 20:17 "rename the bo indicator
 *  to  3h BO"; 20:20 "im talking about the htf bo right .. dont mess up the 3min bo indicator"; 20:25 "1" (a separate indicator).
 *  On the PRICE pane of a 180-MINUTE chart, letters only: BO (green, above its candle) / BD (red, below) when a candle closes beyond a
 *  1-bar ZigZag pivot with a range bigger than the candle before; Tst (green below a pullback low / red above a pullback high) when the
 *  pullback overlaps the BO candle and comes within 1/3 ATR(14) of the broken pivot (Bo3hLogic.h b3h has every rule). The Tst is LIVE on
 *  the leg still running and final once the pivot is confirmed; a BO / BD is decided on closed candles only. On any other chart period it
 *  draws nothing. Computed natively from the chart's own candles; no settings, no hovers, no other text. lsBreakout is not touched.
 *  FILE   lsFlexLevels\Bo3h.status-<MKT>-<spb>.txt   version, counts, drawn
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>     // BEFORE irtsdk.h
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "Bo3hLogic.h"
#include "MarketKey.h"
#include "HostSlot.h"
#ifndef LSPERF_PLUGIN
  #define LSPERF_PLUGIN "Bo3h"
#endif
#include "PerfGuard.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#if defined(_WIN32)
#define B3_SEP "\\"
#else
#define B3_SEP "/"
#endif
static const char* B3_VERSION = "1.1.0";   // (1.1.0, 2026-11-23 23:06 CT "include sp and ut"): + SP / UT (b3h 1.1.0); at most 2 marks a side of a candle
// was static const char* B3_VERSION = "1.0.0";   // = b3h::VERSION = the setVersion literal
static const unsigned long B3_UP = 0x0022C55E, B3_DN = 0x00EF4444;   // green bullish / red bearish
static const int B3_FONT = 7, B3_GAP = 3;                            // lsBreakout's letter size and gap

static long long b3Ms() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string b3Flex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + B3_SEP "InvestorRT" B3_SEP "rtx" B3_SEP "lsFlexLevels" : std::string(); }
static bool b3Publish(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { std::remove(tmp.c_str()); return false; }
    return true;
#endif
}
static short b3C(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }

struct B3State {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    // the marks by chart bar index with that candle's high / low as calc read them; recomputed only when the chart changed
    struct M { int bar = 0; char code = 0; int dir = 0; bool live = false; double h = 0, l = 0; };
    std::vector<M> marks; long n = -1; long long t0 = 0; double k[3] = { 0, 0, 0 }; int cnt[6] = { 0, 0, 0, 0, 0, 0 }; int drawn = 0;   // cnt: BO, BD, Tst, live Tst, (1.1.0) SP, UT
    std::string statusPath, statusBody; long long statusMs = -1;
};

class Bo3h : public cppExtension {
public:
    SS_SAFE_DIALOG(Bo3h, "Bo3h")               // the chart-less Add / settings copy never calls into the chart
    Bo3h() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<B3State> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(B3State& S);
    void feed(B3State& S);
    void status(B3State& S, long long now);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<Bo3h*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Bo3h*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); Bo3h* me = static_cast<Bo3h*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

bool Bo3h::identify(B3State& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000 || mkt.empty() || spb <= 0) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.marks.clear(); S.n = -1; S.t0 = 0; S.cnt[0] = S.cnt[1] = S.cnt[2] = S.cnt[3] = S.cnt[4] = S.cnt[5] = 0; S.drawn = 0;
        const std::string d = b3Flex();
        S.statusPath = d.empty() ? std::string() : d + B3_SEP "Bo3h.status-" + mkey::norm(mkt) + "-" + std::to_string(spb) + ".txt";
    }
    return true;
}

// the chart's own candles (every bar of a 180-minute chart is a 3-hour bar; the last one is forming) through b3h::run. Recomputed only
// when a bar was added, the history changed or the forming candle moved. Marks keep the candle's high / low so draw reads no bars.
void Bo3h::feed(B3State& S) {
    if (S.spb != b3h::PERIOD) { S.marks.clear(); S.n = -1; S.cnt[0] = S.cnt[1] = S.cnt[2] = S.cnt[3] = S.cnt[4] = S.cnt[5] = 0; return; }
    const long n = getBarCount(); if (n < 4) { S.marks.clear(); S.n = n; return; }
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const double kh = hi[(int)n - 1], kl = lo[(int)n - 1], kc = cl[(int)n - 1];
    if (S.n == n && S.t0 == (long long)times[0] && S.k[0] == kh && S.k[1] == kl && S.k[2] == kc) return;
    const int first = std::max(0, (int)n - b3h::MAX_BARS);
    std::vector<b3h::Bar> B((size_t)(n - first));
    for (int i = first; i < (int)n; ++i) { b3h::Bar& b = B[(size_t)(i - first)]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; }
    const std::vector<b3h::Mark> M = b3h::run(B);
    S.marks.clear(); S.cnt[0] = S.cnt[1] = S.cnt[2] = S.cnt[3] = S.cnt[4] = S.cnt[5] = 0;
    for (const b3h::Mark& m : M) { if (m.bar < 0 || m.bar >= (int)B.size()) continue;
        B3State::M x; x.bar = m.bar + first; x.code = m.code; x.dir = m.dir; x.live = m.live; x.h = B[(size_t)m.bar].h; x.l = B[(size_t)m.bar].l; S.marks.push_back(x);
        if (m.code == 'T') { ++S.cnt[2]; if (m.live) ++S.cnt[3]; } else if (m.code == 'S') ++S.cnt[4]; else if (m.code == 'U') ++S.cnt[5]; else ++S.cnt[m.dir > 0 ? 0 : 1]; }
    std::stable_sort(S.marks.begin(), S.marks.end(), [](const B3State::M& a, const B3State::M& b) {   // (1.1.0) draw priority on a candle side: BO / BD, SP / UT, Tst
        const auto pr = [](char c) { return c == 'B' ? 0 : c == 'T' ? 2 : 1; }; return pr(a.code) < pr(b.code); });
    S.n = n; S.t0 = (long long)times[0]; S.k[0] = kh; S.k[1] = kl; S.k[2] = kc;
}

void Bo3h::status(B3State& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = std::string("VERSION,") + B3_VERSION + "\nMARKET," + mkey::norm(S.mkt) + "," + S.sym + "," + std::to_string(S.spb) + "\n" +
        (S.spb == b3h::PERIOD ? "H3,b3h " + std::string(b3h::VERSION) + ",BO " + std::to_string(S.cnt[0]) + ",BD " + std::to_string(S.cnt[1]) + ",Tst " + std::to_string(S.cnt[2]) + ",live " + std::to_string(S.cnt[3]) + ",SP " + std::to_string(S.cnt[4]) + ",UT " + std::to_string(S.cnt[5]) + ",bars " + std::to_string(S.n) + ",drawn " + std::to_string(S.drawn)
                                : std::string("H3,off: this is not a 180-minute chart (nothing is drawn)")) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    b3Publish(S.statusPath, body);
}

void Bo3h::pump() {
    B3State* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    try { feed(*S); } catch (...) { S->marks.clear(); S->n = -1; }
    status(*S, b3Ms());
}

// letters only, lsBigBidAsk's style (a 1 px black edge, then the colour): BO green above its candle, BD red below; a bullish Tst green
// below the pullback low, a bearish Tst red above the pullback high. Two marks on one side of a candle stack outward.
int Bo3h::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    B3State* S = slot_.get(this, false);
    if (!S || !S->keyed) return RTX_OK;
    S->drawn = 0; if (S->marks.empty() || S->spb != b3h::PERIOD) return RTX_OK;   // draw never reads bars
    try {
        const long n = getBarCount(); if (n < 4 || n < S->n) return RTX_OK;
        RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
        int axisR = pane.right;
        if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) axisR = scale.left - 2;
        if (pane.bottom - pane.top < 60 || axisR - pane.left < 60) return RTX_OK;
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0, (int)n - 200); ve = (int)n - 1; }
        const int aL = pane.left + 2, aT = pane.top + 20, aR = axisR - 1, aB = pane.bottom - 1;   // the IRT title row stays clear
        FONT f0; f0.id = HELVETICA; f0.size = B3_FONT; f0.style = BOLD; setFont(f0);
        int ll = 0, la = 0, ld = 0; getFontMetrics(&ll, &la, &ld);
        const int lh = std::max((int)f0.size + 5, la + ld), lift = std::max(1, (ll + ld + 1) / 2);
        std::map<std::pair<int, int>, int> rows;   // (bar, above) -> marks already there
        struct RCTi { int l, t, r, b; }; std::vector<RCTi> boxes;   // (1.1.0) the letters drawn so far
        for (const B3State::M& m : S->marks) {
            if (m.bar < vs || m.bar > ve || m.bar >= (int)n) continue;
            const char* tx = m.code == 'T' ? "Tst" : m.code == 'S' ? "SP" : m.code == 'U' ? "UT" : m.dir > 0 ? "BO" : "BD";
            const bool above = m.code == 'B' ? m.dir > 0 : m.dir < 0;      // BO above, BD below; Tst / SP / UT on their pivot's side
            PNT p; p.set(m.bar, 0.0f, kBarCenter); const int cx = (int)p.h;
            const int yh = (int)getVerticalDrawPosition((float)m.h), yl = (int)getVerticalDrawPosition((float)m.l), top = std::min(yh, yl), bot = std::max(yh, yl);
            const int w = (int)getTextWidth(tx, -1) + 2; int& nOn = rows[std::make_pair(m.bar, above ? 1 : 0)];
            if (nOn >= 2) continue;                                        // (1.1.0) his rule: at most 2 marks a candle side (BO / BD first, then SP / UT, then Tst)
            // (1.1.0) the first row (outward from the candle) where the letter touches no letter already drawn - a neighbour candle's too
            int row = nOn, l = cx - w / 2, r = l + w, t = 0, b = 0; bool ok = false;
            for (; row < 4 && !ok; ++row) { t = above ? top - B3_GAP - lh * (row + 1) : bot + B3_GAP + lh * row; b = t + lh; ok = true;
                for (const RCTi& z : boxes) if (l < z.r && z.l < r && t < z.b && z.t < b) { ok = false; break; } }
            if (!ok) continue;
            ++nOn;
            if (l < aL || r > aR || t < aT || b > aB) continue;
            { RCTi z; z.l = l; z.t = t; z.r = r; z.b = b; boxes.push_back(z); }
            setTextColor(0x00000000);
            for (int d = 0; d < 4; ++d) { const int dx = d == 0 ? -1 : d == 1 ? 1 : 0, dy = d == 2 ? -1 : d == 3 ? 1 : 0;
                RCT q; q.set(b3C(l + dx), b3C(t - lift + dy), b3C(r + dx), b3C(b - lift + dy)); q.drawText(tx, true, false); }
            setTextColor((COLOR)(m.dir > 0 ? B3_UP : B3_DN));
            RCT q; q.set(b3C(l), b3C(t - lift), b3C(r), b3C(b - lift)); q.drawText(tx, true, false);
            ++S->drawn;
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    Bo3h* p = new Bo3h();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | NO_UI);       // on the price pane, on the instrument's scale; no hovers, no settings
    p->setDescription("3h BO v1.0.0 - on the price pane of a 180-minute chart, letters only: BO (green, above) / BD (red, below) when a candle closes beyond a ZigZag pivot with a range bigger than the candle before; Tst when the pullback overlaps the BO candle and comes within 1/3 ATR of the broken pivot (live on the running leg, final once the pivot is confirmed).");
    p->setVersion("1.1.0");   // = B3_VERSION (gex-build reads this literal)
    return p;
}
