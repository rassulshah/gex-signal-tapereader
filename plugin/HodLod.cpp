/********************************************************************************
 *  HodLod.cpp  --  Investor/RT RTX extension  lsHodLod  (1.0.0, 2026-10-09)
 *
 *  THE SESSION'S HOD / LOD, on every market's chart (Rassul 2026-10-09, mockup v5 approved: "check everything then build it").
 *    PIECE 1  the session candle on the LEFT edge of the price pane, at real prices: body RTH open -> last close, wicks to the HOD
 *             and LOD, green when close >= open. Above / below: HOD and LOD with their time, the key level each extreme swept
 *             (PFH / PDH / ONH / LonHI ...), took / size for the first extreme and the gap for the second; inside the body the gap,
 *             the range in points and $, and at the open's edge the reclaim time and the Rec. Took (1st extreme -> reclaim). "pending" until the second
 *             extreme prints.
 *    PIECE 2  the table (mockup v15), bottom, centred-left and clear of the candle: "ES  10:00" + the read ("LOD IN  (model in
 *             test)  -  expect HOD after 11:00 83%"), then 1st Time | Reclaim | 2nd Time for E (expected, grey) and A (actual):
 *             E "8:51 +-30m  either" | "8:51 - 9:21  76%" | "after 11:00 83%  HOD";  A "8:51 LOD  in" | "9:09  in" | "pending".
 *             Every other value (took, size, rec. took, gap, room, range, close, the hle 2.0 model outputs) is in the status file.
 *  Every definition is in HodLodLogic.h (tested against an independent Python reference on every bar of 2026-10-09 ES).
 *  Native: the chart's own closed bars + two generated headers (HodLodExpected.h = the model candles, HodLodReadTables.h = the
 *  read tables, built in when present), HodLodExpectedV2.h (hle 2.0, the studies' conditional model) and HodLodMedians.h (the
 *  typical timing). No file is read at run time. No settings (one invisible output, OUTPUT_NO_UI).
 *  Writes: HodLod.status-<MKT>-<secs>.txt (every value drawn), HodLod.reads-<MKT>-<secs>.csv (the read every closed RTH bar +
 *  the day's OUTCOME line) and, on 3-min charts with the read built in, HodLodRead.reads-<MKT>.csv (the nightly scorer's format).
 ********************************************************************************/
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#include "HostSlot.h"
#include "HodLodLogic.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>

static const COLOR HL_GREY = 0x009CA3AF;

struct HLState {
    std::string root, mkt; int mi = -1;
    long nBars = -1; RTDATE lastStamp = 0, firstStamp = 0; float lastTick = 0;
    long long per = 0; double tick = 0;
    bool have = false;                 // a view was built
    hl::Tracker* T = nullptr;          // the last tracker (owned)
    hl::View view; hl::ReadOut rd;
    std::string state = "starting";
    hl::LogState mine, scorer; bool firstPass = true;
    time_t lastStatusWrite = 0; std::string lastStatus;
    bool faulted = false;
    long long recomputes = 0, statusWrites = 0;   // test counters (checklist #31 / #32)
    // (HL104) MenthorQ level snapshots of MenthorQLevels.csv (the one external input): one per distinct content, <= 32, newest last
    std::vector<hl::MqSnap> mq; long long mqStamp = -2; time_t mqChecked = 0; std::string mqWhy; long long mqLoads = 0;
    ~HLState() { delete T; }
};

