// LiquidityProfileSettings.h  (lsLiquidityProfile / lsLiquidityFlow 0.45.0, 2026-10-18 settings audit)  -  the settings window tables.
// Same rows in the same order as 0.44 (IRT keeps window values by position: parameter versions 5 / 2 kept). Fixed: the big-order line
// "min lots | ended lines | font" was ~648 px (cut off) -> the font row starts its own line; "Big resting orders" and "Verdict banner"
// now grey their rows; ranges = what the code draws (Font 10-16: 6-9 were drawn as 10). The three per-market files (size / bigorders /
// banner) keep their strict formats; the window is registered, filled and read through SettingsHost.h.
// (0.45.1, his decision list) label Offset X / Y removed, "Ended lines kept per side" hidden -> parameter version 6.
#pragma once
#include "SettingsSpec.h"

namespace lpset {

static const char* const PLUGIN_VERSION = "0.59.1";   // (0.59.1) no settings change   // (0.59.0, 2026-10-27) "Level tag on lines" (the diamond row reused, shown again; parameter version 13) //   // (0.58.0, 2026-10-27) "Queue Q at levels" row hidden (the Q is not drawn; value kept) //   // (0.57.0, 2026-10-26 night) no settings change (QueueMeter 1.5.0 his HELD rule)   // (0.56.0, 2026-10-26) self-awareness SELFLIQ (SelfAware.h, SELF-AWARE.md; ships OFF per market) // (0.55.1) no settings change   // (0.55.0) the LMT zone rows relabelled for the LIMIT-ORDER LINES (same keys / positions; parameter version 11): master, icebergs, ladders, pulled lines, queue Q, line / word colours, words; the zone-only rows (ABC / diamond) hidden   // (0.54.1) label-only build (RE-BID?): rows, keys and parameter version 10 unchanged   // (0.54.0) LMT zones 1.3.0: the tags row is now "After-fill tags (RE-BID / ACCEPTED)" (default on), the colour rows "RE-BID offer / bid colour"; parameter version 10 so old saved windows reload once
// history: (0.53.0) LMT zones 1.2.0: the "State tags" row appended (parameter version 9); the colour rows relabelled to what they colour now (keys / positions unchanged)   // (0.52.1) never delete MARKET_DEPTH (crash fix)   // (0.49.0) the queue meter (no new rows) // (0.48.0) the LMT zones rows (appended: earlier rows keep their positions; parameter version 8)   // (0.47.0) the air pocket (AP? band) row; (0.46.1) book object parked at done / destroy; (0.46.0) LMT F only
enum K { K_WIDTH, K_FONT, K_SOLID_BG, K_BIG_ORDERS, K_BIG_REACH, K_BIG_MIN_LOTS, K_BIG_KEEP, K_BIG_FONT,
         K_BANNER, K_BANNER_BG, K_BANNER_FONT, K_BANNER_COLOR, K_AIR_POCKET,
         K_LMZ, K_LMZ_PRINT, K_LMZ_ORIGIN, K_LMZ_NEAR, K_LMZ_FAR, K_LMZ_ABC, K_LMZ_GAMMA, K_LMZ_DIAMOND,
         K_LMZ_C_OFFER, K_LMZ_C_BID, K_LMZ_C_GREY, K_LMZ_C_GAMMA, K_LMZ_TAGS, K_N };

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
        // (0.48.0, Rassul approved LMT zones mockup v3 2026-10-22 09:34 CT) the zones of real resting orders + LMT / LMT F columns; which kinds a
        // market draws is set by the study (per-market rule / nightly params) - these boxes can only hide them
        ss::masterOf(ss::boolRow("lmz", "Limit-order lines (resting 9+ min)", true)),
        ss::in(ss::boolRow("lmz_print", "Icebergs (I R15 500)", true), "lmz"),
        ss::same(ss::in(ss::boolRow("lmz_origin", "Ladders (4o 87)", true), "lmz")),
        ss::in(ss::boolRow("lmz_near", "Pulled lines (very dark)", true), "lmz"),
        ss::hide(ss::in(ss::boolRow("lmz_far", "Queue Q at levels", true), "lmz")),
        ss::hide(ss::in(ss::boolRow("lmz_abc", "ABC breaks (grey)", true), "lmz")),   // (0.55.0) zone-only: hidden
        ss::in(ss::boolRow("lmz_gamma", "Gamma lines (grey)", true), "lmz"),
        ss::in(ss::boolRow("lmz_diamond", "Level tag on lines", true), "lmz"),   // (0.59.0) reused: the level name + "?" on an order line at a key / volume / gamma level (was the zone-only gamma diamond, hidden 0.55.0)
        // (0.54.0) offer / bid colour = the RE-BID tag (red: an offer swept above failing = bearish; green: a bid swept below failing =
        // bullish); the grey = every zone line, label, FILLING, ACCEPTED and X
        ss::in(ss::colorRow("lmz_c_offer", "Offer line colour", 0xEF4444), "lmz"),
        ss::same(ss::in(ss::colorRow("lmz_c_bid", "Bid line colour", 0x22C55E), "lmz")),
        ss::in(ss::colorRow("lmz_c_grey", "THRU word colour", 0x8A8F98), "lmz"),
        ss::same(ss::in(ss::colorRow("lmz_c_gamma", "Gamma line colour", 0x6B7280), "lmz")),
        ss::in(ss::boolRow("lmz_tags", "HELD / THRU words", true), "lmz"),   // (0.54.0) renamed (was State tags, 0.53.0); same key / position
    };
    static const ss::Spec S = { "LiquidityProfile", PLUGIN_VERSION, 13, R, K_N, 0 };   // (0.59.0) 12 -> 13: "Level tag on lines" shown   // (0.58.0) 11 -> 12: "Queue Q at levels" hidden   // (0.55.0) 10 -> 11: rows relabelled for the order lines (old windows reload once)   // (0.54.0) 9 -> 10: the tags row means the after-fill read now (old windows reload once); (0.53.0) 8 -> 9: one row appended
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
