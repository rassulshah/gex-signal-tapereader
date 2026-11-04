# lsBreakout 1.3.0 - records from its own 3-bar decisions + the follow-through tie (2026-11-03 evening)

## Request
Rassul removed lsBigBidAsk from his charts (19:45 CT) and said "do it" to two changes:
1. lsBigBidAsk wrote the BO / BD and Rly decision records the nightly scores. lsBreakout must now write them from ITS OWN 3-bar
   decisions, in the same layout, plus a Pb record. One line per decision, never repeated on a restart / recompute (lsBigBidAsk's
   Oct 30 file held most fired lines 8 - 10 times). The ZigZag setting must be in the records.
2. The follow-through tie (the smallest fix he approved): the follow-through close may EQUAL the BO / BD candle's close. lsBreakout
   only; lsBigBidAsk's 1-bar behaviour stays exactly as it is.

## Records (BreakoutLogic.h recPlan / recMerge / recKey; Breakout.cpp records())
| File (under `%USERPROFILE%\InvestorRT\rtx\lsFlexLevels`) | Lines |
|---|---|
| `Breakout\<MKT>-<SYM>-<spb>-breakouts-<daykey>.csv` | `BBO|` 27 fields = `bba::boLine` (lsBigBidAsk's writer), one per decided candidate: fired / no_confirm / reason |
| `Breakout\Rly\<MKT>-<SYM>-<spb>-<daykey>.csv` | `RLY|` 17 fields = `rly::line` (lsBigBidAsk's writer), one per judged wave (3-min charts) |
| `Breakout\<MKT>-<SYM>-<spb>-pb-<daykey>.csv` | `BPB|t_pivot|side|t0|t_fire|t_conf|t_dec|pivot_high|pivot_low|bo_high|bo_low|bo_close|bo_pivot|version`, one per Pb |

- Names exactly as lsBigBidAsk built them: `<MKT>` = its marketName (EP -> ES, 6E -> EU, ...), `<SYM>` = its fileKey (the contract),
  `<daykey>` = its sessionOf (days since 1970, 17:00 CT starts the next day). The BO line's rth / file day = the decision candle's (as
  lsBigBidAsk); Rly = the wave's RTH day; Pb = its decision bar's day.
- The ZigZag setting: every line's version field is `1.3.0-zz3`, and every file starts with
  `# lsBreakout 1.3.0 | ES ESZ6 180 | zz_rev_bars 3 | ft_tie 1 | ...`, then lsBigBidAsk's column header. lsBigBidAsk's own lines
  carry `1.32.0` (1-bar), so the two can always be told apart.
- ONE line per decision: each line has a key (BO = t0 + dir, Rly = t_e + dir, Pb = t_pivot + side). Before any append the file is
  read and every key already in it is dropped; a key counts as written only after a good atomic replace (.tmp -> rename). So a restart,
  a recompute (history changed), two charts of the same market / contract / period, or two DLL objects never repeat a line, and a
  failed write is retried after 1 s without losing or doubling anything. First line per decision wins (as lsBigBidAsk's loader).
- Only the last 3 sessions (daykeys) are written. The records are never read back for a decision (the letters still come only from
  the chart's bars).
- Status file: new line `RECORDS,lsFlexLevels\Breakout (ES-ESZ6-180),zz_rev_bars 3,ft_tie 1,lines written N,pending P,write fails F`.

## Follow-through tie (BigBidAskLogic.h boWaves / boFromCandles `ftTie`, default false)
- Rule 5: BD needs a later red candle in the same wave with a lower low AND close <= the BD candle's close (was <); BO: green, higher
  high AND close >= the BO close.
- Only lsBreakout passes it (`Engine::ftTie`, on when zzRev > 1). lsBigBidAsk passes nothing (false): its decisions, records and parity
  are unchanged; `bba::VERSION` stays 1.32.0.
- Effect, ES Oct 20 - Nov 3 19:45 (3-bar, one full pass): fired 142 -> 144. The two added: Oct 23 01:48 BO, and Nov 3 19:06 BD
  (broke the 18:39 swing low 7553.25, BD candle close 7539; the 19:21 red candle low 7537.5 close 7539 = the tie; shown at the 19:21
  close) - plus its Pb at 19:21 -> the 19:06 BD. Three other fired BO / BD get an EARLIER follow-through candle (the letter appears
  sooner, same candle). Week fixtures: ES +1 added / 2 earlier, NQ 1 earlier, GC 2 earlier, nothing lost, Rly unchanged.

## Nightly scorer (LRA `analytics\lra\orderflow_badges.py` 1.3.1 -> 1.4.0)
- `read_bbo` reads `lsFlexLevels\Breakout\<MKT>-*-180-breakouts-*.csv` too and PREFERS it: for a day that has a Breakout file, the old
  BigBidAsk file of that day is not read. The old BigBidAsk files stay the history.
- FOUND: the old `read_bbo` only accepted the 20-field 1.21 / 1.22 lines, so every 24 / 27-field line written since Oct 22 (1.23+) was
  skipped - the BO / BD reads had no new data. Now `_bbo_row` reads 20 / 24 / 27 fields; from the old files it keeps 20-field and rule-5
  27-field lines (retired rules 1 - 4 skipped). `t` = the decision candle (follow-through close, no look-ahead), `t0` = the letter's
  candle, `zz` = 3 / 1, `src` = Breakout / BigBidAsk. First line per (candle, side) wins (was the last).
- No nightly module reads the Rly records (grep of analytics: only orderflow_badges reads BBO; nothing reads lsFlexLevels\Rly). The
  Breakout\Rly and Pb files are there for the next scorer.
- Tests: test_orderflow_badges 26 / 26 (new test_read_bbo_breakout_folder with real lsBreakout lines), test_signal_registry 11 / 11
  (1 skipped). The real plugin output of the record suite (220 BBO lines) reads back 220 / 220.

## Tests (tests_bo100/run_tests.sh <real irtsdk.h folder>, ASan / UBSan -Werror)
- test_bo100_parity 9 / 9: zzRev = 1 (tie off) still = lsBigBidAsk's recorded ES Oct 30 decisions, BO / BD 20 / 20, Rly 7 / 7.
- test_bo120_week (35 + new [4] / [5]): ftTie = false == the 1.2.0 expectations (`expected_wk3_*_strict.txt`); the tie changes only
  no_confirm -> fired or an earlier follow-through; [2] expectations updated (`expected_wk3_*`); [5] ES Oct 20 - Nov 3 19:45 one pass ==
  the independent tie harness (1069 / 1069 lines, `expected_eve_ES_full.txt`), fired 142 -> 144 = exactly the two above.
- test_bo130_rec 20 / 20 (new): names / headers / zz; every line parses with bba::parseBoLine / rly::parseLine and equals the engine's
  decision, one per decision (BO / BD 220, Rly 71, Pb 48 on ES Oct 28 - 30); no duplicate after a restart, a recompute, two charts on
  one DLL + a second DLL object (files byte-identical), a bar-by-bar feed + restart (append only); a blocked write is counted, retried,
  written once; a half-written file is completed without repeats. Mutation (dedupe off) fails 7 checks.
- test_bo100_host 22 / 22 (version 1.3.0; layout unchanged), test_bo110_pb 21 / 21, render check.
- MinGW vs the real irtsdk.h: Breakout.cpp and BigBidAsk.cpp 0 errors, 0 warnings outside the SDK.

## Build
No new includes (gex-build line unchanged). lsBreakout rebuilds (1.3.0); lsBigBidAsk also rebuilds because BigBidAskLogic.h changed
(same behaviour, same 1.32.0 - it is off his charts anyway). Installs when he exits IRT.

## Not verified - needs his IRT / the nightly
- The first real files after the install (Breakout folder, his contract name, daykeys).
- orderflow_badges still takes its BARS from lsBigBidAsk's candle records (`BigBidAsk\<MKT>-*-180-<day>.csv`). With lsBigBidAsk off the
  charts those stop, so from Nov 4 the BO / BD (and every other) read has no bars to score against until a bar source replaces them
  (e.g. the fp_bars research store). NOT changed here - needs a decision.
