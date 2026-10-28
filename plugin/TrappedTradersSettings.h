// TrappedTradersSettings.h (lsTrappedTraders 0.1.1 2026-10-28: marks everywhere by default; 0.1.0 2026-10-26; Rassul 20:15 CT "lets just call underwater traders trapped traders.
// build it"): the ONE settings table of TrappedTraders.cpp. Controls only (no how-to text). File: lsFlexLevels\TrappedTraders.settings-<MKT>.txt
// (keyed, per market). Parameter version 1.
#pragma once
#include "SettingsSpec.h"
#ifndef TTR_SEP
#if defined(_WIN32)
#define TTR_SEP "\\"
#else
#define TTR_SEP "/"
#endif
#endif

namespace ttrset {
static const char* const PLUGIN_VERSION = "0.1.2";
enum MarksMode { MARKS_LEVELS = 0, MARKS_ALL = 1, MARKS_OFF = 2 };
static const char* const MARKS_CHOICES = "Levels only;Everywhere;Off";
enum K { K_VERSION, K_MARKS, K_SHOW_RATIO, K_SHOW_BOX, K_COL_BULL, K_COL_BEAR, K_N };
// (0.1.1, 2026-10-28, Rassul 20:31 CT "lsTrappedTraders is not only at key levels... it works with defending"): marks EVERYWHERE by default
// (every forming pivot's Stage 3 trigger); "Levels only" stays a choice. A file written by 0.1.0 with the old default (Levels only) = Everywhere.
inline void upgrade(const std::string& fileVersion, ss::Values& V) {
    if ((fileVersion == "0.1.0" || fileVersion.empty()) && V.i(K_MARKS) == MARKS_LEVELS && (size_t)K_MARKS < V.v.size()) V.v[(size_t)K_MARKS] = MARKS_ALL;
}
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::labelRow("version", "LRA Trapped Traders v0.1.2", 200),
        ss::listRow("marks", "Trigger marks", MARKS_ALL, MARKS_CHOICES, 110),
        ss::same(ss::boolRow("show_ratio", "Ratio on marks", true)),
        ss::boolRow("show_box", "Live box", true),
        ss::colorRow("col_bull", "Sellers trapped", 0x22C55E, 40),
        ss::same(ss::colorRow("col_bear", "Buyers trapped", 0xEF4444, 40)),
    };
    static const ss::Spec S = { "TrappedTraders", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) {
    return flex.empty() || mkt.empty() ? std::string() : flex + TTR_SEP + "TrappedTraders.settings-" + mkt + ".txt";
}
} // namespace ttrset
