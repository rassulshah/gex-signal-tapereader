// WavePOC_test_support/irtsdk.h - the mock Investor/RT SDK for test_wavepoc_host.cpp (lsWavePOC 1.0.0, 2026-10-28).
// One mock::Host = one indicator on one chart: bars, volume at price, the indicator's own parameter set (getParameters), the pane
// geometry and every drawing call recorded (fills, boxes, ovals, lines, texts) with the pen / colour in force.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <string>
#include <vector>

typedef unsigned long COLOR;
typedef unsigned long RTDATE;
typedef unsigned short RTBOOL;
typedef int RTX_RESULT;
static const RTX_RESULT RTX_OK = 0;
static const RTX_RESULT RTX_FAIL = -1;
static const COLOR COLOR_BLACK = 0;
enum PEN_STYLE { P_SOLID, P_DOT, P_DASH };
enum BRUSH_STYLE { PAT_HOLLOW, PAT_SOLID };
enum DRAWTYPE { DRAW_OPAQUE, DRAW_TRANSLUCENT };
enum RTX_REASON { REASON_DRAW = 5, REASON_PREDRAW = 7 };
enum { NO_UI = 1 << 12, POST_DRAWING = 1 << 18, OVERLAY = 1 << 19, PRE_DRAWING = 1 << 21, INSTRUMENT_SCALE = 1 << 30 };
static const unsigned long VAP_REQUIRED = 1UL << 31;
enum fARRAY { barOpen, barHigh, barLow, barClose, fEmptyArray };
enum iARRAY { barDateTime };
enum pARRAY { barVolumeProfile };
enum SYMBOL_PROPERTY_ID { SYM_TICKINCR = 171 };
enum BAR_POSITION { kBarLeft, kBarCenter, kBarRight };
enum FONT_ID { HELVETICA = 4 };
enum FONT_STYLE { PLAIN, BOLD };
static const short kParmAppendSameLine = 1;
class VOLPROFILE { public: float price; long long buyVolume, sellVolume, totalVolume; };
class BARSTATISTICS { public: int prices; long long buyVolume, sellVolume, totalVolume; };

typedef struct DEPTH_ORDER { RTDATE timestamp; float price; float previousPrice; float bid, ask; unsigned long bidsize, asksize; unsigned long size; int orderID; int aggressorOrderID; unsigned char action; unsigned char buySell; } DEPTH_ORDER;
namespace mock {
struct Param { int type = 0; bool b = false; int i = 0; std::string name; };   // 1 bool, 2 int, 3 other
inline std::vector<Param> declared;
struct Prim { char k; int x0, y0, x1, y1; COLOR col; int w; std::string text; bool bold; };   // F fill, B box, O oval, L line, T text
struct Host {
    std::string root = "EP", sym = "EPZ26"; int spb = 180; double tick = 0.25;
    std::vector<float> f[4]; std::vector<RTDATE> t;
    std::vector<std::vector<std::pair<float, long long> > > vap;   // per bar (price, volume)
    std::vector<int> vapMissing;                                   // bars that read "no VAP"
    void* userData = nullptr;
    int parmsMode = 0;   // 0 = hands the saved set, 1 = refuses, 2 = blank (all zero)
    int revBars = 3; bool zigzag = false; bool cobLegs = true;   // (1.3.0) row 3
    std::map<std::string, std::vector<float> > cust;   // (1.3.0) RTL custom indicators by name (getCustomIndicator)
    std::vector<int> saved; std::string savedTmpl;     // (1.3.0) saveParameters
    bool drawing = false; int customCalls = 0, customInDraw = 0;   // (1.3.1) RTL reads, and those made inside draw() (IRT crash, 1.3.0)
    bool inParmsInit = false; int parmsReadInInit = 0;             // (1.3.1) getParameters called inside parmsInit
    int paneL = 0, paneT = 0, paneR = 1600, paneB = 800, rightMargin = 300;
    RTX_REASON reason = REASON_DRAW;
    bool dialog = false;   // the settings window copy (no bar size)
    std::vector<Prim> drawn; COLOR pen = 0; int penW = 1; COLOR text = 0; bool bold = false; int curH = 0, curV = 0;
    double lo = 0, hi = 0;   // price scale of the pane
    std::vector<DEPTH_ORDER> mbo; size_t mboAt = 0;   // (1.1.0) the order-by-order queue this chart's feed hands out
    int first = 0;           // first visible bar
    std::string chartLabel;  // (1.3.2) getChartLabel
};
extern Host* current;
}

