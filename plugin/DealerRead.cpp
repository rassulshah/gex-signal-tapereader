/********************************************************************************
 *  DealerRead.cpp  --  Investor/RT RTX extension  lsDealerRead  (v4.5.4, 2026-10-08)
 *
 *  THE DEALER READ: one compact strip docked in a corner of the price pane (mockup v11, ~1,470 x 176 px at font 10):
 *    left    the key level, the phase tabs APPROACH / SWEEP / RECLAIM / RETEST / TRADE, and WHAT TO DO
 *    top     the TRADE line (stop, target, R:R, record) and the flag chips (news, data age)
 *    middle  the LEVEL checklist, titled by its MenthorQ level ("CR0 4,200 - 0DTE CALL RESISTANCE"): SPEED, COLOR, ZOMMA
 *    right   the FUEL checklist: COVER, GAMMA, VANNA
 *    each row: a check box (ok / bad / watch / waiting), the greek, a centred meter (- weaker | + stronger) with its value,
 *    and the reason in plain words ("walking into gamma peak 4,210").
 *  Dashed border = still learning for this market (every checklist is scored nightly before it is trusted).
 *
 *  1.5.0 (2026-09-30, mockups v28 + v29): top box = MAGNET only (the dealer facts woven in, expiry time, Agrees / Fights /
 *  Pinned on its own line); bottom box order = Key Level, VERDICT (the Reader's Strength: will the level hold after the
 *  reclaim so the stop beyond the sweep is not hit), greeks, TAPE, then the trade checklist right above STOP / TGT.
 *
 *  1.6.1 (2026-10-01, Reader 2.49): settings text only - STP/RESWEEP, counted vs shown votes, DEADLINE. No new parameters.
 *  1.6.0 (2026-10-01, one decision system): when the file has the decision rows (lra/verdict.py) the bottom box is the
 *  same as the Reader panel - VERDICT (SUPPORTED / MIXED / OPPOSED + side + tally), the votes and the dealer lines with
 *  check / cross / dash / circle marks, the BOOK trust flag, then the PLAN band (TRIGGER, STOP, TARGET + R). Setting
 *  View: Standard / Summary - Summary shows the whole read as sentences instead. Older files: the 1.5 layout.
 *
 *  2.0.0 (2026-10-02): the DEALER REASONS box (Reasons box: Top centre / left / right / Off) - forced futures by cause
 *  (gamma / vanna / charm / positions, each as a share of the futures traded), gamma strength with speed / zomma / color,
 *  and the reasons lsDealerSig marks on the bars. File 1.6 rows FSUM / GSTR / SIG (lra/forced.py).
 *
 *  2.1.0 (2026-10-02): when price works a level the box is the Read in STAGES (mockup Dealer Read Stages) - 1 APPROACH,
 *  2 TURN STARTS, 3 TURN CONTINUES, every reason with the time it became known, value, chips and the MenthorQ numbers;
 *  settings Stages shown (All / Current only) and Reasons box width. File 1.7 rows STAGE / STG / SR / SSUM (lra/stages.py).
 *
 *  2.2.0 (2026-10-02): ONE compact grid replaces every box - a row per stage, a cell per reason (CODE value, best first,
 *  dashed = a guess); no level near = FORCED / GAMMA / REASONS rows. Drag the grip on its left edge to move it (kept per
 *  market in DealerRead.pos-<MKT>.txt), double-click the grip to put it back. The old boxes are no longer drawn.
 *  3.0.0 (2026-10-02 evening, file 1.8): THE TURN - one read, no stages. A header (level, low / high, MenthorQ levels held,
 *  gamma, fuel left), then the reasons as numbered sentences in time order, each with its Rev Reason tag (dashed = a guess,
 *  grey = a lean), word-wrapped to the box width. The same grip / drag / double-click. No turn yet = one line saying so;
 *  no level near = the FORCED / GAMMA / REASONS rows as in 2.2.
 *
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Dealer-<MKT>.csv (LRA analytics/lra/dealer_irt.py); grammar in
 *  DealerLogic.h. Parameters read only in the parms callbacks. Never black lines: the chart is black.
 ********************************************************************************/
#include "irtsdk.h"
// windows.h may expose the legacy far macro; DealerLogic's existing Node::far
// member must remain visible in this implementation unit.
#ifdef far
#undef far
#endif
#include "DealerLogic.h"
#include "HostSlot.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>

static const COLOR C_BLUE  = 0x00E3C341;   // WALL = YELLOW (1.2.7, Skylit colours, Rassul 2026-09-30); the name stays
static const COLOR C_MQ    = 0x0022D3EE;   // MenthorQ level text = cyan, as the MQ lines on the chart
static const COLOR C_AMBER = 0x00C084FC;   // FUEL = PURPLE (1.2.7, Skylit; a lighter purple so text reads on black); the name stays
static const COLOR C_GREEN = 0x0022C55E;
static const COLOR C_RED   = 0x00EF4444;
static const COLOR C_YEL   = 0x00FCD34D;
static const COLOR C_GREY  = 0x0064748B;
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_GROUND = 0x00000000;   // the strip's FILL (matches the chart); no line is ever this colour
static const COLOR C_BOXBG = 0x0005070C;
static const COLOR C_TRACK = 0x001F2937;
static const COLOR C_DOBG  = 0x000B1A2E;
static const COLOR C_DOBRD = 0x001E3A5F;
static const COLOR C_DOTXT = 0x00BFDBFE;
static const COLOR C_TABON = 0x001E3A5F;
static const COLOR C_TABONB = 0x0093C5FD;
// (4.1.0) the read line: prices that matter light grey bold, buyers green, sellers red, labels white, the rest a shade dimmer
static const COLOR C_RDHI = 0x00F1F5F9, C_RDBUY = 0x0086EFAC, C_RDSELL = 0x00FCA5A5, C_RDBOLD = 0x00FFFFFF, C_RDTXT = 0x00AEB8C5;
static const COLOR C_TABOFF = 0x000B0F19;
static const COLOR C_TABOFB = 0x00374151;
static const COLOR C_BORDER = 0x0064748B;

static COLOR colOf(char c) { return c == 'G' ? C_GREEN : c == 'R' ? C_RED : c == 'Y' ? C_YEL : c == 'A' ? C_AMBER : c == 'B' ? C_BLUE : c == 'N' ? C_MUTED : C_INK; }
static COLOR stCol(const std::string& st) { return st == "ok" ? C_GREEN : st == "bad" ? C_RED : st == "watch" ? C_YEL : C_GREY; }
static const char* stWord(const std::string& st, bool wall)
{
    if (st == "ok") return wall ? "STRONGER" : "HELPING";
    if (st == "bad") return wall ? "WEAKER" : "AGAINST";
    if (st == "watch") return "WATCH";
    return "WAITING";
}

// DealerLogic clamps ordinary meter input, but atof("nan") survives its range
// comparisons. Do not cast non-finite external-file values to drawing pixels.
static bool drMeterValue(const dl::Row& r, float& value)
{
    if (!r.hasV || !std::isfinite(r.v)) return false;
    value = r.v;
    if (value > 100.0f) value = 100.0f;
    if (value < -100.0f) value = -100.0f;
    return true;
}

static bool drValidDate(int year, int month, int day)
{
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (year < 1970 || month < 1 || month > 12 || day < 1) return false;
    int limit = days[month - 1];
    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) ++limit;
    return day <= limit;
}

// Days since 1970-01-01, using the civil-calendar conversion rather than mktime
// so stale status remains deterministic across DST transitions.
static long long drDaysSinceEpoch(int year, int month, int day)
{
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    const unsigned mp = (unsigned)(month + (month > 2 ? -3 : 9));
    const unsigned doy = (153 * mp + 2) / 5 + (unsigned)day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (long long)era * 146097LL + (long long)doe - 719468LL;
}

// The feed supplies both ASOF seconds and a calendar date. Use both when
// available: a file from two days ago must not look fresh merely because its
// time-of-day is close to the current clock. Older rows without a valid date
// retain the established time-of-day-only fallback.
static double dealerAgeMin(double asofSo, int year, int month, int day, int clockOffsetMin, const struct tm& now)
{
    if (!std::isfinite(asofSo) || asofSo < 0.0 || asofSo >= 86400.0) return 0.0;
    const double nowSo = now.tm_hour * 3600.0 + now.tm_min * 60.0 + now.tm_sec;
    const double sourceSo = asofSo + clockOffsetMin * 60.0;
    const int nowYear = now.tm_year + 1900, nowMonth = now.tm_mon + 1, nowDay = now.tm_mday;
    if (!drValidDate(year, month, day) || !drValidDate(nowYear, nowMonth, nowDay)) return dl::staleMin(sourceSo, nowSo);
    const double source = drDaysSinceEpoch(year, month, day) * 86400.0 + sourceSo;
    const double current = drDaysSinceEpoch(nowYear, nowMonth, nowDay) * 86400.0 + nowSo;
    return current > source ? (current - source) / 60.0 : 0.0;
}

static bool drShouldWriteStatus(const std::string& path, const std::string& payload, const std::string& lastPath,
                                const std::string& lastPayload, std::time_t lastWrite, std::time_t now)
{
    if (path != lastPath || lastWrite == 0 || now == (std::time_t)-1) return true;
    const double elapsed = std::difftime(now, lastWrite);
    if (elapsed < 0.0 || elapsed < 1.0) return false;
    return payload != lastPayload || elapsed >= 60.0;
}

struct PIdx { int rstg, rwid, rpos, rtop, view, market, corner, font, trade, todo, clock, layout, explain, keyLvl, keyFuel, keyTrig, lift, moveX, widthPct, show, apos, atop, keyHow; };
static PIdx PX;
struct Settings { int bg = 0; int rstg = 0, rwid = 100, rpos = 0, rtop = 30, view = 0, market = 0, corner = 0, font = 10, clock = 0, layout = 0, lift = 30, moveX = 0, widthPct = 100, show = 2, apos = 0, atop = 30; bool trade = true, todo = true, explain = true; };   // layout 0 Stacked, 1 Compact, 2 Mini, 3 Full

// Investor/RT may invoke the single DLL extension object for multiple chart
// hosts. This record contains every mutable draw/cache/layout value; it lives
// in the current host's SDK user-data slot, never in the shared object.
struct DealerReadState {
    Settings cfg;
    dl::Data D;
    std::string mkt, root;
    float u = 1.0f;
    long long loadedStamp = -2; std::string loadedPath;
    bool inV14 = false; int profReach = 330; long long reachStamp = -2;
    bool dragging = false; short gripL = 0, gripT = 0, gripR = 0, gripB = 0, gridL = 0, gridT = 0, gridH = 0, dragDX = 0, dragDY = 0;
    int posX = -1, posB = -1; std::string posMkt;
    int dltW = 50; bool dltLeft = false; long long dltStamp = -2; short lastClearR = 0, lastLeft = 0; int lastW = 0;
    std::string lastStatusPath, lastStatusText; std::time_t lastStatusWrite = 0;
};

class DealerRead : public cppExtension {
public:
    DealerRead();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);
    virtual int done(void);
    virtual int destroy(void);

    DealerReadState& state();
    HostSlot<DealerReadState> slot_;
    void releaseState();
    bool dialogReady();
    void readSettings(Settings& S);
    void load();
    void render(const Settings& S);
    void renderFull(const Settings& S);
    void renderSmall(const Settings& S);
    void renderV14(const Settings& S);                 // (1.4.0) mockup v27: Dealers + Magnet on top, the Key Level box below
    short topBox(const Settings& S, short x0, short y0, short W);
    std::vector<std::string> verdictLines(const Settings& S, short W);   // (1.5.0) the VERDICT text, wrapped
    void keyBox(const Settings& S, short x0, short y0, short W, short H, bool wallLvl, const std::vector<dl::Row>& rows, const dl::Row* tape, const std::string& stale);
    void tick(short x, short y, short sz, COLOR c);
    void mark(short x, short y, short sz, const std::string& q);   // (1.6.0) check / cross / dash / circle
    short keyBoxV2H(const Settings& S, short W);
    void keyBoxV2(const Settings& S, short x0, short y0, short W, short H, bool wallLvl, const std::string& stale);
    void renderAnalyst(const Settings& S, short x0, short y0, short W, short H);
    std::vector<std::string> wrapWords(const std::string& s, int firstW, int restW, int sz, bool bold, int maxLines);
    void miniBoxes(short& x, short y, const std::vector<dl::Row>& rows, short sz);
    std::string fit(const std::string& s, int maxW, int sz, bool bold);
    short U(float v);
    void fill(short l, short t, short r, short b, COLOR c);
    void frame(short l, short t, short r, short b, COLOR c, bool dashed);
    void line(short x1, short y1, short x2, short y2, COLOR c, int w);
    int  textW(const char* s, int sz, bool bold);
    void text(short x, short y, const char* s, COLOR col, int sz, bool bold, int just = 0);   // 0 left, 1 centre, 2 right; y = middle
    void checkBox(short x, short y, short sz, const std::string& st);
    void meter(short x, short y, short w, short h, const dl::Row& r);
    void checklist(short x, short y, short w, short h, const std::string& title, COLOR tcol, const std::string& score, char scol,
                   const std::vector<dl::Row>& rows, bool wall, int fs);
    void writeStatus(const char* what);
    void renderReasons(const Settings& S);            // (2.0.0) DEALER REASONS: forced futures by cause, gamma strength, reasons
    void renderStages(const Settings& S);             // (2.1.0) the Read in stages: approach / turn starts / turn continues
    void renderGrid(const Settings& S);               // (2.2.0) ONE compact grid: a row per stage, a cell per reason, draggable
    void renderTurn(const Settings& S);               // (3.0.0) THE TURN: header + numbered sentences, draggable
    virtual int mouse(RTX_EVENT* e);
    short clearRight(const RCT& pane);      // where he dropped it: px from the pane's left, px from its bottom (-1 = default)
    void loadPos(); void savePos();
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

DealerReadState& DealerRead::state()
{
    DealerReadState* p = slot_.get(this, true);
    if (!p) throw std::bad_alloc();
    return *p;
}

void DealerRead::releaseState()
{
    slot_.release(this);
}

int DealerRead::done(void)    { releaseState(); return RTX_OK; }
int DealerRead::destroy(void) { releaseState(); return RTX_OK; }

#define cfg (state().cfg)
#define D (state().D)
#define mkt (state().mkt)
#define root (state().root)
#define u (state().u)
#define loadedStamp (state().loadedStamp)
#define loadedPath (state().loadedPath)
#define inV14 (state().inV14)
#define profReach (state().profReach)
#define reachStamp (state().reachStamp)
#define dragging (state().dragging)
#define gripL (state().gripL)
#define gripT (state().gripT)
#define gripR (state().gripR)
#define gripB (state().gripB)
#define gridL (state().gridL)
#define gridT (state().gridT)
#define gridH (state().gridH)
#define dragDX (state().dragDX)
#define dragDY (state().dragDY)
#define posX (state().posX)
#define posB (state().posB)
#define posMkt (state().posMkt)
#define dltW (state().dltW)
#define dltLeft (state().dltLeft)
#define dltStamp (state().dltStamp)
#define lastClearR (state().lastClearR)
#define lastLeft (state().lastLeft)
#define lastW (state().lastW)
#define lastStatusPath (state().lastStatusPath)
#define lastStatusText (state().lastStatusText)
#define lastStatusWrite (state().lastStatusWrite)

