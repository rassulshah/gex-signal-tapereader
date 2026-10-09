#include "shim.h"
#include "mockirt.h"
#include "irtsdk.h"
#include <cstring>
long long g_now = 0; std::vector<MTick> g_ticks; std::vector<long long> g_bars; std::string g_root = "ES", g_sym = "ESZ26";
std::vector<std::string> g_text; long g_lines = 0, g_rects = 0, g_ttCalls = 0; long long g_ttLastStart = 0, g_ttMaxBack = 0; bool g_timerOk = true;
static std::map<int, std::vector<float>> g_f;             // output arrays
static std::vector<unsigned long> g_dt;
cppExtension::cppExtension() {}
cppExtension::~cppExtension() {}
cppExtension::CPEN::CPEN(COLOR c, short w, PEN_STYLE s) : color(c), style(s), width(w) {}
cppExtension::FONT::FONT(FONT_ID i, short s, FONT_STYLE st) : id(i), size(s), style(st) {}
std::map<unsigned long, long> g_lineColors; static unsigned long g_pen = 0;   // (2.0.2) lines drawn per pen colour
void cppExtension::PNT::drawLineTo() { g_lines++; g_lineColors[g_pen]++; }
RTX_RESULT cppExtension::PNT::set(int bar, float, BAR_POSITION) { h = (short)(40 + bar * 6); v = 0; return RTX_OK; }
void cppExtension::PNT::setDrawPosition() {}
void cppExtension::RCT::draw(short, COLOR, COLOR, DRAWTYPE, BRUSH_STYLE) { g_rects++; }
void cppExtension::RCT::drawText(const char* s, RTBOOL, RTBOOL) { g_text.push_back(s ? s : ""); }
void cppExtension::RCT::getPaneRect(RTBOOL) { left = 0; top = 0; right = 1200; bottom = 300; }
void cppExtension::RCT::set(short l, short t, short r, short b) { left = l; top = t; right = r; bottom = b; }
cppExtension::RTARRAY::RTARRAY(fARRAY f) : tempFloat(0), bFreeTempArray(0), pArray(nullptr), fArray(f), count(0) {}
float& cppExtension::RTARRAY::operator[](int i)
{
    if (pArray) { std::vector<float>* v = (std::vector<float>*)pArray; return (*v)[(size_t)i]; }
    std::vector<float>& v = g_f[(int)fArray]; if (v.size() <= (size_t)i) v.resize((size_t)i + 1); return v[(size_t)i];
}
cppExtension::RTARRAY::~RTARRAY() { if (pArray) delete (std::vector<float>*)pArray; }
cppExtension::RTARRAYI::RTARRAYI(iARRAY a) : tempULong(0), pArray(nullptr), iArray(a), count(0) {}
unsigned long& cppExtension::RTARRAYI::operator[](int i)
{
    if (pArray) { std::vector<unsigned long>* v = (std::vector<unsigned long>*)pArray; return (*v)[(size_t)i]; }
    if (iArray == barDateTime) { g_dt.assign(g_bars.begin(), g_bars.end()); return g_dt[(size_t)i]; }
    return tempULong;
}
cppExtension::RTARRAYI::~RTARRAYI() { if (pArray) delete (std::vector<unsigned long>*)pArray; }
cppExtension::RTTICKS::RTTICKS(RTDATE start)
{
    g_ttCalls++; g_ttLastStart = (long long)start; if (g_now - (long long)start > g_ttMaxBack) g_ttMaxBack = g_now - (long long)start;
    auto* d = new std::vector<unsigned long>(); auto* p = new std::vector<float>(); auto* b = new std::vector<float>(); auto* a = new std::vector<float>(); auto* s = new std::vector<unsigned long>();
    for (auto& k : g_ticks) if (k.t >= (long long)start && k.t <= g_now) { d->push_back((unsigned long)k.t); p->push_back(k.px); b->push_back(k.bid); a->push_back(k.ask); s->push_back((unsigned long)k.q); }
    dt = new RTARRAYI(fEmptyArray == 0 ? (iARRAY)0 : (iARRAY)0); dt->pArray = (ARRAYI*)d;
    price = new RTARRAY((fARRAY)0); price->pArray = (ARRAY*)p; bid = new RTARRAY((fARRAY)0); bid->pArray = (ARRAY*)b; ask = new RTARRAY((fARRAY)0); ask->pArray = (ARRAY*)a;
    size = new RTARRAYI((iARRAY)0); size->pArray = (ARRAYI*)s; count = (long)d->size(); pTickData = nullptr; pInstrument = nullptr;
}
cppExtension::RTTICKS::~RTTICKS() { delete dt; delete price; delete bid; delete ask; delete size; }
int g_timerId = -1; RTX_RESULT cppExtension::createTimer(int id, int, void*) { if (g_timerOk) g_timerId = id; return g_timerOk ? RTX_OK : RTX_FAIL; }
RTDATE cppExtension::currentDate() { return (RTDATE)g_now; }
RTX_RESULT cppExtension::destroyTimer(int) { return RTX_OK; }
long cppExtension::getBarCount() { return (long)g_bars.size(); }
int cppExtension::getIntegerValue(int) { return 0; }
struct tm* cppExtension::getLocaltime(RTDATE d, struct tm* t) { time_t x = (time_t)d; gmtime_r(&x, t); return t; }
int cppExtension::getParameterCount() { return 0; }
short cppExtension::getPixelsPerBar() { return 6; }
float cppExtension::getProperty(SYMBOL_PROPERTY_ID, const char*) { return 0.25f; }
char* cppExtension::getRootSymbol(char* b, const char*) { static char s[32]; strcpy(s, g_root.c_str()); if (b) strcpy(b, s); return s; }
int cppExtension::getSecondsPerBar() { return 180; }
char* cppExtension::getSymbol() { static char s[32]; strcpy(s, g_sym.c_str()); return s; }
int cppExtension::getTextWidth(const char* s, int) { return (int)strlen(s) * 6; }
RTX_RESULT cppExtension::getVisibleBars(int* a, int* b) { *a = g_bars.size() > 160 ? (int)g_bars.size() - 160 : 0; *b = (int)g_bars.size() - 1; return RTX_OK; }
void cppExtension::setArrayCount(int) {}
void cppExtension::setDescription(const char*) {}
void cppExtension::setExtendedFlags(unsigned long) {}
void cppExtension::setFlags(unsigned long) {}
RTX_RESULT cppExtension::setFont(FONT&) { return RTX_OK; }
RTX_RESULT cppExtension::setIntegerParameter(const char*, int, short, short) { return RTX_OK; }
RTX_RESULT cppExtension::setOutputParameter(const char*, int, CPEN*, COLOR, unsigned long) { return RTX_OK; }
void cppExtension::setParameterDialogHeight(int, int) {}
RTX_RESULT cppExtension::setParameterVersion(unsigned) { return RTX_OK; }
void cppExtension::setPen(COLOR c, short, PEN_STYLE) { g_pen = (unsigned long)c; }
void cppExtension::setTextColor(COLOR) {}
void cppExtension::setVersion(const char*) {}
// (1.1.5) added for TapeFlow 1.1.5
long g_invalidates = 0;
RTX_RESULT cppExtension::setListParameter(const char*, short, const char*, short, short) { return RTX_OK; }
RTX_RESULT cppExtension::invalidateChart(RTBOOL) { g_invalidates++; return RTX_OK; }
std::vector<float> mockOut(int k) { return g_f[(int)fOut1 + k]; }
// (2.0.0) the chart's OHLC arrays for the signals (bar i)
void mockBar(int i, float o, float h, float l, float c) { auto put = [&](int a, float v) { std::vector<float>& x = g_f[a]; if (x.size() <= (size_t)i) x.resize((size_t)i + 1); x[(size_t)i] = v; }; put((int)barOpen, o); put((int)barHigh, h); put((int)barLow, l); put((int)barClose, c); }
