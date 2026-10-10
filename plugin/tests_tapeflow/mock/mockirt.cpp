#include "shim.h"
#include "mockirt.h"
#include "irtsdk.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
long long g_now = 0; std::vector<MTick> g_ticks; std::vector<long long> g_bars; std::string g_root = "ES", g_sym = "ESZ26";
std::vector<std::string> g_text; long g_lines = 0, g_rects = 0, g_ttCalls = 0; long long g_ttLastStart = 0, g_ttMaxBack = 0; bool g_timerOk = true;
static std::map<int, std::map<int, std::vector<float>>> g_fAll; static int g_curChart = 0;   // (2.0.3) per chart
#define g_f g_fAll[g_curChart]                             // output + OHLC arrays of the current chart
static std::vector<unsigned long> g_dt;
cppExtension::cppExtension() {}
cppExtension::~cppExtension() {}
cppExtension::CPEN::CPEN(COLOR c, short w, PEN_STYLE s) : color(c), style(s), width(w) {}
cppExtension::FONT::FONT(FONT_ID i, short s, FONT_STYLE st) : id(i), size(s), style(st) {}
std::map<unsigned long, long> g_lineColors; static unsigned long g_pen = 0;   // (2.0.2) lines drawn per pen colour
std::vector<MSeg> g_segs; long g_dashLines = 0; static int g_style = 0; static short g_px = 0, g_py = 0;
void cppExtension::PNT::drawLineTo() { g_lines++; g_lineColors[g_pen]++; g_segs.push_back(MSeg{g_px, g_py, h, v, g_pen, g_style}); if (g_style == (int)P_DASH) g_dashLines++; g_px = h; g_py = v; }
std::vector<std::pair<int, float> > g_pntSets;   // (2.0.3) every PNT::set(bar, price) with a price (the Marks dash)
int g_ppb = 6;   // (2.0.3) pixels per bar (zoom)
float g_yOrigin = 5000.0f, g_yPxPerPt = 10.0f;   // (2.0.4) price -> y for overlays: y = 500 - (price - origin) * px/pt
RTX_RESULT cppExtension::PNT::set(int bar, float price, BAR_POSITION) { h = (short)(40 + bar * g_ppb); v = 0; if (price != 0.0f) { g_pntSets.push_back(std::make_pair(bar, price)); const double y = 500.0 - ((double)price - g_yOrigin) * g_yPxPerPt; v = (short)std::max(-30000.0, std::min(30000.0, y)); } return RTX_OK; }
void cppExtension::PNT::setDrawPosition() { g_px = h; g_py = v; }
std::vector<MRect> g_rectList;   // (2.0.3) the drawn rectangles
void cppExtension::RCT::draw(short, COLOR, COLOR f, DRAWTYPE, BRUSH_STYLE) { g_rects++; g_rectList.push_back(MRect{left, top, right, bottom, (unsigned long)f}); }
std::vector<std::pair<std::string, short> > g_textAt;   // (2.0.3) text with its left x
static unsigned long g_textColor = 0;
bool g_ticksThrow = false;
std::vector<MText> g_textRects;   // (2.0.4) text with its full rectangle
void cppExtension::RCT::drawText(const char* s, RTBOOL, RTBOOL) { g_text.push_back(s ? s : ""); g_textAt.push_back(std::make_pair(std::string(s ? s : ""), left)); g_textRects.push_back(MText{s ? s : "", left, top, right, bottom, g_textColor}); }
int g_paneH = 300;   // (2.0.3) the pane height (small-pane tests)
void cppExtension::RCT::getPaneRect(RTBOOL) { left = 0; top = 0; right = 1200; bottom = (short)g_paneH; }
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
    g_ttCalls++; g_ttLastStart = (long long)start;
    if (g_now - (long long)start > g_ttMaxBack) g_ttMaxBack = g_now - (long long)start;
    if (g_ticksThrow) throw std::runtime_error("mock: IRT tick request failed");   // (2.0.4) fault injection (quarantine test)
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
short cppExtension::getPixelsPerBar() { return (short)g_ppb; }
float g_tickIncr = 0.25f;   // (2.0.3) SYM_TICKINCR of the chart
float cppExtension::getProperty(SYMBOL_PROPERTY_ID, const char*) { return g_tickIncr; }
char* cppExtension::getRootSymbol(char* b, const char*) { static char s[32]; strcpy(s, g_root.c_str()); if (b) strcpy(b, s); return s; }
static std::map<int, int> g_spbAll;
int cppExtension::getSecondsPerBar() { auto it = g_spbAll.find(g_curChart); return it == g_spbAll.end() ? 180 : it->second; }
void mockSetSpb(int s) { g_spbAll[g_curChart] = s; }
static std::map<int, std::vector<long long> > g_barsAll;
void mockSelectChart(int id) { if (id == g_curChart) return; g_barsAll[g_curChart].swap(g_bars); g_curChart = id; g_bars.swap(g_barsAll[id]); g_barsAll.erase(id); }
int mockCurrentChart() { return g_curChart; }
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
void cppExtension::setPen(COLOR c, short, PEN_STYLE s) { g_pen = (unsigned long)c; g_style = (int)s; }
void cppExtension::setTextColor(COLOR c) { g_textColor = (unsigned long)c; }
void cppExtension::setVersion(const char*) {}
// (1.1.5) added for TapeFlow 1.1.5
long g_invalidates = 0;
RTX_RESULT cppExtension::setListParameter(const char*, short, const char*, short, short) { return RTX_OK; }
RTX_RESULT cppExtension::invalidateChart(RTBOOL) { g_invalidates++; return RTX_OK; }
std::vector<float> mockOut(int k) { return g_f[(int)fOut1 + k]; }
// (2.0.0) the chart's OHLC arrays for the signals (bar i)
void mockBar(int i, float o, float h, float l, float c) { auto put = [&](int a, float v) { std::vector<float>& x = g_f[a]; if (x.size() <= (size_t)i) x.resize((size_t)i + 1); x[(size_t)i] = v; }; put((int)barOpen, o); put((int)barHigh, h); put((int)barLow, l); put((int)barClose, c); }
// (2.0.3) the per-chart user-data slot (HostSlot) for TapeFlowMarks
static std::map<std::pair<const void*, int>, void*> g_ud;   // (2.0.3) keyed by object AND chart: one object can serve two charts
void* cppExtension::getUserData() { auto it = g_ud.find(std::make_pair((const void*)this, g_curChart)); return it == g_ud.end() ? nullptr : it->second; }
void cppExtension::setUserData(void* p) { g_ud[std::make_pair((const void*)this, g_curChart)] = p; }
