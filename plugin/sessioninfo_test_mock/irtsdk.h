#pragma once
#include <algorithm>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

typedef unsigned long COLOR;
typedef unsigned short RTBOOL;
typedef int RTX_RESULT;

static const RTX_RESULT RTX_OK = 0;
static const RTX_RESULT RTX_FAIL = -1;
static const short PLAIN = 0;
static const short BOLD = 1;
static const short HELVETICA = 0;
static const short kParmAppendSameLine = 1;
static const short NUMW_MOCK_UNUSED = 50;
static const int DRAW_OPAQUE = 0;
static const int DRAW_TRANSLUCENT = 1;
static const int PAT_SOLID = 2;
static const unsigned long POST_DRAWING = 1UL << 18;
static const unsigned long OVERLAY = 1UL << 19;
static const unsigned long NO_UI = 1UL << 12;
static const unsigned long INSTRUMENT_SCALE = 1UL << 30;

struct FONT { short id = 0, size = 0, style = 0; };

namespace sessioninfo_mock {
inline std::vector<std::string>& drawnText() { static std::vector<std::string> value; return value; }
inline void clearDrawnText() { drawnText().clear(); }
}

struct RCT {
    short left = 0, top = 0, right = 1400, bottom = 800;
    void set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
    void getPaneRect(bool) { left = 0; top = 0; right = 1400; bottom = 800; }
    void draw(int, COLOR, COLOR, int, int) {}
    void drawText(const char* text, bool, bool) { sessioninfo_mock::drawnText().push_back(text ? text : ""); }
};

inline int localtime_s(struct tm* out, const time_t* in)
{
    return localtime_r(in, out) ? 0 : 1;
}

class cppExtension {
public:
    cppExtension() : listValues(16, -1), integerValues(16, -1), widthCalls(0) {}
    virtual ~cppExtension() {}
    virtual int init(void);
    virtual int calc(int);
    virtual int done(void);
    virtual int destroy(void);
    virtual int setup(void);
    virtual int draw(void) { return RTX_OK; }
    virtual int parmsLoad(void) { return RTX_OK; }
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }

    void setArrayCount(int) {}
    void setFlags(unsigned long) {}
    void setDescription(const char*) {}
    void setVersion(const char*) {}
    void setParameterVersion(unsigned) {}
    void setParameterDialogHeight(int, int = 22) {}
    RTX_RESULT setListParameter(const char*, short, const char*, short = 0, short = 0) { return RTX_OK; }
    RTX_RESULT setIntegerParameter(const char*, int, short = 0, short = 0) { return RTX_OK; }
    int getListIndex(int i) const { return i >= 0 && i < (int)listValues.size() ? listValues[(size_t)i] : -1; }
    int getIntegerValue(int i) const { return i >= 0 && i < (int)integerValues.size() ? integerValues[(size_t)i] : -1; }
    char* getRootSymbol(char* buffer = 0, const char* = 0) {
        if (buffer) { std::strncpy(buffer, rootSymbol.c_str(), 31); buffer[31] = '\0'; }
        return const_cast<char*>(rootSymbol.c_str());
    }
    RTX_RESULT setFont(FONT&) { return RTX_OK; }
    void setTextColor(COLOR) {}
    void setPen(COLOR, short, int) {}
    int getTextWidth(const char* text, int len = -1) {
        ++widthCalls;
        if (!text) return 0;
        return len < 0 ? (int)std::strlen(text) : len;
    }

    void testSetRoot(const std::string& value) { rootSymbol = value; }
    void testSetList(int i, int value) { if (i >= 0 && i < (int)listValues.size()) listValues[(size_t)i] = value; }
    void testSetInteger(int i, int value) { if (i >= 0 && i < (int)integerValues.size()) integerValues[(size_t)i] = value; }
    void testResetWidthCalls() { widthCalls = 0; }
    int testWidthCalls() const { return widthCalls; }

private:
    std::string rootSymbol;
    std::vector<int> listValues;
    std::vector<int> integerValues;
    int widthCalls;
};
