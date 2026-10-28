// WavePOCSettings.h  (lsWavePOC 1.1.0, 2026-10-28)  -  the ONE table of the Wave POC settings window (controls only, no how-to text).
// Two rows (Rassul 2026-10-28): "Reversal bars" (the N of the N-bar reversal pivots, 2-10, default 3) and "Show zigzag" (default OFF -
// he already has a zigzag indicator). The defaults give the full behaviour with nothing touched.
// The chart reads these rows from IRT's OWN per-indicator parameters (getParameters, template wpp::TEMPLATE "IB") - the SessionVWAP 1.8.1
// native pattern; a refused / invalid set = the defaults (status PARMS line says which).
#pragma once
#include "SettingsSpec.h"

namespace wpset {
static const char* const PLUGIN_VERSION = "1.1.0";   // (1.1.0) liquidity nodes; same two rows
enum K { K_REV_BARS, K_ZIGZAG, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::intRow("rev_bars", "Reversal bars", 3, 2, 10, 40),
        ss::boolRow("zigzag", "Show zigzag", false),
    };
    static const ss::Spec S = { "WavePOC", PLUGIN_VERSION, 1, R, K_N, 0 };
    return S;
}
} // namespace wpset