class RTARRAY { public: explicit RTARRAY(fARRAY a) : v_(a == fEmptyArray ? nullptr : &mock::current->f[(int)a]), count(v_ ? (long)v_->size() : 0) {} float operator[](int n) const { return (*v_)[(size_t)n]; }
    RTX_RESULT getCustomIndicator(char* name, int = 1, RTBOOL = false, const char* = nullptr, int = 0) { ++mock::current->customCalls; if (mock::current->drawing) ++mock::current->customInDraw; auto it = mock::current->cust.find(name); if (it == mock::current->cust.end()) return RTX_FAIL; v_ = &it->second; count = (long)v_->size(); return RTX_OK; }
private: const std::vector<float>* v_; public: long count; };
class RTARRAYI { public: explicit RTARRAYI(iARRAY) : count((long)mock::current->t.size()) {} RTDATE operator[](int n) const { return mock::current->t[(size_t)n]; } long count; };
class RTARRAYP {
public:
    explicit RTARRAYP(pARRAY) : count((long)mock::current->vap.size()) {}
    long count;
    RTX_RESULT getBarStatistics(int bar, BARSTATISTICS& s) {
        if (bar < 0 || bar >= count) return RTX_FAIL;
        if (std::find(mock::current->vapMissing.begin(), mock::current->vapMissing.end(), bar) != mock::current->vapMissing.end()) return RTX_FAIL;
        s.prices = (int)mock::current->vap[(size_t)bar].size(); s.totalVolume = 0; for (auto& r : mock::current->vap[(size_t)bar]) s.totalVolume += r.second; return RTX_OK;
    }
    RTX_RESULT getVolumeProfile(int bar, int i, VOLPROFILE& v) {
        if (bar < 0 || bar >= count || i < 0 || i >= (int)mock::current->vap[(size_t)bar].size()) return RTX_FAIL;
        v.price = mock::current->vap[(size_t)bar][(size_t)i].first; v.totalVolume = mock::current->vap[(size_t)bar][(size_t)i].second; v.buyVolume = v.totalVolume / 2; v.sellVolume = v.totalVolume - v.buyVolume; return RTX_OK;
    }
};
namespace mock { inline int dboAlive = 0, dboMade = 0; }
class DEPTH_BY_ORDER {
public:
    explicit DEPTH_BY_ORDER(const char* = nullptr) { ++mock::dboAlive; ++mock::dboMade; host = mock::current; std::memset(&dbo, 0, sizeof dbo); }
    ~DEPTH_BY_ORDER() { --mock::dboAlive; }
    char symbol[24] = { 0 };
    DEPTH_ORDER dbo;
    RTX_RESULT getNext(void) { mock::Host* H = mock::current; if (!H || H->mboAt >= H->mbo.size()) return RTX_FAIL; dbo = H->mbo[H->mboAt++]; return RTX_OK; }
private:
    mock::Host* host;
};
class FONT { public: FONT_ID id = HELVETICA; short size = 12; FONT_STYLE style = PLAIN; };
class CBRUSH { public: CBRUSH(COLOR = 0, BRUSH_STYLE = PAT_SOLID) {} void set() {} };
inline int mockX(int bar) { const double bw = 10; return (int)std::lround(mock::current->paneL + (bar - mock::current->first + 0.5) * bw); }
inline int mockY(double px) { mock::Host& H = *mock::current; return (int)std::lround(H.paneB - 10 - (px - H.lo) / (H.hi - H.lo) * (H.paneB - H.paneT - 20)); }
class PNT {
public:
    short v = 0, h = 0;
    RTX_RESULT set(int bar, float price, BAR_POSITION = kBarLeft) { h = (short)mockX(bar); v = (short)mockY(price); return RTX_OK; }
    void setDrawPosition() { mock::current->curH = h; mock::current->curV = v; }
    void drawLineTo() { mock::Host& H = *mock::current; H.drawn.push_back({ 'L', H.curH, H.curV, h, v, H.pen, H.penW, "", false }); H.curH = h; H.curV = v; }
};
class RCT {
public:
    short top = 0, left = 0, bottom = 0, right = 0;
    void getPaneRect(RTBOOL = false) { mock::Host& H = *mock::current; left = (short)H.paneL; top = (short)H.paneT; right = (short)H.paneR; bottom = (short)H.paneB; }
    void getScaleRect(void) { mock::Host& H = *mock::current; left = (short)(H.paneR + 1); top = (short)H.paneT; right = (short)(H.paneR + 60); bottom = (short)H.paneB; }   // (1.3.2) the price scale, right of the pane
    void set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
    void draw(short w = 0, COLOR border = 0, COLOR fill = 0, DRAWTYPE = DRAW_OPAQUE, BRUSH_STYLE bs = PAT_SOLID) { mock::current->drawn.push_back({ bs == PAT_HOLLOW ? 'B' : 'F', left, top, right, bottom, bs == PAT_HOLLOW ? border : fill, w, "", false }); }
    void drawOval(DRAWTYPE = DRAW_OPAQUE) { mock::Host& H = *mock::current; H.drawn.push_back({ 'O', left, top, right, bottom, H.pen, H.penW, "", false }); }
    void drawText(const char* s, RTBOOL = true, RTBOOL = false) { mock::Host& H = *mock::current; H.drawn.push_back({ 'T', left, top, right, bottom, H.text, 0, s, H.bold }); }
};

