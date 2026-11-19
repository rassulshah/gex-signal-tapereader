// RevReadSettings.h (lsRevReadMarks 0.5.0, 2026-11-13): the ONE settings table of the price-chart marks (the pane lsRevRead has no settings).
// File: lsFlexLevels\RevReadMarks.settings-<MKT>.txt (keyed, per market).
// Rassul 2026-11-13 07:07 CT: "add the hold read as an uncheckeed options in rev read marks"; 07:12 "i'll go with your recomendation".
#pragma once
#include <string>
#include "SettingsSpec.h"

namespace rvset {
static const char* const PLUGIN_VERSION = "0.20.0";   // (0.20.0) FT / Tst / Rev marks - no settings change   // (0.19.4 / 0.19.5) the I set aside - no settings change   // (0.19.3) black tag writing - no settings change   // (0.19.2) the strength line - no settings change   // (0.19.1) marks keep clear of ZZO / CoB labels - no settings change   // (0.19.0) the level marks - no settings change   // (0.18.5) hourly levels in the I - no settings change   // (0.18.4) the I = a reclaim - no settings change   // (0.18.3) no story line over the strip - no settings change   // (0.18.2) lsRevReadMarks has no settings window any more (table kept, unused)   // (0.18.1) earlier sessions keep their letters - no settings change   // (0.18.0) the I on the candle - no settings change   // (0.17.5) letters file - no settings change   // (0.17.4) strip background - no settings change   // (0.17.0, 2026-11-18) the Wyckoff strip - no settings change, in step with RevRead.cpp   // (0.16.0, 2026-11-17 21:17) the plain read - no settings change, in step with RevRead.cpp   // (0.15.x) no settings change (2026-11-17: back in step with RevRead.cpp 0.15.3 - the settings contract failed on it); was "0.14.0";   // (0.14.0) no settings change; was "0.13.3";   // (0.13.3) no settings change; was "0.13.2";   // (0.13.2) no settings change; was "0.13.1";   // (0.13.1) no settings change; was "0.13.0";   // (0.13.0) no settings change; was "0.12.0";   // (0.12.0) no settings change; was "0.11.1";   // (0.11.1) no settings change; was "0.11.0";   // (0.11.0) Book Flow bars - no settings change; was "0.10.1";   // (0.10.1) no settings change; was "0.10.0";   // (0.10.0) the simple accumulation / distribution rule - no settings change; was "0.9.2";   // (0.9.2) bars in a low pane - no settings change; was "0.9.1";   // (0.9.1) no settings change; was "0.9.0";   // (0.9.0) accumulation zones - no settings change; was "0.8.0";   // (0.8.0) lsSR in the background - no settings change; was "0.7.0";   // (2026-11-15: brought back in step with RevRead.cpp 0.7.0 - the settings contract failed on it) was "0.5.0";   // = the setVersion literal in RevRead.cpp (lsRevRead + lsRevReadMarks)
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
