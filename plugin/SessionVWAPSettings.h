// SessionVWAPSettings.h  (lsSessionVWAP 1.6.1, 2026-10-18 settings audit)  -  the ONE table of the Session VWAP settings window.
// Same 15 rows in the same order as svs::Row (parameter version 6 kept); the per-market file keeps its format (SessionVWAPStyle.h).
// New: each line's show box greys its colour (and the VWAP width) while it is off.
#pragma once
#include "SettingsSpec.h"

namespace svset {
static const char* const PLUGIN_VERSION = "1.8.0";   // (1.8.0, 2026-10-27) + "3-bar pivot VWAP" (appended row, parameter version 7)
static const char* const WIDTHS = "1 px;2 px;3 px;4 px";
// the order of svs::Row (R_VWAP .. R_PRIOR_COL)
enum K { K_VWAP, K_VWAP_COLOR, K_VWAP_WIDTH, K_P1, K_P1_COLOR, K_M1, K_M1_COLOR, K_P2, K_P2_COLOR, K_M2, K_M2_COLOR, K_BAND_WIDTH,
         K_OVERNIGHT, K_PRIOR, K_PRIOR_COLOR, K_PIVOT, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("vwap", "VWAP", true)),
        ss::same(ss::in(ss::colorRow("vwap_color", "VWAP colour", 0xFF80FF), "vwap")),
        ss::same(ss::in(ss::listRow("vwap_width", "VWAP width", 1, WIDTHS, 70), "vwap")),
        ss::masterOf(ss::boolRow("p1", "+1 SD band", true)),
        ss::same(ss::in(ss::colorRow("p1_color", "+1 SD colour", 0x008000), "p1")),
        ss::masterOf(ss::boolRow("m1", "-1 SD band", true)),
        ss::same(ss::in(ss::colorRow("m1_color", "-1 SD colour", 0xFF8080), "m1")),
        ss::masterOf(ss::boolRow("p2", "+2 SD band", true)),
        ss::same(ss::in(ss::colorRow("p2_color", "+2 SD colour", 0x00FF00), "p2")),
        ss::masterOf(ss::boolRow("m2", "-2 SD band", true)),
        ss::same(ss::in(ss::colorRow("m2_color", "-2 SD colour", 0xFF0000), "m2")),
        ss::listRow("band_width", "Band width", 0, WIDTHS, 70),
        ss::boolRow("overnight", "Overnight VWAP + bands", true),
        ss::masterOf(ss::boolRow("prior", "Prior RTH VWAP (overnight)", true)),
        ss::same(ss::in(ss::colorRow("prior_color", "Prior RTH VWAP colour", 0x6B7280), "prior")),
        ss::boolRow("pivot", "3-bar pivot VWAP", true),   // (1.8.0, Rassul 2026-10-27 09:46 CT)
    };
    static const ss::Spec S = { "SessionVWAP", PLUGIN_VERSION, 7, R, K_N, 0 };   // (1.8.0) 7: + the pivot row
    return S;
}
} // namespace svset
