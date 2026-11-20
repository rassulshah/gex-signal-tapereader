# lsHodLod 2.35.0 - 2026-11-19 (Rassul 17:27 "lets include sweeps too", 18:10 "build")

The right end of the HOD / LOD box's top line (market strength line) now shows, for EACH market, its latest **Cob** (the Rev after a drawn
Cob), latest **Sw** and latest **BkTst** (the Rev after a Tst of a Bk) - newest first, green / red badges with black text, e.g.
"ES Cob 3:18", "NQ BkTst PHL 2:21", "NQ Sw PHL 1:09". No plain Bk (nightly 11-19: Bk alone -0.20R). Replaces the 2.30.0 Cob-only section.

- TopSignals.h 1.0.0 (new, SDK-free): parses lsFlexLevels\RevRead\wy-<MKT>-180.txt (Sw; Tst then Rev = BkTst, the Tst's level) and
  lsFlexLevels\CobSignal\<MKT>-180.csv (REV of a SHOW Cob with ZZO 10+), current session from 5 PM, latest per market and type.
- HodLod.cpp: the 14 files read bounded (512 KB cap, stamp-cached, torn / oversized keeps the last good text) and parsed at most every
  5 s; the draw only copies the finished badges. Status line TOP_SIGNALS.
- HodLodLogic.h: badges right-aligned on the top line; the oldest drop when they do not fit (half the box width); the market moves fill
  the rest.
- Tests: tests_hodlod/t_hl2350.cpp 1536/1536 (parse, his 11-19 Cob records: ES 3:18 PM Cob Rev, NQ none; 45 layouts: inside the box,
  no overlaps, newest first, oldest dropped, black text on the side's colour; versions). MinGW syntax check vs the real irtsdk.h clean.
- Until IRT is restarted onto lsRevRead 0.20.0 the wy files have no Tst / Rev lines, so BkTst badges appear only after the restart.
