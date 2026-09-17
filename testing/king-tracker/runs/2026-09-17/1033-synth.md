# lsKingTracker regression — 2026-09-17 10:33:00 CT — PASS

_CSV ASOF 10:33:00 · panel 16.37 · SPOT 7702.2 · SCALEREF 7690.5 · elapsed 123 min · rolls {'SPX': 0, 'SPY': -1}_

**Operator / runner note:** synthetic fixture from test_day_export.js (runner self-test)

## Timing

- ✅  CSV ASOF 37980.0 and audit ct 37980 are the same export (<=5s)

## Gate A — the panel's read -> CSV (re-derived from the audit)

- ✅  SPX: 1 KINGTRACK steps == audit rolls 0 + 1
- ✅  SPX: the steps are in time order
- ✅  SPX: no two consecutive steps on the same strike
- ✅  SPX: every step price / strike is the book's ratio (1.0006)
- ✅  SPX: KINGNOW 7689.14,7685,100 == audit kingNow (7689.14 @ 7685)
- ⚠️  SPX: KINGNOW 7685 differs from the last step 7500.0 — a challenger dwelling (not yet confirmed KTRK_CONFIRM_N times), or the strip has moved since
- ✅  SPY: no KINGNOW row when the audit has no live King

## Gate B — CSV -> IRT

- ⚠️  no --irt-read: the IRT lines were not transcribed; the expected chart below is the checklist

## Expected picture (the Gate B checklist for the IRT screenshot)

```json
{
 "SPX": {
  "steps": [
   [
    "00:00:02",
    7500.0,
    7504.04
   ]
  ],
  "now": [
   7685.0,
   7689.14,
   "100"
  ],
  "ratio_now": 1.00054
 },
 "basis_note": "chart price = re-derived price + (chart close at the SCALEREF minute - SCALEREF 7690.5); offset used here: +0.00; the plugin refuses an offset > 300"
}
```
