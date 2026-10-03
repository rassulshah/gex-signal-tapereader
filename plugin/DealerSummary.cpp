/********************************************************************************
 *  DealerSummary.cpp  --  Investor/RT RTX extension  lsDealerSummary  (v1.0.0, 2026-10-03)
 *
 *  Rassul 2026-10-03: "the left of the irt will hold the new summary read, the bottom of the chart will hold the timeline
 *  read" / "lets create a new indicator called Dealer Summary". A vertical panel on the LEFT of the price pane with the
 *  LRA Reader's READ for the chart's market - the same words as the Reader's Read column (Reader 2.58.0): the headline in
 *  its colour (PLAN / WATCH / FAILED BREAKOUT / REVERSAL CANDIDATE / LEVEL FAILURE ...), then the whole read, word-wrapped.
 *  The Dealer Read keeps the timeline (the Turn) at the bottom of the chart; "Room at the bottom" keeps clear of it.
 *  When the read is more than 10 minutes old (the Reader is closed, or the bridge is down) the text dims and the time line
 *  says how old it is - an old read is never shown as if it were live.
 *
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Summary-<MKT>.txt, written by the LRA bridge every time the Reader
 *  sends its reads (level-reversal-analytics analytics/lra/read_store.py). Grammar: DealerLogic.h dl::parseSummary (tested,
 *  test_dealer_logic.cpp). The market comes from the chart's root symbol (EP -> ES, GCE -> GC ...) unless set.
 *  Settings window: controls only. Parameters are read ONLY in the parms callbacks (GAMMA-PROFILE-PLUGIN.md gotcha 3).
 *  Never black text: the chart background is black.
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

static const char* DSUM_VERSION = "1.0.0";
static const COLOR C_BOXBG  = 0x0005070C;
static const COLOR C_BORDER = 0x00334155;
static const COLOR C_INK    = 0x00E5E7EB;
static const COLOR C_MUTED  = 0x0094A3B8;
static const COLOR C_DIM    = 0x0064748B;
static const COLOR C_OLD    = 0x00F59E0B;
static const int OLD_MIN = 10;          // a read older than this dims (the Reader sends at least every 2 minutes)

struct YIdx { int market, font, width, top, bottom, moveX; };
static YIdx YX;
struct YSet { int market = 0, font = 10, width = 300, top = 30, bottom = 230, moveX = 0; };

class DealerSummary : public cppExtension {
public:
    DealerSummary() : cppExtension() {}
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    YSet cfg;
    dl::Summary Sm;
    std::string mkt, root;
    long long loadedStamp = -2; std::string loadedPath;

    bool dialogReady() { int i = getListIndex(YX.market); return i >= 0 && i <= 7; }
    void readSettings(YSet& S);
    void load();
    void fill(short l, short t, short r, short b, COLOR c);
    void frame(short l, short t, short r, short b, COLOR c);
    int textW(const std::string& s, int sz, bool bold);
    void text(short x, short y, const std::string& s, COLOR col, int sz, bool bold);
    void writeStatus(const char* what, int age);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

int DealerSummary::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerSummary::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerSummary::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(5);         // controls only
    const short SL = kParmAppendSameLine;
    YX.market = getParameterCount(); setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    YX.font   = getParameterCount(); setIntegerParameter("Font size (pt)", 10, NUMW, SL);
    YX.width  = getParameterCount(); setIntegerParameter("Width (px)", 300, NUMW);
    YX.moveX  = getParameterCount(); setIntegerParameter("Move right (px)", 0, NUMW, SL);
    YX.top    = getParameterCount(); setIntegerParameter("Down from top (px)", 30, NUMW);
    YX.bottom = getParameterCount(); setIntegerParameter("Room at the bottom (px)", 230, NUMW, SL);
    return RTX_OK;
}

void DealerSummary::readSettings(YSet& S)
{
    S.market = getListIndex(YX.market); if (S.market < 0 || S.market > 7) S.market = 0;
    S.font = getIntegerValue(YX.font); if (S.font < 7 || S.font > 24) S.font = 10;
    S.width = getIntegerValue(YX.width); if (S.width < 140 || S.width > 1200) S.width = 300;
    S.moveX = getIntegerValue(YX.moveX); if (S.moveX < 0 || S.moveX > 3000) S.moveX = 0;
    S.top = getIntegerValue(YX.top); if (S.top < 0 || S.top > 2000) S.top = 30;
    S.bottom = getIntegerValue(YX.bottom); if (S.bottom < 0 || S.bottom > 2000) S.bottom = 230;
}

void DealerSummary::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) { Sm = dl::Summary(); loadedPath.clear(); return; }
    const char* up = getenv("USERPROFILE"); if (!up) { Sm = dl::Summary(); return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Summary-" + mkt + ".txt";
    long long st = dl::fileStamp(path);
    if (st >= 0 && st == loadedStamp && path == loadedPath) return;      // unchanged: keep what is parsed (no disk read)
    Sm = dl::Summary();
    std::ifstream f(path.c_str()); if (!f.is_open()) { loadedPath.clear(); return; }
    std::stringstream ss; ss << f.rdbuf();
    Sm = dl::parseSummary(ss.str());
    loadedStamp = st; loadedPath = path;
}

void DealerSummary::fill(short l, short t, short r, short b, COLOR c) { RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
void DealerSummary::frame(short l, short t, short r, short b, COLOR c)
{
    setPen(c, 1, P_SOLID);
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
int DealerSummary::textW(const std::string& s, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (int)getTextWidth(s.c_str(), -1);
}
// y = the line's middle (the same baseline correction as the Dealer Read)
void DealerSummary::text(short x, short y, const std::string& s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
    int w = (int)getTextWidth(s.c_str(), -1);
    RCT rc; rc.set(x, (short)(yy - sz), (short)(x + w + 4), (short)(yy + sz));
    rc.drawText(s.c_str(), false, false);
}

void DealerSummary::writeStatus(const char* what, int age)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerSummary.status.txt";
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << "VERSION," << DSUM_VERSION << "\nROOT," << root << "\nMARKET," << mkt << "\nREAD," << (Sm.ok ? Sm.asof : "-")
      << "\nAGE_MIN," << age << "\nSTATE," << what << "\n";
}

int DealerSummary::draw(void)
{
    load();
    if (!Sm.ok) { writeStatus(mkt.empty() ? "no market" : "no read yet (the Reader has not sent one)", -1); return RTX_OK; }
    int age = dl::ageMin(Sm.epoch, (long long)time(NULL));
    bool old = age > OLD_MIN;
    RCT pane; pane.getPaneRect(false);
    const int fs = cfg.font, lh = fs + 5, pad = 8, acc = 4;
    int W = cfg.width; if (W > pane.right - pane.left - 8) W = pane.right - pane.left - 8;
    if (W < 140) { writeStatus("pane too narrow", age); return RTX_OK; }
    short x0 = (short)(pane.left + 4 + cfg.moveX), y0 = (short)(pane.top + cfg.top);
    if (x0 + W > pane.right - 4) x0 = (short)(pane.right - 4 - W);
    if (x0 < pane.left) x0 = pane.left;
    int maxB = pane.bottom - cfg.bottom; if (maxB < y0 + 3 * lh) maxB = y0 + 3 * lh;
    if (maxB > pane.bottom - 2) maxB = pane.bottom - 2;
    int inner = W - acc - 2 * pad;
    auto wBold = [&](const std::string& q) { return (float)textW(q, fs, true); };
    auto wNorm = [&](const std::string& q) { return (float)textW(q, fs, false); };
    std::vector<std::string> head = dl::wrapWords(Sm.head, (float)inner, wBold);
    std::vector<std::string> body = dl::wrapWords(Sm.body, (float)inner, wNorm);
    std::string when = Sm.asof.size() >= 5 ? Sm.asof.substr(0, 5) + " CT" : "";
    if (old) when = "OLD - " + dl::ageTxt(age) + (when.empty() ? "" : " (" + when + ")");
    // how many body lines fit between the headline and the time line
    int fixedH = 2 * pad + lh * (int)head.size() + 4 + lh;
    int fitBody = (maxB - y0 - fixedH) / lh; if (fitBody < 1) fitBody = 1;
    if ((int)body.size() > fitBody) {
        body.resize((size_t)fitBody);
        std::string& last = body.back();
        while (!last.empty() && wNorm(last + " ...") > inner) { size_t sp = last.find_last_of(' '); last = sp == std::string::npos ? "" : last.substr(0, sp); }
        last += " ...";
    }
    int H = fixedH + lh * (int)body.size();
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BOXBG);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER);
    COLOR hc = old ? C_DIM : (COLOR)Sm.col;
    fill((short)(x0 + 1), (short)(y0 + 1), (short)(x0 + 1 + acc), (short)(y0 + H), hc);
    short cx = (short)(x0 + acc + pad), y = (short)(y0 + pad + lh / 2);
    for (size_t i = 0; i < head.size(); i++) { text(cx, y, head[i], hc, fs, true); y = (short)(y + lh); }
    y = (short)(y + 4);
    for (size_t i = 0; i < body.size(); i++) { text(cx, y, body[i], old ? C_DIM : C_INK, fs, false); y = (short)(y + lh); }
    text(cx, y, when, old ? C_OLD : C_MUTED, fs > 8 ? fs - 1 : fs, false);
    writeStatus(old ? "drawn (old read)" : "drawn", age);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerSummary *p = new DealerSummary();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Dealer Summary: the LRA Reader's read for this market, in a panel on the left of the chart.");
    p->setVersion("1.0.0");
    return p;
}
