// CandleAbsorbSettings.h (lsCandleAbsorb 1.2.0, 2026-11-11): the ONE settings table. Rassul 2026-11-11: "Update the candle absorb indicator so
// that it doesn't display designers by default and only displays the dash marking but the setting allows you to select the signals you want
// displayed along with markings like delta poc (default) as well as volume poc and bid poc and ask poc." (13:41 "Go" on the mockup, 13:48 "Build")
// File: lsFlexLevels\CandleAbsorb.settings-<MKT>.txt (keyed, per market). Controls only (no how-to text).
#pragma once
#include <string>
#include "SettingsSpec.h"

namespace caset {
static const char* const PLUGIN_VERSION = "1.2.1";   // = the setVersion literal in CandleAbsorb.cpp = cab::VERSION
enum K { K_SIG_A, K_SIG_AE, K_SIG_D, K_SIG_S, K_MARK_DELTA, K_MARK_VOLUME, K_MARK_BID, K_MARK_ASK, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::boolRow("sig_a", "Signal A (absorbed)", false),
        ss::boolRow("sig_ae", "Signal Ae (absorbed + exhaustion)", false),
        ss::boolRow("sig_d", "Signal D (demand)", false),
        ss::boolRow("sig_s", "Signal S (supply)", false),
        ss::boolRow("mark_delta", "Delta POC dash", false),   // (1.2.1) off by default
        ss::boolRow("mark_volume", "Volume POC dash", false),
        ss::boolRow("mark_bid", "Bid POC tick", true),        // (1.2.1) on by default
        ss::boolRow("mark_ask", "Ask POC tick", true),        // (1.2.1) on by default
    };
    static const ss::Spec S = { "CandleAbsorb", PLUGIN_VERSION, 2, R, K_N, 0 };   // parameter version 2 (1 = no settings, 1.1.1)
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\CandleAbsorb.settings-" + mkt + ".txt"; }
} // namespace caset
