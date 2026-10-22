// DealerHedgeSettings.h  (lsDealerHedge 0.1.1, 2026-10-22)  -  the ONE table of the oH settings window (controls only, no how-to).
// File: lsFlexLevels\DealerHedge.settings-<MKT>.txt (keyed, per market). Show / hide + colours for what it draws, the zone width and the
// detection thresholds Rassul can change (the rest are PARAM rows of lsFlexLevels\LRA-DealerHedge-<MKT>.csv from the nightly test).
// A value set here wins over the params file for the same number.
#pragma once
#include "SettingsSpec.h"

namespace dhset {
static const char* const PLUGIN_VERSION = "0.2.0";
enum K { K_SHOW_ZONES, K_BUY_COLOR, K_SELL_COLOR, K_FAINT_PCT, K_PULSE, K_SHOW_LABELS, K_SHOW_HOVER, K_WIDTH_ATR, K_FLOW_X, K_REP_N, K_REF_N, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show_zones", "oH zones", true)),
        ss::same(ss::in(ss::colorRow("buy_color", "Buying", 0x0022C55E, 0), "show_zones")),
        ss::same(ss::in(ss::colorRow("sell_color", "Selling", 0x00EF4444, 0), "show_zones")),
        ss::in(ss::intRow("faint_pct", "Settled band strength %", 30, 10, 60), "show_zones"),
        ss::same(ss::in(ss::boolRow("pulse", "Pulse while live", true), "show_zones")),
        ss::boolRow("show_labels", "oH labels below the bar", true),
        ss::same(ss::boolRow("show_hover", "Hover", true)),
        ss::floatRow("width_atr", "Zone width (x ATR each side)", 0.5, 0.1, 2.0),
        ss::floatRow("flow_x", "Flow vs normal (x)", 2.0, 1.0, 10.0),
        ss::intRow("rep_n", "Same-size repeats", 4, 2, 20),
        ss::same(ss::intRow("ref_n", "Refills", 4, 2, 20)),
    };
    static const ss::Spec S = { "DealerHedge", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
} // namespace dhset