class cppExtension {
public:
    typedef ::DEPTH_BY_ORDER DEPTH_BY_ORDER;   // nested in the real SDK
    cppExtension() {}
    virtual ~cppExtension() {}
    virtual int init(void); virtual int setup(void); virtual int calc(int); virtual int done(void); virtual int destroy(void);
    virtual int draw(void) { return RTX_FAIL; }
    virtual int parmsInit(unsigned) { return RTX_FAIL; }
    virtual int parmsLoad(void) { return RTX_FAIL; }
    virtual int parmsApply(void) { return RTX_FAIL; }
    virtual int parmsUpdt(unsigned) { return RTX_OK; }
    long getBarCount(void) { return (long)mock::current->t.size(); }
    char* getRootSymbol(char* out = nullptr, const char* = nullptr) { if (out) { std::snprintf(out, 32, "%s", mock::current->root.c_str()); return out; } return const_cast<char*>(mock::current->root.c_str()); }
    char* getSymbol(void) { return const_cast<char*>(mock::current->sym.c_str()); }
    const char* getChartLabel(void) { return mock::current->chartLabel.c_str(); }   // (1.3.2)
    int getSecondsPerBar(void) { return mock::current->spb; }
    float getProperty(SYMBOL_PROPERTY_ID, const char* = nullptr) { return (float)mock::current->tick; }
    double getTickIncrement(const char* = nullptr) { return mock::current->tick; }
    const char* getPeriodicityLabel(void) { return mock::current->dialog ? "" : "3 Min"; }
    struct tm* getLocaltime(RTDATE d, struct tm* out) { time_t t = (time_t)d; return gmtime_r(&t, out); }
    void* getUserData(void) { return mock::current->userData; }
    void setUserData(void* p) { mock::current->userData = p; }
    RTX_REASON getReason(void) { return mock::current->reason; }
    // parameters
    void setParameterVersion(unsigned) {}
    void setParameterDialogHeight(int, int = 22) {}
    int getParameterCount(void) { return (int)mock::declared.size(); }
    RTX_RESULT setBoolParameter(const char* n, RTBOOL b, short = 0) { mock::Param p; p.type = 1; p.b = b != 0; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setIntegerParameter(const char* n, int d, short = 0, short = 0) { mock::Param p; p.type = 2; p.i = d; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setFloatParameter(const char* n, float, short = 0, short = 0) { mock::Param p; p.type = 3; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setColorParameter(const char* n, COLOR, short = 0, short = 0) { mock::Param p; p.type = 3; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setListParameter(const char* n, short, const char*, short = 0, short = 0) { mock::Param p; p.type = 3; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setLabelParameter(const char* n, short = 0, short = 0) { mock::Param p; p.type = 3; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setStringParameter(const char* n, const char*, size_t, short = 0, short = 0) { mock::Param p; p.type = 3; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTBOOL isBoxChecked(int) { return 0; } int getIntegerValue(int) { return 0; } float getFloatValue(int) { return 0; } COLOR getParameterColor(int) { return 0; } int getListIndex(int) { return -1; }
    RTX_RESULT getParameterText(int, char* t, size_t n) { if (t && n) t[0] = 0; return RTX_OK; }
    RTX_RESULT enableParameter(int, RTBOOL) { return RTX_OK; } RTX_RESULT checkBox(int, RTBOOL) { return RTX_OK; } void setIntegerValue(int, int) {} void setFloatValue(int, float) {}
    RTX_RESULT setParameterColor(int, COLOR) { return RTX_OK; } RTX_RESULT setListIndex(int, int = -1) { return RTX_OK; } RTX_RESULT setParameterText(int, const char*) { return RTX_OK; }
    // IRT's own per-indicator set: template tokens I (int) / B (RTBOOL) at natural alignment
    RTX_RESULT getParameters(void* pStruct, size_t sz, const char* t, const char*) {
        mock::Host& H = *mock::current; if (H.inParmsInit) ++H.parmsReadInInit; if (H.parmsMode == 1) return RTX_FAIL;
        unsigned char* b = static_cast<unsigned char*>(pStruct); std::memset(b, 0, sz); size_t o = 0; int k = 0;
        for (const char* c = t; *c; ++c, ++k) {
            if (*c == 'I') { o = (o + alignof(int) - 1) / alignof(int) * alignof(int); int v = H.parmsMode == 2 ? 0 : (k == 0 ? H.revBars : 0); if (o + sizeof v > sz) return RTX_FAIL; std::memcpy(b + o, &v, sizeof v); o += sizeof v; }
            else if (*c == 'B') { o = (o + alignof(RTBOOL) - 1) / alignof(RTBOOL) * alignof(RTBOOL); RTBOOL v = (RTBOOL)(H.parmsMode == 2 ? 0 : H.parmsMode == 3 && k == 2 ? 7 : ((k == 1 && H.zigzag) || (k == 2 && H.cobLegs) ? 1 : 0)); if (o + sizeof v > sz) return RTX_FAIL; std::memcpy(b + o, &v, sizeof v); o += sizeof v; }
            else return RTX_FAIL;
        }
        return RTX_OK;
    }
    RTX_RESULT saveParameters(void* pStruct, size_t sz, const char* t) {   // (1.3.0) records the values saved (I / B tokens)
        mock::Host& H = *mock::current; H.saved.clear(); H.savedTmpl = t; const unsigned char* b = static_cast<const unsigned char*>(pStruct); size_t o = 0;
        for (const char* c = t; *c; ++c) {
            if (*c == 'I') { o = (o + alignof(int) - 1) / alignof(int) * alignof(int); int v; if (o + sizeof v > sz) return RTX_FAIL; std::memcpy(&v, b + o, sizeof v); H.saved.push_back(v); o += sizeof v; }
            else if (*c == 'B') { o = (o + alignof(RTBOOL) - 1) / alignof(RTBOOL) * alignof(RTBOOL); RTBOOL v; if (o + sizeof v > sz) return RTX_FAIL; std::memcpy(&v, b + o, sizeof v); H.saved.push_back(v); o += sizeof v; }
            else return RTX_FAIL;
        }
        return RTX_OK;
    }
    // drawing
    RTX_RESULT getVisibleBars(int* a, int* b) { *a = mock::current->first; *b = (int)mock::current->t.size() - 1; return RTX_OK; }
    short getPixelsPerBar(void) { return 10; }
    short getVerticalDrawPosition(float px) { return (short)mockY(px); }
    RTX_RESULT setFont(FONT& f) { mock::current->bold = f.style == BOLD; return RTX_OK; }
    int getTextWidth(const char* s, int = -1) { return (int)std::strlen(s) * 7; }
    void setPen(COLOR c, short w, PEN_STYLE) { mock::current->pen = c; mock::current->penW = w; }
    void setTextColor(COLOR c) { mock::current->text = c; }
    void setArrayCount(int) {} void setFlags(unsigned long) {} void setDescription(const char*) {} void setVersion(const char*) {}
};
