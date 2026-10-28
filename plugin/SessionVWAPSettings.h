// SessionVWAPSettings.h  (lsSessionVWAP 1.6.1, 2026-10-18 settings audit)  -  the ONE table of the Session VWAP settings window.
// Same 15 rows in the same order as svs::Row (parameter version 6 kept); the per-market file keeps its format (SessionVWAPStyle.h).
// New: each line's show box greys its colour (and the VWAP width) while it is off.
#pragma once
#include "SettingsSpec.h"

#ifndef SVS_DEF_ON   // (1.8.3) = SessionVWAPStyle.h: false in the plugin (only the pivot VWAP on); old test suites: all-on
#ifdef SVS_TEST_ALL_ON_DEFAULTS
#define SVS_DEF_ON true
#else
#define SVS_DEF_ON false
#endif
#endif
#ifndef SVS_PIV_DEF   // (1.8.4) = SessionVWAPStyle.h: the pivot VWAP default - false in the plugin; old suites (all-on): true
#ifdef SVS_TEST_ALL_ON_DEFAULTS
#define SVS_PIV_DEF true
#else
#define SVS_PIV_DEF false
#endif
#endif
namespace svset {
static const char* const PLUGIN_VERSION = "1.8.5";   // (1.8.5) Key Levels column keep-out (drawing only; parameter version stays 11)
// was 1.8.4:   // (1.8.4, 2026-10-28 16:26 CT Rassul "the chart is extremely messy", remove the Pivot VWAP line) parameter version 11: every older set is reset once to the new defaults = NO line drawn (pivot VWAP off too); he can tick any line back
// was "1.8.3";   // (1.8.3, 2026-10-28 14:08 CT, Rassul: only 1 VWAP drawn = the 3-bar pivot VWAP) parameter version 10: every older set is reset once to the new defaults (pivot VWAP on; session VWAP, bands, overnight, prior off)
//   // (1.8.2, 2026-10-28, Rassul 11:27 CT "did you fix the session vwap. as you can see it
// isnt working properly") parameter version 9 (rows unchanged): every older saved set is replaced by the intended defaults (parmsInit)
// (1.8.0, 2026-10-27) + "3-bar pivot VWAP" (appended row, parameter version 7)
// (1.8.1, 2026-10-27 22:50 CT, Rassul "the settings never work") + the pivot VWAP's above / below colour and width (appended rows,
// parameter version 8); the chart now reads these rows from IRT's own per-indicator parameters (svp:: in SessionVWAP.cpp)
static const char* const WIDTHS = "1 px;2 px;3 px;4 px";
// the order of svs::Row (R_VWAP .. R_PRIOR_COL)
enum K { K_VWAP, K_VWAP_COLOR, K_VWAP_WIDTH, K_P1, K_P1_COLOR, K_M1, K_M1_COLOR, K_P2, K_P2_COLOR, K_M2, K_M2_COLOR, K_BAND_WIDTH,
         K_OVERNIGHT, K_PRIOR, K_PRIOR_COLOR, K_PIVOT, K_PIV_UP, K_PIV_DN, K_PIV_WIDTH, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("vwap", "VWAP", SVS_DEF_ON)),
        ss::same(ss::in(ss::colorRow("vwap_color", "VWAP colour", 0xFF80FF), "vwap")),
        ss::same(ss::in(ss::listRow("vwap_width", "VWAP width", 1, WIDTHS, 70), "vwap")),
        ss::masterOf(ss::boolRow("p1", "+1 SD band", SVS_DEF_ON)),
        ss::same(ss::in(ss::colorRow("p1_color", "+1 SD colour", 0x008000), "p1")),
        ss::masterOf(ss::boolRow("m1", "-1 SD band", SVS_DEF_ON)),
        ss::same(ss::in(ss::colorRow("m1_color", "-1 SD colour", 0xFF8080), "m1")),
        ss::masterOf(ss::boolRow("p2", "+2 SD band", SVS_DEF_ON)),
        ss::same(ss::in(ss::colorRow("p2_color", "+2 SD colour", 0x00FF00), "p2")),
        ss::masterOf(ss::boolRow("m2", "-2 SD band", SVS_DEF_ON)),
        ss::same(ss::in(ss::colorRow("m2_color", "-2 SD colour", 0xFF0000), "m2")),
        ss::listRow("band_width", "Band width", 0, WIDTHS, 70),
        ss::boolRow("overnight", "Overnight VWAP + bands", SVS_DEF_ON),
        ss::masterOf(ss::boolRow("prior", "Prior RTH VWAP (overnight)", SVS_DEF_ON)),
        ss::same(ss::in(ss::colorRow("prior_color", "Prior RTH VWAP colour", 0x6B7280), "prior")),
        ss::masterOf(ss::boolRow("pivot", "3-bar pivot VWAP", SVS_PIV_DEF)),   // (1.8.4) default off   // (1.8.0, Rassul 2026-10-27 09:46 CT)
        ss::same(ss::in(ss::colorRow("piv_up", "Above", 0x22C55E), "pivot")),   // (1.8.1) colour while the bar closed above it
        ss::same(ss::in(ss::colorRow("piv_dn", "Below", 0xEF4444), "pivot")),   // (1.8.1) colour while the bar closed below it
        ss::in(ss::listRow("piv_width", "Pivot VWAP width", 1, WIDTHS, 70), "pivot"),   // (1.8.1) default 2 px (own line: the pivot line is full)
    };
    static const ss::Spec S = { "SessionVWAP", PLUGIN_VERSION, 11, R, K_N, 0 };   // (1.8.4) 11: reset once to all-off   // (1.8.3) 10: reset to the pivot-only defaults   // (1.8.0) 7: + the pivot row; (1.8.1) 8: + colours, width; (1.8.2) 9: reset to the defaults
    return S;
}
} // namespace svset
