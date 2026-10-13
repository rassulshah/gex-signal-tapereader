/********************************************************************************
 *  FirstExtreme.cpp  --  Investor/RT RTX extension  lsFirstExtreme  (1.0.0, 2026-10-12)  SHADOW MODE
 *
 *  THE FIRST FINAL EXTREME (Rassul 2026-10-12 18:51 "see review and build the indicator/signals"): the reviewed v2 research model
 *  (first_extreme_upgrade/, locked primary), native. Every closed 3-min RTH bar from 9 min after the open: the probability that the
 *  running LOD is the day's final LOD and printed first (p_low), the same for the HOD (p_high), and In = p_low + p_high (the first
 *  extreme is already in, side unknown). The locked call: the first bar where p_low or p_high >= 70% before the market's deadline
 *  (training first-extreme 75th percentile + 30 min) makes ONE immutable call "LOD?" / "HOD?" on that candle, labelled with the
 *  time it was called. A later break of that level marks it FAILED (kept, never removed or replaced). No call on weak days.
 *  After the close: RIGHT / WRONG (the exact-first target) / UNRESOLVED (same-bar order).
 *  Every definition is in FirstExtremeLogic.h (native port of extreme_features.py: 140,244 research decisions matched to 7e-16;
 *  the 244 locked test calls replayed exactly) and FirstExtremeView.h (states, panel, marker). No file is read at run time.
 *  3-min charts only (the model's bars). Needs 14 complete sessions on the chart (about 3 weeks of 3-min bars) + the overnight.
 *  Writes (lsFlexLevels): FirstExtreme.decisions-<MKT>.csv (every assessed bar: the shadow forecast log), FirstExtreme.calls-<MKT>.csv
 *  (the immutable call), FirstExtreme.status-<MKT>.txt (what is drawn), FirstExtreme.place-<MKT>.txt (Position).
 ********************************************************************************/
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#include "HostSlot.h"
#include "FirstExtremeView.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <string>
#include <vector>

static const COLOR FX_GREY = 0x009CA3AF;

struct FXState {
    std::string root, mkt; int mi = -1;
    long nBars = -1; RTDATE lastStamp = 0, firstStamp = 0;
    bool have = false; std::string state = "starting";
    fx2::View view; std::vector<long long> ends;   // chart bar index -> END stamp (closed bars)
    long long logDay = -1; int logElapsed = -1; long long callDay = -1;           // what is already in the log files
    time_t lastStatusWrite = 0; std::string lastStatus; bool faulted = false;
    long long recomputes = 0;
};

