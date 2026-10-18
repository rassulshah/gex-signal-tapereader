// OrderFlowPaneSettings.h  (lsBidAskStats 1.13.2 / lsBookFlow 1.3.2, 2026-10-18 settings audit)  -  the settings window tables of the
// order-flow panes. The rows, their order and types are unchanged (IRT keeps window values by position, so no parameter version bump);
// the per-market place files keep their existing layout (BidAskStats.place-<MKT>.txt / BookFlow.place-<MKT>.txt, written by the panes)
// and the window is registered, filled and read through SettingsHost.h.
#pragma once
#include "SettingsSpec.h"

namespace ofset {

static const char* const BAS_VERSION = "1.13.2";
static const char* const BF_VERSION = "1.3.2";
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";

namespace bask { enum K { K_RATIO_BARS, K_POSITION, K_SPIKE_AVG, K_OFFSET_X, K_OFFSET_Y, K_N }; }
inline const ss::Spec& bidAskSpec() {
    static const ss::Row R[bask::K_N] = {
        ss::intRow("ratio_bars", "Ratio bars", 20, 5, 200, 50),
        ss::same(ss::listRow("position", "Position", 0, POS_CHOICES, 110)),
        ss::floatRow("spike_avg", "Spike vs average", 2.0, 0.0, 10.0, 50),     // own line (the first line was ~458 px, cut off)
        ss::intRow("offset_x", "Offset X", 0, -600, 600, 50),
        ss::same(ss::intRow("offset_y", "Offset Y", 0, -600, 600, 50)),
    };
    static const ss::Spec S = { "BidAskStats", BAS_VERSION, 5, R, bask::K_N };
    return S;
}

// BookFlow: the 1.3.0 order kept exactly (Ratio, Position, Spike, Near, Record, Offset X, Offset Y)
namespace bfk { enum K { K_RATIO_BARS, K_POSITION, K_SPIKE_AVG, K_NEAR_TICKS, K_RECORD, K_OFFSET_X, K_OFFSET_Y, K_N }; }
inline const ss::Spec& bookFlowSpec() {
    static const ss::Row R[bfk::K_N] = {
        ss::intRow("ratio_bars", "Ratio bars", 20, 5, 200, 50),
        ss::same(ss::listRow("position", "Position", 0, POS_CHOICES, 110)),
        ss::floatRow("spike_avg", "Spike vs average", 2.0, 0.0, 10.0, 50),     // own line (the first line was ~458 px, cut off)
        ss::intRow("near_ticks", "Near ticks (0 = auto)", 0, 0, 2000, 50),
        ss::same(ss::boolRow("record", "Record order feed", true)),
        ss::intRow("offset_x", "Offset X", 0, -600, 600, 50),
        ss::same(ss::intRow("offset_y", "Offset Y", 0, -600, 600, 50)),
    };
    static const ss::Spec S = { "BookFlow", BF_VERSION, 3, R, bfk::K_N };
    return S;
}

inline int tenths(double x) { return (int)std::llround(x * 10.0); }

} // namespace ofset
