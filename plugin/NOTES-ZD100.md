# lsZoneDefense 1.0.0 + lsZoneDefenseMarks 1.0.0 - "Zone Defense" (2026-11-08)

Rassul 2026-11-08: "Zone Defense is good. lets build it. add it to the self improvement process" - design approved on
claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G. Jumpstart: research-artifacts/setups/ZONE-DEFENSE-JUMPSTART.md (level-reversal-analytics).

## Files
| File | What |
|---|---|
| ZoneDefenseLogic.h | zd::Engine, SDK-free: zones, touch + queue, reload, sweep, end, truth. The ONE copy of the logic (plugin, zdrun CLI, tests). Locked design values as named constants (LIFT_X, RELOAD_SEC, SWEEP_LIFT_S, QUEUE_REST_S, BROKE_TICKS ...) - lra.consistency_check zd_* rows read them. |
| ZoneDefenseParams.h | per market: signs (reload / pull / sweep; 0 = white), big-order tier; per period: drawn zone-size floor (p60), heavy-selling x, normal cancel share. Written by the jumpstart, then ONLY by lra.zone_defense (nightly, judged on held-out days). |
| ZoneDefenseRead.h | the journal model + the 4 Read lines + the zone label (what both plugins draw). |
| ZoneDefenseSettings.h | the one settings table (both plugins): Read lines, Sold / left bars, Zones, Zone label, Big order line, Reload diamond, Sweep triangle, Zones drawn per side. File lsFlexLevels\ZoneDefense.settings-<MKT>.txt. On the settings contract (tests_settings). |
| ZoneDefense.cpp | lsZoneDefense: its own pane on the order feed (off::MboPane like BookFlow; every event once through onRaw). Writes the journal lsFlexLevels\ZoneDefense\<MKT>-<spb>-<session>.csv and draws ONLY from it. |
| ZoneDefenseMarks.cpp | lsZoneDefenseMarks (= ZoneDefense.cpp + ZD_MARKS): the price-chart overlay - band, label, one big-order line, gray reload diamond, sweep triangle. Reads the journals of the last 3 sessions. |
| tests_zd/ | t_zd.cpp (mock host, 2 real sessions), t_marks.cpp, parity_zd.py (independent Python replay), zdrun.cpp + zdrun-build.bat (the engine as a CLI for the nightly), fx/ (CL / HG 2026-11-04 slices). |

## The pane (4 lines, verdict first, green bullish / red bearish / white not proven, time right-justified, 12-hour)
1. Reload (touch + 60 s): broke 2+ ticks through = "Bid exhausted · broke through at 10:03"; heavy selling into the zone (sold >= reloadX
   x what was shown) in a market whose reloadSign is -1 = "Bid exhausted · absorbed 2,333 (13x) · still at the zone" (red); light selling
   and still there = "Buyers holding · sold 120 of 450 shown" (green); unproven markets: "Quiet · ..." (white). Offers mirrored (Sellers).
2. Queue (60 s before the touch): "Queue: cancels 34% (norm 9%) · pulling" - orders rested 30 s+ at the zone / 1 tick behind; red only
   where pullSign = -1 (GC, HG, NG); never green.
3. Refill: "Refill: 530 times · absorbed 2,333 (13x) · bid 145 of 177 -> 0 by 10:03".
4. Sweep (first stop run per touch, decided 5 s after it, never changes): "Sweep: swept lift 1.4x · ... · re-bid 0.6x" / "Sweep: weak ..."
   / "Sweep: none" (no run in 15 min); "watching" until decided.
Bars: per candle the lots sold into the zone (red) and what is left in it (green dot). No hovers.

## Why the reload colours are the opposite of the design's first idea
Jumpstart (71 market-days, the engine's own decisions, truth after the decision): heavy selling into the zone after the touch made it
break MORE in 6 of 7 markets, in both halves of the days (NG -29, ES -20, GC -18, NQ -17, HG -16, CL -15 pts of 9-min hold). EU too thin.

## No repaint / restart
Every decision is one journal line, written once (append-only). The pane and the marks draw only from the journal; a restart reloads it,
new zones never reuse an id (tests: journal byte-identical before / after a restart; decided lines identical live vs at the close).

## Tests (2026-11-08)
- tests_zd/t_zd.cpp: 36 passed - PARITY plugin journal == engine journal on CL and HG real slices (every line), live vs close, no repaint
  across a restart, layout (no overlaps, 4 lines, colours), versions / title "Zone Defense v1.0.0".
- tests_zd/t_marks.cpp: 10 passed - bands / labels / marks from the journal, no overlap, same picture on every draw and after a restart.
- tests_zd/parity_zd.py: independent Python replay vs the engine on 2 full sessions: CL 52 / 52, HG 56 / 56 (resting at the touch, the
  queue's cancel share and count, sold in the reload window, broke early).
- tests_settings/settings_contract.cpp: + zdset (PASS 47,647 checks with the current headers).
Build: g++ -std=c++17 -O2 -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock t_zd.cpp -o t && ./t fx

## Nightly
lra.zone_defense (part zone_defense:nightly): scores new raw MBO days with zdrun, challenges on the build half, judges on the check half,
rewrites ZoneDefenseParams.h only when a row changes (a market only on 10+ days), never Mon-Fri 06:10-15:00 CT.

## Not in 1.0.0
The filled-then-pulled LADDER label ("ladder 8x25: 2 filled 9:27, 6 pulled 9:30") - needs its own detector; next build.
NG draws only once his Rithmic NG order feed is on (the research uses Databento NG).
