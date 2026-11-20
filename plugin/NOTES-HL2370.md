# lsHodLod 2.37.0 - new box top + spoken sweep alerts (2026-11-19, 19:29-20:30 CT)

Rassul approved the mockup `hodlod-new-box-mockup.html` at 19:29 CT ("ok.. do it"). At 19:28 he said "if there are too many signals you will have to adjust font size". At 19:33 he said "switch positions of sweeps and Cobs". He also approved the spoken sweep alert recommendation.

## Part A - the box layout

**The Now row** sits just above the box, in the same spot as in 2.36.0. Left to right:

- The gamma pill (`+GAMMA` / `-GAMMA`), at the far left. It moved out of the box header and is left-aligned with the box's left edge.
- `0DTE Exp 3:00 PM`. This is the header's old expiry text with "0DTE" added. It still turns amber in the last hour and adds "done" after expiry.
- The Now line (NowLevels 1.0.0), centred and unchanged.

If the left part would touch the Now line, the expiry drops first, then the pill. The Now line is never covered. The pill and expiry are drawn even when the Now line has nothing to show. The confirm badges stay clear of the whole row.

**Line 1 inside the box is the signal line.** It shows the 2.35.0 tags: the latest Cob, Sw and BkTst for each market.

- They sit in three groups, after the 19:33 change: **Cob** on the left (left-aligned), **Sw** in the centre, **BkTst** on the right (right-aligned).
- Tags are newest first within each group.
- Fills are green for bullish and red for bearish, with black bold text.

When the line is crowded:

- Each group owns a third of the line, and it may borrow empty width from a neighbour.
- The steps are taken in this order: 11 pt, then 10 pt (his floor), then tighter tag spacing.
- Only after that does the oldest tag of the most crowded group drop. Once a tag has dropped, the line stays at 10 pt with tight spacing.
- Tags never overlap and never extend past the box.

This is in HodLodTop.h 1.0.0.

**Line 2** is `ES  Close 4:00 PM │ LTD Dec 18 (29d) │ NQ -9%  ES -3%` on the left and `[Norm?]  v2.37.0` on the right. Hrly, 3m and Day are gone. The gamma pill and the expiry have moved to the Now row. The Range Day pill stays. The market moves drop off first when the line is narrow.

**The tables** (LOD SWEEPS | SESSION | HOD SWEEPS) are unchanged. The box height is the same as in 2.36.0: 24 scenarios were compared against the 2.36.0 logic, and the heights are identical. The signal row is always there, so the height does not change when tags come and go.

## Part B - spoken sweep alerts (SweepVoice.h 1.0.0)

A clip plays when a Sw mark is **confirmed at its candle close**. The marks are the `Sw` lines in `lsFlexLevels\RevRead\wy-<MKT>-<spb>.txt`, the same file the Now line reads. Only the marks drawn on his chart count:

- The level must be a named key level: ONL/ONH, LonLO/LonHI (LLO/LHI), AsiaLO/AsiaHI (ALO/AHI), PDH/PDL, PFH/PFL, VAH/VAL, PWH/PWL, CWH/CWL, PHH/PHL, 2hH/2hL, 3hH/3hL. DPoc, WPoc and Open are not spoken.
- The strength must be V/R 10 or more.
- The Sw colour rule applies: the candle must be green for up and red for down, checked against the chart's own bars when the candle is known.

The message is `<market>, sweep, <level>, <up|down>`. For example, `NQ, sweep, PDL, up` is a bullish sweep of a low.

The clips are pre-rendered: `%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\Voice\sw-<MKT>-<LV>-<up|down>.wav`. That is 7 markets × 22 levels × 2 sides = 308 clips, about 2 s each. They were made with Kokoro af_heart (the LRA voice, the same engine as tools/audio/explain_audio.py), 24 kHz, 16-bit mono. They play with `PlaySoundA(file, NULL, SND_FILENAME|SND_ASYNC|SND_NODEFAULT)`, and HodLod.cpp has `#pragma comment(lib, "winmm.lib")`.

Rules:

- One alert per market + level + side per session (5 PM to 5 PM).
- A mark only speaks if its candle ends after this chart started. Marks already present at chart load or reload never speak.
- RTH only: the bar's minute must be inside the market's RTH, per hl::MKTS.
- A mark first seen more than 5 minutes late stays quiet.
- A quiet mark (outside RTH, late, or wrong colour) does not use up its level's alert. An overnight PHL sweep does not silence the RTH one. A bug was caught and fixed here by the real ES test.
- There is one hub per DLL, a static shared by every chart. Several charts or timeframes of a market speak once.
- A queue keeps clips from talking over each other: each clip waits for the last one's play time plus 350 ms.
- A missing clip plays the "SystemAsterisk" sound instead. This is counted and logged.

