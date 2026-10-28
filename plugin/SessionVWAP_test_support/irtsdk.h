#pragma once
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

typedef unsigned long COLOR;
typedef unsigned long RTDATE;
typedef unsigned short RTBOOL;
typedef int RTX_RESULT;
static const RTX_RESULT RTX_OK = 0;
static const RTX_RESULT RTX_FAIL = -1;
static const COLOR COLOR_BLACK = 0;

enum PEN_STYLE { P_SOLID };
enum DRAW_TYPE { DRAW_CONNECTEDLINE, DRAW_INVISIBLE };
enum OUTPUT_FLAGS { OUTPUT_ENABLED, OUTPUT_NO_UI };   // (RA1009) OUTPUT_NO_UI: SessionVWAP 1.3.4 hides its outputs
enum DRAW_FLAGS { CONNECT_CNONZERO = 1, NO_AUTOSCALE = 2 };
enum BRUSH_STYLE { PAT_SOLID };
enum { POST_DRAWING = 1 << 0, OVERLAY = 1 << 1, INSTRUMENT_SCALE = 1 << 2 };
enum { DRAW_OPAQUE = 0 };
enum fARRAY { fOut1, fOut2, fOut3, fOut4, fOut5, fOut6, fOut7, fOut8, fOut9, fOut10, fOut11, barHigh, barLow, barClose, barOpen };   // (2026-10-27) + barOpen (SessionVWAP 1.7.0 self-awareness reads it)
enum iARRAY { barVolume, barDateTime };
enum BAR_POSITION { kBarCenter };
enum FONT_ID { HELVETICA };
enum FONT_STYLE { BOLD };

static const short kParmAppendSameLine = 1;
namespace mock {
// (1.4.3 settings) the parameters the plugin declared (one DLL object: declared once) and each chart's values.
// IRT fills list / check-box / colour values only while the settings window is open: outside it a list reads -1, a box
// unchecked and a colour black (what our plugins found on his IRT) - `dialog` switches between the two.
struct Param { int type = 0; bool b = false; COLOR c = 0; int list = 0, listCount = 0; std::string name; };   // type 1 bool, 2 colour, 3 list
inline std::vector<Param> declared;                    // C++17 inline: one list for every test TU
struct Line { COLOR c; short w; int b0 = 0, b1 = 0; float p0 = 0, p1 = 0; };   // pen + (bar, price) ends, for the render check
struct Host {
    bool dialog = false;
    std::vector<Param> val;                                   // this chart's saved values (empty = the declared defaults)
    std::vector<Line> lines;                                  // every drawLineTo, with the pen in force
    COLOR pen = 0; short penW = 1;
    int curBar = 0; float curPx = 0;                          // the draw position (setDrawPosition)
    std::string root;
    std::vector<float> f[15];
    std::vector<unsigned long> i[2];
    void* userData = NULL;
    int fontPt = 9;
    std::string chart = "ES 3 Min";   // (1.6.0) getChartLabel / getSecondsPerBar for the Market State post
    int spb = 180;
    // (SessionVWAP 1.8.1) IRT's own per-indicator parameter set, handed to the extension by getParameters (any copy, not only the
    // window): `val` is this indicator's saved set. parmsMode 0 = IRT hands it, 1 = refuses (RTX_FAIL), 2 = hands a blank (all-zero) set.
    int parmsMode = 0;
    std::vector<Param> former;                                // an older-version saved set (getFormerParameters), empty = none
    int getParmsCalls = 0;
};
extern Host* current;
inline Param* param(int i) {
    if (!current || i < 0 || i >= (int)declared.size()) return NULL;
    if (current->val.size() != declared.size()) current->val = declared;
    return &current->val[(size_t)i];
}
inline std::vector<float>& floats(fARRAY a) { return current->f[(int)a]; }
inline std::vector<unsigned long>& ints(iARRAY a) { return current->i[(int)a]; }
}

class RTARRAY {
public:
    explicit RTARRAY(fARRAY a) : values(&mock::floats(a)), count((long)mock::floats(a).size()) {}
    float& operator[](int n) { return (*values)[(size_t)n]; }
private:
    std::vector<float>* values;
public:
    long count;   // (2026-10-27) like the SDK's RTARRAY::count
};
class RTARRAYI {
public:
    explicit RTARRAYI(iARRAY a) : values(&mock::ints(a)), count((long)mock::ints(a).size()) {}
    unsigned long& operator[](int n) { return (*values)[(size_t)n]; }
private:
    std::vector<unsigned long>* values;
public:
    long count;
};
class CPEN {
public:
    CPEN(COLOR = 0, short = 1, PEN_STYLE = P_SOLID) {}
};
class FONT {
public:
    FONT_ID id = HELVETICA;
    short size = 9;
    FONT_STYLE style = BOLD;
};
#define KO_PNT PNT   // (SessionVWAP 1.6.3) KeepOutDraw.h: this mock's PNT is not nested in cppExtension
class PNT {
public:
    short v = 100, h = 400;
    int bar = 0; float px = 0;
    RTX_RESULT set(int b, float p, BAR_POSITION = kBarCenter) { bar = b; px = p; return RTX_OK; }
    void setDrawPosition() { if (mock::current) { mock::current->curBar = bar; mock::current->curPx = px; } }
    void drawLineTo() { if (mock::current) { mock::Line l; l.c = mock::current->pen; l.w = mock::current->penW; l.b0 = mock::current->curBar; l.p0 = mock::current->curPx; l.b1 = bar; l.p1 = px; mock::current->lines.push_back(l); mock::current->curBar = bar; mock::current->curPx = px; } }
};
class RCT {
public:
    short top = 0, left = 0, bottom = 600, right = 800;
    void getPaneRect(RTBOOL = false) {}
    void set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
    void draw(short = 0, COLOR = 0, COLOR = 0, int = DRAW_OPAQUE, BRUSH_STYLE = PAT_SOLID) {}
    void drawText(const char*, RTBOOL = true, RTBOOL = false) {}
};

