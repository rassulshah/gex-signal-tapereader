/********************************************************************************
 *  FirstExtreme.cpp  --  Investor/RT RTX extension  lsFirstExtreme  2.0.0 (2026-10-12)  RETIRED
 *
 *  The 1st Extreme model now runs INSIDE the HOD / LOD indicator (lsHodLod 2.0.0; Rassul 2026-10-12 21:47 "combine the hod lod
 *  indicator and the first extreme. it should just be the hod lod indicator"). This stub draws one grey line asking for the indicator
 *  to be removed from the chart and does nothing else: no model, no files (lsHodLod writes FirstExtreme.decisions / calls / status
 *  now - two writers would duplicate the shadow log). The 1.0.1 source is kept as plugin\retired\FirstExtreme_1_0_1.cpp.
 ********************************************************************************/
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "FirstExtreme"
#endif
#ifdef far
#undef far
#endif
#include <string>

static const COLOR FX_GREY = 0x009CA3AF;

class FirstExtreme : public cppExtension {
public:
    FirstExtreme() : cppExtension() {}
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
    CPEN p(FX_GREY, 1, P_SOLID);
    setOutputParameter("1st Extreme", DRAW_INVISIBLE, &p, FX_GREY, OUTPUT_NO_UI);
    setArrayDrawingFlags(0, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    return RTX_OK;
}

int FirstExtreme::draw(void)
{ LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    RCT pane; pane.getPaneRect(false);
    if (pane.right - pane.left > 300 && pane.bottom - pane.top > 40)
        text(pane.left + 8, pane.top + 14, "1st Extreme is now inside HOD/LOD - remove this indicator", FX_GREY, 8);
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    FirstExtreme *p = new FirstExtreme();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("RETIRED: the 1st Extreme model now runs inside the HOD / LOD indicator (lsHodLod 2.0.0). Remove this indicator from the chart.");
    p->setVersion("2.0.1");
    return p;
}
