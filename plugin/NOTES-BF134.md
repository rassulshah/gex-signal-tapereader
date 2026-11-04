# lsBookFlow 1.34.0 - level box + squares, turn-mark ratio label (2026-11-04)

Rassul, 2026-11-04: liked the BookFlow level-box mockup ("clean and it looks good", bf_mockup.html / bf_*.png / bf3_*.png), asked for it
also at prior-day POC / VAH / VAL, VWAP and its bands and the Pb tests of BO / BD ("the pullback is the signal we were working on"), then
"just build it". Same day on the turn marks: "999 makes no sense - you can compare the green bar to the red bar".

## 1. Turn marks (triangle + label over the defending bar)
- **Before:** "NN%" = how much bigger the defending Refill-Effort net was; "999%" when the attacking net was <= 0.
- **After:** the label is the ratio of the two bars AS DRAWN under the mark: defending bar / attacking bar, one decimal ("1.5x", "3.0x").
  The turn rule already needs the defending bar bigger, so it is always >= 1.0x.
  - Attacking bar <= 0 (it pulled more than it added - drawn as a stub) or missing: the triangle alone, no number.
  - Forming candle: "1.5x ?" (or "?" alone).
- Records unchanged: the turns file keeps its `margin` field (marginPct) so lsDefenseLines and the nightly read it as before.
- Status: new line `TURN_LABEL,...`.
- Code: `bf::turnLabel` / `bf::turnMarkText` (BookFlowLogic.h); BookFlow.cpp `mark` uses `netOf` of both sides of that candle.

## 2. Level box (BookLevelBox.h 1.0.0, SDK-free)
One row right of the "Defending?" line, same top, 16 px high, never past the price axis:

`G 7630 | Defenders stacking 4.3x? | Behind stacking 3.3x? | Attackers normal?`

- **When:** only while price is within 4 ticks of a level's zone (zone = level +- 4 ticks, so 2 ES points of the level). Every distance
  is in ticks, exactly as the nightly's zone / away rules, so it scales per market by its tick size. Hidden otherwise.
- **Definitions = lra.book_level_combo 1.1.1, line for line** (DEFENDERS durable adds vs rested pulls 30 s, 1.5x, quiet floor; BEHIND
  1-12 ticks beyond the zone vs -150..-91 s, 2.0x / 0.67x; ATTACKERS within 8 ticks of price vs the median of the 30-s windows ending
  -271..-31 s, 1.4x / 1.5x; 15-s persistence walked from -270 s; side from the approach; arrival = first trade in the zone after >= 8
  ticks away). See the header of BookLevelBox.h.
- **Live timing:** a second's adds are final only after 5 s (the durable rule), so the box shows the second 5 s before the newest
  event - the replay's exact value for that second, 5 s late. Anchor = the arrival; while price approaches within the band without an
  arrival yet, the anchor is the shown second itself ("as if arriving now").
- **Colours by implication:** at a support defenders stacking / behind stacking / attackers pulling = green, the opposite red; at a
  resistance flipped (sellers stacking = red). steady / normal / quiet = gray. Level name white, separators dim.
- **"?":** Attackers always. Defenders and Behind until the nightly's `LRA-BookLevel-<MKT>.csv` says `SHOW|D|1` / `SHOW|B|1`
  (set by book_level_combo only when a cell with that read is "proven"). Written as "2.1x?" (no space, like Ex? / R?).
