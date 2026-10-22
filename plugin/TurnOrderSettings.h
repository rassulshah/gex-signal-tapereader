// TurnOrderSettings.h (lsTurnOrder 1.0.0, 2026-10-22, Rassul 15:45 CT "I'll go with your recommendation"): the ONE settings table of the
// TURN ORDER mark. File: lsFlexLevels\TurnOrder.settings-<MKT>.txt (keyed, per market). Controls only (no how-to text).
#pragma once
#include "SettingsSpec.h"

namespace torset {
static const char* const PLUGIN_VERSION = "1.1.0";
enum K { K_SHOW, K_COL_BID, K_COL_OFFER, K_GATE_ATR, K_MIN_SIZE, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show", "Turn orders", true)),
        ss::same(ss::in(ss::colorRow("col_bid", "Bid", 0x22C55E, 0), "show")),
        ss::same(ss::in(ss::colorRow("col_offer", "Offer", 0xEF4444, 0), "show")),
        ss::floatRow("gate_atr", "Confirm: price left by (ATR)", 0.5, 0.1, 3.0, 50),
        ss::intRow("min_size", "Min order size (lots, 0 = Big Lot S20)", 0, 0, 5000, 50),
    };
    static const ss::Spec S = { "TurnOrder", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
inline std::string settingsPath(const std::string& flex, const std::string& mkt) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\TurnOrder.settings-" + mkt + ".txt"; }
} // namespace torset