DealerRead::DealerRead() : cppExtension() { }
short DealerRead::U(float v) { return (short)(v * u + 0.5f); }
bool DealerRead::dialogReady() { int i = getListIndex(PX.market); return i >= 0 && i <= 7; }
int DealerRead::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
// (4.3.0) the Position setting is saved for the dialog's market when he changes it (a draw never writes it)
static void drSavePlace(cppExtension* x, const Settings& S)
{
    char b[32] = {0}; const char* rs = x->getRootSymbol(b);
    dl::savePlace("DealerRead", dl::marketFor(S.market, rs ? rs : ""), S.corner);
    dl::savePlace("DealerReadBg", dl::marketFor(S.market, rs ? rs : ""), S.bg);   // (4.5.2) Background kept like the Position (IRT lists read -1 outside the dialog)
}
int DealerRead::parmsApply(void) { if (dialogReady()) { readSettings(cfg); drSavePlace(this, cfg); } return RTX_OK; }
int DealerRead::parmsUpdt(unsigned int) { if (dialogReady()) { readSettings(cfg); drSavePlace(this, cfg); } return RTX_OK; }

int cppExtension::setup(void)
{
    // (1.3.2, Rassul 2026-09-30: "why cant you have the how-to below fuel as text" / "many dropdowns that shouldnt even be
    // there") only real settings have a control; the whole guide is plain text (setLabelParameter) below them.
    // IRT keeps saved values BY POSITION (the parameter version does not reset them): remove + re-add the indicator once.
    // (4.2.0, Rassul 2026-10-07 08:14 "it should be at the bottom left of the chart ... set this as the default ... it should not
    // cross over into the delta profile ... selectable and draggable ... a setting that allows me to enter how much i want to move it
    // to the left or the right ... what is the analyst ... if we dont need this remove it") only the settings the Read uses now.
    // The Analyst box (3.x) was replaced by this Read on 2026-10-06 and no longer draws: its settings, the corner / layout / view
    // lists and the old lift / width boxes are gone. Version 10: IRT resets the saved values to these defaults once.
    // (4.3.0, Rassul 2026-10-07 09:16 "fix placement, with fixed height and width ... top left, top center, top right, bottom left
    // ... middle right instead of the user entering amount of placement") Position = one of 9 spots (default Bottom left); the box
    // has a fixed size; nothing is dragged (one shared IRT object moved the copper Read when he clicked the gold chart).
    setParameterVersion(12);   // (4.4.0) + Background
    setParameterDialogHeight(3);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    PX.market   = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    PX.corner   = pc++; setListParameter("Position", 6, dl::ANCHORS, 0, SL);
    PX.font     = pc++; setIntegerParameter("Font size (pt)", 10, NUMW);
    PX.clock    = pc++; setIntegerParameter("Clock offset (min)", 0, NUMW, SL);
    // (4.4.0, Rassul 2026-10-07 10:44 "a more transparent background so the candles are more visible in the background")
    PX.rwid     = pc++; setListParameter("Background", 0, "See-through;Solid");
    PX.moveX = PX.rtop = PX.trade = PX.todo = PX.layout = PX.show = PX.lift = PX.apos = PX.atop = PX.widthPct = PX.view = PX.rpos = PX.rstg = -1;
    PX.keyHow = PX.keyTrig = PX.keyLvl = PX.keyFuel = PX.explain = -1;
    return RTX_OK;
}

void DealerRead::readSettings(Settings& S)
{
    { int v = getListIndex(PX.market); if (v >= 0 && v <= 7) S.market = v; }
    { int v = getListIndex(PX.corner); if (v >= 0 && v <= 8) S.corner = v; }
    { int v = getIntegerValue(PX.font); if (v >= 7 && v <= 20) S.font = v; }
    { int v = getIntegerValue(PX.clock); if (v >= -720 && v <= 720) S.clock = v; }
    { int v = getListIndex(PX.rwid); if (v >= 0 && v <= 1) S.bg = v; }
    // (4.2.0 / 4.3.0) the retired settings keep fixed values
    S.trade = S.todo = true; S.layout = 0; S.show = 2; S.lift = 30; S.apos = 0; S.atop = 30; S.widthPct = 100; S.moveX = 0; S.rtop = 0; S.rwid = 100;
    S.view = 0; S.rpos = 0; S.rstg = 0; S.explain = false;
}

short DealerRead::clearRight(const RCT& pane)
{
    // the Delta Profile sits just left of the Dealer Profile (PLACE Right) or at the chart's left edge (PLACE Left); its letters
    // ("A? 7.8x -39") reach ~100 px further toward the candles
    const char* up = getenv("USERPROFILE");
    if (up) {
        std::string sp = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DeltaProfile.status.txt";
        long long st = dl::fileStamp(sp);
        if (st >= 0 && st != dltStamp) {
            dltStamp = st;
            std::ifstream g(sp.c_str()); std::string ln;
            while (std::getline(g, ln)) {
                if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
                if (ln.rfind("WIDTH,", 0) == 0) { int w = atoi(ln.c_str() + 6); if (w >= 20 && w <= 600) dltW = w; }
                if (ln.rfind("PLACE,", 0) == 0) dltLeft = ln.substr(6) == "Left";
            }
        }
    }
    short r = (short)(pane.right - profReach - U(8));
    if (!dltLeft) r = (short)(r - dltW - U(110));
    return (short)dl::stableMin("dr|" + std::to_string(pane.right) + "|" + std::to_string(pane.bottom), r);   // (4.5.1) no flip-flop between charts
}

void DealerRead::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) { D = dl::Data(); loadedPath.clear(); return; }
    const char* up = getenv("USERPROFILE"); if (!up) { D = dl::Data(); return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Dealer-" + mkt + ".csv";
    {   // (1.4.0) how far the Profile reaches in from the scale (its status file, REACH,<px>) - the boxes stay left of it
        std::string sp = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerProfile.status.txt";
        long long ss_ = dl::fileStamp(sp);
        if (ss_ >= 0 && ss_ != reachStamp) {
            reachStamp = ss_;
            std::ifstream g(sp.c_str()); std::string ln;
            while (std::getline(g, ln)) if (ln.rfind("REACH,", 0) == 0) { int r = atoi(ln.c_str() + 6); if (r >= 60 && r <= 1500) profReach = r; }
        }
    }
    long long st = dl::fileStamp(path);
    if (st >= 0 && st == loadedStamp && path == loadedPath) return;      // unchanged: keep what is parsed (no disk read)
    D = dl::Data();
    std::ifstream f(path.c_str()); if (!f.is_open()) { loadedPath.clear(); return; }
    std::stringstream ss; ss << f.rdbuf();
    D = dl::parseText(ss.str());
    loadedStamp = st; loadedPath = path;
}

void DealerRead::fill(short l, short t, short r, short b, COLOR c) { RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
void DealerRead::line(short x1, short y1, short x2, short y2, COLOR c, int w)
{
    setPen(c, (short)w, P_SOLID);
    PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
}
void DealerRead::frame(short l, short t, short r, short b, COLOR c, bool dashed)
{
    (void)dashed;                                   // (4.1.1, Rassul 2026-10-06 23:04 "I don't want dashed borders. They need to be solid lines")
    setPen(c, 1, P_SOLID);
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
// (4.1.2, Rassul 2026-10-06 23:06 "the time is in 24-hour format. I want it in central time format, not 24-hour") every clock time
// the Dealer Read draws (all CT already) is shown on the 12-hour clock: 22:52 -> 10:52 PM, 09:05 -> 9:05 AM. Measuring and drawing
// both go through it, so wrapping and badge widths match what is drawn.
static std::string to12(const char* in)
{
    std::string s(in ? in : ""), o; o.reserve(s.size() + 8);
    const size_t n = s.size();
    auto dig = [&](size_t k) { return k < n && isdigit((unsigned char)s[k]) != 0; };
    for (size_t i = 0; i < n; i++) {
        bool startOk = i == 0 || !(dig(i - 1) || s[i - 1] == ':' || s[i - 1] == '.' || s[i - 1] == ',');
        size_t hl = dig(i) ? (dig(i + 1) ? 2 : 1) : 0;                       // H or HH
        if (startOk && hl && i + hl < n && s[i + hl] == ':' && dig(i + hl + 1) && dig(i + hl + 2)
            && !dig(i + hl + 3) && !(i + hl + 3 < n && s[i + hl + 3] == ':')) {   // H:MM / HH:MM, not H:MM:SS or 123:45
            int h = atoi(s.substr(i, hl).c_str()), m = atoi(s.substr(i + hl + 1, 2).c_str());
            if (h <= 23 && m <= 59) {
                char b[32]; snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM");
                o += b; i += hl + 2; continue;
            }
        }
        o += s[i];
    }
    return o;
}

int DealerRead::textW(const char* s0, int sz, bool bold)
{
    std::string s_ = to12(s0); const char* s = s_.c_str();
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (int)getTextWidth(s, -1);
}
void DealerRead::text(short x, short y, const char* s0, COLOR col, int sz, bool bold, int just)
{
    std::string s_ = to12(s0); const char* s = s_.c_str();
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short yy = (short)(y - (short)(sz * 0.45f + 0.5f));        // the rect draw baselines low (GammaProfile v0.73)
    int w = (int)getTextWidth(s, -1);
    short l = just == 0 ? x : (just == 1 ? (short)(x - w / 2) : (short)(x - w));
    RCT rc; rc.set(l, (short)(yy - sz), (short)(l + w + 4), (short)(yy + sz));
    rc.drawText(s, false, false);
}
void DealerRead::checkBox(short x, short y, short sz, const std::string& st)
{
    COLOR c = stCol(st);
    frame(x, (short)(y - sz / 2), (short)(x + sz), (short)(y + sz / 2), c, false);
    short m = (short)(sz / 4 > 2 ? sz / 4 : 2);
    if (st == "ok") {        // a tick
        line((short)(x + m), y, (short)(x + sz / 2 - 1), (short)(y + sz / 2 - m), c, 2);
        line((short)(x + sz / 2 - 1), (short)(y + sz / 2 - m), (short)(x + sz - m), (short)(y - sz / 2 + m), c, 2);
    } else if (st == "bad") { // a cross
        line((short)(x + m), (short)(y - sz / 2 + m), (short)(x + sz - m), (short)(y + sz / 2 - m), c, 2);
        line((short)(x + m), (short)(y + sz / 2 - m), (short)(x + sz - m), (short)(y - sz / 2 + m), c, 2);
    } else if (st == "watch") {
        text((short)(x + sz / 2), y, "!", c, (int)(sz * 0.8f), true, 1);
    } else {
        fill((short)(x + sz / 2 - 1), (short)(y - 1), (short)(x + sz / 2 + 1), (short)(y + 1), c);
    }
}
void DealerRead::meter(short x, short y, short w, short h, const dl::Row& r)
{
    fill(x, (short)(y - h / 2), (short)(x + w), (short)(y + h / 2), C_TRACK);
    short mid = (short)(x + w / 2);
    int fs = (int)(9 * u + 0.5f);   // (1.2.3) 8 -> 9
    float value = 0.0f;
    if (!drMeterValue(r, value)) { text(mid, y, r.lab.c_str(), C_MUTED, fs, false, 1); return; }
    short ln = (short)(w / 2 * std::fabs(value) / 100.0f);
    COLOR c = value >= 0 ? C_GREEN : C_RED;
    if (value >= 0) fill(mid, (short)(y - h / 2), (short)(mid + ln), (short)(y + h / 2), c);
    else          fill((short)(mid - ln), (short)(y - h / 2), mid, (short)(y + h / 2), c);
    line(mid, (short)(y - h / 2 - 1), mid, (short)(y + h / 2 + 1), 0x0094A3B8, 1);
    int tw = textW(r.lab.c_str(), fs, true);
    if (value >= 0) {
        if (ln > tw + 6) text((short)(mid + ln - 3), y, r.lab.c_str(), 0x00052E16, fs, true, 2);
        else text((short)(mid + ln + 3), y, r.lab.c_str(), C_INK, fs, true, 0);
    } else text((short)(mid - ln - 3), y, r.lab.c_str(), C_INK, fs, true, 2);
}
void DealerRead::checklist(short x, short y, short w, short h, const std::string& title, COLOR tcol, const std::string& score, char scol,
                           const std::vector<dl::Row>& rows, bool wall, int fs)
{
    fill(x, y, (short)(x + w), (short)(y + h), C_BOXBG);
    frame(x, y, (short)(x + w), (short)(y + h), tcol, false);
    short ty = (short)(y + U(16));
    text((short)(x + U(10)), ty, title.c_str(), tcol, fs + 1, true, 0);
    text((short)(x + w - U(10)), ty, score.c_str(), colOf(scol), fs + 1, true, 2);
    short ry = (short)(y + U(48));
    for (size_t i = 0; i < rows.size() && i < 3; i++) {
        const dl::Row& r = rows[i];
        checkBox((short)(x + U(10)), ry, U(16), r.st);
        text((short)(x + U(34)), ry, r.n.c_str(), C_INK, fs - 1, true, 0);
        meter((short)(x + U(96)), ry, U(130), U(14), r);
        std::string why = std::string(stWord(r.st, wall)) + "  " + r.why;
        // the effect word in its colour, then the cause in ink
        const char* word = stWord(r.st, wall);
        text((short)(x + U(236)), ry, word, stCol(r.st), fs - 1, true, 0);
        int ww = textW(word, fs - 1, true);
        std::string cause = r.why;
        int room = (int)(x + w - U(8)) - (int)(x + U(236) + ww + U(6));
        while (!cause.empty() && textW(cause.c_str(), fs - 1, false) > room) cause = cause.substr(0, cause.size() - 1);
        text((short)(x + U(236) + ww + U(6)), ry, cause.c_str(), C_INK, fs - 1, false, 0);
        ry = (short)(ry + U(34));
    }
}

void DealerRead::renderFull(const Settings& S)
{
    u = S.font / 10.0f;
    int fs = S.font;
    RCT pane; pane.getPaneRect(false);
    RCT scale; scale.getScaleRect();
    short paneR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneR = (short)(scale.left - 2);
    bool top = S.trade;
    short W = (short)(U(1466) * S.widthPct / 100), H = top ? U(176) : U(150);
    if (!D.hasLevel) { W = U(900); H = U(44); }
    short x0 = (S.corner == 1 || S.corner == 3) ? (short)(paneR - W - U(6)) : (S.corner >= 4 ? (short)((pane.left + paneR) / 2 - W / 2) : (short)(pane.left + U(6)));
    short y0 = (S.corner <= 1 || S.corner == 4) ? (short)(pane.bottom - H - U(6) - S.lift) : (short)(pane.top + U(6));
    x0 = (short)(x0 + S.moveX);                          // (1.2.4) the sideways nudge
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_GROUND);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER, D.learn);

    char b[200];
    if (!D.hasPrice) {
        sprintf_s(b, sizeof(b), "Dealer Read: no data for %s (root %s) - is the LRA Reader running?", mkt.empty() ? "?" : mkt.c_str(), root.c_str());
        text((short)(x0 + U(10)), (short)(y0 + H / 2), b, C_MUTED, fs, false, 0);
        return;
    }
    if (!D.hasLevel) {
        sprintf_s(b, sizeof(b), "%s  Dealer Read - no key level within 0.6 EM", mkt.c_str());
        text((short)(x0 + U(10)), (short)(y0 + U(14)), b, C_INK, fs, true, 0);
        if (!D.bookLine.empty()) text((short)(x0 + U(10)), (short)(y0 + U(32)), D.bookLine.c_str(), D.book < 0 ? C_AMBER : C_BLUE, fs - 1, false, 0);
        return;
    }
    // ---- left column
    sprintf_s(b, sizeof(b), "%s %s  %s", mkt.c_str(), dl::fmtPx(D.lvlPx, mkt).c_str(), D.lvlLabel.c_str());
    short ly = (short)(y0 + U(18));
    text((short)(x0 + U(10)), ly, b, C_INK, fs + 1, true, 0);
    text((short)(x0 + U(10)), (short)(ly + U(22)), D.ctx.c_str(), C_MUTED, fs - 1, false, 0);
    short tabY = (short)(ly + U(44)), tabW = U(58), tabH = U(18);
    for (int i = 0; i < dl::NPHASE; i++) {
        short tx = (short)(x0 + U(10) + i * (tabW + U(3)));
        bool on = i == D.phase, done = i < D.phase;
        fill(tx, (short)(tabY - tabH / 2), (short)(tx + tabW), (short)(tabY + tabH / 2), on ? C_TABON : C_TABOFF);
        frame(tx, (short)(tabY - tabH / 2), (short)(tx + tabW), (short)(tabY + tabH / 2), on ? C_TABONB : C_TABOFB, false);
        text((short)(tx + tabW / 2), tabY, dl::phaseName(i), on ? C_INK : (done ? C_GREEN : C_MUTED), fs - 3, true, 1);
    }
    if (S.todo) {
        short dx = (short)(x0 + U(10)), dy = (short)(tabY + tabH / 2 + U(8)), dw = U(366), dh = (short)(y0 + H - U(8) - dy);
        fill(dx, dy, (short)(dx + dw), (short)(dy + dh), C_DOBG);
        frame(dx, dy, (short)(dx + dw), (short)(dy + dh), C_DOBRD, false);
        text((short)(dx + U(8)), (short)(dy + U(10)), "WHAT TO DO", C_MUTED, fs - 3, true, 0);
        int maxW = dw - U(16);
        struct Wd { DealerRead* s; int f; int operator()(const std::string& t) const { return s->textW(t.c_str(), f, true); } } wd = { this, fs - 1 };
        std::vector<std::string> ls = dl::wrap(D.doText, maxW, wd);
        short yy = (short)(dy + U(26));
        for (size_t i = 0; i < ls.size() && yy < dy + dh - U(4); i++) { text((short)(dx + U(8)), yy, ls[i].c_str(), C_DOTXT, fs - 1, true, 0); yy = (short)(yy + U(15)); }
    }
    // ---- the two checklists (and the TRADE line on top of them)
    short cx = (short)(x0 + U(390)), cw = U(530), gap = U(8);
    short cy = (short)(y0 + (top ? U(30) : U(6))), ch = (short)(y0 + H - U(6) - cy);
    if (top) {
        short bx = cx, bw = (short)(2 * cw + gap), by = (short)(y0 + U(4)), bh = U(22);
        fill(bx, by, (short)(bx + bw), (short)(by + bh), C_DOBG);
        frame(bx, by, (short)(bx + bw), (short)(by + bh), C_DOBRD, false);
        short my = (short)(by + bh / 2), tx = (short)(bx + U(10));
        text(tx, my, "TRADE", C_MUTED, fs - 2, true, 0); tx = (short)(tx + U(48));
        for (size_t i = 0; i < D.trade.size(); i++) {
            text(tx, my, D.trade[i].k.c_str(), C_MUTED, fs - 2, false, 0); tx = (short)(tx + textW(D.trade[i].k.c_str(), fs - 2, false) + U(6));
            text(tx, my, D.trade[i].v.c_str(), colOf(D.trade[i].col), fs - 1, true, 0); tx = (short)(tx + textW(D.trade[i].v.c_str(), fs - 1, true) + U(18));
        }
        short fx = (short)(bx + bw - U(6));
        for (int i = (int)D.flags.size() - 1; i >= 0; i--) {
            int w = textW(D.flags[(size_t)i].k.c_str(), fs - 2, true) + U(14);
            fx = (short)(fx - w);
            COLOR c = colOf(D.flags[(size_t)i].col);
            frame(fx, (short)(my - U(8)), (short)(fx + w), (short)(my + U(8)), c, false);
            text((short)(fx + w / 2), my, D.flags[(size_t)i].k.c_str(), c, fs - 2, true, 1);
            fx = (short)(fx - U(4));
        }
    }
    COLOR wcol = (D.wallCol == 'A' || D.wall.empty()) ? C_AMBER : C_BLUE;
    checklist(cx, cy, cw, ch, D.wallTitle, wcol, D.wallScore, D.wallCol, D.wall, true, fs);
    checklist((short)(cx + cw + gap), cy, cw, ch, D.fuelTitle.empty() ? std::string("FUEL") : D.fuelTitle, C_AMBER, D.fuelScore, D.fuelCol, D.fuel, false, fs);
    // stale
    RTDATE now = currentDate(); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(now, &t);
    double age = dealerAgeMin(D.asofSo, D.y, D.mo, D.d, S.clock, t);
    if (age > 10.0) {
        sprintf_s(b, sizeof(b), "STALE %dm", (int)(age + 0.5));
        text((short)(x0 + U(376)), (short)(y0 + U(18)), b, C_RED, fs, true, 2);
    }
}


