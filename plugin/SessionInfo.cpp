/********************************************************************************
 *  lsSessionInfo - three small boxes (1.1.0: + the NEWS heading on top) on each market's chart, each dragged by its grip like any text box (Rassul 2026-10-06 21:28:
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
#include <cctype>
#include <ctime>

static const COLOR C_INK = 0x00E5E7EB, C_MUTED = 0x009CA3AF, C_GREY = 0x0064748B, C_BOXBG = 0x000B1220, C_BORDER = 0x00334155;
static const COLOR C_AMBER = 0x00F59E0B, C_RED = 0x00EF4444, C_CYAN = 0x0022D3EE;
static const COLOR C_GREEN = 0x0022C55E, C_YELLOW = 0x00FACC15;   // (1.0.2) ACTIVE green, CHOP yellow (Rassul 23:10)   // 0x00RRGGBB, as DealerRead

struct SPIdx { int market, show, font, clock, moveX, moveY; };
static SPIdx SP;
struct SSet { int market = 0, show = 0, font = 10, clock = 0, moveX = 0, moveY = 0; };
struct Win { std::string tag, from, to, pct, lo, hi; };   // (1.2.0) pct = the window's median share of the RTH range, lo-hi = the middle half of days

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
    // (1.1.0) box 2 = the NEWS heading (calendar events + breaking headlines for this market), on top by default
    short bx[3] = {0, 0, 0}, by[3] = {0, 0, 0}, bw[3] = {0, 0, 0}, bh[3] = {0, 0, 0};
    int posX[3] = {-1, -1, -1}, posY[3] = {-1, -1, -1};
    struct Ev { std::string t, imp, title; bool brk; };
    std::vector<Ev> news;
    std::vector<std::pair<std::string, std::string> > pins; std::string pinX, pinXs;   // (1.4.0) PIN|price|share, PINX|price|share
    std::string posMkt;
    int dragging = -1; short dDX = 0, dDY = 0;

    bool ready() { int i = getListIndex(SP.market); return i >= 0 && i <= 7; }
    void readS(SSet& S)
    {
        S.market = getListIndex(SP.market);
        S.show = getListIndex(SP.show); if (S.show < 0 || S.show > 3) S.show = 0;
        S.font = getIntegerValue(SP.font); if (S.font < 7 || S.font > 20) S.font = 10;
        S.clock = getIntegerValue(SP.clock); if (S.clock < -720 || S.clock > 720) S.clock = 0;
        { int v = getIntegerValue(SP.moveX); if (v >= -3000 && v <= 3000) S.moveX = v; }
        { int v = getIntegerValue(SP.moveY); if (v >= -2000 && v <= 2000) S.moveY = v; }
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
    setParameterVersion(2);   // (1.4.0) + Move right / Move down
    setParameterDialogHeight(5);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    SP.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    SP.show   = pc++; setListParameter("Show", 0, "All;0DTE expiry;RTH active / chop;News", 0, SL);
    SP.font   = pc++; setIntegerParameter("Font size (pt)", 10, NUMW);
    SP.clock  = pc++; setIntegerParameter("Clock offset (min)", 0, NUMW, SL);
    // (1.4.0, Rassul 2026-10-07 08:17 "make sure they have settings also like the dealer read that allow me to move them around")
    SP.moveX  = pc++; setIntegerParameter("Move right px (minus = left)", 0, NUMW);
    SP.moveY  = pc++; setIntegerParameter("Move down px (minus = up)", 0, NUMW, SL);
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
    W.clear(); exp.clear(); sample.clear(); news.clear(); pins.clear(); pinX.clear(); pinXs.clear();
    std::ifstream f(p.c_str()); if (!f.is_open()) { path.clear(); return; }
    std::string ln;
    while (std::getline(f, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        std::vector<std::string> c; std::stringstream ss(ln); std::string x;
        while (std::getline(ss, x, '|')) c.push_back(x);
        if (c.empty()) continue;
        if (c[0] == "EXP" && c.size() >= 2) exp = c[1];
        else if (c[0] == "WIN" && c.size() >= 4) {
            Win w; w.tag = c[1]; w.from = c[2]; w.to = c[3];
            if (c.size() >= 8) { w.pct = c[5]; w.lo = c[6]; w.hi = c[7]; }
            W.push_back(w);
        }
        else if (c[0] == "SAMPLE" && c.size() >= 2) sample = c[1];
        else if (c[0] == "PIN" && c.size() >= 3) pins.push_back(std::make_pair(c[1], c[2]));
        else if (c[0] == "PINX" && c.size() >= 3) { pinX = c[1]; pinXs = c[2]; }
        else if ((c[0] == "CAL" || c[0] == "NEWS") && c.size() >= 4) { Ev e; e.t = c[1]; e.imp = c[2]; e.title = c[3]; e.brk = c[0] == "NEWS"; news.push_back(e); }
    }
    stamp = st; path = p;
}

void SessionInfo::loadPos()
{
    if (posMkt == mkt) return;
    posMkt = mkt; for (int b = 0; b < 3; b++) posX[b] = posY[b] = -1;
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    for (int b = 0; b < 3; b++) {
        std::ifstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionInfo.pos-" + mkt + "-" + (b == 2 ? "news" : b ? "rth" : "panel") + ".txt").c_str());
        if (f.is_open()) { char c; f >> posX[b] >> c >> posY[b]; if (!f) posX[b] = posY[b] = -1; }
    }
}

void SessionInfo::savePos(int b)
{
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionInfo.pos-" + mkt + "-" + (b == 2 ? "news" : b ? "rth" : "panel") + ".txt").c_str(), std::ios::trunc);
    if (f.is_open()) f << posX[b] << "," << posY[b] << "\n";
}

// (1.0.2) times on the 12-hour clock, as the Dealer Read (Rassul 23:06)
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
                char b[16]; snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM");
                o += b; i += hl + 2; continue;
            }
        }
        o += s[i];
    }
    return o;
}

void SessionInfo::box(int b, const std::vector<std::pair<std::string, COLOR> >& lines0, const std::vector<bool>& bold, short defX, short defY)
{
    std::vector<std::pair<std::string, COLOR> > lines(lines0);
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
    // (1.3.0, Rassul 2026-10-07 08:00 "the layout is messed up ... i need to be able to select the news ... and drag them") ONE panel,
    // one click-and-drag moves all of it. (1.4.0, 08:17 "the news box should be at the top, with the active times below it and below
    // that the 0 dte box ... keep the lines simple like Active 8:20-9:35 AM (65% of RTH)") order NEWS / ACTIVE-CHOP / 0DTE; the
    // Move right / Move down settings shift the default spot (top of the price pane, a third of the way across); a drag overrides
    // them, a double-click goes back to them.
    short defX = (short)(pane.left + (pane.right - pane.left) / 3 + cfg.moveX), defY = (short)(pane.top + 8 + cfg.moveY);
    auto t12 = [](int m, bool suffix) { char b[16]; int h = (m / 60) % 24; snprintf(b, sizeof b, suffix ? "%d:%02d %s" : "%d:%02d", h % 12 == 0 ? 12 : h % 12, m % 60, h < 12 ? "AM" : "PM"); return std::string(b); };
    auto range12 = [&](int a, int b) { bool same = (a / 60 < 12) == (b / 60 < 12); return same ? t12(a, false) + "-" + t12(b, true) : t12(a, true) + "-" + t12(b, true); };
    std::vector<std::pair<std::string, COLOR> > L; std::vector<bool> B;
    if (cfg.show == 0 || cfg.show == 3) {          // NEWS: upcoming calendar events + breaking news
        bool any = !news.empty();
        L.push_back(std::make_pair(mkt + (any ? " NEWS" : " NEWS   nothing scheduled in 24h, no breaking news"), any ? C_INK : C_GREY)); B.push_back(true);
        for (size_t i = 0; i < news.size(); i++) {
            std::string ttl = news[i].title; if (ttl.size() > 72) ttl = ttl.substr(0, 69) + "...";
            if (news[i].brk) {
                L.push_back(std::make_pair(to12(("BREAKING " + news[i].t + "  " + ttl).c_str()), C_AMBER)); B.push_back(true);
            } else {
                int e = hm(news[i].t);                      // "Thu 07:30" (another day) -> -1: still to come
                bool past = news[i].t.size() == 5 && e >= 0 && e < now && !(now >= 17 * 60 && e < 17 * 60);
                bool soon = news[i].t.size() == 5 && e >= now && e - now <= 30;
                COLOR c = past ? C_GREY : soon ? C_RED : news[i].imp == "High" ? C_AMBER : C_INK;
                L.push_back(std::make_pair(to12((news[i].t + "  " + ttl + (news[i].imp == "High" ? "  (high)" : "")).c_str()), c)); B.push_back(soon);
            }
        }
    }
    if ((cfg.show == 0 || cfg.show == 2) && !W.empty()) {   // ACTIVE / CHOP: "Active 8:20-9:35 AM (64% of RTH)"
        std::string nowTag;
        for (size_t i = 0; i < W.size(); i++) if (now >= hm(W[i].from) && now < hm(W[i].to)) nowTag = W[i].tag;
        L.push_back(std::make_pair(mkt + (nowTag.empty() ? " RTH now: normal" : nowTag == "ACTIVE" ? " RTH now: ACTIVE" : " RTH now: CHOP - wait"),
                                   nowTag == "ACTIVE" ? C_GREEN : nowTag == "CHOP" ? C_YELLOW : C_INK)); B.push_back(true);
        for (size_t i = 0; i < W.size(); i++) {
            bool on = now >= hm(W[i].from) && now < hm(W[i].to);
            std::string ln = std::string(W[i].tag == "ACTIVE" ? "Active " : "Chop ") + range12(hm(W[i].from), hm(W[i].to));
            if (!W[i].pct.empty()) ln += " (" + W[i].pct + "% of RTH)";
            L.push_back(std::make_pair(ln, W[i].tag == "ACTIVE" ? C_GREEN : C_YELLOW)); B.push_back(on);
        }
    }
    if (cfg.show == 0 || cfg.show == 1) {          // 0DTE expiry, last
        int e = hm(exp);
        if (e < 0) { L.push_back(std::make_pair(mkt + " no options expiry today", C_GREY)); B.push_back(false); }
        else {
            int left = e - now;
            if (now >= 17 * 60) left = e + 1440 - now;   // the evening belongs to the next session: tomorrow's expiry
            COLOR c = left <= 0 ? C_GREY : left <= 15 ? C_RED : left <= 60 ? C_AMBER : C_INK;
            if (left > 0) snprintf(s, sizeof s, "%s 0DTE expires %s CT  (in %dh %02dm)", mkt.c_str(), t12(e, true).c_str(), left / 60, left % 60);
            else snprintf(s, sizeof s, "%s 0DTE expired %s CT", mkt.c_str(), t12(e, true).c_str());
            L.push_back(std::make_pair(std::string(s), c)); B.push_back(true);
            // (1.4.0, Rassul 08:18 "the 0DTE box should indicate possible pin target(s) in it on the second line") the 0DTE strikes
            // near price where dealers are long gamma (>= 30% of the 0DTE gamma within 0.5 EM) - or none: short gamma, moves run
            if (left > 0) {
                if (!pins.empty()) {
                    std::string t = pins.size() > 1 ? "Pin targets " : "Pin target ";
                    for (size_t i = 0; i < pins.size(); i++) t += (i ? ", " : "") + pins[i].first + " (" + pins[i].second + "%)";
                    L.push_back(std::make_pair(t + " of 0DTE gamma", C_INK)); B.push_back(false);
                } else if (!pinX.empty()) {
                    L.push_back(std::make_pair("No pin: 0DTE short gamma at " + pinX + " - moves through it can run", C_MUTED)); B.push_back(false);
                }
            }
        }
    }
    bw[1] = bw[2] = 0;
    if (!L.empty()) box(0, L, B, defX, defY); else bw[0] = 0;
    return RTX_OK;
}

int SessionInfo::mouse(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    PNT m; if (m.getMouse(e) != RTX_OK) return RTX_FAIL;
    RCT pane; pane.getPaneRect(false);
    int on = -1;
    for (int b = 0; b < 3; b++)
        if (bw[b] > 0 && m.h >= bx[b] && m.h <= bx[b] + bw[b] && m.v >= by[b] && m.v <= by[b] + bh[b]) on = b;
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
    p->setDescription("LRA Session Info: when today's 0DTE options expire, and the market's ACTIVE / CHOP windows in RTH. Click anywhere on a box and drag it.");
    p->setVersion("1.4.0");   // (1.4.0) order NEWS / ACTIVE-CHOP / 0DTE, simple lines, Move right / down settings;   // (1.3.0) one panel (news + 0DTE + RTH), one drag;   // (1.2.0) each window's share of the RTH range;   // (1.1.0) the NEWS heading: calendar + FinancialJuice breaking news for the market;   // (1.0.2) drag from anywhere on a box; ACTIVE green, CHOP yellow, 12-hour times;   // (1.0.1) evening = the next session's expiry
    return p;
}
