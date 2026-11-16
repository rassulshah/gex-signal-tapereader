// RevReadSettings.h (lsRevReadMarks 0.5.0, 2026-11-13): the ONE settings table of the price-chart marks (the pane lsRevRead has no settings).
// File: lsFlexLevels\RevReadMarks.settings-<MKT>.txt (keyed, per market).
// Rassul 2026-11-13 07:07 CT: "add the hold read as an uncheckeed options in rev read marks"; 07:12 "i'll go with your recomendation".
#pragma once
#include <string>
#include "SettingsSpec.h"

namespace rvset {
static const char* const PLUGIN_VERSION = "0.10.0";   // (0.10.0) the simple accumulation / distribution rule - no settings change; was "0.9.2";   // (0.9.2) bars in a low pane - no settings change; was "0.9.1";   // (0.9.1) no settings change; was "0.9.0";   // (0.9.0) accumulation zones - no settings change; was "0.8.0";   // (0.8.0) lsSR in the background - no settings change; was "0.7.0";   // (2026-11-15: brought back in step with RevRead.cpp 0.7.0 - the settings contract failed on it) was "0.5.0";   // = the setVersion literal in RevRead.cpp (lsRevRead + lsRevReadMarks)
enum K { K_SHOW_POC, K_SHOW_HOLD, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::boolRow("show_poc", "POC dash", true),
        ss::boolRow("show_hold", "Hold read dots", false),   // off by default (a 60-minute chart draws them regardless, 0.4.1): a dot at a high / low that is likely done (the strong tier only)
    };
    static const ss::Spec S = { "RevReadMarks", PLUGIN_VERSION, 1, R, K_N, 0 };   // parameter version 1
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\RevReadMarks.settings-" + mkt + ".txt"; }
} // namespace rvset
