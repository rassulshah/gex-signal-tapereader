// OrderFlowPaneSettings.h  (lsBidAskStats 1.13.2 / lsBookFlow 1.3.2, 2026-10-18 settings audit)  -  the settings window tables of the
// order-flow panes. The rows, their order and types are unchanged (IRT keeps window values by position, so no parameter version bump);
// the per-market place files keep their existing layout (BidAskStats.place-<MKT>.txt / BookFlow.place-<MKT>.txt, written by the panes)
// and the window is registered, filled and read through SettingsHost.h.
#pragma once
#include "SettingsSpec.h"

namespace ofset {

static const char* const BAS_VERSION = "1.13.3";
static const char* const BF_VERSION = "1.3.3";
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";

// (1.13.3 / 1.3.3, Rassul 2026-10-18 "I'll go with your recommendation") Offset X / Y removed (the 9-spot Position only), Ratio bars and
// Spike vs average hidden (data-tuned: kept in the file at their values, set by the per-market data), BookFlow records always.
namespace bask { enum K { K_RATIO_BARS, K_POSITION, K_SPIKE_AVG, K_N }; }
inline const ss::Spec& bidAskSpec() {
    static const ss::Row R[bask::K_N] = {
        ss::hide(ss::intRow("ratio_bars", "Ratio bars", 20, 5, 200, 50)),
        ss::listRow("position", "Position", 0, POS_CHOICES, 110),
        ss::hide(ss::floatRow("spike_avg", "Spike vs average", 2.0, 0.0, 10.0, 50)),
    };
    static const ss::Spec S = { "BidAskStats", BAS_VERSION, 6, R, bask::K_N, 0 };   // 6: the window rows changed
    return S;
}

namespace bfk { enum K { K_RATIO_BARS, K_POSITION, K_SPIKE_AVG, K_NEAR_TICKS, K_N }; }
inline const ss::Spec& bookFlowSpec() {
    static const ss::Row R[bfk::K_N] = {
        ss::hide(ss::intRow("ratio_bars", "Ratio bars", 20, 5, 200, 50)),
        ss::listRow("position", "Position", 0, POS_CHOICES, 110),
        ss::hide(ss::floatRow("spike_avg", "Spike vs average", 2.0, 0.0, 10.0, 50)),
        ss::intRow("near_ticks", "Near ticks (0 = auto)", 0, 0, 2000, 50),
    };
    static const ss::Spec S = { "BookFlow", BF_VERSION, 4, R, bfk::K_N, 0 };   // 4: the window rows changed
    return S;
}

inline int tenths(double x) { return (int)std::llround(x * 10.0); }

} // namespace ofset