class HodLod : public cppExtension {
public:
    HodLod() : cppExtension() {}
    virtual int draw(void);
    HostSlot<HLState> slot_;
    HLState* state(bool create) { return slot_.get(this, create); }
    void clearState() { slot_.release(this); }
    void refresh(HLState& S);
    bool loadMq(HLState& S);
    void recompute(HLState& S, long n);
    void writeFiles(HLState& S, const std::vector<hl::ReadRow>& rows);
    void writeStatus(HLState& S, const hl::Layout* L);
    std::string flexPath(const std::string& name);
    int textW(const std::string& s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s.c_str(), -1); }
    void text(int x, int yc, const std::string& s, COLOR col, int sz, bool bold)
    {   // as SessionInfo: the text's vertical centre on yc
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
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<HodLod*>(this)->clearState(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<HodLod*>(this)->clearState(); return RTX_OK; }
int cppExtension::calc(int)
{
    HodLod* x = static_cast<HodLod*>(this);
    HLState* S = x->state(true);
    if (S) x->refresh(*S);
    return RTX_OK;
}

int cppExtension::setup(void)
{
    // (Rassul 2026-10-09) no settings at all: one invisible value array, hidden from the settings window
    setParameterVersion(1);
    setParameterDialogHeight(1);
    CPEN p(HL_GREY, 1, P_SOLID);
    setOutputParameter("HOD/LOD", DRAW_INVISIBLE, &p, HL_GREY, OUTPUT_NO_UI);
    setArrayDrawingFlags(0, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    return RTX_OK;
}

std::string HodLod::flexPath(const std::string& name)
{
    const char* up = getenv("USERPROFILE");
    if (!up) return "";
#ifdef _WIN32
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\" + name;
#else
    return std::string(up) + "/" + name;     // mock tests on Linux
#endif
}

// the chart -> the market; a symbol change resets everything for this chart (checklist #1 / #30)
void HodLod::refresh(HLState& S)
{
    char rb[32] = { 0 }; const char* rs = getRootSymbol(rb);
    std::string root = rs ? rs : "";
    if (root != S.root) {
        delete S.T; S.T = nullptr;
        S.root = root; S.mkt = hl::marketForRoot(root); S.mi = hl::mktIndex(S.mkt);
        S.mq.clear(); S.mqStamp = -2; S.mqChecked = 0; S.mqWhy.clear();
        S.nBars = -1; S.have = false; S.faulted = false; S.mine = hl::LogState(); S.scorer = hl::LogState(); S.firstPass = true;
        S.state = S.mi < 0 ? "unknown market (root " + root + ")" : "starting";
    }
    if (S.mi < 0 || S.faulted) return;
    if (loadMq(S)) S.nBars = -1;            // (HL104) new MenthorQ levels: rebuild once
    long n = getBarCount();
    if (n < 3) { S.state = "too few bars"; S.have = false; return; }
    RTARRAYI dt(barDateTime);
    if ((long)dt.count < n) { S.state = "timestamp array unavailable"; S.have = false; return; }
    RTDATE last = (RTDATE)dt[(int)n - 1], first = (RTDATE)dt[0];
    float tk = getProperty(SYM_TICKINCR);
    // recompute only when the bars or the tick change (a new bar = the forming bar closed); drawing reuses the result (#32)
    if (n == S.nBars && last == S.lastStamp && first == S.firstStamp && tk == S.lastTick) return;
    S.nBars = n; S.lastStamp = last; S.firstStamp = first; S.lastTick = tk;
    try { recompute(S, n); }
    catch (const std::exception& e) { S.faulted = true; S.have = false; S.state = std::string("fault: ") + e.what() + " (cleared on symbol change)"; }
    catch (...) { S.faulted = true; S.have = false; S.state = "fault (cleared on symbol change)"; }
}

// (HL104) MenthorQLevels.csv (lsFlexLevels, written by the MenthorQ bridge, read by lsDealerProfile too): stat at most every 5 s,
// read only when the stamp changes, at most 64 KiB, strict parse; a changed level set becomes a new snapshot stamped with the file's
// write time (local CT). Missing / oversized / garbage -> no new snapshot (a stale one from an earlier session is never used).
static long long hlLocalAbs(time_t t)
{
    struct tm lt; memset(&lt, 0, sizeof lt);
#ifdef _WIN32
    if (localtime_s(&lt, &t) != 0) return 0;
#else
    if (!localtime_r(&t, &lt)) return 0;
#endif
    return hl::daysFromCivil(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday) * 86400 + lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec;
}
bool HodLod::loadMq(HLState& S)
{
    time_t now = time(nullptr);
    if (S.mqStamp != -2 && now - S.mqChecked < 5) return false;
    S.mqChecked = now;
    std::string p = flexPath("MenthorQLevels.csv");
    if (p.empty()) { S.mqWhy = "MenthorQ levels: no profile folder"; return false; }
#ifdef _WIN32
    struct _stat64 st; bool ok = _stat64(p.c_str(), &st) == 0;
#else
    struct stat st; bool ok = stat(p.c_str(), &st) == 0;
#endif
    long long stamp = ok ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
    if (stamp == S.mqStamp) return false;
    S.mqStamp = stamp;
    if (!ok) { S.mqWhy = "MenthorQLevels.csv missing: key levels only"; return true; }
    if ((long long)st.st_size > (long long)hl::MQ_MAX_BYTES) { S.mqWhy = "MenthorQLevels.csv over 64 KiB: ignored"; return true; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) { S.mqWhy = "MenthorQLevels.csv unreadable"; return true; }
        text.resize(hl::MQ_MAX_BYTES + 1); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); }
    S.mqLoads++;
    std::vector<hl::MqLevel> lv; std::string why;
    double tick = (double)getProperty(SYM_TICKINCR); tick = std::round(tick * 1e9) / 1e9;
    if (!hl::parseMq(text, S.mkt, S.root, tick, lv, why)) { S.mqWhy = why + ": key levels only"; return true; }
    S.mqWhy = why;
    bool same = !S.mq.empty() && S.mq.back().lv.size() == lv.size();
    for (size_t i = 0; same && i < lv.size(); i++) same = lv[i].code == S.mq.back().lv[i].code && lv[i].px == S.mq.back().lv[i].px && lv[i].col == S.mq.back().lv[i].col;
    if (same) return false;
    hl::MqSnap sn; sn.atAbs = hlLocalAbs((time_t)st.st_mtime); sn.lv = lv;
    S.mq.push_back(sn);
    if (S.mq.size() > 32) S.mq.erase(S.mq.begin());
    return true;
}