- **Narrow pane:** Attackers drops first, then Behind, then Defenders; the level name alone is never drawn.
- **Levels:**
  - KeyLevels (`LRA-KeyLevels-<MKT>.csv`) incl. its DPoc / VAH / VAL = the prior FULL-session profile (typed "profile").
  - MenthorQ / Dealer levels (`LRA-Dealer-` / `LRA-Levels-<MKT>.csv`), the same files, freshness and contract rules as the pane's level
    names (NearLevels.h).
  - **pdPOC / pdVAH / pdVAL = prior-day RTH 70% value area:** NOT computed natively (BookFlow has no volume-at-price and the MBO feed
    has no history before the chart opened). Source: the nightly's `LRA-BookLevel-<MKT>.csv` PROFILE lines (book_level_combo.prior_profile
    on the MBO store's trades), used only when their day is the previous weekday of the chart's session; otherwise only the KeyLevels
    DPoc / VAH / VAL (full session) are watched. The status line says which.
  - RTH VWAP and +-1 / +-2 SD: SessionVWAPLogic.h `svl::run` on this chart's bars (the same lines lsSessionVWAP draws), the last
    CLOSED bar's values, from the open + 15 min (as the nightly), RTH only. Note: the nightly uses a trade-by-trade VWAP; the bar-based one
    can differ by a tick or two.
  - Pb test zones: lsBreakout's fired BO / BD records of this session
    (`lsFlexLevels\Breakout\<MKT>-<SYM>-180-breakouts-<daykey>.csv`, 3-bar "zz" lines only); zone = bottom half of the BO candle / top
    half of the BD candle (mid rounded half-to-even like bo_pb.pb_windows); valid from the fire bar until the next fired BO / BD fires or a
    bar trades beyond the wave origin (3-bar ZigZag `zzd::Engine::step` on this chart's bars; 3-min charts only, else the origin rule is
    off and the status says so). Named "Pb BO 9:42" (the BO / BD candle's clock).
- **Away states** reset once a day at RTH open - 60 min (find_touches starts there).

## 3. Squares
- Two thin rows (3 px, 1 px gap) under the bars of each candle during which the box showed: Defenders row, Behind row.
- Colour = what the box mostly showed in that candle: red if red >= 20 s, else green if green >= 20 s, else gray.
- Decided once every second of the candle was evaluated (5 s after its close), never changed; C lines in the record; reloaded after a
  restart (identical).
- The pane keeps 10 px under the bars ONLY while a candle with squares is in view (scale() moves the axis by exactly 10 px, same px per
  lot); otherwise the axis and every bar are exactly as before. Squares are skipped (counted) if there is no room.

## 4. Records and status
- `lsFlexLevels\BookFlow\<MKT>-<SYM>-<spb>-<session>-levelbox.csv`: header; `S|sec|clock|level|ltype|side|arrival|k|lo|hi|D word|x|col|
  B ...|A ...|text` on every change of level / side / words / colours (not on every ratio change); `C|bar_end|D|B|secs|...|level` per
  decided square. For the nightly's live-vs-replay parity check.
- Status line `LEVEL_BOX,1.0.0,...`: params source, quiet floor, SHOW, level counts per type, profile / VWAP / Pb notes, what is shown
  now, seconds evaluated / shown, us per evaluated second, engine events / orders / arrivals, squares, record lines / fails.

## 5. Nightly (LRA repo) - book_level_combo 1.2.0
- New `export_flex` writes `lsFlexLevels\LRA-BookLevel-<MKT>.csv` (atomic, CRLF): VERSION|1, MARKET, AT, PARAM def_x / quiet_lots /
  behind_stack_x / behind_thin_x / att_stack_x / att_pull_x, SHOW|D / SHOW|B (1 only when a cell with that read is proven), PROFILE lines
  of the latest RTH session in the trades store. Called after deploy_params; an error never stops the night (reported in the json).
- Until the first export: the plugin uses the study defaults (ES quiet 40 lots; other markets 40 until their first arrival, then scaled
  from this session's own arrivals as quiet_floor does).

## Tests (Linux)
- `tests_bf134/test_blb_replay.cpp` - PARITY on the real ES 2026-10-30 feed (Rassul's IRT record, 06:25-13:20 CT, 10.6 M events) vs
  `lra.book_level_combo.replay / smooth` on the six prototype cases (G 7630 held, PWL 7648.50 broke +84 s, PS 7600 broke +217 s,
  VWAP 7651 resistance broke +18 s, VWAP -1 band, Pb test of the 12:33 BD): **every per-second array identical (5 x 601 x 6) and every
  smoothed read identical (571 s x 3 lines x 6 cases)**; the live path (away / arrival rule, 5 s late) equals the nightly at every one of the
  529 shown seconds whose arrival is one of these cases (4,026 s shown that day, 57 arrivals); G 7630 at +60 s "Defenders stacking 4.3x | Behind stacking 3.3x | Attackers normal"; VWAP
  resistance "Defenders stacking 20x+ | Attackers stacking 1.8-2.0x". 46 / 46, also clean under ASan / UBSan.
  - The reference was run with the replay starting at the first event (the nightly normally starts its book 125 min before the day's
    first case - orders resting from before that are unknown to it) and ending 30 s after the last window (the nightly misses leaves
    after its last window, so the very last seconds of its last case can count adds as durable). Both are nightly-side approximations.
  - A nightly grid artifact: book_level_combo.replay skips events priced off its grid (lowest zone - 1000 .. highest + 1000 ticks), incl.
    a MOD that moves an order off the grid, so that order keeps counting at its old price. Not fixed (needs a CACHE_V bump); no effect on
    these cases.
- `tests_bf134/test_blb_logic.cpp` - 41 / 41: persistence (15th second, interrupted count), colours flip at a resistance, texts (20x+,
  steady / normal / quiet, ? rules), layout (drops from the right, never past the axis, never the name alone), squares (20 s rule, C line
  round trip), params file (good / VERSION first / wrong market / garbled / out of range / the nightly's exact CRLF export), Pb zones
  (half-to-even mid, window until the origin breaks / the next BO / BD), turn label == the bars' ratio over a grid, versions.
- `tests_bf134/test_bf134_host.cpp` (mock SDK, ASan / UBSan) - 32 / 32: hidden 8 ticks away, arrival = support, box text / colours,
  one row right of the line, inside the pane, nothing over it, persistence, hidden again away, squares only for candles at the level,
  never over a bar, axis reserve only while squares are in view (exactly unchanged otherwise), the record (header, C and S lines), no
  repaint over later candles, restart = identical squares reloaded and drawn, status lines, params file SHOW / wrong market.
- `tests_aggbook/test_aggbook_host.cpp -DBOOK` - 127 pass, 1 fail (pre-existing line 337, unchanged); the turn-mark checks now expect
  "1.5x" (90 / 60) instead of "50%", plus every drawn label == turnMarkText of the drawn bars.
- `tests_bf131/test_bf133.cpp` - 12 / 13: only its 1.33.0 version pin fails (expected after the bump).
- MinGW `-fsyntax-only` against the real irtsdk.h: BookFlow.cpp 0 errors, 0 warnings outside the SDK.
- CPU (the engine on the real day, -O2): about 160 ns per order event and about 6 us per evaluated second; ES 10-30 averaged ~420
  events / s = well under 0.1 ms of CPU per second (a 20,000-event burst ~3 ms). Memory about 5 MB per chart (700-s rings + the book).

## Not verified (needs his IRT)
- The look on his chart after the restart (box position next to the Defending? line, squares under the bars), 2+ markets.
- That IRT calls scale() every redraw for this pane (the squares' 10 px depend on it; if not, the squares are skipped, never drawn over
  bars - the status counts "no room").
- Pb zones need lsBreakout's records on the same contract and a 3-min chart for the origin rule.
- The prior RTH profile appears only after the nightly's first export.

## Versions
BF_VERSION = bf::VERSION = ofset::BF_VERSION = setVersion literal = 1.34.0; BookLevelBox 1.0.0; book_level_combo 1.2.0. No settings change
(parameter version unchanged, SS_SAFE_DIALOG kept). gex-build.bat: BookFlow's line also hashes BookLevelBox.h, ZzoDeltaLogic.h,
ZzoParams.h, SessionVWAPLogic.h, OnTouchLogic.h, OnTouchParams.h.
