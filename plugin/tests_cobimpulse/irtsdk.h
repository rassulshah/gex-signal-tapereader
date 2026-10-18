#ifndef CIMP_TEST_SDK_H
#define CIMP_TEST_SDK_H
// TEST DOUBLE ONLY (lsCobImpulse host tests). Not the Linnsoft SDK: just the calls CobImpulse.cpp makes, with the same signatures.
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
enum { SYM_TICKINCR = 171 };
enum { POST_DRAWING = 1, OVERLAY = 2, NO_UI = 4, INSTRUMENT_SCALE = 8, TRACK_MOUSE = (1 << 27), CALC_LAST = (1 << 28), FRONT_DRAWING = (1 << 29) };
enum iARRAY { barDateTime, barVolume };
enum fARRAY { barOpen, barHigh, barLow, barClose, fEmptyArray };
enum { E_MOUSE_UP = 7, E_MOUSE_MOVE, E_MOUSE_DBL, E_MOUSE_SCROLL, E_MOUSE_HOVER = 113 };
typedef struct RTX_EVENT { int type; union { struct { short v, h, shift, control; long bar; float price; } mouse; } v; } RTX_EVENT;
namespace mock {
struct Bar { unsigned long time = 0, volume = 0; float open = 100, high = 101, low = 99, close = 100; };
struct Draw { std::string type, text; int l, t, r, b; COLOR color; int width, pattern; };
struct Host {
    std::vector<Bar> bars; void* data = nullptr; bool noSlot = false;
    std::string symbol = "NQZ6", root = "NQ"; int seconds = 180; unsigned long now = 0;
    std::string chart = "NQ 3 Min";                             // (0.4.1) getChartLabel (the Market State key)
    std::map<std::string, std::vector<float>> custom;          // his RTL by name (aligned to bars unless shorter)
    std::map<std::string, bool> customFail;
    int customCalls = 0, invalidates = 0;
    int paneTop = 0, paneBottom = 800, paneLeft = 0, paneRight = 3200; float priceBase = 30000; float pxPerPoint = 2;
    COLOR ink = 0; std::vector<Draw> draws; float tick = 0.25f; int barOffset = 0;   // x of bar i = 20 + (i - barOffset) x 10
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
    struct PNT {
        short h = 0, v = 0;
        RTX_RESULT set(int bar, float price, BAR_POSITION pos = kBarLeft) {
            h = (short)(20 + (bar - mock::current->barOffset) * 10 + (pos == kBarCenter ? 4 : pos == kBarRight ? 8 : 0));
            v = (short)(mock::current->paneBottom - (price - mock::current->priceBase) * mock::current->pxPerPoint);
            return RTX_OK;
        }
    };
    struct RCT {
        short left = 0, top = 0, right = 0, bottom = 0;
        void set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
        void getPaneRect(RTBOOL = false) { set((short)mock::current->paneLeft, (short)mock::current->paneTop, (short)mock::current->paneRight, (short)mock::current->paneBottom); }
        void draw(short width = 0, COLOR border = 0, COLOR fill = 0, DRAWTYPE = DRAW_OPAQUE, BRUSH_STYLE pat = PAT_SOLID) {
            mock::Draw d; d.type = "rect"; d.l = left; d.t = top; d.r = right; d.b = bottom; d.color = border; d.width = width; d.pattern = (int)pat; (void)fill;
            mock::current->draws.push_back(d);
        }
        void drawText(const char* t, RTBOOL centered = true, RTBOOL = false) {
            mock::Draw d; d.type = centered ? "ctext" : "text"; d.text = t; d.l = left; d.t = top; d.r = right; d.b = bottom; d.color = mock::current->ink; d.width = 0; d.pattern = 0;
            mock::current->draws.push_back(d);
        }
    };
    struct RTARRAYI {
        iARRAY type; long count;
        RTARRAYI(iARRAY t) : type(t), count((long)mock::current->bars.size()) {}
        unsigned long& operator[](int i) { mock::Bar& b = mock::current->bars.at((size_t)i); return type == barDateTime ? b.time : b.volume; }
    };
    struct RTARRAY {
        fARRAY type; long count; const std::vector<float>* cust = nullptr;
        RTARRAY(fARRAY t) : type(t), count(t == fEmptyArray ? 0 : (long)mock::current->bars.size()) {}
        float& operator[](int i) {
            if (cust) { static float tmp; tmp = cust->at((size_t)i); return tmp; }
            mock::Bar& b = mock::current->bars.at((size_t)i);
            return type == barOpen ? b.open : type == barHigh ? b.high : type == barLow ? b.low : b.close;
        }
        RTX_RESULT getCustomIndicator(char* name, int = 1, RTBOOL = false, const char* = nullptr, int = 0) {
            ++mock::current->customCalls;
            if (mock::current->customFail[name]) return RTX_FAIL;
            auto it = mock::current->custom.find(name); if (it == mock::current->custom.end()) return RTX_FAIL;
            cust = &it->second; count = (long)std::min(it->second.size(), mock::current->bars.size()); return RTX_OK;
        }
    };
    virtual ~cppExtension() {}
    virtual int init(); virtual int setup(); virtual int calc(int); virtual int done(); virtual int destroy();
    virtual int draw() { return RTX_FAIL; } virtual int mouse(RTX_EVENT*) { return RTX_FAIL; }
    virtual int parmsLoad() { return RTX_FAIL; } virtual int parmsApply() { return RTX_FAIL; } virtual int parmsUpdt(unsigned int) { return RTX_FAIL; }
    void* getUserData() { return mock::current->data; } void setUserData(void* p) { if (!mock::current->noSlot) mock::current->data = p; }
    long getBarCount() { return (long)mock::current->bars.size(); }
    const char* getRootSymbol(char* b = nullptr) { if (b) { std::strcpy(b, mock::current->root.c_str()); return b; } return mock::current->root.c_str(); }
    const char* getSymbol() { return mock::current->symbol.c_str(); }
    int getSecondsPerBar() { return mock::current->seconds; }
    const char* getChartLabel() { return mock::current->chart.c_str(); }
    float getProperty(int, const char* = nullptr) { return mock::current->tick; }
    RTDATE currentDate() { return mock::current->now; }
    struct tm* getLocaltime(RTDATE stamp, struct tm* t) { time_t v = (time_t)stamp; return gmtime_r(&v, t); }   // stamps are Central wall time in the tests
    void setParameterVersion(unsigned) {}
    void setFont(FONT&) {} void setTextColor(COLOR c) { mock::current->ink = c; }
    int getTextWidth(const char* t, int = -1) { return (int)std::strlen(t) * 6; }
    RTX_RESULT invalidateChart(RTBOOL = false) { ++mock::current->invalidates; return RTX_OK; }
    void setArrayCount(int) {} void setFlags(unsigned long) {} void setDescription(const char*) {} void setVersion(const char*) {}
};
#endif
