// StopRunMarkSettings.h (lsStopRun 1.2.0, 2026-10-22, Rassul 15:33-15:37 CT; 1.1.0 approved 16:13 CT; 1.2.0 17:39 / 18:05 / 18:07 CT):
// the ONE settings table of the St / Sta stop-run mark. Controls only (no how-to text). File: lsFlexLevels\StopRunMark.settings-<MKT>.txt.
// 1.2.0: the detection numbers (burst window, min fills, ratio, near ticks, big lots, the decision) and the level switches moved to the
// per-market params file lsFlexLevels\LRA-StopSweep-<MKT>.csv (the St / Sta validation's locked rule, data-driven per market); the
// 1.1 "big orders took %" and "reclaim required" settings are gone with their rules.
#pragma once
#include "SettingsSpec.h"
#ifndef SRM_SEP
#if defined(_WIN32)
#define SRM_SEP "\\"
#else
#define SRM_SEP "/"
#endif
#endif

namespace srmset {
static const char* const PLUGIN_VERSION = "1.2.1";
enum K { K_VERSION, K_SHOW, K_COL_ST, K_COL_BULL, K_COL_BEAR, K_MIN_IDS, K_FONT, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::labelRow("version", "LRA Stop Run v1.2.0", 200),
        ss::boolRow("show", "Show St / Sta marks", true),
        ss::colorRow("col_st", "St (fired)", 0x9CA3AF, 40),
        ss::same(ss::colorRow("col_bull", "Sta bullish", 0x22C55E, 40)),
        ss::same(ss::colorRow("col_bear", "Sta bearish", 0xEF4444, 40)),
        ss::intRow("min_ids", "Min orders", 2, 1, 50, 45),
        ss::same(ss::intRow("font", "Font size", 10, 8, 14, 40)),
    };
    static const ss::Spec S = { "StopRunMark", PLUGIN_VERSION, 3, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + SRM_SEP "StopRunMark.settings-" + mkt + ".txt"; }
} // namespace srmset
