# lsDayModel regression — 2026-09-17 10:33:00 CT — PASS

_CSV ASOF 10:33:00 · panel 16.37 · SPOT 7702.2 · SCALEREF 7690.5 · elapsed 123 min · rolls {'SPX': 0, 'SPY': -1}_

**Operator / runner note:** synthetic fixture from test_day_export.js (runner self-test)

## Timing

- ✅  CSV ASOF 37980.0 and audit ct 37980 are the same export (<=5s)

## Gate A — the panel's read -> CSV (re-derived from the audit)

- ✅  the audit carries AU.day (panel >= 16.37): yes
- ✅  DAYACT row re-derived from the audit == the CSV (7690.00,7724.00,7676.00,7702.20 vs 7690.00,7724.00,7676.00,7702.20)
- ✅  DAYEXP row re-derived from the audit == the CSV (7690.00,7735.78,7674.54,7705.31 vs 7690.00,7735.78,7674.54,7705.31)
- ✅  EXPMODEL row re-derived from the audit == the CSV (em-open60,61.2,1,0.75,38.6,pin vs em-open60,61.2,1,0.75,38.6,pin)
- ✅  basis em-open60 fits the stage (123 min after the open -> open60/em-open60/weekday)
- ✅  placement f 0.75 inside [0.05, 0.95]
- ✅  EM 38.6 (pin): present with a pin state, or absent with a non-EM basis
- ✅  the EM was pinned 2 min after the open (<= 60)
- ✅  DAYEXP O 7690.0 inside [L 7674.54, H 7735.78], close 7705.31 inside too

## Gate B — CSV -> IRT

- ⚠️  no --irt-read: the IRT candle was not transcribed; the expected chart below is the checklist

## Expected picture (the Gate B checklist for the IRT screenshot)

```json
{
 "expected_candle_es": {
  "o": 7690.0,
  "h": 7735.78,
  "l": 7674.54,
  "c": 7705.31
 },
 "expected_candle_chart": {
  "o": 7690.0,
  "h": 7735.78,
  "l": 7674.54,
  "c": 7705.31
 },
 "actual_candle_es": {
  "o": 7690.0,
  "h": 7724.0,
  "l": 7676.0,
  "c": 7702.2
 },
 "actual_candle_chart": {
  "o": 7690.0,
  "h": 7724.0,
  "l": 7676.0,
  "c": 7702.2
 },
 "model": {
  "rng": 61.235681920000005,
  "basis": "em-open60",
  "f": 0.7475365853658537,
  "drive": 1,
  "driveReDerived": true,
  "hi": 7735.775912565026,
  "lo": 7674.540230645026,
  "close": 7705.30892048,
  "stageNote": ""
 },
 "stage": "open60",
 "elapsed_min": 123,
 "DAYEHOD": [
  "7735.78",
  "",
  "303m"
 ],
 "DAYELOD": [
  "7674.54",
  "",
  "3m"
 ],
 "DAYHOD": [
  "7724.00",
  "",
  "48m"
 ],
 "DAYLOD": [
  "7676.00",
  "",
  "102m"
 ],
 "DAYEMUD": [
  "+61.2",
  "201m",
  "3062"
 ],
 "DAYMUD": [
  "-14.0",
  "54m",
  "700"
 ],
 "basis_note": "chart price = ES price + (chart close at the SCALEREF minute 37980 - SCALEREF 7690.5); offset used here: +0.00"
}
```
