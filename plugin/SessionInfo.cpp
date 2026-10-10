/********************************************************************************
 *  lsSessionInfo - one session-information panel on each market's chart (Rassul 2026-10-06 21:28:
 *  "I also want to know when 0dte options are expiring for each market ... selectable ... so i can move it around" / "the most
 *  favorable times to trade a market ... customized to each market and displayed on its chart, for the rth session").
 *
 *  1.7.1 (2026-10-12, Rassul / mockups 1-13): ONE short cell flush top-left - "Regime: Positive" over "Expiry 12:30 PM
 *         (in 13h 45m) . pin 4,200 (GW 0DTE)"; ends before the last price bar and the profiles; never a mid-word cut. The news,
 *         the RTH windows, the expected move and its slider are removed (the range used moved to lsHodLod's header).
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

using sil::hm; using sil::parseInt;

// (HL105, Rassul "you should have setting where to put them like upper left etc.") ONE setting: Position (9 spots), default Top left.
// The market is the chart's own; the font is 8 pt (his other boxes).
struct SPIdx { int moveX = 0; };   // parameter index (HL101: a member, no static state)
struct SSet { int moveX = 0; };
static const int SI_FONT = 8, SI_POS_DEFAULT = 0;

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
        if (st >= 0) { std::ifstream f(p.c_str()); std::string ln;                                          // RAII: closed at scope end
            if (std::getline(f, ln)) { if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
                if (ln.size() == 1 && ln[0] >= '0' && ln[0] <= '8') v = ln[0] - '0'; } }                    // (HL105) strict: one digit 0-8, else the default (#29)
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
    std::string hlPath, hlRoot; long long hlStamp = -2; int hlX = 0;   // (1.7.1) lsHodLod's reserved column (its status file)
    hl::Box hlBox = { 0, 0, 0, 0 }; bool hlBoxOk = false, hlBoxRead = false;                  // (HL105) lsHodLod's table box (fresh, this chart)
    long long loads = 0;                    // test counter (#31)
};

class SessionInfo : public cppExtension {
public:
    SessionInfo() : cppExtension() {}
    SPIdx SP;   // set in setup(); one per extension object
    virtual int parmsLoad(void) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int parmsApply(void) { if (ready()) { readS(cfg); savePl(); } return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { if (ready()) { readS(cfg); savePl(); } return RTX_OK; }
    // IRT can return -1 for LIST parameters outside its settings copy: the Position is persisted per market (from the settings
    // dialog's callbacks only - loading never saves, #29) and read back on every draw
    void savePl() { char b[32] = {0}; const char* rs = getRootSymbol(b); std::string mk = dl::marketForRoot(rs ? rs : "");
        dl::savePlace("SessionInfoPos171", mk, cfg.moveX); }
    virtual int draw(void);

    SSet cfg;                                   // scratch: re-read from the settings (or the per-market files) on every draw
    HostSlot<SIState> slot_;
    SIFiles files_;                             // (HL101) the place / layout-room caches, per object (no function statics)
    SIState* st(bool create) { return slot_.get(this, create); }
    void clearState() { slot_.release(this); }

    bool ready() { int i = getListIndex(SP.moveX); return i >= 0 && i <= 8; }
    void readS(SSet& S) { int v = getListIndex(SP.moveX); S.moveX = v >= 0 && v <= 8 ? v : SI_POS_DEFAULT; }   // clamped (#29)
    void load(SIState& S);
    int priceEdge();
    int reservedLeft(SIState& S);
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
    setParameterVersion(9);   // (1.7.1) BUMPED: the only setting is Position (9 spots), default Top left
    setParameterDialogHeight(1);
    int pc = 0;
    SP.moveX = pc++; setListParameter("Position", SI_POS_DEFAULT, dl::ANCHORS);
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
    int m = lt.tm_hour * 60 + lt.tm_min;
    dow = lt.tm_wday;
    if (m >= 1440) dow = (dow + 1) % 7; else if (m < 0) dow = (dow + 6) % 7;
    return (m % 1440 + 1440) % 1440;
}

void SessionInfo::load(SIState& S)
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    S.root = rs ? rs : "";
    std::string mk = dl::marketForRoot(S.root);                                 // (HL105) the chart's own market
    if (mk != S.mkt) { S.mkt = mk; S.hlStamp = -2; }                          // a symbol / market change resets the caches
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

// (1.7.1) the price edge: the last bar's right edge (x), where the bar area ends and the profiles begin; -1 = unknown
int SessionInfo::priceEdge()
{
    long n = getBarCount(); if (n < 2) return -1;
    RTARRAY cl(barClose); if ((long)cl.count < n) return -1;
    PNT p; if (p.set((int)(n - 1), cl[(int)(n - 1)], kBarRight) != RTX_OK) return -1;
    return p.h > 0 ? (int)p.h : -1;
}
// (1.7.1) lsHodLod's reserved candle column: RESERVED_RIGHT_X in its status file HodLod.status-<MKT>-<secs>.txt (with CHART,<root>,
// <secs>). Fresh (written in the last 90 s) and this chart's root -> that x; stale or another chart -> 200 px if it reserved one,
// else 0; absent -> 0. Read only when the file's stamp changes (#31), at most 4 KiB, strict ints.
int SessionInfo::reservedLeft(SIState& S)
{
    long n = getBarCount(); if (n < 3) return 0;
    RTARRAYI dt(barDateTime); if ((long)dt.count < n) return 0;
    long long per = 0; for (long i = n - 1; i > 0 && i > n - 12; i--) { long long d = (long long)dt[(int)i] - (long long)dt[(int)(i - 1)]; if (d > 0 && (per == 0 || d < per)) per = d; }
    if (per <= 0 || per > 3600) return 0;
    const char* up = getenv("USERPROFILE"); if (!up) return 0;
#ifdef _WIN32
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\HodLod.status-" + S.mkt + "-" + std::to_string(per) + ".txt";
#else
    std::string p = std::string(up) + "/HodLod.status-" + S.mkt + "-" + std::to_string(per) + ".txt";   // as lsHodLod's mock path
#endif
    long long st = dl::fileStamp(p);
    S.hlBoxOk = false;
    if (st < 0) { S.hlStamp = st; S.hlX = 0; S.hlRoot.clear(); return 0; }
    if (st != S.hlStamp || p != S.hlPath) {
        S.hlStamp = st; S.hlPath = p; S.hlX = 0; S.hlRoot.clear(); S.hlBoxRead = false;
        std::ifstream f(p.c_str(), std::ios::binary); std::string ln; int lines = 0;
        while (f.is_open() && lines++ < 400 && std::getline(f, ln)) {
            if (ln.size() > 512) continue;
            if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
            int v = 0;
            if (ln.compare(0, 17, "RESERVED_RIGHT_X,") == 0 && sil::parseInt(ln.substr(17), v) && v >= 0 && v <= 4000) S.hlX = v;
            if (ln.compare(0, 6, "CHART,") == 0) { size_t c = ln.find(',', 6); S.hlRoot = ln.substr(6, c == std::string::npos ? std::string::npos : c - 6); }
            if (ln.compare(0, 10, "TABLE_BOX,") == 0) {                // l,t,r,b (pane x / y), strict ints
                int v4[4]; size_t p0 = 10; bool okb = true;
                for (int k = 0; k < 4 && okb; k++) { size_t q = ln.find(',', p0); okb = sil::parseInt(ln.substr(p0, q == std::string::npos ? std::string::npos : q - p0), v4[k]) && v4[k] >= -10000 && v4[k] <= 10000; p0 = q == std::string::npos ? ln.size() : q + 1; }
                if (okb && v4[2] > v4[0] && v4[3] > v4[1]) { S.hlBox = hl::Box{ v4[0], v4[1], v4[2], v4[3] }; S.hlBoxRead = true; }
            }
        }
    }
    long long mt = st / 1000003LL;                                     // dl::fileStamp = mtime * 1000003 + size
    bool fresh = (long long)time(nullptr) - mt <= 90;
    if (fresh && S.hlRoot == S.root) { S.hlBoxOk = S.hlBoxRead; return S.hlX; }
    return S.hlX > 0 ? 200 : 0;
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
    load(*S);
    if (S->mkt.empty()) return RTX_OK;
    if (ready()) readS(cfg); else cfg.moveX = files_.loadPlace("SessionInfoPos171", S->mkt, SI_POS_DEFAULT);
    int pos = cfg.moveX >= 0 && cfg.moveX <= 8 ? cfg.moveX : SI_POS_DEFAULT;
    sil::Inputs I; I.mkt = S->mkt; I.D = S->D; I.sourceAvailable = S->sourceAvailable; I.show = 0; I.fs = SI_FONT;
    I.now = nowMin(I.dow);
    sil::Slider SL;
    std::vector<sil::Section> sec = sil::build(I, SL);
    RCT pane; pane.getPaneRect(false);
    // (HL105) the usable area: [the pane's left edge (2 px in; right of lsHodLod's candle column when it draws one), the LAST price
    // bar's right edge] x [the pane's top, its bottom]; never beyond the last bar, never into the profiles' room
    int right = files_.clearRightOf(pane.right);
    int edge = priceEdge(); if (edge > pane.left + 120 && edge < right) right = edge;
    if (edge <= pane.left + 120) right = (std::min)(right, (int)pane.right - (int)((pane.right - pane.left) * 0.15));   // no bar x: the old rule
    int left = (std::max)((int)pane.left + 2, reservedLeft(*S));
    hl::Box region = { left, pane.top, right, pane.bottom };
    if (region.r - region.l < 120) region.r = pane.right;
    hl::Measure M = [&](const std::string& s, int fs, bool bold) { return textW(s, fs, bold); };
    hl::Layout L = sil::layout(sec, SL, region, pos, SI_FONT, M);
    // two boxes in the same spot: stack this one next to lsHodLod's table (its TABLE_BOX in its status file)
    if (L.tableOk && S->hlBoxOk && hl::overlap(L.table, S->hlBox)) {
        bool above = (L.table.t + L.table.b) / 2 <= (S->hlBox.t + S->hlBox.b) / 2;          // the nearer side first, then the other
        for (int k = 0; k < 2; k++, above = !above) {
            hl::Box r2 = region;
            if (above) r2.b = S->hlBox.t - 2; else r2.t = S->hlBox.b + 2;
            if (r2.b - r2.t <= 30) continue;
            hl::Layout L2 = sil::layout(sec, SL, r2, pos, SI_FONT, M);
            if (L2.tableOk && !hl::overlap(L2.table, S->hlBox)) { L = L2; break; }
        }
    }
    execute(L);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionInfo *p = new SessionInfo();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);   // drawn after every other indicator
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // (1.7.0, audit #36) the 0DTE countdown and news colours keep moving while the feed is idle
    p->setDescription("LRA Session Info: one short box, flush top-left: the gamma regime, today's 0DTE expiry countdown and the pin (GW 0DTE). Use Settings > Position to place the panel.");
    p->setVersion("1.7.1");   // (1.7.1) one cell (Regime / Expiry . pin), flush top-left, ends before the last price bar and the profiles, words never cut; the news, the RTH windows, the expected move and its slider removed (Rassul 2026-10-12); (1.7.0) wide 4-section layout, Background setting removed, per-chart state; (1.6.7) drawn on top of the other indicators; (1.6.6) solid background always; (1.6.5) overflow-safe numeric fields; (1.6.4) Market selection restored per native chart; (1.6.3) Show saved per market; (1.6.1) below the chart title, data age from 10 min; (1.6.0) gamma / HVL, GW0 pin, expected move, news countdown, stale line; (1.5.0) 9-spot Position
    return p;
}
