// CVDSettings.h  (lsCVD 1.3.0, 2026-10-18, Rassul "I'll go with your recommendation": show / hide + colour for what CVD draws)
// The ONE table of the CVD settings window (1.2.0 had no settings). File: lsFlexLevels\CVD.settings-<MKT>.txt (keyed, per market).
// Each drawn element has a Show box that greys its colours: the CVD line, the delta % bars (up / down), the marks (bullish / bearish).
// The header readouts (CVD / Delta numbers), the axis and the dark label text stay fixed. Defaults = what 1.2.0 drew.
#pragma once
#include "SettingsSpec.h"

namespace cvdset {
static const char* const PLUGIN_VERSION = "1.3.1";
enum K { K_SHOW_LINE, K_LINE_COLOR, K_SHOW_BARS, K_BAR_UP, K_BAR_DN, K_SHOW_MARKS, K_MARK_BULL, K_MARK_BEAR, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show_line", "CVD line", true)),
        ss::same(ss::in(ss::colorRow("line_color", "Line", 0x60A5FA, 0), "show_line")),
        ss::masterOf(ss::boolRow("show_bars", "Delta % bars", true)),
        ss::same(ss::in(ss::colorRow("bar_up", "Up", 0x205C49, 0), "show_bars")),
        ss::same(ss::in(ss::colorRow("bar_dn", "Down", 0x5E2B35, 0), "show_bars")),
        ss::masterOf(ss::boolRow("show_marks", "Marks", true)),
        ss::same(ss::in(ss::colorRow("mark_bull", "Bullish", 0x34D399, 0), "show_marks")),
        ss::same(ss::in(ss::colorRow("mark_bear", "Bearish", 0xF87171, 0), "show_marks")),
    };
    static const ss::Spec S = { "CVD", PLUGIN_VERSION, 2, R, K_N, 0 };   // parameter version 2: 1.2.0 registered nothing (version 1)
    return S;
}
} // namespace cvdset
