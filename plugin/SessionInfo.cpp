/********************************************************************************
 *  lsSessionInfo - one session-information panel on each market's chart (Rassul 2026-10-06 21:28:
 *  "I also want to know when 0dte options are expiring for each market ... selectable ... so i can move it around" / "the most
 *  favorable times to trade a market ... customized to each market and displayed on its chart, for the rth session").
 *
 *  1.7.0 (2026-10-09, mockup v5 approved: "check everything then build it") the WIDE layout: four sections side by side, a
 *  header + 3 lines each, at the top centre (shifted left so it ends well before the latest bars):
 *    NEWS | GAMMA / Exp <time> (<countdown>) | <MKT> RTH now: <state> + Active / Chop windows | EXPECTED MOVE
 *  The expected-move slider is coloured red -> grey -> green with a white spot marker and "spot X - N% up the range", plus small
 *  green (E-HOD) and red (E-LOD) ticks where the HOD / LOD model candle (HodLodExpected.h) expects today's HOD / LOD, computed
 *  natively from the chart's own bars (an arrow at the end when outside the range). Every 1.6.7 content element is kept; only the
 *  layout changes. The dead Background setting is gone (audit #41). Layout + content: SessionInfoLayout.h (tested, previewed).
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Session-<MKT>.csv (written by the bridge; format unchanged).
 *  State: per chart in the host's user-data slot (HostSlot.h): the parsed file, its stamp and the bar cache.
 ********************************************************************************/
#include "irtsdk.h"
// windows.h can define far as a legacy keyword macro; DealerLogic's shared Node::far field must remain its declared identifier.
#ifdef far
#undef far
#endif
#include "DealerLogic.h"
#include "HostSlot.h"
#include "HodLodLogic.h"
#include "SessionInfoLayout.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <climits>
#include <ctime>
#include <map>
#include <deque>
#include <chrono>

using sil::hm; using sil::parseInt; using sil::inWindow; using sil::to12;
using sil::MAX_SESSION_LINE; using sil::MAX_SESSION_WINDOWS; using sil::MAX_SESSION_NEWS;
typedef sil::Win Win;

struct SPIdx { int market = 0, show = 1, font = 2, clock = 3, moveX = 4; };   // parameter indices (HL101: a member, no static state)
struct SSet { int market = 0, show = 0, font = 8, clock = 0, moveX = 1; };

// (HL101, Rassul "no leaks or errors": no static / mutable shared state) the per-object copies of dl::loadPlace / dl::clearRightOf /
// dl::stableMin - the same rules, but their caches live in the extension object (freed with it), bounded, never function statics
struct SIFiles {
    std::vector<std::pair<std::string, std::pair<long long, int> > > place;      // path -> (stamp, value); <= 64 entries
    long long s1 = -2, s2 = -2; int reach = 150, dw = 50; bool dLeft = false;     // the profiles' status files, by stamp
    std::map<int, std::deque<std::pair<long long, int> > > hist;                  // paneRight -> the last 5 s of values; <= 16 keys
    int loadPlace(const char* plugin, const std::string& mkt, int def)
    {
        if (mkt.empty()) return def;
        std::string p = dl::placePath(plugin, mkt);
        long long st = dl::fileStamp(p);
        for (size_t i = 0; i < place.size(); i++) if (place[i].first == p) { if (place[i].second.first == st) return place[i].second.second; place.erase(place.begin() + (long)i); break; }
        int v = def;
        if (st >= 0) { std::ifstream f(p.c_str()); int x = -1; if (f >> x && x >= 0 && x <= 8) v = x; }   // RAII: closed at scope end
        if (place.size() >= 64) place.erase(place.begin());
        place.push_back(std::make_pair(p, std::make_pair(st, v)));
        return v;
    }
    int stableMin(int key, int v)
    {
        long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        if (hist.size() > 16 && !hist.count(key)) hist.clear();
        std::deque<std::pair<long long, int> >& q = hist[key];
        q.push_back(std::make_pair(now, v));
        while (!q.empty() && now - q.front().first > 5000) q.pop_front();
        if (q.size() > 400) q.pop_front();
        int m = v; for (size_t i = 0; i < q.size(); i++) if (q[i].second < m) m = q[i].second;
        return m;
    }
    int clearRightOf(int paneRight)
    {
        const char* up = getenv("USERPROFILE");
        if (up) {
            std::string a = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerProfile.status.txt";
            std::string b = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DeltaProfile.status.txt";
            long long t1 = dl::fileStamp(a), t2 = dl::fileStamp(b);
            if (t1 != s1) { s1 = t1; std::ifstream f(a.c_str()); std::string ln; int n = 0;
                while (n++ < 200 && std::getline(f, ln)) if (ln.rfind("REACH,", 0) == 0) { int r = atoi(ln.c_str() + 6); if (r >= 40 && r <= 1500) reach = r; } }
            if (t2 != s2) { s2 = t2; std::ifstream f(b.c_str()); std::string ln; int n = 0;
                while (n++ < 200 && std::getline(f, ln)) { if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
                    if (ln.rfind("WIDTH,", 0) == 0) { int w = atoi(ln.c_str() + 6); if (w >= 20 && w <= 600) dw = w; }
                    if (ln.rfind("PLACE,", 0) == 0) dLeft = ln.substr(6) == "Left"; } }
        }
        int r = paneRight - reach - 8;
        if (!dLeft) r -= dw + 110;
        return stableMin(paneRight, r);
    }
};

