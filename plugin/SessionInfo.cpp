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

struct SPIdx { int market, show, font, clock, moveX, moveY, bg; };
static SPIdx SP;
struct SSet { int market = 0, show = 0, font = 8, clock = 0, moveX = 0, moveY = 0, bg = 0; };   // (1.6.0) bg 0 = see-through, 1 = solid
struct Win { std::string tag, from, to, pct, lo, hi; };   // (1.2.0) pct = the window's median share of the RTH range, lo-hi = the middle half of days

class SessionInfo : public cppExtension {
public:
    SessionInfo() : cppExtension() {}
    virtual int parmsLoad(void) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int parmsApply(void) { if (ready()) { readS(cfg); savePl(); } return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { if (ready()) { readS(cfg); savePl(); } return RTX_OK; }
    void savePl() { char b[32] = {0}; const char* rs = getRootSymbol(b); dl::savePlace("SessionInfo", dl::marketFor(cfg.market, rs ? rs : ""), cfg.moveX); }
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
    struct Ev { std::string t, imp, title, cd; bool brk; };   // (1.6.0) cd = "in 2h 05m" on the next high-impact item
    std::vector<Ev> news;
    std::vector<std::pair<std::string, std::string> > pins; std::string pinX, pinXs;   // (1.4.0) PIN|price|share, PINX|price|share
    std::string posMkt;
    std::string gam, hvl, gw0, gw0d, emLo, emHi, emPx, emPct, emUsed, dAge, dRec;   // (1.6.0) GAM / GW0 / EMR / DAGE rows
    int dragging = -1; short dDX = 0, dDY = 0;

    bool ready() { int i = getListIndex(SP.market); return i >= 0 && i <= 7; }
    void readS(SSet& S)
    {
        S.market = getListIndex(SP.market);
        S.show = getListIndex(SP.show); if (S.show < 0 || S.show > 4) S.show = 0;
        S.bg = getListIndex(SP.bg); if (S.bg < 0 || S.bg > 1) S.bg = 0;
        S.font = getIntegerValue(SP.font); if (S.font < 6 || S.font > 20) S.font = 8;
        S.clock = getIntegerValue(SP.clock); if (S.clock < -720 || S.clock > 720) S.clock = 0;
        { int v = getListIndex(SP.moveX); if (v >= 0 && v <= 8) S.moveX = v; }   // (1.5.0) moveX = the Position (0-8)
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
    std::string fit(const std::string& s, int w, int fs, bool bold)   // (1.6.0) cut to the box width with "..."
    {
        if (textW(s.c_str(), fs, bold) <= w) return s;
        std::string t = s;
        while (!t.empty() && textW((t + "...").c_str(), fs, bold) > w) t.erase(t.size() - 1);
        while (!t.empty() && t[t.size() - 1] == ' ') t.erase(t.size() - 1);
        return t + "...";
    }
};

int cppExtension::init(void)    { return RTX_OK; }   // required by the SDK (as every LRA plugin)
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(5);   // (1.5.0) the 9-spot Position replaces Move right / down; (1.6.0) Background
    setParameterDialogHeight(5);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    SP.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    SP.show   = pc++; setListParameter("Show", 0, "All;0DTE + gamma;RTH active / chop;News;Expected move", 0, SL);
    SP.font   = pc++; setIntegerParameter("Font size (pt)", 8, NUMW);   // (1.4.0, Rassul 08:22 "make the font ... smaller like around 8pt")
    SP.clock  = pc++; setIntegerParameter("Clock offset (min)", 0, NUMW, SL);
    // (1.4.0, Rassul 2026-10-07 08:17 "make sure they have settings also like the dealer read that allow me to move them around")
    // (1.5.0, Rassul 2026-10-07 09:16 "top left, top center ... middle right instead of the user entering amount of placement")
    SP.moveX  = pc++; setListParameter("Position", 0, dl::ANCHORS);
    SP.moveY  = -1;
    // (1.6.0, Rassul 2026-10-07 10:44 "a more transparent background so the candles are more visible")
    SP.bg     = pc++; setListParameter("Background", 0, "See-through;Solid", 0, SL);
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
    gam.clear(); hvl.clear(); gw0.clear(); gw0d.clear(); emLo.clear(); emHi.clear(); emPx.clear(); emPct.clear(); emUsed.clear(); dAge.clear(); dRec.clear();
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
        else if ((c[0] == "CAL" || c[0] == "NEWS") && c.size() >= 4) { Ev e; e.t = c[1]; e.imp = c[2]; e.title = c[3]; e.brk = c[0] == "NEWS"; if (c.size() >= 5) e.cd = c[4]; news.push_back(e); }
        else if (c[0] == "GAM" && c.size() >= 3) { gam = c[1]; hvl = c[2]; }
        else if (c[0] == "GW0" && c.size() >= 2) { gw0 = c[1]; gw0d = c.size() >= 3 ? c[2] : ""; }
        else if (c[0] == "EMR" && c.size() >= 6) { emLo = c[1]; emHi = c[2]; emPx = c[3]; emPct = c[4]; emUsed = c.size() >= 6 ? c[5] : ""; }
        else if (c[0] == "DAGE" && c.size() >= 3) { dAge = c[1]; dRec = c[2]; }
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
    int fs = cfg.font; short lh = (short)(fs * 1.7f + 0.5f), pad = (short)(fs * 0.6f + 0.5f);
    // (1.6.0, Rassul 2026-10-07 10:44-10:47 "the session indicator is too wide because of the news ... capping it" -> 250 px at
    // 8 pt in the mockup) a FIXED width, scaled with the font; a longer line ends in "..." instead of widening the box
    short W_ = (short)(250.0f * fs / 8.0f + 0.5f);
    int maxW = W_ - 2 * pad;
    short H_ = (short)(lines.size() * lh + pad);
    (void)defX; (void)defY;
    int ax, ay; dl::anchorXY(pane.left, (short)(pane.top + 18), dl::clearRightOf(pane.right), pane.bottom, W_, H_, dl::loadPlace("SessionInfo", mkt, 0), 8, ax, ay);   // (1.6.1) below IRT's own title line
    short x0 = (short)ax, y0 = (short)ay;
    // (1.6.0) see-through by default: IRT's translucent fill lets the candles show through
    RCT bgR; bgR.set(x0, y0, (short)(x0 + W_), (short)(y0 + H_));
    bgR.draw(1, C_BORDER, C_BOXBG, cfg.bg == 1 ? DRAW_OPAQUE : DRAW_TRANSLUCENT, PAT_SOLID);
    for (size_t i = 0; i < lines.size(); i++) {
        short yc = (short)(y0 + pad / 2 + lh * i + lh / 2);
        if (!lines[i].first.empty() && lines[i].first[0] == '\x01') {          // the expected-move bar: "\x01<pct>"
            int pct = atoi(lines[i].first.c_str() + 1); if (pct < 0) pct = 0; if (pct > 100) pct = 100;
            short bl = (short)(x0 + pad), br = (short)(x0 + W_ - pad), bt = (short)(yc - fs / 2 + 1), bb = (short)(yc + fs / 2 - 1);
            fill(bl, bt, br, bb, 0x001F2937);
            short mx = (short)(bl + (br - bl) * pct / 100);
            fill(bl, bt, mx, bb, C_BORDER);
            fill((short)(mx - 1), (short)(bt - 3), (short)(mx + 2), (short)(bb + 3), C_INK);
            continue;
        }
        text((short)(x0 + pad), yc, fit(lines[i].first, maxW, fs, bold[i]).c_str(), lines[i].second, fs, bold[i]);
    }
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
    char s[200];
    short defX = (short)(pane.left + 8), defY = (short)(pane.top + 8);
    auto t12 = [](int m, bool suffix) { char b[16]; int h = (m / 60) % 24; snprintf(b, sizeof b, suffix ? "%d:%02d %s" : "%d:%02d", h % 12 == 0 ? 12 : h % 12, m % 60, h < 12 ? "AM" : "PM"); return std::string(b); };
    auto range12 = [&](int a, int b) { bool same = (a / 60 < 12) == (b / 60 < 12); return same ? t12(a, false) + "-" + t12(b, true) : t12(a, true) + "-" + t12(b, true); };
    std::vector<std::pair<std::string, COLOR> > L; std::vector<bool> B;
    auto add = [&](const std::string& t, COLOR c, bool b) { L.push_back(std::make_pair(t, c)); B.push_back(b); };
    auto gapLine = [&]() { if (!L.empty() && !L.back().first.empty()) add("", C_INK, false); };
    bool all = cfg.show == 0;
    // (1.6.0, Rassul 10:44-10:58) order: NEWS / gamma + 0DTE + pin / ACTIVE-CHOP / expected move, a blank line between them
    if (all || cfg.show == 3) {
        bool any = !news.empty();
        add(mkt + (any ? " NEWS" : " NEWS - none in 24h"), any ? C_INK : C_GREY, true);
        for (size_t i = 0; i < news.size(); i++) {
            if (news[i].brk) {                          // (1.6.0 "you do not need to add breaking") time + headline, amber
                add(to12((news[i].t + "  " + news[i].title).c_str()), C_AMBER, true);
            } else {
                int e = hm(news[i].t);
                bool past = news[i].t.size() == 5 && e >= 0 && e < now && !(now >= 17 * 60 && e < 17 * 60);
                bool soon = news[i].t.size() == 5 && e >= now && e - now <= 30;
                COLOR c = past ? C_GREY : soon ? C_RED : news[i].imp == "High" ? C_AMBER : C_INK;
                std::string ln = news[i].t + "  " + news[i].title;
                if (!news[i].cd.empty() && !past) ln += " - " + news[i].cd;      // (1.6.0) the next high item counts down
                add(to12(ln.c_str()), c, soon || !news[i].cd.empty());
            }
        }
    }
    if (all || cfg.show == 1) {
        gapLine();
        // (1.6.0, Rassul 10:44 "before the 0 dte line indicate if the market regime is positive or negative gamma"; 10:48-10:59
        // "levels can break" on one line, the HVL on its own line under it)
        if (!gam.empty()) {
            bool neg = gam == "NEG";
            add(neg ? "Gamma NEGATIVE - levels can break" : "Gamma POSITIVE - levels tend to hold", neg ? C_AMBER : C_INK, true);
            if (!hvl.empty()) add(std::string(neg ? "Turns positive above HVL " : "Turns negative below HVL ") + hvl, C_MUTED, false);
        }
        int e = hm(exp);
        if (e < 0) add(mkt + " no options expiry today", C_GREY, false);
        else {
            int left = e - now;
            if (now >= 17 * 60) left = e + 1440 - now;
            COLOR c = left <= 0 ? C_GREY : left <= 15 ? C_RED : left <= 60 ? C_AMBER : C_INK;
            if (left > 0) snprintf(s, sizeof s, "%s 0DTE expires %s (in %dh %02dm)", mkt.c_str(), t12(e, true).c_str(), left / 60, left % 60);
            else snprintf(s, sizeof s, "%s 0DTE expired %s", mkt.c_str(), t12(e, true).c_str());
            add(s, c, true);
            // (1.6.0, Rassul 10:51 "a strike with a lot of gamma ... which is one of the menthor q levels") the pin = MenthorQ's 0DTE
            // Gamma Wall (GW0), and how far price is from it
            if (left > 0) {
                if (!gw0.empty()) {
                    std::string ln = "Pin GW0 " + gw0;
                    if (!gw0d.empty()) {
                        bool above = gw0d[0] != '-';
                        std::string pts = gw0d; if (!pts.empty() && (pts[0] == '+' || pts[0] == '-')) pts = pts.substr(1);
                        ln += " - " + pts + (above ? " pts above" : " pts below");
                    }
                    add(ln, C_INK, false);
                } else add("No pin today", C_MUTED, false);
            }
        }
    }
    if ((all || cfg.show == 2) && !W.empty()) {
        gapLine();
        std::string nowTag;
        for (size_t i = 0; i < W.size(); i++) if (now >= hm(W[i].from) && now < hm(W[i].to)) nowTag = W[i].tag;
        add(mkt + (nowTag.empty() ? " RTH now: normal" : nowTag == "ACTIVE" ? " RTH now: ACTIVE" : " RTH now: CHOP - wait"),
            nowTag == "ACTIVE" ? C_GREEN : nowTag == "CHOP" ? C_YELLOW : C_INK, true);
        for (size_t i = 0; i < W.size(); i++) {
            bool on = now >= hm(W[i].from) && now < hm(W[i].to);
            std::string ln = std::string(W[i].tag == "ACTIVE" ? "Active " : "Chop ") + range12(hm(W[i].from), hm(W[i].to));
            if (!W[i].pct.empty()) ln += " (" + W[i].pct + "% of RTH)";
            add(ln, W[i].tag == "ACTIVE" ? C_GREEN : C_YELLOW, on);
        }
    }
    // (1.6.0, Rassul 10:55 "menthor q has a expected move range and where spot is within it") MenthorQ's 1D Min - 1D Max, a bar with
    // where price sits, and how much of it the session has used
    if ((all || cfg.show == 4) && !emLo.empty()) {
        gapLine();
        add("Expected move " + emLo + " - " + emHi, C_INK, true);
        add("\x01" + emPct, C_INK, false);
        add("Spot " + emPx + " - " + emPct + "% up the range", C_INK, false);
        if (!emUsed.empty()) add("Day range used: " + emUsed + "% of the move", C_MUTED, false);
    }
    // (1.6.0, Rassul 11:01 "it had data from 4 or 5 am") stale data says so, in grey; IRT not recording this market, in amber
    {
        int age = dAge.empty() ? -1 : atoi(dAge.c_str());
        bool rec = dRec.empty() || dRec == "OK";
        if (age > 10 || !rec) {                                      // (1.6.1) the Reader rebuilds every 5 min: 6-9 is normal
            gapLine();
            std::string t = age > 10 ? "data " + std::to_string(age) + " min old" : "";
            if (!rec) t += std::string(t.empty() ? "" : " - ") + "IRT not recording " + mkt;
            add(t, rec ? C_GREY : C_AMBER, !rec);
        }
    }
    bw[1] = bw[2] = 0;
    if (!L.empty()) box(0, L, B, defX, defY); else bw[0] = 0;
    return RTX_OK;
}

int SessionInfo::mouse(RTX_EVENT* e)
{
    (void)e; return RTX_FAIL;            // (1.5.0) placed by the Position setting - no dragging
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionInfo *p = new SessionInfo();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Session Info: when today's 0DTE options expire, and the market's ACTIVE / CHOP windows in RTH. Click anywhere on a box and drag it.");
    p->setVersion("1.6.1");   // (1.6.1) below the chart title, data age from 10 min;   // (1.6.0) fixed 250 px width (scaled with the font), see-through, gamma / HVL, GW0 pin, expected move, news countdown, stale line;   // (1.5.0) 9-spot Position (default Top left), blank line between sections, no drag;   // (1.4.0) order NEWS / ACTIVE-CHOP / 0DTE, simple lines, Move right / down settings;   // (1.3.0) one panel (news + 0DTE + RTH), one drag;   // (1.2.0) each window's share of the RTH range;   // (1.1.0) the NEWS heading: calendar + FinancialJuice breaking news for the market;   // (1.0.2) drag from anywhere on a box; ACTIVE green, CHOP yellow, 12-hour times;   // (1.0.1) evening = the next session's expiry
    return p;
}
