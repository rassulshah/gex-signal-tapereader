/********************************************************************************
 *  lsContext 0.1.0 (2026-10-26) -- the SHARED CONTEXT (L2) and TEAM READ (L4) of the smart indicators. Draws NOTHING.
 *
 *  Rassul 2026-10-26 12:25-12:53 CT: "create a shared context ... make sure the indicators are aware of this context and ... aware of
 *  the others' read" / "build the architecture, its layers .. then build the team and the processes of communication".
 *  Design: smart_indicators/TEAM-DESIGN.md (option C, hybrid blackboard; lsContext is the orchestrator).
 *
 *  Per chart, per CLOSED bar t (all the logic is in ContextPump.h / ContextLogic.h, unit-tested on Linux):
 *    - copies the chart's closed bars at calc (CALC_LAST: after his other indicators), then a 250 ms clock re-checks the board
 *      between IRT's calc calls (busy 3-min charts call calc only every ~6-10 s)
 *    - waits until every source present on the chart has posted bar t to MarketState, or the timeout (3 s; LRA-Context-<MKT>.csv
 *      PARAM,TIMEOUT_MS), then builds CTX(t) ONCE from facts decided for <= t only (no look-ahead, never repainted), posts it as
 *      MarketState source "CTX"; UNKNOWN fields are "?", never 0
 *    - TEAM(t) from the teammates' aware reads (AWSTAGE / AWDP / AWAGG / AWHODLOD) decided for t; a conflict takes a side only when
 *      the nightly trust table (LRA-Context-trust-<MKT>.csv) says so
 *    - appends CTX / TEAM lines to lsFlexLevels\Context-<MKT>-<spb>s-<yyyymmdd>.rec (the nightly link test reads them) and writes
 *      Context-<MKT>-<spb>s.status.txt
 *  Links are SWITCHED OFF per market (LRA-Context-<MKT>.csv, default all off): until a link is proven and switched on, no indicator
 *  changes what it shows. The team read is INTERNAL (Rassul 13:28 "no team box"): posted, recorded, read by Claude - never drawn.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>     // BEFORE irtsdk.h (MSVC C2371 RTBOOL clash otherwise)
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0) calc / draw timing, memory, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "Context"
#endif
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "ContextPump.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define CX_VERSION "0.1.0"   // + PerfGuard 1.0.0 (smart step paused while over budget), vocabulary v2 (ContextPump / ContextLogic / ContextRead 0.2.0)
static const int CX_BARS = 400;          // closed bars copied per calc (ATR, legs, the 100-bar ATR base)
static const int CX_TIMER_MS = 250;

static long long cxNowMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string cxDir() { const char* up = std::getenv("USERPROFILE"); return (up && up[0]) ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
static bool cxReadFile(const std::string& p, std::string& out) { std::ifstream f(p.c_str(), std::ios::binary); if (!f) return false; std::stringstream s; s << f.rdbuf(); out = s.str(); return true; }
static bool cxAppend(const std::string& p, const std::string& text) { std::ofstream f(p.c_str(), std::ios::app | std::ios::binary); if (!f) return false; f << text; return f.good(); }
static bool cxWrite(const std::string& p, const std::string& text) {
    const std::string tmp = p + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::trunc | std::ios::binary); if (!f) return false; f << text; if (!f.good()) return false; }
#if defined(_WIN32)
    return MoveFileExA(tmp.c_str(), p.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return std::rename(tmp.c_str(), p.c_str()) == 0;
#endif
}
static int cxTimerId(const void* me) { return 8311 + (int)(((uintptr_t)me >> 4) % 100000); }

struct CXState {
    cp::Pump P;
    bool timerOn = false, timerTried = false, busy = false;
    std::string day;                       // yyyymmdd of the last closed bar (the record file's day)
    long long linksMs = -1, statusMs = -1; std::string linksStamp;
    long ctxN = 0, teamN = 0, writeFails = 0; std::string lastState;
};

class Context : public cppExtension {
public:
    SS_SAFE_DIALOG(Context, "Context")
    Context() : cppExtension() {}
    virtual int draw(void) { return RTX_OK; }                       // draws nothing, ever: the team read is internal (decision 2, no box)
    virtual int timer(RTX_EVENT* e);
    virtual int parmsLoadBody(void)  { return RTX_OK; }             // no settings: the switches live in LRA-Context-<MKT>.csv
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    void pump(bool fromCalc);
    void release() { CXState* S = slot_.get(this, false); if (S) { if (S->timerOn) { destroyTimer(cxTimerId(S)); S->timerOn = false; } S->P.clear(); } slot_.release(this); }
private:
    HostSlot<CXState> slot_;
    void copyBars(CXState& S);
    void loadFiles(CXState& S, long long now);
    void step(CXState& S, long long now);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<Context*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Context*>(this)->release(); return RTX_OK; }
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<Context*>(this)->pump(true); return RTX_OK; }
int cppExtension::setup(void)   { setParameterVersion(1); return RTX_OK; }

int Context::timer(RTX_EVENT* e)
{
    CXState* S = slot_.get(this, false);
    if (!S || !e || e->v.timer.id != cxTimerId(S)) return RTX_FAIL;      // not ours
    pump(false); return RTX_OK;
}

// the closed bars (n - 2 is the last closed; n - 1 is forming), oldest first, with the SignalBoard bar id
void Context::copyBars(CXState& S)
{
    const long n = getBarCount(); S.P.bars.clear(); if (n < 3) return;
    RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume), dt(barDateTime);
    const long last = n - 2, first = last - CX_BARS + 1 < 0 ? 0 : last - CX_BARS + 1;
    S.P.bars.reserve((size_t)(last - first + 1));
    for (long i = first; i <= last; ++i) {
        struct tm t; std::memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        ctx::Bar b; b.t = cp::barId(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; b.v = (double)vo[i]; b.minOfDay = t.tm_hour * 60 + t.tm_min;
        S.P.bars.push_back(b);
        if (i == last) { char d[16]; std::snprintf(d, sizeof(d), "%04d%02d%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday); S.day = d; }
    }
}

// the per-market switches and the trust table, re-read every 30 s (a change applies without restarting IRT)
void Context::loadFiles(CXState& S, long long now)
{
    if (S.linksMs >= 0 && now >= S.linksMs && now - S.linksMs < 30000) return;
    S.linksMs = now; const std::string dir = cxDir(); if (dir.empty()) return;
    std::string text;
    S.P.links = cxReadFile(dir + "\\LRA-Context-" + S.P.market + ".csv", text) ? cr::parseLinks(text, S.P.market) : cr::Links();
    text.clear();
    S.P.trust = cxReadFile(dir + "\\LRA-Context-trust-" + S.P.market + ".csv", text) ? cp::parseTrust(text, S.P.market) : std::map<std::string, double>();
}

void Context::step(CXState& S, long long now)
{
    loadFiles(S, now);
    // the whole of lsContext is a SMART feature: over its perf budget (or the process over memory) it pauses - nothing is built or
    // posted, readers fall back to their isolated reads after the timeout; the status file still says why
    const bool allowed = lsperf::smartAllowed(LSPERF_PLUGIN, S.P.sym);
    cp::Out o; if (allowed) o = S.P.step(now); else o.state = "paused (perf budget)";
    const std::string dir = cxDir();
    const std::string base = dir.empty() ? std::string() : dir + "\\Context-" + S.P.market + "-" + std::to_string(S.P.spb) + "s";
    if (!base.empty() && (!o.ctxLine.empty() || !o.teamLine.empty())) {
        std::string rec; if (!o.ctxLine.empty()) { rec += o.ctxLine + "\n"; ++S.ctxN; } if (!o.teamLine.empty()) { rec += o.teamLine + "\n"; ++S.teamN; }
        if (!cxAppend(base + "-" + S.day + ".rec", rec)) ++S.writeFails;
    }
    S.lastState = o.state;
    if (!base.empty() && (S.statusMs < 0 || now < S.statusMs || now - S.statusMs >= 10000)) {
        S.statusMs = now; std::string on; for (const auto& l : S.P.links.on) if (l.second) on += l.first + " ";
        std::ostringstream st;
        st << "VERSION," << CX_VERSION << "\nMARKET," << S.P.market << "\nSYMBOL," << S.P.sym << "\nSPB," << S.P.spb << "\nBARS," << S.P.bars.size()
           << "\nLAST_CLOSED," << S.P.lastClosed() << "\nCTX_BAR," << S.P.ctxBar << "\nTEAM_BAR," << S.P.teamBar << "\nSTATE," << S.lastState
           << "\nLINKS_ON," << (on.empty() ? "none" : on) << "\nLINKS_FILE," << S.P.links.note << "\nTIMEOUT_MS," << S.P.links.timeoutMs
           << "\nTRUST_ROWS," << S.P.trust.size() << "\nCLOCK," << (S.timerOn ? "on" : "off") << "\nCTX_LINES," << S.ctxN << "\nTEAM_LINES," << S.teamN
           << "\nWRITE_FAILS," << S.writeFails << "\nPERF," << lsperf::statusOf(this) << "\n";
        cxWrite(base + ".status.txt", st.str());
    }
}

void Context::pump(bool fromCalc)
{
    CXState* S = slot_.get(this, true); if (!S || S->busy) return;
    S->busy = true; struct Unbusy { CXState* s; ~Unbusy() { s->busy = false; } } unbusy = { S };
    try {
        const long long now = cxNowMs();
        if (fromCalc) {
            const char* sy = getSymbol(); const char* lab = getChartLabel(); char rb[64]; std::memset(rb, 0, sizeof(rb)); const char* rs = getRootSymbol(rb);
            const std::string sym = sy ? sy : "", chart = lab ? lab : ""; const long spb = getSecondsPerBar();
            if (sym != S->P.sym || chart != S->P.chart || spb != S->P.spb) {      // symbol / period / chart changed: start clean
                S->P.clear(); S->P = cp::Pump(); S->P.sym = sym; S->P.chart = chart; S->P.spb = spb; S->linksMs = -1;
            }
            S->P.market = cp::marketOf(rs ? rs : sym); S->P.rthOpenMin = cp::rthOpenOf(S->P.market);
            const double tk = (double)getProperty(SYM_TICKINCR); if (tk > 0) S->P.tick = tk;
            copyBars(*S);
            if (!S->timerTried && !S->timerOn) { S->timerTried = true; S->timerOn = createTimer(cxTimerId(S), CX_TIMER_MS) == RTX_OK; }
        }
        if (S->P.sym.empty()) return;                                    // the clock before the first calc: nothing copied yet
        step(*S, now);
    } catch (...) { S->lastState = "error (kept the last CTX)"; }
}

extern "C" cppExtension* CreateExtension(void)
{
    Context* p = new Context();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | CALC_LAST);   // CALC_LAST: after the indicators whose facts it reads
    p->setDescription("LRA Context: the shared context of the smart indicators - per closed bar it collects every indicator's facts from the Market State (wave, leg, level, session, trend, VWAP, order flow), posts CTX and the team read, and records both for the nightly link test. Draws nothing; links are off per market until proven.");
    p->setVersion("0.1.0");   // = CX_VERSION
    return p;
}
