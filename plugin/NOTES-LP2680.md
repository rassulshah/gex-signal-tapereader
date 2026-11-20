# lsLimitProfile 2.68.0 - what the liquidity on a level line did (2026-11-20)

## Request
- Rassul 06:02 CT 2026-11-20: "ok. i'll go with your recommendation" (mockup level-liquidity-states-mockup.html): score the reads BUILDING /
  LEAVING / X / H / P / CONFLUENCE on the recorded level-line stretches, draw only what works, record the rest, publish each level's state for
  lsHodLod's Now line, add the states to the nightly.

## Scoring (container, staged files; scripts levelliq_score.py (MBO) + levelliq_bars.py (bars only) in the build folder)
- Data: the 2.61.0 stretch records (<MKT>-liqlines-<session>.csv, 11-11 .. 11-20) + IRT's own MBO record (C:\LRA-Data\mbo\live, 11-12 .. 11-19:
  ES 17/18, NQ 12/16, CL 17/18/19, GC 17/18/19, HG 17/19; NQ 11-13 did not stage - container disk full). Contact = first 3-min candle that
  trades AT the level after price came within 1 ATR, the candle before wholly on the defending side. HELD (his rule) = within the contact
  candle + 2 a candle closes back out before any trade 3+ ticks through.
- MBO contacts: 56 (ES 24, CL 13, HG 11, NQ 5, GC 3); base held 28%.
  | Read | n | held with | held without | 1st half days | 2nd half days | verdict |
  |---|---|---|---|---|---|---|
  | BUILDING (+10%) | 4 | 50% | 26% | 0% (1) vs 20% | 67% (3) vs 28% | LEARNING |
  | LEAVING (-10%) | 16 | 19% | 32% | 25% (4) vs 14% | 17% (12) vs 35% | LEARNING |
  | X (attack slowing, size stays) | 18 | 24% | 30% | 100% (1) vs 10% | 19% (16) vs 37% | LEARNING (no better than base) |
  | H (hit, held, closed back) | 15 | 100% by definition | - | - | - | DESCRIPTION: node still >= half there 13 of 15; next-3-candle hold 43% (14) |
  | P (pulled while hit) | 5 | 0% | 31% | 0% (4) vs 29% | 0% (1) vs 31% | LEARNING (n 5) |
  | CONFLUENCE | 0 | - | - | - | - | NO_DATA: no recorded stretch row named a MenthorQ level next to a key level |
- Bars only (all recorded days, 312 contacts): BUILDING-by-record held LESS in ES 18% (38) vs 31% (65), GC 16% (25) vs 26%, HG 27% (11) vs 50%,
  but the ES day halves disagree (25% vs 26%, then 15% vs 33%) -> LEARNING. NQ base 3%: 3 ticks is ~0.1 ATR on NQ - the rule needs scaling there.
- RevRead 0.21.0 study (43-49 sessions at pivot extremes) found P broke LESS (72-77% vs 80-84%) and X no better than base; here P broke 5 of 5
  (n too small), X no better than base - consistent on X, P unresolved. P is NOT drawn.
- 20+ a side in both halves was reached by no read, so per his rule only H is drawn (a description, his exception).

## What changed (2.68.0)
- LevelLiq.h 1.0.0 (SDK-free, tested): per bright stretch - node fixed at the stretch start, S0 at reach, BUILDING / LEAVING / STEADY at
  candle closes, X / H / P at the contact candle's close (H excludes P), outcome over the next 3 candles, line END at a candle that traded 3+
  ticks through and closed through (a P ends the line only when P is drawn). Draw flags: H only.
- Chart: a small box with the letter H at the contact candle on the level line - green on a bid (blue) line, red on an offer (orange) line,
  dark letter, no border, no number, no hover; never two boxes on one another. The blue / orange line stops at the break candle.
- Record for the nightly: lsFlexLevels\DealerProfile\<MKT>-<spb>-levelliq-<session>.csv (S start, T state change, C contact with letters and
  lots, O outcome, E end). Decided letters / ends are redrawn from it after a restart (never repaint).
- Now file for lsHodLod: lsFlexLevels\DealerProfile\levelliq-<MKT>-<spb>.txt, atomic (.tmp -> replace):
  `LEVELLIQ,1,2.68.0` / `MARKET|ES` / `SPB|180` / `TICK|0.25` / `STAMP|<CT s>` / rows `L|name|level_tick|B or O|state|shown|since_ct_s|conf`,
  state = building / leaving / steady / X / H / P, shown = 1 only for a drawn (passed) state. Written on change, else a 60-s heartbeat
  (so a reader can tell live from stale; never on every repaint).
- Status: TOP_BOOK_G gains "LEVEL LIQ (2.68.0 LevelLiq 1.0.0) watches / letters / drawn / line ends / record lines / now-file writes".
- Nightly: analytics/lra/levelliq_score.py (+ test) scores the plugin's C / O records per market and read, both halves, 20+ a side ->
  LRA-LevelLiq-<MKT>.csv + research-artifacts/setups/LEVEL-LIQ.md. NOT YET ENROLLED in the nightly runner (the runner that calls
  liq_lines_score was not found from here) - enrol it next to liq_lines_score.
- No settings change (parameter version unchanged). Own MBO feed only (no lsBookFlow source -> nothing computed).

## Tests
- test_levelliq_100.cpp: 27 / 27 (ASan / UBSan, -Werror -Wall -Wextra): each state's rule, X+H, P, P not drawn -> line goes on, P drawn ->
  ends, break ends the line, 1-2 ticks through still H, contact needs the candle before on the defending side, offers mirror, restart restores
  letters / ends identically, restore twice adds nothing, a later break never changes a decided H, old candle ignored, two trackers
  independent, Now body unchanged while nothing changes / changes on contact / empty after 3 min, garbled records ignored.
- test_dealerprofile_2680.cpp (host, real DealerProfile.cpp on the mock SDK): 86 passed, 1 failed - the same pre-existing failure as 2.67.0
  ("the level line is bright ..." fixture); the host writes the record and the Now file (checked in the run's flex folder).
- MinGW -fsyntax-only vs the real irtsdk.h: 0 errors; the same 8 pre-existing DealerProfile.cpp warnings as 2.67.0.
- test_levelliq_score.py: 3 / 3.

## Risks / not verified
- Not verified on his IRT: needs the restart, then screenshots in 2+ markets (H boxes, line ends).
- The Limit Profile 2.67.0 king "P" mark: kept as is (it describes what happened); its "break warning" meaning is unproven and the RevRead
  study points the other way - treat it as a description until the king records are scored (no scorer for the king records yet).
