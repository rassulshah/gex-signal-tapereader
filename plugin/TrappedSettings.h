// TrappedSettings.h (lsTrapped / lsTrappedMarks 1.0.0, 2026-10-24; Rassul 23:28 CT "istrapped is a go"): the ONE settings table of
// each build of Trapped.cpp. Controls only (no how-to text). Files: lsFlexLevels\Trapped.settings-<MKT>.txt (the pane) and
// lsFlexLevels\TrappedMarks.settings-<MKT>.txt (the Tr letters on the price chart) - keyed, per market. Parameter version 1.
// Muted (small-trap) shades are drawn as a darker shade of the chosen colour (no extra colour rows).
#pragma once
#include "SettingsSpec.h"
#ifndef TRP_SEP
#if defined(_WIN32)
#define TRP_SEP "\\"
#else
#define TRP_SEP "/"
#endif
#endif

namespace trpset {
static const char* const PLUGIN_VERSION = "1.0.0";
// the pane (lsTrapped)
enum K { K_VERSION, K_SHOW_FUEL, K_SHOW_LABELS, K_SHOW_MSG, K_BIG_X, K_COL_BULL, K_COL_BEAR, K_N };
inline const ss::Spec& paneSpec() {
    static const ss::Row R[K_N] = {
        ss::labelRow("version", "LRA Trapped v1.0.0", 200),
        ss::boolRow("show_fuel", "Show fuel-left line", true),
        ss::boolRow("show_labels", "Show lots and ratio on big spikes", true),
        ss::boolRow("show_msg", "Show the message line", true),
        ss::floatRow("big_x", "Big trap at (x normal)", 2.0, 1.0, 10.0, 50),
        ss::colorRow("col_bull", "Sellers trapped", 0x22C55E, 40),
        ss::same(ss::colorRow("col_bear", "Buyers trapped", 0xEF4444, 40)),
    };
    static const ss::Spec S = { "Trapped", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
// the price-chart letters (lsTrappedMarks)
enum KM { K_MK_VERSION, K_SHOW_TR, K_SHOW_SMALL, K_TR_BIG_X, K_TR_COL_BULL, K_TR_COL_BEAR, K_MK_N };
inline const ss::Spec& marksSpec() {
    static const ss::Row R[K_MK_N] = {
        ss::labelRow("mk_version", "LRA Trapped Marks v1.0.0", 200),
        ss::boolRow("show_tr", "Show Tr letters", true),
        ss::boolRow("show_small", "Show small traps (muted)", true),
        ss::floatRow("tr_big_x", "Big trap at (x normal)", 2.0, 1.0, 10.0, 50),
        ss::colorRow("tr_col_bull", "Sellers trapped", 0x22C55E, 40),
        ss::same(ss::colorRow("tr_col_bear", "Buyers trapped", 0xEF4444, 40)),
    };
    static const ss::Spec S = { "TrappedMarks", PLUGIN_VERSION, 1, R, K_MK_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& plugin, const std::string& mkt) {
    return flex.empty() || mkt.empty() ? std::string() : flex + TRP_SEP + plugin + ".settings-" + mkt + ".txt";
}
} // namespace trpset
