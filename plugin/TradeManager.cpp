/********************************************************************************
 *  lsTradeManager - the TOPSTEP TRADE MANAGER on each market's chart (Rassul 2026-10-06 21:16 "a topstep indicator in irt ...
 *  sending warnings, letting me know whether i should be in this trade or not, telling me about entry, exit executions ...
 *  oversight, guidance and mentorship as well as coaching"; mockup v1 approved 2026-10-07 08:58 "trade manager is approved, we
 *  will improve it").
 *
 *  One small box, three states:
 *    FLAT      READY / WAIT: the day (P&L, room to the stop, trades, losses in a row, MLL room), the next trade the Read is
 *              working (side + key level), a 4-line checklist (key level / Read / time window / news), the size for 1R and the
 *              trigger
 *    IN TRADE  side, size, entry, stop, target, open P&L in R with a stop -> entry -> 2R bar, HOLD / EXIT? from the Read, the
 *              next step (stop to break-even at +1R)
 *    STOPPED   red frame: why the day is over (the guard's alerts)
 *  Data (all written by the LRA bridge, read only when the file changed):
 *    %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Topstep.csv   the Topstep Guard (TopstepX userscript -> bridge): STATUS, PNL,
 *        TRADES, INROW, ROOM, RISK, POS|<mkt>|long/short|size|price|upl, ALERT|level|time|text
 *    LRA-Dealer-<MKT>.csv   PRICE, TURN (side + header: DEMAND / SUPPLY n of m, the level, T1), RDX (extreme, level), RD (trigger)
 *    LRA-Session-<MKT>.csv  WIN (ACTIVE / CHOP), CAL (calendar events)
 *  Click anywhere on the box and drag it; double-click = back to the settings spot. Settings: Market, Font, Move right / down.
 *  Never places, changes or cancels an order - it only reads files and draws.
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>

static const char* TM_VERSION = "1.0.0";
static const COLOR C_INK = 0x00E5E7EB, C_MUTED = 0x009CA3AF, C_GREY = 0x0064748B, C_BOXBG = 0x000B1220, C_BORDER = 0x00334155;
static const COLOR C_AMBER = 0x00F59E0B, C_RED = 0x00EF4444, C_GREEN = 0x0022C55E, C_DKRED = 0x007F1D1D, C_DKGRN = 0x00166534, C_TRACK = 0x001F2937;

struct TIdx { int market, font, moveX, moveY; };
static TIdx TX;
struct TSet { int market = 0, font = 8, moveX = 0, moveY = 0; };

// futures $ per 1.0 price move: the micro (what he trades) and its size cap (plan.json max_size)
static double microPV(const std::string& m) { return m == "ES" ? 5 : m == "NQ" ? 2 : m == "CL" ? 100 : m == "NG" ? 1000 : m == "GC" ? 10 : m == "HG" ? 2500 : m == "EU" ? 12500 : 0; }
static const char* microName(const std::string& m) { return m == "ES" ? "MES" : m == "NQ" ? "MNQ" : m == "CL" ? "MCL" : m == "NG" ? "MNG" : m == "GC" ? "MGC" : m == "HG" ? "MHG" : m == "EU" ? "M6E" : "?"; }
static int microCap(const std::string& m) { return m == "EU" ? 10 : 5; }
static double tickOf(const std::string& m) { return m == "ES" || m == "NQ" ? 0.25 : m == "CL" ? 0.01 : m == "NG" ? 0.001 : m == "GC" ? 0.1 : m == "HG" ? 0.0005 : m == "EU" ? 0.00005 : 0.01; }
static int decOf(const std::string& m) { return m == "GC" ? 1 : m == "HG" ? 4 : m == "EU" ? 5 : m == "NG" ? 3 : 2; }

static std::string fmtPx(const std::string& m, double v)
{
    char b[32]; snprintf(b, sizeof b, "%.*f", decOf(m), v);
    std::string s(b); size_t dot = s.find('.'); std::string ip = dot == std::string::npos ? s : s.substr(0, dot), fp = dot == std::string::npos ? "" : s.substr(dot);
    std::string o; int n = 0; bool neg = !ip.empty() && ip[0] == '-'; if (neg) ip = ip.substr(1);
    for (int i = (int)ip.size() - 1; i >= 0; i--) { o.insert(o.begin(), ip[i]); if (++n % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return (neg ? "-" : "") + o + fp;
}
static std::string money(double v, bool sign = true)
{
    char b[32]; double a = std::fabs(v); snprintf(b, sizeof b, "%.0f", a);
    std::string s(b), o; int n = 0;
    for (int i = (int)s.size() - 1; i >= 0; i--) { o.insert(o.begin(), s[i]); if (++n % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return std::string(v < 0 ? "-" : (sign ? "+" : "")) + "$" + o;
}
static double num(const std::string& s) { std::string t; for (char c : s) if (c != ',') t += c; return atof(t.c_str()); }
static std::vector<std::string> splitBar(const std::string& ln) { std::vector<std::string> c; std::stringstream ss(ln); std::string x; while (std::getline(ss, x, '|')) c.push_back(x); return c; }
static std::vector<std::string> readLines(const std::string& p)
{
    std::vector<std::string> L; std::ifstream f(p.c_str()); std::string ln;
    while (std::getline(f, ln)) { if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1); L.push_back(ln); }
    return L;
}
static int hmMin(const std::string& s) { return s.size() >= 5 && s[2] == ':' ? atoi(s.substr(0, 2).c_str()) * 60 + atoi(s.substr(3, 2).c_str()) : -1; }
static std::string t12(int m)
{
    char b[16]; int h = (m / 60) % 24; snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m % 60, h < 12 ? "AM" : "PM"); return b;
}

class TradeManager : public cppExtension {
public:
    TradeManager() : cppExtension() {}
    virtual int parmsLoad(void) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int parmsApply(void) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { if (ready()) readS(cfg); return RTX_OK; }
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);

    TSet cfg;
    std::string mkt, root;
    // the guard
    std::string status, alertsLvl[6], alertsT[6], alertsTx[6]; int nAlerts = 0;
    double dayPnl = 0, left = 0, stop = 600, risk = 150, room = -1; int trades = 0, maxTrades = 3, inRow = 0; bool haveGuard = false;
    bool inPos = false; std::string posSide; int posSize = 0; double posPx = 0, posUpl = 0;
    // the Read
    bool haveTurn = false, atLevel = false; char side = 0; std::string head, levelTxt, ctl; double ext = 0, lvl = 0, t1 = 0, px = 0;
    // the session
    struct Win { std::string tag; int a, b; }; std::vector<Win> W;
    struct Cal { int m; std::string imp, title; }; std::vector<Cal> C;
    std::map<std::string, long long> stamps;
    short bx = 0, by = 0, bw = 0, bh = 0; int posX = -1, posY = -1; std::string posMkt;
    bool dragging = false; short dDX = 0, dDY = 0;

    bool ready() { int i = getListIndex(TX.market); return i >= 0 && i <= 7; }
    void readS(TSet& S)
    {
        { int v = getListIndex(TX.market); if (v >= 0 && v <= 7) S.market = v; }
        { int v = getIntegerValue(TX.font); if (v >= 6 && v <= 20) S.font = v; }
        { int v = getIntegerValue(TX.moveX); if (v >= -3000 && v <= 3000) S.moveX = v; }
        { int v = getIntegerValue(TX.moveY); if (v >= -2000 && v <= 2000) S.moveY = v; }
    }
    bool changed(const std::string& p) { long long st = dl::fileStamp(p); bool c = stamps[p] != st; stamps[p] = st; return c; }
    void load();
    void loadPos(); void savePos();
    int nowMin() { time_t t = time(0); struct tm lt; localtime_s(&lt, &t); return lt.tm_hour * 60 + lt.tm_min; }
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
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(3);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    TX.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    TX.font   = pc++; setIntegerParameter("Font size (pt)", 8, NUMW, SL);
    TX.moveX  = pc++; setIntegerParameter("Move right px (minus = left)", 0, NUMW);
    TX.moveY  = pc++; setIntegerParameter("Move down px (minus = up)", 0, NUMW, SL);
    return RTX_OK;
}

void TradeManager::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::string dir = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\";
    std::string pG = dir + "LRA-Topstep.csv", pD = dir + "LRA-Dealer-" + mkt + ".csv", pS = dir + "LRA-Session-" + mkt + ".csv";
    static std::string lastMkt; bool mk = lastMkt != mkt; lastMkt = mkt;     // one object serves every chart: re-read on a market switch
    if (changed(pG) || mk) {
        haveGuard = false; inPos = false; nAlerts = 0; status.clear();
        std::vector<std::string> L = readLines(pG);
        std::vector<std::vector<std::string> > al;
        for (size_t i = 0; i < L.size(); i++) {
            std::vector<std::string> c = splitBar(L[i]); if (c.empty()) continue;
            if (c[0] == "STATUS" && c.size() >= 2) { status = c[1]; haveGuard = true; }
            else if (c[0] == "PNL" && c.size() >= 4) { dayPnl = num(c[1]); left = num(c[2]); stop = num(c[3]); }
            else if (c[0] == "TRADES" && c.size() >= 3) { trades = atoi(c[1].c_str()); maxTrades = atoi(c[2].c_str()); }
            else if (c[0] == "INROW" && c.size() >= 2) inRow = atoi(c[1].c_str());
            else if (c[0] == "ROOM" && c.size() >= 2) room = c[1].empty() ? -1 : num(c[1]);
            else if (c[0] == "RISK" && c.size() >= 2) { double r = num(c[1]); if (r > 0) risk = r; }
            else if (c[0] == "POS" && c.size() >= 6 && c[1] == mkt) { inPos = true; posSide = c[2]; posSize = atoi(c[3].c_str()); posPx = num(c[4]); posUpl = num(c[5]); }
            else if (c[0] == "ALERT" && c.size() >= 4) al.push_back(c);
        }
        for (size_t i = 0; i < al.size() && nAlerts < 6; i++) { alertsLvl[nAlerts] = al[i][1]; alertsT[nAlerts] = al[i][2]; alertsTx[nAlerts] = al[i][3]; nAlerts++; }
    }
    if (changed(pD) || mk) {
        haveTurn = atLevel = false; side = 0; head.clear(); levelTxt.clear(); ctl.clear(); ext = lvl = t1 = 0;
        std::vector<std::string> L = readLines(pD);
        for (size_t i = 0; i < L.size(); i++) {
            std::vector<std::string> c = splitBar(L[i]); if (c.empty()) continue;
            if (c[0] == "PRICE" && c.size() >= 2) px = num(c[1]);
            else if (c[0] == "TURN" && c.size() >= 3) { haveTurn = true; side = c[1].empty() ? 0 : c[1][0]; head = c[2]; }
            else if (c[0] == "RDX" && c.size() >= 4) { atLevel = true; ext = num(c[2]); lvl = num(c[3]); }
        }
        // who is in control: the header's first DEMAND / SUPPLY / BALANCED word and its count ("DEMAND? 3 of 5")
        const char* words[3] = {"DEMAND", "SUPPLY", "BALANCED"};
        for (int w = 0; w < 3 && ctl.empty(); w++) {
            size_t p = head.find(words[w]);
            if (p != std::string::npos) { size_t e = head.find(" - ", p); ctl = head.substr(p, e == std::string::npos ? std::string::npos : e - p); }
        }
        size_t a = head.find(" at ");                                         // "LONG at CR 4,165.0 - low 22:15 ..."
        if (a != std::string::npos) { size_t e = head.find(" - ", a); levelTxt = head.substr(a + 4, e == std::string::npos ? std::string::npos : e - a - 4); }
        size_t tp = head.find("T1 ");
        if (tp != std::string::npos) { size_t e = head.find(" ", tp + 3); t1 = num(head.substr(tp + 3, e == std::string::npos ? std::string::npos : e - tp - 3)); }
    }
    if (changed(pS) || mk) {
        W.clear(); C.clear();
        std::vector<std::string> L = readLines(pS);
        for (size_t i = 0; i < L.size(); i++) {
            std::vector<std::string> c = splitBar(L[i]); if (c.empty()) continue;
            if (c[0] == "WIN" && c.size() >= 4) { Win w; w.tag = c[1]; w.a = hmMin(c[2]); w.b = hmMin(c[3]); W.push_back(w); }
            else if (c[0] == "CAL" && c.size() >= 4 && c[1].size() == 5) { Cal k; k.m = hmMin(c[1]); k.imp = c[2]; k.title = c[3]; C.push_back(k); }
        }
    }
}

void TradeManager::loadPos()
{
    if (posMkt == mkt) return;
    posMkt = mkt; posX = posY = -1;
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::ifstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\TradeManager.pos-" + mkt + ".txt").c_str());
    if (f.is_open()) { char c; f >> posX >> c >> posY; if (!f) posX = posY = -1; }
}
void TradeManager::savePos()
{
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\TradeManager.pos-" + mkt + ".txt").c_str(), std::ios::trunc);
    if (f.is_open()) f << posX << "," << posY << "\n";
}

int TradeManager::draw(void)
{
    if (ready()) readS(cfg);
    load();
    if (mkt.empty()) return RTX_OK;
    loadPos();
    RCT pane; pane.getPaneRect(false);
    int fs = cfg.font, now = nowMin();
    short lh = (short)(fs * 1.75f + 0.5f), pad = (short)(fs * 0.7f + 0.5f);
    struct Ln { std::string a, b; COLOR ca, cb; bool bold; int bar; };   // a = a coloured lead word, b = the rest; bar 1 = the R bar
    std::vector<Ln> L;
    auto add = [&](const std::string& a, COLOR ca, const std::string& b, COLOR cb, bool bold) { Ln l; l.a = a; l.b = b; l.ca = ca; l.cb = cb; l.bold = bold; l.bar = 0; L.push_back(l); };
    char s[220];
    bool stopped = status == "stop";
    COLOR frameC = stopped ? C_RED : C_BORDER;
    // ---- the day line (all states)
    std::string dayLn = haveGuard ? "Day " + money(dayPnl) + "   To stop " + (left >= 0 ? money(left, false) : "NONE") + "   Trades " + std::to_string(trades) + "/" + std::to_string(maxTrades) +
                        "   In a row " + std::to_string(inRow) + (room >= 0 ? "   MLL room " + money(room, false) : "") : "Topstep Guard not connected (TopstepX tab + userscript)";
    double pv = microPV(mkt), tk = tickOf(mkt);
    if (stopped) {
        add(mkt + " TRADE MANAGER", C_INK, "   STOP FOR THE DAY", C_RED, true);
        add("", C_INK, dayLn, C_INK, false);
        add("", C_INK, "Your daily stop is hit. No more trades today.", C_INK, true);
        for (int i = nAlerts - 1, k = 0; i >= 0 && k < 3; i--, k++) add("WHY  ", C_RED, alertsTx[i], C_INK, false);
    } else if (inPos) {
        bool lng = posSide == "long";
        double stopPx = (atLevel && ((side == 'L') == lng)) ? (lng ? ext - tk : ext + tk) : 0;
        double dist = stopPx > 0 ? std::fabs(posPx - stopPx) : 0;
        double oneR = (dist > 0 && posSize > 0) ? dist * pv * posSize : risk;
        double rNow = oneR > 0 ? posUpl / oneR : 0;
        snprintf(s, sizeof s, "   %s  %+.1fR", money(posUpl).c_str(), rNow);
        add(mkt + (lng ? " LONG " : " SHORT ") + std::to_string(posSize) + " " + microName(mkt), C_INK, s, posUpl >= 0 ? C_GREEN : C_RED, true);
        std::string ln = "Entry " + fmtPx(mkt, posPx);
        if (stopPx > 0) ln += "   stop " + fmtPx(mkt, stopPx) + " (-1R " + money(oneR, false) + ")";
        if (t1 > 0) ln += "   target " + fmtPx(mkt, t1);
        add("", C_INK, ln, C_INK, false);
        if (stopPx > 0) { Ln b; b.bar = 1; b.a = ""; b.b = ""; b.ca = b.cb = C_INK; b.bold = false; L.push_back(b); }
        bool readSame = (side == 'L') == lng && ctl.find(lng ? "DEMAND" : "SUPPLY") != std::string::npos;
        bool readFlip = ctl.find(lng ? "SUPPLY" : "DEMAND") != std::string::npos;
        if (readSame) add("HOLD  ", C_GREEN, "Read still " + ctl, C_INK, false);
        else if (readFlip) add("EXIT?  ", C_RED, "The Read flipped: " + ctl, C_INK, false);
        else add("WATCH  ", C_AMBER, "Read: " + (ctl.empty() ? std::string("none") : ctl), C_INK, false);
        if (stopPx > 0) add("NEXT  ", C_INK, "At +1R (" + fmtPx(mkt, lng ? posPx + dist : posPx - dist) + "): stop to " + fmtPx(mkt, posPx) + ", break-even", C_INK, false);
        add("NO  ", C_AMBER, std::string("Adding to this trade (cap ") + std::to_string(microCap(mkt)) + " " + microName(mkt) + ")", C_INK, false);
    } else {
        // ---- FLAT: the checklist for the next trade
        bool lvlOk = atLevel && haveTurn;
        bool readOk = side == 'L' ? ctl.find("DEMAND") != std::string::npos : side == 'S' ? ctl.find("SUPPLY") != std::string::npos : false;
        bool readNo = side == 'L' ? ctl.find("SUPPLY") != std::string::npos : side == 'S' ? ctl.find("DEMAND") != std::string::npos : false;
        std::string tag; for (size_t i = 0; i < W.size(); i++) if (now >= W[i].a && now < W[i].b) tag = W[i].tag;
        const Cal* nw = nullptr; for (size_t i = 0; i < C.size(); i++) if (C[i].imp == "High" && C[i].m - now <= 30 && now - C[i].m <= 15) { nw = &C[i]; break; }
        bool ready_ = lvlOk && readOk && tag != "CHOP" && !nw && (trades < maxTrades) && status != "warn";
        add(mkt + " TRADE MANAGER", C_INK, ready_ ? "   READY" : "   WAIT", ready_ ? C_GREEN : C_AMBER, true);
        add("", C_INK, dayLn, C_INK, false);
        if (lvlOk) add("", C_INK, std::string("Next: ") + (side == 'L' ? "LONG" : "SHORT") + " at " + levelTxt, C_INK, true);
        else add("", C_INK, "Next: no key level in play - wait for one", C_MUTED, true);
        add(lvlOk ? "YES  " : "NO  ", lvlOk ? C_GREEN : C_RED, lvlOk ? "Key level: " + levelTxt : "Key level: none within reach", C_INK, false);
        add(readOk ? "YES  " : readNo ? "NO  " : "WAIT  ", readOk ? C_GREEN : readNo ? C_RED : C_AMBER, "Read: " + (ctl.empty() ? std::string("none") : ctl), C_INK, false);
        if (tag == "ACTIVE") add("YES  ", C_GREEN, "Time: Active window", C_INK, false);
        else if (tag == "CHOP") add("NO  ", C_RED, "Time: Chop window - sit out", C_INK, false);
        else add("OK  ", C_INK, "Time: normal", C_INK, false);
        if (nw) add("WAIT  ", C_AMBER, "News: " + nw->title + " at " + t12(nw->m), C_INK, false);
        else add("YES  ", C_GREEN, "News: no high-impact release within 30 min", C_INK, false);
        if (trades >= maxTrades) add("NO  ", C_RED, "Trades: " + std::to_string(trades) + " of " + std::to_string(maxTrades) + " used today", C_INK, false);
        if (lvlOk && pv > 0) {
            double stopPx = side == 'L' ? ext - tk : ext + tk;
            double entry = px > 0 ? px : lvl, dist = std::fabs(entry - stopPx);
            int qty = dist > 0 ? (int)std::floor(risk / (dist * pv)) : 0; if (qty > microCap(mkt)) qty = microCap(mkt);
            if (qty >= 1) {
                std::string ln = "Size " + std::to_string(qty) + " " + microName(mkt) + "   stop " + fmtPx(mkt, stopPx) + "   1R = " + money(dist * pv * qty, false) + (t1 > 0 ? "   T1 " + fmtPx(mkt, t1) : "");
                add("", C_INK, ln, C_INK, false);
            } else add("NO  ", C_RED, "Stop too wide for 1R " + money(risk, false) + " with 1 " + microName(mkt), C_INK, false);
            add("", C_INK, std::string("Trigger: a 3-min close back ") + (side == 'L' ? "above " : "below ") + fmtPx(mkt, lvl) + ". No close, no trade.", C_INK, true);
        }
    }
    // ---- size + place the box
    int w = 0;
    for (size_t i = 0; i < L.size(); i++) {
        int x = textW(L[i].a.c_str(), fs, true) + textW(L[i].b.c_str(), fs, L[i].bold);
        if (x > w) w = x;
    }
    if (w < fs * 30) w = fs * 30;
    short W_ = (short)(w + 2 * pad + 6), H_ = (short)(L.size() * lh + pad);
    short defX = (short)(pane.right - W_ - 340 + cfg.moveX), defY = (short)(pane.top + 8 + cfg.moveY);
    short x0 = posX >= 0 ? (short)(pane.left + posX) : defX, y0 = posY >= 0 ? (short)(pane.top + posY) : defY;
    if (x0 + W_ > pane.right) x0 = (short)(pane.right - W_); if (x0 < pane.left) x0 = pane.left;
    if (y0 + H_ > pane.bottom) y0 = (short)(pane.bottom - H_); if (y0 < pane.top) y0 = pane.top;
    fill(x0, y0, (short)(x0 + W_), (short)(y0 + H_), C_BOXBG);
    frame(x0, y0, (short)(x0 + W_), (short)(y0 + H_), frameC);
    if (stopped) frame((short)(x0 + 1), (short)(y0 + 1), (short)(x0 + W_ - 1), (short)(y0 + H_ - 1), frameC);
    for (size_t i = 0; i < L.size(); i++) {
        short y = (short)(y0 + pad / 2 + lh * i + lh / 2), x = (short)(x0 + pad);
        if (L[i].bar) {                                                      // stop -> entry -> 1R -> 2R, the white mark = now
            bool lng = posSide == "long";
            double stopPx = lng ? ext - tickOf(mkt) : ext + tickOf(mkt), dist = std::fabs(posPx - stopPx);
            double cur = dist > 0 ? (lng ? (px - stopPx) : (stopPx - px)) / (3 * dist) : 0; if (cur < 0) cur = 0; if (cur > 1) cur = 1;
            short bl = x, br = (short)(x0 + W_ - pad), bt = (short)(y - 3), bb = (short)(y + 3);
            fill(bl, bt, br, bb, C_TRACK);
            short xe = (short)(bl + (br - bl) / 3);
            fill(bl, bt, xe, bb, C_DKRED); fill(xe, bt, br, bb, C_DKGRN);
            short xn = (short)(bl + (br - bl) * cur); fill((short)(xn - 1), (short)(bt - 3), (short)(xn + 2), (short)(bb + 3), C_INK);
            continue;
        }
        if (!L[i].a.empty()) { text(x, y, L[i].a.c_str(), L[i].ca, fs, true); x = (short)(x + textW(L[i].a.c_str(), fs, true)); }
        text(x, y, L[i].b.c_str(), L[i].cb, fs, L[i].bold);
    }
    bx = x0; by = y0; bw = W_; bh = H_;
    return RTX_OK;
}

int TradeManager::mouse(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    PNT m; if (m.getMouse(e) != RTX_OK) return RTX_FAIL;
    RCT pane; pane.getPaneRect(false);
    bool on = bw > 0 && m.h >= bx && m.h <= bx + bw && m.v >= by && m.v <= by + bh;
    if (e->type == E_MOUSE_DBL && on) { posX = posY = -1; savePos(); dragging = false; trackMouseDrag(false); invalidateChart(); return RTX_OK; }
    if (e->type == E_MOUSE_CLICK && on) { dragging = true; dDX = (short)(m.h - bx); dDY = (short)(m.v - by); trackMouseDrag(true); return RTX_OK; }
    if (e->type == E_MOUSE_MOVE && dragging) {
        int x = m.h - dDX, y = m.v - dDY;
        if (x + bw > pane.right) x = pane.right - bw; if (x < pane.left) x = pane.left;
        if (y + bh > pane.bottom) y = pane.bottom - bh; if (y < pane.top) y = pane.top;
        posX = x - pane.left; posY = y - pane.top; invalidateChart(); return RTX_OK;
    }
    if (e->type == E_MOUSE_UP && dragging) { dragging = false; trackMouseDrag(false); savePos(); invalidateChart(); return RTX_OK; }
    return RTX_FAIL;
}

extern "C" cppExtension *CreateExtension(void)
{
    TradeManager *p = new TradeManager();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);
    p->setDescription("LRA Trade Manager: your Topstep day, the checklist for the next trade, and how to manage the open one. Click and drag to move.");
    p->setVersion(TM_VERSION);
    return p;
}
