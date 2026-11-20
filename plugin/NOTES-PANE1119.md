# Price-pane cleanup - 2026-11-19 (Rassul "1. ok 2. ok 3. ok", 16:58 CT; mockup price-pane-cleanup-mockup.html / mk10.py)

| Plugin / file | Version | Change |
|---|---|---|
| LevelMarks.h | 1.3.0 | Bk needs the BO candle rule (range >= prior, ties broken by volume); Sw needs the rejection colour; no DPoc / WPoc / Open levels. New `lvm::seq`: FT (within 3 candles, close beyond the Bk extreme), Tst (PB rules: overlap with the Bk candle, 38.2-100% retrace, no take-out of the Bk candle extreme, max 2 per move, the 2nd further back, names its level + %), Rev (PB Rev candle rule within 12 candles). |
| WyMarks.h | 2.3.0 | FT / Tst / Rev codes; badges with black text; FT a plain letter; preferred side per code. |
| WyRead.h | 1.7.1 | Read text says Sw / Tst instead of I. |
| RevRead / RevReadMarks / RevReadSettings | 0.20.0 | Draws FT / Tst / Rev, writes them to wy-<MKT>-<spb>.txt (`t|FT|dir`, `t|Tst|dir|lv|0|62%`, `t|Rev|dir||0|V35`). One Rev per candle: skips its Rev where CobSignal posted a Rev (board 'K', flag 7). RV_SHOW_I stays false. |
| CobSignalLogic.h / CobSignal.cpp | 1.10.0 | Cob and Lvp as badges; Cob tag = ZZO count + strength (from 20 up); only the 38.2% line; Rev as a badge with a strength line, flag 7 on the board. Cob Rev rule unchanged (test below). |
| Breakout.cpp | 1.29.0 | BKO_DRAW=false: no drawing (lsBreakout1 too), empty 'L' board; records continue. |
| ZzoCombo.cpp / ZzoComboMarks.cpp | 3.3.0 / 3.0.5 | ZC_SR_CB=false: no SR / CB badges. |
| WavePOC.cpp | 1.17.0 | Profiles the Bk waves (from the RevRead wy file) and Cob waves; BO / Rly waves off. |
| HodLod (cpp, Logic, Settings) | 2.34.0 | HL_READBOX_DRAW=false: no "Now" box (still built, logged). |
| VolRangeLogic.h / VolRead / RangeRead | 1.7.0 | Bk / Sw on the new rules (Range pane: size rule without the volume tie-break). |
| analytics/lra/revread_letters.py | 1.3.0 | LevelMarks 1.3.0 rules, lvm_seq port, Bk / Sw / Tst+Rev entry scores (entry_trades.csv, report section). |

## Cob Rev test (Oct-Nov, Cobs ZZO 10+, CobSignal records)
Current rule 103 Revs, 89% reached the Cob high, +0.03R; PB Rev candle rule 104, 87%, +0.00R. All shown Cobs 88% vs 85%. Kept the current rule (~3 weeks of records: small sample).

## Entry scores (research bars, in-sample, net)
Bk 3318, 39% win, -0.20R (t -9.2). Sw 992, 51%, +0.09R (t 2.1). Tst+Rev 591, 64%, +0.04R (t 0.9). Cob+Rev is not scored here (needs the CoB engine port).

## Tests (Linux, -Werror, ASan)
levelmarks 53/53 (parity vs mk10: ES identical; NQ adds an 11:03 FT from a 10:54 Bk VAL; ES 10:39 Bk VAL new), wymarks 42/42, cobsignal fib 29/29, htf 44/44, 190 6/6, lvp 26/30 (4 pre-existing), volrange_130 24/25 (1 pre-existing), volrange_logic 4 pre-existing failures, vr150_split 69/69.

## Not done
Cob+Rev nightly scoring; CL pane-height fix (not approved).

## 18:18-18:29 CT follow-up (Rassul "ok", "switch to 10", "lets keep it at 10 so the BRmax or BVmax should be at least 10")
| File | Version | Change |
|---|---|---|
| VolRangeLogic.h / VolRead / RangeRead | 1.8.0 | The panes no longer compute Bk / Sw: they show them on the candles the price chart marked (RevRead\wy-<MKT>-<secs>.txt, bounded / torn-safe / mtime-cached, no file = no Bk / Sw); the pane count stays its own BVMax / BRMax, on a Bk / Sw bar from 10 up. |
| LevelMarks.h | 1.4.0 -> 1.5.0 | Strength line from 10 up (1.4.0); NEW Sw rule (1.5.0): a Sw needs its attempt's BVMax or BRMax >= 10 (SW_MIN), else it is removed in fillStrength (so the wy file, the panes and the Reader never see it). A Bk is never removed by it. |
| RevRead / RevReadMarks / RevReadSettings | 0.20.1 -> 0.20.2 | strength from 10 (0.20.1); the Sw rule via LevelMarks 1.5.0 (0.20.2). |
| CobSignalLogic.h / CobSignal.cpp | 1.11.0 | Cob tag + Cob Rev strength line from 10 up. |
| analytics/lra/revread_letters.py | 1.4.0 | sw_filter: the same Sw rule in the nightly port (attempt strength = max over the attempt candles). |
Tests: tests_levelmarks 59/59 (5 new Sw-rule cases: BVMax 8 / BRMax 9 removed, V35 kept, attempt max counts, a weak Bk kept).

## 2026-11-19 ~18:40 CT - VolRead / RangeRead 1.9.0 + RevRead 0.20.3

Rassul 18:35 CT: "ok but color so it looks nice. build" (mockup vol-range-readbox-mockup.html).

- **VolRead / RangeRead 1.9.0** (VolRangeLogic.h): the read box keeps its 3 plain lines and gets the number column back next to them, for the
  candle under the mouse: BVMax / BV / BVG / BVR (Range: BRMax / BR / BRG / BRR). Labels grey #94a3b8, the Max bold and gold #f0c040 from 10 up
  (white below), G green, R red, the other colour's row a grey "-", a thin grey divider. Pane height unchanged; no room -> the column is dropped.
  Tests: tests_volrange/t_vr190.cpp 58/58 (widths 1180/900/640/420, inside the pane, no overlap, NQ 5:39 PM BVMax 15 gold), t_vr180 26/26, t_vr150_split 69/69.
- **RevRead / RevReadMarks 0.20.3** (WyFreeze.h, new): the repaint after 5 PM. Earlier sessions were recomputed with hour levels only, so their
  key-level marks (ES 11-19 9:42 AM Sw PDL V99+ ...) vanished from wy-<MKT>-<spb>.txt. Now each session's wy lines are written (atomic) to
  RevRead\wymarks-<MKT>-<spb>-<YYYYMMDD>.txt while it is live; once the forming candle is in a newer session the file is read back unchanged
  (+ only the candles that closed after its last write, e.g. the 3:59 PM candle at 5 PM) and replaces the recompute in the wy file. Torn / foreign /
  oversized file (512 KB) -> the old recompute. Sessions from before 0.20.3 have no file -> as before. The I / A / E letters of a frozen session
  in the wy file are frozen too (lsSDMO reads them from there); the pane's own strip is still drawn from the recompute.
  Test: tests_wyfreeze/t_wyfreeze.cpp 365/365 (a candle-by-candle replay across 5 PM keeps session A's lines byte-identical; restart; torn file;
  no file; a file written mid-session; a session read in 2 segments = one file; the real NQ wy body round trip). No settings change.
