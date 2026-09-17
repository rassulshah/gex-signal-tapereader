# lsDayModel — the eyes-on checklist (Gate B, the part no test can see)

Use with the **expected picture** `tools/gp-regress.py --indicator daymodel` prints for the CSV under test. Screenshot
within a minute of the CSV's `ASOF`. Tick each line; transcribe into `irt.json` (`{"chart_close":..., "exp":{"o","h","l","c"}, "act":{...}, "basis":"..."}`).

## Placement (scale)
- [ ] The EXPECTED candle's open sits on the RTH open of the chart's own contract: chart price = CSV price + (chart close at the SCALEREF minute − SCALEREF). Inside RTH on the charted contract the offset is ~0.
- [ ] Its high / low are the runner's `expected_candle_chart` (±1 pt); the body leans the way `EXPMODEL`'s drive says (0 = doji until 30 min).
- [ ] The ACTUAL candle = `DAYACT` (+ offset): O/H/L/C read the same as the chart's own session so far.
- [ ] PDH / PDL / ONH / ONL from the chart's bars (DayModelLogic `chartLevels`) — PDH is the prior RTH high, never a pre-open print (the 0.16 trap: 7690 vs 7687).

## Labels
- [ ] The basis chip reads `EXPMODEL`'s basis (`em-open60` etc.) and the pin state (`pin` / `est`); before the open `em-exante` or `exante`.
- [ ] `DAYEHOD` / `DAYELOD` clocks and durations under the expected tips; `DAYHOD` / `DAYLOD` under the actual tips once `hodLod` is ready.
- [ ] MUD line only when `DAYMUD` is present (reclaimed).
- [ ] No **STALE** badge (CSV < 4 min old); if there is one, stop — the run is not same-moment.

## What to write down
Screenshot file names, every unticked line with what was seen instead, the operator's words.
