// ReversalStageSettings.h  (lsReversalStage 1.0.0, 2026-10-22)  -  the ONE table of the Reversal Stage settings window.
// File: lsFlexLevels\ReversalStage.settings-<MKT>.txt (keyed, per market). Every row is read by ReversalStage.cpp: the steps (show + the
// low-pivot / high-pivot colours), the stage names at the right of the last candle, the Now box (show + its 9-spot Position).
#pragma once
#include "SettingsSpec.h"

namespace rsset {
static const char* const PLUGIN_VERSION = "1.2.0";
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
enum K { K_SHOW_STEPS, K_COL_LOW, K_COL_HIGH, K_SHOW_NAMES, K_SHOW_NOW, K_POSITION, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show_steps", "Steps", true)),
        ss::same(ss::in(ss::colorRow("col_low", "Low pivot", 0x22C55E, 0), "show_steps")),
        ss::same(ss::in(ss::colorRow("col_high", "High pivot", 0xEF4444, 0), "show_steps")),
        ss::boolRow("show_names", "Stage names", true),
        ss::masterOf(ss::boolRow("show_now", "Now box", true)),
        ss::same(ss::in(ss::listRow("position", "Position", 0, POS_CHOICES, 110), "show_now")),
    };
    static const ss::Spec S = { "ReversalStage", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
} // namespace rsset
