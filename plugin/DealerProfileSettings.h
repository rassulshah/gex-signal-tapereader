// DealerProfileSettings.h  (lsDealerProfile 2.17.1, 2026-10-18 settings audit)  -  the ONE table of the Dealer Profile settings window.
// Same 12 rows in the same order (parameter version 6 kept). File: lsFlexLevels\DealerProfile.settings-<MKT>.txt (keyed, per MARKET -
// DealerProfile.settings.txt was keyed by the contract root, so a roll reset every setting), migrated once from that file (left in place).
// lsHodLod reads the "banner" key of this file (it read column 9 of the old positional row).
#pragma once
#include "SettingsSpec.h"

namespace dprset {
static const char* const PLUGIN_VERSION = "2.20.0";
// (2.20.0, Rassul approved the QUEUE METER 2026-10-22 13:51 CT) + "Queue meter" on / off, "Queue meter reach (ATR)", its bid / offer / warning
// colours, appended (earlier rows keep their positions) -> parameter version 9
// (2.19.0, Rassul 2026-10-21 approved Option B) + "Level % badges" (on by default) -> parameter version 8
// (2.17.2, his decision list) "Snapshot outline" removed (dashed: against his no-dashed rule; a saved snap key is ignored);
// Market / Clock offset / Indicators text hidden (kept in the file with their defaults / saved values, not shown) -> parameter version 7.
enum K { K_MARKET, K_WIDTH, K_LEVELS_W, K_BADGE_FONT, K_LIQ_BADGE, K_LEVEL_PCT, K_FONT, K_CLOCK, K_LABELS, K_FADEFAR, K_BANNER, K_INDS, K_QUEUE_METER, K_QM_REACH, K_QM_BID, K_QM_ASK, K_QM_WARN, K_N };
inline const ss::Spec& spec() {
    static const ss::Row R[K_N] = {
        ss::hide(ss::listRow("market", "Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU", 70)),
        ss::intRow("width", "Dealer column px (40-250)", 80, 40, 250, 50),
        ss::intRow("levels_w", "Levels column px (30-150)", 78, 30, 150, 50),
        ss::same(ss::intRow("badge_font", "Badge font (pt)", 9, 8, 14, 50)),
        ss::boolRow("liq_badge", "Liquidity in badge", true),
        ss::boolRow("level_pct", "Level % badges", true),
        ss::same(ss::intRow("font", "Font size (pt)", 10, 10, 24, 50)),
        ss::hide(ss::intRow("clock", "Clock offset (min)", 0, -720, 720, 50)),
        ss::boolRow("labels", "Show buy / sell text in node", true),
        ss::boolRow("fadefar", "Fade strikes beyond 1.2 EM", true),
        ss::same(ss::boolRow("banner", "Whole-book banner", false)),
        ss::hide(ss::textRow("inds", "Indicators in the bar file", "cob_bull,cob_bear", 160, 160)),
        ss::boolRow("queue_meter", "Queue meter", true),
        ss::same(ss::floatRow("qm_reach", "Reach (ATR)", 1.0, 0.25, 2.0, 50)),
        ss::colorRow("qm_bid", "Meter bids", 0x22C55E, 40),
        ss::same(ss::colorRow("qm_ask", "Meter offers", 0xEF4444, 40)),
        ss::same(ss::colorRow("qm_warn", "Originals gone", 0xFACC15, 40)),
    };
    static const ss::Spec S = { "DealerProfile", PLUGIN_VERSION, 9, R, K_N, 0 };
    return S;
}

// DealerProfile.settings.txt (2.11.0+): "key|market|width|font|clock|labels|snap|fadefar|banner|inds|c2|levelsW|badgeFont|liqBadge";
// 2.10 and older: "key|market|width|font|clock|labels|snap|fadefar|banner|inds" (width = the old bar length: 50 -> default 80).
// One row per contract root ("EPZ26", "NQ"): the row of THIS market is used, a 2.11 (c2) row first. Per-chart keys ("EPZ26~3 Minutes*~",
// his file of 10-05) were never matched by 2.2 - 2.17.0, which drew the defaults: they are skipped so nothing he sees changes.
inline bool parseRow(const ss::Spec& S, const std::vector<std::string>& t, ss::Values& V) {
    if (t.size() < 9) return false;
    std::vector<const char*> order; const char* o[] = { "", "market", "", "font", "clock", "labels", "snap", "fadefar", "banner", "inds" };
    for (size_t i = 0; i < 10 && i < t.size(); ++i) order.push_back(o[i]);
    std::vector<std::string> f(t.begin(), t.begin() + (t.size() < 10 ? t.size() : 10));
    if (f.size() > 9 && f[9].empty()) order[9] = "";             // blank = keep the default text (the old reader did the same)
    ss::fromPositional(S, order, f, V);
    char* e = nullptr; const long w = std::strtol(t[2].c_str(), &e, 10); const bool wok = e && !*e && !t[2].empty();
    const bool v2 = t.size() >= 13 && t[10] == "c2";
    if (v2) {
        if (wok && ss::valid(S.rows[K_WIDTH], (double)w)) V.v[K_WIDTH] = (double)w;
        std::vector<const char*> o2(14, ""); o2[11] = "levels_w"; o2[12] = "badge_font"; o2[13] = "liq_badge";
        ss::fromPositional(S, o2, t, V);
    } else if (wok) V.v[K_WIDTH] = w == 50 ? 80.0 : (double)(w < 40 ? 40 : (w > 250 ? 250 : w));
    return true;
}
inline bool legacy(const ss::Spec& S, int mkt, const std::string& text, ss::Values& V) {
    const std::vector<std::string> lines = ss::split(text, '\n');
    int best = -1, bestRank = 99;
    for (size_t i = 0; i < lines.size() && i < 512; ++i) {
        std::string ln = lines[i]; while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
        if (ln.size() > 600) continue;
        const std::vector<std::string> t = ss::split(ln, '|'); if (t.size() < 9 || ss::marketOfRoot(t[0]) != mkt) continue;
        if (t[0].find('~') != std::string::npos) continue;   // a pre-2.2 per-chart key: 2.2-2.17.0 never matched it (they drew the defaults) - kept so
        const int rank = (!V.hint.empty() && t[0] == V.hint) ? -1 : ((t.size() >= 13 && t[10] == "c2") ? 0 : 1);   // the chart's own root first (what 2.17.0 read)
        if (rank < bestRank) { bestRank = rank; best = (int)i; }
    }
    if (best < 0) return false;
    std::string ln = lines[(size_t)best]; while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
    return parseRow(S, ss::split(ln, '|'), V);
}
} // namespace dprset
