/********************************************************************************
 *  HodLod.cpp  --  Investor/RT RTX extension  lsHodLod  (1.0.0, 2026-10-09; 1.1.8 2026-10-12: the day's first IN call ledger; 1.1.9: "called h:mm", "Range used")
 *  2.0.4 (2026-10-13): the projection callout ("HOD after 10:45 . 94.34") keeps out of the left level-label band (the Liquidity
 *         column's level names, e.g. "/PDH") - max(110 px, 15% of the pane); it moves right of the band, never over a level label.
 *  2.0.3 (2026-10-13 10:38): the table 33% smaller ("reduce the size of this by 33%. too big"): 9 pt instead of 13, padding x 2/3.
 *  2.0.2 (2026-10-13): Rassul's table redesign (mockups a-u): Regime / RTH / Expiry header aligned to the columns, Reclaim column gone,
 *         Expected (blue) / Actual ("LOD IN 8:39 (71%)") / Estimate rows, odds in (), bigger fonts, no 1st-extreme line, projection label on
 *         its arrowhead; Session Info retired (its Regime + Expiry are read here from LRA-Session-<MKT>.csv)
 *  2.0.1 (2026-10-13): audit fixes - RTH-completion and tick-admission checks (external audit 2026-10-12, HodLod 1.1.1)
 *  2.0.0 (2026-10-12 night): the 1st Extreme model (was lsFirstExtreme) merged in: a 3rd table line + the LOD? / HOD? marker, same shadow logs
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
#include "HodLodFx.h"            // (2.0.0) the 1st Extreme model, merged in (was the separate lsFirstExtreme)
#include <sys/stat.h>
#include <fstream>
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
    int projIdx = -1;                  // (1.1.0) the chart bar index of the 1st extreme (the projection line's x), -1 = none
    // (2.0.0) the 1st Extreme model (fx2), 3-min charts only: its view, the chart index of the called candle, its shadow logs
    fx2::View fxView; bool fxHave = false; std::string fxState = "not run"; int fxIdx = -1;
    long long fxLogDay = -1, fxCallDay = -1; int fxLogElapsed = -1; time_t fxStatusWrite = 0; std::string fxLastStatus;
    std::string fcSession;                 // (1.1.8) the session whose first IN call is already in HodLod.firstcall-<MKT>.csv
    std::string projLogged, projSession;   // (1.1.6) the targets already in HodLod.proj-<MKT>.csv for projSession (\n-separated keys)
    // (HL104) MenthorQ level snapshots of MenthorQLevels.csv (the one external input): one per distinct content, <= 32, newest last
    std::vector<hl::MqSnap> mq; long long mqStamp = -2; time_t mqChecked = 0; std::string mqWhy; long long mqLoads = 0;
    // (2.0.2, Rassul 05:52 / 06:08: Session Info retired, its Regime + options Expiry move into the table's header) the bridge's
    // LRA-Session-<MKT>.csv: GAM|POS|NEG ... and EXP|hh:mm ... (bounded read, re-read only when the file changes, at most every 5 s)
    long long sessStamp = -2; time_t sessChecked = 0; std::string sessGam, sessExp; int sessAge = -1; bool sessRec = true;
    ~HLState() { delete T; }
};

class HodLod : public cppExtension {
public:
    HodLod() : cppExtension() {}
    virtual int draw(void);
    // (HL105, Rassul "you should have setting where to put them like upper left etc.") ONE setting: Position (9 spots), default
    // Bottom centre. IRT can return -1 for a LIST parameter outside its settings copy, so the choice is also kept per market in
    // HodLod.place-<MKT>.txt - written only from the dialog's callbacks (loading never saves, #29) and read back by stamp.
    virtual int parmsLoad(void) { return RTX_OK; }
    virtual int parmsApply(void) { savePos(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { savePos(); return RTX_OK; }
    int posParam = 0;
    struct PosCache { std::string path; long long stamp = -2; int v = hl::POS_DEFAULT; };
    std::vector<PosCache> posCache;                      // per object (no static state), <= 8 markets
    void savePos();
    int position(const std::string& mkt);
    int priceEdge();
    HostSlot<HLState> slot_;
    HLState* state(bool create) { return slot_.get(this, create); }
    void clearState() { slot_.release(this); }
    void refresh(HLState& S);
    bool loadMq(HLState& S);
    void loadSession(HLState& S);
    void recompute(HLState& S, long n);
    void writeFiles(HLState& S, const std::vector<hl::ReadRow>& rows);
    void writeProj(HLState& S);
    void writeFirstCall(HLState& S, const std::vector<hl::ReadRow>& rows);
    void writeStatus(HLState& S, const hl::Layout* L);
    void computeFx(HLState& S, long n, long long per);
    void writeFxLogs(HLState& S);
    void writeFxStatus(HLState& S);
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
    // (HL105) the only setting: Position (9 spots, default Bottom centre); the value array stays hidden from the settings window
    setParameterVersion(3);   // (1.0.1) 3: IRT resets a stale saved Position (Top left) to the default
    setParameterDialogHeight(1);
    static_cast<HodLod*>(this)->posParam = 0;
    setListParameter("Position", (short)hl::POS_DEFAULT, hl::POS_CHOICES);
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

void HodLod::loadSession(HLState& S)
{
    time_t now = time(nullptr);
    if (S.sessStamp != -2 && now - S.sessChecked < 5) return;
    S.sessChecked = now;
    std::string p = flexPath("LRA-Session-" + S.mkt + ".csv");
    if (p.empty()) { S.sessGam.clear(); S.sessExp.clear(); S.sessStamp = -1; return; }
#ifdef _WIN32
    struct _stat64 st; bool ok = _stat64(p.c_str(), &st) == 0;
#else
    struct stat st; bool ok = stat(p.c_str(), &st) == 0;
#endif
    long long stamp = ok ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
    if (stamp == S.sessStamp) return;
    S.sessStamp = stamp; S.sessGam.clear(); S.sessExp.clear(); S.sessAge = -1; S.sessRec = true;
    if (!ok || (long long)st.st_size > 64 * 1024) return;
    std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) { S.sessStamp = -2; return; }
    std::string line; int nl = 0;
    while (nl++ < 400 && std::getline(f, line)) {
        if (line.size() > 512) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> c; size_t a = 0;
        for (;;) { size_t b = line.find('|', a); c.push_back(line.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
        if (c.size() >= 2 && c[0] == "GAM" && (c[1] == "POS" || c[1] == "NEG")) S.sessGam = c[1];
        else if (c.size() >= 2 && c[0] == "EXP" && c[1].size() == 5 && c[1][2] == ':') S.sessExp = c[1];
        else if (c.size() >= 3 && c[0] == "DAGE") { char* e = nullptr; long v = strtol(c[1].c_str(), &e, 10); if (e && *e == 0) S.sessAge = (int)v; S.sessRec = c[2].empty() || c[2] == "OK"; }
    }
}

// (2.0.2) the header's Regime / Expiry segments, rebuilt every draw so the countdown stays current (CT clock of this PC)
static void hlSessSegs(HLState& S)
{
    S.view.sessSegs.clear();
    if (!S.sessGam.empty()) {
        const bool neg = S.sessGam == "NEG";
        S.view.sessSegs.push_back(hl::Seg{ neg ? "Regime: Negative" : "Regime: Positive", neg ? hl::C_RED : hl::C_GREEN });
    }
    if (S.sessExp.size() == 5) {
        int e = atoi(S.sessExp.substr(0, 2).c_str()) * 60 + atoi(S.sessExp.substr(3, 2).c_str());
        time_t t = time(nullptr); struct tm lt;
#ifdef _WIN32
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        int now = lt.tm_hour * 60 + lt.tm_min, left = e - now; if (now >= 17 * 60) left = e + 1440 - now;
        char b[64]; int h12 = (e / 60) % 12 == 0 ? 12 : (e / 60) % 12;
        if (left > 0) snprintf(b, sizeof b, "Expiry %d:%02d %s (in %dh %02dm)", h12, e % 60, e >= 720 ? "PM" : "AM", left / 60, left % 60);
        else snprintf(b, sizeof b, "Expiry %d:%02d %s (expired)", h12, e % 60, e >= 720 ? "PM" : "AM");
        S.view.sessSegs.push_back(hl::Seg{ b, left > 0 && left <= 60 ? hl::C_AMBER : hl::C_GREY });
    }
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
    hl::ProjLock lock;   // (1.1.6) the target stays where it was set until the 1st extreme is taken out
    hl::Tracker T = hl::runBars(c, B, B.size(), &rows, &lock);
    hl::ReadOut rd = rows.empty() ? hl::ReadOut() : rows.back().out;
    S.view = hl::buildView(T, rd, &rows, &S.mq, &lock);
    computeFx(S, n, per);                     // (2.0.0) the 1st Extreme model on the same chart (this recompute's bar size)
    S.view.fxSegs = hlfx::headerLine(S.fxView, S.fxHave);
    S.projIdx = -1;                           // (1.1.0) the 1st extreme's bar on this chart, matched by its END stamp
    if (S.view.projOk) for (size_t j = B.size(); j-- > 0;) if (B[j].absEnd == S.view.projFromEnd) { S.projIdx = start + (int)j; break; }
    if (!S.mqWhy.empty()) S.view.mqWhy = S.mqWhy + (S.view.mqWhy.empty() ? "" : "; " + S.view.mqWhy);
    S.rd = rd; S.per = per; S.tick = tick; S.have = S.view.has || !S.view.header.empty();
    delete S.T; S.T = new hl::Tracker(T);
    S.state = S.view.has ? (S.view.prior ? "prior session shown" : (S.view.complete ? "RTH complete" : "RTH live")) : "waiting for the RTH open";
    if (start > 0 && lastClosed - start >= 40000) S.state += " (history capped at 40,000 bars)";
    writeFiles(S, rows);
    writeProj(S);
    writeFirstCall(S, rows);
    S.firstPass = false;
}

// the read log (every closed RTH bar's read, live or replayed after a restart) + the OUTCOME line; marks only after a good write (#6)
// (1.1.5, Rassul 2026-10-12 "add updated ... time ... error ... you should also be tracking it for improvement purposes"): one line per
// change of the projected target (side or price) on the live session, 3-min charts only - what the chart showed and when. The
// nightly loop scores each target against the day's final HOD / LOD (error, updates, side flips) to refine the range model.
void HodLod::writeProj(HLState& S)
{
    const hl::View& v = S.view;
    if (S.per != 180 || S.mi < 0 || v.prior || !v.projOk || !(S.tick > 0)) return;
    const int dec = hl::MKTS[S.mi].dec;
    std::string tgt = hl::trimNum((double)v.projTo * S.tick, dec);
    // (1.1.6) one line per locked target: session | side | from | target; every key already in the file for this session is
    // skipped, so a restart (or a replay of the day) never writes a line twice
    std::string key = "\n" + v.session + "|" + (v.projUp ? "HOD" : "LOD") + "|" + std::to_string(v.projFrom) + "|" + tgt + "\n";
    std::string p = flexPath("HodLod.proj-" + S.mkt + ".csv"); if (p.empty()) return;
    if (S.projSession != v.session) {
        S.projSession = v.session; S.projLogged = "\n";
        std::ifstream f(p.c_str()); std::string ln;
        while (std::getline(f, ln)) if (ln.compare(0, v.session.size() + 1, v.session + ",") == 0) {
            std::vector<std::string> c; size_t a = 0, b; while ((b = ln.find(',', a)) != std::string::npos) { c.push_back(ln.substr(a, b - a)); a = b + 1; } c.push_back(ln.substr(a));
            double fr = 0; if (c.size() >= 5 && std::sscanf(c[3].c_str(), "%lf", &fr) == 1)
                S.projLogged += c[0] + "|" + c[2] + "|" + std::to_string((long long)std::llround(fr / S.tick)) + "|" + c[4] + "\n";
        }
    }
    if (S.projLogged.find(key) != std::string::npos) return;
    hl::FileOps F;
    if (!F.exists(p)) F.write(p, "session,bar_end,side,from,target,exp_range,range_so_far,stage,model,version\n", true);
    char re[32]; snprintf(re, sizeof re, "%.6g", v.projRange);
    int at = v.projLockMod >= 0 ? v.projLockMod : v.lastMod;   // the bar the target was set on
    std::string ln = v.session + "," + hl::hhmm(at) + "," + (v.projUp ? "HOD" : "LOD") + "," + hl::trimNum((double)v.projFrom * S.tick, dec) + "," + tgt + "," + re + "," +
        hl::trimNum((double)v.rangeT * S.tick, dec) + "," + std::to_string(v.projStage) + "," + (v.projV2 ? "v2" : "v1") + "," + hl::HL_VERSION + "\n";
    if (F.write(p, ln, true)) S.projLogged += key.substr(1);
}

// (1.1.8) the day's FIRST IN call, once per session (3-min charts): immutable - a session already in the file is never written again,
// so a restart, a replay or a later stronger call cannot change it. The nightly scores it against the day's exact first extreme and
// counts the sessions with no call (coverage).
void HodLod::writeFirstCall(HLState& S, const std::vector<hl::ReadRow>& rows)
{
    if (S.per != 180 || S.mi < 0 || rows.empty() || !(S.tick > 0)) return;
    if (S.fcSession == rows[0].session) return;          // this session's call is already on file
    hl::FirstCall f = hl::firstCall(rows);
    if (!f.ok) return;
    std::string p = flexPath("HodLod.firstcall-" + S.mkt + ".csv"); if (p.empty()) return;
    hl::FileOps F;
    if (F.exists(p)) {
        std::ifstream in(p.c_str()); std::string ln;
        while (std::getline(in, ln)) if (ln.compare(0, f.session.size() + 1, f.session + ",") == 0) { S.fcSession = f.session; return; }
    } else if (!F.write(p, std::string(hl::FIRSTCALL_HEADER) + "\n", true)) return;
    if (F.write(p, hl::firstCallLine(f, S.tick, hl::MKTS[S.mi].dec), true)) S.fcSession = f.session;
}

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

// ------------------------------------------------------------------------------------------------ (2.0.0) the 1st Extreme model
static double fxRound(double v) { return std::round(v * 1e7) / 1e7; }   // the chart's float prices back to their decimal values
static std::string fxDayTxt(long long day)
{
    int y, m, d; hl::civilFromDays(day, y, m, d); char b[16]; snprintf(b, sizeof b, "%04d-%02d-%02d", y, m, d); return b;
}
// what lsFirstExtreme 1.0.1 did: 3-min charts, the closed bars back to 45 calendar days (14 complete sessions + the overnight),
// prices on the instrument's ticks (1.0.1 rounded to 1e-7 only), strictly increasing stamps; fx2::buildView for the last closed bar's day; the shadow logs once per bar
void HodLod::computeFx(HLState& S, long n, long long per)
{
    S.fxHave = false; S.fxIdx = -1;
    const int m = hlfx::fxMarket(S.mkt);
    if (m < 0) { S.fxState = "market not in the 1st Extreme model"; return; }
    if (per != 180) { S.fxState = "3-minute charts only (the model's bars)"; return; }
    RTARRAYI dt(barDateTime), vol(barVolume); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)dt.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) { S.fxState = "price arrays unavailable"; return; }
    if ((long)vol.count < n) { S.fxState = "volume unavailable (the model needs volume)"; return; }
    auto absOf = [&](int i) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        return fx2::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday) * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    };
    const int lastClosed = (int)n - 2;
    if (lastClosed < 1) { S.fxState = "too few bars"; return; }
    // (2.0.0) prices back to the instrument's own ticks: the chart's float 4135.6 is 4135.60009765625 and a 1e-7 rounding keeps that
    // error (GC / HG / NG / 6E), which moved two NG calls in the parity replay; the model was fitted on tick prices
    const double tk = fx2::MKTS[m].tick;
    auto onTick = [&](double v) { return fxRound(std::round(v / tk) * tk); };
    long long cutoff = absOf(lastClosed) - 45LL * 86400;
    int start = lastClosed; while (start > 0 && lastClosed - start < 60000) { if (absOf(start - 1) < cutoff) break; start--; }
    std::vector<fx2::Bar> B; B.reserve((size_t)(lastClosed - start + 1));
    std::vector<long long> ends; ends.reserve((size_t)(lastClosed - start + 1));
    for (int i = start; i <= lastClosed; i++) {
        fx2::Bar b; b.tEnd = absOf(i); b.o = onTick(op[i]); b.h = onTick(hi[i]); b.l = onTick(lo[i]); b.c = onTick(cl[i]); b.v = (double)vol[i];
        if (!B.empty() && b.tEnd <= B.back().tEnd) { ends.push_back(0); continue; }
        B.push_back(b); ends.push_back(b.tEnd);
    }
    if (B.empty()) { S.fxState = "no closed bars"; return; }
    long long day = fx2::floorDiv(B.back().tEnd, 86400);
    S.fxView = fx2::buildView(m, B, day);
    S.fxHave = true;
    S.fxState = S.fxView.state == fx2::ST_UNAVAILABLE ? S.fxView.why : "ok";
    if (S.fxView.called) for (size_t j = ends.size(); j-- > 0;) if (ends[j] == S.fxView.formedEnd) { S.fxIdx = start + (int)j; break; }
    writeFxLogs(S);
}
static bool fxAppend(const std::string& p, const std::string& header, const std::string& text)
{
    bool fresh = false; { std::ifstream t(p.c_str()); fresh = !t.good(); }
    std::ofstream f(p.c_str(), std::ios::out | std::ios::app | std::ios::binary);
    if (!f.is_open()) return false;
    if (fresh) f << header;
    f << text; f.flush(); return f.good();
}
// the shadow logs the nightly scorer reads (same files and lines as lsFirstExtreme): each assessed bar once, the call once per day;
// primed from the file after a restart so the day's lines already there are never repeated
void HodLod::writeFxLogs(HLState& S)
{
    const fx2::View& V = S.fxView;
    if (V.decisions.empty() || V.day < 0) return;
    std::string dayTxt = fxDayTxt(V.day);
    std::string pd = flexPath("FirstExtreme.decisions-" + S.mkt + ".csv"), pc = flexPath("FirstExtreme.calls-" + S.mkt + ".csv");
    if (pd.empty()) return;
    if (S.fxLogDay != V.day) {
        S.fxLogDay = V.day; S.fxLogElapsed = -1; S.fxCallDay = -1;
        std::ifstream f(pd.c_str()); std::string ln;
        while (std::getline(f, ln)) if (ln.compare(0, 10, dayTxt) == 0 && ln.size() > 11) { int e = atoi(ln.c_str() + 11); if (e > S.fxLogElapsed) S.fxLogElapsed = e; }
        std::ifstream g(pc.c_str()); while (std::getline(g, ln)) if (ln.compare(0, 10, dayTxt) == 0) S.fxCallDay = V.day;
    }
    std::string add; int maxE = S.fxLogElapsed;
    for (const auto& D : V.decisions) if (D.elapsed > S.fxLogElapsed) { add += fx2::decisionLine(V, D, dayTxt); maxE = (std::max)(maxE, D.elapsed); }
    if (!add.empty() && fxAppend(pd, fx2::DECISION_HEADER, add)) S.fxLogElapsed = maxE;
    if (V.called && S.fxCallDay != V.day && fxAppend(pc, fx2::CALL_HEADER, fx2::callLine(V, dayTxt))) S.fxCallDay = V.day;
}
void HodLod::writeFxStatus(HLState& S)
{
    if (S.mi < 0) return;
    std::string t = std::string("VERSION,") + fx2::FX_VERSION + " (inside lsHodLod " + hl::HL_VERSION + ")\nMODEL," + fx2::FX_MODEL + "\nMARKET," + S.mkt + "\nSTATE," + S.fxState + "\n";
    if (S.fxHave) {
        const fx2::View& V = S.fxView; char b[512];
        snprintf(b, sizeof b, "VIEW,state=%d,elapsed=%d,p_low=%.4f,p_high=%.4f,p_in=%.4f,low=%.10g,high=%.10g,called=%d,side=%d,price=%.10g,prob=%.4f,formed=%d,declared=%d,failed=%d,failed_at=%d,verdict=%d\n",
                 (int)V.state, V.elapsed, V.pLow, V.pHigh, V.pIn, V.lo, V.hi, (int)V.called, V.side, V.price, V.prob, V.formed, V.declared, (int)V.failed, V.failedAt, V.verdictKnown ? V.verdict : 9);
        t += b;
        std::string line; for (const auto& g : S.view.fxSegs) line += g.s; t += "LINE," + line + "\n";
    }
    time_t now = time(nullptr);
    if (t == S.fxLastStatus && now - S.fxStatusWrite < 30) return;
    if (now - S.fxStatusWrite < 2) return;
    std::string p = flexPath("FirstExtreme.status-" + S.mkt + ".txt"); if (p.empty()) return;
    hl::FileOps F;
    if (hl::writeAtomic(F, p, t)) { S.fxLastStatus = t; S.fxStatusWrite = now; }
}

void HodLod::writeStatus(HLState& S, const hl::Layout* L)
{
    if (S.mi < 0) return;
    std::string text;
    if (S.T && L) text = hl::statusText(S.view, *L, *S.T, S.rd, (int)S.per, S.state) + "CHART," + S.root + "," + std::to_string(S.per) + "\n";   // (HL105) chart identity for SessionInfo
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

static long long hlFileStamp(const std::string& p)
{
#ifdef _WIN32
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return -1;
#endif
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}
void HodLod::savePos()
{
    int v = getListIndex(posParam);
    if (v < 0 || v > 8) return;                                        // not this dialog's value: never overwrite a saved choice
    char rb[32] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = hl::marketForRoot(rs ? rs : "");
    if (mkt.empty()) return;
    std::string p = flexPath("HodLod.place-" + mkt + ".txt"); if (p.empty()) return;
    hl::FileOps F; hl::writeAtomic(F, p, std::to_string(v) + "\n");
}
int HodLod::position(const std::string& mkt)
{
    // (1.0.1, Rassul 2026-10-11 "its default is bottom center. it is moving to upper left") the per-market place file is the ONLY
    // source while drawing: IRT hands a freshly added copy a stale list index (0 = Top left) that the file never had. The file is
    // written only when he presses OK / Apply in Settings; no file = Bottom centre.
    std::string p = flexPath("HodLod.place-" + mkt + ".txt");
    if (mkt.empty() || p.empty()) return hl::POS_DEFAULT;
    long long st = hlFileStamp(p);
    for (size_t i = 0; i < posCache.size(); i++) if (posCache[i].path == p) { if (posCache[i].stamp == st) return posCache[i].v; posCache.erase(posCache.begin() + (long)i); break; }
    PosCache c; c.path = p; c.stamp = st; c.v = hl::POS_DEFAULT;
    if (st >= 0) {
        std::ifstream f(p.c_str()); std::string ln;
        if (std::getline(f, ln)) { if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
            if (ln.size() == 1 && ln[0] >= '0' && ln[0] <= '8') c.v = ln[0] - '0'; }      // strict: one digit 0-8, else the default
    }
    if (posCache.size() >= 8) posCache.erase(posCache.begin());
    posCache.push_back(c);
    return c.v;
}
// the last price bar's right edge (x): where the usable area ends (never beyond the last bar); -1 = unknown
int HodLod::priceEdge()
{
    long n = getBarCount(); if (n < 2) return -1;
    RTARRAY cl(barClose); if ((long)cl.count < n) return -1;
    PNT p; if (p.set((int)(n - 1), cl[(int)(n - 1)], kBarRight) != RTX_OK) return -1;
    return p.h > 0 ? (int)p.h : -1;
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
        writeFxStatus(*S);
        return RTX_OK;
    }
    const double tick = S->tick;
    auto yOf = [&](long long ticks) { return (int)getVerticalDrawPosition((float)((double)ticks * tick)); };
    hl::Measure M = [&](const std::string& s, int fs, bool bold) { return textW(s, fs, bold); };
    int projX = -1;                           // (1.1.0) the projection line's x: the 1st extreme's bar centre
    if (S->view.projOk && S->projIdx >= 0 && S->projIdx < (int)getBarCount()) {
        PNT p; if (p.set(S->projIdx, (float)((double)S->view.projFrom * tick), kBarCenter) == RTX_OK && p.h > 0) projX = (int)p.h;
    }
    loadSession(*S); hlSessSegs(*S);         // (2.0.2) Regime / Expiry in the header (Session Info retired)
    hl::Layout L = hl::layoutAll(S->view, pb, yOf, M, priceEdge(), position(S->mkt), projX);
    if (S->fxHave && S->fxView.called && S->fxIdx >= 0 && S->fxIdx < (int)getBarCount()) {   // (2.0.0) the LOD? / HOD? marker on the called candle
        PNT p; int mx = -1;
        if (p.set(S->fxIdx, (float)S->fxView.price, kBarCenter) == RTX_OK && p.h > 0) mx = (int)p.h;
        std::function<int(double)> yPx = [&](double v) { return (int)getVerticalDrawPosition((float)v); };
        hlfx::marker(L, S->fxView, pb, mx, yPx, M);
    }
    execute(L);
    writeStatus(*S, &L);
    writeFxStatus(*S);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    HodLod *p = new HodLod();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("LRA HOD / LOD: one compact table - Regime, the RTH hours and the options expiry; Expected (1st / 2nd time and their odds) and Actual (the extreme with its IN odds); Estimate (range used, the projected opposite extreme). A projection line from the 1st extreme to the target with its label on the arrowhead. The 1st Extreme model runs in the background (its call box on the candle). Use Settings > Position to place it.");
    p->setVersion("2.0.4");
    return p;
}
