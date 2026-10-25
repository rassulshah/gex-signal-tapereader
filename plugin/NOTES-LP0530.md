# lsLiquidityProfile 0.53.0 - LMT zones as MAP LINES + state tags (LmtZonesLogic.h 1.2.0), 2026-10-25

The "v0.49" mockup ships as version **0.53.0**. 0.49.0 to 0.52.1 were already used (the queue meter, the evening fix build, the MBO merge
and the crash fix). Going back to 0.49 would break the version checks.

## Request
- Rassul approved **option A** and the v0.49 mockup: https://claude.ai/artifact/Jqfac5PiaipzJY9RszvMeB
- Research verdict: `iter1024/reversal-read/RR/LIMIT-PROFILE.md`, section 7.
  - A big resting order is a **magnet / target, not a wall**: zones reverse 30.1% vs 34.7% at random prices.
  - 57% of large orders at pivot candidates end FILLED and price goes through.
  - The only useful state is an order still resting after a 3-min close back (HELD).
- He trades with it Monday 2026-10-26.
- Everything approved on 10-22 is unchanged: grey gamma lines, the yellow diamond, the LMT / LMT F columns, the records, no repaint and the settings.

## What changed
1. **Drawing.**
   - Every drawn zone kind (PRINT / ABC / ORIGIN / NEAR offers) is a thin grey 1 px line at the order's price, in the "Zone line colour" (default 0x8A8F98).
   - Label "LMT <lots> (<KIND>)" sits LEFT of the line start in the same grey. If there is no room on the left, it goes above the line start.
   - The yellow diamond sits just left of the label.
   - Far orders are thin lines with their size, as before. They are now drawn in the zone grey, not the side colour; the mockup shows every map line grey.
   - The ring on the first fill is gone; the FILLING tag says it (the mockup has no ring).
   - The pulled / moved X is grey.
2. **One pooled draw rule** (heterogeneity p 0.85).
   - PRINT / ABC / ORIGIN / NEAR offers are drawn in all 7 markets. No "?", no CL gamma-only rule, no ES print-only rule, no GC / 6E log-only.
   - Bids are never drawn.
   - FAR is as before: on, except 6E.
   - The floors and the 3x / 2.5x median entry / keep lines are unchanged.
   - The nightly PRINT / ORIGIN / NEAR / GAMMA_ONLY / BIDS / ABC rows are ignored; the status line says how many.
3. **State tags.** Each is decided on a CLOSED 3-min bar, recorded, read back after a restart, and never repainted.
   - **Band** = the order price +/- 0.25 ATR, the study's w (minimum 1 tick).
   - **Tolerance** = one more band beyond.
   - **FILLING** (grey text): the zone order traded during the bar at a price in the band.
   - **HELD?** (red on offers / green on bids) needs all four:
     - the bar traded into the band;
     - it closed back on the defended side (below the band for an offer);
     - the extreme since the first contact stayed inside the tolerance;
     - the order was still resting at the close and did not trade in that bar.
   - At most one FILLING and one HELD per zone. A later FILLING may follow a HELD; the HELD stays.
   - **PULLED**: the existing X.
   - **Placement**: tags sit on their bar, above an offer line and below a bid line, and never overlap each other.
4. **Log only** (A / W / S record lines; nothing drawn):
   - FAST / MID / DRIFT arrival: ATR travelled in the 60 s before the first print in the band. Market terciles come from `lmz_contacts.parquet` (ES 0.43 / 0.77, NQ 0.40 / 0.73, CL 0.43 / 0.78, GC 0.41 / 0.71, HG 0.37 / 0.67, NG 0.40 / 0.75, 6E 0.37 / 0.70).
   - EATEN: more than 30 s inside the band.
   - STEPPED: the same order id re-priced toward price after the close back.
5. **Hover** (12 rows at most, 118 characters at most).
   - Content: the question, what it is (lots, x the median, age at contact), why it matters, the honest odds, how to use it and an example.
   - "Now" row: status RESTING / FILLING / HELD? / PULLED, the lots traded so far and the % of the shown size.
   - The title says "a TARGET / magnet, not a wall".
   - Odds row, word for word: "reverses here 30% vs 35% at a random price; held + close back: retest P(2R) 32% (random 30%)".
6. **Record** `<stem>-lmtzones-<session>.csv`, new line types:
   - `T|id|bar_end|FILLING or HELD|traded|resting`
   - `B|bar_end`: the last bar judged, so a restart never decides a tag again.
   - A / W / S: the log-only states.
   - Z lines get a 22nd field: the median lots at the zone's start.
   - The reader takes 21- and 22-field Z lines. It skips unknown line types without counting them as bad. The 1.1.0 reader counts the new lines as "bad" and skips them.