Status line: `SWEEP_VOICE,1.0.0,<spoken count>,<last text>,queued here,fallbacks,last missing,skipped rth / old / colour,<chart start + marks>`. Each alert is also logged to `lsFlexLevels\HodLod\SweepVoice.log`. Two more status lines were added: `NOW_ROW` (what the left part kept) and `SIGNAL_LINE` (font, tight spacing, tags shown and dropped).

No settings change.

### Spoken text (pronunciation check)
Every clip's text was run through the Kokoro phonemizer, and the full table is in `Voice\spoken.txt` (file, text, phonemes, length). Letters are spelled. A bare "A" is read by Kokoro as a schwa ("V A H" came out as "vee-uh-aitch"), so VAH and VAL use "V eh H" and "V eh L", following the lexicon's own rule. There are no digits or Roman numerals in the spoken text.

| Market / level | Shown | Said |
|---|---|---|
| ES / NQ / CL / GC / HG / NG | ES ... | "E S", "N Q", "C L", "G C", "H G", "N G" |
| EU | 6E | "Euro" |
| PDL / PDH / PFL / PFH / ONL / ONH / PHL / PHH / PWL / PWH / CWL / CWH | as is | letters: "P D L" ... |
| VAH / VAL | VAH / VAL | "V eh H" / "V eh L" (said "vee ay aitch") |
| LonHI / LonLO | LonHI / LonLO | "London high" / "London low" |
| AsiaHI / AsiaLO | AsiaHI / AsiaLO | "Asia high" / "Asia low" |
| 2hL / 3hH | 2hL / 3hH | "two hour low" / "three hour high" |

Examples: "N Q, sweep, P D L, up." · "Euro, sweep, London high, down." · "E S, sweep, two hour low, up." · "G C, sweep, V eh H, down."

## Tests
- tests_hodlod/t_hl2370.cpp: 5,884 checks, 0 failed (`-Wall -Wextra -Werror`, ASan/UBSan). It covers:
  - Signal line: 40 widths × tag counts. All four crowding steps are seen, tags never drop before 10 pt with tight spacing, the right tags drop, group order and alignment hold, colours are right, nothing overlaps, and groups borrow width.
  - Box: 288 layouts. No Hrly, 3m, Day, gamma or expiry inside the box; line 2 contents and order are right; the height is fixed.
  - Now row: keep / drop order, and the mockup case.
  - Voice:
    - parse, canonical names, strength
    - 308 unique clips, spoken text
    - once only, no replay on start or reload, two charts of one market, the other side and other markets
    - RTH edges (3:00 PM speaks, 3:03 PM does not), late marks, colour, a new session
    - queue order and spacing, the bounded queue, the missing-clip fallback
    - WAV play time read from a real clip
  - His real ES 11-19 wy file, replayed minute by minute:
    - Only the **8:42 AM PHL V99+** sweep speaks ("ES, sweep, PHL, up").
    - The overnight and pre-RTH sweeps (7:36 PM, 9:18 PM, 12:06 AM, 5:33 AM) stay quiet.
    - A chart opened at 9:00 AM replays nothing.
    - The **9:42 AM PDL** event was a live SWEEP on the Now line, but his file has no Sw mark for it. A V5 mark would stay quiet and a V12 one would speak.
  - Versions and the gex-build line.
- Same box height as 2.36.0 in 24 of 24 scenarios (compared against the 2.36.0 HodLodLogic.h).
- t_hl2360: everything passes except its two 2.36.0 version checks. t_hl2350's top-line badge layout checks are replaced by this design.
- MinGW `-fsyntax-only` against the real irtsdk.h: 0 errors. The only warning outside the SDK is the existing one in first_extreme_inference.cpp.
- check_win32_libs: PlaySoundA → `winmm.lib` pragma in HodLod.cpp. Only HodLod.cpp calls it; SweepVoice.h is SDK-free.

## Differences from the mockup
- The signal tags start at 11 pt (one step above the box's 10 pt) so the font can step down before tags drop, as the 19:28 crowding rule asks. The mockup draws them at the box's size.
- "Norm?" keeps its Range Day pill outline, as today. The mockup shows plain grey text.
- The group order follows the 19:33 change: Cob left, Sw centre, BkTst right.

## Not verified - needs his IRT
- The look on his charts. Screenshots are needed after the restart.
- That PlaySound is audible on his PC's default output.
- That a real RTH sweep speaks once with several charts open.
