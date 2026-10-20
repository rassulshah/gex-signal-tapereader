// DefenseLinesSettings.h (lsDefenseLines 1.0.0, 2026-10-20, Rassul 12:04 / 12:22 CT): the ONE settings table for the pivot turn marks and
// the refill lines. File: lsFlexLevels\DefenseLines.settings-<MKT>.txt (keyed, per market). lsBookFlow reads the same file for its pane
// marks, so one window controls both. Each drawn element has a Show box and its colours (his settings rule).
#pragma once
#include "SettingsSpec.h"

namespace dlset {
static const char* const PLUGIN_VERSION = "1.0.0";
enum K { K_SHOW_LINES, K_COL_LOW, K_COL_HIGH, K_SHOW_MARKS, K_MAX_MINUTES, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show_lines", "Refill lines", true)),
        ss::same(ss::in(ss::colorRow("col_low", "At lows", 0x22C55E, 0), "show_lines")),
        ss::same(ss::in(ss::colorRow("col_high", "At highs", 0xEF4444, 0), "show_lines")),
        ss::boolRow("show_marks", "Turn marks (price + Defense pane)", true),
        ss::intRow("max_minutes", "Line minutes", 15, 3, 120, 50),
    };
    static const ss::Spec S = { "DefenseLines", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\DefenseLines.settings-" + mkt + ".txt"; }
// the turns files written by lsBookFlow for one market and bar size
inline std::string turnsPath(const std::string& flex, const std::string& mkt, int spb) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\BookFlow\\" + mkt + "-" + std::to_string(spb) + "-turns.csv"; }
inline std::string livePath(const std::string& flex, const std::string& mkt, int spb) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\BookFlow\\" + mkt + "-" + std::to_string(spb) + "-turns-live.txt"; }
} // namespace dlset