// one chart's state (checklist #1): the session file snapshot and the HOD/LOD bar cache
struct SIState {
    std::string mkt, root, path;
    long long stamp = -2;
    bool sourceAvailable = false;
    sil::Data D;
    long nBars = -1; RTDATE lastStamp = 0; float lastTick = 0;
    bool haveE = false; double eLod = NAN, eHod = NAN;
    long long loads = 0, barCalcs = 0;      // test counters (#31 / #32)
};

class SessionInfo : public cppExtension {
public:
    SessionInfo() : cppExtension() {}
    SPIdx SP;   // set in setup(); one per extension object
    virtual int parmsLoad(void) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int parmsApply(void) { if (ready()) { readS(cfg); savePl(); } return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { if (ready()) { readS(cfg); savePl(); } return RTX_OK; }
    // IRT can return -1 for LIST parameters outside its settings copy. Persist all list selections that control this shared
    // extension, including Market, by the chart's native market so a stale selection cannot leak into another chart.
    void savePl() { char b[32] = {0}; const char* rs = getRootSymbol(b); std::string r = rs ? rs : "";
        std::string chartMk = dl::marketForRoot(r), mk = dl::marketFor(cfg.market, r);
        dl::savePlace("SessionInfoMarket", chartMk, cfg.market);
        dl::savePlace("SessionInfoWide", mk, cfg.moveX); dl::savePlace("SessionInfoShow", mk, cfg.show); }
    virtual int draw(void);

    SSet cfg;                                   // scratch: re-read from the settings (or the per-market files) on every draw
    HostSlot<SIState> slot_;
    SIFiles files_;                             // (HL101) the place / layout-room caches, per object (no function statics)
    SIState* st(bool create) { return slot_.get(this, create); }
    void clearState() { slot_.release(this); }

    bool ready() { int i = getListIndex(SP.market); return i >= 0 && i <= 7; }
    void readS(SSet& S)
    {
        S.market = getListIndex(SP.market); if (S.market < 0 || S.market > 7) S.market = 0;
        S.show = getListIndex(SP.show); if (S.show < 0 || S.show > 4) S.show = 0;
        S.font = getIntegerValue(SP.font); if (S.font < 6 || S.font > 20) S.font = 8;
        S.clock = getIntegerValue(SP.clock); if (S.clock < -720 || S.clock > 720) S.clock = 0;
        { int v = getListIndex(SP.moveX); if (v >= 0 && v <= 8) S.moveX = v; }
    }
    void load(SIState& S);
    void barsE(SIState& S);
    int nowMin(int& dow);
    int textW(const std::string& s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s.c_str(), -1); }
    void text(int x, int yc, const std::string& s, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(yc - (short)(sz * 0.45f + 0.5f)); int w = (int)getTextWidth(s.c_str(), -1);
        RCT rc; rc.set((short)x, (short)(yy - sz), (short)(x + w + 4), (short)(yy + sz)); rc.drawText(s.c_str(), false, false);
    }
    void line(int x1, int y1, int x2, int y2, COLOR c, int w)
    {
        setPen(c, (short)w, P_SOLID);
        PNT a; a.set(0, 0.0f); a.h = (short)x1; a.v = (short)y1; a.setDrawPosition();
        PNT b; b.set(0, 0.0f); b.h = (short)x2; b.v = (short)y2; b.drawLineTo();
    }
    void execute(const hl::Layout& L);
    std::string fit(const std::string& s, int w, int fs, bool bold)   // binary search, "..." (the regression test checks the call count)
    { hl::Measure M = [&](const std::string& t, int f, bool b) { return textW(t, f, b); }; return sil::fit(s, w, fs, bold, M); }
};