class FirstExtreme : public cppExtension {
public:
    FirstExtreme() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoad(void) { return RTX_OK; }
    virtual int parmsApply(void) { savePos(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { savePos(); return RTX_OK; }
    struct PosCache { std::string path; long long stamp = -2; int v = fx2::POS_DEFAULT; };
    std::vector<PosCache> posCache;
    HostSlot<FXState> slot_;
    FXState* state(bool create) { return slot_.get(this, create); }
    void clearState() { slot_.release(this); }
    void savePos();
    int position(const std::string& mkt);
    int priceEdge();
    void refresh(FXState& S);
    void recompute(FXState& S, long n);
    void writeLogs(FXState& S);
    void writeStatus(FXState& S);
    std::string flexPath(const std::string& name);
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
    void execute(const fx2::Layout& L);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<FirstExtreme*>(this)->clearState(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<FirstExtreme*>(this)->clearState(); return RTX_OK; }
int cppExtension::calc(int)
{
    FirstExtreme* x = static_cast<FirstExtreme*>(this);
    FXState* S = x->state(true);
    if (S) x->refresh(*S);
    return RTX_OK;
}
int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(1);
    setListParameter("Position", (short)fx2::POS_DEFAULT, fx2::POS_CHOICES);
    CPEN p(FX_GREY, 1, P_SOLID);
    setOutputParameter("1st Extreme", DRAW_INVISIBLE, &p, FX_GREY, OUTPUT_NO_UI);
    setArrayDrawingFlags(0, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    return RTX_OK;
}

std::string FirstExtreme::flexPath(const std::string& name)
{
    const char* up = getenv("USERPROFILE");
    if (!up) return "";
#ifdef _WIN32
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\" + name;
#else
    return std::string(up) + "/" + name;
#endif
}

void FirstExtreme::refresh(FXState& S)
{
    char rb[32] = { 0 }; const char* rs = getRootSymbol(rb);
    std::string root = rs ? rs : "";
    if (root != S.root) {
        S = FXState(); S.root = root; S.mkt = fx2::marketForRoot(root); S.mi = fx2::mktIndex(S.mkt);
        S.state = S.mi < 0 ? "unknown market (root " + root + ")" : "starting";
    }
    if (S.mi < 0 || S.faulted) return;
    long n = getBarCount();
    if (n < 3) { S.state = "too few bars"; S.have = false; return; }
    RTARRAYI dt(barDateTime);
    if ((long)dt.count < n) { S.state = "timestamp array unavailable"; S.have = false; return; }
    RTDATE last = (RTDATE)dt[(int)n - 1], first = (RTDATE)dt[0];
    if (n == S.nBars && last == S.lastStamp && first == S.firstStamp) return;   // recompute only when a bar closes
    S.nBars = n; S.lastStamp = last; S.firstStamp = first;
    try { recompute(S, n); }
    catch (const std::exception& e) { S.faulted = true; S.have = false; S.state = std::string("fault: ") + e.what() + " (cleared on symbol change)"; }
    catch (...) { S.faulted = true; S.have = false; S.state = "fault (cleared on symbol change)"; }
}

static double fxRound(double v) { return std::round(v * 1e7) / 1e7; }   // the chart's float prices back to their decimal values

void FirstExtreme::recompute(FXState& S, long n)
{
    S.recomputes++;
    RTARRAYI dt(barDateTime), vol(barVolume); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) { S.state = "price arrays unavailable"; S.have = false; return; }
    if ((long)vol.count < n) { S.state = "volume unavailable (the model needs volume)"; S.have = false; return; }
    auto absOf = [&](int i) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        return fx2::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday) * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    };
    long long per = 0, prev = absOf((int)n - 1);
    for (int i = (int)n - 2; i >= 0 && i >= (int)n - 41; i--) { long long a = absOf(i), d = prev - a; if (d > 0 && (per == 0 || d < per)) per = d; prev = a; }
    if (per != 180) { S.state = "3-minute charts only (the model's bars)"; S.have = false; return; }
    // the closed bars (the last one is still forming) back to 45 calendar days: 14 complete sessions + the overnight
    const int lastClosed = (int)n - 2;
    long long cutoff = absOf(lastClosed) - 45LL * 86400;
    int start = lastClosed; while (start > 0 && lastClosed - start < 60000) { if (absOf(start - 1) < cutoff) break; start--; }
    std::vector<fx2::Bar> B; B.reserve((size_t)(lastClosed - start + 1)); S.ends.assign((size_t)n, 0);
    for (int i = start; i <= lastClosed; i++) {
        fx2::Bar b; b.tEnd = absOf(i); b.o = fxRound(op[i]); b.h = fxRound(hi[i]); b.l = fxRound(lo[i]); b.c = fxRound(cl[i]); b.v = (double)vol[i];
        if (!B.empty() && b.tEnd <= B.back().tEnd) continue;   // strictly increasing stamps only
        B.push_back(b); S.ends[(size_t)i] = b.tEnd;
    }
    if (B.empty()) { S.state = "no closed bars"; S.have = false; return; }
    long long day = fx2::floorDiv(B.back().tEnd, 86400);
    S.view = fx2::buildView(S.mi, B, day);
    S.have = true;
    S.state = S.view.state == fx2::ST_UNAVAILABLE ? S.view.why : "ok";
    writeLogs(S);
}

