#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "DealerSummary.cpp"

struct Host {
    std::string root;
    int market = 0;
    int integer[6] = {0, 10, 300, 0, 30, 230};
    void* userData = NULL;
};

static Host hosts[6];
static int activeHost = 0;
static short paneLeft = 0, paneTop = 0, paneRight = 700, paneBottom = 500;
static std::vector<short> fontSizes;
static std::vector<std::pair<short, short> > textRects;
static int fillCount = 0;
static unsigned long extendedFlags = 0;

cppExtension::cppExtension() {}
cppExtension::~cppExtension() {}
cppExtension::FONT::FONT(FONT_ID i, short s, FONT_STYLE st) : id(i), size(s), style(st) {}

void cppExtension::setArrayCount(int) {}
void cppExtension::setFlags(unsigned long) {}
void cppExtension::setExtendedFlags(unsigned long flags) { extendedFlags = flags; }
void cppExtension::setDescription(const char*) {}
void cppExtension::setVersion(const char*) {}
void cppExtension::setParameterDialogHeight(int, int) {}
RTX_RESULT cppExtension::setParameterVersion(unsigned) { return RTX_OK; }
int cppExtension::getParameterCount(void) { return 0; }
RTX_RESULT cppExtension::setListParameter(const char*, short, const char*, short, short) { return RTX_OK; }
RTX_RESULT cppExtension::setIntegerParameter(const char*, int, short, short) { return RTX_OK; }
int cppExtension::getListIndex(int) { return hosts[activeHost].market; }
int cppExtension::getIntegerValue(int index) { return index >= 0 && index < 6 ? hosts[activeHost].integer[index] : 0; }
long cppExtension::getBarCount(void) { return 0; }

char* cppExtension::getRootSymbol(char* rootSymbol, const char*)
{
    if (!rootSymbol) return NULL;
    std::strncpy(rootSymbol, hosts[activeHost].root.c_str(), 31);
    rootSymbol[31] = '\0';
    return rootSymbol;
}
void* cppExtension::getUserData(void) { return hosts[activeHost].userData; }
void cppExtension::setUserData(void* p) { hosts[activeHost].userData = p; }
void cppExtension::setPen(COLOR, short, PEN_STYLE) {}
void cppExtension::setTextColor(COLOR) {}
RTX_RESULT cppExtension::setFont(FONT& f) { fontSizes.push_back(f.size); return RTX_OK; }
int cppExtension::getTextWidth(const char* s, int len)
{
    if (!s) return 0;
    const int n = len < 0 ? (int)std::strlen(s) : len;
    return n * 7;
}

RTX_RESULT cppExtension::PNT::set(int, float, BAR_POSITION) { return RTX_OK; }
void cppExtension::PNT::setDrawPosition() {}
void cppExtension::PNT::drawLineTo() {}
void cppExtension::RCT::getPaneRect(RTBOOL)
{
    left = paneLeft; top = paneTop; right = paneRight; bottom = paneBottom;
}
void cppExtension::RCT::set(short l, short t, short r, short b)
{
    left = l; top = t; right = r; bottom = b;
}
void cppExtension::RCT::draw(short, COLOR, COLOR, DRAWTYPE, BRUSH_STYLE) { ++fillCount; }
void cppExtension::RCT::drawText(const char*, RTBOOL, RTBOOL) { textRects.push_back(std::make_pair(left, right)); }

static const std::string base = "/tmp/DealerSummaryRegression";

static std::string filePath(const std::string& market)
{
    return base + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Summary-" + market + ".txt";
}

static std::string statusPath()
{
    return base + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerSummary.status.txt";
}

static void writeFile(const std::string& path, const std::string& text)
{
    std::ofstream f(path.c_str(), std::ios::binary | std::ios::trunc);
    assert(f.is_open());
    f << text;
    assert((bool)f);
}