// ---- (1.1) operator 2026-09-29: "too much space" -> D STACKED is the default: the level on its own row on top, one line of
// WHAT TO DO, then the two checklists side by side; the strip fits the pane width. Compact (2 lines) / Mini (1 line) too.
std::string DealerRead::fit(const std::string& s, int maxW, int sz, bool bold)
{
    std::string t = s;
    if (maxW <= 0) return "";
    while (!t.empty() && textW(t.c_str(), sz, bold) > maxW) t = t.substr(0, t.size() - 1);
    return t;
}
void DealerRead::miniBoxes(short& x, short y, const std::vector<dl::Row>& rows, short sz)
{
    for (size_t i = 0; i < rows.size() && i < 3; i++) { checkBox(x, y, sz, rows[i].st); x = (short)(x + sz + U(4)); }
}
// (1.2.6) word wrap by measured width: line 1 may be shorter (a label sits in front of it)
std::vector<std::string> DealerRead::wrapWords(const std::string& s, int firstW, int restW, int sz, bool bold, int maxLines)
{
    std::vector<std::string> out; std::string cur, word;
    std::vector<std::string> words; std::stringstream ss(s); while (ss >> word) words.push_back(word);
    for (size_t i = 0; i < words.size(); i++) {
        std::string t = cur.empty() ? words[i] : cur + " " + words[i];
        int lim = out.empty() ? firstW : restW;
        if (textW(t.c_str(), sz, bold) <= lim || cur.empty()) { cur = t; continue; }
        out.push_back(cur); cur = words[i];
        if ((int)out.size() == maxLines) { cur.clear(); break; }
    }
    if (!cur.empty() && (int)out.size() < maxLines) out.push_back(cur);
    return out;
}

// (1.2.6, Rassul 2026-09-30) THE ANALYST: NOW (3 lines of plain words: where price is, what dealers must do, the greeks / tape
// / options) + the bias, then 3 scenarios - what happens and why, and what to act on. Stop / target / R:R stay in the Read.
void DealerRead::renderAnalyst(const Settings& S, short x0, short y0, short W, short H)
{
    int fs = S.font;
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_GROUND);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER, false);
    short bt = (short)(y0 + U(3)), bh = U(86);   // (1.3.3, Rassul: "the NOW section ... should be 10pt") 5 lines at the full font   // (1.3.1) NOW = 5 lines (Rassul: "4-5 sentences", each with its why)
    fill((short)(x0 + U(3)), bt, (short)(x0 + W - U(3)), (short)(bt + bh), C_DOBG);
    frame((short)(x0 + U(3)), bt, (short)(x0 + W - U(3)), (short)(bt + bh), C_DOBRD, false);
    char b[120];
    sprintf_s(b, sizeof(b), "NOW %s  %s %s", D.aTime.c_str(), mkt.c_str(), dl::fmtPx(D.px, mkt).c_str());
    short l1 = (short)(bt + U(9));
    text((short)(x0 + U(9)), l1, b, C_TABONB, fs - 1, true, 0);
    int labW = textW(b, fs - 1, true) + U(12);
    int biasW = D.aBias.empty() ? 0 : textW(D.aBias.c_str(), fs - 1, true) + U(16);
    if (!D.aBias.empty()) text((short)(x0 + W - U(9)), l1, D.aBias.c_str(), colOf(D.aBiasCol), fs - 1, true, 2);
    std::string para = D.aNow.empty() ? std::string("The Analyst writes here after the next Reader update (every 5 minutes).") : D.aNow;
    std::vector<std::string> ln = wrapWords(para, W - U(18) - labW - biasW, W - U(18), fs, false, 5);
    for (size_t i = 0; i < ln.size(); i++)
        text((short)(x0 + U(9) + (i == 0 ? labW : 0)), (short)(l1 + (short)i * U(16)), ln[i].c_str(), C_INK, fs, false, 0);
    if (D.scn.empty()) return;                            // (1.3.0) no level: NOW only
    short st = (short)(bt + bh + U(3)), sb = (short)(y0 + H - U(3));
    short cw = (short)((W - U(12)) / 3);
    for (size_t i = 0; i < 3; i++) {
        short cx = (short)(x0 + U(3) + (short)i * (cw + U(3)));
        COLOR c = i < D.scn.size() ? colOf(D.scn[i].col) : C_GREY;
        if (i < D.scn.size() && D.scn[i].col == 'L') c = C_AMBER;          // lime (the fuel colour)
        if (i < D.scn.size() && D.scn[i].col == 'N') c = C_MUTED;
        fill(cx, st, (short)(cx + cw), sb, C_BOXBG);
        frame(cx, st, (short)(cx + cw), sb, c, false);
        if (i >= D.scn.size()) continue;
        const dl::Data::Scn& s = D.scn[i];
        short ty = (short)(st + U(10));
        int oddW = textW(s.odds.c_str(), fs - 3, false);
        text((short)(cx + cw - U(6)), ty, s.odds.c_str(), C_MUTED, fs - 3, false, 2);
        int actW = textW(s.act.c_str(), fs - 1, true);
        text((short)(cx + cw - U(14) - oddW), ty, s.act.c_str(), C_INK, fs - 1, true, 2);
        text((short)(cx + U(6)), ty, fit(s.title, cw - U(26) - oddW - actW, fs - 1, true).c_str(), c, fs - 1, true, 0);
        text((short)(cx + U(6)), (short)(ty + U(17)), fit(s.l1, cw - U(12), fs - 2, false).c_str(), C_INK, fs - 2, false, 0);
        text((short)(cx + U(6)), (short)(ty + U(32)), fit(s.l2, cw - U(12), fs - 2, false).c_str(), C_INK, fs - 2, false, 0);
        if (!s.l3.empty()) text((short)(cx + U(6)), (short)(ty + U(47)), fit(s.l3, cw - U(12), fs - 2, false).c_str(), C_TABONB, fs - 2, false, 0);   // the options there
    }
}

void DealerRead::render(const Settings& S) { if (S.layout == 3) renderFull(S); else renderSmall(S); }

void DealerRead::tick(short x, short y, short sz, COLOR c)
{   // a checkmark, sz px wide, centred on y
    line(x, y, (short)(x + sz / 3), (short)(y + sz / 3), c, 3);
    line((short)(x + sz / 3), (short)(y + sz / 3), (short)(x + sz), (short)(y - sz / 2), c, 3);
}

short DealerRead::topBox(const Settings& S, short x0, short y0, short W)
{   // Dealers: ... / (faint line) / Magnet: ... / Agrees | Fights | Pinned: ... on its own line. Returns the height.
    int fs = S.font;
    short lh = U(16), pad = U(9);
    int labD = textW("Dealers:", fs, true) + U(6), labM = textW("Magnet:", fs, true) + U(6);
    bool magOnly = !D.aMag.empty();                       // (1.5.0, mockup v29) Magnet only, the dealer facts woven into it
    std::vector<std::string> L1 = magOnly ? std::vector<std::string>() : wrapWords(D.aDeal, W - 2 * pad - labD, W - 2 * pad, fs, false, 3);
    std::string mag = D.aMag, rel, relW;
    size_t p = std::string::npos;
    const char* keys[3] = { "Agrees:", "Fights:", "Pinned:" };
    for (int i = 0; i < 3; i++) { size_t q = mag.find(keys[i]); if (q != std::string::npos && q < p) p = q; }
    if (p != std::string::npos) { rel = mag.substr(p); mag = mag.substr(0, p); while (!mag.empty() && mag.back() == ' ') mag.pop_back(); }
    std::vector<std::string> L2 = mag.empty() ? std::vector<std::string>() : wrapWords(mag, W - 2 * pad - labM, W - 2 * pad, fs, false, magOnly ? 6 : 3);
    std::vector<std::string> L3;
    int labR = 0;
    if (!rel.empty()) {
        size_t c = rel.find(':'); relW = rel.substr(0, c + 1); rel = rel.substr(c + 1); while (!rel.empty() && rel[0] == ' ') rel.erase(0, 1);
        labR = textW(relW.c_str(), fs, true) + U(6);
        L3 = wrapWords(rel, W - 2 * pad - labR, W - 2 * pad, fs, false, 2);
    }
    short H = (short)(U(8) + lh * (short)L1.size() + (L2.empty() ? 0 : (magOnly ? 0 : U(10)) + lh * (short)(L2.size() + L3.size())) + U(4));
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_DOBG);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_DOBRD, false);
    short y = (short)(y0 + U(4) + lh / 2);
    if (!magOnly) {
        text((short)(x0 + pad), y, "Dealers:", C_TABONB, fs, true, 0);
        for (size_t i = 0; i < L1.size(); i++) { text((short)(x0 + pad + (i == 0 ? labD : 0)), y, L1[i].c_str(), C_INK, fs, false, 0); y = (short)(y + lh); }
    }
    if (!L2.empty()) {
        if (!magOnly) {
            short ys = (short)(y - lh / 2 + U(4));
            line((short)(x0 + pad), ys, (short)(x0 + W - pad), ys, 0x00164E63, 1);
            y = (short)(y + U(10));
        }
        text((short)(x0 + pad), y, "Magnet:", C_MQ, fs, true, 0);
        for (size_t i = 0; i < L2.size(); i++) { text((short)(x0 + pad + (i == 0 ? labM : 0)), y, L2[i].c_str(), C_INK, fs, false, 0); y = (short)(y + lh); }
        if (!L3.empty()) {
            COLOR rc = relW[0] == 'A' ? C_GREEN : relW[0] == 'F' ? C_RED : 0x00F59E0B;
            text((short)(x0 + pad), y, relW.c_str(), rc, fs, true, 0);
            for (size_t i = 0; i < L3.size(); i++) { text((short)(x0 + pad + (i == 0 ? labR : 0)), y, L3[i].c_str(), C_INK, fs, false, 0); y = (short)(y + lh); }
        }
    }
    return H;
}

std::vector<std::string> DealerRead::verdictLines(const Settings& S, short W)
{
    if (D.vLabel.empty()) return std::vector<std::string>();
    char b[32]; if (D.vPts >= 0 && D.vOf > 0) sprintf_s(b, sizeof(b), "%s %d/%d", D.vLabel.c_str(), D.vPts, D.vOf); else sprintf_s(b, sizeof(b), "%s", D.vLabel.c_str());
    int lw = textW(b, S.font, true) + U(10);
    return wrapWords(D.vText, W - U(9) * 2 - U(14) - lw, W - U(9) * 2 - U(14) - lw, S.font, false, 2);
}

