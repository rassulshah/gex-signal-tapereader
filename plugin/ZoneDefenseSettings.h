// ZoneDefenseSettings.h (lsZoneDefense 1.0.0 + lsZoneDefenseMarks 1.0.0, 2026-11-08): the ONE settings table of the pane and the
// price-chart marks. File: lsFlexLevels\ZoneDefense.settings-<MKT>.txt (keyed, per market); both plugins open the same window.
#pragma once
#include <string>
#include "SettingsSpec.h"

namespace zdset {
static const char* const PLUGIN_VERSION = "1.0.0";   // = the setVersion literal in ZoneDefense.cpp (lsZoneDefense + lsZoneDefenseMarks)
enum K { K_SHOW_LINES, K_SHOW_BARS, K_SHOW_ZONES, K_SHOW_LABELS, K_SHOW_BIG, K_SHOW_DIAMOND, K_SHOW_SWEEP, K_MAX_ZONES, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::boolRow("show_lines", "Read lines (pane)", true),
        ss::boolRow("show_bars", "Sold / left bars (pane)", true),
        ss::masterOf(ss::boolRow("show_zones", "Zones (price chart)", true)),
        ss::in(ss::boolRow("show_labels", "Zone label", true), "show_zones"),
        ss::in(ss::boolRow("show_big", "Big order line", true), "show_zones"),
        ss::boolRow("show_diamond", "Reload diamond", true),
        ss::boolRow("show_sweep", "Sweep triangle", true),
        ss::intRow("max_zones", "Zones drawn per side", 3, 1, 10, 50),
    };
    static const ss::Spec S = { "ZoneDefense", PLUGIN_VERSION, 1, R, K_N, 0 };   // parameter version 1
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\ZoneDefense.settings-" + mkt + ".txt"; }
} // namespace zdset
