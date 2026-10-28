// dp_mock_cols.h -- (CR5200) the SDK calls lsDealerProfile 2.11.0 adds, for the mocked hosts of the Dealer Profile tests
// (included AFTER a test's own cppExtension definitions; the real irtsdk.h declares them). Chart options are per mocked chart key.
#ifndef DP_MOCK_COLS_H
#define DP_MOCK_COLS_H
#include <map>
#include <string>
#include <cstring>
struct MockOpts { int margin = 0, ppb = 5, sets = 0; bool fail = false; };
static std::string MOCK_CHART = "ES 3m";
static std::map<std::string, MockOpts> MOCK_OPTS;              // by chart label
static int MOCK_INT[32]; static bool MOCK_CHK[32]; static std::string MOCK_TXT[32]; static int MOCK_LIST[32];
static MockOpts& mockOpt() { return MOCK_OPTS[MOCK_CHART]; }
char* cppExtension::getChartLabel(void) { return (char*)MOCK_CHART.c_str(); }
static COLOR MOCK_BG = 0x00141821;   // (DP 2.36.1) the chart background colour the mocked host reports
COLOR cppExtension::getBackgroundColor(void) { return MOCK_BG; }
RTX_RESULT cppExtension::getChartOption(CHART_OPTION w, int* v)
{
    MockOpts& o = mockOpt(); if (o.fail || !v) return RTX_FAIL;
    if (w == OPT_RIGHT_MARGIN) { *v = o.margin; return RTX_OK; }
    if (w == OPT_PIXELS_PER_BAR) { *v = o.ppb; return RTX_OK; }
    return RTX_FAIL;
}
RTX_RESULT cppExtension::setChartOption(CHART_OPTION w, int v)
{
    MockOpts& o = mockOpt(); if (o.fail) return RTX_FAIL;
    if (w == OPT_RIGHT_MARGIN) { o.margin = v; ++o.sets; return RTX_OK; }
    return RTX_FAIL;
}
RTX_RESULT cppExtension::setListIndex(int i, int idx) { if (i >= 0 && i < 32) MOCK_LIST[i] = idx; return RTX_OK; }
void cppExtension::setIntegerValue(int i, int v) { if (i >= 0 && i < 32) MOCK_INT[i] = v; }
int cppExtension::getIntegerValue(int i) { return i >= 0 && i < 32 ? MOCK_INT[i] : 0; }
RTX_RESULT cppExtension::checkBox(int i, RTBOOL b) { if (i >= 0 && i < 32) MOCK_CHK[i] = b != 0; return RTX_OK; }
RTBOOL cppExtension::isBoxChecked(int i) { return i >= 0 && i < 32 && MOCK_CHK[i]; }
RTX_RESULT cppExtension::setParameterText(int i, const char* t) { if (i >= 0 && i < 32) MOCK_TXT[i] = t ? t : ""; return RTX_OK; }
RTX_RESULT cppExtension::getParameterText(int i, char* t, size_t n) { if (i < 0 || i >= 32 || !t || !n) return RTX_FAIL; std::strncpy(t, MOCK_TXT[i].c_str(), n - 1); t[n - 1] = 0; return RTX_OK; }
#endif