void DealerRead::keyBox(const Settings& S, short x0, short y0, short W, short H, bool wallLvl, const std::vector<dl::Row>& rows,
                        const dl::Row* tape, const std::string& stale)
{   // (1.5.0, mockup v28) Key Level / VERDICT / the greeks / TAPE / the trade checklist (a light band) / STOP + TGT
    int fs = S.font;
    COLOR bc = wallLvl ? C_BLUE : C_AMBER, bg = wallLvl ? 0x00161203 : 0x0012091F, band = wallLvl ? 0x00262006 : 0x00231433;
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), bg);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), bc, false);
    short pad = U(9), y = (short)(y0 + U(13));
    text((short)(x0 + pad), y, "Key Level:", C_MUTED, fs, true, 0);
    int kl = textW("Key Level:", fs, true) + U(6);
    text((short)(x0 + pad + kl), y, fit(D.keyLvl, W - 2 * pad - kl - (stale.empty() ? 0 : textW(stale.c_str(), fs - 1, true) + U(10)), fs, true).c_str(), bc, fs, true, 0);
    if (!stale.empty()) text((short)(x0 + W - pad), y, stale.c_str(), C_RED, fs - 1, true, 2);
    y = (short)(y0 + U(26));
    // the VERDICT: will the level hold after the reclaim, so the stop beyond the sweep is not hit?
    std::vector<std::string> vl = verdictLines(S, W);
    if (!D.vLabel.empty()) {
        COLOR vc = D.vLabel == "STRONG" ? C_GREEN : D.vLabel == "OK" ? 0x00F59E0B : D.vLabel == "WEAK" ? C_RED : C_MUTED;
        COLOR vbg = D.vLabel == "STRONG" ? 0x00052E16 : D.vLabel == "OK" ? 0x002A1D05 : D.vLabel == "WEAK" ? 0x002A0E12 : 0x00111827;
        short vt = y, vb = (short)(y + U(8) + U(15) * (short)(vl.empty() ? 1 : vl.size()));
        fill((short)(x0 + pad), vt, (short)(x0 + W - pad), vb, vbg);
        fill((short)(x0 + pad), vt, (short)(x0 + pad + U(3)), vb, vc);
        char b[32]; if (D.vPts >= 0 && D.vOf > 0) sprintf_s(b, sizeof(b), "%s %d/%d", D.vLabel.c_str(), D.vPts, D.vOf); else sprintf_s(b, sizeof(b), "%s", D.vLabel.c_str());
        short ty = (short)(vt + U(4) + U(15) / 2), tx = (short)(x0 + pad + U(10));
        text(tx, ty, b, vc, fs, true, 0);
        short lx = (short)(tx + textW(b, fs, true) + U(10));
        for (size_t i = 0; i < vl.size(); i++) text(lx, (short)(ty + (short)i * U(15)), vl[i].c_str(), C_INK, fs, false, 0);
        y = (short)(vb + U(4));
    }
    // greeks
    y = (short)(y + U(10));
    for (size_t i = 0; i < rows.size() && i < 3; i++) {
        const dl::Row& r = rows[i];
        checkBox((short)(x0 + pad), y, U(11), r.st);
        text((short)(x0 + pad + U(18)), y, r.n.c_str(), C_INK, fs, true, 0);
        meter((short)(x0 + pad + U(86)), y, U(96), U(11), r);
        text((short)(x0 + pad + U(190)), y, fit(r.why, W - pad - U(190) - pad, fs, false).c_str(), C_INK, fs, false, 0);
        y = (short)(y + U(20));
    }
    short ys = (short)(y - U(8));
    line((short)(x0 + 1), ys, (short)(x0 + W - 1), ys, 0x003B0764, 1);
    y = (short)(ys + U(12));
    if (tape) {
        text((short)(x0 + pad), y, "TAPE", C_INK, fs - 1, false, 0);
        text((short)(x0 + pad + textW("TAPE", fs - 1, false) + U(6)), y, fit(tape->why, W - 2 * pad - U(50), fs - 1, false).c_str(), C_MUTED, fs - 1, false, 0);
        y = (short)(y + U(12));
    }
    // the trade checklist band, right above STOP / TGT (the trade parameters together)
    short bt = (short)(y + U(2)), bb = (short)(bt + U(28));
    fill((short)(x0 + pad), bt, (short)(x0 + W - pad), bb, band);
    short cy = (short)((bt + bb) / 2), cx = (short)(x0 + pad + U(6));
    for (size_t i = 0; i < D.chk.size() && i < 3; i++) {
        const dl::Data::Chk& c = D.chk[i];
        bool ok = c.st == "ok";
        fill(cx, (short)(cy - U(7)), (short)(cx + U(14)), (short)(cy + U(7)), C_TRACK);
        text((short)(cx + U(7)), cy, c.n.c_str(), C_INK, fs - 3, true, 1);
        cx = (short)(cx + U(19));
        tick(cx, cy, U(12), ok ? C_GREEN : 0x004B5563);
        cx = (short)(cx + U(17));
        text(cx, cy, c.name.c_str(), C_INK, fs - 1, false, 0);
        cx = (short)(cx + textW(c.name.c_str(), fs - 1, false) + U(5));
        text(cx, cy, c.val.c_str(), ok ? C_INK : C_MUTED, fs - 2, false, 0);
        cx = (short)(cx + textW(c.val.c_str(), fs - 2, false) + U(12));
    }
    y = (short)(bb + U(14));
    text((short)(x0 + pad), y, D.stopTxt.c_str(), 0x00FCA5A5, fs, true, 0);
    text((short)(x0 + pad + textW(D.stopTxt.c_str(), fs, true) + U(20)), y, fit(D.tgtTxt, W - 2 * pad - textW(D.stopTxt.c_str(), fs, true) - U(20), fs, true).c_str(), 0x0086EFAC, fs, true, 0);
}

void DealerRead::mark(short x, short y, short sz, const std::string& q)
{   // (1.6.0) the Reader's marks: check = supports the trade, cross = against, dash = neutral, circle = waiting
    if (q == "ok") { line(x, y, (short)(x + sz / 3), (short)(y + sz / 3), C_GREEN, 3); line((short)(x + sz / 3), (short)(y + sz / 3), (short)(x + sz), (short)(y - sz / 2), C_GREEN, 3); }
    else if (q == "bad") { short h = (short)(sz / 2); line(x, (short)(y - h), (short)(x + sz), (short)(y + h), C_RED, 3); line(x, (short)(y + h), (short)(x + sz), (short)(y - h), C_RED, 3); }
    else if (q == "neutral") { line((short)(x + 1), y, (short)(x + sz - 1), y, C_MUTED, 3); }
    else { short h = (short)(sz / 2 - 1); frame((short)(x + 1), (short)(y - h), (short)(x + sz - 1), (short)(y + h), C_GREY, false); }
}

short DealerRead::keyBoxV2H(const Settings& S, short W)
{
    if (S.view == 1) {
        std::vector<std::string> L = wrapWords(D.summary, W - U(18), W - U(18), S.font, false, 9);
        return (short)(U(26) + U(26) + U(16) * (short)(L.empty() ? 1 : L.size()) + U(10));
    }
    size_t nl = D.votes.size(), nr = D.dlines.size();
    size_t n = nl > nr ? nl : nr;
    return (short)(U(26) + U(26) + U(19) * (short)n + U(8) + U(28) + U(8));
}

void DealerRead::keyBoxV2(const Settings& S, short x0, short y0, short W, short H, bool wallLvl, const std::string& stale)
{   // (1.6.0) Key Level / VERDICT / votes | dealer lines + BOOK / PLAN band - or the Summary view
    int fs = S.font;
    COLOR bc = wallLvl ? C_BLUE : C_AMBER, bg = wallLvl ? 0x00161203 : 0x0012091F, band = wallLvl ? 0x00262006 : 0x00231433;
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), bg);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), bc, false);
    short pad = U(9), y = (short)(y0 + U(13));
    text((short)(x0 + pad), y, "Key Level:", C_MUTED, fs, true, 0);
    int kl = textW("Key Level:", fs, true) + U(6);
    text((short)(x0 + pad + kl), y, fit(D.keyLvl, W - 2 * pad - kl - (stale.empty() ? 0 : textW(stale.c_str(), fs - 1, true) + U(10)), fs, true).c_str(), bc, fs, true, 0);
    if (!stale.empty()) text((short)(x0 + W - pad), y, stale.c_str(), C_RED, fs - 1, true, 2);
    // the VERDICT band
    y = (short)(y0 + U(26));
    bool v2No = D.v2Word == "OPPOSED" || D.v2Word == "NO TRADE";
    COLOR vc = D.v2Word == "SUPPORTED" ? C_GREEN : v2No ? C_RED : C_MUTED;
    COLOR vbg = D.v2Word == "SUPPORTED" ? 0x00052E16 : v2No ? 0x002A0E12 : 0x00111827;
    short vt = y, vb = (short)(y + U(22));
    fill((short)(x0 + pad), vt, (short)(x0 + W - pad), vb, vbg);
    fill((short)(x0 + pad), vt, (short)(x0 + pad + U(3)), vb, vc);
    short cy = (short)((vt + vb) / 2), cx = (short)(x0 + pad + U(10));
    mark(cx, cy, U(14), D.v2Word == "SUPPORTED" ? "ok" : v2No ? "bad" : D.v2Word == "WAIT" ? "wait" : "neutral");
    cx = (short)(cx + U(22));
    char b[96]; if (D.v2Word == "NO TRADE") sprintf_s(b, sizeof(b), "NO TRADE %s - re-swept", D.v2Side.c_str()); else sprintf_s(b, sizeof(b), "%s %s %d/%d", D.v2Word.c_str(), D.v2Side.c_str(), D.v2Sup, D.v2Meas);
    text(cx, cy, b, vc, fs + 1, true, 0);
    cx = (short)(cx + textW(b, fs + 1, true) + U(10));
    if (D.v2Wait > 0) { sprintf_s(b, sizeof(b), "%d waiting", D.v2Wait); text(cx, cy, b, C_MUTED, fs - 1, false, 0); }
    if (D.bookState == "FLIP") { std::string fb = "BOOK FLIPPED " + D.bookPct + "% - read may be backwards"; text((short)(x0 + W - pad - U(6)), cy, fit(fb, W / 2, fs - 1, true).c_str(), C_RED, fs - 1, true, 2); }
    y = (short)(vb + U(6));
    if (S.view == 1) {                                     // the Summary view: the whole read as sentences
        std::vector<std::string> L = wrapWords(D.summary, W - U(18), W - U(18), fs, false, 9);
        y = (short)(y + U(10));
        for (size_t i = 0; i < L.size(); i++) { text((short)(x0 + pad), y, L[i].c_str(), C_INK, fs, false, 0); y = (short)(y + U(16)); }
        return;
    }
    // votes (left) | the dealer lines + BOOK (right): mark, NAME, value
    short colW = (short)((W - 2 * pad) / 2), yl = (short)(y + U(10)), yr = yl;
    auto row = [&](short x, short yy, const std::string& q, const std::string& n, const std::string& v, short w) {
        mark(x, yy, U(12), q);
        text((short)(x + U(18)), yy, n.c_str(), C_INK, fs, true, 0);
        int nw = textW(n.c_str(), fs, true) + U(6);
        text((short)(x + U(18) + nw), yy, fit(v, w - U(18) - nw - U(6), fs - 1, false).c_str(), 0x00CBD5E1, fs - 1, false, 0);
    };
    for (size_t i = 0; i < D.votes.size(); i++) { row((short)(x0 + pad), yl, D.votes[i].q, D.votes[i].n, D.votes[i].v, colW); yl = (short)(yl + U(19)); }
    for (size_t i = 0; i < D.dlines.size(); i++) { row((short)(x0 + pad + colW), yr, D.dlines[i].q, D.dlines[i].n, D.dlines[i].v, colW); yr = (short)(yr + U(19)); }
    // (1.6.0, Rassul 2026-10-01: BOOK works in the background, not level information) only its FLIPPED warning shows, in the VERDICT band
    y = (short)((yl > yr ? yl : yr) - U(4));
    line((short)(x0 + pad + colW - U(6)), (short)(y0 + U(26) + U(22) + U(10)), (short)(x0 + pad + colW - U(6)), (short)(y - U(6)), 0x00334155, 1);
    // the PLAN band: TRIGGER / STOP / TARGET
    short bt = (short)(y + U(2)), bb = (short)(bt + U(28));
    fill((short)(x0 + pad), bt, (short)(x0 + W - pad), bb, band);
    short py = (short)((bt + bb) / 2), px = (short)(x0 + pad + U(6));
    short pw = (short)((W - 2 * pad - U(12)) / (D.plan.empty() ? 1 : (short)D.plan.size()));
    for (size_t i = 0; i < D.plan.size(); i++) { row(px, py, D.plan[i].q, D.plan[i].n, D.plan[i].v, pw); px = (short)(px + pw); }
}

void DealerRead::renderV14(const Settings& S)
{
    u = S.font / 10.0f;
    RCT pane; pane.getPaneRect(false);
    RCT scale; scale.getScaleRect();
    short paneR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneR = (short)(scale.left - 2);
    short clearR = (short)(paneR - profReach - U(8));        // stay left of the Profile
    char b[120];
    RTDATE now = currentDate(); struct tm tt; memset(&tt, 0, sizeof(tt)); getLocaltime(now, &tt);
    double age = dealerAgeMin(D.asofSo, D.y, D.mo, D.d, S.clock, tt);
    std::string stale;
    if (D.hasPrice && age > 10.0) { if (age >= 90) sprintf_s(b, sizeof(b), "STALE %dh", (int)(age / 60 + 0.5)); else sprintf_s(b, sizeof(b), "STALE %dm", (int)(age + 0.5)); stale = b; }
    // the Dealers + Magnet box (top)
    if (S.show >= 1 && (!D.aMag.empty() || !D.aDeal.empty())) {
        short W = (short)(U(860) * S.widthPct / 100);
        short x0 = (short)(pane.left + U(6) + S.moveX);
        if (S.apos == 1) x0 = (short)((pane.left + clearR) / 2 - W / 2);
        if (S.apos == 2) x0 = (short)(clearR - W);
        if (x0 + W > clearR) W = (short)(clearR - x0);
        if (W < U(300)) W = U(300);
        topBox(S, x0, (short)(pane.top + S.atop), W);
    }
    if (S.show == 1) return;
    if (D.keyLvl.empty()) {                                    // no level: the "waiting" box with NEXT (1.3.4)
        Settings S2 = S; S2.show = 0; inV14 = true; renderSmall(S2); inV14 = false; return;
    }
    bool wallLvl = D.noWall.empty() && !D.wall.empty();
    std::vector<dl::Row> rows;
    if (wallLvl) rows = D.wall;
    else {
        for (size_t i = 0; i < D.fuel.size(); i++) if (D.fuel[i].n != "GAMMA") rows.push_back(D.fuel[i]);
        for (size_t i = 0; i < D.xrows.size(); i++) if (D.xrows[i].n == "CHARM") rows.push_back(D.xrows[i]);
    }
    const dl::Row* tape = 0;
    for (size_t i = 0; i < D.xrows.size(); i++) if (D.xrows[i].n == "TAPE") tape = &D.xrows[i];
    short W = (short)(U(450) * S.widthPct / 100); if (W < U(340)) W = U(340);
    if (D.hasV2) {                                         // (1.6.0) the decision layer, the same as the Reader panel
        if (W < U(560)) W = U(560);
        short H2 = keyBoxV2H(S, W);
        bool right2 = S.corner == 1 || S.corner == 3, bottom2 = S.corner <= 1 || S.corner == 4;
        short x2 = right2 ? (short)(clearR - W) : (S.corner >= 4 ? (short)((pane.left + clearR) / 2 - W / 2) : (short)(pane.left + U(6)));
        x2 = (short)(x2 + S.moveX);
        if (x2 + W > clearR) x2 = (short)(clearR - W);
        if (x2 < pane.left + 2) x2 = (short)(pane.left + 2);
        short y2 = bottom2 ? (short)(pane.bottom - H2 - U(6) - S.lift) : (short)(pane.top + U(6));
        keyBoxV2(S, x2, y2, W, H2, wallLvl, stale);
        return;
    }
    size_t nv = D.vLabel.empty() ? 0 : verdictLines(S, W).size(); if (!D.vLabel.empty() && nv == 0) nv = 1;
    short vH = D.vLabel.empty() ? 0 : (short)(U(8) + U(15) * (short)nv + U(4));
    short H = (short)(U(26) + vH + U(10) + U(20) * (short)(rows.size() < 3 ? rows.size() : 3) + U(4) + (tape ? U(12) : 0) + U(2) + U(28) + U(14) + U(14));
    bool right = S.corner == 1 || S.corner == 3, bottom = S.corner <= 1 || S.corner == 4;
    short x0 = right ? (short)(clearR - W) : (S.corner >= 4 ? (short)((pane.left + clearR) / 2 - W / 2) : (short)(pane.left + U(6)));
    x0 = (short)(x0 + S.moveX);
    if (x0 + W > clearR) x0 = (short)(clearR - W);
    if (x0 < pane.left + 2) x0 = (short)(pane.left + 2);
    short y0 = bottom ? (short)(pane.bottom - H - U(6) - S.lift) : (short)(pane.top + U(6));
    keyBox(S, x0, y0, W, H, wallLvl, rows, tape, stale);
}

