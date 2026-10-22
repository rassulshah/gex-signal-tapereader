// SweepRefillSettings.h (lsSweepRefill 1.1.0, 2026-10-22; 1.0.0 Rassul 16:16 / 16:18 / 16:28 CT, 1.1.0 approved 17:39 CT): the ONE
// settings table of the Sr mark. Controls only (no how-to text). File: lsFlexLevels\SweepRefill.settings-<MKT>.txt (keyed, per market).
// 1.1.0: the detection numbers (min prices, refill %, window, stay) moved to the per-market params file lsFlexLevels\LRA-StopSweep-<MKT>.csv
// (the validation's locked rule; data-driven per market), so their settings are gone; added the ZigZag line (show + colour) and the
// swing line (show).
#pragma once
#include "SettingsSpec.h"
#ifndef SWR_SEP
#if defined(_WIN32)
#define SWR_SEP "\\"
#else
#define SWR_SEP "/"
#endif
#endif

namespace swrset {
static const char* const PLUGIN_VERSION = "1.1.1";
enum K { K_VERSION, K_SHOW_BULL, K_COL_BULL, K_SHOW_BEAR, K_COL_BEAR, K_SHOW_ZZ, K_COL_ZZ, K_SHOW_REF, K_GAP_BARS, K_FONT, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::labelRow("version", "LRA Sweep Refill v1.1.0", 200),
        ss::boolRow("show_bull", "Show bullish Sr", true),
        ss::same(ss::colorRow("col_bull", "Bullish", 0x22C55E, 40)),
        ss::boolRow("show_bear", "Show bearish Sr", true),
        ss::same(ss::colorRow("col_bear", "Bearish", 0xEF4444, 40)),
        ss::boolRow("show_zz", "Show ZigZag", true),
        ss::same(ss::colorRow("col_zz", "ZigZag", 0x93C5FD, 40)),
        ss::boolRow("show_ref", "Line to the swing taken out", true),
        ss::intRow("gap_bars", "Min candles between Sr", 1, 1, 10, 40),
        ss::same(ss::intRow("font", "Font size", 10, 8, 14, 40)),
    };
    static const ss::Spec S = { "SweepRefill", PLUGIN_VERSION, 2, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + SWR_SEP "SweepRefill.settings-" + mkt + ".txt"; }
} // namespace swrset
