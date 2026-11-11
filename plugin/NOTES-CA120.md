# lsCandleAbsorb 1.2.0 (2026-11-11 CT) - settings: letters off by default, markings by choice
Rassul 2026-11-11: "Update the candle absorb indicator so that it doesn't display designers by default and only displays the dash marking but the
setting allows you to select the signals you want displayed along with markings like delta poc (default) as well as volume poc and bid poc and ask
poc." 13:41 "Go" (mockup https://claude.ai/artifact/5MxPdNftvMteQ2pyJCGnHN), 13:48 "Build".
- Settings window (CandleAbsorbSettings.h, file lsFlexLevels\CandleAbsorb.settings-<MKT>.txt, per market): Signal A / Ae / D / S (all OFF by default),
  Delta POC dash (ON), Volume POC dash, Bid POC tick, Ask POC tick (OFF). NO_UI flag dropped; parameter version 2.
- Volume POC = the row with the most volume (white dash; 2 px wider a side when it sits on the delta POC row and both show).
  Bid POC = the row with the most traded at the bid (short red tick on the body's left edge). Ask POC = most at the ask (green tick, right edge).
- A letter switched off is hidden only: it is still decided and written to the record (CandleAbsorb\<MKT>-<spb>.csv) - nightly scoring unchanged.
- Tests: tests_candleabsorb/t_ca120.cpp 11 pass (real ES 11-09 bars + VAP; all letters on + delta only = the 1.1.1 scene byte for byte; records
  identical; POC rows = brute force). MinGW real SDK 0 errors. Settings contract 49,339 checks pass (26 tables, CandleAbsorb added).
- gex-build: the CandleAbsorb line lists the settings headers; the stale "after 3:00 PM" wording removed from the PENDING lines.
- NOT verified on his charts: the settings window itself (open it once: right-click the indicator), tick sizes at his bar width.
