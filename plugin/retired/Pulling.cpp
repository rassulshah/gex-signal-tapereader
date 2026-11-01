/********************************************************************************
 *  Pulling.cpp  --  Investor/RT RTX extension  lsPulling  1.0.0 (2026-10-14 night)  RETIRED
 *
 *  lsPulling's read now runs INSIDE lsPullStats 1.5.0 (Rassul approved 20:04 CT 2026-10-14: "merge the Pulling read into PullStats and
 *  retire lsPulling"; design the same night: "the PullStats line shows ONLY ... the locked arrival read after the 3-min window"). The
 *  arrival read - in the first 3 minutes after price reaches a key / gamma level, was the size resting at the level cancelled or traded
 *  - is drawn by lsPullStats from ZoneRead.h 1.1.0. lsPulling was never added to any of his charts.
 *  This stub keeps the DLL name so a chart that still lists the indicator loads cleanly; it draws nothing, has no settings, opens no
 *  order feed and writes no files (no MBO recorder, no record, no status - lsPullStats records the order feed). The 0.3.0 source is kept
 *  as plugin\retired\Pulling_0_3_0.cpp; PullingLogic.h stays in plugin\ unused (lsPullStats does not include it).
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

#define PL_VERSION "1.0.0"          // RETIRED stub; keep equal to the setVersion literal below

class Pulling : public cppExtension {
public:
    Pulling() : cppExtension() {}
    virtual int draw(void) { return RTX_OK; }                         // draws nothing
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::setup(void)   { setParameterVersion(1); return RTX_OK; }   // no settings (0.3.0 had none either)

extern "C" cppExtension* CreateExtension(void) {
    Pulling* p = new Pulling();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI);
    p->setDescription("RETIRED on 2026-10-14: lsPulling's read (was the size resting at a key / gamma level cancelled or traded in the 3 minutes after price arrived) is now the read line of lsPullStats 1.5.0. This indicator draws nothing - remove it from the chart and use PullStats.");
    p->setVersion("1.0.0");   // PL_VERSION
    return p;
}
