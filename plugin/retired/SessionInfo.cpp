/********************************************************************************
 *  SessionInfo.cpp  --  Investor/RT RTX extension  lsSessionInfo  2.0.0 (2026-10-13)  RETIRED
 *
 *  Session Info is retired (Rassul 2026-10-13 06:08 "retire session info"): its Regime and options Expiry are shown in the HOD / LOD
 *  table's header (lsHodLod 2.0.2, read from the same LRA-Session-<MKT>.csv). This stub draws one grey line asking for the
 *  indicator to be removed from the chart and does nothing else. The 1.7.2 source is kept as plugin\retired\SessionInfo_1_7_2.cpp.
 ********************************************************************************/
// (2026-10-28) windows.h BEFORE irtsdk.h (Linnsoft note, irtsdk.h:123): PerfGuard.h pulls windows.h after it,
// which broke the build on Windows SDK 10.0.26100 (C2371 RTBOOL). Same pattern as CVD / the plugins that compile.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "SessionInfo"
#endif
#ifdef far
#undef far
#endif
#include <string>

static const COLOR SI_GREY = 0x009CA3AF;

class SessionInfo : public cppExtension {
public:
    SessionInfo() : cppExtension() {}
    virtual int draw(void);
    void text(int x, int yc, const std::string& s, COLOR col, int sz)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(yc - (short)(sz * 0.45f + 0.5f)); int w = (int)getTextWidth(s.c_str(), -1);
        RCT rc; rc.set((short)x, (short)(yy - sz), (short)(x + w + 4), (short)(yy + sz)); rc.drawText(s.c_str(), false, false);
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::setup(void)
{
    setParameterVersion(2);
    CPEN p(SI_GREY, 1, P_SOLID);
    setOutputParameter("Session Info", DRAW_INVISIBLE, &p, SI_GREY, OUTPUT_NO_UI);
    setArrayDrawingFlags(0, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    return RTX_OK;
}

int SessionInfo::draw(void)
{ LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    RCT pane; pane.getPaneRect(false);
    if (pane.right - pane.left > 300 && pane.bottom - pane.top > 40)
        text(pane.left + 8, pane.top + 14, "Session Info is now inside HOD/LOD - remove this indicator", SI_GREY, 8);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionInfo *p = new SessionInfo();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("RETIRED: the Regime and options Expiry are now in the HOD / LOD table header (lsHodLod 2.0.2). Remove this indicator from the chart.");
    p->setVersion("2.0.1");
    return p;
}