void DealerRead::renderSmall(const Settings& S)
{
    if (S.layout == 0 && !inV14 && (!D.aDeal.empty() || !D.aMag.empty() || !D.keyLvl.empty())) { renderV14(S); return; }   // (1.4.0) mockup v27
    u = S.font / 10.0f;
    int fs = S.font;
    RCT pane; pane.getPaneRect(false);
    RCT scale; scale.getScaleRect();
    short paneR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneR = (short)(scale.left - 2);
    short avail = (short)(paneR - pane.left - U(12));
    short W = (short)(U(860) * S.widthPct / 100); if (W > avail) W = avail; if (W < U(340)) W = U(340);   // (1.2.4) width %
    short H = S.layout == 0 ? U(132) : (S.layout == 1 ? U(44) : U(22));   // (1.2.6) stacked 176 -> 132 (compact, same font)
    if (!D.hasLevel || !D.hasPrice) H = D.next.empty() ? U(40) : (short)(U(40) + U(16) * (short)D.next.size());   // (1.3.4) + the NEXT lines
    short HA = D.scn.empty() ? U(92) : U(174);            // (1.3.0) no level = NOW only (no empty scenario boxes)
    bool sep = S.apos <= 2;                               // (1.3.0, Rassul: "the analyst text is supposed to be on top and the read at the bottom")
    short HT = S.show == 1 ? HA : (S.show == 2 && !sep ? (short)(H + HA + U(4)) : H);
    short x0 = (S.corner == 1 || S.corner == 3) ? (short)(paneR - W - U(6)) : (S.corner >= 4 ? (short)((pane.left + paneR) / 2 - W / 2) : (short)(pane.left + U(6)));
    short y0 = (S.corner <= 1 || S.corner == 4) ? (short)(pane.bottom - HT - U(6) - S.lift) : (short)(pane.top + U(6));
    x0 = (short)(x0 + S.moveX);                          // (1.2.4) the sideways nudge
    if (S.show >= 1 && sep) {                             // (1.3.0) the Analyst at the top of the chart, the Read where it is
        short ax = S.apos == 0 ? (short)(pane.left + U(6)) : (S.apos == 2 ? (short)(paneR - W - U(6)) : (short)((pane.left + paneR) / 2 - W / 2));
        renderAnalyst(S, (short)(ax + S.moveX), (short)(pane.top + S.atop), W, HA);
        if (S.show == 1) return;
    } else if (S.show >= 1) {                             // "Above the Read": stacked, as before
        renderAnalyst(S, x0, y0, W, HA);
        if (S.show == 1) return;
        y0 = (short)(y0 + HA + U(4));
    }
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_GROUND);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER, D.learn);
    char b[200];
    // stale (drawn at the right end of the first line)
    RTDATE now = currentDate(); struct tm tt; memset(&tt, 0, sizeof(tt)); getLocaltime(now, &tt);
    double age = dealerAgeMin(D.asofSo, D.y, D.mo, D.d, S.clock, tt);
    std::string stale;
    if (D.hasPrice && age > 10.0) { if (age >= 90) sprintf_s(b, sizeof(b), "STALE %dh", (int)(age / 60 + 0.5)); else sprintf_s(b, sizeof(b), "STALE %dm", (int)(age + 0.5)); stale = b; }
    if (!D.hasPrice) {
        sprintf_s(b, sizeof(b), "Dealer Read: no data for %s (root %s)", mkt.empty() ? "?" : mkt.c_str(), root.c_str());
        text((short)(x0 + U(8)), (short)(y0 + H / 2), b, C_MUTED, fs - 1, false, 0);
        return;
    }
    if (!D.hasLevel) {                                   // (1.2.2) two readable lines instead of one faint one
        sprintf_s(b, sizeof(b), "%s DEALER READ  -  waiting: no key level within 0.6 EM of %s", mkt.c_str(), dl::fmtPx(D.px, mkt).c_str());
        text((short)(x0 + U(8)), (short)(y0 + U(12)), fit(b, W - U(90), fs - 1, true).c_str(), C_INK, fs - 1, true, 0);
        short ny = (short)(y0 + U(29));
        for (size_t i = 0; i < D.next.size(); i++) {      // (1.3.4, Rassul: "there is nothing there") the nearest level above / below and why it is not a trade yet
            text((short)(x0 + U(8)), ny, fit(D.next[i], W - U(16), fs - 1, false).c_str(), C_INK, fs - 1, false, 0);
            ny = (short)(ny + U(16));
        }
        text((short)(x0 + U(8)), ny, fit(D.bookLine, W - U(16), fs - 2, false).c_str(), D.book < 0 ? C_AMBER : C_BLUE, fs - 2, false, 0);
        if (!stale.empty()) text((short)(x0 + W - U(8)), (short)(y0 + U(12)), stale.c_str(), C_RED, fs - 1, true, 2);
        return;
    }
    std::string trade;
    for (size_t i = 0; i < D.trade.size(); i++) {
        const std::string& k = D.trade[i].k;
        if (k == "RECORD" || k == "SWEEP ROOM") continue;
        std::string v = D.trade[i].v; size_t zp = v.find(" zone edge"); if (zp != std::string::npos) v = v.substr(0, zp);
        size_t pp = v.find(" ("); if (k == "TARGET" && pp != std::string::npos) v = v.substr(0, pp);
        trade += (trade.empty() ? "" : "  ") + std::string(k == "TARGET" ? "TGT" : k.c_str()) + " " + v;
    }
    sprintf_s(b, sizeof(b), "%s %s  %s", mkt.c_str(), dl::fmtPx(D.lvlPx, mkt).c_str(), D.lvlLabel.c_str());
    std::string lvl = b;
    std::string lvlMq;                                     // (1.2.4) "GC  ONL 4,198 + MQ PS 0D / HVL 0D 4,200" (the MQ part cyan)
    if (!D.keyName.empty()) { lvl = mkt + "  " + D.keyName; if (!D.mqName.empty()) lvlMq = "+ MQ " + D.mqName; }
    COLOR wcol = (D.wallCol == 'A' || D.wall.empty()) ? C_AMBER : C_BLUE;
    short box = U(12);

    if (S.layout == 2) {                                   // ---- MINI: one line
        short y = (short)(y0 + H / 2), x = (short)(x0 + U(8));
        sprintf_s(b, sizeof(b), "%s - %s", lvl.c_str(), dl::phaseName(D.phase));
        text(x, y, b, C_INK, fs - 1, true, 0); x = (short)(x + textW(b, fs - 1, true) + U(12));
        text(x, y, "LEVEL", wcol, fs - 1, true, 0); x = (short)(x + textW("LEVEL", fs - 1, true) + U(6));
        miniBoxes(x, y, D.wall, box); x = (short)(x + U(4));
        std::string ws = D.wallScore; text(x, y, ws.c_str(), colOf(D.wallCol), fs - 1, true, 0); x = (short)(x + textW(ws.c_str(), fs - 1, true) + U(14));
        text(x, y, "FUEL", C_AMBER, fs - 1, true, 0); x = (short)(x + textW("FUEL", fs - 1, true) + U(6));
        miniBoxes(x, y, D.fuel, box); x = (short)(x + U(4));
        text(x, y, D.fuelScore.c_str(), colOf(D.fuelCol), fs - 1, true, 0); x = (short)(x + textW(D.fuelScore.c_str(), fs - 1, true) + U(14));
        int room = (int)(x0 + W - U(10) - (stale.empty() ? 0 : textW(stale.c_str(), fs - 1, true) + U(10))) - x;
        text(x, y, fit(trade, room, fs - 2, false).c_str(), C_INK, fs - 2, false, 0);
        if (!stale.empty()) text((short)(x0 + W - U(8)), y, stale.c_str(), C_RED, fs - 1, true, 2);
        return;
    }
    if (S.layout == 1) {                                   // ---- COMPACT: two lines
        short y1 = (short)(y0 + U(12)), y2 = (short)(y0 + U(32)), x = (short)(x0 + U(8));
        text(x, y1, lvl.c_str(), C_INK, fs - 1, true, 0); x = (short)(x + textW(lvl.c_str(), fs - 1, true) + U(8));
        text(x, y1, dl::phaseName(D.phase), C_TABONB, fs - 2, true, 0); x = (short)(x + textW(dl::phaseName(D.phase), fs - 2, true) + U(6));
        for (size_t i = 0; i < D.trig.size(); i++) if (D.trig[i].on) { text(x, y1, D.trig[i].name.c_str(), C_GREEN, fs - 3, true, 0); x = (short)(x + textW(D.trig[i].name.c_str(), fs - 3, true) + U(6)); }
        x = (short)(x + U(6));
        std::string wt = fit(D.wallTitle, U(170), fs - 2, true);
        text(x, y1, wt.c_str(), wcol, fs - 2, true, 0); x = (short)(x + textW(wt.c_str(), fs - 2, true) + U(8));
        for (size_t i = 0; i < D.wall.size() && i < 3; i++) { checkBox(x, y1, box, D.wall[i].st); x = (short)(x + box + U(3)); text(x, y1, D.wall[i].n.c_str(), C_MUTED, fs - 3, false, 0); x = (short)(x + textW(D.wall[i].n.c_str(), fs - 3, false) + U(8)); }
        text(x, y1, "FUEL", C_AMBER, fs - 2, true, 0); x = (short)(x + textW("FUEL", fs - 2, true) + U(6));
        for (size_t i = 0; i < D.fuel.size() && i < 3; i++) { checkBox(x, y1, box, D.fuel[i].st); x = (short)(x + box + U(3)); text(x, y1, D.fuel[i].n.c_str(), C_INK, fs - 3, false, 0); x = (short)(x + textW(D.fuel[i].n.c_str(), fs - 3, false) + U(8)); }
        text((short)(x0 + W - U(8)), y1, D.fuelScore.c_str(), colOf(D.fuelCol), fs - 2, true, 2);
        x = (short)(x0 + U(8));
        text(x, y2, trade.c_str(), C_INK, fs - 2, false, 0); x = (short)(x + textW(trade.c_str(), fs - 2, false) + U(14));
        int room = (int)(x0 + W - U(10) - (stale.empty() ? 0 : textW(stale.c_str(), fs - 2, true) + U(10))) - x;
        text(x, y2, fit(">> " + D.doText, room, fs - 2, true).c_str(), C_DOTXT, fs - 2, true, 0);
        if (!stale.empty()) text((short)(x0 + W - U(8)), y2, stale.c_str(), C_RED, fs - 2, true, 2);
        return;
    }
    // (1.2.3, Rassul 2026-09-30: "enough space to make the font a little bigger") every text in the stacked layout +1 pt
    // ---- STACKED (D): row 1 the level, row 2 WHAT TO DO, then the two checklists side by side
    short r1t = (short)(y0 + U(4)), r1h = U(20), y1 = (short)(r1t + r1h / 2);   // (1.2.6) compact
    fill((short)(x0 + U(4)), r1t, (short)(x0 + W - U(4)), (short)(r1t + r1h), C_DOBG);
    frame((short)(x0 + U(4)), r1t, (short)(x0 + W - U(4)), (short)(r1t + r1h), C_DOBRD, false);
    short x = (short)(x0 + U(10));
    text(x, y1, lvl.c_str(), C_INK, fs, true, 0); x = (short)(x + textW(lvl.c_str(), fs, true) + U(8));
    if (!lvlMq.empty()) { text(x, y1, lvlMq.c_str(), C_MQ, fs, true, 0); x = (short)(x + textW(lvlMq.c_str(), fs, true) + U(14)); }
    else x = (short)(x + U(6));
    short tabW = U(54), tabH = U(14);
    for (int i = 0; i < dl::NPHASE; i++) {
        bool on = i == D.phase, done = i < D.phase;
        fill(x, (short)(y1 - tabH / 2), (short)(x + tabW), (short)(y1 + tabH / 2), on ? C_TABON : C_TABOFF);
        frame(x, (short)(y1 - tabH / 2), (short)(x + tabW), (short)(y1 + tabH / 2), on ? C_TABONB : C_TABOFB, false);
        text((short)(x + tabW / 2), y1, dl::phaseName(i), on ? C_INK : (done ? C_GREEN : C_MUTED), fs - 3, true, 1);
        x = (short)(x + tabW + U(2));
    }
    x = (short)(x + U(12));
    // flags (news) and stale at the right end, the trade numbers between
    short rx = (short)(x0 + W - U(10));
    if (!stale.empty()) { text(rx, y1, stale.c_str(), C_RED, fs - 1, true, 2); rx = (short)(rx - textW(stale.c_str(), fs - 1, true) - U(10)); }
    for (int i = (int)D.flags.size() - 1; i >= 0; i--) {
        const dl::KV& f = D.flags[(size_t)i];
        if (f.col == 'G') continue;                    // only what needs attention (a news release soon, old data)
        text(rx, y1, f.k.c_str(), colOf(f.col), fs - 1, true, 2); rx = (short)(rx - textW(f.k.c_str(), fs - 1, true) - U(10));
    }
    text(x, y1, fit(trade, rx - x, fs - 1, false).c_str(), C_INK, fs - 1, false, 0);
    // (1.2) the chip row: SWEEP - Trapped, TRIGGER - RevBar / RevVol / RevWave (any one lights the TRIGGER tab), each lit
    // chip with its value and its study number (Rassul 2026-09-29)
    short yc = (short)(r1t + r1h + U(10));
    {
        short cx = (short)(x0 + U(10));
        std::string grp;
        for (size_t i = 0; i < D.trig.size(); i++) {
            const dl::Trig& g = D.trig[i];
            if (g.group != grp) {
                if (!grp.empty()) cx = (short)(cx + U(8));
                grp = g.group;
                text(cx, yc, grp.c_str(), C_MUTED, fs - 2, true, 0); cx = (short)(cx + textW(grp.c_str(), fs - 2, true) + U(6));
            }
            std::string lab = g.name + (g.on && !g.val.empty() ? "  " + g.val : "") + (g.on && g.group == "TRIGGER" ? "  - " + g.note : "");
            COLOR c = !g.on ? C_GREY : (g.name == "RevBar" ? C_GREEN : g.name == "RevVol" ? 0x0038BDF8 : g.name == "RevWave" ? 0x00A78BFA : 0x0094A3B8);
            int tw = textW(lab.c_str(), fs - 2, g.on) + U(24);
            if (cx + tw > x0 + W - U(8)) break;
            frame(cx, (short)(yc - U(7)), (short)(cx + tw), (short)(yc + U(7)), c, false);
            checkBox((short)(cx + U(3)), yc, U(10), g.on ? "ok" : "wait");
            text((short)(cx + U(17)), yc, lab.c_str(), g.on ? c : C_MUTED, fs - 2, g.on, 0);
            cx = (short)(cx + tw + U(5));
        }
        // (1.2.6, Rassul 2026-09-30: "adding tape and charm at the top in the already available space is fine") TAPE + CHARM
        // in the rest of the chip row: check box, name, the reason - no extra height
        short room = (short)(x0 + W - U(10) - (cx + U(14)));
        if (!D.xrows.empty() && room > U(120)) {
            short xx = (short)(cx + U(14)), each = (short)(room / (short)D.xrows.size());
            for (size_t i = 0; i < D.xrows.size() && i < 2; i++) {
                const dl::Row& q = D.xrows[i];
                checkBox(xx, yc, U(10), q.st);
                text((short)(xx + U(14)), yc, q.n.c_str(), stCol(q.st) == C_GREY ? C_MUTED : stCol(q.st), fs - 2, true, 0);
                short nx = (short)(xx + U(14) + textW(q.n.c_str(), fs - 2, true) + U(6));
                std::string w = q.why; size_t cp = w.find(": "); if (cp != std::string::npos && cp < 20) w = w.substr(cp + 2);
                text(nx, yc, fit(w, xx + each - nx - U(10), fs - 2, false).c_str(), C_INK, fs - 2, false, 0);
                xx = (short)(xx + each);
            }
        }
    }
    short y2 = (short)(yc + U(15));
    if (S.todo) text((short)(x0 + U(10)), y2, fit(">> " + D.doText, W - U(20), fs - 1, true).c_str(), C_DOTXT, fs - 1, true, 0);
    short ct = (short)(y2 + U(9)), cb = (short)(y0 + H - U(4)), cw = (short)((W - U(12)) / 2);
    for (int k = 0; k < 2; k++) {
        short cx = (short)(x0 + U(4) + k * (cw + U(4)));
        const std::vector<dl::Row>& rows = k == 0 ? D.wall : D.fuel;
        COLOR tc = k == 0 ? (rows.empty() && !D.noWall.empty() ? C_GREY : wcol) : C_AMBER;
        std::string sc = k == 0 ? D.wallScore : D.fuelScore; char scol = k == 0 ? D.wallCol : D.fuelCol;
        fill(cx, ct, (short)(cx + cw), cb, C_BOXBG);
        frame(cx, ct, (short)(cx + cw), cb, tc, false);
        short ty = (short)(ct + U(10));
        bool noWallBox = k == 0 && rows.empty() && !D.noWall.empty();
        int scW = textW(sc.c_str(), fs, true);
        std::string title = fit(k == 0 ? D.wallTitle : (D.fuelTitle.empty() ? std::string("FUEL") : D.fuelTitle), cw - scW - U(24), fs, true);
        if (!noWallBox) text((short)(cx + U(6)), ty, title.c_str(), tc, fs, true, 0);
        if (false && S.explain) {   // (1.1.2) no text on the chart beyond the checklist itself - the description explains   // (1.1) what the box answers, in the room left on the title line
            const char* q = k == 0 ? "  is the wall getting stronger? (gamma)" : "  are dealers reversing their trade? (delta flow)";
            int used = textW(title.c_str(), fs - 1, true) + U(6);
            std::string qq = fit(q, cw - scW - U(24) - used, fs - 3, false);
            text((short)(cx + U(6) + used), ty, qq.c_str(), C_MUTED, fs - 3, false, 0);
        }
        text((short)(cx + cw - U(6)), ty, sc.c_str(), colOf(scol), fs, true, 2);
        if (noWallBox) {                                  // (1.2.6) compact: NO WALL + why, top-left (no big empty box)
            text((short)(cx + U(8)), ty, "NO WALL", C_MUTED, fs, true, 0);
            std::vector<std::string> ln = wrapWords(D.noWall, cw - U(16), cw - U(16), fs - 1, false, 3);
            for (size_t i = 0; i < ln.size(); i++) text((short)(cx + U(8)), (short)(ty + U(17) + (short)i * U(15)), ln[i].c_str(), C_INK, fs - 1, false, 0);
            continue;
        }
        short ry = (short)(ct + U(27));
        for (size_t i = 0; i < rows.size() && i < 3; i++) {
            const dl::Row& r = rows[i];
            checkBox((short)(cx + U(6)), ry, box, r.st);
            text((short)(cx + U(26)), ry, r.n.c_str(), C_INK, fs - 1, true, 0);
            meter((short)(cx + U(84)), ry, U(84), U(10), r);
            short wx = (short)(cx + U(226));      // (1.1.3) past the meter AND its value label ("7% vol" ran into the reason)
            text(wx, ry, fit(r.why, cw - U(232), fs - 1, false).c_str(), C_INK, fs - 1, false, 0);
            ry = (short)(ry + U(17));
        }
    }
}

