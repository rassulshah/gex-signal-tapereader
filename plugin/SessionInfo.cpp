/********************************************************************************
 *  lsSessionInfo - two small boxes on each market's chart, each dragged by its grip like any text box (Rassul 2026-10-06 21:28:
 *  "I also want to know when 0dte options are expiring for each market ... on the copper chart there should be a label or text
 *  box indicating when the 0dte options expire for copper ... selectable ... so i can move it around" / "the most favorable times
 *  to trade a market ... when each market is actually moving ... so i dont get stuck in chop ... customized to each market and
 *  displayed on its chart, for the rth session"; 21:47 "best time to trade is just when the markets are volatile ... for the rth").
 *
 *    0DTE box     "HG 0DTE expires 12:00 CT  in 1h 25m"   - amber in the last 60 min, red in the last 15, grey once expired
 *                  ("expired 12:00 - hedges released")
 *    RTH box      "ACTIVE 08:10-09:25" / "CHOP 10:40-12:00" (the market's own RTH volatility study, lra/session_info.py:
 *                  15-min high-low range vs its RTH median, 17 months of 3-min bars) + "NOW  ACTIVE" / "NOW  CHOP - wait"
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Session-<MKT>.csv (written by the bridge every few minutes).
 *  Drag a box by its grip (the small bar on its left); double-click the grip = back to the default spot. The spot is kept per
 *  market in SessionInfo.pos-<MKT>-<box>.txt. Settings: controls only. IRT runs ONE object for every chart: settings are read
 *  on every draw.
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

static const COLOR C_INK = 0x00E5E7EB, C_MUTED = 0x009CA3AF, C_GREY = 0x0064748B, C_BOXBG = 0x000B1220, C_BORDER = 0x00334155;
static const COLOR C_AMBER = 0x00F59E0B, C_RED = 0x00EF4444, C_CYAN = 0x0022D3EE;   // 0x00RRGGBB, as DealerRead

struct SPIdx { int market, show, font, clock; };
static SPIdx SP;
struct SSet { int market = 0, show = 0, font = 10, clock = 0; };
struct Win { std::string tag, from, to; };

class SessionInfo : public cppExtension {
public:
    SessionInfo() : cppExtension() {}
    virtual int parmsLoad(void) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int parmsApply(void) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);

    SSet cfg;
    std::string mkt, root, exp, sample;
    std::vector<Win> W;
    long long stamp = -2; std::string path;
    // two boxes: 0 = 0DTE, 1 = RTH windows. pos = offset from the pane's left / top (-1 = default)
    short bx[2] = {0, 0}, by[2] = {0, 0}, bw[2] = {0, 0}, bh[2] = {0, 0};
    int posX[2] = {-1, -1}, posY[2] = {-1, -1};
    std::string posMkt;
    int dragging = -1; short dDX = 0, dDY = 0;

    bool ready() { int i = getListIndex(SP.market); return i >= 0 && i <= 7; }
    void readS(SSet& S)
    {
        S.market = getListIndex(SP.market);
        S.show = getListIndex(SP.show); if (S.show < 0 || S.show > 2) S.show = 0;
        S.font = getIntegerValue(SP.font); if (S.font < 7 || S.font > 20) S.font = 10;
        S.clock = getIntegerValue(SP.clock); if (S.clock < -720 || S.clock > 720) S.clock = 0;
    }
    void load();
    void loadPos(); void savePos(int b);
    int nowMin();
    void fill(short l, short t, short r, short b, COLOR c) { RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
    void frame(short l, short t, short r, short b, COLOR c)
    {
        setPen(c, 1, P_SOLID); PNT p; p.set(0, 0.0f);
        p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
    }
    int textW(const char* s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s, -1); }
    void text(short x, short y, const char* s, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(y - (short)(sz * 0.45f + 0.5f)); int w = (int)getTextWidth(s, -1);
        RCT rc; rc.set(x, (short)(yy - sz), (short)(x + w + 4), (short)(yy + sz)); rc.drawText(s, false, false);
    }
    void box(int b, const std::vector<std::pair<std::string, COLOR> >& lines, const std::vector<bool>& bold, short defX, short defY);
};

int cppExtension::init(void)    { return RTX_OK; }   // required by the SDK (as every LRA plugin)
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(4);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    SP.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    SP.show   = pc++; setListParameter("Show", 0, "Both;0DTE expiry;RTH active / chop", 0, SL);
    SP.font   = pc++; setIntegerParameter("Font size (pt)", 10, NUMW);
    SP.clock  = pc++; setIntegerParameter("Clock offset (min)", 0, NUMW, SL);
    return RTX_OK;
}

static int hm(const std::string& s) { return s.size() >= 5 ? atoi(s.substr(0, 2).c_str()) * 60 + atoi(s.substr(3, 2).c_str()) : -1; }

int SessionInfo::nowMin()
{
    time_t t = time(0); struct tm lt; localtime_s(&lt, &t);
    int m = lt.tm_hour * 60 + lt.tm_min + cfg.clock;
    return (m % 1440 + 1440) % 1440;
}

void SessionInfo::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) { W.clear(); exp.clear(); return; }
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Session-" + mkt + ".csv";
    long long st = dl::fileStamp(p);
    if (st >= 0 && st == stamp && p == path) return;              // unchanged: no disk read
    W.clear(); exp.clear(); sample.clear();
    std::ifstream f(p.c_str()); if (!f.is_open()) { path.clear(); return; }
    std::string ln;
    while (std::getline(f, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        std::vector<std::string> c; std::stringstream ss(ln); std::string x;
        while (std::getline(ss, x, '|')) c.push_back(x);
        if (c.empty()) continue;
        if (c[0] == "EXP" && c.size() >= 2) exp = c[1];
        else if (c[0] == "WIN" && c.size() >= 4) { Win w; w.tag = c[1]; w.from = c[2]; w.to = c[3]; W.push_back(w); }
        else if (c[0] == "SAMPLE" && c.size() >= 2) sample = c[1];
    }
    stamp = st; path = p;
}

void SessionInfo::loadPos()
{
    if (posMkt == mkt) return;
    posMkt = mkt; posX[0] = posY[0] = posX[1] = posY[1] = -1;
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    for (int b = 0; b < 2; b++) {
        std::ifstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionInfo.pos-" + mkt + "-" + (b ? "rth" : "0dte") + ".txt").c_str());
        if (f.is_open()) { char c; f >> posX[b] >> c >> posY[b]; if (!f) posX[b] = posY[b] = -1; }
    }
}

void SessionInfo::savePos(int b)
{
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionInfo.pos-" + mkt + "-" + (b ? "rth" : "0dte") + ".txt").c_str(), std::ios::trunc);
    if (f.is_open()) f << posX[b] << "," << posY[b] << "\n";
}

void SessionInfo::box(int b, const std::vector<std::pair<std::string, COLOR> >& lines, const std::vector<bool>& bold, short defX, short defY)
{
    RCT pane; pane.getPaneRect(false);
    int fs = cfg.font; short lh = (short)(fs * 1.7f + 0.5f), pad = (short)(fs * 0.6f + 0.5f), grip = (short)(fs * 0.8f + 0.5f);
    int w = 0;
    for (size_t i = 0; i < lines.size(); i++) { int tw = textW(lines[i].first.c_str(), fs, bold[i]); if (tw > w) w = tw; }
    short W_ = (short)(w + 2 * pad + grip + 4), H_ = (short)(lines.size() * lh + pad);
    short x0 = posX[b] >= 0 ? (short)(pane.left + posX[b]) : defX;
    short y0 = posY[b] >= 0 ? (short)(pane.top + posY[b]) : defY;
    if (x0 + W_ > pane.right) x0 = (short)(pane.right - W_); if (x0 < pane.left) x0 = pane.left;
    if (y0 + H_ > pane.bottom) y0 = (short)(pane.bottom - H_); if (y0 < pane.top) y0 = pane.top;
    fill(x0, y0, (short)(x0 + W_), (short)(y0 + H_), C_BOXBG);
    frame(x0, y0, (short)(x0 + W_), (short)(y0 + H_), C_BORDER);
    fill((short)(x0 + 2), (short)(y0 + 3), (short)(x0 + grip - 1), (short)(y0 + H_ - 3), C_BORDER);   // the grip
    for (size_t i = 0; i < lines.size(); i++)
        text((short)(x0 + grip + pad), (short)(y0 + pad / 2 + lh * i + lh / 2), lines[i].first.c_str(), lines[i].second, fs, bold[i]);
    bx[b] = x0; by[b] = y0; bw[b] = W_; bh[b] = H_;
}

int SessionInfo::draw(void)
{
    if (ready()) readS(cfg);
    load();
    if (mkt.empty()) return RTX_OK;
    loadPos();
    RCT pane; pane.getPaneRect(false);
    int now = nowMin();
    char s[160];
    short defX = (short)(pane.left + 8), defY = (short)(pane.top + 8);
    if (cfg.show == 0 || cfg.show == 1) {
        std::vector<std::pair<std::string, COLOR> > L; std::vector<bool> B;
        int e = hm(exp);
        if (e < 0) { L.push_back(std::make_pair(mkt + " no options expiry today", C_GREY)); B.push_back(false); }
        else {
            int left = e - now;
            if (now >= 17 * 60) left = e + 1440 - now;   // the evening belongs to the next session: tomorrow's expiry
            COLOR c = left <= 0 ? C_GREY : left <= 15 ? C_RED : left <= 60 ? C_AMBER : C_INK;
            if (left > 0) snprintf(s, sizeof s, "%s 0DTE expires %s CT   in %dh %02dm", mkt.c_str(), exp.c_str(), left / 60, left % 60);
            else snprintf(s, sizeof s, "%s 0DTE expired %s CT - hedges released", mkt.c_str(), exp.c_str());
            L.push_back(std::make_pair(std::string(s), c)); B.push_back(true);
        }
        box(0, L, B, defX, defY);
        defY = (short)(by[0] + bh[0] + 6);
    } else bw[0] = 0;
    if ((cfg.show == 0 || cfg.show == 2) && !W.empty()) {
        std::vector<std::pair<std::string, COLOR> > L; std::vector<bool> B;
        std::string nowTag;
        for (size_t i = 0; i < W.size(); i++) if (now >= hm(W[i].from) && now < hm(W[i].to)) nowTag = W[i].tag;
        snprintf(s, sizeof s, "%s RTH  NOW  %s", mkt.c_str(), nowTag.empty() ? "normal" : nowTag == "ACTIVE" ? "ACTIVE" : "CHOP - wait");
        L.push_back(std::make_pair(std::string(s), nowTag == "ACTIVE" ? C_CYAN : nowTag == "CHOP" ? C_AMBER : C_INK)); B.push_back(true);
        for (size_t i = 0; i < W.size(); i++) {
            bool on = now >= hm(W[i].from) && now < hm(W[i].to), past = now >= hm(W[i].to);
            snprintf(s, sizeof s, "%-6s %s-%s", W[i].tag.c_str(), W[i].from.c_str(), W[i].to.c_str());
            L.push_back(std::make_pair(std::string(s), past ? C_GREY : W[i].tag == "ACTIVE" ? C_CYAN : C_MUTED)); B.push_back(on);
        }
        box(1, L, B, defX, defY);
    } else bw[1] = 0;
    return RTX_OK;
}

int SessionInfo::mouse(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    PNT m; if (m.getMouse(e) != RTX_OK) return RTX_FAIL;
    RCT pane; pane.getPaneRect(false);
    short grip = (short)(cfg.font * 0.8f + 0.5f) + 2;
    int on = -1;
    for (int b = 0; b < 2; b++)
        if (bw[b] > 0 && m.h >= bx[b] && m.h <= bx[b] + grip && m.v >= by[b] && m.v <= by[b] + bh[b]) on = b;
    if (e->type == E_MOUSE_DBL && on >= 0) {
        posX[on] = posY[on] = -1; savePos(on); dragging = -1; trackMouseDrag(false); invalidateChart(); return RTX_OK;
    }
    if (e->type == E_MOUSE_CLICK && on >= 0) {
        dragging = on; dDX = (short)(m.h - bx[on]); dDY = (short)(m.v - by[on]); trackMouseDrag(true); return RTX_OK;
    }
    if (e->type == E_MOUSE_MOVE && dragging >= 0) {
        posX[dragging] = m.h - dDX - pane.left; if (posX[dragging] < 0) posX[dragging] = 0;
        posY[dragging] = m.v - dDY - pane.top; if (posY[dragging] < 0) posY[dragging] = 0;
        invalidateChart(); return RTX_OK;
    }
    if (e->type == E_MOUSE_UP && dragging >= 0) {
        int b = dragging; dragging = -1; trackMouseDrag(false); savePos(b); invalidateChart(); return RTX_OK;
    }
    return RTX_FAIL;
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionInfo *p = new SessionInfo();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);
    p->setDescription("LRA Session Info: when today's 0DTE options expire, and the market's ACTIVE / CHOP windows in RTH. Drag a box by its grip.");
    p->setVersion("1.0.1");   // (1.0.1) evening = the next session's expiry
    return p;
}
