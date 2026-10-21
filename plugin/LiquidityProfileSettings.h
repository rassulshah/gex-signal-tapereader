// LiquidityProfileSettings.h  (lsLiquidityProfile / lsLiquidityFlow 0.45.0, 2026-10-18 settings audit)  -  the settings window tables.
// Same rows in the same order as 0.44 (IRT keeps window values by position: parameter versions 5 / 2 kept). Fixed: the big-order line
// "min lots | ended lines | font" was ~648 px (cut off) -> the font row starts its own line; "Big resting orders" and "Verdict banner"
// now grey their rows; ranges = what the code draws (Font 10-16: 6-9 were drawn as 10). The three per-market files (size / bigorders /
// banner) keep their strict formats; the window is registered, filled and read through SettingsHost.h.
// (0.45.1, his decision list) label Offset X / Y removed, "Ended lines kept per side" hidden -> parameter version 6.
#pragma once
#include "SettingsSpec.h"

namespace lpset {

static const char* const PLUGIN_VERSION = "0.47.0";   // (0.47.0) the air pocket (AP? band) row; (0.46.1) book object parked at done / destroy; (0.46.0) LMT F only
enum K { K_WIDTH, K_FONT, K_SOLID_BG, K_BIG_ORDERS, K_BIG_REACH, K_BIG_MIN_LOTS, K_BIG_KEEP, K_BIG_FONT,
         K_BANNER, K_BANNER_BG, K_BANNER_FONT, K_BANNER_COLOR, K_AIR_POCKET, K_N };

inline const ss::Spec& profileSpec() {
    static const ss::Row R[K_N] = {
        ss::intRow("width", "Width px", 60, 20, 200, 50),
        ss::same(ss::intRow("font", "Font pt", 11, 10, 16, 40)),
        ss::boolRow("solid_bg", "Solid background", true),
        ss::masterOf(ss::boolRow("big_orders", "Big resting orders", true)),
        ss::same(ss::in(ss::floatRow("big_reach", "Big order reach (ATR)", 2.0, 0.5, 5.0, 45), "big_orders")),
        // 0 in the file = the market's own floor; the window shows that floor (per market) and saves it back as 0
        ss::in(ss::perMarket(ss::intRow("big_min_lots", "Big order min lots", 30, 1, 100000, 60), 30, 10, 15, 15, 12, 10, 50), "big_orders"),
        ss::hide(ss::in(ss::intRow("big_keep", "Ended lines kept per side", 10, 0, 50, 40), "big_orders")),   // (0.45.1) data-tuned: hidden, file value kept
        ss::same(ss::in(ss::intRow("big_font", "Big order font pt", 10, 7, 16, 40), "big_orders")),
        ss::masterOf(ss::boolRow("banner", "Verdict banner", true)),
        ss::same(ss::in(ss::boolRow("banner_bg", "Banner solid background", true), "banner")),
        ss::in(ss::intRow("banner_font", "Banner font pt", 11, 7, 16, 40), "banner"),
        ss::same(ss::in(ss::colorRow("banner_color", "Banner background", 0x141414), "banner")),
        ss::boolRow("air_pocket", "Air pocket (AP band)", true),            // (0.47.0) the band from the turn's close to T1 while the path is open
    };
    static const ss::Spec S = { "LiquidityProfile", PLUGIN_VERSION, 7, R, K_N, 0 };
    return S;
}
// lsLiquidityFlow (the same source built with LQ_FLOW): Font only
enum KF { KF_FONT, KF_N };
inline const ss::Spec& flowSpec() {
    static const ss::Row R[KF_N] = { ss::intRow("font", "Font pt", 11, 10, 16, 50) };
    static const ss::Spec S = { "LiquidityFlow", PLUGIN_VERSION, 2, R, KF_N, 0 };
    return S;
}

} // namespace lpset