static std::string readFile(const std::string& path)
{
    std::ifstream f(path.c_str(), std::ios::binary);
    assert(f.is_open());
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static std::string summary(const std::string& head, const std::string& body, bool timestamp = true)
{
    std::string text = "VERSION|1.0\n";
    if (timestamp) text += "ASOF|12:00:00 CT|2026-10-08|" + std::to_string((long long)std::time(NULL)) + "\n";
    return text + "HEAD|" + head + "|#ffffff\nBODY|" + body + "\n";
}

static void setSettings(int host, const std::string& root, int font, int top = 30)
{
    hosts[host].root = root;
    hosts[host].market = 0;
    hosts[host].integer[0] = 0;
    hosts[host].integer[1] = font;
    hosts[host].integer[2] = 300;
    hosts[host].integer[3] = 0;
    hosts[host].integer[4] = top;
    hosts[host].integer[5] = 30;
}

int main()
{
    assert(setenv("USERPROFILE", base.c_str(), 1) == 0);
    std::remove(statusPath().c_str());
    for (int i = 0; i < 6; ++i) std::remove(filePath(i == 0 ? "ES" : i == 1 ? "NQ" : i == 2 ? "CL" : i == 3 ? "GC" : i == 4 ? "HG" : "NG").c_str());
    YX.market = 0; YX.font = 1; YX.width = 2; YX.moveX = 3; YX.top = 4; YX.bottom = 5;
    DealerSummary indicator;

    // Missing ASOF previously looked live. It must now render/status as old and unknown.
    setSettings(0, "EP", 10);
    writeFile(filePath("ES"), summary("PLAN", "read without a timestamp", false));
    activeHost = 0;
    indicator.parmsApply();
    textRects.clear();
    indicator.draw();
    assert(readFile(statusPath()).find("STATE,drawn (old read)") != std::string::npos);
    assert(readFile(statusPath()).find("AGE_MIN,-1") != std::string::npos);
    assert(((DSState*)hosts[0].userData)->sm.ok);
    indicator.done();

    // The one DLL instance must keep callbacks, parsed reads, and caches isolated per host chart.
    setSettings(0, "EP", 9);
    setSettings(1, "NQ", 22);
    writeFile(filePath("ES"), summary("ES HEAD", "ES body"));
    writeFile(filePath("NQ"), summary("NQ HEAD", "NQ body"));
    activeHost = 0; indicator.parmsApply(); indicator.draw();
    activeHost = 1; indicator.parmsApply(); indicator.draw();
    DSState* esState = (DSState*)hosts[0].userData;
    DSState* nqState = (DSState*)hosts[1].userData;
    assert(esState && nqState && esState != nqState);
    assert(esState->cfg.font == 9 && nqState->cfg.font == 22);
    assert(esState->mkt == "ES" && nqState->mkt == "NQ");
    assert(esState->sm.head == "ES HEAD" && nqState->sm.head == "NQ HEAD");
    activeHost = 0; indicator.draw();
    assert(((DSState*)hosts[0].userData)->cfg.font == 9);
    assert(((DSState*)hosts[0].userData)->sm.head == "ES HEAD");

    // An oversized external file must be rejected before it is parsed/wrapped/drawn.
    setSettings(2, "CL", 10);
    writeFile(filePath("CL"), std::string(MAX_SUMMARY_BYTES + 1, 'x'));
    activeHost = 2; indicator.parmsApply();
    const int fillsBeforeLarge = fillCount;
    indicator.draw();
    assert(!((DSState*)hosts[2].userData)->sm.ok);
    assert(fillCount == fillsBeforeLarge);
    assert(readFile(statusPath()).find("STATE,summary file exceeds 64 KiB") != std::string::npos);

    // A valid read with an impossible top offset must not produce an inverted/off-pane rectangle.
    setSettings(3, "GC", 10, 2000);
    writeFile(filePath("GC"), summary("GC HEAD", "GC body"));
    activeHost = 3; indicator.parmsApply();
    const int fillsBeforeShort = fillCount;
    indicator.draw();
    assert(fillCount == fillsBeforeShort);
    assert(readFile(statusPath()).find("STATE,pane too short") != std::string::npos);

    // A long unbroken external token must be ellipsized before the SDK text rectangle is created.
    setSettings(4, "HG", 10);
    writeFile(filePath("HG"), summary("HG HEAD", std::string(3000, 'X')));
    activeHost = 4; indicator.parmsApply();
    textRects.clear();
    indicator.draw();
    assert(!textRects.empty());
    for (size_t i = 0; i < textRects.size(); ++i) {
        assert(textRects[i].first >= paneLeft);
        assert(textRects[i].second <= paneRight);
    }

    // Inactive feeds must still invoke draw so the ten-minute staleness promise can take effect.
    cppExtension* created = CreateExtension();
    assert((extendedFlags & CALL_CONTINUOUSLY) != 0);
    delete static_cast<DealerSummary*>(created);

    for (int i = 0; i < 5; ++i) { activeHost = i; indicator.destroy(); assert(hosts[i].userData == NULL); }
    std::puts("DealerSummary regression tests: 6 passed");
    return 0;
}
