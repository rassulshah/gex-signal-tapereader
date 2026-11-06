# lsHodLod 2.19.0 - Range Day pill (2026-11-06)

Rassul 14:54 CT "how do I identify if the market will have a big move today"; 15:08 "do it ... at the open and then 30 min later ...
hold cob longer ... buy pullbacks"; 15:11 "I don't want the big range day in the hod lod box to take another line. keep it compact and
near the gamma regime".

- One pill in the header, right before the gamma pill: `Big 61%` (green), `Small 56%` (red) or `Norm` (grey). No new line.
- Before open + 30 min: the pre-open read with "?" (`Big? 48%`). From open + 30 min: the 30-minute read, final for the day.
- Inputs computed natively in buildView: overnight range / ATR14, prior RTH range / ATR14, |gap| / ATR14 (> 4 ATR = roll = unknown),
  news day (HodLodEvents.h), first-30-minute range / ATR14 (Rth.rngAt[0]). ATR14 = mean of the last 14 RTH ranges (14 needed).
- Models: lsFlexLevels\LRA-RangeDay-<MKT>.csv written nightly by lra.range_day 1.0.0 (logistic, per market, refit on every session;
  held-out AUC pre 0.55-0.74, 30-min 0.61-0.76). Big = range >= 1.25x the market's median RTH range / ATR; Small <= 0.80x.
  Pill thresholds PILL,0.45,0.50 (file). No file / fewer than 14 sessions = no pill. Status line RANGEDAY (probs + inputs).
- Test tests_hodlod/t_hl2190.cpp (19 passed): C++ = Python probabilities on 105 real sessions; pill text; 1260 layout scenarios with
  the pill (drawn before the gamma pill, inside, no overlaps, sweep sections never dropped); with no pill the box is identical to
  2.18.0. t_hl2180 still passes except its 2.18.0 version check. MinGW syntax check 0 errors.
- Bug caught by the parity test: FILL lines of the pre-open models were skipped (field count), so the pre-open gap (unknown before
  the open) would have used 0 instead of the typical gap.