static std::string fxDayTxt(long long day)
{
    long long z = day + 719468; long long era = (z >= 0 ? z : z - 146096) / 146097; unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long yy = (long long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); unsigned mp = (5 * doy + 2) / 153;
    int d = (int)(doy - (153 * mp + 2) / 5 + 1), mo = (int)(mp < 10 ? mp + 3 : mp - 9); int y = (int)(yy + (mo <= 2));
    char b[16]; snprintf(b, sizeof b, "%04d-%02d-%02d", y, mo, d); return b;
}
static bool fxAppend(const std::string& p, const std::string& header, const std::string& text)
{
    bool fresh = false; { std::ifstream t(p.c_str()); fresh = !t.good(); }
    std::ofstream f(p.c_str(), std::ios::out | std::ios::app | std::ios::binary);
    if (!f.is_open()) return false;
    if (fresh) f << header;
    f << text; f.flush(); return f.good();
}
// the shadow log: each assessed bar once (primed from the file after a restart: the day's lines already there are not repeated)
void FirstExtreme::writeLogs(FXState& S)
{
    const fx2::View& V = S.view;
    if (V.decisions.empty() || V.day < 0) return;
    std::string dayTxt = fxDayTxt(V.day);
    std::string pd = flexPath("FirstExtreme.decisions-" + S.mkt + ".csv"), pc = flexPath("FirstExtreme.calls-" + S.mkt + ".csv");
    if (pd.empty()) return;
    if (S.logDay != V.day) {
        S.logDay = V.day; S.logElapsed = -1; S.callDay = -1;
        std::ifstream f(pd.c_str()); std::string ln;
        while (std::getline(f, ln)) if (ln.compare(0, 10, dayTxt) == 0 && ln.size() > 11) { int e = atoi(ln.c_str() + 11); if (e > S.logElapsed) S.logElapsed = e; }
        std::ifstream g(pc.c_str()); while (std::getline(g, ln)) if (ln.compare(0, 10, dayTxt) == 0) S.callDay = V.day;
    }
    std::string add; int maxE = S.logElapsed;
    for (const auto& D : V.decisions) if (D.elapsed > S.logElapsed) { add += fx2::decisionLine(V, D, dayTxt); maxE = std::max(maxE, D.elapsed); }
    if (!add.empty() && fxAppend(pd, fx2::DECISION_HEADER, add)) S.logElapsed = maxE;
    if (V.called && S.callDay != V.day && fxAppend(pc, fx2::CALL_HEADER, fx2::callLine(V, dayTxt))) S.callDay = V.day;
}

void FirstExtreme::writeStatus(FXState& S)
{
    if (S.mi < 0) return;
    std::string t = std::string("VERSION,") + fx2::FX_VERSION + "\nMODEL," + fx2::FX_MODEL + "\nMARKET," + S.mkt + "\nSTATE," + S.state + "\n";
    if (S.have) {
        const fx2::View& V = S.view; char b[512];
        snprintf(b, sizeof b, "VIEW,state=%d,elapsed=%d,p_low=%.4f,p_high=%.4f,p_in=%.4f,low=%.10g,high=%.10g,called=%d,side=%d,price=%.10g,prob=%.4f,formed=%d,declared=%d,failed=%d,failed_at=%d,verdict=%d\n",
                 (int)V.state, V.elapsed, V.pLow, V.pHigh, V.pIn, V.lo, V.hi, (int)V.called, V.side, V.price, V.prob, V.formed, V.declared, (int)V.failed, V.failedAt, V.verdictKnown ? V.verdict : 9);
        t += b;
        for (const auto& l : fx2::panelLines(V)) { std::string s; for (const auto& g : l) s += g.s; t += "LINE," + s + "\n"; }
    }
    time_t now = time(nullptr);
    if (t == S.lastStatus && now - S.lastStatusWrite < 30) return;
    if (now - S.lastStatusWrite < 2) return;
    std::string p = flexPath("FirstExtreme.status-" + S.mkt + ".txt"); if (p.empty()) return;
    std::string tmp = p + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::out | std::ios::trunc | std::ios::binary); if (!f.is_open()) return; f << t; if (!f.good()) return; }
    std::remove(p.c_str()); if (std::rename(tmp.c_str(), p.c_str()) == 0) { S.lastStatus = t; S.lastStatusWrite = now; }
}

void FirstExtreme::execute(const fx2::Layout& L)
{
    for (const auto& it : L.items) {
        switch (it.k) {
            case fx2::K_FILL: { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw(0, (COLOR)it.c1, (COLOR)it.c1, DRAW_OPAQUE, PAT_SOLID); break; }
            case fx2::K_RECT: { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw((short)it.w, (COLOR)it.c1, (COLOR)it.c2, DRAW_OPAQUE, PAT_SOLID); break; }
            case fx2::K_LINE: line(it.l, it.t, it.r, it.b, (COLOR)it.c1, it.w); break;
            case fx2::K_TEXT: text(it.l, (it.t + it.b) / 2, it.s, (COLOR)it.c1, it.fs, it.bold); break;
            default: break;
        }
    }
}

