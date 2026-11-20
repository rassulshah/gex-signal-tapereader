# lsHodLod 2.39.0 - the GAMMA line (2026-11-20)

## Request
- Rassul 07:45 CT 2026-11-20 "build" on gamma-line-mockup.html + hodlod-new-box-mockup.html: a GAMMA line built like the Now line,
  between the Now line and the HOD / LOD box; the regime pill moves to its start, "0DTE Exp 3:00 PM" to the far left of its row.

## What changed
- GammaLine.h 1.0.0 (new, SDK-free): book parse, names, states, pressure, record, V / C hook, segments, layout.
- Line: regime pill | state badge | G V C | Next, centred over the box; the Now line one row up (3 px gap); 0DTE at the box's left edge.
- Significant strikes: GammaProfile-MQ-<MKT>.csv STRIKE rows with |% of the biggest| >= 50, nearest at / below and above the last
  closed close. Name = MenthorQ code (PS, CR, HVL, GW, CW, PW; from the book's type column, its CW / PW rows and LRA-GexNodes STR names),
  else the plain price ("PS 7,100", "7,150"). Drawn at MenthorQ's strike (no shift). Intraday set only, judged by Fresh.h like the regime;
  STALE / missing / other market = no gamma line (the pill + 0DTE stay on the Now row exactly as 2.37.x).
- States: WATCH (gold, word of the nearer strike), TESTING (gold, forming candle at / through a watched strike), BREAK / SWEEP (red /
  green, decided at the candle close, shown while that candle is the last closed one). Close and pressure disagree = WATCH.
- PRESSURE GAUGE (which one was used): the FALLBACK. The book carries only % of the biggest strike (no dealer contracts), so
  g(price) = sum(signed % / 100 x Gaussian(distance, sigma = half the median strike spacing)); flow = -g x move (live: live - last close;
  BREAK: close - prior close; SWEEP: extreme - prior close); normal = mean |flow| of the 20 closed candles before (>= 5). Arrow when
  |g| >= 0.10 and flow >= 10% of normal, "1.4x" from 1.2x. Not a ratio to futures volume.
- V / C: dashes until lsFlexLevels\OptFlow-<MKT>.csv exists (hook for Monday's MenthorQ integration):
  `OPTFLOW,1` / `MARKET|ES` / `STAMP|<CT wall s>` / `V|<-1,0,1>|<ratio or ->` / `C|...`; used when <= 10 min old.
- RECORD (for the nightly to enrol): lsFlexLevels\HodLod\GammaLine-<MKT>-<spb>s-<session date>.csv, append-only, one row per CLOSED
  candle decided once: ct_end_s,clock,market,spb,state(WATCH/TESTING/BREAK/SWEEP),dir,strike,name,expect,pressure_dir,pressure_ratio,
  gamma_at_strike,close,book_asof_ct_s,version. After a restart the line is redrawn from it (no re-decision).
- Narrow pane: 0DTE drops first, then Next, then G V C, then the pill's setups, then its regime word, then the pill; the badge stays.
- Status file: GAMMA_LINE line (drawn level, state, book, hook, record); NOW_ROW says when the pill moved.
- Versions: HL_VERSION = setVersion = PLUGIN_VERSION = 2.39.0 + header comment; no settings change. gex-build HodLod line + GammaLine.h.

## Tests
- tests_hodlod/t_hl2390.cpp 75 / 75 (ASan / UBSan, -Werror): real ES book 11-20 (7100 PS king, 7150, 7200, 7000; WATCH row = mockup),
  TESTING, BREAK (short gamma) / long gamma closing through = WATCH, SWEEP (long gamma both sides) / short gamma closing back = WATCH,
  pressure rules, V / C hook fresh / stale / other market, replay of the real ES 11-20 overnight bars with a restart after every bar =
  straight pass (record + every line), a later book never changes a decision, torn / duplicate record rows, stale / missing / garbled /
  oversized / other-market books, narrow-pane drop order 0-6, centred, 0DTE clear, Now line above, two markets / bar sizes, versions.
- t_hl2380 35 / 35 (version checks made version-agnostic). t_hl2372 / 2371 / 2370 / 2360 / 2350 / 240: same pass / fail counts as on the
  untouched 2.38.0 sources (old version pins, missing tools folder in the container) - no new failure.
- MinGW -fsyntax-only vs the real irtsdk.h: 0 errors; warnings only in the SDK + the old first_extreme_inference.cpp one (same as 2.38.0).

## Risks / not verified
- Not seen on his IRT yet: needs exit IRT (2.38.0 is also still pending), then a screenshot (ES + NQ) for layout / overlaps.
- On today's ES book every significant strike is short gamma, so a close through 7150 counted as BREAK both ways while price chopped
  around it overnight (9 BREAKs, 14 TESTING in 198 candles) - the nightly scoring decides whether a hysteresis rule is needed.
- A restart in the middle of a candle decides the just-closed candle with the book current then (up to one candle late).
