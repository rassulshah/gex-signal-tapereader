/********************************************************************************
 *  DealerRead.cpp  --  Investor/RT RTX extension  lsDealerRead  (v1.0, 2026-09-29)
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
#include "DealerLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
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

struct PIdx { int rstg, rwid, rpos, rtop, view, market, corner, font, trade, todo, clock, layout, explain, keyLvl, keyFuel, keyTrig, lift, moveX, widthPct, show, apos, atop, keyHow; };
static PIdx PX;
struct Settings { int rstg = 0, rwid = 55, rpos = 0, rtop = 30, view = 0, market = 0, corner = 0, font = 10, clock = 0, layout = 0, lift = 30, moveX = 0, widthPct = 100, show = 2, apos = 0, atop = 30; bool trade = true, todo = true, explain = true; };   // layout 0 Stacked, 1 Compact, 2 Mini, 3 Full

class DealerRead : public cppExtension {
public:
    DealerRead();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    Settings cfg;
    dl::Data D;
    std::string mkt, root;
    float u;   // scale: font / 10

    bool dialogReady();
    void readSettings(Settings& S);
    void load();
    long long loadedStamp = -2; std::string loadedPath;   // (1.2.5) re-read the file only when it changed
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
    bool inV14 = false; int profReach = 330; long long reachStamp = -2;   // px the Profile takes from the scale edge (its status file)
    void renderAnalyst(const Settings& S, short x0, short y0, short W, short H);
    std::vector<std::string> wrapWords(const std::string& s, int firstW, int restW, int sz, bool bold, int maxLines);
    void miniBoxes(short& x, short y, const std::vector<dl::Row>& rows, short sz);
    std::string fit(const std::string& s, int maxW, int sz, bool bold);
    short U(float v) { return (short)(v * u + 0.5f); }
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
    bool dragging = false; short gripL = 0, gripT = 0, gripR = 0, gripB = 0, gridL = 0, gridT = 0, gridH = 0, dragDX = 0, dragDY = 0;
    int posX = -1, posB = -1; std::string posMkt;      // where he dropped it: px from the pane's left, px from its bottom (-1 = default)
    void loadPos(); void savePos();
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

DealerRead::DealerRead() : cppExtension() { u = 1.0f; }
bool DealerRead::dialogReady() { int i = getListIndex(PX.market); return i >= 0 && i <= 7; }
int DealerRead::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerRead::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerRead::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
    // (1.3.2, Rassul 2026-09-30: "why cant you have the how-to below fuel as text" / "many dropdowns that shouldnt even be
    // there") only real settings have a control; the whole guide is plain text (setLabelParameter) below them.
    // IRT keeps saved values BY POSITION (the parameter version does not reset them): remove + re-add the indicator once.
    setParameterVersion(9);   // (3.0.0) the Turn: same controls in the same places, two renamed; the guide rewritten
    setParameterDialogHeight(34);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    PX.market   = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    PX.corner   = pc++; setListParameter("Read position", 0, "Bottom left;Bottom right;Top left;Top right;Bottom centre;Top centre", 0, SL);
    PX.font     = pc++; setIntegerParameter("Font size (pt)", 10, 0);
    PX.trade    = pc++; setBoolParameter("TRADE line", true, SL);
    PX.todo     = pc++; setBoolParameter("WHAT TO DO box", true, SL);
    PX.layout   = pc++; setListParameter("Layout", 0, "Stacked;Compact;Mini;Full (wide)");
    PX.show     = pc++; setListParameter("Show", 2, "Read;Analyst;Both", 0, SL);
    PX.lift     = pc++; setIntegerParameter("Read: lift from bottom (px)", 30, 0);
    PX.moveX    = pc++; setIntegerParameter("Move right (px)", 0, 0, SL);
    PX.apos     = pc++; setListParameter("Analyst position", 0, "Top left;Top centre;Top right;Above the Read");
    PX.atop     = pc++; setIntegerParameter("Analyst: down from top (px)", 30, 0, SL);
    PX.widthPct = pc++; setIntegerParameter("Width (% 40-100)", 100, 0);
    PX.clock    = pc++; setIntegerParameter("Clock offset (min)", 0, 0, SL);
    PX.view     = pc++; setListParameter("View", 0, "Standard;Summary");   // (1.6.0) Summary = the whole read as sentences
    PX.rpos     = pc++; setListParameter("Turn read", 0, "Show;Off");                    // (3.0.0) the Turn box; drag its grip to move it
    PX.rtop     = pc++; setIntegerParameter("Box: lift from bottom (px)", 30, 0, SL);
    PX.rstg     = pc++; setListParameter("Reasons shown", 0, "All;Last 4");          // (3.0.0)
    PX.rwid     = pc++; setIntegerParameter("Box width (% of chart 30-100)", 55, 0, SL);
    pc++; setLabelParameter("THE TURN (3.0.0) - why price turned at the level, from", 380);
    pc++; setLabelParameter("  the options data, in plain sentences, in the order", 380);
    pc++; setLabelParameter("  they became known. The turn = the low (high) bar and", 380);
    pc++; setLabelParameter("  2 bars either side; the tape is only used for demand /", 380);
    pc++; setLabelParameter("  supply bars and failed breaks (trapped traders).", 380);
    pc++; setLabelParameter("Each reason has its Rev Reason tag: Exhaustion, Tape,", 380);
    pc++; setLabelParameter("  Short cover, IV move, Fear/greed fading, Pin, Pin Exp,", 380);
    pc++; setLabelParameter("  Cushion, 0DTE fear leaving, Profit taking ?, New bets ?", 380);
    pc++; setLabelParameter("  A dashed tag = an educated guess (checked against the", 380);
    pc++; setLabelParameter("  next day's open interest); grey text = a lean only.", 380);
    pc++; setLabelParameter("  GLD / SPX / QQQ / USO after a tag = from the ETF, used", 380);
    pc++; setLabelParameter("  only when the futures options show nothing for it.", 380);
    pc++; setLabelParameter("e.g. HG 2 Oct 10:45: 1) Tape - the breakdown under 6.5405", 380);
    pc++; setLabelParameter("  failed, sellers trapped; 2) Short cover - dealers bought", 380);
    pc++; setLabelParameter("  back 20 of the 44 they sold; 3) Pin - the expiring 6.55", 380);
    pc++; setLabelParameter("  strike held price into the 12:00 expiry.", 380);
    pc++; setLabelParameter("The header ends with fuel left = futures dealers still", 380);
    pc++; setLabelParameter("  have to buy (sell) back - more fuel for the turn.", 380);
    pc++; setLabelParameter("MOVE IT: drag the grip on its left edge; double-click", 380);
    pc++; setLabelParameter("  the grip to put it back at the bottom left", 380);
    PX.keyHow = PX.keyTrig = PX.keyLvl = PX.keyFuel = PX.explain = -1;
    return RTX_OK;
}

void DealerRead::readSettings(Settings& S)
{
    S.market = getListIndex(PX.market);
    S.corner = getListIndex(PX.corner); if (S.corner < 0 || S.corner > 5) S.corner = 0;
    S.moveX = getIntegerValue(PX.moveX); if (S.moveX < -3000 || S.moveX > 3000) S.moveX = 0;
    S.widthPct = getIntegerValue(PX.widthPct); if (S.widthPct < 40 || S.widthPct > 100) S.widthPct = 100;   // a fresh dialog can read 0
    S.show = getListIndex(PX.show); if (S.show < 0 || S.show > 2) S.show = 2;
    S.apos = getListIndex(PX.apos); if (S.apos < 0 || S.apos > 3) S.apos = 0;
    S.atop = getIntegerValue(PX.atop); if (S.atop < 0 || S.atop > 600) S.atop = 30;
    S.font = getIntegerValue(PX.font); if (S.font < 7) S.font = 10;   // (1.0.1) a first dialog can show ??? / a wrong number if (S.font > 20) S.font = 20;
    S.trade = isBoxChecked(PX.trade) != 0; S.todo = isBoxChecked(PX.todo) != 0;
    S.clock = getIntegerValue(PX.clock); if (S.clock < -720) S.clock = -720; if (S.clock > 720) S.clock = 720;
    S.layout = getListIndex(PX.layout); if (S.layout < 0 || S.layout > 3) S.layout = 0;
    S.lift = getIntegerValue(PX.lift); if (S.lift < 0 || S.lift > 400) S.lift = 30;
    S.view = getListIndex(PX.view); if (S.view < 0 || S.view > 1) S.view = 0;
    S.rpos = getListIndex(PX.rpos); if (S.rpos < 0 || S.rpos > 1) S.rpos = 0;
    S.rtop = getIntegerValue(PX.rtop); if (S.rtop < 0 || S.rtop > 900) S.rtop = 30;
    S.rstg = getListIndex(PX.rstg); if (S.rstg < 0 || S.rstg > 1) S.rstg = 0;
    S.rwid = getIntegerValue(PX.rwid); if (S.rwid < 30 || S.rwid > 100) S.rwid = 55;     // (3.0.0) box width, % of the chart
    S.explain = false;
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
    setPen(c, 1, dashed ? P_DOT : P_SOLID);
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
int DealerRead::textW(const char* s, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (int)getTextWidth(s, -1);
}
void DealerRead::text(short x, short y, const char* s, COLOR col, int sz, bool bold, int just)
{
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
    if (!r.hasV) { text(mid, y, r.lab.c_str(), C_MUTED, fs, false, 1); return; }
    short ln = (short)(w / 2 * std::fabs(r.v) / 100.0f);
    COLOR c = r.v >= 0 ? C_GREEN : C_RED;
    if (r.v >= 0) fill(mid, (short)(y - h / 2), (short)(mid + ln), (short)(y + h / 2), c);
    else          fill((short)(mid - ln), (short)(y - h / 2), mid, (short)(y + h / 2), c);
    line(mid, (short)(y - h / 2 - 1), mid, (short)(y + h / 2 + 1), 0x0094A3B8, 1);
    int tw = textW(r.lab.c_str(), fs, true);
    if (r.v >= 0) {
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
    double age = dl::staleMin(D.asofSo + S.clock * 60.0, t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec);
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

static std::string bookQ(const std::string& st) { return st == "MATCH" ? "ok" : st == "FLIP" ? "bad" : st == "MIXED" ? "neutral" : "wait"; }

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
    double age = dl::staleMin(D.asofSo + S.clock * 60.0, tt.tm_hour * 3600.0 + tt.tm_min * 60.0 + tt.tm_sec);
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
    double age = dl::staleMin(D.asofSo + S.clock * 60.0, tt.tm_hour * 3600.0 + tt.tm_min * 60.0 + tt.tm_sec);
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
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << "VERSION,3.0.0\nROOT," << root << "\nMARKET," << mkt << "\nLEVEL," << (D.hasLevel ? D.lvlLabel : "none") << "\nPHASE," << D.phase << "\nSTATE," << what << "\n";
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
    if (!e) return RTX_FAIL;
    PNT m; if (m.getMouse(e) != RTX_OK) return RTX_FAIL;
    bool onGrip = m.h >= gripL && m.h <= gripR && m.v >= gripT && m.v <= gripB && gripR > gripL;
    RCT pane; pane.getPaneRect(false);
    if (e->type == E_MOUSE_DBL && onGrip) {
        posX = posB = -1; savePos(); dragging = false; trackMouseDrag(false); invalidateChart(); return RTX_OK;
    }
    if (e->type == E_MOUSE_CLICK && onGrip) {
        dragging = true; dragDX = (short)(m.h - gridL); dragDY = (short)(m.v - gridT); trackMouseDrag(true); return RTX_OK;
    }
    if (e->type == E_MOUSE_MOVE && dragging) {
        posX = m.h - dragDX - pane.left; if (posX < 0) posX = 0;
        posB = pane.bottom - (m.v - dragDY + gridH); if (posB < 0) posB = 0;
        invalidateChart(); return RTX_OK;
    }
    if (e->type == E_MOUSE_UP && dragging) {
        dragging = false; trackMouseDrag(false); savePos(); invalidateChart(); return RTX_OK;
    }
    return RTX_FAIL;
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
    short x0 = posX >= 0 ? (short)(pane.left + posX) : (short)(pane.left + U(10));
    short maxR = (short)(pane.right - U(4));
    if (x0 + W > maxR) x0 = (short)(maxR - W);
    if (x0 < pane.left) x0 = pane.left;
    short y0 = posB >= 0 ? (short)(pane.bottom - posB - gridH) : (short)(pane.bottom - S.rtop - gridH);
    if (y0 < pane.top) y0 = pane.top;
    gridL = x0; gridT = y0;
    fill(x0, y0, (short)(x0 + W), (short)(y0 + gridH), C_BOXBG);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + gridH), C_BORDER, true);
    gripL = x0; gripT = y0; gripR = (short)(x0 + gw); gripB = (short)(y0 + gridH);
    fill((short)(gripL + 1), (short)(gripT + 1), gripR, (short)(gripB - 1), 0x001E293B);
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
    loadPos();
    u = S.font / 10.0f;
    int fs = S.font - 1;
    RCT pane; pane.getPaneRect(false);
    short lh = U(15), pad = U(6), gw = U(9), gap = U(6);
    int W = (int)((pane.right - pane.left) * S.rwid / 100);
    int minW = U(360); if (W < minW) W = minW;
    if (W > pane.right - pane.left - U(8)) W = pane.right - pane.left - U(8);
    int inner = W - gw - 2 * pad;
    auto wBold = [&](const std::string& q) { return (float)textW(q.c_str(), fs, true); };
    auto wNorm = [&](const std::string& q) { return (float)textW(q.c_str(), fs, false); };
    std::vector<std::string> head;
    if (D.hasTurn) head = dl::wrapWords(D.turnHead, (float)inner, wBold);
    else head.push_back(mkt + (D.stageSide == 'L' ? " - watching LONG at " : " - watching SHORT at ") + D.stageLvl + " - no turn yet");
    if (!D.terr.empty()) head.push_back("turn read failed: " + D.terr);
    // the reason rows: number, time, tag, then the sentence lines
    int nW = textW("10)", fs, false) + U(4), tW = textW("00:00", fs, false) + gap, tagW = 0;
    std::vector<size_t> idx;
    for (size_t i = 0; i < D.trows.size(); i++) idx.push_back(i);
    if (S.rstg == 1 && idx.size() > 4) idx.erase(idx.begin(), idx.end() - 4);
    for (size_t k = 0; k < idx.size(); k++) {
        const dl::Data::TRow& R = D.trows[idx[k]];
        std::string tg = R.tag + (R.src.empty() ? "" : " " + R.src);
        int w = textW(tg.c_str(), fs - 1, true) + U(10); if (w > tagW) tagW = w;
    }
    int txW = inner - nW - tW - tagW - gap; if (txW < U(120)) txW = U(120);
    std::vector<std::vector<std::string> > lines(idx.size());
    int nLines = (int)head.size();
    for (size_t k = 0; k < idx.size(); k++) { lines[k] = dl::wrapWords(D.trows[idx[k]].text, (float)txW, wNorm); if (lines[k].empty()) lines[k].push_back(""); nLines += (int)lines[k].size(); }
    gridH = (short)(pad * 2 + lh * nLines + U(3) * (short)idx.size());
    short x0 = posX >= 0 ? (short)(pane.left + posX) : (short)(pane.left + U(10));
    short maxR = (short)(pane.right - U(4));
    if (x0 + W > maxR) x0 = (short)(maxR - W);
    if (x0 < pane.left) x0 = pane.left;
    short y0 = posB >= 0 ? (short)(pane.bottom - posB - gridH) : (short)(pane.bottom - S.rtop - gridH);
    if (y0 < pane.top) y0 = pane.top;
    gridL = x0; gridT = y0;
    fill(x0, y0, (short)(x0 + W), (short)(y0 + gridH), C_BOXBG);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + gridH), C_BORDER, true);
    gripL = x0; gripT = y0; gripR = (short)(x0 + gw); gripB = (short)(y0 + gridH);
    fill((short)(gripL + 1), (short)(gripT + 1), gripR, (short)(gripB - 1), 0x001E293B);
    for (short d = (short)(gripT + U(6)); d < gripB - U(4); d = (short)(d + U(4))) { fill((short)(gripL + U(3)), d, (short)(gripL + U(4)), (short)(d + 1), C_MUTED); fill((short)(gripL + U(6)), d, (short)(gripL + U(7)), (short)(d + 1), C_MUTED); }
    short cx0 = (short)(x0 + gw + pad);
    short y = (short)(y0 + pad + lh / 2);
    for (size_t h = 0; h < head.size(); h++) { text(cx0, y, head[h].c_str(), h + 1 == head.size() && !D.terr.empty() ? C_MUTED : C_TABONB, fs, true, 0); y = (short)(y + lh); }
    for (size_t k = 0; k < idx.size(); k++) {
        const dl::Data::TRow& R = D.trows[idx[k]];
        y = (short)(y + U(3));
        char nb[8]; snprintf(nb, sizeof(nb), "%d)", R.n);
        text((short)(cx0 + nW - U(4)), y, nb, C_MUTED, fs, false, 2);
        text((short)(cx0 + nW), y, R.t.c_str(), C_MUTED, fs, false, 0);
        int r, g, b; dl::tagColour(R.tag, r, g, b);
        COLOR tc = (COLOR)((r << 16) | (g << 8) | b);
        bool guess = R.kind == "guess" || dl::isGuess(R.tag);
        bool lean = R.kind == "lean";
        std::string tg = R.tag + (R.src.empty() ? "" : " " + R.src);
        short tx = (short)(cx0 + nW + tW);
        frame(tx, (short)(y - lh / 2 + 1), (short)(tx + textW(tg.c_str(), fs - 1, true) + U(8)), (short)(y + lh / 2 - 1), lean ? C_GREY : tc, guess);
        text((short)(tx + U(4)), y, tg.c_str(), lean ? C_GREY : tc, fs - 1, true, 0);
        short sx = (short)(cx0 + nW + tW + tagW + gap);
        for (size_t l = 0; l < lines[k].size(); l++) {
            text(sx, y, lines[k][l].c_str(), lean ? C_GREY : C_INK, fs, false, 0);
            if (l + 1 < lines[k].size()) y = (short)(y + lh);
        }
        y = (short)(y + lh);
    }
}

int DealerRead::draw(void)
{
    load();
    renderGrid(cfg);              // (2.2.0) only the grid - the old Read boxes (Magnet, Key Level, verdict, trade box) are gone
    writeStatus(D.hasPrice ? "drawn" : "no data");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerRead *p = new DealerRead();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);   // (2.2.0) TRACK_MOUSE: drag the grid
    p->setDescription("LRA Dealer Read: the Turn - why price turned at the level, as numbered sentences. Drag its grip to move it.");
    p->setVersion("3.0.0");
    return p;
}
