// StopRunMarkSettings.h (lsStopRun 1.0.0, 2026-10-22, Rassul 15:33-15:37 CT): the ONE settings table of the St / Sta stop-run mark.
// Controls only (no how-to text). File: lsFlexLevels\StopRunMark.settings-<MKT>.txt (keyed, per market).
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
static const char* const PLUGIN_VERSION = "1.0.0";
enum K { K_VERSION, K_SHOW, K_COL_ST, K_COL_BULL, K_COL_BEAR, K_WINDOW, K_MIN_FILLS, K_RATIO, K_MIN_IDS, K_NEAR, K_BIG, K_ABSORB, K_FONT, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::labelRow("version", "LRA Stop Run v1.0.0", 200),
        ss::boolRow("show", "Show St / Sta marks", true),
        ss::colorRow("col_st", "St (fired)", 0x9CA3AF, 40),
        ss::same(ss::colorRow("col_bull", "Sta bullish", 0x22C55E, 40)),
        ss::same(ss::colorRow("col_bear", "Sta bearish", 0xEF4444, 40)),
        ss::intRow("window_ms", "Burst window (ms)", 50, 10, 1000, 50),
        ss::same(ss::intRow("min_fills", "Min fills", 5, 2, 200, 45)),
        ss::floatRow("ratio", "Burst vs normal (x)", 2.0, 1.0, 20.0, 50),
        ss::same(ss::intRow("min_ids", "Min orders", 2, 1, 50, 45)),
        ss::perMarket(ss::intRow("near_ticks", "Near level (ticks)", 4, 1, 50, 45), 4, 8, 5, 5, 3, 3, 3),
        ss::same(ss::perMarket(ss::intRow("big_lots", "Big resting (lots)", 20, 1, 1000, 50), 20, 5, 5, 3, 3, 3, 5)),
        ss::intRow("absorb_bars", "Absorbed within (candles)", 3, 1, 5, 40),
        ss::same(ss::intRow("font", "Font size", 10, 8, 14, 40)),
    };
    static const ss::Spec S = { "StopRunMark", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + SRM_SEP "StopRunMark.settings-" + mkt + ".txt"; }
} // namespace srmset
