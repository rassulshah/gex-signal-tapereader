# lsKingTracker regression — 2026-09-17 09:42:23 CT — PASS

_CSV ASOF 09:42:23 · panel 16.39 · SPOT 7700.0 · SCALEREF 7703.01 · elapsed 63 min · rolls {'SPX': 3, 'SPY': 2, 'IF': 2}_

**Operator / runner note:** toggle test step 1: IRT reopened on 0.14, Source as saved (IF expected)

## Timing

- ✅  CSV ASOF 34943.0 and audit ct 34943 are the same export (<=5s)

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
- ✅  SPY: KINGNOW 7701.05,762,100 == audit kingNow (7701.0452 @ 762)
- ✅  SPY: the last step is the live King (762.0)
- ✅  IF: 3 KINGTRACK steps == audit rolls 2 + 1
- ✅  IF: the steps are in time order
- ✅  IF: no two consecutive steps on the same strike
- ✅  IF: every step price / strike is the book's ratio (1.0091, 1.0092, 1.0091)
- ✅  IF: KINGNOW 7670.23,7600,100 == audit kingNow (7670.232252214952 @ 7600)
- ✅  IF: the last step is the live King (7600.0)

## Gate B — CSV -> IRT

- ✅  status file: Source = IF
- ✅  IF: drawn=True under Source IF (CSV has 3 steps)
- ✅  IF: the plugin loaded 3 steps == the CSV's 3
- ✅  IF: the plugin's KINGNOW 7600.0 +100 == the panel's (7600 +100)
- ✅  SPX: NOT drawn under Source IF
- ✅  SPY: NOT drawn under Source IF
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
    7599.99
   ],
   [
    "08:54:39",
    760.0,
    7680.84
   ],
   [
    "09:38:22",
    762.0,
    7701.05
   ]
  ],
  "now": [
   762.0,
   7701.05,
   "100"
  ],
  "ratio_now": 10.10636
 },
 "IF": {
  "steps": [
   [
    "00:48:26",
    7550.0,
    7619.77
   ],
   [
    "08:33:02",
    7500.0,
    7569.31
   ],
   [
    "08:54:39",
    7600.0,
    7670.23
   ]
  ],
  "now": [
   7600.0,
   7670.23,
   "100"
  ],
  "ratio_now": 1.00924
 },
 "toggle": {
  "source": "IF",
  "drawn": [
   "IF"
  ],
  "offset": -0.26
 },
 "basis_note": "chart price = re-derived price + (chart close at the SCALEREF minute - SCALEREF 7703.01); offset used here: +0.00; the plugin refuses an offset > 300",
 "source_note": "lsKingTracker Source = Skylit draws SPX + SPY; Source = IF draws only the IF Magnet (0.11) \u2014 transcribe whichever the chart is on"
}
```
