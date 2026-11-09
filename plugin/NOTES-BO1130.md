# lsBreakout / lsBreakout1 1.13.0 - Pdoc (2026-11-09)

Rassul 14:09 CT: "Lets also have PBoc. It happens when the pb is testing only the wave pb poc. This is for rally also ... we cover pb which is
testing the breakout level and no poc, and when the pb is testing the wave poc which was prior reaccumulation before the breakout or rally or
the wave pb poc when the poc is not being tested and the market is currently reaccumulating". 14:12: "Change the name to pdoc where d represent
the developing poc. Now go". Mockup https://claude.ai/artifact/R8iksrRZTw7waYPhTR2eJC (ES 11-04 overnight: 00:48 Pb, 01:06 Pboc, 01:21 Pdoc).

## The three pullback labels
| Label | The pivot bar straddles (BO / Rly up: low <= X < high; mirror for BD / Rly down) |
|---|---|
| Pb | neither POC (the breakout-level test, unchanged rule) |
| Pboc (1.12.0) | the wave POC = the earlier reaccumulation (no-BO Rly: only with the POC at / below the wave's 50%) |
| **Pdoc** (1.13.0) | the wave PB POC = the pullback's developing POC, and NOT the wave POC = reaccumulation happening now |

## Pdoc rule
- **Developing PB POC** = lsWavePOC's wave PB POC (wpc::pullbackOf's rule), computed natively in lsBreakout (`Engine::pbPocTk`): volume at
  price of the bars after the wave end (BO / BD wave end, or the Rly wave end) through the pivot's **decision bar**; rows of the wave's row
  size (rowTicksFor(wave range)); POC = the biggest row (first), its centre. Exists only once the pullback has retraced >= 38.2% of the wave.
- Its value is taken at the decision bar and frozen with the label (never redrawn).
- No PB POC (-> plain Pb) when, at or before the pivot bar, price trades beyond the wave end, a 16:00 CT session end passes, or a new BO / BD or
  Rly fires; such an event after the pivot bar ends the profile there (the value lsWavePOC freezes). A close beyond the origin = invalid.
  (lsWavePOC also freezes on a Rev; lsBreakout's Rev is decided after its Pb, so it is not part of this rule.)
- Both POCs straddled -> **Pboc** wins.
- BO / BD Pbs: same Pb rule, max 2 per BO / BD. No-BO Rly waves: same pullback pivots as Rly Pboc (first 2, invalidation unchanged); a pullback
  that is not a Pboc (POC above 50% or not tested) can be a Pdoc.
- **No repaint**: a wave or pullback bar without VAP -> nothing drawn at that pivot; once IRT has it, Pb / Pboc / Pdoc is drawn once.
- An **Lvp** on the pivot still hides the letter. The thrust record (BRO) stays for Rly Pboc only.

## Records (lsFlexLevels\Breakout\<MKT>-<SYM>-<spb>-pb-<daykey>.csv)
- `BPB|...|version|boc|wave_poc|pdoc|pb_poc` - 18 fields (pdoc 1 / 0 / na, pb_poc when 1). 14 / 16-field lines still keyed.
- `BPD|t_pivot|side|t_rly_end|t_rly_start|t_conf|t_dec|pivot_high|pivot_low|pb_poc|wave_poc|origin|version` - a Rly Pdoc (key D|).

## Status
`PDOC` line: decided Pdoc after BO / BD, Rly, waiting for pullback VAP, drawn now.

## Tests (tests_bo100/test_bo1130_pdoc.cpp, replaces test_bo1120_pboc in run_tests.sh) - 60 / 60, real ES bars + VAP, bar-by-bar replay
- ES 11-09: 00:45 BD Pdoc (PB POC 7429.75, wave POC 7426.75 not tested; checked by hand in python); 07:15 / 23:24 / 00:33 Pboc and 07:45 / 20:03 Pb unchanged.
- ES 10-21: BD Pdoc 01:06 / 02:12 / 14:30; Rly Pdoc 22:12 (10-20) / 00:12 / 12:03 (3 BPD lines); Rly Pboc 03:27 / 08:15 unchanged.
- ES 11-04: BO Pdoc 01:21; Rly 08:30 Pdoc (POC above 50% so no Pboc, PB POC tested); 08:36 no label; Pboc 19:27 / 13:15 unchanged.
- Every Pdoc straddles its PB POC and not the wave POC; every Pboc straddles the wave POC.
- PARITY: the developing PB POC == lsWavePOC's own wpc::pullbackOf POC at the same decision bar for all 23 pullback pivots that have one.
- No repaint: no confirmed label ever changed; pullback VAP missing until 01:03 -> 00:45 drawn nothing, then Pdoc once; BPB written once.
- Lvp hides a Pdoc; status PDOC line; BPB 18 fields + key; BPD key.
- NQ: no NQ VAP history here (lsWavePOC's NQ fixture is 27 bars, too short for a BO) - not tested on NQ.

## lsWavePOC 1.10.3
Rebuild only (BreakoutLogic.h is compiled in); no change. t1100 26/26.
