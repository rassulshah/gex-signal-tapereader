# lsBreakout / lsBreakout1 1.16.0 - every pullback label needs >= 38.2% (2026-11-09)

Rassul 21:06 CT "see this pb, it is too low" (ES 11-09 BD Pboc ~14:45 chart time: 27.6% of the BD wave); 21:08 "all pullbacks including pb pboc pdoc
are required to be at least 38.2%".

Rule (BreakoutLogic.h, Engine::deepPb, PB_MIN_RETRACE = 0.382):
- The wave = the BO / BD wave as drawn: from its start pivot bar (tw0, the last confirmed ZigZag pivot of the other kind before the BO / BD candle)
  to its end bar (tw1 / topT, the first wave pivot at or after the candle; its extreme so far while unconfirmed). The same bars the PB POC gate uses.
- Up wave (BO / Rly up, pullback pivot LOW): (wave high - pivot low) >= 0.382 x (wave high - wave origin low). Down wave mirrored.
- BO / BD Pb, Pboc, Pdoc (incl. provisional Pbs and the no-VAP fallback Pb): checked right after the BO-candle overlap test. A shallower pivot
  gets no label and does NOT count toward the 2 per BO / BD (a later deep pivot can take the slot).
- No-BO Rly Pboc / Pdoc: checked before the 2-per-wave count, on the Rly wave (origin bar -> end bar). Shallower: no label, not counted.
- Pdoc keeps its own extra rule (the PB POC itself >= 38.2% deep, 1.15.0).
- Decided at the pivot confirmation, never repainted. No wave origin known -> no label.
- Status line PDOC adds "pullbacks shallower than 38.2% of their wave (no label, 1.16.0): after BO / BD n / Rly n" (this pass).
- Fewer Pbs also means fewer Revs (a Rev needs its Pb).

Impact (Oct 1 - Nov 9, IRT 3m bar store, VAP-free replay of BO / BD Pbs; trade = enter at the pivot decision close, target = the BO wave end,
stop = beyond the lower of pivot / BO candle; 100-bar cap):
  ES kept 257 67% +0.24R | removed 28 64% -0.10R | freed slots 5 40% -0.16R
  NQ kept 290 56% -0.03R | removed 28 82% +0.09R | freed slots 3 33% +0.43R
  CL kept 260 61% +0.12R | removed 32 69% -0.13R | freed slots 4 50% +0.14R
  GC kept 282 52% -0.12R | removed 32 75% -0.01R | freed slots 1 100% +0.24R
Removed pivots: median retrace 0.30-0.32. Pooled removed avg -0.04R vs kept +0.05R. Rly labels need VAP: checked on the ES fixtures only.
ES 11-09 BD pivot (bar store 15:15 end-time; the screenshot's Pboc) retrace 0.268 -> removed (it was a -1R loss).

Tests: tests_bo100/test_bo1160_retrace.cpp (new, 36/36 under ASan/UBSan): deepPb hand cases; every decided Pb / Pboc / Pdoc / Rly Pboc /
Rly Pdoc in the ES 11-09 / 10-21 / 11-04 fixtures >= 38.2% (each type present); shallow pivots skipped; ES 11-09 20:03 gone; no-VAP fallback
Pbs all >= 38.2%. test_bo1130_pdoc 56/56 (20:03 now no label), test_bo1140_entry 0 fails; older suites fail the same checks as 1.15.0.
MinGW vs the real irtsdk.h: Breakout and Breakout1 0 errors, 0 warnings outside the SDK.