void DealerRead::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerRead.status.txt";
    std::ostringstream text;
    text << "VERSION,4.5.4\nROOT," << root << "\nMARKET," << mkt << "\nLEVEL," << (D.hasLevel ? D.lvlLabel : "none") << "\nPHASE," << D.phase << "\nSTATE," << what << "\n";
    const std::string payload = text.str();
    const std::time_t now = std::time(NULL);
    // Different charts can share this legacy status file: cap those switches
    // to one write per second, and retain a 60 s heartbeat for unchanged data.
    if (!drShouldWriteStatus(path, payload, lastStatusPath, lastStatusText, lastStatusWrite, now)) return;
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << payload;
    if (!f.good()) return;
    lastStatusPath = path; lastStatusText = payload; lastStatusWrite = now;
}

// (2.0.0, Rassul 2026-10-02: "i need a way of seeing how vanna and charm are forcing dealers to buy back futures, clear
// position change is also forcing dealers to buy back futures, and also see how speed zomma and color are impacting gamma")
// The DEALER REASONS box (mockup Dealer Read Forced Futures): FORCED FUTURES by cause since the current run started, with
// each part's share of the futures traded; GAMMA STRENGTH and how speed / zomma / color change it; the Dealer Sig reasons.
static COLOR chipCol(char c) { return c == 'G' ? C_GREEN : c == 'Y' ? C_YEL : c == 'R' ? C_RED : c == 'B' ? 0x0060A5FA : c == 'P' ? 0x00C08CFF : c == 'A' ? 0x00F59E0B : C_MUTED; }
static COLOR stageCol(int n) { return n == 1 ? 0x0060A5FA : n == 2 ? 0x00F59E0B : 0x004ADE80; }

// (2.1.0, Rassul 2026-10-02: "the dealer read examine options data as price approaches the gamma level, starts and continues the
// reversal turn giving reversal reasons" + "don't cut short the reversal reasons. You can take educated guesses") - mockup
// Dealer Read Stages. Every row: time it became known, code, value, chips, the reason with its MenthorQ numbers.
void DealerRead::renderStages(const Settings& S)
{
    u = S.font / 10.0f;
    int fs = S.font, rf = S.font - 1;
    RCT pane; pane.getPaneRect(false);
    short W = (short)(S.rwid * u), lh = U(15), pad = U(10);
    short right = (short)(pane.right - profReach - U(12));
    if (W > right - pane.left - U(8)) W = (short)(right - pane.left - U(8));
    std::vector<int> show;
    int now = D.stageNow < 1 ? 1 : D.stageNow;
    for (size_t i = 0; i < D.stgs.size(); i++) if (S.rstg == 0 || D.stgs[i].n == now) show.push_back((int)i);
    if (show.empty()) return;
    // column widths: the widest chip set across the rows shown (capped)
    int chipW = 0;
    for (size_t r = 0; r < D.srows.size(); r++) {
        bool on = false; for (size_t k = 0; k < show.size(); k++) if (D.stgs[show[k]].n == D.srows[r].n) on = true;
        if (!on) continue;
        int w = 0; for (size_t c = 0; c < D.srows[r].chips.size(); c++) w += textW(D.srows[r].chips[c].name.c_str(), rf - 1, false) + U(12);
        if (w > chipW) chipW = w;
    }
    if (chipW > U(180)) chipW = U(180);
    int H = pad + lh + U(4);
    for (size_t k = 0; k < show.size(); k++) {
        H += lh + U(6);
        for (size_t r = 0; r < D.srows.size(); r++) if (D.srows[r].n == D.stgs[show[k]].n) H += lh;
        if (!D.stgs[show[k]].sum.empty()) H += lh + U(4);
    }
    H += pad;
    short x0 = S.rpos == 1 ? (short)(pane.left + U(10)) : S.rpos == 2 ? (short)(right - W) : (short)((pane.left + right) / 2 - W / 2);
    if (x0 < pane.left + U(4)) x0 = (short)(pane.left + U(4));
    short y0 = (short)(pane.top + S.rtop);
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BOXBG);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER, true);      // dashed = still learning
    short y = (short)(y0 + pad + lh / 2);
    std::string ttl = "DEALER READ  " + mkt + "  " + (D.stageSide == 'L' ? "LONG at " : "SHORT at ") + D.stageLvl;
    text((short)(x0 + pad), y, ttl.c_str(), C_TABONB, fs, true, 0);
    const char* nowName = now == 1 ? "1 APPROACH" : now == 2 ? "2 TURN STARTS" : "3 TURN CONTINUES";
    std::string rt = std::string("now: ") + (D.stageNow == 0 ? "watching" : nowName) + (D.stageExt.empty() ? "" : "  extreme " + D.stageExt + " (" + D.stageExtT + " bar)");
    text((short)(x0 + W - pad), y, rt.c_str(), stageCol(now), rf, true, 2);
    y = (short)(y + lh + U(4));
    short xT = (short)(x0 + pad), xC = (short)(xT + U(40)), xV = (short)(xT + U(160)), xCh = (short)(xV + U(8)), xTx = (short)(xCh + chipW + U(4));
    for (size_t k = 0; k < show.size(); k++) {
        const dl::Data::Stg& G = D.stgs[show[k]];
        COLOR sc = stageCol(G.n);
        y = (short)(y + U(6));
        fill(x0 + U(4), (short)(y - lh / 2), (short)(x0 + U(7)), (short)(y + lh / 2), sc);
        text(xT, y, G.title.c_str(), sc, fs, true, 0);
        int tw = textW(G.title.c_str(), fs, true);
        std::string sub = G.sub + (G.n == now && D.stageNow > 0 ? "   << NOW" : "");
        text((short)(xT + tw + U(10)), y, fit(sub, x0 + W - pad - (xT + tw + U(10)), rf, false).c_str(), C_MUTED, rf, false, 0);
        y = (short)(y + lh);
        for (size_t r = 0; r < D.srows.size(); r++) {
            const dl::Data::SRow& R = D.srows[r];
            if (R.n != G.n) continue;
            COLOR cc = R.side == 'B' ? C_GREEN : R.side == 'S' ? C_RED : C_INK;
            text(xT, y, R.t.c_str(), C_MUTED, rf, false, 0);
            text(xC, y, R.code.c_str(), cc, rf, true, 0);
            if (dl::isGuess(R.code)) { int cw = textW(R.code.c_str(), rf, true); frame((short)(xC - U(2)), (short)(y - lh / 2 + U(1)), (short)(xC + cw + U(3)), (short)(y + lh / 2 - U(1)), cc, true); }
            text(xV, y, R.val.c_str(), cc, rf, true, 2);
            short cx = xCh;
            for (size_t c = 0; c < R.chips.size(); c++) {
                COLOR chc = chipCol(R.chips[c].col);
                int w = textW(R.chips[c].name.c_str(), rf - 1, false) + U(8);
                if (cx + w > xCh + chipW) break;
                frame(cx, (short)(y - U(6)), (short)(cx + w), (short)(y + U(6)), chc, false);
                text((short)(cx + U(4)), y, R.chips[c].name.c_str(), chc, rf - 1, false, 0);
                cx = (short)(cx + w + U(4));
            }
            text(xTx, y, fit(R.text, x0 + W - pad - xTx, rf, false).c_str(), C_INK, rf, false, 0);
            y = (short)(y + lh);
        }
        if (!G.sum.empty()) {
            fill(xT, (short)(y - lh / 2), (short)(x0 + W - pad), (short)(y + lh / 2 + U(2)), C_DOBG);
            text((short)(xT + U(6)), (short)(y + U(1)), fit(G.sum, W - 2 * pad - U(12), rf, true).c_str(), C_DOTXT, rf, true, 0);
            y = (short)(y + lh + U(4));
        }
    }
}

void DealerRead::renderReasons(const Settings& S)
{
    if (S.rpos == 3 || mkt.empty()) return;
    if (D.hasStage && !D.stgs.empty()) { renderStages(S); return; }    // (2.1.0) a level is being worked: the Read in stages
    if (!D.hasFsum && !D.hasGstr && D.sigs.empty() && D.ferr.empty()) return;
    u = S.font / 10.0f;
    int fs = S.font;
    RCT pane; pane.getPaneRect(false);
    const COLOR CG = 0x004EA3FF, CV = 0x00C08CFF, CC = 0x00FFB347, CP = 0x003FD0A8;
    short W = U(560), lh = U(17), pad = U(10);
    int nSig = (int)D.sigs.size() < 4 ? (int)D.sigs.size() : 4;
    short H = (short)(pad + lh                                   // title
              + (D.hasFsum ? lh * 5 + U(6) : 0)                   // FORCED + 4 causes
              + (D.hasGstr ? lh * 4 + U(6) : 0)                   // GAMMA STRENGTH + 3
              + (nSig ? lh * (nSig + 1) + U(6) : 0)
              + (D.ferr.empty() ? 0 : lh) + pad);
    short x0;
    short right = (short)(pane.right - profReach - U(12));
    if (S.rpos == 1) x0 = (short)(pane.left + U(10));
    else if (S.rpos == 2) x0 = (short)(right - W);
    else x0 = (short)((pane.left + right) / 2 - W / 2);
    if (x0 < pane.left + U(4)) x0 = (short)(pane.left + U(4));
    short y0 = (short)(pane.top + S.rtop);
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BOXBG);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER, D.learn);
    short y = (short)(y0 + pad + lh / 2);
    text((short)(x0 + pad), y, "DEALER REASONS", C_TABONB, fs + 1, true, 0);
    std::string hdr = mkt + (D.hasGstr ? "  grid " + D.gstr.t : "");
    text((short)(x0 + W - pad), y, hdr.c_str(), C_MUTED, fs - 1, false, 2);
    y = (short)(y + lh);
    short xName = (short)(x0 + pad), xBar = (short)(x0 + U(120)), bw = U(150), xPct = (short)(xBar + bw + U(8)), xWhy = (short)(xPct + U(52));
    if (D.hasFsum) {
        const dl::Data::FWin& F = D.fsum;
        float tot = F.gam + F.van + F.cha + F.pos;
        y = (short)(y + U(6));
        std::string t1 = "FORCED FUTURES " + F.ta + " - " + F.tb;
        text(xName, y, t1.c_str(), C_INK, fs, true, 0);
        char tb[96]; snprintf(tb, sizeof(tb), "dealers %s %s = %s of %ld traded", tot >= 0 ? "BUY" : "SELL", dl::signedN(tot).c_str(), dl::shareTxt(tot, F.fut).c_str(), std::lround(F.fut));
        text((short)(x0 + W - pad), y, tb, tot >= 0 ? C_GREEN : C_RED, fs, true, 2);
        y = (short)(y + lh);
        float vals[4] = { F.gam, F.van, F.cha, F.pos };
        float mx = 1; for (int i = 0; i < 4; i++) if (std::fabs(vals[i]) > mx) mx = std::fabs(vals[i]);
        const char* names[4] = { "Gamma  price", "Vanna  IV", "Charm  time", "Positions" };
        COLOR cols[4] = { CG, CV, CC, CP };
        char why[4][96];
        snprintf(why[0], 96, "price %s -> %s", F.pa.c_str(), F.pb.c_str());
        snprintf(why[1], 96, "front IV %.2f -> %.2f%%", F.iva, F.ivb);
        snprintf(why[2], 96, "time %s -> %s", F.ta.c_str(), F.tb.c_str());
        why[3][0] = 0;
        for (int i = 0; i < 4; i++) {
            text(xName, y, names[i], cols[i], fs - 1, true, 0);
            fill(xBar, (short)(y - U(5)), (short)(xBar + bw), (short)(y + U(5)), C_TRACK);
            short mid = (short)(xBar + bw / 2), ln = (short)(bw / 2 * std::fabs(vals[i]) / mx);
            if (vals[i] >= 0) fill(mid, (short)(y - U(5)), (short)(mid + ln), (short)(y + U(5)), C_GREEN);
            else fill((short)(mid - ln), (short)(y - U(5)), mid, (short)(y + U(5)), C_RED);
            std::string v = dl::signedN(vals[i]);
            if (vals[i] >= 0) text((short)(mid - U(3)), y, v.c_str(), C_INK, fs - 1, true, 2);
            else text((short)(mid + U(3)), y, v.c_str(), C_INK, fs - 1, true, 0);
            text(xPct, y, dl::shareTxt(vals[i], F.fut).c_str(), C_MUTED, fs - 1, false, 0);
            text(xWhy, y, fit(why[i], x0 + W - pad - xWhy, fs - 1, false).c_str(), C_MUTED, fs - 1, false, 0);
            y = (short)(y + lh);
        }
    }
    if (D.hasGstr) {
        const dl::Data::GStr& G = D.gstr;
        y = (short)(y + U(6));
        char t2[96]; snprintf(t2, sizeof(t2), "GAMMA STRENGTH %.1f / pt at %s", G.g, G.px.c_str());
        text(xName, y, t2, C_INK, fs, true, 0);
        y = (short)(y + lh);
        char r[3][128];
        snprintf(r[0], 128, "price 0.25 EM lower %.1f (%s)   higher %.1f (%s)", G.sdn, dl::gChange(G.g, G.sdn).c_str(), G.sup, dl::gChange(G.g, G.sup).c_str());
        snprintf(r[1], 128, "IV +1 pt %.1f (%s)   IV -1 pt %.1f (%s)", G.zp, dl::gChange(G.g, G.zp).c_str(), G.zm, dl::gChange(G.g, G.zm).c_str());
        snprintf(r[2], 128, "in 30 min %.1f (%s)   in 60 min %.1f (%s)", G.c30, dl::gChange(G.g, G.c30).c_str(), G.c60, dl::gChange(G.g, G.c60).c_str());
        const char* nm[3] = { "Speed", "Zomma", "Color" };
        for (int i = 0; i < 3; i++) {
            text(xName, y, nm[i], CG, fs - 1, true, 0);
            text(xBar, y, fit(r[i], x0 + W - pad - xBar, fs - 1, false).c_str(), C_INK, fs - 1, false, 0);
            y = (short)(y + lh);
        }
    }
    if (nSig) {
        y = (short)(y + U(6));
        text(xName, y, "REASONS (Dealer Sig)", C_INK, fs, true, 0);
        y = (short)(y + lh);
        for (int i = 0; i < nSig; i++) {
            const dl::Data::Sig& g = D.sigs[D.sigs.size() - 1 - i];
            char tm[16]; snprintf(tm, sizeof(tm), "%02d:%02d", g.h, g.mi);
            text(xName, y, tm, C_MUTED, fs - 1, false, 0);
            text((short)(xName + U(48)), y, g.code.c_str(), g.side == 'B' ? C_GREEN : C_RED, fs - 1, true, 0);
            text(xBar, y, fit(g.text, x0 + W - pad - xBar, fs - 1, false).c_str(), C_INK, fs - 1, false, 0);
            y = (short)(y + lh);
        }
    }
    if (!D.ferr.empty()) text(xName, y, fit("forced futures not available: " + D.ferr, W - 2 * pad, fs - 1, false).c_str(), C_MUTED, fs - 1, false, 0);
}


