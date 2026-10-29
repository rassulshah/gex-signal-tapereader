// WavePOCSettings.h  (lsWavePOC 1.3.0, 2026-10-28; 1.3.0 + row 3 "CoB legs", default on, parameter version 2)  -  the ONE table of the Wave POC settings window (controls only, no how-to text).
// Two rows (Rassul 2026-10-28): "Reversal bars" (the N of the N-bar reversal pivots, 2-10, default 3) and "Show zigzag" (default OFF -
// he already has a zigzag indicator). The defaults give the full behaviour with nothing touched.
// The chart reads these rows from IRT's OWN per-indicator parameters (getParameters, template wpp::TEMPLATE "IB") - the SessionVWAP 1.8.1
// native pattern; a refused / invalid set = the defaults (status PARMS line says which).
#pragma once
#include "SettingsSpec.h"

namespace wpset {
static const char* const PLUGIN_VERSION = "1.3.4";   // (1.3.3) 14-bar extreme, AVG or 3 ATR, coloured spine - same rows / parameter version 2
// 1.3.2:   // (1.3.1) crash fix, same rows / parameter version 2
// 1.3.0:   // (1.3.0) + "CoB legs" (every CoB leg gets a profile)
// was 1.2.0:   // (1.2.0) no lines across the chart, smaller profile; (1.1.0) liquidity nodes; (1.1.1) liquidity 1.9x + raw separation; same two rows
enum K { K_REV_BARS, K_ZIGZAG, K_COB_LEGS, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::intRow("rev_bars", "Reversal bars", 3, 2, 10, 40),
        ss::boolRow("zigzag", "Show zigzag", false),
        ss::boolRow("cob_legs", "CoB legs", true),   // (1.3.0, Rassul 17:27 CT) every CoB leg gets a profile
    };
    static const ss::Spec S = { "WavePOC", PLUGIN_VERSION, 2, R, K_N, 0 };   // parameter version 2 (1.3.0: row appended)
    return S;
}
} // namespace wpset