int cppExtension::init(void)    { return RTX_OK; }   // required by the SDK (as every LRA plugin)
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { static_cast<SessionInfo*>(this)->clearState(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<SessionInfo*>(this)->clearState(); return RTX_OK; }

int cppExtension::setup(void)
{
    SPIdx& SP = static_cast<SessionInfo*>(this)->SP;   // the parameter indices live in the object (no static state)
    setParameterVersion(7);   // (1.7.0) BUMPED: the Background setting is removed; Position defaults to Top centre
    setParameterDialogHeight(4);
    const short SLN = kParmAppendSameLine;
    int pc = 0;
    SP.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    SP.show   = pc++; setListParameter("Show", 0, "All;0DTE + gamma;RTH active / chop;News;Expected move", 0, SLN);
    SP.font   = pc++; setIntegerParameter("Font size (pt)", 8, NUMW);
    SP.clock  = pc++; setIntegerParameter("Clock offset (min)", 0, NUMW, SLN);
    SP.moveX  = pc++; setListParameter("Position", 1, dl::ANCHORS);   // (1.7.0) default Top centre
    return RTX_OK;
}

int SessionInfo::nowMin(int& dow)
{
    time_t t = time(0); struct tm lt; memset(&lt, 0, sizeof(lt));
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    int m = lt.tm_hour * 60 + lt.tm_min + cfg.clock;
    dow = lt.tm_wday;
    if (m >= 1440) dow = (dow + 1) % 7; else if (m < 0) dow = (dow + 6) % 7;
    return (m % 1440 + 1440) % 1440;
}

void SessionInfo::load(SIState& S)
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    S.root = rs ? rs : "";
    std::string mk = dl::marketFor(cfg.market, S.root);
    if (mk != S.mkt) { S.mkt = mk; S.nBars = -1; S.haveE = false; }          // a symbol / market change resets the bar cache
    if (S.mkt.empty()) { S.D.clear(); S.sourceAvailable = false; S.stamp = -2; S.path.clear(); return; }
    const char* up = getenv("USERPROFILE");
    if (!up) { S.D.clear(); S.sourceAvailable = false; S.stamp = -2; S.path.clear(); return; }
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Session-" + S.mkt + ".csv";
    long long fst = dl::fileStamp(p);
    if (fst == S.stamp && p == S.path) return;                       // unchanged (including a known missing file): no open or parse (#31)
    S.D.clear(); S.sourceAvailable = false; S.stamp = fst; S.path = p;
    if (fst < 0) return;
    std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) return;
    std::string text; text.resize(sil::MAX_SESSION_BYTES + 1);
    f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount());
    S.loads++;
    bool ok = !f.bad() && sil::parseSession(text, S.D);
    if (!ok || dl::fileStamp(p) != fst) {
        S.D.clear(); S.sourceAvailable = false; S.stamp = -2; S.path.clear();  // never render a partial, oversized or failed snapshot
        return;
    }
    S.sourceAvailable = true;
}

// (1.7.0) E-LOD / E-HOD from today's RTH open and the HOD/LOD model (the same logic and header as lsHodLod), from the chart's
// closed bars; recomputed only when the bars change (#32)
void SessionInfo::barsE(SIState& S)
{
    int mi = hl::mktIndex(dl::marketForRoot(S.root));
    if (mi < 0 || hl::mktIndex(S.mkt) != mi) { S.haveE = false; return; }     // a Market override for another market: no ticks
    long n = getBarCount();
    if (n < 3) { S.haveE = false; return; }
    RTARRAYI dt(barDateTime);
    if ((long)dt.count < n) { S.haveE = false; return; }
    float tk = getProperty(SYM_TICKINCR);
    RTDATE last = (RTDATE)dt[(int)n - 1];
    if (n == S.nBars && last == S.lastStamp && tk == S.lastTick) return;
    S.nBars = n; S.lastStamp = last; S.lastTick = tk; S.haveE = false; S.barCalcs++;
    double tick = (double)tk;
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return;
    tick = std::round(tick * 1e9) / 1e9;
    RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    auto absOf = [&](int i) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        return hl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday) * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    };
    long long per = 0, prev = absOf((int)n - 1);
    for (int i = (int)n - 2; i >= 0 && i >= (int)n - 41; i--) { long long a = absOf(i), d = prev - a; if (d > 0 && (per == 0 || d < per)) per = d; prev = a; }
    if (per <= 0 || per > 900) return;
    const int lastClosed = (int)n - 2;
    long long cutoff = absOf(lastClosed) - 2LL * 86400;                    // today's session only needs the last day
    int start = lastClosed;
    while (start > 0 && lastClosed - start < 4000) { if (absOf(start - 1) < cutoff) break; start--; }
    std::vector<hl::Bar> B; B.reserve((size_t)(lastClosed - start + 1));
    for (int i = start; i <= lastClosed; i++) { hl::Bar b; b.absEnd = absOf(i); b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; B.push_back(b); }
    hl::Cfg c; c.mkt = mi; c.per = per; c.tick = tick; c.readOpen = hl::readOpenFor(mi); c.readN = hl::readNFor(mi);
    hl::Tracker T = hl::runBars(c, B, B.size(), nullptr);
    if (!T.cur.any) return;                                                // before today's open: no ticks
    hl::View v = hl::buildView(T, hl::ReadOut());
    if (std::isfinite(v.eLod) && std::isfinite(v.eHod)) { S.haveE = true; S.eLod = v.eLod; S.eHod = v.eHod; }
}

