# lsRevRead / lsRevReadMarks 0.23.0, lsVolRead / lsRangeRead 1.11.0 - the AR bar (2026-11-23)

## Requests (Rassul, 2026-11-23 CT)
- 09:25 "i want you to implement the AR which is an automatic rally bar that occurs after a cx . so if there is a climax, and there is a bar that has its low below the pivot high and is within 3 bars of the pivot and has a BRMax of at least 10. so if the cx was in a downwave , the ARbar would have to be green . do you understand and show me a mockup"
- 09:26 "the AR bar should also have a strength line"
- 09:26, circling the green NQ 6:51 AM candle on the Cx mockup (Cx 6:45, pivot low 6:48): "here is a decent example. see the AR bar"
- 09:34 "go" on the mockup (ar-mockup.html: Without AR / With AR on NQ 11-23 5:57-7:12 AM, NQ 11-20 8:15-9:03 AM, ES 11-20 12:06-1:45 PM) and the estimate (2.5 to 3 h, $0, one restart).
- Asked with the mockup and NOT answered before the build: when the Cx sits on the candle before the pivot, does the pivot candle itself count (ES 11-20 1:00 PM, the wide red pivot candle after the 12:57 PM Cx)? Built as "no": the AR is one of the 3 candles AFTER the pivot candle.

## The rule (LevelMarks 1.13.0: arCands + arPick) - bullish shown, bearish = mirror
1. There is a drawn Cx (the price chart's badge; +1 = at a pivot low, the climax of a down wave).
2. The AR is one of the 3 candles after the PIVOT candle (the Cx is on the pivot candle or on the candle before it).
3. It is green (close above open; close = open is not green).
4. It overlaps the pivot candle: its low is below the pivot candle's high.
5. Its BRMax is 10 or more (the Range pane's count on the range in ticks: the larger of "all candles" and "same colour").
- The first candle that passes is the AR; one per Cx, one per candle. After a Cx at a pivot high: a red candle whose high is above the pivot candle's low.
- Strength line: the Bk / Sw line of that candle (V or R with the larger count, volume wins a tie): NQ 6:51 AM = "R51".
- Known when the AR candle has closed and the pivot is confirmed (NQ this morning: 6:57 AM, with the Cx badge); a candidate's BRMax is fixed at its close and candidates are tried in order, so the AR never changes after.

## What changed
- **LevelMarks.h 1.13.0**: `arCands()` (per session, from the session's candles: the Cx's pivot, the candles with colour + overlap) and `arPick()` (the first with BRMax 10+, from the chart's closed candles as fillStrength; the strength line); `seqText('A')` = "AR".
- **RevRead.cpp 0.23.0** (pane half): collects the candidates next to `seq()`, picks after the strength lines, writes `stamp|AR|dir||0|strength` into `lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt` (and the session file). Four lines of host code. The live session gets its ARs after the restart; sessions already frozen keep their files (no AR added to past days).
- **WyMarks.h 2.5.0 / RevReadMarks.cpp 0.23.0** (marks half): the badge "AR" + its strength line, green or red by its side, over a green candle / under a red one, laid out with the other badges (it may share a candle with a Bk / Sw / Cx: two marks).
- **VolRangeLogic.h 1.11.0 / VolRead.cpp / RangeRead.cpp**: the Range pane shows "AR" over that bar with its number from 10 up (as a Bk / Sw bar), the bar in its candle's colour; the Volume pane shows no AR. A Bk / Sw on the same candle keeps the pane's label; CB / SR / Ct stand over an AR. Status: new `AR,1.11.0 ...` line.
- NOT changed: Bk / Sw / Cx / I / FT / Tst / Rev, the pane's letter strip (B / C / I only), the voice, the Now line, the signal line, lsTurn, lsBreakout (it uses the Bk marks only), the Cx rule of 1.10.0.
- gex-build.bat is not edited: lsRevRead, lsRevReadMarks, lsVolRead, lsRangeRead (and lsBreakout / lsBreakout1, which list WyMarks.h) rebuild by themselves.

## Tests (2026-11-23, research machine)
- tests_levelmarks/t_ar.cpp (ASan / UBSan, -Werror, C++17 and C++14): 59 checks, 0 failed. Real NQ candles with the real Cx badges of the marks file: 11-23 6:51 AM = AR green R51 (his example; belongs to the 6:45 Cx, pivot 6:48); 11-20 8:33 AM red V99+ and 8:48 AM green V99+ (the 2nd candle after its pivot, 8:45 passed over); 11-23 2:12 AM red; every AR has the colour, BRMax 10+, a strength line and a Cx; one per Cx. ES 11-20 12:06-1:45 PM: none. Candle-by-candle replay from Sunday 5 PM: an AR never changes, never leaves, never shows before its candle closed. Hand-made: BRMax 9 no / 10 yes; a red candle passed over; close = open not green; low AT the pivot candle's high no / one tick below yes; the 3rd candle yes / the 4th no; the Cx on the candle before the pivot; a Cx elsewhere, of the other side, not drawn any more or with its pivot not confirmed: nothing; the mirror; one AR per candle.
- The plugin code against the mockup's prototype on every candle: NQ 11-17..23, ES 11-14..23, HG overnight: 20 of 20 ARs identical (candle, side, Cx); 64 Cx badges.
- tests_wymarks/t_wymarks_ar.cpp: 14 checks, 0 failed (the line as the pane writes it; the badge parsed with its strength; AR + Bk on one candle; the file's "A" letters are not AR; the tag over a green candle, under a red one, covering nothing). t_wymarks 42 and t_levelmarks 95: 0 failed, old and new header.
- tests_volrange/t_vr1110.cpp: 28 checks, 0 failed (Range: AR on the three candles, drawn once over the 6:51 bar with its 51; Volume: none; Bk over AR; the number from 10; no lines / no file: nothing). t_vr1100 37, t_vr150_split 69, t_vr180 26, t_vr190 58, test_volrange_130 26: 0 failed. Panes 1.10.0 against 1.11.0 on the real files (no AR lines yet): identical drawing on 19,484 candle-panes.
- tests_levelmarks/t_ar_readers.cpp: 7 checks, 0 failed - the spoken sweeps, the Now line, the signal line, lsTurn's sweeps and levels and the panes' Bk / Sw / Cx return the same thing from his real NQ file with 20+ AR lines added.
- Real irtsdk.h syntax check, C++14 and C++17, -Wall -Wextra: RevRead.cpp, RevReadMarks.cpp, VolRead.cpp, RangeRead.cpp 0 errors; warnings outside the SDK 6 / 3 / 0 / 0 - the same as before the change. Mock SDK compile (VolRead, RangeRead), -Werror: ok.

## Not verified (needs his IRT)
- The badge on his charts after the restart. Check: `wy-NQ-180.txt` must carry `...|AR|1||0|R51` on the 6:51 AM candle of 11-23 (stamp 3878264460) once the pane has run; `RangeRead.status-<MKT>-180.txt` must say VERSION,1.11.0 with the AR line.
- The pane half (RevRead.cpp) was compiled, not run: no host harness for it here. Its four new lines only call the two tested functions and write the line the tests parse.
- How the badge sits next to a Cx / Bk badge at his zoom (the layout engine is the tested one; the real font widths are IRT's).

## Not done
- No nightly scoring of the AR yet (how often it holds, what follows). It is a label of what happened, shown as he defined it; say the word and it gets a scorecard.
