#ifndef DAYMODEL_LIFECYCLE_MOCK_IRTSDK_H
#define DAYMODEL_LIFECYCLE_MOCK_IRTSDK_H

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>

#ifndef NOMINMAX
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#define far mock_far

#define sprintf_s std::snprintf

typedef unsigned long COLOR;
typedef unsigned long RTDATE;
typedef int PEN_STYLE;

enum { RTX_FAIL = -1, RTX_OK = 0 };
enum { POST_DRAWING = 1 << 0, OVERLAY = 1 << 1, NO_UI = 1 << 2, INSTRUMENT_SCALE = 1 << 3 };
enum { P_SOLID = 0, P_DASH = 1, P_DOT = 2 };
enum { HELVETICA = 0, BOLD = 1, PLAIN = 0, DRAW_OPAQUE = 0, PAT_SOLID = 0, kParmAppendSameLine = 0 };
enum { barOpen = 0, barHigh = 1, barLow = 2, barClose = 3, barDateTime = 4 };

struct FONT { short id; short size; short style; };
struct PNT {
    short h, v;
    void set(long, float) { h = 0; v = 0; }
    void setDrawPosition() {}
    void drawLineTo() {}
    void drawText(const char*) {}
};
struct RCT {
    short left, top, right, bottom;
    RCT() : left(0), top(0), right(0), bottom(0) {}
    void set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
    void draw(int, COLOR, COLOR, int, int) {}
    void drawText(const char*, bool, bool) {}
    void getPaneRect(bool) { left = top = 0; right = bottom = 100; }
};
class RTARRAY {
public:
    explicit RTARRAY(int) {}
    float operator[](int) const { return 0.0f; }
};
class RTARRAYI {
public:
    explicit RTARRAYI(int) {}
    unsigned long operator[](int) const { return 0UL; }
};

class cppExtension {
    void** currentUserData_;
public:
    cppExtension() : currentUserData_(0) {}
    ~cppExtension() {}
    virtual int init(void);
    virtual int setup(void);
    virtual int calc(int);
    virtual int done(void);
    virtual int destroy(void);
    virtual int draw(void) { return RTX_FAIL; }
    virtual int parmsLoad(void) { return RTX_FAIL; }
    virtual int parmsApply(void) { return RTX_FAIL; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }

    void mockSetCurrentUserDataSlot(void** slot) { currentUserData_ = slot; }
    void* getUserData(void) { return currentUserData_ ? *currentUserData_ : 0; }
    void setUserData(void* data) { if (currentUserData_) *currentUserData_ = data; }

    long getBarCount(void) { return 0; }
    void getLocaltime(RTDATE, struct tm* out) { if (out) std::memset(out, 0, sizeof(*out)); }
    RTDATE currentDate(void) { return 0UL; }
    int getIntegerValue(int) { return 10; }
    int getListIndex(int) { return 0; }
    int isBoxChecked(int) { return 1; }
    void setParameterVersion(int) {}
    void setParameterDialogHeight(int) {}
    void setListParameter(const char*, int, const char*, int = 0, short = 0) {}
    void setIntegerParameter(const char*, int, int = 0, short = 0) {}
    void setBoolParameter(const char*, bool, short = 0) {}
    void setColorParameter(const char*, COLOR, int = 0, short = 0) {}
    void setPen(COLOR, short, PEN_STYLE) {}
    void setFont(FONT) {}
    void getFontMetrics(int* lead, int* asc, int* desc) { if (lead) *lead = 0; if (asc) *asc = 0; if (desc) *desc = 0; }
    int getTextWidth(const char*, int = -1) { return 0; }
    void setTextColor(COLOR) {}
    void setArrayCount(int) {}
    void setFlags(int) {}
    void setDescription(const char*) {}
    void setVersion(const char*) {}
};

#endif