// ------------------------------------------------------------------ (2.2.0) the grid
// Rassul 2026-10-02: "the dealer read is taking up too much space, it should be at the bottom and more compact" / "i should be
// able to select it and move it around" / "it is also displaying the old read in the background which you can get rid of" /
// "this is still too wide, use 1 grid approach". One grid: a row per stage (1 APPROACH, 2 TURN, 3 CONTINUES), a cell per reason
// (CODE value, best first), drag the grip on its left edge to move it - the spot is kept per market in DealerRead.pos-<MKT>.txt.
void DealerRead::loadPos()
{
    if (posMkt == mkt) return;
    posMkt = mkt; posX = posB = -1;
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::ifstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerRead.pos-" + mkt + ".txt").c_str());
    if (f.is_open()) { char c; f >> posX >> c >> posB; if (!f) posX = posB = -1; }
}
void DealerRead::savePos()
{
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerRead.pos-" + mkt + ".txt").c_str(), std::ios::trunc);
    if (f.is_open()) f << posX << "," << posB << "\n";
}

int DealerRead::mouse(RTX_EVENT* e)
{
    (void)e; return RTX_FAIL;            // (4.3.0) placed by the Position setting - no dragging
}

void DealerRead::renderGrid(const Settings& S)
{
    if (S.rpos == 1 || mkt.empty()) { gripR = gripL; return; }
    if (D.hasTurn || D.hasStage) { renderTurn(S); return; }       // (3.0.0) a level is being worked: the Turn (or "no turn yet")
    loadPos();
    u = S.font / 10.0f;
    int fs = S.font - 1;
    RCT pane; pane.getPaneRect(false);
    short lh = U(16), pad = U(5), gw = U(9), cgap = U(4), labW = U(84);
    struct Cell { std::string code, val; COLOR col; bool guess; };
    struct Line { std::string lab; COLOR lc; bool now; std::vector<Cell> cells; };
    std::vector<Line> L;
    std::string head;
    if (D.hasStage && !D.stgs.empty()) {
        int now = D.stageNow < 1 ? 1 : D.stageNow;
        head = mkt + (D.stageSide == 'L' ? " LONG " : " SHORT ") + D.stageLvl + (D.stageExt.empty() ? "" : "  ext " + D.stageExt + " " + D.stageExtT);
        for (size_t k = 0; k < D.stgs.size(); k++) {
            const dl::Data::Stg& G = D.stgs[k];
            if (S.rstg == 1 && G.n != now) continue;
            Line ln; ln.lab = G.n == 1 ? "1 APPROACH" : G.n == 2 ? "2 TURN" : "3 CONTINUES"; ln.lc = stageCol(G.n); ln.now = G.n == now && D.stageNow > 0;
            for (size_t r = 0; r < D.srows.size() && (int)ln.cells.size() < S.rwid; r++) {
                const dl::Data::SRow& R = D.srows[r];
                if (R.n != G.n || R.val.empty()) continue;
                bool weak = false; for (size_t c = 0; c < R.chips.size(); c++) { const std::string& q = R.chips[c].name; if (q == "No data" || q == "Noise" || q == "Not yet" || q == "Flat" || q == "Mixed" || q == "Thin" || q == "None") weak = true; }
                if (weak) continue;
                Cell c; c.code = R.code; c.val = R.val; c.col = R.side == 'B' ? C_GREEN : R.side == 'S' ? C_RED : C_INK; c.guess = dl::isGuess(R.code);
                ln.cells.push_back(c);
            }
            L.push_back(ln);
        }
    } else {
        head = mkt + "  no level near - forced futures";
        if (D.hasFsum) {
            const dl::Data::FWin& F = D.fsum; float tot = F.gam + F.van + F.cha + F.pos;
            Line ln; ln.lab = "FORCED " + F.ta; ln.lc = C_TABONB; ln.now = false;
            const char* nm[4] = { "Gam", "Van", "Chm", "Pos" }; float v[4] = { F.gam, F.van, F.cha, F.pos };
            for (int i = 0; i < 4; i++) { Cell c; c.code = nm[i]; c.val = dl::signedN(v[i]); c.col = v[i] > 0 ? C_GREEN : v[i] < 0 ? C_RED : C_INK; c.guess = false; ln.cells.push_back(c); }
            Cell t; t.code = "Tot"; t.val = dl::signedN(tot) + " " + dl::shareTxt(tot, F.fut); t.col = tot >= 0 ? C_GREEN : C_RED; t.guess = false; ln.cells.push_back(t);
            L.push_back(ln);
        }
        if (D.hasGstr) {
            const dl::Data::GStr& G = D.gstr; char b[24];
            Line ln; ln.lab = "GAMMA " + G.t; ln.lc = C_TABONB; ln.now = false;
            snprintf(b, sizeof(b), "%.1f/pt", G.g); Cell c0; c0.code = "G"; c0.val = b; c0.col = G.g < 0 ? C_RED : C_GREEN; c0.guess = false; ln.cells.push_back(c0);
            Cell c1; c1.code = "Spd"; c1.val = dl::gChange(G.g, G.sdn) + "/" + dl::gChange(G.g, G.sup); c1.col = C_INK; c1.guess = false; ln.cells.push_back(c1);
            Cell c2; c2.code = "Zom"; c2.val = dl::gChange(G.g, G.zp); c2.col = C_INK; c2.guess = false; ln.cells.push_back(c2);
            Cell c3; c3.code = "Col"; c3.val = dl::gChange(G.g, G.c60); c3.col = C_INK; c3.guess = false; ln.cells.push_back(c3);
            L.push_back(ln);
        }
        if (!D.sigs.empty()) {
            Line ln; ln.lab = "REASONS"; ln.lc = C_TABONB; ln.now = false;
            for (size_t i = 0; i < D.sigs.size() && i < 5; i++) {
                const dl::Data::Sig& g = D.sigs[D.sigs.size() - 1 - i]; char tm[8]; snprintf(tm, sizeof(tm), "%02d:%02d", g.h, g.mi);
                Cell c; c.code = g.code; c.val = tm; c.col = g.side == 'B' ? C_GREEN : C_RED; c.guess = dl::isGuess(g.code); ln.cells.push_back(c);
            }
            L.push_back(ln);
        }
    }
    if (L.empty()) { gripR = gripL; return; }
    // widths: label column + each row's cells; the grid is as wide as its widest row
    int W = 0;
    std::vector<std::vector<int> > cw(L.size());
    for (size_t i = 0; i < L.size(); i++) {
        int w = labW;
        for (size_t c = 0; c < L[i].cells.size(); c++) {
            int x = textW(L[i].cells[c].code.c_str(), fs, true) + U(4) + textW(L[i].cells[c].val.c_str(), fs, false) + U(10);
            cw[i].push_back(x); w += x + cgap;
        }
        if (w > W) W = w;
    }
    int hw = textW(head.c_str(), fs, true) + U(10);
    if (hw > W) W = hw;
    W += gw + 2 * pad;
    gridH = (short)(pad * 2 + lh * (short)(L.size() + 1));
    short clearR = clearRight(pane);
    int ax, ay; dl::anchorXY(pane.left, pane.top + U(16), clearR, pane.bottom, W, gridH, dl::loadPlace("DealerRead", mkt, 6), U(8), ax, ay);
    short x0 = (short)ax, y0 = (short)ay;
    lastClearR = clearR; lastLeft = pane.left; lastW = W;
    gridL = x0; gridT = y0;
    fill(x0, y0, (short)(x0 + W), (short)(y0 + gridH), C_BOXBG);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + gridH), C_BORDER, true);
    gripL = x0; gripT = y0; gripR = (short)(x0 + W); gripB = (short)(y0 + gridH);   // (4.1.2) the whole box is the handle: click anywhere on it and drag
    fill((short)(gripL + 1), (short)(gripT + 1), (short)(x0 + gw), (short)(gripB - 1), 0x001E293B);
    for (short d = (short)(gripT + U(6)); d < gripB - U(4); d = (short)(d + U(4))) { fill((short)(gripL + U(3)), d, (short)(gripL + U(4)), (short)(d + 1), C_MUTED); fill((short)(gripL + U(6)), d, (short)(gripL + U(7)), (short)(d + 1), C_MUTED); }
    short cx0 = (short)(x0 + gw + pad);
    short y = (short)(y0 + pad + lh / 2);
    text(cx0, y, head.c_str(), C_TABONB, fs, true, 0);
    y = (short)(y + lh);
    for (size_t i = 0; i < L.size(); i++) {
        const Line& ln = L[i];
        if (ln.now) fill(cx0, (short)(y - lh / 2 + 1), (short)(cx0 + labW - U(4)), (short)(y + lh / 2 - 1), C_TABON);
        text((short)(cx0 + U(3)), y, ln.lab.c_str(), ln.lc, fs, true, 0);
        short cx = (short)(cx0 + labW);
        for (size_t c = 0; c < ln.cells.size(); c++) {
            const Cell& C = ln.cells[c];
            frame(cx, (short)(y - lh / 2 + 1), (short)(cx + cw[i][c]), (short)(y + lh / 2 - 1), C.guess ? C.col : C_TRACK, C.guess);
            text((short)(cx + U(5)), y, C.code.c_str(), C.col, fs, true, 0);
            text((short)(cx + U(5) + textW(C.code.c_str(), fs, true) + U(4)), y, C.val.c_str(), C_INK, fs, false, 0);
            cx = (short)(cx + cw[i][c] + cgap);
        }
        y = (short)(y + lh);
    }
}