7. **Settings.** "State tags (FILLING / HELD)" checkbox appended (parameter version 8 -> 9).
   - The colour rows are relabelled to what they now colour: "HELD offer colour", "HELD bid colour", "Zone line colour". Their keys and positions are unchanged.
   - The file is now `LMZ2,...,tags`. Old `LMZ1` files load with tags on.
8. **Fix found by the new restart test:** after a restart the old bars come again before the book refills. In 1.1.0 they ended live zones SHRANK at old bar times. SHRANK is now judged only on bars the live book saw.
9. **Versions.** LQ_VERSION = setVersion = lpset::PLUGIN_VERSION = lqp::VERSION = 0.53.0; lmz::VERSION 1.2.0.

## Tests (all on Linux, g++ 13, ASan / UBSan)
- `tests_lmz/test_lmz.cpp`: **85 / 85** (was 43 in 0.48.0, 51 in 1.1.0), -Werror. Covers:
  - HELD, FILLING (incl. filled away inside the bar), and no HELD on no close back / a sweep past the tolerance / a pull before contact;
  - one tag of each per zone, and a later FILLING after a HELD;
  - restore + re-fed bars giving 0 new T / B lines and identical tags;
  - unknown line types skipped, 1.1.0 records loading, bids never drawn (BIDS,1 ignored), and all 7 markets drawing the near offer;
  - FAST / DRIFT / EATEN / STEPPED lines and their read-back;
  - the hover rows, and LMZ1 / LMZ2 settings.
- `tests_lmz/test_lmz_host.cpp` (mock host, the whole plugin): **45 / 45**. Covers:
  - grey line, label left of the line, no red box, diamond, gamma lines, bid not drawn;
  - hover text;
  - HELD? red above the line, then FILLING grey, then the grey X; T / B lines recorded;
  - the restart drawing identically, with tags read back and still 2 T lines;
  - settings: master off, State tags off, the colours applied, old LMZ1 file, garbled file;
  - the table: tags row last, parameter version 9.
- `tests_lmz/replay_lmz.cpp` on recorded MBO (the store, via conv_mbo.py):

| session | events | zones (non-far / drawn) | contacted | FILLING | HELD | PULLED (before contact) | log FAST / MID / DRIFT / EATEN / STEPPED |
|---|---|---|---|---|---|---|---|
| HG 2026-10-22 | 1.34 M | 82 / 42 | 63 | 44 | 17 | 26 (16) | 18 / 17 / 24 / 9 / 1 |
| CL 2026-10-12 | 2.76 M | 127 / 60 | 96 | 76 | 24 | 28 (19) | 18 / 31 / 41 / 11 / 2 |
| ES 2026-10-22 | 19.4 M | 156 / 82 | 111 | 73 | 38 | 53 (37) | 18 / 36 / 53 / 13 / 3 |

  - Drawn zones only: HG FILLING 19 / HELD 8; CL 37 / 14; ES 28 / 20.
  - FILLING is the common outcome (66-79% of contacted zones) and HELD the minority (25-34%), as the study says (57% of orders fill).
  - Every session passes two restart checks:
    - restart after every bar: zones and tags identical;
    - IRT restart path (fresh engine, record read, all bars fed again): 0 new tags, ends or B lines.
- Regression suites:
  - test_lqp044_host 21 / 21 (was 19 / 21: the version mismatch is fixed);
  - test_ap047 55 / 55 (was 54 / 55);
  - test_lqp044 114 / 114; test_lqp452 27 / 27;
  - test_lqp453_host 19 / 20 (the MKT ink failure is the same as before);
  - test_lqp461_host 8 / 10 (the two park checks have failed since the 0.52.1 never-delete change, same as before).
- Settings: contract 43,642 checks, 0 failures (22 tables); dialog guard 24 / 24. The settings host test's single HodLod failure was there before.
- MinGW against the real irtsdk.h (Profile and Flow builds, -Dsmall=char -Dfar= -Dnear=): 0 errors, the same 37 / 30 warnings as 0.52.1, none in the changed code. LmtZonesLogic.h alone: -Wall -Wextra -Wpedantic -Werror clean.

## Risks / not verified (needs his IRT)
- **Live look.** The live chart look (label room left of a line that starts at the left edge, tags near the last bar) still needs his ChartView export after the install.
- **More lines.** HG, ES and CL draw more zone lines than before, because the per-market exceptions are gone. The replay draws 42-82 zones + 64-100 far lines a day; they are now thin grey lines. If it is busy, untick "Far orders (thin)" or a kind.
- **IRT restart.** Tags are decided only by the indicator that saw the bar. The live book refills from the feed, so a zone whose order is not seen again keeps its recorded state.
