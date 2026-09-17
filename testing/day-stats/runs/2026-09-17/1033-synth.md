# lsDayStats regression — 2026-09-17 10:33:00 CT — PASS

_CSV ASOF 10:33:00 · panel 16.37 · SPOT 7702.2 · SCALEREF 7690.5 · elapsed 123 min · rolls {'SPX': 0, 'SPY': -1}_

**Operator / runner note:** synthetic fixture from test_day_export.js (runner self-test)

## Timing

- ✅  CSV ASOF 37980.0 and audit ct 37980 are the same export (<=5s)

## Gate A — the panel's read -> CSV (re-derived from the audit)

- ✅  the audit carries AU.day (panel >= 16.37): yes
- ✅  DAYSA row re-derived from the audit == the CSV (HOD,7724.00,33480,48.0,3.0,9.0,31140,18,54.0,LOD,7676.00,36720,54.0,48.0,2400,, vs HOD,7724.00,33480,48.0,3.0,9.0,31140,18,54.0,LOD,7676.00,36720,54.0,48.0,2400,,)
- ✅  DAYSE row re-derived from the audit == the CSV (LOD,7674.54,30780,3.0,12.0,50.0,33600,23,201.0,HOD,7735.78,48780,300.0,61.2,3062,34.4,75.7 vs LOD,7674.54,30780,3.0,12.0,50.0,33600,23,201.0,HOD,7735.78,48780,300.0,61.2,3062,34.4,75.7)
- ✅  CONDE row re-derived from the audit == the CSV (pos60-bottom+orclock,3.0,303.0,66,108,39,29 vs pos60-bottom+orclock,3.0,303.0,66,108,39,29)
- ✅  CONDE basis pos60-bottom+orclock fits the stage (123 min -> pos60-bottom+orclock/pos60-middle/pos60-top+orclock/pos30-bottom/pos30-middle/pos30-top/pre-open)
- ✅  CONDE t2 303.0 > t1 3.0
- ✅  CONDE carries the 2ND ladder (39% last hr / 29% last 30)
- ✅  (16.36) the E row does not copy the actual 1ST clock/took after the READ (A 33480/48.0 vs E 30780/3.0)

## Gate B — CSV -> IRT

- ⚠️  no --irt-read: the IRT strip was not transcribed; the expected cells below are the checklist

## Expected picture (the Gate B checklist for the IRT screenshot)

```json
{
 "A_cells": [
  "A",
  "HOD 9:18am 7724",
  "48m",
  "3m",
  "9m",
  "8:39am",
  "18%",
  "54m",
  "51m",
  "LOD 10:12am 7676",
  "54m",
  "$2400  48.0p"
 ],
 "E_cells": [
  "E",
  "LOD ~8:33am",
  "~3m",
  "~12m",
  "~50m",
  "~9:20am",
  "~23%",
  "~3h 21m",
  "~4h 48m",
  "HOD ~1:33pm  39% last hr",
  "~5h 00m",
  "~$3062  61.2p"
 ],
 "stage": "open60",
 "elapsed_min": 123,
 "cond": {
  "t1": 3,
  "t2": 303,
  "lodPct": 66,
  "basis": "pos60-bottom+orclock",
  "n": 108,
  "lastHrPct": 39,
  "last30Pct": 29,
  "readIn": "HOD"
 },
 "basis_note": "A prices land on the chart (+0.00); E clocks = open + t1 / t2; the 2ND cell carries the ladder"
}
```