class cppExtension {
public:
    cppExtension() {}
    virtual ~cppExtension() {}
    virtual int init(void);
    virtual int setup(void);
    virtual int calc(int);
    virtual int done(void);
    virtual int destroy(void);
    virtual int draw(void) { return RTX_FAIL; }
    long getBarCount(void) { return mock::current ? (long)mock::current->f[(int)barClose].size() : 0; }
    char* getRootSymbol(char* out = NULL, const char* = NULL) {
        if (!mock::current) return NULL;
        if (out) { std::snprintf(out, 32, "%s", mock::current->root.c_str()); return out; }
        return const_cast<char*>(mock::current->root.c_str());
    }
    const char* getSymbol(void) { return mock::current ? mock::current->root.c_str() : NULL; }   // (1.4.2 svRecordPath)
    // (1.5.1) the settings window's copy has no bar size: "" inside the dialog, as IRT (svInDialog); (1.6.0) chart label + bar size
    const char* getPeriodicityLabel(void) { return (mock::current && !mock::current->dialog) ? "3 Min" : ""; }
    const char* getChartLabel(void) { return mock::current ? mock::current->chart.c_str() : NULL; }
    int getSecondsPerBar(void) { return mock::current ? mock::current->spb : 0; }
    struct tm* getLocaltime(RTDATE d, struct tm* out) { time_t t = (time_t)d; return gmtime_r(&t, out); }
    void* getUserData(void) { return mock::current ? mock::current->userData : NULL; }
    void setUserData(void* p) { if (mock::current) mock::current->userData = p; }
    void setParameterVersion(unsigned) {}
    void setParameterDialogHeight(int) {}
    RTX_RESULT setOutputParameter(const char*, int, CPEN*, COLOR, unsigned long = 0) { return RTX_OK; }
    void setArrayDrawingFlags(int, DRAW_FLAGS) {}
    int getParameterCount(void) { return (int)mock::declared.size(); }
    static int countChoices(const char* c) { int n = 1; for (; c && *c; ++c) if (*c == ';') ++n; return n; }
    RTX_RESULT setBoolParameter(const char* n, RTBOOL b, short = 0) { mock::Param p; p.type = 1; p.b = b != 0; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setColorParameter(const char* n, COLOR c, short = 0, short = 0) { mock::Param p; p.type = 2; p.c = c; p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setListParameter(const char* n, short d, const char* ch, short = 0, short = 0) { mock::Param p; p.type = 3; p.list = d; p.listCount = countChoices(ch); p.name = n; mock::declared.push_back(p); return RTX_OK; }
    RTBOOL isBoxChecked(int i) { mock::Param* p = mock::param(i); return (p && p->type == 1 && mock::current->dialog && p->b) ? 1 : 0; }
    COLOR getParameterColor(int i) { mock::Param* p = mock::param(i); return (p && p->type == 2 && mock::current->dialog) ? p->c : 0; }
    int getListIndex(int i) { mock::Param* p = mock::param(i); return (p && p->type == 3 && mock::current->dialog) ? p->list : -1; }
    RTX_RESULT checkBox(int i, RTBOOL b) { mock::Param* p = mock::param(i); if (!p || p->type != 1) return RTX_FAIL; p->b = b != 0; return RTX_OK; }
    RTX_RESULT setParameterColor(int i, COLOR c) { mock::Param* p = mock::param(i); if (!p || p->type != 2) return RTX_FAIL; p->c = c; return RTX_OK; }
    RTX_RESULT setListIndex(int i, int v = -1) { mock::Param* p = mock::param(i); if (!p || p->type != 3 || v >= p->listCount) return RTX_FAIL; p->list = v; return RTX_OK; }
    RTX_RESULT setIntegerParameter(const char*, int, short = 0, short = 0) { return RTX_OK; }
    int getIntegerValue(int) { return mock::current ? mock::current->fontPt : 9; }
    // (SessionVWAP 1.6.1, settings layer) the rest of the parameter API SettingsHost.h may call (rows counted like the others)
    void setIntegerValue(int, int) {}
    RTX_RESULT setFloatParameter(const char*, float, short = 0, short = 0) { mock::Param p; p.type = 4; mock::declared.push_back(p); return RTX_OK; }
    float getFloatValue(int) { return 0.0f; } void setFloatValue(int, float) {}
    RTX_RESULT setLabelParameter(const char*, short = 0, short = 0) { mock::Param p; p.type = 5; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT setStringParameter(const char*, const char*, size_t, short = 0, short = 0) { mock::Param p; p.type = 6; mock::declared.push_back(p); return RTX_OK; }
    RTX_RESULT getParameterText(int, char* t, size_t n) { if (t && n) t[0] = 0; return RTX_OK; } RTX_RESULT setParameterText(int, const char*) { return RTX_OK; }
    RTX_RESULT enableParameter(int, RTBOOL) { return RTX_OK; }
    RTX_RESULT setFont(FONT&) { return RTX_OK; }
    void setTextColor(COLOR) {}
    void setPen(COLOR c, short w, PEN_STYLE) { if (mock::current) { mock::current->pen = c; mock::current->penW = w; } }
    RTX_RESULT getVisibleBars(int* a, int* b) { long n = getBarCount(); *a = n > 300 ? (int)n - 300 : 0; *b = (int)n - 1; return RTX_OK; }
    int getTextWidth(const char* s, int = -1) { return (int)std::strlen(s) * 8; }
    void setArrayCount(int) {}
    void setFlags(unsigned long) {}
    void setDescription(const char*) {}
    void setVersion(const char*) {}
    // (SessionVWAP 1.8.1) the SDK's parameter-set calls: one template token per row in row order (B RTBOOL, U COLOR, S short list,
    // I int), fields at their natural alignment as the compiler lays out the plugin's struct
    static size_t alignUp(size_t o, size_t a) { return (o + a - 1) / a * a; }
    static RTX_RESULT fillFrom(const std::vector<mock::Param>& src, void* pStruct, size_t sz, const char* t, bool blank) {
        unsigned char* b = static_cast<unsigned char*>(pStruct); std::memset(b, 0, sz); size_t o = 0; size_t k = 0;
        for (const char* c = t; *c; ++c, ++k) {
            const mock::Param* p = k < src.size() ? &src[k] : NULL;
            if (*c == 'B') { o = alignUp(o, alignof(RTBOOL)); if (o + sizeof(RTBOOL) > sz) return RTX_FAIL; RTBOOL v = (!blank && p && p->type == 1 && p->b) ? 1 : 0; std::memcpy(b + o, &v, sizeof v); o += sizeof v; }
            else if (*c == 'U') { o = alignUp(o, alignof(COLOR)); if (o + sizeof(COLOR) > sz) return RTX_FAIL; COLOR v = (!blank && p && p->type == 2) ? p->c : 0; std::memcpy(b + o, &v, sizeof v); o += sizeof v; }
            else if (*c == 'S') { o = alignUp(o, alignof(short)); if (o + sizeof(short) > sz) return RTX_FAIL; short v = (short)((!blank && p && p->type == 3) ? p->list : 0); std::memcpy(b + o, &v, sizeof v); o += sizeof v; }
            else return RTX_FAIL;
        }
        return RTX_OK;
    }
    RTX_RESULT getParameters(void* pStruct, size_t sz, const char* t, const char*) {
        if (!mock::current) return RTX_FAIL;
        ++mock::current->getParmsCalls;
        if (mock::current->parmsMode == 1) return RTX_FAIL;
        if (mock::current->val.size() != mock::declared.size()) mock::current->val = mock::declared;
        return fillFrom(mock::current->val, pStruct, sz, t, mock::current->parmsMode == 2);
    }
    RTX_RESULT getFormerParameters(void* pStruct, size_t sz, const char* t, const char*) {
        if (!mock::current || mock::current->former.empty()) return RTX_FAIL;
        return fillFrom(mock::current->former, pStruct, sz, t, false);
    }
    RTX_RESULT saveParameters(void* pStruct, size_t sz, const char* t) {
        if (!mock::current) return RTX_FAIL;
        if (mock::current->val.size() != mock::declared.size()) mock::current->val = mock::declared;
        const unsigned char* b = static_cast<const unsigned char*>(pStruct); size_t o = 0, k = 0;
        for (const char* c = t; *c; ++c, ++k) {
            mock::Param* p = k < mock::current->val.size() ? &mock::current->val[k] : NULL;
            if (*c == 'B') { o = alignUp(o, alignof(RTBOOL)); RTBOOL v; if (o + sizeof v > sz) return RTX_FAIL; std::memcpy(&v, b + o, sizeof v); o += sizeof v; if (p) p->b = v != 0; }
            else if (*c == 'U') { o = alignUp(o, alignof(COLOR)); COLOR v; if (o + sizeof v > sz) return RTX_FAIL; std::memcpy(&v, b + o, sizeof v); o += sizeof v; if (p) p->c = v; }
            else if (*c == 'S') { o = alignUp(o, alignof(short)); short v; if (o + sizeof v > sz) return RTX_FAIL; std::memcpy(&v, b + o, sizeof v); o += sizeof v; if (p) p->list = v; }
            else return RTX_FAIL;
        }
        return RTX_OK;
    }
};

#define localtime_s(out, timep) gmtime_r((timep), (out))