// ------------------------------------------------------------------ (3.0.0) the Turn
// Rassul 2026-10-02: "i want sentences so its more human readable" / "i dont think we should have stages or phases, it should all
// be part of the Turn" / "the turn = the low bar + 2 bars to the left and 2 to the right". One box: the header, then 1) 2) 3) ...
// each = time, the Rev Reason tag (a dashed frame = a guess), the sentence word-wrapped to the box width (grey = a lean).
void DealerRead::renderTurn(const Settings& S)
{
    // (4.0.0, Rassul 2026-10-06 12:44-12:50 "split the dealer read in the middle. Each gets up to 4 points with 3 lines each to make
    // the strongest case for a reversal") one box: the header, then OPTIONS on the left and FOOTPRINT on the right. Each side up to 4
    // numbered points (the Reader picks the strongest FOR the turn), each point at most 3 lines.
    loadPos();
    u = S.font / 10.0f;
    int fs = S.font - 1;
    RCT pane; pane.getPaneRect(false);
    short lh = U(15), pad = U(6), gw = 0, gap = U(6), cgap = U(10);    // (4.4.0) no grip strip (nothing is dragged)
    // (4.4.0, Rassul 2026-10-07 11:01 "today in the morning it had data from 4 or 5 am") how old the file is: > 5 min = a grey
    // "data N min old" after the header; > 15 min = STALE - the header says when it last updated, the outline and text go grey
    RTDATE nowD = currentDate(); struct tm ntm; memset(&ntm, 0, sizeof(ntm)); getLocaltime(nowD, &ntm);
    double ageMin = dealerAgeMin(D.asofSo, D.y, D.mo, D.d, S.clock, ntm);
    bool stale = ageMin > 15.0;
    bool noFp = D.liveRec == "STOPPED";
    // (4.2.0) the width: the room between the chart's left edge and the profiles, times the Width setting (never across the Delta
    // Profile), at most 1000 px at font 10
    short clearR = clearRight(pane);
    short leftEdge = (short)(pane.left + (dltLeft ? dltW + U(110) : 0));
    int avail = clearR - leftEdge - U(10);
    int W = U(760); if (W > avail) W = avail > U(300) ? avail : U(300);       // (4.3.0) one fixed width: it never shakes
    int inner = W - gw - 2 * pad;
    int colW = (inner - cgap) / 2;
    // (4.4.0, Rassul 10:44 "simplify the header line for the dealer read, its too long") ONE line: who is in control, the trade,
    // the targets. Dropped: the market name, "last 90 min", the low / high time, short / long gamma (now on the Session Info) and
    // the bar range - the read line below says them.
    auto simplify = [](const std::string& h) {
        std::vector<std::string> seg; size_t a = 0;
        while (true) { size_t b = h.find(" - ", a); seg.push_back(h.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 3; }
        std::string o;
        for (size_t i = 0; i < seg.size(); i++) {
            const std::string& g = seg[i];
            if (g.empty() || g == "short gamma" || g == "long gamma" || g == "last 90 min" || g.compare(0, 4, "low ") == 0 ||
                g.compare(0, 5, "high ") == 0 || g.compare(0, 5, "bars ") == 0 || (g.size() <= 3 && g.size() >= 2 && isupper((unsigned char)g[0]) && isupper((unsigned char)g[1]))) continue;
            o += (o.empty() ? "" : " - ") + g;
        }
        return o.empty() ? h : o;
    };
    std::vector<std::string> head;
    std::string hd = D.hasTurn ? simplify(to12(D.turnHead.c_str())) : std::string(D.stageSide == 'L' ? "watching LONG at " : "watching SHORT at ") + D.stageLvl + " - no turn yet";
    if (stale) {                                                   // the time of the last update, 12-hour CT
        int so = (int)D.asofSo; char b[48]; int h = so / 3600 % 24;
        snprintf(b, sizeof(b), "STALE - last update %d:%02d %s", h % 12 == 0 ? 12 : h % 12, so / 60 % 60, h < 12 ? "AM" : "PM");
        hd = b;
    }
    std::string tag;                                               // grey, after the header
    if (!stale && ageMin > 10.0) tag = "data " + std::to_string((int)(ageMin + 0.5)) + " min old";
    if (D.liveSrc == "MQ") tag += std::string(tag.empty() ? "" : " - ") + "price: MenthorQ";
    {
        int tagW = tag.empty() ? 0 : textW(("  " + tag).c_str(), fs - 1, false);
        int room = inner - tagW; std::string h1 = hd;
        if (textW(h1.c_str(), fs, true) > room) { while (!h1.empty() && textW((h1 + "...").c_str(), fs, true) > room) h1.erase(h1.size() - 1); h1 += "..."; }
        head.push_back(h1);
    }
    if (!D.terr.empty()) head.push_back("turn read failed: " + D.terr);
    // (4.1.0, Rassul 2026-10-06 16:10 "the read above, right below the header" / 16:14 "highlight anything important" / 16:16 "a
    // lighter shade of gray instead of yellow") the READ LINE: what happened, what to wait for and the flip, prices that matter
    // light grey bold, buyers green, sellers red, the rest a shade dimmer
    auto wSty = [&](const std::string& q, char st) { return (float)textW(q.c_str(), fs, st == 'p' || st == 'b'); };
    std::vector<std::vector<dl::StyPiece> > rdl;
    if (D.hasTurn && !D.readLine.empty()) rdl = dl::wrapStyled(dl::parseStyled(D.readLine), (float)inner, (float)inner, wSty);
    if (head.size() > 1) head.resize(1);                          // (4.4.0) fixed height: header 1 line, read line 3 lines
    if (rdl.size() > 3) rdl.resize(3);
    // the two sides: rows by section (older files without a section: everything on the options side)
    std::vector<dl::Data::TRow> side[2];
    for (size_t i = 0; i < D.trows.size(); i++) side[D.trows[i].sec == 'F' ? 1 : 0].push_back(D.trows[i]);
    struct Col { std::vector<dl::TGroup> G; std::vector<std::vector<std::string> > L; std::vector<int> tagsW; int tW = 0, h = 0; };
    Col C[2];
    int nW = textW("4)", fs, false) + U(4);
    for (int c = 0; c < 2; c++) {
        Col& K = C[c];
        K.G = dl::groupTurn(side[c], 3);           // (4.2.0, Rassul 2026-10-07 08:36 "only top 3 reasons instead of 4")
        K.tW = textW("00:00", fs, false) + gap;
        for (size_t k = 0; k < K.G.size(); k++) { int w = textW(K.G[k].t.c_str(), fs, false) + gap; if (w > K.tW) K.tW = w; }
        K.tagsW.assign(K.G.size(), 0); K.L.assign(K.G.size(), std::vector<std::string>());
        K.h = 1;                                                   // the section label
        for (size_t k = 0; k < K.G.size(); k++) {
            for (size_t j = 0; j < K.G[k].rows.size(); j++) {
                const dl::Data::TRow& R = side[c][K.G[k].rows[j]];
                std::string tg = R.tag + (R.src.empty() ? "" : " " + R.src);
                K.tagsW[k] += textW(tg.c_str(), fs - 1, true) + U(8) + U(3);
            }
            // (4.0.2) lines after the first start under the time (the full column width); (4.1.2) at most 3 lines
            int restW = colW - nW; if (restW < U(80)) restW = U(80);
            int firstW = colW - nW - K.tW - K.tagsW[k] - gap; if (firstW < U(30)) firstW = U(30);
            std::vector<std::string> ln = wrapWords(K.G[k].text, firstW, restW, fs, false, 40);
            if (ln.empty()) ln.push_back("");
            // (4.1.2, Rassul 23:31 "each section can have up to four bullets and each bullet can have up to three lines ... it should
            // not be in more than three lines") a longer point keeps its first 3 lines and ends in "..." (the Reader writes them shorter)
            // (4.4.0, Rassul 10:44 "make the dealer read less taller by utilizing the vertical black space") at most 2 lines a point
            if (ln.size() > 2) {
                ln.resize(2);
                std::string& L3 = ln[1];
                while (!L3.empty() && textW((L3 + "...").c_str(), fs, false) > restW) {
                    size_t sp = L3.find_last_of(' ');
                    if (sp == std::string::npos) { L3.clear(); break; }
                    L3.erase(sp);
                }
                L3 += "...";
            }
            K.L[k] = ln;
            K.h += (int)ln.size();
        }
        if (K.G.empty()) K.h += 1;                                 // "-" when the side has nothing strong
    }
    int bodyLines = C[0].h > C[1].h ? C[0].h : C[1].h;
    int nGroups = (int)(C[0].G.size() > C[1].G.size() ? C[0].G.size() : C[1].G.size());
    // (4.5.0, Rassul 16:45 "the dealer read is taking up a lot of space. so let the vertical height depend on the number of points
    // instead of keeping it fixed") the box is as tall as what it shows: the header, the read lines, and the taller of the two
    // sides (its label + each point's lines + the gap after each point). Still capped at 3 read lines and 3 points x 2 lines.
    int hOpt = C[0].h, hFp = noFp ? 2 : C[1].h;
    int gOpt = (int)C[0].G.size(), gFp = noFp ? 0 : (int)C[1].G.size();
    bodyLines = hOpt > hFp ? hOpt : hFp;
    if (bodyLines > 7) bodyLines = 7;
    nGroups = gOpt > gFp ? gOpt : gFp;
    gridH = dl::readBoxH(pad, lh, (int)head.size(), (int)rdl.size(), bodyLines, nGroups, U(2), U(3), U(6));
    int place = dl::loadPlace("DealerRead", mkt, 6);                // Bottom left unless he picked another spot for this market
    int ax, ay; dl::anchorXY(leftEdge, pane.top + U(16), clearR, pane.bottom, W, gridH, place, U(8), ax, ay);
    short x0 = (short)ax, y0 = (short)ay;
    lastClearR = clearR; lastLeft = pane.left; lastW = W;
    gridL = x0; gridT = y0;
    // (4.4.0, Rassul 10:44) see-through background; the outline says who is in control: lime = demand (bullish), red = supply
    // (bearish), yellow = neither; grey when the data is stale or there is no live turn
    {
        RCT bgR; bgR.set(x0, y0, (short)(x0 + W), (short)(y0 + gridH));
        int bgSet = dialogReady() ? S.bg : dl::loadPlace("DealerReadBg", mkt, S.bg);   // (4.5.2)
        (void)bgSet; bgR.draw(0, C_BOXBG, C_BOXBG, DRAW_OPAQUE, PAT_SOLID);   // (4.5.5, Rassul 2026-10-09 11:22) always a solid background
        std::string h0 = D.hasTurn ? hd : "";                    // the simplified header starts with the control word
        size_t p0 = h0.find_first_not_of(" ");
        std::string w0 = p0 == std::string::npos ? "" : h0.substr(p0, 6);
        COLOR oc = stale || !D.hasTurn || hd.compare(0, 12, "No live turn") == 0 ? C_GREY
                 : w0.compare(0, 6, "DEMAND") == 0 ? 0x0084CC16 : w0.compare(0, 6, "SUPPLY") == 0 ? C_RED : 0x00EAB308;
        frame(x0, y0, (short)(x0 + W), (short)(y0 + gridH), oc, false);
        frame((short)(x0 + 1), (short)(y0 + 1), (short)(x0 + W - 1), (short)(y0 + gridH - 1), oc, false);
    }
    gripL = x0; gripT = y0; gripR = (short)(x0 + W); gripB = (short)(y0 + gridH);
    short cx0 = (short)(x0 + gw + pad);
    short y = (short)(y0 + pad + lh / 2);
    short cxMid = (short)((cx0 + x0 + W - pad) / 2);         // (4.1.1, Rassul 23:04 "the header is justified left. Move it so it's justified center")
    // (4.4.2, Rassul 16:38 "in the dealer read make supply red and demand green in the heading") the control word in its colour
    auto headText = [&](short xs, const std::string& s, COLOR hc) {
        size_t p0 = s.find_first_not_of(' ');
        bool dem = !stale && p0 != std::string::npos && s.compare(p0, 6, "DEMAND") == 0;
        bool sup = !stale && p0 != std::string::npos && s.compare(p0, 6, "SUPPLY") == 0;
        if (!dem && !sup) { text(xs, y, s.c_str(), hc, fs, true, 0); return; }
        size_t e = s.find(' ', p0); if (e == std::string::npos) e = s.size();
        std::string w = s.substr(0, e), rest = s.substr(e);
        text(xs, y, w.c_str(), dem ? C_GREEN : C_RED, fs, true, 0);
        text((short)(xs + textW(w.c_str(), fs, true)), y, rest.c_str(), hc, fs, true, 0);
    };
    for (size_t h = 0; h < head.size(); h++) {
        COLOR hc = h + 1 == head.size() && !D.terr.empty() ? C_MUTED : stale ? C_GREY : C_TABONB;
        if (h == 0 && !tag.empty()) {                              // header + grey tag, centred together
            int w1 = textW(head[0].c_str(), fs, true), w2 = textW(("  " + tag).c_str(), fs - 1, false);
            short xs = (short)(cxMid - (w1 + w2) / 2);
            headText(xs, head[0], hc);
            text((short)(xs + w1), y, ("  " + tag).c_str(), C_MUTED, fs - 1, false, 0);
        } else if (h == 0) headText((short)(cxMid - textW(head[0].c_str(), fs, true) / 2), head[0], hc);
        else text(cxMid, y, head[h].c_str(), hc, fs, true, 1);
        y = (short)(y + lh);
    }
    if (!rdl.empty()) {
        float sw = wSty(" ", 'n');
        for (size_t l = 0; l < rdl.size(); l++) {
            float x = (float)cx0;
            for (size_t k = 0; k < rdl[l].size(); k++) {
                const dl::StyPiece& q = rdl[l][k];
                if (q.sp) x += sw;
                COLOR c = q.s == 'p' ? C_RDHI : q.s == 'g' ? C_RDBUY : q.s == 'r' ? C_RDSELL : q.s == 'b' ? C_RDBOLD : C_RDTXT;
                text((short)x, y, q.t.c_str(), c, fs, q.s == 'p' || q.s == 'b', 0);
                x += wSty(q.t, q.s);
            }
            y = (short)(y + lh);
        }
        fill(cx0, (short)(y - lh / 2 + U(2)), (short)(x0 + W - pad), (short)(y - lh / 2 + U(3)), 0x00334155);    // under the read line
        y = (short)(y + U(6));
    }
    short yBody = (short)(y + U(2));
    short midX = (short)(cx0 + colW + cgap / 2);
    fill(midX, (short)(yBody - lh / 2), (short)(midX + 1), (short)(y0 + gridH - pad), 0x00334155);     // the split down the middle
    for (int c = 0; c < 2; c++) {
        Col& K = C[c];
        short lx = c == 0 ? cx0 : (short)(cx0 + colW + cgap);
        short yy = yBody;
        text((short)(lx + colW / 2), yy, c == 0 ? "OPTIONS" : "FOOTPRINT", C_MUTED, fs - 2, true, 1);   // (4.4.0) centred over its section
        yy = (short)(yy + lh);
        // (4.4.0, Rassul 11:05 "use menthorq or irts own data") the footprint is IRT's alone: while IRT is not recording this market
        // the side says so instead of showing old points
        if (c == 1 && noFp) { text(lx, yy, "no footprint - IRT not recording", 0x00F59E0B, fs, false, 0); continue; }
        if (K.G.empty()) { text(lx, yy, "-", C_MUTED, fs, false, 0); continue; }
        for (size_t k = 0; k < K.G.size(); k++) {
            char nb[16]; snprintf(nb, sizeof(nb), "%d)", (int)k + 1);
            text((short)(lx + nW - U(4)), yy, nb, C_MUTED, fs, false, 2);
            text((short)(lx + nW), yy, K.G[k].t.c_str(), C_MUTED, fs, false, 0);
            short tx = (short)(lx + nW + K.tW);
            bool allLean = true;
            for (size_t j = 0; j < K.G[k].rows.size(); j++) {
                const dl::Data::TRow& R = side[c][K.G[k].rows[j]];
                int r, g, b; dl::tagColour(R.tag, r, g, b);
                COLOR tc = (COLOR)((r << 16) | (g << 8) | b);
                if (R.pole == 'D') tc = C_RDBUY; else if (R.pole == 'S') tc = C_RDSELL;     // (4.1.0) Last 90: green = demand, red = supply
                bool guess = R.kind == "guess" || dl::isGuess(R.tag);
                bool lean = R.kind == "lean"; if (!lean) allLean = false;
                std::string tg = R.tag + (R.src.empty() ? "" : " " + R.src);
                short w = (short)(textW(tg.c_str(), fs - 1, true) + U(8));
                frame(tx, (short)(yy - lh / 2 + 1), (short)(tx + w), (short)(yy + lh / 2 - 1), lean ? C_GREY : tc, guess);
                text((short)(tx + U(4)), yy, tg.c_str(), lean ? C_GREY : tc, fs - 1, true, 0);
                tx = (short)(tx + w + U(3));
            }
            for (size_t l = 0; l < K.L[k].size(); l++) {
                short sx = l == 0 ? (short)(tx + gap) : (short)(lx + nW);
                text(sx, yy, K.L[k][l].c_str(), allLean || stale ? C_GREY : C_INK, fs, false, 0);
                if (l + 1 < K.L[k].size()) yy = (short)(yy + lh);
            }
            yy = (short)(yy + lh + U(3));
        }
    }
}

int DealerRead::draw(void)
{
    // (2026-10-05) IRT runs ONE object of this DLL for every chart: read THIS chart's settings on every draw (the HG chart's settings
    // leaked into the GC chart - status files showed MARKET HG while drawing GC)
    if (dialogReady()) readSettings(cfg);
    load();
    renderGrid(cfg);              // (2.2.0) only the grid - the old Read boxes (Magnet, Key Level, verdict, trade box) are gone
    writeStatus(D.hasPrice ? "drawn" : "no data");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerRead *p = new DealerRead();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);   // (4.5.6) FRONT_DRAWING: the box is drawn after every other indicator, so no line shows through it;   // (2.2.0) TRACK_MOUSE: drag the grid
    p->setDescription("LRA Dealer Read: the Turn - why price turned at the level, as numbered sentences. Drag its grip to move it.");
    p->setVersion("4.5.6");   // (4.5.6) drawn on top of the other indicators (lines no longer cross the solid box); (4.5.5) solid background always; (4.5.4) host-scoped SDK user-data state with done/destroy cleanup; (4.5.3) calendar-aware ASOF age, finite meter guard, bounded status writes; (4.5.2) Background (Solid / See-through) kept after the settings window closes; (4.5.1) the box no longer shakes when two charts share the profiles' status files; (4.5.0) height follows the content; (4.4.2) DEMAND green / SUPPLY red in the header; (4.4.1) "data N min old" from 10 min;   // (4.4.0) see-through, outline by control, 1-line header, 2 lines a point, centred section names, stale / no-footprint states;   // (4.3.0) 9-spot Position setting, fixed size, no drag;   // (4.2.0) bottom-left default, never over the profiles, Move right / Lift settings, Analyst settings gone;   // (4.1.2) 12-hour clock, drag from anywhere on the box, 3 lines a point;   // (4.1.1) header centred, every border solid;   // (4.0.0) OPTIONS | FOOTPRINT split; (4.1.0) the read line + the Last 90 min read
    return p;
}