void SessionInfo::execute(const hl::Layout& L)
{
    for (size_t i = 0; i < L.items.size(); i++) {
        const hl::Item& it = L.items[i];
        switch (it.k) {
            case hl::K_FILL: { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw(0, (COLOR)it.c1, (COLOR)it.c1, DRAW_OPAQUE, PAT_SOLID); break; }
            case hl::K_RECT: { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw((short)it.w, (COLOR)it.c1, (COLOR)it.c2, DRAW_OPAQUE, PAT_SOLID); break; }
            case hl::K_LINE: line(it.l, it.t, it.r, it.b, (COLOR)it.c1, it.w); break;
            case hl::K_TEXT: text(it.l, (it.t + it.b) / 2, it.s, (COLOR)it.c1, it.fs, it.bold); break;
            default: break;
        }
    }
}

int SessionInfo::draw(void)
{
    SIState* S = st(true);
    if (!S) return RTX_OK;
    bool settingsReady = ready();
    if (settingsReady) readS(cfg);
    else {
        cfg = SSet();
        char b[32] = {0}; const char* rs = getRootSymbol(b); std::string chartMk = dl::marketForRoot(rs ? rs : "");
        cfg.market = files_.loadPlace("SessionInfoMarket", chartMk, 0);
        if (cfg.market < 0 || cfg.market > 7) cfg.market = 0;
        cfg.font = getIntegerValue(SP.font); if (cfg.font < 6 || cfg.font > 20) cfg.font = 8;
        cfg.clock = getIntegerValue(SP.clock); if (cfg.clock < -720 || cfg.clock > 720) cfg.clock = 0;
    }
    load(*S);
    if (S->mkt.empty()) return RTX_OK;
    if (!settingsReady) cfg.show = files_.loadPlace("SessionInfoShow", S->mkt, cfg.show);
    int pos = files_.loadPlace("SessionInfoWide", S->mkt, 1);                 // (1.7.0) new key: the wide panel starts at Top centre
    if (cfg.show == 0 || cfg.show == 4) barsE(*S);
    sil::Inputs I; I.mkt = S->mkt; I.D = S->D; I.sourceAvailable = S->sourceAvailable; I.show = cfg.show; I.fs = cfg.font;
    I.now = nowMin(I.dow); I.haveE = S->haveE; I.eLod = S->eLod; I.eHod = S->eHod;
    sil::Slider SL;
    std::vector<sil::Section> sec = sil::build(I, SL);
    RCT pane; pane.getPaneRect(false);
    // (1.7.0) the region: below IRT's title line, left of the profiles, and ending well before the latest bars (15% of the pane)
    int right = (std::min)(files_.clearRightOf(pane.right), (int)pane.right - (int)((pane.right - pane.left) * 0.15));
    hl::Box region = { pane.left, pane.top + 18, right, pane.bottom };
    if (region.r - region.l < 120) region.r = pane.right;
    hl::Measure M = [&](const std::string& s, int fs, bool bold) { return textW(s, fs, bold); };
    hl::Layout L = sil::layout(sec, SL, region, pos, cfg.font, M);
    execute(L);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionInfo *p = new SessionInfo();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);   // drawn after every other indicator
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // (1.7.0, audit #36) the 0DTE countdown and news colours keep moving while the feed is idle
    p->setDescription("LRA Session Info: news, gamma regime and today's 0DTE expiry, the RTH active / chop windows and the expected move (with the HOD / LOD model's ticks), side by side. Use Settings > Position to place the panel.");
    p->setVersion("1.7.0");   // (1.7.0) wide 4-section layout at top centre, coloured expected-move slider with E-HOD / E-LOD ticks, Background setting removed, per-chart state; (1.6.7) drawn on top of the other indicators; (1.6.6) solid background always; (1.6.5) overflow-safe numeric fields; (1.6.4) Market selection restored per native chart; (1.6.3) Show saved per market; (1.6.1) below the chart title, data age from 10 min; (1.6.0) gamma / HVL, GW0 pin, expected move, news countdown, stale line; (1.5.0) 9-spot Position
    return p;
}
