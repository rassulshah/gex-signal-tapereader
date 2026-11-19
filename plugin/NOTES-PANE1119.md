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
