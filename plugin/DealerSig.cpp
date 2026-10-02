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
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

static const char* DS_VERSION = "1.0.0";
static const COLOR S_GREEN = 0x0022C55E;
static const COLOR S_RED   = 0x00EF4444;

struct SIdx { int market, font, clock, gap; };
static SIdx SX;
struct SSet { int market = 0, font = 10, clock = 0, gap = 6; };

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
    setParameterVersion(1);
    setParameterDialogHeight(30);
    const short SL = kParmAppendSameLine;
    MARKER mb; memset(&mb, 0, sizeof(mb)); mb.number = kMarkerArrayUp;   mb.size = 2; mb.color = S_GREEN; mb.location = kMarkerBeneathLow;
    MARKER ms; memset(&ms, 0, sizeof(ms)); ms.number = kMarkerArrayDown; ms.size = 2; ms.color = S_RED;   ms.location = kMarkerAboveHigh;
    setOutputSignalParameter("Dealer BUY reason", &mb, OUTPUT_DISABLED);
    setOutputSignalParameter("Dealer SELL reason", &ms, OUTPUT_DISABLED);
    SX.market = getParameterCount(); setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    SX.font   = getParameterCount(); setIntegerParameter("Font size (pt)", 10, 0, SL);
    SX.clock  = getParameterCount(); setIntegerParameter("Clock offset (min)", 0, 0);
    SX.gap    = getParameterCount(); setIntegerParameter("Gap from the bar (px)", 6, 0, SL);
    setLabelParameter("HOW TO READ IT - example: GC 1 Oct, low 4,179.6 at 10:09", 380);
    setLabelParameter("  10:18 bar: green SC under it = the 10:20 grid showed", 380);
    setLabelParameter("  dealers buying 196 futures back (12.6% of the futures", 380);
    setLabelParameter("  traded) after selling 331 into the drop", 380);
    setLabelParameter("GREEN under a bar = a reason dealers BUY futures (long)", 380);
    setLabelParameter("RED above a bar = a reason dealers SELL futures (short)", 380);
    setLabelParameter("Each mark sits on the bar where the reason became known", 380);
    setLabelParameter("  (when MenthorQ's grid landed), once per reason", 380);
    setLabelParameter("SC = short cover: dealers trade back what they hedged", 380);
    setLabelParameter("  with the push (gamma)", 380);
    setLabelParameter("Van = IV moved past its noise and forced dealers (vanna)", 380);
    setLabelParameter("Chm = time decay forced dealers (charm)", 380);
    setLabelParameter("PutS / CallB = customers sold puts / bought calls near", 380);
    setLabelParameter("  price: dealers buy futures", 380);
    setLabelParameter("CallS / PutB = customers sold calls / bought puts: sell", 380);
    setLabelParameter("A mark = at least 1% of the futures traded (flow: 50+", 380);
    setLabelParameter("  contracts, 20%+ of the options near price)", 380);
    setLabelParameter("All modelled from MenthorQ open interest - LEARNING until", 380);
    setLabelParameter("  each reason is scored. The Dealer Read explains them.", 380);
    return RTX_OK;
}

void DealerSig::readSettings(SSet& S)
{
    S.market = getListIndex(SX.market); if (S.market < 0 || S.market > 7) S.market = 0;
    S.font = getIntegerValue(SX.font); if (S.font < 7 || S.font > 24) S.font = 10;
    S.clock = getIntegerValue(SX.clock); if (S.clock < -720 || S.clock > 720) S.clock = 0;
    S.gap = getIntegerValue(SX.gap); if (S.gap < 0 || S.gap > 60) S.gap = 6;
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
    for (size_t k = 0; k < D.sigs.size(); k++) {
        int b = barOf(D.sigs[k], (int)n, dt);
        if (b < from) continue;
        if (D.sigs[k].side == 'B') o1[b] = kSignalTrue; else o2[b] = kSignalTrue;
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
    load();
    drawn = 0;
    long n = getBarCount();
    if (n < 1 || D.sigs.empty()) { writeStatus(mkt.empty() ? "no market" : (D.sigs.empty() ? "no reasons yet" : "no bars")); return RTX_OK; }
    RTARRAY hi(barHigh), lo(barLow);
    RTARRAYI dt(barDateTime);
    RCT pane; pane.getPaneRect(false);
    int fs = cfg.font;
    FONT f; f.id = HELVETICA; f.size = (short)fs; f.style = BOLD; setFont(f);
    std::vector<int> stackB((size_t)n, 0), stackS((size_t)n, 0);
    for (size_t k = 0; k < D.sigs.size(); k++) {
        const dl::Data::Sig& g = D.sigs[k];
        int b = barOf(g, (int)n, dt);
        if (b < 0) continue;
        bool buy = g.side == 'B';
        PNT p; p.set(b, buy ? lo[b] : hi[b], kBarCenter);
        if (p.h < pane.left || p.h > pane.right) continue;
        int w = (int)getTextWidth(g.code.c_str(), -1);
        int lineH = fs + 4;
        int& st = buy ? stackB[(size_t)b] : stackS[(size_t)b];
        short top = buy ? (short)(p.v + cfg.gap + st * lineH) : (short)(p.v - cfg.gap - (st + 1) * lineH);
        st++;
        setTextColor(buy ? S_GREEN : S_RED);
        RCT rc; rc.set((short)(p.h - w / 2 - 2), top, (short)(p.h + w / 2 + 4), (short)(top + lineH));
        rc.drawText(g.code.c_str(), true, false);
        drawn++;
    }
    writeStatus("drawn");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerSig *p = new DealerSig();
    p->setArrayCount(2);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | ARRAY1_IS_SIGNAL | ARRAY2_IS_SIGNAL);
    p->setDescription("LRA Dealer Sig: where an options-market reason for a reversal becomes known. The guide is below the settings.");
    p->setVersion("1.0.0");
    return p;
}
