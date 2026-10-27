// CobImpulseSettings.h  (lsCobImpulse 0.5.0, 2026-10-18, Rassul "I'll go with your recommendation": show / hide + colour for what it draws)
// The ONE table of the Cob Impulse settings window (0.4.1 had no settings). File: lsFlexLevels\CobImpulse.settings-<MKT>.txt (keyed).
// Each drawn element has a Show box that greys its colour: the Cob label text and the thin box around the impulse bar(s).
// The hover stays as it is. Defaults = what 0.4.1 drew (his IRT Cob label colour, a white box).
#pragma once
#include "SettingsSpec.h"

namespace ciset {
static const char* const PLUGIN_VERSION = "0.6.0";
enum K { K_SHOW_LABELS, K_LABEL_COLOR, K_SHOW_BOX, K_BOX_COLOR, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::masterOf(ss::boolRow("show_labels", "Cob labels", true)),
        ss::same(ss::in(ss::colorRow("label_color", "Label", 0x80FFFF, 0), "show_labels")),
        ss::masterOf(ss::boolRow("show_box", "Impulse bar box", true)),
        ss::same(ss::in(ss::colorRow("box_color", "Box", 0xFFFFFF, 0), "show_box")),
    };
    static const ss::Spec S = { "CobImpulse", PLUGIN_VERSION, 2, R, K_N, 0 };   // parameter version 2: 0.4.1 registered nothing (version 1)
    return S;
}
} // namespace ciset
