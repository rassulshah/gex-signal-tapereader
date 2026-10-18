/********************************************************************************
 *  HotReloadDemoImpl.cpp  --  Investor/RT RTX extension  lsHotReloadDemo (implementation build)  1.0.1 (2026-10-18)  TEST ONLY
 *  1.0.1 (17:21 CT): the box moves BELOW the Liquidity Profile read at the top left (it covered the REV / ABS headline) - the 2nd hot-reload test
 *
 *  Rassul 2026-10-18 15:17 CT: "is there a way for you to do the build without me having to restart every time, maybe i can just
 *  refresh the chart instead?"  -- this tiny indicator proves the hot-reload shim on his IRT before any real indicator is converted.
 *
 *  Draws a box at the top left: "Hot Reload Demo v<version>  build <n>  calcs <k>" and the shim's status line, plus one blue line at
 *  the last close. The test: change DEMO_VERSION, the build publishes a new implementation, he refreshes the chart, the version and
 *  build in the box change without restarting IRT; "calcs" restarting from 1 shows the chart's state was rebuilt on the new build.
 *
 *  It is a normal RTX build (links irtsdkV143-x64.lib, exports cpp_main) that IRT never loads directly: gex-hot-build.bat compiles it
 *  with /DHR_BUILD=<n> and copies it to InvestorRT\hotreload\lsHotReloadDemo_impl_<n>.dll; the shim lsHotReloadDemo.dll forwards to it.
 *  No settings (one hidden output, like Session Info). Remove it from the chart when the test is done.
 *  Settings contract (plugin\tests_settings\settings_contract.cpp, 2026-10-18): EXEMPT - it has no user settings (no window rows to
 *  register, save or read), it is a temporary test indicator, and it is not in SETSRC; the contract still gates its build.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include <string>
#include "HotImpl.h"
#include "HotReloadDemoLogic.h"
#include "HostSlot.h"

#define DEMO_VERSION "1.0.1"                 // keep equal to the setVersion literal below
#define DEMO_ARRAYS 1
#define DEMO_FLAGS (POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE)
#define DEMO_FLAGS_EXT 0
#define DEMO_PARAMS "v1|output:Hot Reload Demo"   // the settings setup() declares, in order (the shim compares builds by this)

HR_IMPL_EXPORTS("lsHotReloadDemo", DEMO_VERSION, DEMO_PARAMS, DEMO_ARRAYS, DEMO_FLAGS, DEMO_FLAGS_EXT)

static const COLOR DEMO_WHITE = 0x00FFFFFF;
static const COLOR DEMO_BLUE = 0x00F6A23B;   // RGB(59, 162, 246)

struct DemoState { long calcs = 0; int lastBar = -1; float lastClose = 0.0f; };
static HostSlot<DemoState> g_slot;

class HotReloadDemo : public cppExtension {
public:
    HotReloadDemo() : cppExtension() {}
    virtual int draw(void);
    void text(int x, int yc, const std::string& s, COLOR col, int sz)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(yc - (short)(sz * 0.45f + 0.5f)); int w = (int)getTextWidth(s.c_str(), -1);
        RCT rc; rc.set((short)x, (short)(yy - sz), (short)(x + w + 4), (short)(yy + sz)); rc.drawText(s.c_str(), false, false);
    }
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void)
{
    setParameterVersion(1);
    CPEN p(DEMO_BLUE, 1, P_SOLID);
    setOutputParameter("Hot Reload Demo", DRAW_INVISIBLE, &p, DEMO_BLUE, OUTPUT_NO_UI);
    setArrayDrawingFlags(0, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    return RTX_OK;
}
int cppExtension::calc(int)
{
    DemoState* s = g_slot.get(this, true);
    if (!s) return RTX_OK;
    long n = getBarCount();
    if (n > 0) {
        RTARRAY cl(barClose);
        float c = cl[(int)(n - 1)];
        if (c == c && c > 0.0f) { s->lastBar = (int)(n - 1); s->lastClose = c; }
    }
    s->calcs++;
    return RTX_OK;
}
int cppExtension::done(void) { return RTX_OK; }
int cppExtension::destroy(void) { g_slot.release(this); return RTX_OK; }

int HotReloadDemo::draw(void)
{
    DemoState* s = g_slot.get(this, false);
    RCT pane; pane.getPaneRect(false);
    if (pane.right - pane.left < hrdemo::BOX_W + 20 || pane.bottom - pane.top < hrdemo::TOP_Y + hrdemo::BOX_H + 20) return RTX_OK;
    if (s && s->lastBar >= 0) {
        PNT a;
        if (a.set(s->lastBar, s->lastClose) == RTX_OK && a.v > pane.top && a.v < pane.bottom) {
            CPEN pen(DEMO_BLUE, 2, P_SOLID); pen.set();
            PNT l; l.h = pane.left; l.v = a.v; l.setDrawPosition();
            PNT r; r.h = pane.right; r.v = a.v; r.drawLineTo();
        }
    }
    int x = pane.left + 8, y = pane.top + hrdemo::TOP_Y;            // (1.0.1) under the Liquidity Profile read block (was +24: on top of it)
    RCT box; box.set((short)x, (short)y, (short)(x + hrdemo::BOX_W), (short)(y + hrdemo::BOX_H));
    box.draw(1, DEMO_WHITE, getBackgroundColor());
    text(x + 8, y + 15, hrdemo::title(DEMO_VERSION, hotreload::implBuild(), s ? s->calcs : 0), DEMO_WHITE, 11);
    text(x + 8, y + 34, hrdemo::statusLine(hotreload::shimStatus()), DEMO_WHITE, 10);
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void)
{
    HotReloadDemo* p = new HotReloadDemo();
    p->setArrayCount(DEMO_ARRAYS);
    p->setFlags(DEMO_FLAGS);
    p->setDescription("TEST ONLY: proves hot reload (a new build loads on a chart refresh, no IRT restart). Shows its version, build and the shim status. Remove after the test.");
    p->setVersion("1.0.1");   // DEMO_VERSION
    return p;
}
