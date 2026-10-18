# Market State spec 1.0 (2026-10-18) - what every indicator posts

Rassul 2026-10-17 23:18: the indicators must work together and be aware of each other so the read can judge the situation.
API: `MarketState.h` (namespace `mst`, SDK-free, tested). Every indicator, once per CLOSED bar (and when a slow state changes), calls

    mst::publish(SRC, symbol, spb, chartLabel, steadyMs, barCtSeconds, kv);

with the same `symbol` / `spb` / chart label hash convention as BannerBoard / SignalBoard (`bb::key`). `barCtSeconds` = the closed
bar's stamp as Central wall-clock seconds (the same conversion every plugin already uses for SignalBoard). Values: plain numbers
(ratios 1 decimal, lots integers, prices with the market's decimals), sides as `buy`/`sell`, `bull`/`bear`, `long`/`short`;
`?` = not known. A fact is decided once for its bar and never changed (no repaint). Remove the entry (`mst::clear`) on destroy.
Keep each source under ~40 keys. Do not change what the indicator DRAWS for this - posting is behind the scenes.

| SRC | Plugin | Keys (all for the last closed bar unless noted) |
|---|---|---|
| AGG | BidAskStats (Aggression) | `buy_lots`, `sell_lots`, `buy_x`, `sell_x` (vs the plain 20-bar average), `spike` (`buy`/`sell`/`none`, >= 2.0x), `pace_x`, `sweep` (`buy`/`sell`/`none`), `big_aggr` (lots of the largest aggressor order), `big_tier` (`S20`/`S10`/`S5`/`none`) |
| BOOK | BookFlow | `bid_net`, `ask_net` (added - pulled, lots), `bid_net_x`, `ask_net_x`, `bid_state`, `ask_state` (`arriving`/`leaving`/`flat`), `spike` (`bid_arriving`/`bid_leaving`/`ask_arriving`/`ask_leaving`/`none`), `rebuilt_pct` (share of hit size restored near price) |
| LOTS | Liquidity Profile MKT / LMT columns | `s20`, `s10`, `s5`, `mu` (the Big Lot tiers in force, slow state), `mkt_buy_max`, `mkt_sell_max` (largest market order this bar, lots), `mkt_tier`, `lmt_filled_max`, `lmt_filled_side`, `iceberg` (`bid`/`ask`/`none`), `resting_bid_max`, `resting_bid_px`, `resting_ask_max`, `resting_ask_px` (largest single resting orders within 2 ATR, slow state), `resting_tier_bid`, `resting_tier_ask` |
| LIQ | Liquidity Profile zones | `zone_id` (level name + price, the zone price is working or nearest), `zone_lo`, `zone_hi`, `zone_level` (e.g. `PDL`, `CR`, `VAH`), `zone_lots`, `zone_x` (vs normal, shown only >= 2.0x), `zone_side` (`support`/`resistance`), `zone_step` (`arrived`/`absorbing`/`attack_fading`/`counter`/`defenders_pulling`/`break`/`held`/`none`), `absorb_lots`, `pulled_before_pct` |
| DP | Delta Profile | `node_px`, `node_x`, `node_side` (`buy`/`sell` delta), `a` (`final`/`tentative`/`none` - an A on this bar), `a_side` (`bull`/`bear`), `a_px`, `a_x`, `top3` (prices of the top 3 nodes, `;` not allowed - use `/`) |
| GEX | Dealer (gamma) Profile | `regime` (`pos`/`neg`/`?`), `hvl`, `cr`, `ps`, `near_level`, `near_dist_ticks`, `near_lots` (dealer hedge lots at that node), `data_age_min`, `stale` (`0`/`1`) |
| HODLOD | HOD/LOD box | `hod`, `lod`, `hod_time`, `lod_time`, `first_extreme` (`hod`/`lod`/`?`), `proj_hi`, `proj_lo`, `range_used` (RTH range / average RTH range), `q_m`, `q_o`, `q_v`, `trend` (`-2..+2`), `regime`, `gex_band_hi`, `gex_band_lo`, `data_age_min` |
| COB | CobImpulse | `cob` (`bull`/`bear`/`none` on this bar), `cob_odds` (0-1 or `?`), `cob_odds_base`, `cob_odds_n`, `cob_tag` (`R`/`V`/`C`/``), `cob_gamma` |
| VWAP | Session VWAP | `vwap`, `sd1_hi`, `sd1_lo`, `sd2_hi`, `sd2_lo`, `pos` (`above2`/`above1`/`above`/`below`/`below1`/`below2`), `slope` (`up`/`down`/`flat`) |
| TREND | (VWAP trend engine, posted by Session VWAP or HOD/LOD - whichever owns VwapTrendLogic) | `trend` (`-2..+2`), `trend_word` |
| READ | Tape Reader / Order Flow Read (the integrator) | `zone_id`, `story` (as `zone_step`), `story_path` (`S0`/`S1`/`S2`/`F`/`?` so far), `odds` (0-1 or `?`), `odds_q`, `odds_base`, `odds_n`, `badge` (`ABS`/`REV`/`?`), `grade` (knowledge grade for this read in this market) |

The integrator (Tape Reader) reads all sources with `mst::readAll`, runs the zone story, looks up the odds (`OddsTable.h`) and
writes the snapshot `lsFlexLevels\MarketState\<MKT>-<spb>.json` every 2 s (`mst::snapshotJson`) for Claude's live tools.
SignalBoard events (A / B / F) stay as they are; the Delta Profile A event gains a `flag` value 1 = final, 2 = tentative.
