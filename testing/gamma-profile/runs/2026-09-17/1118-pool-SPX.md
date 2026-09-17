# lsGammaProfile regression — 2026-09-17 11:18:47 CT — PASS

_CSV ASOF 11:18:47 · panel 16.40 · King 7650 (-) · ratio 1.0090512730849373 · spot 7635.46 (trinity) · SCALEREF 7704.12 · flip 7588.34 · CW 7645 · PW 7510 · rolls 3_

**Operator / runner note:** first 16.40 export: the pool on the SPX file

## Timing

- ✅  CSV ASOF 40727.0 and audit ct 40727 are the same export (<=5s)

## Gate A — Atlas (as read) -> CSV

- ✅  SPX->ES ratio present in the audit: 1.0090512730849373 (front-month basis +69.2 pts on the King)
- ✅  STRIKE rows (100) == tape strikes the panel read (100)
- ✅  every STRIKE row is a strike the panel read from the tape
- ✅  every ES price = round(SPX x ratio, 0.25)
- ✅  every %King equals the tape (King forced to +/-100 by its sign)
- ✅  ranks 1..10 follow |%King| (ties to the lower strike)
- ✅  every STRIKE row's pooled rank (field 8) == the SPY + SPXW pool re-derived from the audit (100 SPX + 100 SPY rows)
- ✅  the pool's five (SPY 763 SPX 7650 SPX 7655 SPY 760 SPY 764) == the audit's
- ✅  exactly one King flag, on the tape King 7650
- ✅  KING row (7719.25) == the flagged strike's ES
- ✅  SCALEREF 7704.12 == the ES1 payload spot the panel read (7704.1163)
- ✅  SCALEREF within 200 pts of the King (sanity)
- ✅  FLIP row = companion 0DTE 7588.34 -> ES 7657.00, window 0DTE
- ✅  CW row = companion 0DTE 7645 -> ES 7714.25, window 0DTE
- ✅  PW row = companion 0DTE 7510 -> ES 7578.00, window 0DTE
- ✅  REGIME sign POS: spot 7635.46 vs flip 7588.34 (+47.12) -> expected POS
- ✅  REGIME type is one of the six (TREND_UP, high)
- ✅  REGIME row == the audit's regime read
- ✅  spot source named: trinity (spx 7635.46)

## Atlas screen -> panel

- ⚠️  no --atlas-read: the Atlas screen was not transcribed for this run

## Gate B — CSV -> IRT chart

- ⚠️  no --irt-read: the IRT screen was not transcribed; the expected chart below is the checklist

## Expected chart (the Gate B checklist for the IRT screenshot)

```json
{
 "atlas_pool": [
  {
   "book": "SPY",
   "strike": 763.0,
   "pct": -100,
   "es": 7710.75,
   "badge": 1
  },
  {
   "book": "SPX",
   "strike": 7650.0,
   "pct": -100,
   "es": 7719.25,
   "badge": 2
  },
  {
   "book": "SPX",
   "strike": 7655.0,
   "pct": 65,
   "es": 7724.25,
   "badge": 3
  },
  {
   "book": "SPY",
   "strike": 760.0,
   "pct": -33,
   "es": 7680.5,
   "badge": 4
  },
  {
   "book": "SPY",
   "strike": 764.0,
   "pct": -23,
   "es": 7721.0,
   "badge": 5
  }
 ],
 "tags": {
  "7650": "K",
  "7655": "C",
  "7635": "F"
 },
 "top5": [
  7650.0,
  7655.0,
  7645.0,
  7635.0,
  7630.0
 ],
 "cw": "7645",
 "pw": "7510",
 "flip_between": [
  7585.0,
  7590.0
 ],
 "brackets": {},
 "named": {
  "7655": "R"
 },
 "air": [],
 "regime": "REGIME  +gamma  |  TREND UP  |  FOLLOW, don't fade  !CONFLICT",
 "basis_note": "chart price = ES price + (chart last close - SCALEREF 7704.12)"
}
```
