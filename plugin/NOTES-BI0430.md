# BI (Break In) - lsRevRead 0.43.0 / lsRevReadMarks 0.39.0 / lsVolRead + lsRangeRead 1.17.0 (2026-11-27)

Requests (CT): 09:50 "consolidate AR and Vob (t) into a new BI (Break In) signal ... bvmax of at least 20 and its low is below the high of
the dnpivot and vice versa and it is coming from a pivot that has taken out a key level"; 09:55 his example (ES 11-27 LOD); 09:56 "its the
green one after the lod"; 09:57 "go"; 10:00 "ok" = BVMax OR BRMax 20+ (his example: BVMax 2, BRMax 99+); 10:08 "also add that the
downpivot just swept a level"; 10:09 "there were stops that were swept and after this there was a reversal bar called a breakin (BI) bar
that closes back above the level that was swept with strength indicated from the volume or range"; 10:10 the sweep trades below the level.
Mockup: BI-mockup.html (ES 11-27, real bars).

## The rule (LevelMarks 1.31.0 biCands / biPick) - bullish, bearish = mirror
1. A confirmed 3-bar High / Low ZigZag pivot LOW; its leg = from the pivot high it started from.
2. The leg traded BELOW a major LOW level (session ON / London / Asia, PD, PF, weekly, 3h S1 / S2) for the FIRST time this session part
   (since the RTH open in the day session) - the stop run; the close before it was above the level.
3. The BI = the first GREEN candle after the pivot candle, before the next pivot, low below the pivot candle's high, that CLOSES BACK
   ABOVE that swept level; not the RTH opening candle, not a data-release candle.
4. Its own BVMax or BRMax >= 20. One BI per pivot. Badge "BI" / the swept level / V or R strength.

## What changed
- LevelMarks.h 1.31.0: biCands / biPick (code 'N'); Bk / Sw / Cx / AR computed as before.
- RevRead.cpp 0.43.0: writes "stamp|BI|dir|level|px|strength" to wy-<MKT>-<spb>.txt; lsRevReadMarks 0.39.0 draws BI and no longer draws
  VOB / VOT or AR (their lines are still written so the nightly keeps scoring them).
- WyMarks.h 2.9.0: BI code; over a bullish BI, under a bearish one.
- VolRangeLogic.h 1.17.0 (lsVolRead / lsRangeRead 1.17.0): "BI" on both panes, first after CB; Sw / AR lines not marked.

## Not changed yet (tell him)
- HOD / LOD box: the SW signal row, the spoken alerts ("V O B" clips) and the Level line's live VOB / VOT word still come from the Sw
  marks. Switching them to BI needs new voice clips.
- BI is not yet measured per market (20+ a side).

## Tests
t_bi131 10 pass (his ES example R99+, swept-and-closed-back rule, no BI without the close back, mirror, colour, strength, no repaint on
prefixes); t_levelmarks 95, t_session122, t_gc1125 pass; t_ar / t_major / t_strength124: only their old version pins fail (as before);
t_wymarks 42 + t_wymarks_bi 5 pass (t_wymarks_ar: old version pin); t_vr1170 8 pass; t_vr1100 / t_vr180 / t_vr1110: the Sw / AR-on-the-
panes checks now fail by design (replaced by BI) + old version pins. MinGW syntax (real irtsdk.h): RevRead, RevReadMarks, VolRead,
RangeRead 0 errors outside the SDK.