void HodLod::recompute(HLState& S, long n)
{
    S.recomputes++;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) { S.state = "chart tick size unavailable"; S.have = false; return; }
    tick = std::round(tick * 1e9) / 1e9;      // the float property, back to its decimal value (0.0005 not 0.000500000024)
    RTARRAYI dt(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) { S.state = "price arrays unavailable"; S.have = false; return; }
    auto absOf = [&](int i) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        return hl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday) * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    };
    // bar size from the chart's own stamps (bar END): the smallest positive gap of the last 40 bars
    long long per = 0, prev = absOf((int)n - 1);
    for (int i = (int)n - 2; i >= 0 && i >= (int)n - 41; i--) { long long a = absOf(i), d = prev - a; if (d > 0 && (per == 0 || d < per)) per = d; prev = a; }
    if (per <= 0) { S.state = "bar size unknown"; S.have = false; return; }
    if (per > 900) { S.state = "needs bars of 15 minutes or less"; S.have = false; return; }
    // the closed bars (a later bar exists: all but the last) back to 21 calendar days (time cutoff, #23), at most 40,000 bars (#40)
    const int lastClosed = (int)n - 2;
    long long cutoff = absOf(lastClosed) - 21LL * 86400;
    int start = lastClosed;
    while (start > 0 && lastClosed - start < 40000) { long long a = absOf(start - 1); if (a < cutoff) break; start--; }
    std::vector<hl::Bar> B; B.reserve((size_t)(lastClosed - start + 1));
    RTARRAYI vo(barVolume);                   // (HL101) volume for hlr::timing's VWAP trend; missing -> equal weights, flagged
    const bool haveVol = (long)vo.count >= n;
    for (int i = start; i <= lastClosed; i++) {
        hl::Bar b; b.absEnd = absOf(i); b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i];
        b.v = haveVol ? (double)vo[i] : NAN;
        B.push_back(b);
    }
    hl::Cfg c; c.mkt = S.mi; c.per = per; c.tick = tick; c.readOpen = hl::readOpenFor(S.mi); c.readN = hl::readNFor(S.mi);
    std::vector<hl::ReadRow> rows;
    hl::Tracker T = hl::runBars(c, B, B.size(), &rows);
    hl::ReadOut rd = rows.empty() ? hl::ReadOut() : rows.back().out;
    S.view = hl::buildView(T, rd, &rows, &S.mq);
    if (!S.mqWhy.empty()) S.view.mqWhy = S.mqWhy + (S.view.mqWhy.empty() ? "" : "; " + S.view.mqWhy);
    S.rd = rd; S.per = per; S.tick = tick; S.have = S.view.has || !S.view.header.empty();
    delete S.T; S.T = new hl::Tracker(T);
    S.state = S.view.has ? (S.view.prior ? "prior session shown" : (S.view.complete ? "RTH complete" : "RTH live")) : "waiting for the RTH open";
    if (start > 0 && lastClosed - start >= 40000) S.state += " (history capped at 40,000 bars)";
    writeFiles(S, rows);
    S.firstPass = false;
}

// the read log (every closed RTH bar's read, live or replayed after a restart) + the OUTCOME line; marks only after a good write (#6)
void HodLod::writeFiles(HLState& S, const std::vector<hl::ReadRow>& rows)
{
    if (rows.empty() && !(S.view.complete && !S.view.prior)) return;
    hl::FileOps F;
    std::string session = rows.empty() ? S.view.session : rows[0].session;
    std::string mine = flexPath("HodLod.reads-" + S.mkt + "-" + std::to_string(S.per) + ".csv");
    if (mine.empty()) return;
    if (!S.mine.primed || S.mine.session != session) {
        if (!F.exists(mine)) F.write(mine, std::string(hl::HL_LOG_HEADER) + "\n", true);
        hl::primeFromFile(S.mine, mine, session, 4, 5, "OUTCOME");
    }
    int dec = hl::MKTS[S.mi].dec;
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i].barEnd <= S.mine.lastBarEnd) continue;
        if (!F.write(mine, hl::readLogLine(S.mkt, (int)S.per, rows[i], !S.firstPass, S.tick, dec) + "\n", true)) break;
        S.mine.lastBarEnd = rows[i].barEnd;
    }
    if (S.view.complete && !S.view.prior && S.view.session == session && !S.mine.outcomeDone)
        if (F.write(mine, hl::outcomeLine(S.view, (int)S.per, S.tick) + "\n", true)) { S.mine.outcomeDone = true; S.mine.lastBarEnd = S.view.lastMod; }
