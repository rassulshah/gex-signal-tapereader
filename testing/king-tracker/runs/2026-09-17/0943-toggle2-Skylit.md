# lsKingTracker regression — 2026-09-17 09:43:38 CT — PASS

_CSV ASOF 09:43:38 · panel 16.39 · SPOT 7700.0 · SCALEREF 7701.56 · elapsed 63 min · rolls {'SPX': 3, 'SPY': 2, 'IF': 2}_

**Operator / runner note:** toggle test step 2: Source flipped to Skylit in the dialog

## Timing

- ✅  CSV ASOF 35018.0 and audit ct 35018 are the same export (<=5s)

## Gate A — the panel's read -> CSV (re-derived from the audit)

- ✅  SPX: 4 KINGTRACK steps == audit rolls 3 + 1
- ✅  SPX: the steps are in time order
- ✅  SPX: no two consecutive steps on the same strike
- ✅  SPX: every step price / strike is the book's ratio (1.0091, 1.0091, 1.0091, 1.0092)
- ✅  SPX: KINGNOW 7679.83,7610,100 == audit kingNow (7679.8312 @ 7610)
- ✅  SPX: the last step is the live King (7610.0)
- ✅  SPY: 3 KINGTRACK steps == audit rolls 2 + 1
- ✅  SPY: the steps are in time order
- ✅  SPY: no two consecutive steps on the same strike
- ✅  SPY: every step price / strike is the book's ratio (10.1074, 10.1069, 10.1061)
- ✅  SPY: KINGNOW 7700.84,762,-100 == audit kingNow (7700.8405 @ 762)
- ✅  SPY: the last step is the live King (762.0)
- ✅  IF: 3 KINGTRACK steps == audit rolls 2 + 1
- ✅  IF: the steps are in time order
- ✅  IF: no two consecutive steps on the same strike
- ✅  IF: every step price / strike is the book's ratio (1.0091, 1.0092, 1.0091)
- ✅  IF: KINGNOW 7669.54,7600,100 == audit kingNow (7669.537940649273 @ 7600)
- ✅  IF: the last step is the live King (7600.0)

## Gate B — CSV -> IRT

- ✅  status file: Source = Skylit
- ✅  SPX: drawn=True under Source Skylit (CSV has 4 steps)
- ✅  SPX: the plugin loaded 4 steps == the CSV's 4
- ✅  SPX: the plugin's KINGNOW 7610.0 +100 == the panel's (7610 +100)
- ✅  SPY: drawn=True under Source Skylit (CSV has 3 steps)
- ✅  SPY: the plugin loaded 3 steps == the CSV's 3
- ✅  SPY: the plugin's KINGNOW 762.0 -100 == the panel's (762 -100)
- ✅  IF: NOT drawn under Source Skylit
- ⚠️  no --irt-read: the IRT lines were not transcribed; the expected chart below is the checklist

## Expected picture (the Gate B checklist for the IRT screenshot)

```json
{
 "SPX": {
  "steps": [
   [
    "00:00:02",
    7540.0,
    7609.19
   ],
   [
    "06:08:07",
    7550.0,
    7619.28
   ],
   [
    "08:54:39",
    7650.0,
    7720.2
   ],
   [
    "09:10:23",
    7610.0,
    7679.83
   ]
  ],
  "now": [
   7610.0,
   7679.83,
   "100"
  ],
  "ratio_now": 1.00918
 },
 "SPY": {
  "steps": [
   [
    "00:00:02",
    752.0,
    7599.78
   ],
   [
    "08:54:39",
    760.0,
    7680.63
   ],
   [
    "09:38:22",
    762.0,
    7700.84
   ]
  ],
  "now": [
   762.0,
   7700.84,
   "-100"
  ],
  "ratio_now": 10.10609
 },
 "IF": {
  "steps": [
   [
    "00:48:26",
    7550.0,
    7619.08
   ],
   [
    "08:33:02",
    7500.0,
    7568.62
   ],
   [
    "08:54:39",
    7600.0,
    7669.54
   ]
  ],
  "now": [
   7600.0,
   7669.54,
   "100"
  ],
  "ratio_now": 1.00915
 },
 "toggle": {
  "source": "Skylit",
  "drawn": [
   "SPX",
   "SPY"
  ],
  "offset": 1.19
 },
 "basis_note": "chart price = re-derived price + (chart close at the SCALEREF minute - SCALEREF 7701.56); offset used here: +0.00; the plugin refuses an offset > 300",
 "source_note": "lsKingTracker Source = Skylit draws SPX + SPY; Source = IF draws only the IF Magnet (0.11) \u2014 transcribe whichever the chart is on"
}
```
