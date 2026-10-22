// tests_bba115: tests_bigba/irtsdk.h + the settings-window calls BigBidAsk 1.11+ makes (getParameterCount, label / colour / list rows, getBlendedColor)
#ifndef BBA_TEST_SDK_H
#define BBA_TEST_SDK_H
// TEST DOUBLE ONLY (lsBigBidAsk host tests). Not the Linnsoft SDK: just the calls BigBidAsk.cpp makes, with the same signatures
// (checked against the real irtsdk.h by the MinGW syntax check). Volume at price per bar, per tick; settings that read UNCHECKED /
// 0 outside the settings window (as IRT does, BUILD-LESSONS #42 / #44).
#include <climits>
#include <cstring>
#include <ctime>
#include <map>
#include <string>
#include <vector>
typedef unsigned long RTDATE;
typedef unsigned long COLOR;
typedef int RTX_RESULT;
typedef bool RTBOOL;
enum { RTX_OK = 0, RTX_FAIL = -1 };
enum SYMBOL_PROPERTY_ID { SYM_TICKINCR = 171 };
enum { POST_DRAWING = 1, OVERLAY = 2, NO_UI = 4, INSTRUMENT_SCALE = 8, VAP_REQUIRED = 16, TRACK_MOUSE = (1 << 27), CALC_LAST = (1 << 28), FRONT_DRAWING = (1 << 29) };
enum { kParmAppendSameLine = 1 };
enum PEN_STYLE { P_SOLID = 0 };
enum iARRAY { barDateTime, barVolume };
enum fARRAY { barOpen, barHigh, barLow, barClose, fEmptyArray };
enum pARRAY { barVolumeProfile };
enum { E_MOUSE_UP = 7, E_MOUSE_MOVE, E_MOUSE_DBL, E_MOUSE_SCROLL, E_MOUSE_HOVER = 113 };
typedef struct RTX_EVENT { int type; union { struct { short v, h, shift, control; long bar; float price; } mouse; } v; } RTX_EVENT;
class VOLPROFILE { public: float price; long buyVolume; long sellVolume; long totalVolume; long tickCount; long maxDelta; long minDelta; };
class BARSTATISTICS { public: long buyVolume, sellVolume, totalVolume, maxPriceVol, maxPriceDelta, maxSideVolume; int ticks, tpos, maxTPOs; long maxDelta, minDelta, openDelta; short prices; };
namespace mock {
struct VRow { float price; long ask, bid; };     // ask = bought at the ask (buyVolume), bid = sold at the bid (sellVolume)
struct Bar { unsigned long time = 0, volume = 0; float open = 100, high = 101, low = 99, close = 100; std::vector<VRow> rows; bool noVap = false; };
struct Draw { std::string type, text; int l, t, r, b; COLOR color; int width, pattern, font; bool bold; COLOR fill = 0; int mode = 0; };   // (1.25.0) + fill colour / draw mode (translucent) for the OB / BO boxes
struct Host {
    std::vector<Bar> bars; void* data = nullptr; bool noSlot = false;
    std::string symbol = "ESZ6", root = "ES", chart = "ES 3m"; int seconds = 180; unsigned long now = 0;
    int paneTop = 0, paneBottom = 800, paneLeft = 0, paneRight = 2600, scaleW = 60; float priceBase = 7700; float pxPerTick = 12;
    COLOR ink = 0, pen = 0, brush = 0; short font = 12; bool bold = false; std::vector<Draw> draws; float tick = 0.25f;
    int barOffset = 0, barW = 60;   // x of bar i = 20 + (i - barOffset) * barW (left), +barW/4 centre .. +barW/2 right
    int visStart = -1, visEnd = -1;
    bool inDialog = false; std::map<int, int> ints; std::map<int, float> floats; std::map<int, bool> boxes;   // the dialog's values
    std::map<int, COLOR> colors; std::map<int, bool> enabled;   // (1.16.0) swatches, greyed rows
    int vapCalls = 0;
};
extern Host* current;
}
class cppExtension {
public:
    enum FONT_ID { HELVETICA = 4 };
    enum FONT_STYLE { PLAIN = 0, BOLD = 1 };
    enum DRAWTYPE { DRAW_OPAQUE, DRAW_TRANSLUCENT };
    enum BRUSH_STYLE { PAT_NONE, PAT_HOLLOW, PAT_SOLID };
    enum BAR_POSITION { kBarLeft, kBarCenter, kBarRight };
    struct FONT { FONT_ID id = HELVETICA; short size = 12; FONT_STYLE style = PLAIN; };
    struct CBRUSH { COLOR c; CBRUSH(COLOR c_, BRUSH_STYLE) : c(c_) {} void set() { mock::current->brush = c; } };
    struct PNT {
        short v = 0, h = 0;
        RTX_RESULT set(int bar, float price, BAR_POSITION pos = kBarLeft) {
            const mock::Host* H = mock::current;
            h = (short)(20 + (bar - H->barOffset) * H->barW + (pos == kBarCenter ? H->barW / 4 : pos == kBarRight ? H->barW / 2 : 0));
            v = (short)(H->paneBottom - 100 - (price - H->priceBase) / H->tick * H->pxPerTick);
            return RTX_OK;
        }
    };
    struct RCT {
        short top = 0, left = 0, bottom = 0, right = 0;
        void set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
        void getPaneRect(RTBOOL = false) { set((short)mock::current->paneLeft, (short)mock::current->paneTop, (short)mock::current->paneRight, (short)mock::current->paneBottom); }
        void getScaleRect(void) { set((short)(mock::current->paneRight - mock::current->scaleW), (short)mock::current->paneTop, (short)mock::current->paneRight, (short)mock::current->paneBottom); }
        void draw(short width = 0, COLOR border = 0, COLOR fill = 0, DRAWTYPE mode = DRAW_OPAQUE, BRUSH_STYLE pat = PAT_SOLID) {
            mock::Draw d; d.type = "rect"; d.l = left; d.t = top; d.r = right; d.b = bottom; d.color = border; d.width = width; d.pattern = (int)pat; d.fill = fill; d.mode = (int)mode; d.font = 0; d.bold = false;
            mock::current->draws.push_back(d);
        }
        void drawOval(DRAWTYPE mode = DRAW_OPAQUE) {
            mock::Draw d; d.type = "oval"; d.l = left; d.t = top; d.r = right; d.b = bottom; d.color = mock::current->pen; d.fill = mock::current->brush; d.width = 1; d.pattern = 0; d.mode = (int)mode; d.font = 0; d.bold = false;
            mock::current->draws.push_back(d);
        }
        void drawRounded(int, int) {
            mock::Draw d; d.type = "ring"; d.l = left; d.t = top; d.r = right; d.b = bottom; d.color = mock::current->pen; d.width = 1; d.pattern = 0; d.font = 0; d.bold = false;
            mock::current->draws.push_back(d);
        }
        void drawText(const char* t, RTBOOL centered = true, RTBOOL = false) {
            mock::Draw d; d.type = centered ? "ctext" : "text"; d.text = t; d.l = left; d.t = top; d.r = right; d.b = bottom; d.color = mock::current->ink; d.width = 0; d.pattern = 0;
            d.font = mock::current->font; d.bold = mock::current->bold; mock::current->draws.push_back(d);
        }
    };
    struct RTARRAYI {
        iARRAY type; long count;
        RTARRAYI(iARRAY t) : type(t), count((long)mock::current->bars.size()) {}
        unsigned long& operator[](int i) { mock::Bar& b = mock::current->bars.at((size_t)i); return type == barDateTime ? b.time : b.volume; }
    };
    struct RTARRAY {
        fARRAY type; long count;
        RTARRAY(fARRAY t) : type(t), count(t == fEmptyArray ? 0 : (long)mock::current->bars.size()) {}
        float& operator[](int i) { mock::Bar& b = mock::current->bars.at((size_t)i); return type == barOpen ? b.open : type == barHigh ? b.high : type == barLow ? b.low : b.close; }
    };
    struct RTARRAYP {
        long count;
        RTARRAYP(pARRAY) : count((long)mock::current->bars.size()) {}
        RTX_RESULT getBarStatistics(int bar, BARSTATISTICS& s) {
            ++mock::current->vapCalls;
            if (bar < 0 || bar >= (int)mock::current->bars.size()) return RTX_FAIL;
            const mock::Bar& b = mock::current->bars[(size_t)bar];
            if (b.noVap) return RTX_FAIL;
            std::memset(&s, 0, sizeof(s)); s.prices = (short)b.rows.size();
            for (const auto& r : b.rows) { s.buyVolume += r.ask; s.sellVolume += r.bid; s.totalVolume += r.ask + r.bid; }
            return RTX_OK;
        }
        RTX_RESULT getVolumeProfile(int bar, int k, VOLPROFILE& v) {
            ++mock::current->vapCalls;
            const mock::Bar& b = mock::current->bars.at((size_t)bar);
            if (b.noVap || k < 0 || k >= (int)b.rows.size()) return RTX_FAIL;
            std::memset(&v, 0, sizeof(v)); v.price = b.rows[(size_t)k].price; v.buyVolume = b.rows[(size_t)k].ask; v.sellVolume = b.rows[(size_t)k].bid;
            v.totalVolume = v.buyVolume + v.sellVolume; return RTX_OK;
        }
    };
    virtual ~cppExtension() {}
    virtual int init(); virtual int setup(); virtual int calc(int); virtual int done(); virtual int destroy();
    virtual int draw() { return RTX_FAIL; } virtual int mouse(RTX_EVENT*) { return RTX_FAIL; }
    virtual int parmsLoad() { return RTX_FAIL; } virtual int parmsApply() { return RTX_FAIL; } virtual int parmsUpdt(unsigned int) { return RTX_FAIL; }
    void* getUserData() { return mock::current->data; } void setUserData(void* p) { if (!mock::current->noSlot) mock::current->data = p; }
    long getBarCount() { return (long)mock::current->bars.size(); }
    char* getRootSymbol(char* b = nullptr, const char* = nullptr) { static std::string r; r = mock::current->root; if (b) { std::strcpy(b, r.c_str()); return b; } return &r[0]; }
    char* getSymbol() { static std::string s; s = mock::current->symbol; return &s[0]; }
    char* getChartLabel() { static std::string s; s = mock::current->chart; return &s[0]; }
    char* getPeriodicityLabel() { static std::string s; s = mock::current->inDialog ? "" : "3 Min"; return &s[0]; }
    int getSecondsPerBar() { return mock::current->seconds; }
    float getProperty(SYMBOL_PROPERTY_ID, const char* = nullptr) { return mock::current->tick; }
    RTDATE currentDate() { return mock::current->now; }
    struct tm* getLocaltime(RTDATE stamp, struct tm* t) { time_t v = (time_t)stamp; return gmtime_r(&v, t); }   // stamps are Central wall time in the tests
    RTX_RESULT getVisibleBars(int* s, int* e) {
        if (mock::current->visStart < 0) { *s = 0; *e = (int)mock::current->bars.size() - 1; return RTX_OK; }
        *s = mock::current->visStart; *e = mock::current->visEnd; return RTX_OK;
    }
    void setParameterVersion(unsigned) {} void setParameterDialogHeight(int, int = 22) {}
    RTX_RESULT setIntegerParameter(const char*, int, short = 0, short = 0) { ++nParm; return RTX_OK; }
    RTX_RESULT setFloatParameter(const char*, float, short = 0, short = 0) { ++nParm; return RTX_OK; }
    RTX_RESULT setBoolParameter(const char*, RTBOOL, short = 0) { ++nParm; return RTX_OK; }
    // IRT hands the CHART copy 0 / unchecked; only the dialog copy has values
    int getIntegerValue(int i) { return mock::current->inDialog ? mock::current->ints[i] : 0; }
    float getFloatValue(int i) { return mock::current->inDialog ? mock::current->floats[i] : 0.0f; }
    RTBOOL isBoxChecked(int i) { return mock::current->inDialog ? mock::current->boxes[i] : false; }
    void setIntegerValue(int i, int v) { mock::current->ints[i] = v; }
    void setFloatValue(int i, float v) { mock::current->floats[i] = v; }
    RTX_RESULT checkBox(int i, RTBOOL v) { mock::current->boxes[i] = v; return RTX_OK; }
    void setFont(FONT& f) { mock::current->font = f.size; mock::current->bold = f.style == BOLD; }
    void setTextColor(COLOR c) { mock::current->ink = c; }
    void setPen(COLOR c, short, PEN_STYLE) { mock::current->pen = c; }
    int getTextWidth(const char* t, int = -1) { return (int)std::strlen(t) * (mock::current->font * 6 / 11) + (mock::current->bold ? 2 : 0); }
    void getFontMetrics(int* lead, int* asc, int* desc) { *lead = 0; *asc = mock::current->font; *desc = 3; }
    RTX_RESULT invalidateChart(RTBOOL = false) { return RTX_OK; }
    // (tests_bba115) the 1.11+ settings window calls: rows counted so control numbers are real; values as the chart copy sees them
    int nParm = 0; int getParameterCount() { return nParm; }
    RTX_RESULT setLabelParameter(const char*, short = 0, short = 0) { ++nParm; return RTX_OK; }
    RTX_RESULT setColorParameter(const char*, COLOR, short = 0, short = 0) { ++nParm; return RTX_OK; }
    RTX_RESULT setListParameter(const char*, short, const char*, short = 0, short = 0) { ++nParm; return RTX_OK; }
    int getListIndex(int i) { return mock::current->inDialog ? mock::current->ints[i] : 0; }
    void setListIndex(int i, int v) { mock::current->ints[i] = v; }
    void setLabelText(int, const char*) {}
    // (1.16.0) the colour swatches and greying (the real SDK: setParameterColor / getParameterColor / enableParameter)
    RTX_RESULT setParameterColor(int i, COLOR c) { mock::current->colors[i] = c; return RTX_OK; }
    COLOR getParameterColor(int i) { return mock::current->inDialog ? mock::current->colors[i] : 0; }
    RTX_RESULT enableParameter(int i, RTBOOL b) { mock::current->enabled[i] = b; return RTX_OK; }
    // (1.16.1 settings layer) TEXT rows (not used by BigBidAsk; SettingsHost.h compiles them)
    RTX_RESULT setStringParameter(const char*, const char*, size_t, short = 0, short = 0) { ++nParm; return RTX_OK; }
    RTX_RESULT getParameterText(int, char* t, size_t n) { if (t && n) t[0] = 0; return RTX_OK; } RTX_RESULT setParameterText(int, const char*) { return RTX_OK; }
    COLOR getBlendedColor(COLOR a, COLOR, float) { return a; }
    void setArrayCount(int) {} void setFlags(unsigned long) {} void setDescription(const char*) {} void setVersion(const char*) {}
};
#endif
