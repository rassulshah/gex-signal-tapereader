# lsDeltaProfile (DLT) 2.32.0 - no circles on the price chart (2026-11-03)

Request (Rassul, 16:12 CT, screenshot with a gray ring circled near 14:57 on ES 3m): "get rid of the circles that relate to the delta profile".

Which plugin drew it: lsDeltaProfile. The circled hollow gray ring is the 2.19.0 "gray circle" (C_GRAY_RING 0x9CA3AF, big node with
no winner). His ES status file at the time: GRAY_DRAWN 5, BUBBLES_DRAWN 2, shared A rings too (ONE_CIRCLE). lsCandleAbsorb draws text
codes only, lsBigBidAsk circles a print only on a shared-A candle in green / red (not gray), lsWavePOC / lsBreakout draw no gray rings.

What changed (drawing only):
- `PAINT_CIRCLES = false` in DeltaProfile.cpp: the paint loop no longer draws ANY circle - coloured A bubble, gray big-node ring, live A
  ring (forming candle), shared A ring with its letter A, and the leader tick of an offset bubble.
- Unchanged: every A decision (researchA, same / next candle), D / RW / AL / TR record lines, SharedAbsorb SAB lines, SignalBoard A / R /
  N / C / Y / H posts, Market State facts, the profile column and its ratio labels, Ex? marks, banner, combiner. Bubble layout is still
  computed, so the label / bubble disposition (and every status count) is identical to 2.31.0.
- Status: new line `CIRCLES,off (...),not painted N`; GRAY_DRAWN / BUBBLES_DRAWN / ONE_CIRCLE keep their 2.31.0 meaning (decided).
- Versions: DLT_VERSION = setVersion literal = dpset::PLUGIN_VERSION = 2.32.0; parameter version 17 unchanged (no settings change).

Tests: tests_dp217/test_dp232.cpp (61 pass, -Wall -Wextra -Werror ASan/UBSan): same-candle A, next-candle live A, gray / late A with
"Draw late A" ticked, shared A - zero ovals painted while the D line / SAB line / SignalBoard A final / status counts exist; 40 later
candles and a restart: still none, record unchanged. MinGW syntax check vs the real irtsdk.h: 0 errors (pre-existing
misleading-indentation warnings only). Superseded: the ring assertions in test_dp222 (8), test_dp223 (6), test_repaint216 (+5),
test_dp_bubble_halfway (now segfaults indexing an empty ring list). Pre-existing failures unchanged: test_dp228 (2), test_dp229 / 2291 /
230 / 2301 (8 each, READ strip removed in 2.31.0), test_columns280 (10), test_trap221 (14), test_offscreen271 (abort).

Not verified - needs his IRT: installs on File -> Exit; check BUILD-STATUS "installed" and VERSION,2.32.0 in DeltaProfile.status-ES-180.txt.
To bring circles back: set PAINT_CIRCLES = true (one line).
