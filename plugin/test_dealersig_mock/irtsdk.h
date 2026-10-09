#pragma once
#include <algorithm>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

typedef unsigned long COLOR;
typedef unsigned long RTDATE;

enum { RTX_FAIL = -1, RTX_OK = 0 };
enum { OUTPUT_DISABLED = 1 };
enum { kParmAppendSameLine = 1 };
enum { kMarkerArrayUp = 1, kMarkerArrayDown = 2, kMarkerBeneathLow = 3, kMarkerAboveHigh = 4 };
static const unsigned long kSignalTrue = 0x3f800000UL;

enum fARRAY { iOut1, iOut2, barDateTime, barHigh, barLow };

struct MARKER { int number; int size; COLOR color; int location; };

namespace mockrt {
static std::vector<unsigned long> out1, out2, dates;
static std::vector<float> highs, lows;
static std::vector<int> lists(16, 0), integers(16, 10);
static std::string root = "GCE";
static int parameterCount = 0;
static int pntResult = RTX_OK, pntCalls = 0, drawTextCalls = 0;

inline void reset()
{
    out1.assign(3, 0); out2.assign(3, 0); dates.assign(3, 0);
    highs.assign(3, 100.0f); lows.assign(3, 99.0f);
    lists.assign(16, 0); integers.assign(16, 10);
    root = "GCE"; parameterCount = 0; pntResult = RTX_OK; pntCalls = 0; drawTextCalls = 0;
}
inline std::vector<unsigned long>& ints(fARRAY a)
{
    if (a == iOut1) return out1;
    if (a == iOut2) return out2;
    return dates;
}
inline std::vector<float>& floats(fARRAY a) { return a == barHigh ? highs : lows; }
}

class RTARRAY {
public:
    explicit RTARRAY(fARRAY a) : array(a) {}
    float& operator[](int i) {
        std::vector<float>& v = mockrt::floats(array);
        if (i >= (int)v.size()) v.resize((size_t)i + 1, 0.0f);
        return v[(size_t)i];
    }
private:
    fARRAY array;
};

class RTARRAYI {
public:
    explicit RTARRAYI(fARRAY a) : array(a) {}
    unsigned long& operator[](int i) {
        std::vector<unsigned long>& v = mockrt::ints(array);
        if (i >= (int)v.size()) v.resize((size_t)i + 1, 0);
        return v[(size_t)i];
    }
private:
    fARRAY array;
};

class cppExtension {
public:
    enum FONT_ID { HELVETICA = 1 };
    enum FONT_STYLE { PLAIN, BOLD };
    class FONT { public: FONT_ID id; short size; FONT_STYLE style; FONT() : id(HELVETICA), size(10), style(PLAIN) {} };
    enum BAR_POSITION { kBarCenter };
    class PNT {
    public:
        short v = -32000, h = -32000;
        int set(int bar, float, BAR_POSITION) {
            ++mockrt::pntCalls;
            if (mockrt::pntResult != RTX_OK) return mockrt::pntResult;
            h = (short)(100 + bar * 10); v = 200; return RTX_OK;
        }
    };
    class RCT {
    public:
        short top = 0, left = 0, bottom = 500, right = 500;
        void getPaneRect(bool) { top = 0; left = 0; bottom = 500; right = 500; }
        void set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
        void drawText(const char*, bool, bool) { ++mockrt::drawTextCalls; }
    };

    cppExtension() {}
    virtual ~cppExtension() {}
    virtual int init(void);
    virtual int setup(void);
    virtual int calc(int);
    virtual int done(void);
    virtual int destroy(void);
    virtual int parmsLoad(void) { return RTX_FAIL; }
    virtual int parmsApply(void) { return RTX_FAIL; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    virtual int draw(void) { return RTX_FAIL; }

    void setArrayCount(int) {}
    void setFlags(unsigned long) {}
    void setDescription(const char*) {}
    void setVersion(const char*) {}
    int getParameterCount(void) { return mockrt::parameterCount; }
    int setParameterVersion(unsigned) { return RTX_OK; }
    void setParameterDialogHeight(int) {}
    int setOutputSignalParameter(const char*, MARKER*, unsigned long = 0) { return RTX_OK; }
    int setListParameter(const char*, short d, const char*, short = 0, short = 0) { mockrt::lists[(size_t)mockrt::parameterCount] = d; ++mockrt::parameterCount; return RTX_OK; }
    int setIntegerParameter(const char*, int d, short = 0, short = 0) { mockrt::integers[(size_t)mockrt::parameterCount] = d; ++mockrt::parameterCount; return RTX_OK; }
    int getListIndex(int i) { return i >= 0 && i < (int)mockrt::lists.size() ? mockrt::lists[(size_t)i] : -1; }
    int getIntegerValue(int i) { return i >= 0 && i < (int)mockrt::integers.size() ? mockrt::integers[(size_t)i] : 0; }
    char* getRootSymbol(char* out = 0) {
        if (out) { std::strncpy(out, mockrt::root.c_str(), 31); out[31] = '\0'; return out; }
        return const_cast<char*>(mockrt::root.c_str());
    }
    long getBarCount(void) { return (long)mockrt::dates.size(); }
    std::tm* getLocaltime(RTDATE dt, std::tm* out) { std::time_t t = (std::time_t)dt; return gmtime_r(&t, out); }
    int setFont(FONT&) { return RTX_OK; }
    void setTextColor(COLOR) {}
    int getTextWidth(const char* s, int = -1) { return s ? (int)std::strlen(s) * 7 : 0; }
};

static const unsigned long POST_DRAWING = 1UL << 0;
static const unsigned long OVERLAY = 1UL << 1;
static const unsigned long INSTRUMENT_SCALE = 1UL << 2;
static const unsigned long ARRAY1_IS_SIGNAL = 1UL << 3;
static const unsigned long ARRAY2_IS_SIGNAL = 1UL << 4;
