# lsZoneDefense 1.0.1 + lsZoneDefenseMarks 1.0.1 - numbered, aligned Read block (2026-11-08)

Rassul 2026-11-08 (mockup claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G, "Pane before / after"):
- The Read block gets the Zone line and every line is NUMBERED in event order:
  `1. Zone` (range · lots · oldest order at the touch; held green / broken or faded red / forming white), `2. Queue`,
  `3. Pressure` (renamed from "Reload": the selling pressure into the zone in the 60 s after the touch; verdict Buyers holding /
  Bid exhausted / Quiet), `4. Refill`, `5. Sweep`.
- "Look nice": labels in one fixed left column (grey), the verdict word at ONE x in its colour and bold, the detail on the same line
  in a calmer tint of the same colour (green 8FD6AB / red F2A0A0 / white A9B2BD), times 12-hour in one right-aligned column (grey),
  even 14 px rows, nothing drawn under the time column (the detail is cut with "..." first). The sold / bid-left bars are dimmed
  (6E2A2A / 3F8F5F, 3 px dots) and start under the text. No hovers.
- The gray diamond on the price chart marks the refill bar (settings row renamed "Refill diamond", key show_diamond unchanged).
- Status line: ZONE_DEFENSE_PARAMS reports pressureX / signs pressure (journal line codes unchanged: ZR = the pressure decision).
- Engine, journal and parity unchanged (ZoneDefenseLogic.h only bumps zd::VERSION).

Tests: tests_zd/t_zd.cpp 40 passed (5 numbered lines in order, no overlap, labels / verdicts / times each in one column, even rows,
colours, dimmed bars, live vs close, no repaint across a restart, parity with the engine on CL / HG real slices); t_marks.cpp 10 passed;
parity_zd.py HG 56 / 56 unchanged; settings contract PASS.
