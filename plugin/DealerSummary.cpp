/********************************************************************************
 *  DealerSummary.cpp  --  Investor/RT RTX extension  lsDealerSummary  (v1.0.1, 2026-10-08)
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
// windows.h may define the legacy `far` macro before the RTX source is included.
// DealerLogic's shared Node member is named far, so contain that SDK/Windows macro here.
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
#include <ctime>
#include <cctype>

static const char* DSUM_VERSION = "1.0.1";
static const COLOR C_BOXBG  = 0x0005070C;
static const COLOR C_BORDER = 0x00334155;
static const COLOR C_INK    = 0x00E5E7EB;
static const COLOR C_MUTED  = 0x0094A3B8;
static const COLOR C_DIM    = 0x0064748B;
static const COLOR C_OLD    = 0x00F59E0B;
static const int OLD_MIN = 10;          // a read older than this dims (the Reader sends at least every 2 minutes)
static const size_t MAX_SUMMARY_BYTES = 64 * 1024; // external bridge input: enough for a read, bounded on repaint
static const size_t MAX_HEAD_DISPLAY = 512;
static const size_t MAX_BODY_DISPLAY = 4096;

struct YIdx { int market, font, width, top, bottom, moveX; };
static YIdx YX;
struct YSet { int market = 0, font = 10, width = 300, top = 30, bottom = 230, moveX = 0; };

// getUserData()/setUserData() are scoped to the host chart.  The same DLL object can
// service several charts, so no chart-specific settings, file cache, or read may live
// in DealerSummary itself.
struct DSState {
    YSet cfg;
    dl::Summary sm;
    std::string mkt, root, loadedPath, loadError;
    long long loadedStamp = -2;
};

static bool hasVisibleText(const std::string& s)
{
    for (size_t i = 0; i < s.size(); ++i) if (!std::isspace((unsigned char)s[i])) return true;
    return false;
}

// Keep rendering work bounded even if a bridge file is corrupted or unexpectedly verbose.
// The ellipsis is an explicit display truncation; the file remains the sole data source.
static std::string displayPrefix(const std::string& s, size_t maxChars)
{
    if (s.size() <= maxChars) return s;
    size_t cut = s.rfind(' ', maxChars);
    if (cut == std::string::npos || cut < maxChars / 2) cut = maxChars;
    return s.substr(0, cut) + " ...";
}

static bool readSummaryText(const std::string& path, std::string& text, std::string& error)
{
    text.clear(); error.clear();
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f.is_open()) { error = "summary file unavailable"; return false; }
    std::vector<char> bytes(MAX_SUMMARY_BYTES + 1);
    f.read(&bytes[0], (std::streamsize)bytes.size());
    std::streamsize count = f.gcount();
    if (f.bad()) { error = "summary file read failed"; return false; }
    if (count == (std::streamsize)bytes.size()) { error = "summary file exceeds 64 KiB"; return false; }
    text.assign(&bytes[0], (size_t)count);
    return true;
}

class DealerSummary : public cppExtension {
public:
    DealerSummary() : cppExtension() {}
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int done(void);
    virtual int destroy(void);
    virtual int draw(void);

    bool dialogReady() { int i = getListIndex(YX.market); return i >= 0 && i <= 7; }
    DSState* state(bool create);
    HostSlot<DSState> slot_;
    void readSettings(YSet& S);
    void load(DSState& S);
    void fill(short l, short t, short r, short b, COLOR c);
    void frame(short l, short t, short r, short b, COLOR c);
    int textW(const std::string& s, int sz, bool bold);
    void text(short x, short y, const std::string& s, COLOR col, int sz, bool bold);
    void writeStatus(const DSState& S, const char* what, int age);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

DSState* DealerSummary::state(bool create)
{
    return slot_.get(this, create);
}

int DealerSummary::parmsLoad(void)  { if (dialogReady()) readSettings(state(true)->cfg); return RTX_OK; }
int DealerSummary::parmsApply(void) { if (dialogReady()) readSettings(state(true)->cfg); return RTX_OK; }
int DealerSummary::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(state(true)->cfg); return RTX_OK; }

int DealerSummary::done(void)
{
    DSState* S = state(false);
    if (S) { S->sm = dl::Summary(); S->mkt.clear(); S->root.clear(); S->loadedPath.clear(); S->loadError.clear(); S->loadedStamp = -2; }
    return RTX_OK;
}

int DealerSummary::destroy(void)
{
    slot_.release(this);
    return RTX_OK;
}

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

void DealerSummary::load(DSState& S)
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    S.root = rs ? rs : "";
    S.mkt = dl::marketFor(S.cfg.market, S.root);
    if (S.mkt.empty()) { S.sm = dl::Summary(); S.loadedPath.clear(); S.loadedStamp = -2; S.loadError = "no market"; return; }
    const char* up = getenv("USERPROFILE");
    if (!up) { S.sm = dl::Summary(); S.loadedPath.clear(); S.loadedStamp = -2; S.loadError = "USERPROFILE unavailable"; return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Summary-" + S.mkt + ".txt";
    long long st = dl::fileStamp(path);
    if (st == S.loadedStamp && path == S.loadedPath) return;             // unchanged: no file open/read or parse
    S.sm = dl::Summary(); S.loadError.clear(); S.loadedStamp = st; S.loadedPath = path;
    std::string text;
    if (!readSummaryText(path, text, S.loadError)) return;
    S.sm = dl::parseSummary(text);
    if (!S.sm.ok || !hasVisibleText(S.sm.head)) { S.sm = dl::Summary(); S.loadError = "summary content invalid"; }
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

void DealerSummary::writeStatus(const DSState& S, const char* what, int age)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerSummary.status.txt";
    std::string content = std::string("VERSION,") + DSUM_VERSION + "\nROOT," + S.root + "\nMARKET," + S.mkt +
        "\nREAD," + (S.sm.ok ? S.sm.asof : "-") + "\nAGE_MIN," + std::to_string(age) + "\nSTATE," + what + "\n";
    // The status file is intentionally shared for compatibility.  Suppress only an
    // identical consecutive write, so interleaved chart states still replace it.
    static std::string lastPath, lastContent;
    if (path == lastPath && content == lastContent) return;
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << content;
    if (f) { lastPath = path; lastContent = content; }
}

int DealerSummary::draw(void)
{
    DSState* S = state(true);
    load(*S);
    if (!S->sm.ok) { writeStatus(*S, S->mkt.empty() ? "no market" : (S->loadError.empty() ? "no read yet (the Reader has not sent one)" : S->loadError.c_str()), -1); return RTX_OK; }
    int age = dl::ageMin(S->sm.epoch, (long long)time(NULL));
    bool old = age < 0 || age > OLD_MIN; // an absent/invalid/future timestamp must never look live
    RCT pane; pane.getPaneRect(false);
    const int fs = S->cfg.font, lh = fs + 5, pad = 8, acc = 4;
    int W = S->cfg.width; if (W > pane.right - pane.left - 8) W = pane.right - pane.left - 8;
    if (W < 140) { writeStatus(*S, "pane too narrow", age); return RTX_OK; }
    int x0 = pane.left + 4 + S->cfg.moveX, y0 = pane.top + S->cfg.top;
    if (x0 + W > pane.right - 4) x0 = (short)(pane.right - 4 - W);
    if (x0 < pane.left) x0 = pane.left;
    int maxB = pane.bottom - S->cfg.bottom;
    if (maxB > pane.bottom - 2) maxB = pane.bottom - 2;
    const int minBoxH = 2 * pad + 2 * lh + 4; // one headline and the time line
    if (maxB - y0 < minBoxH) { writeStatus(*S, "pane too short", age); return RTX_OK; }
    int inner = W - acc - 2 * pad;
    auto wBold = [&](const std::string& q) { return (float)textW(q, fs, true); };
    auto wNorm = [&](const std::string& q) { return (float)textW(q, fs, false); };
    std::vector<std::string> head = dl::wrapWords(displayPrefix(S->sm.head, MAX_HEAD_DISPLAY), (float)inner, wBold);
    std::vector<std::string> body = dl::wrapWords(displayPrefix(S->sm.body, MAX_BODY_DISPLAY), (float)inner, wNorm);
    auto ellipsize = [&](std::string& line, bool bold) {
        const std::string suffix = " ...";
        const float w = bold ? wBold(line) : wNorm(line);
        if (w <= inner) return;
        while (!line.empty() && (bold ? wBold(line + suffix) : wNorm(line + suffix)) > inner) {
            size_t sp = line.find_last_of(' '); line = sp == std::string::npos ? "" : line.substr(0, sp);
        }
        line += suffix;
    };
    for (size_t i = 0; i < head.size(); ++i) ellipsize(head[i], true);
    for (size_t i = 0; i < body.size(); ++i) ellipsize(body[i], false);
    const int maxHead = 1 + (maxB - y0 - minBoxH) / lh;
    if ((int)head.size() > maxHead) {
        head.resize((size_t)maxHead); head.back() += " ..."; ellipsize(head.back(), true);
    }
    std::string when = S->sm.asof.size() >= 5 ? S->sm.asof.substr(0, 5) + " CT" : "";
    if (old) when = age < 0 ? "OLD - timestamp unavailable" : "OLD - " + dl::ageTxt(age) + (when.empty() ? "" : " (" + when + ")");
    // Fit only complete body lines.  A shallow pane keeps the headline/time line on-screen.
    int fixedH = 2 * pad + lh * (int)head.size() + 4 + lh;
    int fitBody = (maxB - y0 - fixedH) / lh;
    if ((int)body.size() > fitBody) {
        if (fitBody <= 0) body.clear();
        else { body.resize((size_t)fitBody); body.back() += " ..."; ellipsize(body.back(), false); }
    }
    int H = fixedH + lh * (int)body.size();
    fill((short)x0, (short)y0, (short)(x0 + W), (short)(y0 + H), C_BOXBG);
    frame((short)x0, (short)y0, (short)(x0 + W), (short)(y0 + H), C_BORDER);
    COLOR hc = old ? C_DIM : (COLOR)S->sm.col;
    fill((short)(x0 + 1), (short)(y0 + 1), (short)(x0 + 1 + acc), (short)(y0 + H), hc);
    short cx = (short)(x0 + acc + pad), y = (short)(y0 + pad + lh / 2);
    for (size_t i = 0; i < head.size(); i++) { text(cx, y, head[i], hc, fs, true); y = (short)(y + lh); }
    y = (short)(y + 4);
    for (size_t i = 0; i < body.size(); i++) { text(cx, y, body[i], old ? C_DIM : C_INK, fs, false); y = (short)(y + lh); }
    text(cx, y, when, old ? C_OLD : C_MUTED, fs > 8 ? fs - 1 : fs, false);
    writeStatus(*S, old ? "drawn (old read)" : "drawn", age);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerSummary *p = new DealerSummary();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setExtendedFlags(CALL_CONTINUOUSLY); // stale external reads must age even when the feed is inactive
    p->setDescription("LRA Dealer Summary: the LRA Reader's read for this market, in a panel on the left of the chart.");
    p->setVersion(DSUM_VERSION);
    return p;
}