#if HL_HAVE_READ
    // the nightly scorer's own file (hodlod_read.py LOG_COLS) - the read model is fitted on 3-min bars, so only 3-min charts write it
    if (S.per == 180 && hl::readSessionMatches(S.mi)) {
        std::string sp = flexPath("HodLodRead.reads-" + S.mkt + ".csv");
        if (!S.scorer.primed || S.scorer.session != session) hl::primeFromFile(S.scorer, sp, session, 1, 2, nullptr);
        for (size_t i = 0; i < rows.size(); i++) {
            if (rows[i].barEnd <= S.scorer.lastBarEnd || rows[i].out.state != 1) continue;
            if (!F.write(sp, hl::scorerLine(S.mkt, rows[i]) + "\n", true)) break;
            S.scorer.lastBarEnd = rows[i].barEnd;
        }
    }
#endif
}

// the status file: every value drawn, keyed by market AND seconds per bar (#9); written on change, at most every 2 s, and
// every 30 s regardless; atomic (#5)
void HodLod::writeStatus(HLState& S, const hl::Layout* L)
{
    if (S.mi < 0) return;
    std::string text;
    if (S.T && L) text = hl::statusText(S.view, *L, *S.T, S.rd, (int)S.per, S.state);
    else text = std::string("VERSION,") + hl::HL_VERSION + "\nMARKET," + S.mkt + "\nPER," + std::to_string(S.per) + "\nSTATE," + S.state + "\n";
    time_t now = time(nullptr);
    bool changed = text != S.lastStatus;
    if (!(changed && now - S.lastStatusWrite >= 2) && now - S.lastStatusWrite < 30) return;
    std::string p = flexPath("HodLod.status-" + S.mkt + "-" + std::to_string(S.per) + ".txt");
    if (p.empty()) return;
    hl::FileOps F;
    if (hl::writeAtomic(F, p, text)) { S.lastStatus = text; S.lastStatusWrite = now; S.statusWrites++; }
}

void HodLod::execute(const hl::Layout& L)
{
    for (size_t i = 0; i < L.items.size(); i++) {
        const hl::Item& it = L.items[i];
        switch (it.k) {
            case hl::K_FILL: { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw(0, (COLOR)it.c1, (COLOR)it.c1, DRAW_OPAQUE, PAT_SOLID); break; }
            case hl::K_RECT:
                if (it.c2 == 0xFFFFFFFFu) {       // hollow (the candle body): four lines
                    line(it.l, it.t, it.r, it.t, (COLOR)it.c1, it.w); line(it.r, it.t, it.r, it.b, (COLOR)it.c1, it.w);
                    line(it.r, it.b, it.l, it.b, (COLOR)it.c1, it.w); line(it.l, it.b, it.l, it.t, (COLOR)it.c1, it.w);
                } else { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw((short)it.w, (COLOR)it.c1, (COLOR)it.c2, DRAW_OPAQUE, PAT_SOLID); }
                break;
            case hl::K_LINE: line(it.l, it.t, it.r, it.b, (COLOR)it.c1, it.w); break;
            case hl::K_TEXT: text(it.l, (it.t + it.b) / 2, it.s, (COLOR)it.c1, it.fs, it.bold); break;
            default: break;
        }
    }
}

int HodLod::draw(void)
{
    HLState* S = state(true);
    if (!S) return RTX_OK;
    refresh(*S);
    if (S->mi < 0) return RTX_OK;
    RCT pane; pane.getPaneRect(false);
    hl::Box pb = { pane.left, pane.top, pane.right, pane.bottom };
    if (!S->have || !S->T) {
        std::string s = "HOD/LOD: " + S->state;
        if (pb.r - pb.l > 200 && pb.b - pb.t > 40) text(pb.l + 8, pb.b - 14, s, HL_GREY, 8, false);
        writeStatus(*S, nullptr);
        return RTX_OK;
    }
    const double tick = S->tick;
    auto yOf = [&](long long ticks) { return (int)getVerticalDrawPosition((float)((double)ticks * tick)); };
    hl::Measure M = [&](const std::string& s, int fs, bool bold) { return textW(s, fs, bold); };
    hl::Layout L = hl::layoutAll(S->view, pb, yOf, M);
    execute(L);
    writeStatus(*S, &L);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    HodLod *p = new HodLod();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("LRA HOD / LOD: the RTH session as one candle at real prices on the left (HOD / LOD, the level each swept, took, size, reclaim, wick, gap, range) and a table comparing it with the research model candle, with the live read on top.");
    p->setVersion("1.0.0");
    return p;
}
