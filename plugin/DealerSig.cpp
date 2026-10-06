/********************************************************************************
 *  DealerSig.cpp  --  Investor/RT RTX extension  lsDealerSig  (v1.0.0, 2026-10-02)
 *
 *  THE DEALER SIG: marks on the price bars where an options-market reason for a reversal becomes known (layout A, Rassul
 *  2026-10-02: one mark per reason, on the bar where it becomes known). Works with lsDealerRead - the Read's DEALER REASONS
 *  box explains every mark (forced futures by cause, gamma strength). Options market only, no tape (Rassul: "the dealer
 *  read is getting info from the options market - you dont need to analyze the tape yet").
 *
 *  Show text = the reason's code, green UNDER the bar when it makes dealers BUY futures (a long reason), red ABOVE the bar
 *  when it makes them SELL (a short reason):
 *     SC     short cover - dealers who hedged with the push now trade back as price turns (gamma)
 *     Van    IV moved beyond its noise and forced dealers to trade (vanna)
 *     Chm    time decay forced dealers to trade (charm)
 *     PutS / CallB   customers sold puts / bought calls near price - dealers buy futures
 *     CallS / PutB   customers sold calls / bought puts near price - dealers sell futures
 *  (1.1.0) plus the Read's stage reasons: Exh (push-side options ran out), Lvl (MenthorQ levels held at the extreme), IV,
 *     0D (0DTE fear leaving), PT? (profit taking - guess), CallB? / PutB? (new bets - guess); a guess has a dashed frame
 *  (1.5.0, 2026-10-02 20:58, Rassul) the new letters (Ex, Dm/Sp, Tr, Sh, Pt, Np, Gm, V+/V-, Pn, Px, 0D, F+/F-, G+/G-), at most the
 *     top two on a bar (ranked by the engine), no numbers, no dashed frames - the Dealer Read explains each one.
 *  (1.4.0, 2026-10-02 19:15, Rassul: "so many signals after the 2 bars near the low"; "near the lows it should look for
 *  reversal up signals and when it goes to a gamma value higher, reversal down"; "get rid of the how to section"): only the
 *  Turn's reasons, and only on the turn's bars (the low / high bar and 2 bars either side; a reason MenthorQ showed later sits
 *  on the turn's last bar) - green under a low, red above a high; the forced ledger's SC / Van are LSIG rows, drawn only with
 *  "Dealer ledger marks: On" (smaller, lighter); labels never overlap (stacked away from the bar); the value is on the mark;
 *  the settings guide is gone.
 *  (1.3.0, 2026-10-02 evening) the marks are now the Turn's reasons (file 1.8): Trap, FGF, Pin, PinX, Cush, NB? join the codes;
 *  nothing else changed - any code in the file is drawn.
 *  (1.2.0) and the tape where it shows demand / supply from the gex level: Dem / Sup (the turn bars' volume + range vs the
 *     same-time normal), Abs (heavy push volume absorbed), Exh from the tape (quiet bars, last push got nowhere)
 *  Every mark is >= 1% of the futures traded in its window (flow marks: >= 50 contracts and >= 20% of the options traded
 *  within 1 EM). All MODELLED from MenthorQ's open interest and scored nightly before any is trusted.
 *  The same code is used whatever the source.
 *
 *  Also two signal outputs (off by default, for Signal Markers / scans / alerts): 1 = a dealer BUY reason on the bar,
 *  2 = a dealer SELL reason.
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Dealer-<MKT>.csv rows SIG|<yyyy-mm-dd HH:MM:SS CT>|<code>|B|S|<v>|<text>
 *  (LRA analytics/lra/forced.py via dealer_irt.py, file 1.6); grammar in DealerLogic.h. Never black lines: the chart is black.
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
#include <ctime>

static const char* DS_VERSION = "1.5.2";   // 1.5.2 (2026-10-05): per-chart settings; a reason timed after the last bar is not drawn on it
static const COLOR S_GREEN = 0x0022C55E;
static const COLOR S_RED   = 0x00EF4444;

struct SIdx { int market, font, clock, gap, ledger; };
static SIdx SX;
struct SSet { int market = 0, font = 10, clock = 0, gap = 6; bool ledger = false; };

class DealerSig : public cppExtension {
public:
    DealerSig() : cppExtension() {}
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    SSet cfg;
    dl::Data D;
    std::string mkt, root;
    long long loadedStamp = -2; std::string loadedPath;
    int drawn = 0;

    bool dialogReady() { int i = getListIndex(SX.market); return i >= 0 && i <= 7; }
    void readSettings(SSet& S);
    void load();
    int barOf(const dl::Data::Sig& g, int n, RTARRAYI& dt);
    void fillSignals(int from);
    void writeStatus(const char* what);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }
int cppExtension::calc(int iStartBar) { static_cast<DealerSig*>(this)->fillSignals(iStartBar); return RTX_OK; }

int DealerSig::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerSig::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerSig::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(2);           // (1.5.0) "Clock offset" removed
    setParameterDialogHeight(6);    // (1.4.0) controls only - the how-to text is gone
    const short SL = kParmAppendSameLine;
    MARKER mb; memset(&mb, 0, sizeof(mb)); mb.number = kMarkerArrayUp;   mb.size = 2; mb.color = S_GREEN; mb.location = kMarkerBeneathLow;
    MARKER ms; memset(&ms, 0, sizeof(ms)); ms.number = kMarkerArrayDown; ms.size = 2; ms.color = S_RED;   ms.location = kMarkerAboveHigh;
    setOutputSignalParameter("Dealer BUY reason", &mb, OUTPUT_DISABLED);
    setOutputSignalParameter("Dealer SELL reason", &ms, OUTPUT_DISABLED);
    SX.market = getParameterCount(); setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    SX.font   = getParameterCount(); setIntegerParameter("Font size (pt)", 10, NUMW, SL);
    // (1.5.0, Rassul 21:19 "why do you even have that option?") no clock offset: the dealer files and his charts are both Central
    SX.clock  = -1;
    SX.gap    = getParameterCount(); setIntegerParameter("Gap from the bar (px)", 6, NUMW);
    SX.ledger = getParameterCount(); setListParameter("Dealer ledger marks", 0, "Off;On");      // (1.4.0) the forced ledger's SC / Van
    return RTX_OK;
}

void DealerSig::readSettings(SSet& S)
{
    S.market = getListIndex(SX.market); if (S.market < 0 || S.market > 7) S.market = 0;
    S.font = getIntegerValue(SX.font); if (S.font < 7 || S.font > 24) S.font = 10;
    S.clock = 0;
    S.gap = getIntegerValue(SX.gap); if (S.gap < 0 || S.gap > 60) S.gap = 6;
    S.ledger = getListIndex(SX.ledger) == 1;
}

void DealerSig::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) { D = dl::Data(); loadedPath.clear(); return; }
    const char* up = getenv("USERPROFILE"); if (!up) { D = dl::Data(); return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Dealer-" + mkt + ".csv";
    long long st = dl::fileStamp(path);
    if (st >= 0 && st == loadedStamp && path == loadedPath) return;      // unchanged: keep what is parsed
    D = dl::Data();
    std::ifstream f(path.c_str()); if (!f.is_open()) { loadedPath.clear(); return; }
    std::stringstream ss; ss << f.rdbuf();
    D = dl::parseText(ss.str());
    loadedStamp = st; loadedPath = path;
}

// the bar (chart local time + clock offset) whose span holds the reason's time; -1 = not on the chart
int DealerSig::barOf(const dl::Data::Sig& g, int n, RTARRAYI& dt)
{
    int from = n - 3000; if (from < 0) from = 0;
    for (int i = n - 1; i >= from; i--) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        double so0 = t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec - cfg.clock * 60.0;
        double so1 = 1e9;
        if (i + 1 < n) {
            struct tm u; memset(&u, 0, sizeof(u)); getLocaltime((RTDATE)dt[i + 1], &u);
            if (u.tm_year == t.tm_year && u.tm_mon == t.tm_mon && u.tm_mday == t.tm_mday) so1 = u.tm_hour * 3600.0 + u.tm_min * 60.0 + u.tm_sec - cfg.clock * 60.0;
        } else {
            // (1.5.2, Rassul 2026-10-05 "why is it showing dm ... even when price is going down") the newest bar only holds a reason
            // timed within 15 min of its stamp - a reason stamped later (yesterday evening's 23:33 Dm written with today's date)
            // used to fall onto the live bar
            so1 = so0 + 15 * 60.0;
        }
        if (dl::sigInBar(g, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, so0, so1)) return i;
        if (t.tm_year + 1900 < g.y || (t.tm_year + 1900 == g.y && (t.tm_mon + 1 < g.mo || (t.tm_mon + 1 == g.mo && t.tm_mday < g.d)))) break;
    }
    return -1;
}

void DealerSig::fillSignals(int from)
{
    load();
    long n = getBarCount(); if (n < 1) return;
    RTARRAYI o1(iOut1), o2(iOut2);
    RTARRAYI dt(barDateTime);
    if (from < 0) from = 0;
    for (int i = from; i < (int)n; i++) { o1[i] = 0; o2[i] = 0; }
    std::vector<const dl::Data::Sig*> all;
    for (size_t k = 0; k < D.sigs.size(); k++) all.push_back(&D.sigs[k]);
    if (cfg.ledger) for (size_t k = 0; k < D.lsigs.size(); k++) all.push_back(&D.lsigs[k]);
    for (size_t k = 0; k < all.size(); k++) {
        int b = barOf(*all[k], (int)n, dt);
        if (b < from) continue;
        if (all[k]->side == 'B') o1[b] = kSignalTrue; else o2[b] = kSignalTrue;
    }
}

void DealerSig::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerSig.status.txt";
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << "VERSION," << DS_VERSION << "\nROOT," << root << "\nMARKET," << mkt << "\nSIGS," << D.sigs.size() << "\nDRAWN," << drawn << "\nSTATE," << what << "\n";
}

int DealerSig::draw(void)
{
    // (2026-10-05) IRT runs ONE object of this DLL for every chart: read THIS chart's settings on every draw (the HG chart's settings
    // leaked into the GC chart - status files showed MARKET HG while drawing GC)
    if (dialogReady()) readSettings(cfg);
    load();
    drawn = 0;
    long n = getBarCount();
    // (1.4.0, Rassul 2026-10-02 19:09-19:11) the Turn's reasons only (SIG, timed inside the turn: the low / high bar and 2 bars
    // either side - green under the bars at a low, red above them at a high); the forced ledger's SC / Van (LSIG) only when
    // "Dealer ledger marks" is On, drawn smaller. Every label is placed where it touches no other one (dl::placeFree).
    std::vector<std::pair<const dl::Data::Sig*, bool> > all;           // (mark, is ledger)
    for (size_t k = 0; k < D.sigs.size(); k++) all.push_back(std::make_pair(&D.sigs[k], false));
    if (cfg.ledger) for (size_t k = 0; k < D.lsigs.size(); k++) all.push_back(std::make_pair(&D.lsigs[k], true));
    if (n < 1 || all.empty()) { writeStatus(mkt.empty() ? "no market" : (all.empty() ? "no reasons yet" : "no bars")); return RTX_OK; }
    RTARRAY hi(barHigh), lo(barLow);
    RTARRAYI dt(barDateTime);
    RCT pane; pane.getPaneRect(false);
    std::vector<dl::Box> used;
    std::map<int, int> perBar;
    for (int pass = 0; pass < 2; pass++) {                            // the Turn's reasons first: they keep the spots next to the bar
        for (size_t k = 0; k < all.size(); k++) {
            if ((pass == 0) == all[k].second) continue;
            const dl::Data::Sig& g = *all[k].first;
            bool led = all[k].second;
            int b = barOf(g, (int)n, dt);
            if (b < 0) continue;
            bool buy = g.side == 'B';
            // (1.6.0, Rassul 2026-10-06 13:52 "options signals below the bar, fp signals above the bar ... add o to the signal like
            // oSh") a code starting with 'o' (and the dealer ledger) is an OPTIONS reason: under the bar; the rest is FOOTPRINT: over
            // the bar. The colour still says the side (green = long, red = short).
            bool below = led || (!g.code.empty() && g.code[0] == 'o');
            if (!led) { int& cnt = perBar[b * 2 + (below ? 1 : 0)]; if (cnt >= 2) continue; cnt++; }   // at most two per side of a bar
            PNT p; p.set(b, below ? lo[b] : hi[b], kBarCenter);
            if (p.h < pane.left || p.h > pane.right) continue;
            int fs = led ? (cfg.font > 8 ? cfg.font - 2 : cfg.font) : cfg.font;
            FONT f; f.id = HELVETICA; f.size = (short)fs; f.style = led ? PLAIN : BOLD; setFont(f);
            std::string txt = g.code;
            // (1.5.0, Rassul 2026-10-02 20:58 "dont put any numbers or dashed borders either .. the read can elaborate") the letter only
            while (!txt.empty() && txt[txt.size() - 1] == '?') txt.erase(txt.size() - 1);
            int w = (int)getTextWidth(txt.c_str(), -1), lineH = fs + 4;
            dl::Box x; x.l = p.h - w / 2 - 3; x.r = p.h + w / 2 + 4;
            x.t = below ? p.v + cfg.gap : p.v - cfg.gap - lineH; x.b = x.t + lineH;
            x = dl::placeFree(used, x, lineH, below);
            COLOR c = buy ? S_GREEN : S_RED;
            if (led) c = buy ? 0x0086EFAC : 0x00FCA5A5;                 // the ledger: lighter, smaller, not bold
            setTextColor(c);
            RCT rc; rc.set((short)x.l, (short)x.t, (short)x.r, (short)x.b);
            rc.drawText(txt.c_str(), true, false);
            drawn++;
        }
    }
    writeStatus("drawn");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerSig *p = new DealerSig();
    p->setArrayCount(2);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | ARRAY1_IS_SIGNAL | ARRAY2_IS_SIGNAL);
    p->setDescription("LRA Dealer Sig: the Turn's reversal reasons on the turn's bars - green under a low, red above a high.");
    p->setVersion("1.5.2");
    return p;
}
