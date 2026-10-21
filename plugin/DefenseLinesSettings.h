// DefenseLinesSettings.h (lsDefenseLines 1.2.0, 2026-10-20, Rassul 12:04 / 12:22 / 13:54 CT): the ONE settings table for the refill lines,
// the exhaustion lines, the dots and the Defense pane's turn marks. File: lsFlexLevels\DefenseLines.settings-<MKT>.txt (keyed, per market).
// lsBookFlow reads the same file for its pane marks, so one window controls both. Each drawn element has a Show box and its colours.
#pragma once
#include "SettingsSpec.h"

namespace dlset {
static const char* const PLUGIN_VERSION = "1.7.0";
enum K { K_SHOW_LINES, K_COL_LOW, K_COL_HIGH, K_SHOW_E, K_COL_E, K_SHOW_DOTS, K_COL_DOT, K_SHOW_MARKS, K_MAX_CANDLES, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show_lines", "Refill lines (R)", true)),
        ss::same(ss::in(ss::colorRow("col_low", "At lows", 0x22C55E, 0), "show_lines")),
        ss::same(ss::in(ss::colorRow("col_high", "At highs", 0xEF4444, 0), "show_lines")),
        ss::masterOf(ss::boolRow("show_e", "Exhaustion lines (E)", true)),
        ss::same(ss::in(ss::colorRow("col_e", "Colour", 0xA3A3A3, 0), "show_e")),
        ss::masterOf(ss::boolRow("show_dots", "Dot on the candle", true)),
        ss::same(ss::in(ss::colorRow("col_dot", "Colour", 0xFACC15, 0), "show_dots")),
        ss::boolRow("show_marks", "Turn marks (Defense pane)", true),
        ss::intRow("max_candles", "Line candles", 3, 1, 20, 50),
    };
    static const ss::Spec S = { "DefenseLines", PLUGIN_VERSION, 2, R, K_N, 0 };   // parameter version 2: the rows changed (1.1.0)
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\DefenseLines.settings-" + mkt + ".txt"; }
// the turns files written by lsBookFlow for one market and bar size
inline std::string turnsPath(const std::string& flex, const std::string& mkt, int spb) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\BookFlow\\" + mkt + "-" + std::to_string(spb) + "-turns.csv"; }
inline std::string livePath(const std::string& flex, const std::string& mkt, int spb) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\BookFlow\\" + mkt + "-" + std::to_string(spb) + "-turns-live.txt"; }
} // namespace dlset
