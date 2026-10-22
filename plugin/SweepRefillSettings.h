// SweepRefillSettings.h (lsSweepRefill 1.0.0, 2026-10-22, Rassul 16:16 / 16:18 / 16:28 CT): the ONE settings table of the Sr mark.
// Controls only (no how-to text). File: lsFlexLevels\SweepRefill.settings-<MKT>.txt (keyed, per market).
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
static const char* const PLUGIN_VERSION = "1.0.0";
enum K { K_VERSION, K_SHOW_BULL, K_COL_BULL, K_SHOW_BEAR, K_COL_BEAR, K_MIN_PRICES, K_MIN_LOTS, K_REFILL_PCT, K_WINDOW_S, K_GAP_BARS, K_FONT, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::labelRow("version", "LRA Sweep Refill v1.0.0", 200),
        ss::boolRow("show_bull", "Show bullish Sr", true),
        ss::same(ss::colorRow("col_bull", "Bullish", 0x22C55E, 40)),
        ss::boolRow("show_bear", "Show bearish Sr", true),
        ss::same(ss::colorRow("col_bear", "Bearish", 0xEF4444, 40)),
        ss::intRow("min_prices", "Min prices swept", 8, 3, 100, 45),
        ss::same(ss::perMarket(ss::intRow("min_lots", "Min lots resting", 30, 1, 5000, 50), 30, 30, 30, 30, 30, 30, 30)),
        ss::intRow("refill_pct", "Refill (%)", 70, 10, 300, 45),
        ss::same(ss::intRow("window_s", "Within (s)", 3, 1, 10, 40)),
        ss::intRow("gap_bars", "Min candles between Sr", 1, 1, 10, 40),
        ss::same(ss::intRow("font", "Font size", 10, 8, 14, 40)),
    };
    static const ss::Spec S = { "SweepRefill", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + SWR_SEP "SweepRefill.settings-" + mkt + ".txt"; }
} // namespace swrset
