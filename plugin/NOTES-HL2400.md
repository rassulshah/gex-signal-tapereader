# lsHodLod 2.40.0 - TODAY PLAN in the HOD / LOD box (2026-11-20)

## Request
- Rassul 09:43 CT 2026-11-20 "this looks good. build" on hodlod-box-plan-mockup.html: inside the HOD / LOD box replace LOD SWEEPS (left)
  with "BUY AT · 3-HOUR SUPPORT" and HOD SWEEPS (right) with "SELL AT · 3-HOUR RESISTANCE" = the MQ Reader's TODAY PLAN for this chart's
  market (his Reader 09:33: ES "S1 7,105  PS0+PS  NEAR  T1 7,150.00", sell side "-"). Rows must match the Reader for the same moment.
- Names he uses from now on: the Now line = the "Level line", the gamma line = the "Options line" (no on-chart text change).

## Where the plan comes from (the Reader's own sources, read natively, never the network)
- 3-hour R1 / R2 / S1 / S2, standing flags, swing dir + extreme: lsFlexLevels\CobSignal.status-<MKT>-180.txt HTFNOW (written by
  lsCobSignal; the bridge's lra.hodlod_box 1.9.0 reads the same line for the Reader). Stale (> 14 h old) = no plan, as the bridge.
- Live price (price_live, else price), expected move (em), the ladder (code, price, price_fmt, mq_only, mq_zone) and the time (now_ct):
  the Reader's state file C:\Dev\level-reversal-analytics\data\menthorq\reader\<session>\state.json (the bridge writes it with every Reader
  build and serves it as /api/reader "state"). Only session / now_ct / markets.<MKT> are parsed (bounded JSON reader, 16 MiB cap, ~20 ms
  for today's 3.3 MB file); re-read only when its stamp changes, checked at most every 20 s; torn / garbled reads keep the last good copy;
  another session's file = "-".
- Rules = menthorq-reader.user.js todayPlanHtf() line(), ported 1:1 (TodayPlan.h 1.0.0): standing + NEAR (<= 0.25 EM) or SPRING / UPTHRUST;
  tags = ladder rows within 0.06 EM (LH / LL -> LonHI / LonLO, AH / AL -> AsiaHI / AsiaLO, + MenthorQ abbreviations; MQ-only rows = their
  abbreviations), T1 = nearest ladder price >= 0.25 EM beyond, S2 / R2 skipped when equal to S1 / R1; level text = the bridge's px_txt.

## What changed
- TodayPlan.h 1.0.0 (new, SDK-free). HodLodLogic.h: View plan rows; the side sections draw the plan when planOn: header strip (green BUY /
  red SELL title, plan time right), column heads Level / At / (chip) / Target, rows: level in the side colour, tags GREY (his colour rule),
  chip NEAR (#9a3412 / #fde68a as the mockup) / SPRING / UPTHRUST, "T1 7,150.00"; empty side "-". Narrow panes: tier 1 = no time, tighter
  gaps; tier 2 = first tag + "+n"; tier 3 = no tags, no At column, titles "3H". The SESSION section, signal line, info line, Level line and
  Options line unchanged. The box is shorter (1-2 plan rows instead of 6 sweep rows); bottom edge docked where it was; the Level / Options
  lines anchor to the box top, so they move down with it and stay clear.
- The sweep rows are still built every draw and written to the status file (SWROW|HOD / LOD lines, format unchanged) for the bridge 1.36.0
  sweep odds; they are not drawn. New status line TODAY_PLAN (version, why, time, buy / sell rows, sources, drawn or not).
- Versions: HL_VERSION = setVersion = PLUGIN_VERSION = 2.40.0 + header comment; no settings change. gex-build HodLod line + TodayPlan.h.

## Tests
- tests_hodlod/t_hl2400.cpp 30,507 / 0 (ASan / UBSan, -Werror):
  [1] 14 cases = the Reader: golden_2400.txt is written by gen_2400.py, which runs the Reader's OWN todayPlanHtf() (tp_reader_2400.js, copied
  verbatim) in node on the bridge's htf dict (lra.hodlod_box.htf_status / px_txt): the real ES 09:43 state + CobSignal status ("S1 7,105 |
  PS0+PS | NEAR | 7,150.00", sell "-"), NEAR on / off / at the 0.25 EM edge / just past it, no price_live, SPRING, UPTHRUST with key-level
  tags, S2 shown, S1 not standing, no EM, NQ / GC / 6E formats - every row and the time identical.
  [2] files: real status / state, decoy nested "markets", escapes, torn / garbled / empty / other market, stale / missing HTFNOW, formats.
  [3] box, 6 markets x 7 widths (600-1900) x 3 font scales x 4 positions x 3 plans (+ the stretched pass): inside the pane, no text overlaps,
  every text in its section, green / red titles, grey tags, NEAR chips, "-" rows, shorter than the sweep box, bottom edge unchanged.
  [4] host: SWROW lines still written (same loop, same format), sweep rows still built, TODAY_PLAN line, sources, versions, gex-build.
- t_hl2391 39 / 0 and t_hl2390 75 / 0 (version checks made version-agnostic); t_hl2380 35 / 0, t_hl2330 7566 / 0, t_hl2250 13 / 0;
  t_hl2372 / 2371 / 2370 / 2360 / 2350 / 240 / 2210: identical pass / fail counts to the untouched 2.39.1 sources (old pins) - no new failure.
- MinGW -fsyntax-only vs the real irtsdk.h: 0 errors; only the old first_extreme_inference.cpp warning (same as 2.39.1).
- Render: render_2400.py draws the logic's own layout (hl2400_render-0.png = the mockup's ES row).

## Differs from the mockup
- "At" tags are GREY (his instruction / colour rule), the mockup drew them blue-violet.
- On the narrowest panes the plan uses compact forms; in 44 of 1,188 test layouts where the old sweep form still fitted, the plan does
  not and the box falls back to the header-only form (mostly with 2 rows per side and 4 tags).
- The plan time in the strips is the Reader state's now_ct (when the Reader last rebuilt), as the Reader's own header shows it.

## Risks / not verified
- Not seen on his IRT yet: installs when he exits IRT; then screenshot ES + NQ for layout / overlaps.
- The plugin reads the Reader state file in the LRA repo (not lsFlexLevels): if the bridge stops rebuilding it, the plan shows the last
  build's rows with its time (as the Reader would); if the folder moves, both sides show "-" and the status line says why.
- If the Reader's todayPlanHtf() rules change, TodayPlan.h must follow (re-run gen_2400.py against the new userscript to see differences).