static long long fxFileStamp(const std::string& p)
{
#ifdef _WIN32
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return -1;
#endif
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}
void FirstExtreme::savePos()
{
    int v = getListIndex(0);
    if (v < 0 || v > 8) return;
    char rb[32] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = fx2::marketForRoot(rs ? rs : "");
    if (mkt.empty()) return;
    std::string p = flexPath("FirstExtreme.place-" + mkt + ".txt"); if (p.empty()) return;
    std::ofstream f(p.c_str(), std::ios::out | std::ios::trunc); if (f.is_open()) f << v << "\n";
}
int FirstExtreme::position(const std::string& mkt)
{
    std::string p = flexPath("FirstExtreme.place-" + mkt + ".txt");
    if (mkt.empty() || p.empty()) return fx2::POS_DEFAULT;
    long long st = fxFileStamp(p);
    for (size_t i = 0; i < posCache.size(); i++) if (posCache[i].path == p) { if (posCache[i].stamp == st) return posCache[i].v; posCache.erase(posCache.begin() + (long)i); break; }
    PosCache c; c.path = p; c.stamp = st; c.v = fx2::POS_DEFAULT;
    if (st >= 0) { std::ifstream f(p.c_str()); std::string ln; if (std::getline(f, ln)) { if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.size() == 1 && ln[0] >= '0' && ln[0] <= '8') c.v = ln[0] - '0'; } }
    if (posCache.size() >= 8) posCache.erase(posCache.begin());
    posCache.push_back(c);
    return c.v;
}
int FirstExtreme::priceEdge()
{
    long n = getBarCount(); if (n < 2) return -1;
    RTARRAY cl(barClose); if ((long)cl.count < n) return -1;
    PNT p; if (p.set((int)(n - 1), cl[(int)(n - 1)], kBarRight) != RTX_OK) return -1;
    return p.h > 0 ? (int)p.h : -1;
}

int FirstExtreme::draw(void)
{
    FXState* S = state(true);
    if (!S) return RTX_OK;
    refresh(*S);
    if (S->mi < 0) return RTX_OK;
    RCT pane; pane.getPaneRect(false);
    fx2::Box pb = { pane.left, pane.top, pane.right, pane.bottom };
    if (!S->have) {
        std::string s = "1st Extreme: " + S->state;
        if (pb.r - pb.l > 200 && pb.b - pb.t > 40) text(pb.l + 8, pb.t + 14, s, FX_GREY, 8, false);
        writeStatus(*S);
        return RTX_OK;
    }
    int markerX = -1;
    if (S->view.called) {
        for (int i = (int)S->ends.size() - 1; i >= 0; i--) if (S->ends[(size_t)i] == S->view.formedEnd) {
            PNT p; if (p.set(i, (float)S->view.price, kBarCenter) == RTX_OK && p.h > 0) markerX = (int)p.h; break; }
    }
    fx2::Measure M = [&](const std::string& s, int fs, bool bold) { return textW(s, fs, bold); };
    std::function<int(double)> yOf = [&](double v) { return (int)getVerticalDrawPosition((float)v); };
    fx2::Layout L = fx2::layoutAll(S->view, pb, M, priceEdge(), position(S->mkt), markerX, yOf);
    execute(L);
    writeStatus(*S);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    FirstExtreme *p = new FirstExtreme();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("LRA 1st Extreme (SHADOW MODE): the probability that today's running LOD / HOD is the session's first final extreme, every 3-min bar from 9 min after the open, and one immutable LOD? / HOD? call when a side reaches 70% before the market's deadline (labelled with the time it was called; a later break marks it FAILED). Reviewed v2 research model: 75% right on 73% of days in its Jul-Sep test, 63% in the later Sep-Oct check - not yet proven. 3-min charts only. Use Settings > Position to place the panel.");
    p->setVersion("1.0.0");
    return p;
}
