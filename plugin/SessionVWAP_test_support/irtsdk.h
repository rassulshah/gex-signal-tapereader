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
enum fARRAY { fOut1, fOut2, fOut3, fOut4, fOut5, fOut6, fOut7, fOut8, fOut9, fOut10, fOut11, barHigh, barLow, barClose };
enum iARRAY { barVolume, barDateTime };
enum BAR_POSITION { kBarCenter };
enum FONT_ID { HELVETICA };
enum FONT_STYLE { BOLD };

namespace mock {
struct Host {
    std::string root;
    std::vector<float> f[14];
    std::vector<unsigned long> i[2];
    void* userData = NULL;
    int fontPt = 9;
};
extern Host* current;
inline std::vector<float>& floats(fARRAY a) { return current->f[(int)a]; }
inline std::vector<unsigned long>& ints(iARRAY a) { return current->i[(int)a]; }
}

class RTARRAY {
public:
    explicit RTARRAY(fARRAY a) : values(&mock::floats(a)) {}
    float& operator[](int n) { return (*values)[(size_t)n]; }
private:
    std::vector<float>* values;
};
class RTARRAYI {
public:
    explicit RTARRAYI(iARRAY a) : values(&mock::ints(a)) {}
    unsigned long& operator[](int n) { return (*values)[(size_t)n]; }
private:
    std::vector<unsigned long>* values;
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
class PNT {
public:
    short v = 100, h = 400;
    RTX_RESULT set(int, float, BAR_POSITION = kBarCenter) { return RTX_OK; }
    void setDrawPosition() {}
    void drawLineTo() {}
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
    struct tm* getLocaltime(RTDATE d, struct tm* out) { time_t t = (time_t)d; return gmtime_r(&t, out); }
    void* getUserData(void) { return mock::current ? mock::current->userData : NULL; }
    void setUserData(void* p) { if (mock::current) mock::current->userData = p; }
    void setParameterVersion(unsigned) {}
    void setParameterDialogHeight(int) {}
    RTX_RESULT setOutputParameter(const char*, int, CPEN*, COLOR, unsigned long = 0) { return RTX_OK; }
    void setArrayDrawingFlags(int, DRAW_FLAGS) {}
    int getParameterCount(void) { return 5; }
    RTX_RESULT setIntegerParameter(const char*, int, short = 0, short = 0) { return RTX_OK; }
    int getIntegerValue(int) { return mock::current ? mock::current->fontPt : 9; }
    RTX_RESULT setFont(FONT&) { return RTX_OK; }
    void setTextColor(COLOR) {}
    void setPen(COLOR, short, PEN_STYLE) {}
    RTX_RESULT getVisibleBars(int* a, int* b) { long n = getBarCount(); *a = n > 300 ? (int)n - 300 : 0; *b = (int)n - 1; return RTX_OK; }
    int getTextWidth(const char* s, int = -1) { return (int)std::strlen(s) * 8; }
    void setArrayCount(int) {}
    void setFlags(unsigned long) {}
    void setDescription(const char*) {}
    void setVersion(const char*) {}
};

#define localtime_s(out, timep) gmtime_r((timep), (out))
