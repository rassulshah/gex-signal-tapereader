# RESUME NOTE — read this before anything else
_written 2026-09-02, amended 2026-09-17 23:45 CT (v16.48 / GP 0.69 / DS 0.14) · **panel v16.48** · **companion v1.21** · RTX plugins: **lsGammaProfile v0.69**, **lsDayModel v0.18**, **lsDayStats v0.14**, **lsKingTracker v0.15** · supersedes every earlier resume note_

# ⚠⚠⚠ 2026-09-17 ~23:45 CT — v16.48 + DS 0.14: THE E ROW NEVER COPIES THE TAPE · ONE MODEL ON BOTH HALVES OF THE READ LINE. START HERE.

"both .. do it" → (1) `hodlodCondE`'s +orclock branch is gone: the E row's 1ST / TOOK are the stage's own median (09-17:
HOD ~8:51am · ~21m, not the tape's 8:33 · 3m); test_daystats_cond's mutation guards that it stays gone. (2) The panel
writes `READ1` (the first extreme through `hlSecondRead`, same row shape as READ2; `AU.day.first`); DS 0.14 prints the
left half from it ("HOD IN 74%" / "· if not, ~clock (50%)" while < 50) and falls back to the HLTAB cell only before the
model's 36-minute gate; tone green ≥ 70 / amber ≤ 30. Runner: READ1 == re-derivation, opposite sides, side == DAYSA 1ST.
**He needs: Tampermonkey 16.48 (5 min, reload Atlas) + close IRT (DS 0.14; 0.13 also pending) → INSTALLED → reopen →
Corner = Top-center.** Tomorrow: watch the line from 8:45 (HLTAB half only) → 9:06 (both halves, the model) through the
day; compare with the replay shape in the CHANGELOG; the E row's 1ST should read ~8:51-ish on a top-tercile open, never
the A row's clock.
**STILL OPEN on the E row (his call, one at a time — offered, not agreed):** BOP · WICK · W.END · WICK% · MUD are weekday
MEANS (E.wick.bop 20.8 vs bopMed 6) and do not compose (TOOK + BOP ≠ WICK); MUD / MUDt are two durations of one leg (§10.2:
MUD = the move in pts · $). Proposal: medians, composed like the A row (WICK = TOOK + BOP → W.END; MUDt = HL GAP − BOP);
MUD = |2ND − open| pts · $ (E from the expected candle).

# ⚠⚠⚠ 2026-09-17 ~23:15 CT — v16.47 + DS 0.13: THE EM WAS 10× · THE PANEL CENTRES · BOTH CLOCKS AFTER THE CLOSE. START HERE.

His first look at DS 0.12 (the evening panel): cut off at the left; "the took values make no sense. How is the expected
value 3m?"; "do a sanity check of all the expected values"; "how would it have looked at 9am?"; "the first extremity
should always be first ... if the lod occurs, it should be before the HOD and vice versa" (it does; now pinned by test).
- **BUG, FIXED (16.47):** the 0DTE straddle EM was converted SPX → ES with `irtRatio()` = ES per SPY (10.1×): emEs 278
  for a 27.55-point straddle, the range model clamped at 2.5× the weekday (172.5 pts ≈ $8625 on a 36.5-point day) —
  since 16.33, every E range / expected candle was the clamp. `gpEsPerSpx()` (≈ 1.009), `AU.day.esSpx`; Gate A 1.3b–f;
  Gate L now FAILS an audit whose emEs/emSpx is outside 0.95–1.06 (tonight's live export fails it, the fixture passes).
  Lesson (PROJECT-CONSTANTS): a stub that models the wrong function proves nothing — test_day_export's `irtRatio` stub
  returned the SPX ratio, and the runner re-derived from the same wrong number.
- **DS 0.13:** Corner + Top-center / Bottom-center (appended entries 4 / 5), left-clamped anchors (`dsl::anchorX`),
  `dsl::readLine` (first extreme leads; after the close "HOD IN 8:33am · LOD IN 9:09am"), text rect = the text's width.
- (superseded by 16.48 / DS 0.14 above)
- **OPEN THREAD — the E row, one element at a time (nothing changed yet; his call):**
  1. TOOK / 1ST: the stage's `+orclock` rule copies the opening hour's extreme clock (tautological once the first extreme
     is inside the first hour — most days). Proposal: the stage median without it (pos60-top: 21 min → "HOD ~8:51am").
     `study-daystats-cond.py` measured orclock at MAE 38.4 → 33.3 — a real gain as a forecast, but not what he wants on an
     E row beside the A row. If he says drop it: `hodlodCondE` `+orclock` branch off; CONDE basis loses the suffix;
     day-derive / Gate A / fixtures / the runner follow; condstats keeps the rule text for the record.
  2. BOP · WICK · W.END · WICK% · MUD are weekday MEANS (E.wick.bop 20.8 vs bopMed 6) and do not compose. Proposal:
     medians, composed like the A row (WICK = TOOK + BOP → W.END = open + WICK; MUDt = HL GAP − BOP).
  3. MUD vs MUDt: two durations for one thing (A row identical by construction). §10.2: MUD = the move (pts · $), MUDt =
     the duration. Proposal: MUD = |2ND − open| in pts and $ (E: from the expected candle), MUDt unchanged.
  4. The READ line's FIRST half is still the 2026-08 HLTAB table and wobbles (9/17: 47 → 32 → 40 → 47 → 58 → 45 → 33 →
     76 → 62 → 49 at 11:00 → 57 at 1:00pm); the secondin logistic on the first side is AUC 0.859, calibrated (scratch
     study, OOF by day). Proposal: one model, both halves (`hlSecondRead` on both sides; READ2 grows a second row or the
     READ row takes p from the logit); the 36-minute gate stays (18–36 min is not calibrated, max error 0.183). Before
     36 min the line shows the first half only.
- **9am replay (09-17, the panel's own functions on the day file's ES 1-min rows):** 9:00 "HOD 40%" alone; 9:06 "HOD 47%
  · LOD IN 6% · if not, ~9:15am (50%)" (the low printed 9:09); 10:00 "LOD IN 25% · if not, ~10:09am"; 11:00 80 %; 1:00pm
  95 %; 2:30pm 99 %. Scratch: `replay-0917.js` (rebuild from the CHANGELOG entry if needed — the panel's ex() harness +
  data/2026-09-17.json futBars.ES rows, CT = ET − 1h).

# ⚠⚠⚠ 2026-09-17 ~22:30 CT — v16.46 + DS 0.12: "HOD IN 96% · LOD IN 80%" — THE SECOND EXTREME'S READ, A NIGHTLY-REFIT MODEL. START HERE.

He rejected the descending rungs ("I don't want the probabilities going down") and asked for one rising number per
extreme + a good, tested, self-calibrating model. Built: `tools/secondin.py` (logistic on the chart's own bars; OOF AUC
0.883, calibrated, floors), refit every nightly into BASERATES.secondIn, courier-gated (`siNormalise`); panel writes
READ2 every export; DS 0.12 prints "LOD IN 80%" (+ "if not, ~HH:MM (50%)" while < 50 %; the A-row clock after the close).
(Done 23:00 CT: 16.46 installed, DS 0.12 + GP 0.69 INSTALLED 21:46 CT, reopened.)
Watch tomorrow: the number should climb through the morning once the first extreme is called; compare against the
mockup's shape. The rolling-20 check is in BASERATES.secondIn.rolling20 — show it in the testing tab next (not built).

# ⚠ 21:55 CT — STUDY (tools/study-second-quantile.py): the stage p20 16.45 ships is NOT better than the base rate; the 1ST-clock tercile is (−10 % pinball) and the SURVIVAL p20 (re-fit on days whose 2ND is still ahead) is what keeps "80 %" honest as the day passes. NEXT BUILD (agreed?): the panel re-computes the rung every export from a baked survival table × 1ST-clock tercile; plugin unchanged.

# ⚠⚠ 2026-09-17 ~21:40 CT — v16.45 + DS 0.11: THE READ LINE'S SECOND HALF. "HOD IN 96% · LOD after 10:42am 80%", one descending rung (p20 → p50 → last hr → last 30 → any minute → IN <clock> after the close), from the study's new t2 percentiles per stage (n=301, rebaked). **He needs: Tampermonkey 16.45 + close IRT for DS 0.11 (and GP 0.69 still pending).** Eyes-on tomorrow morning: the rung and its clock on the Day Stats title line.

# (21:05 CT — DS 0.10: the A row's prices = the chart's own RTH high / low (the evening drift 7722 → 7708 was the live-close bias on ES1-Sep prices after the roll). OPEN: test_hodlod b5/b6/s2 — BASERATES.json regenerated by his machine at 15:16; rebake the ladder.)

# (20:50 CT — GP 0.69: the repair-on-load fired on every click (black colour rows) and froze the dialog; now once per load, colours excluded. Restart IRT, then Level labels at = Off.)

# (19:35 CT — GP 0.68: "Level labels at" Left / Center / Right / Off; level lines confined to the price area between the strips. GEX build → restart IRT; he picks Right or Off.)

# ✅ 2026-09-17 ~19:15 CT — VERIFIED: Atlas's five == IRT's five (16.43 live, poolSrc = atlas, runner 24/24 after the fix). v16.44: a pooled node is stamped first-seen whatever its own % (7575 / 7500 had no band); the runner's ES column; the harness mutant leak fixed (PROJECT-CONSTANTS). **He needs Tampermonkey 16.44 (panel only).** Tomorrow RTH is the real day-long test.

# ⚠⚠⚠ 2026-09-17 ~18:05 CT — v16.43: THE POOL IS ATLAS'S OWN RANKING. START HERE.

16.42's audit showed Atlas's after-close five all from the monthly SPX book on the 2026-09-18 slice (third Friday
tomorrow); ours shared one. 16.43: the pooled rank = the merged slice's order when the payload is fresh, mapped to rail
strikes via the ratios (SPY whole strike / SPX 5-wide; SPXW + monthly share the SPX rail); stale/absent -> own-%;
`AU.poolSrc`. Gate A §13 pins tonight's payload; Gate L re-derives from the slice. **He needs: Tampermonkey 16.43
(5 min, reload Atlas). Acceptance: the runner on the next export (Atlas's five == ours, poolSrc = atlas) + his IRT
screenshot beside Atlas.** Tomorrow RTH: the slice is today's 0DTE again — SPY + SPXW dominate, as at 11:18.
Open question for him: on monthly-expiry evenings the SPX rail badges 7675 (−15 % on the SPXW tape) because Atlas does.

# ⚠⚠⚠ 2026-09-17 ~17:45 CT — v16.42: AU.atlas — what Atlas drew, same second; the runner diffs the five. START HERE.

He says IRT's five ≠ Atlas's five. His Atlas tab after the close drew 7771/7746/7695/7645 (100/80/66/80 %) — SPX-ratio
strikes 7700/7675/7625/7575 with percentages that are not the SPXW 0DTE ladder's → after hours the derived slice is a
different window/book than our SPY + SPXW pool (the open "Derived-window caveat"). 16.42 records the page's own merged
derived slice in the audit (`AU.atlas`, rows [ES, %, $K, rank] + per-book rows/ratio) and `gp-regress.py` checks
Atlas's five == ours. **NEXT: after his Tampermonkey update (wait 5 min, reload Atlas), stage GammaProfile.audit.json,
read AU.atlas, and correct `gpAtlasPool` to Atlas's actual rule (which books, which window, after-hours behaviour).**
Do not change the pool before the numbers are in hand.

# (17:20 CT — GP 0.67: bands ONLY on the primary nodes (the Show filter's set, like the top-node lines they replaced) — 0.64's "every node >= 5%" was a wall after hours. 0.66: the tape strip is font x 7 + 8, the strike-to-% gap halved at his request.)

# ⚠⚠⚠ 2026-09-17 ~16:50 CT — GP 0.65: THE VERSION BUMP DOES NOT RESET — repair-on-load added. START HERE.

His first 0.64 dialog was scrambled (old values by position: Font 0, Hide % 1547868, black colours). setParameterVersion
does NOT reload defaults on a saved instance (PROJECT-CONSTANTS). 0.65 = KT 0.13's cure: `scrambled()` +
`migrateScrambled()` in the parms callbacks put every row to the reviewed defaults once. **He needs: GEX build → close
IRT → INSTALLED → reopen; open the dialog, it should read the defaults; Apply.** Then the 0.64 acceptance (below).

# ⚠⚠⚠ 2026-09-17 ~15:40 CT — v16.41 + GP 0.64: THE REVIEWED DIALOG, BOTH KINGS, MAG, TIME-BOUNDED NODE BANDS. START HERE.

He reviewed every dialog row (see CHANGELOG 0.64 for the full list) and asked for Atlas's look: the node's line becomes a
BAND from the bar the node was first seen to now, polarity colour, opacity by %. Panel 16.41 stamps a 9th STRIKE field
(first-seen CT second, >= 5 % of own King, per book, persisted, reset daily); GP 0.64 draws the band from it, draws BOTH
King lines, puts Mag + both Kings in the chip, tags MAG on the exact-strike Skylit node, and ships the 27-row dialog at
parameter version 7 with defaults = his settings. **He needs: Tampermonkey 16.41 (wait 5 min after the push, reload Atlas)
+ GEX build 0.64 (close IRT → INSTALLED → reopen). After the reset he re-enters NOTHING — check the dialog reads Both SPY
and SPX / 90 / tape on / Bottom-C / 120.** Bands start at the minute 16.41 first saw each node — full history from
tomorrow's open. Acceptance: his screenshot vs Atlas same minute: badges + % (fetched, runner) and band starts (eyes-on).
Verify from the export folder: `GammaProfile.status-Both-*.txt` (GPSETTINGS shows bands=1, ifmag=<strike>, bars=N).

# ⚠⚠⚠ 2026-09-17 ~12:45 CT — GP 0.63: the rail clear of the price scale, tape columns on both rails (+ left background), GPSETTINGS in the status file. START HERE.

0.62 screenshot: SPX bars under the price scale (the pane rect reaches the scale; the tape strip had been hiding this),
"tapes missing" (Tape columns off after the reset + 0.61 forced it off on the SPY rail), chip at Bottom-L while he
expects centre. 0.63: `gpl::usableRight` (scale rect), tape columns on the SPY rail with a dark background, GPSTATUS
15th field = offset, GPRECT + GPSETTINGS lines. **He needs: GEX build → close IRT → INSTALLED → reopen; tick "Tape
columns"; send a screenshot.** Then READ `GammaProfile.status-Both-Left.txt`: if `panelpos=1` and the chip draws left,
that is a draw bug; if `panelpos=0`, it is the setting (or IRT did not reset on the version bump — then say so).
Values check: chart price = CSV price + the offset in the status line.

# ⚠⚠⚠ 2026-09-17 ~12:25 CT — GP 0.62: the SPY rail at the SPX thickness, bubbles outside short bars, SPY rail drawn first. START HERE.

0.61 Both WORKED (status file: both rails rendered, 2 + 3 primaries) but looked broken: SPY bars 40 px tall / 6–11 px
long (auto thickness from the 10-pt SPY spacing × width 40), bubbles off the pane, the SPY rail painted over the regime
chip. 0.62: `gpl::autoBarH` shared from the SPX rail, `gpl::bubbleOutside` (bar shorter than the bubble → outside),
SPY rail drawn first. **He needs: GEX build → close IRT → INSTALLED → reopen; set SPY rail width ≥ 120.** Acceptance:
his screenshot — SPY bars as lines with ①④⑤ visible, chip clean — then the same-minute compare vs Atlas.
Note: the status file is named by cfg.side (his instance is Side = Left → `GammaProfile.status-Both-Left.txt`).

# ⚠⚠⚠ 2026-09-17 ~11:55 CT — GP 0.61: BOOK = BOTH — one instance, SPY rail left + SPX rail right. START HERE.

He asked why two instances were needed at all ("so I don't have to add another gamma profile indicator") — no good
reason. Built: Book = Both (fifth entry, appended), SPX rail RIGHT (Side ignored), SPY rail LEFT at the new LAST row
"SPY rail width px" (default 40; 0 = same as Width px); Hide % under and everything else shared (his call); levels,
regime chip and panel once, from the SPX file; the status file carries two GPSTATUS lines. `gpl::railLayout` pinned.
**He needs: GEX build installs 0.61 (close IRT → INSTALLED → reopen), then ONE lsGammaProfile instance: Book = Both,
Rank = Atlas merge, Width px 90, SPY rail width 40; delete the second instance.** Acceptance: his screenshot vs Atlas
at the same minute + `GammaProfile.status-Both-Right.txt` with two `rendered 1` lines (read over the bridge).
The 0.60 two-instance diagnosis is moot unless Both also shows one rail — then the status file names the gate.

# ⚠⚠⚠ 2026-09-17 ~11:35 CT — GP 0.60: THE RANK ROW MOVED TO THE END + A STATUS FILE PER INSTANCE. START HERE.

With two 0.59 instances (SPY Left / SPX Right) he saw ONE rail: "it displays one or the other but not both". The 0.59
diff shows `Rank` was inserted after "Line style", mid-list, no version bump — the KT 0.12 scramble repeated the same
morning it was repaired there (fourteen rows after it read their neighbour's saved value on both instances). 0.60 moves
Rank to the END, bumps the parameter version to 6 (IRT resets both instances to defaults — **he re-enters Book, Side,
Width, Hide % under, Rank on each**), and writes `GammaProfile.status-<Book>-<Side>.txt` after every draw (GPSTATUS line:
file, strikes, anchor, bars drawn, rendered). **Next: GEX build installs 0.60 → he restarts IRT → sets the two instances
→ read the two status files over the bridge.** Two files with `rendered 1` and both rails on screen = done (then the
same-minute screenshot vs Atlas). One file, or `rendered 0` / `strikes 0`, = the real cause, named by gate. Not yet
proven that the scramble hid the rail: Book/Side/Width sit above the insertion point and were untouched.

# ⚠⚠⚠ 2026-09-17 ~10:40 CT — v16.40 + GP 0.59: THE SPY BOOK + THE ATLAS POOL (two rails, five badges shared). START HERE.

He compared Atlas's ES1 view to IRT: ES has NO native book on Skylit — the ES chart is the DERIVED layer, one pool of the
SPY and SPXW books each scaled to its own King, top five by that percentage (both Kings always in; 3 SPY + 2 SPX that
morning; measured on the live feed, raw dollars — NOT a dollar ranking, monthly SPX excluded). His call: two rails, SPY
LEFT and SPX RIGHT, the five badges split as Atlas splits them; the day candle off for now. Built: the panel writes
`GammaProfile-SPY.csv` (Book = SPY had been dead since 0.47) and an 8th STRIKE field = the pooled rank on both files;
GP 0.59 `Rank = Atlas merge` draws from it (unpooled rows grey). Mockup: `two-rails-mockup.html`. **He needs: Tampermonkey
16.40 + GEX build installs 0.59 (restart IRT), then a SECOND lsGammaProfile instance: Side = Left, Book = SPY, Rank =
Atlas merge; the existing one Book = SPX, Rank = Atlas merge.** Eyes-on = his next screenshot vs Atlas (same minute).
Still open: Derived-window caveat (the pool is 0DTE like the nodes=5 request; his page selector may differ).

# ⚠ 2026-09-17 ~09:55 CT — KT 0.15: the dialog's black colour swatches (0.13 used setIntegerValue on colour rows — wrong setter) repaired with setParameterColor; black never shown or drawn. GEX build installs it; restart IRT.

# ✅ 2026-09-17 09:42–09:44 CT — THE TOGGLE TEST PASSED on his live chart (KT 0.14, real exports): IF 25/25 → he flipped to Skylit 27/27 (SPX + SPY drawn, IF not) → back to IF 25/25 (IF drawn, SPX/SPY not); the switch takes effect on Apply, no restart. Runs + fixtures in `testing/king-tracker/` (0942-ready, 0943-skylit, 0944-if). The King tracker's IF work is DONE and accepted by test.

# ⚠⚠ 2026-09-17 ~09:50 CT — KT 0.14: `KingTracker.status.txt` (KTSTATUS lines per book after every draw) so the Source toggle can be verified from the export folder; `gp-regress.py --indicator kingtracker --kt-status`. The toggle test with him flipping the dialog is the next step.

# ⚠⚠ 2026-09-17 ~07:45 CT — GP 0.58: the regime chip box now sized to its content (the CW had run outside it), pill 4 px from its text. Installs itself via GEX build.

# ⚠⚠⚠ 2026-09-17 ~07:20 CT — v16.39 + KT 0.13: IF DEFAULT, POLARITY COLOURS, THE SCRAMBLE REPAIRED. START HERE.

His 0.12 dialog was scrambled (IRT stores values by position; moving Source to the top shifted them — width 16776960,
black NDX, black Magnet → invisible IF line). He answered with requirements instead of a choice: IF default; the
Magnet's colour = its gamma sign; black never an option. Built: panel 16.39 writes each step's polarity (7th field);
KT 0.13 defaults to IF, colours the Magnet gold/magenta per step (gamma profile colours), refuses black for any book,
and `migrateScrambled()` restores his known layout on load. **Never reorder the parameter list again** (in the .cpp).
He needs: Tampermonkey 16.39 + close/reopen IRT once GEX build logs INSTALLED (read `plugin/out/BUILD-STATUS.txt`).
Acceptance = his next screenshot: gold/magenta IF line on the ES chart, sane numbers in the dialog.

# ⚠⚠⚠ 2026-09-17 ~06:40 CT — GEX BUILD: THE PLUGINS COMPILE AND INSTALL THEMSELVES. START HERE.

He asked for a way out of the command window. `setup-gex-build.bat` (run once) installs a hidden 2-minute task that
compiles any plugin whose source changed into `plugin\out\` and copies the DLL into IRT's folder as soon as IRT is closed
(PENDING → INSTALLED in `tools/gex-build.log`; `plugin/out/BUILD-STATUS.txt` at a glance — READ IT OVER THE BRIDGE to
verify a build compiled instead of asking him). From here the deploy of a .cpp is: commit over the bridge → wait 2 min →
read the status file → tell him "restart IRT". The compile-*.bat and `plugin/compile-all.bat` remain for manual use.
**He needs to run `setup-gex-build.bat` once** (first run builds all four; with IRT open they sit PENDING until he
closes it). Then KT 0.12 / GP 0.57 / DM 0.18 / DS 0.9 install themselves — Source = IF in the King tracker after that.

# ⚠⚠⚠ 2026-09-17 ~06:10 CT — KT 0.12 (+ GP 0.57 / DM 0.18 / DS 0.9): SOURCE FIRST, AND THE FONT GUARD. START HERE.

His 0.11 eyes-on: Source = IF applied, yet SPX/SPY still drew and no Magnet. Cause: every plugin's parms callbacks only
read the dialog when Font size read 6..48 — he runs 3 — so NO King tracker setting had ever reached the plugin. KT 0.12:
guard = the Source list; Source is the first row, the rows the source does not draw are greyed; `setParameterVersion(2)`
(defaults re-read). The other three plugins got the probe widened (1..200). **He needs all four compile bats** (IRT
closed), then Source = IF in lsKingTracker; the Magnet line also needs panel 16.38 exporting (his screenshot showed
STALE 14m — reload Atlas). Not yet seen working on his chart — the next screenshot is the acceptance.

# ⚠⚠⚠ 2026-09-17 ~04:00 CT — v16.38 + lsKingTracker 0.11: THE KING TRACKER SWITCHES SKYLIT ⇄ IF. START HERE.

He asked whether the King tracker had the IF option — it did not (the 16th's 17:30 proposal had dropped out of every
list). His decision: **"switch back and forth between IF and Skylit"** — one source or the other, like the gamma
profile's Book. Built: the panel samples IF's 0DTE Magnet as its own journeys (`IF` on the SPX chain for ES, `IFQ` on
QQQ for NQ; same dwell / anti-oscillation; stale chain → no sample), and lsKingTracker 0.11 has a `Source` list
(Skylit | IF, **default Skylit**) + an IF colour; on IF only the Magnet draws (no IF SPY book exists, so SPY hides).
Regression updated with the build (KT rows 32, logic 21; all 21 suites green). **He needs: Tampermonkey 16.38 +
`compile-kingtracker.bat`, then Source in the indicator's settings.** Still to install from the 03:30 build: the other
three compile bats (GP 0.56 / DM 0.17 / DS 0.8) and the first `regress.bat` run on his MSVC.

# ⚠⚠⚠ 2026-09-17 ~03:30 CT — v16.37 + GP 0.56 / DM 0.17 / DS 0.8 / KT 0.10: THE REGRESSION FOR ALL FOUR INDICATORS. START HERE.

His last instruction of the 16th: *"I had told you to make a regression and you did it for gamma profile. This regression
needs to be extended to the other indicators and you need to keep updating the test cases, so you can fire the regression
either collectively or separately for the indicators."* Built and green (21 suites): **`python3 tools/regress.py
[all|gamma|daymodel|daystats|kingtracker]`** here, **`regress.bat [indicator]`** on his machine; master doc
**`testing/REGRESSION.md`**, trail **`testing/RESULTS.md`**. Three gates per indicator — A (node, executes the panel's
builders), B (C++ logic headers, no SDK: `DayModelLogic.h` / `DayStatsLogic.h` / `KingTrackerLogic.h` /
`ContractOffsetLogic.h` are NEW and the four .cpp now delegate to them — a refactor, no behaviour change), L (the live
runner `gp-regress.py --indicator …` re-run on pinned CSV+audit fixtures). New this build: `test_kingtracker_rows.js`,
`test_day_export.js` (the whole day section end to end, then `tools/day-derive.js` must reproduce every row from the
audit), the panel's **`AU.day`** audit block (every input the candle and the E row used), the runner for the other three
with the expected picture + `CHECKLIST.md` per indicator. **It caught one on its first run:** the pre-EM range stages had
used `<` on the opening window (the 09:00 bar dropped) while everything else and both studies use `<=` — fixed in 16.37.
**Standing rule (BUILD-CHECKLIST 2b): touch an indicator → update its cases in the same commit → its regression green.**

He needs (nothing is installed yet from this build): Tampermonkey **16.37** (panel; AU.day) + `compile-gammaprofile.bat`,
`compile-daymodel.bat`, `compile-daystats.bat`, `compile-kingtracker.bat` (refactors; the shared anchor). Then, once, on
his machine: `regress.bat` — the first run of the suites on MSVC (never done). Next: stage his first real 16.37 export
(CSV + audit) as the day fixtures (`testing/day-model/fixtures/`, `day-stats/`, `king-tracker/`, and the `live` lists in
`tools/regress.py`) — the day fixtures are SYNTHETIC until then. Still open from the 16th: the Day Stats A-row HOD/LOD
drift after the close (first check with a live export), item E, the volume/trend-day evening, the NQ cross-check.

# ⚠⚠⚠ 2026-09-16 ~22:25 CT — v16.36 + lsGammaProfile 0.55. START HERE.

Late fixes on his eyes-on: the E row no longer copies the actual 1ST once the READ is IN (it printed the same clock and
TOOK as the A row — "how can that be?"); the depth pill sits on the same line as its neighbours (0.55, after 0.51/0.54
missed it); the King (Magnet) is on the chip after the PW (0.52); IF is the default Book, one rail, switch by dropdown
(0.53). Latest: panel 16.36 · companion 1.21 · GP 0.55 · DS 0.7 · KT 0.9 · DM 0.16.

# ⚠⚠⚠ 2026-09-16 ~22:00 CT — v16.35 / companion 1.21 / GP 0.50 / DS 0.7: THE IF EXTRAS + THE 2ND LADDER. START HERE.

Operator: "all the things we talked about" → built: the wall-depth pill (0D / WK / MO on the CW / PW tags and the chip's
second line), the slope word on the chip's first line (STEEP dn / STEEP up / flat, from companion 1.21's `gf.sDn/sUp`),
and lsDayStats' 2ND-clock ladder ("39% last hr") + "=" for a read-in 1ST. Thresholds (`GP_DEPTH_T0/TW` 0.70,
`GP_SLOPE_STEEP` 0.10) are hand-set starting points — the CW/PW rows carry the shares so they can be measured. Nothing
from the day's discussions is now unbuilt except: the daily record's use (needs ~60 days), the volume / trend-day-flag
evening, the NQ cross-check, re-fitting `GP_EM_MODEL` on real pins. His install: Tampermonkey panel 16.35 + companion 1.21,
`compile-gammaprofile.bat` + `compile-daystats.bat` (+ `compile-kingtracker.bat` if 0.9 was not compiled yet).

# ⚠⚠⚠ 2026-09-16 ~21:05 CT — v16.34 / GP 0.49 / KT 0.9: SCALEREF NOW CARRIES ITS OWN MINUTE. START HERE.

0.48's "anchor on the 15:00 bar" was still wrong at 20:51 (PW drawn at the live price): Skylit's ES1 spot had frozen at
**14:26 CT** on FOMC day, not at the close. The fix that cannot be wrong by a clock assumption: the panel writes
`SCALEREF,<px>,<CT sod>,<CT date>` from the vendor minute of the quote (`levels[].t`), the plugins anchor on the bar of
THAT minute; no time → the RTH rule. He needs: Tampermonkey 16.34 + `compile-gammaprofile.bat` + `compile-kingtracker.bat`
(IRT closed). ⚠ The 0.48 RTH-rule fallback stays as the path for older CSVs. Open, seen tonight: the Day Stats A row's
HOD/LOD drifting +16/+18 after the close (the panel's Yahoo day bars re-scaled) — check first thing tomorrow.

# ⚠⚠⚠ 2026-09-16 ~19:45 CT — lsGammaProfile 0.48 / lsKingTracker 0.8: THE AFTER-HOURS OFFSET. START HERE.

His first look at Book = IF (0.47 installed, 16.32/16.33 panel) at ~17:xx CT: the IF book drew 22 pts high. SCALEREF
(Skylit's ES1 spot, from the SPX options book) freezes at the cash close; the chart does not; the 0.46 ASOF-bar anchor
absorbed the evening move. Fixed by anchoring on the last RTH-stamped bar at or before ASOF (both plugins). He needs
`compile-gammaprofile.bat` + `compile-kingtracker.bat` (IRT closed). Inside RTH the old and new anchors agree. Also seen
in that screenshot, working as designed: the Day Stats E row reading `1ST HOD ~10:48am` = the READ-IN branch of 16.32
(the actual first extreme once the classifier calls it), `2ND ~1:36pm` = the pos60-middle row.

# ⚠⚠⚠ 2026-09-16 ~19:15 CT — v16.33: THE CANDLE (IF EM + PLACEMENT) + THE DAILY RECORD. START HERE.

**Built and on origin (awaiting his Tampermonkey update — one link, no compile):** panel **16.33**. Operator: "ok. let's
build the updated indicators and using insider finance." (1) The day-model candle's range now uses the **0DTE straddle
from InsiderFinance pinned at the open** (`gpDayEmPin`, clean ≤15 min, `est` ≤60 min, otherwise the v16.18 stages) at
every stage, and the candle is **placed** by where the open sits in the opening range / initial balance (`gpDayModel`;
symmetric pre-open). Both from `design/EM-RANGE-STUDY.md` (n=299 out-of-fold: range −19% pre-open, E-HOD/E-LOD −25% at
60 min). (2) **The daily record** (`gpDayRecordTake` → `dayRecord` in the day file): regime sign, flip distance, walls,
King, EM, event — taken in the first 15 min after the open — so the doctrine inputs can be TESTED in ~60 sessions. Read
`design/EM-RANGE-STUDY.md` §7 and `design/KEY-LEVELS-STUDY.md` before proposing any other predictor: everything with a
history has been tried (EM + opening range work; prior-day range, overnight, gap, weekday, VIX1D close, all key levels:
null). v16.32 (the Day Stats E row conditional on the morning) is in the same update.

**Tomorrow's eyes-on:** at 08:30 the candle should draw symmetric with basis `em-exante` (EXPMODEL row) if Atlas was open;
at 09:00 it places (`em-open30`); at 09:30 `em-open60`. DAYSE's 1ST/2ND clocks switch stages the same way (`CONDE` row).
If Atlas opened late the basis reads the legacy stage and the EM field is blank — expected, not a bug.

**Still on the table, in order:** the IF extras (#1 wall-depth pill, mockup delivered) · the lsDayStats LADDER for the 2ND
clock (mockup first) · one evening on volume / prior-close location / a trend-day flag (recommendation given, not
started) · the NQ cross-check · re-fit `GP_EM_MODEL` on the real pins at ~60 days · items E, F step 9, D opens.

# ⚠⚠⚠ 2026-09-16 ~18:10 CT — v16.32: THE DAY STATS E ROW, CONDITIONAL ON THE MORNING. START HERE.

**Built and on origin (awaiting his Tampermonkey update):** panel **16.32**. The E row's 1ST / 2ND clocks, Took, HL Gap
and 1ST = LOD/HOD now come from `hodlodCondE()` by stage (pre-open medians → pos30 terciles → pos60 + the OR-extreme
clock rule → READ IN = the actual first extreme) instead of the weekday trimmed mean. Measured first, out-of-fold, base
re-derived per fold (`design/DAYSTATS-COND-STUDY.md`): 1ST clock 38.4 → 33.3 min, 1ST accuracy 0.48 → 0.61 (→ a fact
at READ IN); the 2ND clock is NOT predictable (93 min either way) — pooled median carried, no claim. New `CONDE` row.
His instruction: "test and confirm that the model is better than base" — done in the doc, per target, including the
one that is not. lsDayStats unchanged (same DAYSE row). Proposed next for the strip (mockup first): the LADDER for the
2ND clock ("in the last hour 39%") instead of a clock. Also still on the table: the candle (placement + EM,
`design/EM-RANGE-STUDY.md`, mockup delivered), the IF extras (#1 wall-depth pill), the daily record row for regime /
flip / King so they can be tested at all.

# ⚠⚠⚠ 2026-09-16 ~17:30 CT — IF FOR THE OTHER PLUGINS + THE EM STUDY (item C measured). START HERE.

**Where the discussion stands (nothing built since 16.31 / 0.47).** After the IF book, he asked how IF could feed the
OTHER plugins. Answered, in order of value: (1) lsDayModel — EM as the range budget, walls as hard caps on the projected
extremes, regime sign as the candle skew; (2) lsKingTracker — IF's King (their Magnet) as a slow "standing" track under
the flow King (agreement = confirmation, flow King with nothing behind it on IF = hedge-node signature); (3) lsDayStats —
realized range / EM per day; (4) FlexLevels SPY walls — leave. NOT: their intraday ΔGEX as a REAL-vs-HEDGE read (OI is
static; payload lags). He then asked whether the EM would make the expected high/low better or worse → "run the study".

**The study — `design/EM-RANGE-STUDY.md` (read it; n=299, 2025-06 → 2026-09, out-of-fold).** EM = best range predictor
at every stage (18.3 vs prior-day 22.6; beats OPEN30/OPEN60 alone; +9% on them when added). The expected HIGH/LOW barely
move (~1 pt): a PERFECT range placed symmetric round the open still errs 18.4/side — the candle's error is PLACEMENT.
Placement by where the open sits in the first hour: 17.4/22.0 → 12.8/16.8 (−25%). Proposal on the table, his rule
applies (discuss, mockup, one at a time): placement first, EM into the range second, record the pinned straddle daily.
Proxy caveat: VIX1D-open stands in for the straddle (level agrees 0.91–1.36× on 9 clean pins; not a validation).

**Also pending from the afternoon:** the two IF-extras mockups (`if-extras-mockup.html`: #1 wall-depth pill 0D/WK/MO on
CW/PW, #2 slope word on the regime chip) — delivered, awaiting his pick; his installs (16.31 + four DLL compiles + second
lsGammaProfile Book=IF); the first side-by-side run; items E, C(build), F step 9.

⚠ Browser landmine (new): `navigate` without a tabId took over the ATLAS tab (the panel's host) — always create a fresh
tab first and pass its id. Atlas was sent back within ~10 s; the recorder's state lives in localStorage.

# ⚠⚠⚠ 2026-09-16 15:05 CT — THE IF BOOK (item F) + the afternoon's fixes. START HERE.

**Latest on origin:** panel 16.30 · companion 1.20 · lsGammaProfile 0.47 · lsKingTracker 0.7 · lsDayModel 0.16 ·
lsDayStats 0.6. His last known installs: panel 16.29, companion 1.20, DLLs uncertain (GammaProfile 0.45 or 0.46 at
14:26; KingTracker / DayModel / DayStats possibly still old). **Every DLL needs one more compile pass.**

**What the afternoon built, in order:** the moving-profile fix (0.46 / KT 0.7: offset anchored on the ASOF bar) ·
DayModel 0.16 (bar-stamp-aware RTH window; swept PDH 7690→7687 = IRT) · companion 1.20 (couriers keep polling while
Atlas is hidden during the session) · two live regression runs (`testing/gamma-profile/RESULTS.md`: 13:25 and 14:26 —
the 7630 question RESOLVED as my misread of the velocity chip; IF's payload found 30 min behind on FOMC → item E) ·
**the IF book (item F)**: `design/IF-BOOK-OPTION.md` is the investigation, the numbers, the verdict (ADD, don't
replace) and the step-by-step plan; steps 1–6 built, 7–9 are his install, the first side-by-side run, and the
doctrine question of what IF's book is FOR.

**Next, in order:** (1) install: Tampermonkey [16.30] + close IRT + compile-gammaprofile / -kingtracker / -daymodel /
-daystats + reopen; add a second lsGammaProfile with Book = IF, Side = Left. (2) `gp-regress.py` on BOTH files at one
minute (the Skylit run with an Atlas zoom, the IF run without) → RESULTS.md. (3) Item E (flip payload age) — discuss.
(4) C (day candle, measure first). (5) design doc §5 step 9.

# ⚠⚠⚠ 2026-09-16 ~13:00 CT — THE REGRESSION SUITE (item D) + 0.45 / 16.29 / DayStats 0.6. START HERE.

**Read `testing/gamma-profile/REGRESSION.md` before touching lsGammaProfile or `gammaProfileBuild`.** Three layers now
exist and must stay green: `test_gammaprofile_build.js` (Gate A, 40), `plugin/test_gammaprofile_logic.cpp` (Gate B
logic, 57 — the plugin's decisions live in `plugin/GammaProfileLogic.h`), `tools/gp-regress.py` (the live same-moment
run from the CSV + `GammaProfile.audit.json` + Atlas/IRT screenshot transcriptions). Trail: `RESULTS.md`, `runs/`.

**Versions on origin:** panel 16.29 (audit sidecar; `SPY KING <strike>` label), lsGammaProfile 0.45 (bracket, tag
beside bubble, G·C), lsKingTracker 0.6, lsDayStats 0.6 (IQR removed), lsDayModel 0.16 (bar-stamp-aware RTH window: the swept PDH read the 08:27-08:30 bar as RTH on his end-stamped chart). **His install state when this
was written:** panel 16.28 running, lsGammaProfile DLL = 0.42 (compiled 11:12), KingTracker 0.5, DayStats 0.5 —
i.e. the 0.43/0.44/0.45 and KingTracker 0.6 changes are NOT on his chart until he runs the three compile bats.

**⚠⚠ DEPLOY LANDMINE FOUND 13:40 CT — READ BEFORE ANY `device_commit_files`:** the device bridge caches a staged
source by PATH + MTIME. `/mnt/user-data/outputs/` does not always advance a file's mtime on `cp`, so re-committing the
SAME staged path after editing re-sends the PREVIOUS bytes and reports "written". That is what "lost" the 16.28
userscript (11:21), the 0.44 GammaProfile.cpp (11:58) and DayModel 0.16 (13:37) — each landed only on the second try
from a path whose mtime differed. RULE: stage every commit into a FRESH directory (`/mnt/user-data/outputs/deploy-<HHMMSS>/`),
then confirm with `device_list_dir` that the device file's SIZE matches the clone's before waiting for the sync.

**Next, in order:** (1) he installs (Tampermonkey 16.29 · close IRT · compile-gammaprofile / compile-kingtracker /
compile-daystats / compile-daymodel · reopen) → (2) the FIRST LIVE RUN: computer-use approval, Atlas + IRT screenshots at one minute,
transcribe both, stage CSV + audit, `gp-regress.py --shots …`, record in RESULTS.md — that run is the acceptance test
for the bracket / G·C / tag placement he has not seen yet → (3) `run-logic-tests.bat` on his MSVC once → (4) C (the day
candle: measure first) → (5) T-run upgrade to gp-regress.

**Regime facts settled today (don't reopen):** sign = spot vs InsiderFinance's 0DTE zero-gamma (`dte0.gf.flip`, ours
over their chain, reconciled against their 0DTE header 2026-09-16: walls exact, flip within the spot move) with a 3-pt
AT buffer; type = Range / Trend / Whipsaw from the ladder's structure (`gpRegime`, `gexRegime` thresholds + the
absolute-value polarity of the three largest nodes within 30 pts + King rolls); conflict surfaced. The companion cannot
select IF's dropdown (it fetches the page); their header follows the dropdown on THEIR tab only. Cadence: the CSV
writes on the IRT export timer (3 m default; 1 m chip available), the plugin re-reads on every redraw.

# ⚠⚠⚠ 2026-09-16 LIVE SESSION — A + B BUILT (v16.27 · lsGammaProfile 0.42 · lsKingTracker 0.6). START HERE.

**Deployed to origin; his install is the next step** (Tampermonkey update → close IRT → `compile-gammaprofile.bat` +
`compile-kingtracker.bat` → reopen). Until he installs, the chart still runs 0.41 / 16.26 and the CSV has no
REGIME / FLIP / CW / PW rows and no pattern tags. **What shipped and why: `changelog/CHANGELOG.md` head.**

**The morning, in order (so the next context knows what was verified and what was not):**
1. 09:21 — recording confirmed (v16.26 lamps IRT 0m · IF 1m · YF 1m · FF 4ev, no replay banner). He restarted IRT.
2. 09:47 — **a real same-moment test**: the CSV (09:47:09), Atlas's SPXW ladder and the IRT rail agree row-for-row on
   strike / %King / rank (7635 +3 … 7580 −10; badges 5 on 7610, 4 on 7605), King 7685 SPX = 7689.25 front = 7755 Dec,
   SCALEREF 7614.06 vs ES1 7613.50. Day rows match DAY STATS. Only the two agreed defects showed: the G flood (A) and
   the regime line flapping FOLLOW→FADE between 09:27 and 09:48 on nothing (B).
3. **Skylit rolled ES1 to December between 09:47 and 10:38** (tab 7604 → 7686; CSV KING 7689.25 → 7756.75, SCALEREF
   7614 → 7687). The basis fix self-healed (King stayed at 7756 on his chart). Fallout fixed today: the validator's
   September scale band (T2 10:38 FAIL was the roll, not data) and lsKingTracker's history scale (0.6).
4. The regime conversation (how / standard / IF / 0DTE): settled as sign = spot vs the 0DTE flip, type = Skylit
   structure, conflict surfaced. **His screenshot showed IF's header follows its expiry dropdown** (0DTE: ZG 7617.03,
   CW 7675, PW 7600) — our capture had documented the default. The companion cannot select it (it fetches the page from
   Atlas); ours-over-their-chain reconciled: `dte0.gf.flip 7620.72 · cr 7675 · ps 7600` (his console paste, ~10:20).
5. He asked for CW / PW / FLIP "called out on the nodes themselves" → "keep them as options — lines or labels" →
   "build". Built without a mockup on his word; **first eyes-on after install is the acceptance test** — expect
   questions about tag placement (CW/PW beyond the tip, FLIP tick across the strip).

**Open after this build:** C (day candle: measure first with `tools/study-hodlod.py`, LOCKED-ITEMS C) · Q12 top-3 vs
top-5 · the scheduled T-runs' log rows for today · the T2 lesson: **a device write can be clobbered by a scheduled
run's repo reset** — always re-stage and confirm the file after a commit, and confirm origin.

# ⚠⚠⚠ SAVE 2026-09-15 ~9 PM CT — "save everything, we will do it tomorrow." START HERE.

**Everything with code is BUILT, DEPLOYED, INSTALLED and LIVE-VERIFIED on his machine** (origin `e1da736`): panel
v16.26 · lsGammaProfile 0.41 (Tape columns ON, strike|%King matching Skylit's ladder row-for-row) · lsDayModel 0.15
(candle low holds the RTH day, 7644 not 7652) · lsKingTracker 0.5. The King, the nodes, the SPXW→ES mapping
(ES = SPX + 4.25, ratio 1.000558 on all 100 nodes, = Skylit's own ES1 payload) and the Dec basis (~+76) are ALL
verified consistent with Atlas + tape. **Nothing is broken. Nothing is half-built.**

**TOMORROW'S ORDER (agreed, one at a time) — read `session-state/LOCKED-ITEMS.md` ⭐⭐ ACTIVE 2026-09-15 first:**
1. **A + B in ONE gamma build**: pattern labels per doctrine (Gatekeeper = ONE dominant blocker; air pocket bounded
   both sides; rug/rrug; pika/barney with S6 thresholds; magnitude overrides pattern) + the panel-written REGIME row
   (IF flip sign · three doctrine types · velocity · conflict flag) displayed by the plugin + the missing FLIP/CW/PW
   rows. One userscript update, one `compile-gammaprofile.bat`. Doctrine gate already run (all FOLLOWING).
2. **C — the day candle**: his "why is it so bad" is diagnosed (LOCKED-ITEMS C): structural symmetric range + the
   READ not clamping the geometry + no regime input. Recommendation on the table: **run `tools/study-hodlod.py`
   FIRST** to measure a READ-clamp + drive-skewed range across the recorded sessions, then decide. Do not hardcode a
   skew from one Tuesday.
3. **09:05 T1 same-moment audit** (scheduled) — confirm King rolls vs Atlas's 0DTE ladder as they happen.
4. Q12 (top-3 vs top-5) when there's a gap.

**How this session went, so the next one behaves the same way:** he pushed hard on VERIFICATION — "did you
actually compare…", "check my screen", "look at the other monitor" — and every time he was right to. Twice I
declared a mismatch that was the documented trap (front-vs-Dec scale coincidence 7579≈7581; the Atlas chart on a
"Rolling front + 3 · 3 Days" window vs the 0DTE tape). **Before calling anything a mismatch: pin BOOK, WINDOW and
SCALE, same moment.** He accepts "I haven't built it yet, I was waiting for your go" — he does NOT accept a claim of
verification that wasn't done. Give step-by-step (cmd, not PowerShell; the full `cd /d` line; close IRT for the DLL
swap; the raw GitHub link as a clickable link). Show a mockup before UI changes (he asked, and the first one had a
`top` global collision — renamed). Computer-use grants expire in ~hours and must be re-approved from the DESKTOP app
(phone approval never attaches); the claude-in-chrome extension was unusable on the Atlas tab all night.

# ⚠⚠ 2026-09-15 EVENING — v16.26 + lsGammaProfile 0.41 + lsDayModel 0.15: Tape columns + candle-low fix

**Evening live audit result (8 PM CT, overnight book):** gamma King, nodes and King lines ALL verified aligned — Atlas
SPXW ladder yellow-King 7500 = tape 7500 = IRT 7581 (Dec, basis ~+76 from the live overnight bar); ranks 1/2/3
match cell-for-cell. Two operator "mismatches" were the documented traps: (a) IRT's King "7577" vs an Atlas node
"7579" = same digits, different contracts (the 7579 is a front-scale secondary node = IRT 7655); (b) the Atlas chart
was on **"Rolling front + 3 · 3 Days"** (4 expirations) while the tape/IRT read 0DTE — window mismatch, not a bug.
- **lsGammaProfile 0.41 — "Tape columns"** (new, OFF by default, appended last): prints [raw SPXW strike | %King] per
  node at the pane edge (bars shift inward), so IRT reads row-for-row against Skylit's SPXW ladder. Outside-% folds
  into the column. **Needs recompile; then turn the setting ON.**
- **Panel v16.26:** STRIKE rows carry the raw SPXW strike as a 7th field (type at 6, empty). Additive.
- **lsDayModel 0.15 — candle-low session bug FIXED:** measureChartDay now measures the RTH day (08:30-15:00 of the most
  recent RTH day, stepping back over weekends), not IRT's rolling session — after the close it no longer jumps to the
  evening session's low. Operator-reported (7652 vs DAYLOD 7643.50). **Needs recompile.**
⚠ DEPLOY ORDER: reload Atlas → `cd /d "C:\Dev\gex-signal-tapereader\plugin"` then `compile-gammaprofile.bat`, then
(fresh cmd) `compile-daymodel.bat`, IRT closed for each DLL swap; confirm 0.41 / 0.15; enable "Tape columns".
⚠ STILL OPEN: the air-pocket band over-shades the whole thin far-OTM zone (needs a cap — only band BETWEEN real
nodes); the panel writes no CW/PW/FLIP/EM rows (walls never draw); Q12 top-3-vs-top-5 analysis; the full same-moment
RTH audit (T1 09:05) to confirm the King-roll history against Atlas's 0DTE ladder as rolls happen.

# ⚠⚠ 2026-09-15 — v16.25 + lsKingTracker 0.5: TIGHTER ANCHOR + King LINES on the Dec contract

Follow-up to v16.24 (read that section next). **Live-verified v16.24 first:** gamma King jumped 7564→7643 (~80pt
fix), G flood gone — but landed ~10-14 low because SCALEREF was a node-median biased ~10 high.
- **Panel v16.25:** SCALEREF now = the ES1 derived payload's own SPXW spot (`derived[SPXW].levels[last].s`, host/ES
  = true front price), fallbacks ladder-price→esOfSpx then node-median, sanity-gated ±200 of the King. Gamma-King
  residual ~10→~0. **No gamma recompile needed — just the Atlas reload.**
- **lsKingTracker 0.5:** had the SAME SPOT-anchor bug lsGammaProfile did — the front-scale King step-LINES stayed a
  full spread below price ("kings not correct"). Now anchors on SCALEREF → SPX/SPY King lines land on the Dec chart.
  SPOT fallback; ±300 clamp still protects NQ books. **Needs recompile (compile-kingtracker.bat).**
- **lsDayModel / lsDayStats — deliberately untouched:** their rows are ~Dec scale (SPY×D.scale) and self-align
  (measureChartDay open-anchors the expected candle to the chart's own open; DayStats shifts printed prices by
  chartClose−SPOT). ⚠ If the day candle still looks off after this, it's the ~1% SPY-ratio residual (a SEPARATE
  day-model scale item — the panel's `sessionBodyRaw` D.scale runs ~1% hot), NOT the contract basis. Verify live.
⚠ DEPLOY ORDER: reload Atlas (panel 16.25 → tighter gamma King + SCALEREF for the King tracker), THEN recompile
lsKingTracker: `cd /d "C:\Dev\gex-signal-tapereader\plugin"` then `compile-kingtracker.bat`, close IRT for the DLL
swap, reopen, confirm 0.5.

# ⚠⚠ 2026-09-15 — v16.24 + lsGammaProfile 0.40: THE CONTRACT-BASIS FIX (front ES1 vs charted EPZ26 Dec)

**THE session's big finding, diagnosed live (Atlas + tape + IRT at one moment).** The operator charts **EPZ26
(December)**; the panel reads Atlas's **ES1 (front month, Sep — expires 9/18)** and writes gamma levels in front
scale. Dec trades ~65–90 pts ABOVE front (calendar carry), so every King/node was drawn that far BELOW price on his
chart. Symptoms he reported: King "7564 not in IRT" (it was — ~87 pts under the Dec price), and "a lot of nodes with
G" (the spot↔King gap spanned ~100 pts because SPOT was ~Dec scale while the King was front scale).

⚠ **The node VALUES are correct — I verified vs Atlas:** Atlas yellow-Kings 7560 (=tape 7560→ES 7564); nodes 7584
68% (=tape 68%), 7599 58% (≈tape 59%); SPY King 758 (=tape 758, ±1 roll jitter). Only the *position* (scale) was
wrong. Do NOT "fix" node values — they read the book right.

**The fix (anchor everything to a front-price reference, shift in the plugin):**
- **Panel v16.24** — `gammaProfileBuild` emits **`SCALEREF,<frontES>`**: the front-month ES price the ladder is
  scaled to (esOfSpx of the SPXW index; fallback = median ES of the strongest nodes). Additive; harmless to others.
- **lsGammaProfile 0.40** — it ALREADY had `applyContractOffset` (v0.39) but anchored on **SPOT**, which the day
  model writes in ~Dec scale, so off≈0 and it never shifted the front-scale ladder. Now it anchors on **SCALEREF**
  (front scale = the nodes' scale): off = chartClose − SCALEREF = the Sep→Dec spread → the whole book lands on the
  Dec chart. **Spot is pinned to the chart's live close** for the marker + the Gatekeeper role test → collapses the
  G flood. Falls back to SPOT if SCALEREF absent. Self-heals to 0 once the front itself rolls to Dec (after 9/18).

**⚠ DEPLOY ORDER (told the operator):** (1) reload Atlas so the CSV starts carrying SCALEREF; (2) recompile
lsGammaProfile — `cd /d "C:\Dev\gex-signal-tapereader\plugin"` then `compile-gammaprofile.bat`, **close IRT** before
it swaps `lsGammaProfile.dll`, reopen, confirm **0.40** in the settings dialog. Panel changes go live on the Atlas
reload (5-min raw-cache wait).

**⚠ STILL OPEN — the same SCALEREF anchor must roll into the other three plugins** (lsKingTracker, lsDayModel,
lsDayStats) so the King line, day candle and stats align too — otherwise only the gamma rail is corrected. Plus two
separate gaps found this session: (a) the **air-pocket band over-shades** the whole thin far-OTM zone above price
(the "top area shaded wrong" — needs a cap so it only bands BETWEEN real nodes); (b) the panel writes **no
CW/PW/FLIP/EM rows**, so the flex walls never draw (the plugin is built for them; nothing to draw). Neither is in
this build. The anchor value is a node-median approximation (~few pts) — refine to the true SPXW index price if a
clean source is wired.

# ⚠⚠ 2026-09-15 — v16.23 + lsDayModel 0.14: THREE ROBUSTNESS FIXES (config ✕, auto-regrant, stale-candle guard)

Three operator pains, all "I don't want to get involved every time," fixed in one pass. **Panel v16.23 is deployed
(committed to `C:\Dev`, sync-pushed); lsDayModel 0.14 is SOURCE-ONLY — it needs a recompile+install to take effect.**

1. **"i cant even close the tapereader config."** The gear toggled the config but there was no visible close control.
   Added a real **✕** in the config title bar (`cfgHtml` → `.gpts-cfg-close` span, ~L9720; wired in `wireConfig`,
   ~L9903: `stopPropagation` + hide). The gear still toggles it too.
2. **The recurring folder re-grant** ("please make sure there is a fix … so i dont have to get involved evrerytime").
   Chrome wipes the File System Access permission to `prompt` on EVERY page load (the handle survives in IndexedDB,
   the grant does not), so the IRT + GammaProfile export goes silent after each reload until a click carries a fresh
   `requestPermission()`. New **`irtArmGestureResume()`** (~L6291) arms a ONE-TIME capture-phase document
   click/keydown listener that re-requests the grant **synchronously inside that first gesture** (a `.then()` chain
   loses activation — the v14.53 lesson) and, on `granted`, resets `IRT_TICK_LAST` and fires both writers at once.
   Armed at boot (~L8801) AND re-armed from all three `needsGesture` branches in `irtExportNow` (~L6478/6485/6490),
   so a MID-SESSION lapse also self-heals on the next click anywhere. If the grant persisted it resolves `granted`
   with no prompt; worst case he approves ONE prompt on his first click. This is the closest to "never get involved"
   the browser permits — a timer can never carry the grant (no activation), only a gesture can.
3. **The huge, mis-aligned "expected" candle "printing for no reason."** ROOT CAUSE was a STALE CSV, not the model:
   the file was cold since 23:58 the night before (folder grant had lapsed — see #2), so lsDayModel drew the frozen
   `DAYEXP` candle onto a book that no longer matched the chart. **lsDayModel 0.14** adds a STALE GUARD:
   `staleAgeMin()` (ONE shared age calc for the badge + the guard, overnight-wrap aware) and, past **15 min** since
   `ASOF` (five missed 3-min writes, well beyond jitter), the EXPECTED candle + its E-lines + annotations STOP
   drawing and are excluded from the panel bounds (no ballooned panel). The ACTUAL candle is measured live from the
   chart's own RTH bars (`measureChartDay`) so it keeps drawing; the STALE badge (v0.8, ≥4 min) still explains why
   EXP is gone. If stale AND no actual candle yet (pre-open with a cold file), only the badge shows.

**⚠ TO FINISH #3:** recompile lsDayModel and install the DLL — `compile-daymodel.bat` (a FRESH cmd window: PATH
overflows if builds run back-to-back), then **close IRT** before replacing `lsDayModel.dll` (IRT holds the DLL open),
reopen IRT. Confirm the settings dialog shows **0.14**. Until then the stale guard is not live; #1 and #2 are live
now (panel v16.23) after a **5-min** raw-cache wait + an Atlas tab RELOAD.

# ⚠⚠ 2026-09-15 — v16.22: THE KING STUDY IS RTH-ONLY (the roll count was after-hours chatter)

His console dump of `gpts_kingday_v1` was the whole story — SPY journey = 29 rolls, ALL after-hours
(`761@22:32 762@22:37 761@22:40 …` bouncing between two adjacent strikes 22:32–23:57), SPXW = none. Those
after-hours flips filled the 30-move buffer and EVICTED the real RTH journey → the line drew flat (all moves at
so≈81000, past every RTH bar) and the count was nonsense. The earlier "SPX 13 rolls" was the stale `krOf` census
fallback. Operator: **"limit the study to king node during rth."** Done, four ways:
- **Recording gated to RTH** — `updateKingJourney` returns outside 08:30–15:00 CT. No after-hours rolls recorded.
- **Display + roll count + deflection bars all RTH-only** — a journey already polluted with after-hours flips draws
  clean immediately (the 761↔762 moves are filtered on the draw side; `kingChartBars` caps at the RTH close).
- **`KING_CONFIRM_N` 2→4** — a strike must DWELL 4 polls to count as a roll; rapid near-tied flips drop, a real
  sustained roll still records. Verified: 761↔762 ×6 → 0 rolls; sustained 762 ×4 → 1 roll.
- **Dropped the `krOf` census fallback** (frozen since 09-03) — an empty journey draws no line, never stale data.
⚠ SPXW journeys during RTH via `trackSpxwNodes → sampleTapeHistory('SPXW')` (empty now only because after-hours).
**VERIFY tomorrow RTH:** roll count should be a handful matching Atlas, SPX line should step and sit on the gamma
King, deflection ledger a few clustered taps. The existing polluted journey already draws clean tonight.

# ⚠⚠ 2026-09-15 — v16.21: KING CHART, FLAT LINES + OVER-COUNTED DEFLECTIONS FIXED

His screenshot (v16.20 installed): both King lines FLAT all day, though the ledger read "SPY K 10 rolls · SPX K 13
rolls today" — so RECORDING WAS FINE; the DRAW was broken — plus ~26 "held" taps, one every few bars.
- **Flat lines = a time-mapping bug.** `kingSteps` derived each move's second-of-day from `now` (`nowSo-(now-m.t)`).
  On a PARKED / prior-day view (across midnight) that goes NEGATIVE, so every bar took the last King → flat.
  Fix (v16.21): stamp each move with its own `so` at record time (`updateKingJourney`), keep it through
  `kingJourney`, map by it; older moves (no `so`) fall back to `msToCtSecOfDay(m.t)` — an ABSOLUTE CT second-of-day
  matching the candle `.so` base (`naiveSecOfDay` = CT wall-clock). So existing rolls draw correctly too, no
  re-record needed. Verified: 6 rolls → a 6-step line.
- **Over-counted deflections.** `kingChartEvents` pushed one event per BAR near the line. Now ONE event per TOUCH
  (contiguous run, tolerating a 1-bar poke-out): break if the close crossed to the far side during the touch, else
  held. Verified: a 15-bar hover → 1 event (was ~7-8).
- ⚠ VERIFY on his reload: the SPX/SPY King lines should now STEP with the rolls (not flat), and the tap ledger
  should show a handful of clustered taps, not one per bar. The deflection pts is the touch's most extreme close-vs-King.

# ⚠⚠ 2026-09-15 — v16.20: THE KING CHART (SPX King tracked + placed exactly on Atlas)

The panel's King chart (node-ladder section toggle → "kingchart"; `kingChartHtml`, ~L9544) draws the SPX + SPY
Kings as step lines with deflect/break stats. Two real bugs, both fixed — diagnosed from the code + the KINGNOW
data, NOT guessed:
1. **SPX journey wasn't persisted.** `KINGDAY`/`loadKingDay` handled SPY+QQQ only; the SPXW journey was created
   live (`updateKingJourney` on the `sampleTapeHistory('SPXW')` path) but dropped on the install-forced reload →
   the chart fell back to the `krOf` census (frozen since 2026-09-03) → SPX King line flat/stale. Fix: SPXW + NDX
   are first-class in `KINGDAY` init, `KING_CONFIRM`, `KINGHIST`, and `loadKingDay`'s rehydrate list.
2. **SPX line was ~7 pts off Atlas.** It was drawn with `dispScale` (~1.0015); the Atlas-verified conversion is
   `esOfSpx` (skylitFutPx / persisted GP_SPXWR, ~1.00058). Fix: `kingSteps` now takes a converter; SPX uses
   `esOfSpxKC` (exact ES), SPY still uses `rr`. Verified against KINGNOW (strike 7595 → ES 7599.37): new line
   0.00 pts off the gamma King (24/24 node-for-node with Atlas); old dispScale line was +7.0.

**Deflect/break stats** compute in `kingChartEvents` from the (now correct) lines. The **hold-rate-by-factor**
panel is n-gated and fills from the tap record (design/KING-STUDY.md) — not a bug, data-accrual.
**⚠ OPEN / verify live during RTH:** the SPX/SPY King steps should sit on the gamma Kings and match Atlas; watch
that the SPX steps walk with the rolls (not flat). Note: the journey samples `tapeMap('SPXW')` while the gamma
King uses `tapeMapLive` — they agree during RTH (tapeMap only goes stale after the close); if they ever diverge
intraday, switch the journey to tapeMapLive. The krOf census (`ktTick`/`krTick`) is still frozen since 09-03 —
the chart no longer needs it (KINGDAY is primary), but the Atlas-comparable roll tally would want it revived.

> **THE DAY MODEL now lives in `design/DAY-MODEL.md` — the ONE source of truth. Read it first for anything about
> the expected candle / READ. It documents both layers, the 3 adaptive stages + coefficients, what's null, the
> self-calibration pipeline, and the file/function map. If any doc, tab or comment disagrees with it, it is stale.**
>
> **INDICATOR QA: `design/IRT-VS-SKYLIT-TESTING-PLAN.md` — the standing plan to check IRT indicators against Skylit
> (source of truth) at 5 CT checkpoints/day. GOLDEN RULE: same-moment snapshots only (a stale CSV vs live Atlas is
> noise). Two gates: Skylit↔CSV (panel) and CSV↔IRT (plugin). Logs → `testing/irt-vs-skylit/`. First live run: next RTH.**

# ⚠⚠ 2026-09-15 — v16.19: STAGE-3 (60-MIN IB) + THE INDICATOR SWEEP (all null — do NOT re-test)

**He asked whether indicators/IB/divergence/momentum/volume would improve the model. I TESTED them all over the
corpus (out-of-fold) instead of guessing. Result — the opening range already captures the day's volatility, so
almost nothing adds skill. Do not re-run these:**
- RANGE (on top of open30, MAE 19.4): +volume 19.5 (nothing), +relative-volume 19.5, +|drive| 19.6 (worse),
  +gap 19.7 (worse), +prior-day-range 19.5 (subsumed). **Only +open60 helped: 19.4 → 18.6 (~4%).**
- DIRECTION (drive30 68%): open-location 50%, gap 46%, rvol-gated 60% (thin) — nothing beats the opening drive.
- **BUILT: stage-3.** Once 60 min of RTH is in, RANGE re-anchors to the 60-min initial balance
  (`open60 = 25.47 + 1.099*IB60`, MAE 18.5, R^2 0.32); direction still the opening-30-min drive. Self-calibrating
  (study-hodlod `_predict_block` fits `open60` nightly into BASERATES.predict; the panel reads it, literal is fallback).
  Stage ladder now: exante (pre-open) → open30 (≥30 min) → open60 (≥60 min). `EXPMODEL` row names the stage.
- **The ONE real unexplored lever is the GEX STRUCTURE (King distance, air-pocket width, node density at the open) —
  H5, still "blocked" on the event-level ledger the nightly is accruing. That is where future modelling effort goes,
  NOT classical indicators. Do not add indicators; measure structure when the ledger exists.**


# ⚠⚠ 2026-09-15 — v16.18: THE EXPECTED CANDLE IS NOW A PREDICTIVE, ADAPTIVE, SELF-CALIBRATING MODEL

**His ask:** *"make sure I have a solid prediction model for the expected candle... make it a better predictive,
adaptive model that works."* I backtested the OLD model and told him the truth: the weekday-mean expected candle
is NOT predictive — **R² 2.3%, MAE 24.2pt** — it draws the same ~56pt Monday every Monday. So I built a real one.

**What the data says (out-of-fold, 283 ES sessions, `/tmp` backtest reproduced by tools/study-hodlod.py):**
- **OPEN30 (the strong predictor):** `range = 25.9 + 1.44 × openingRange30` → **R² 30%, MAE 19.5pt**. The opening
  30-min range predicts the day's range ~13× better than the weekday. Available 30 min into RTH.
- **EXANTE (at the open):** `range = 40.7 + 0.36 × priorDayRange` → MAE 23.0 (beats weekday's 24.2).
- **DIR30 (direction):** sign of the opening-30-min DRIVE calls the close **68%** (base 53%). A modest body lean.
- **Dropped as NULL:** gap (46%), prior-day direction (50%), and the v16.17 recent-weekday body lean (47% — worse
  than a coin flip; F-6 predicted it, the backtest confirmed it).

**How it works now (panel export, `if(haveE)` block ~L6631):**
- Pre-open / first 30 min → EXANTE (prior-day range) if available, else the weekday mean.
- 30 min in → re-anchor the expected RANGE to today's opening-30-min range (OPEN30), and lean the body with the
  opening drive. A wide open now predicts a big day (would have flagged 14-Sep); a quiet open predicts a small one.
- Clamp: expected range held within 0.4×–2.5× the weekday mean so one bad bar can't draw an absurd candle.
- `EXPMODEL,<basis>,<rngPts>,<driveSign>` CSV row exposes which stage drew the candle.
- ⚠ SCALE: uses `measureBars(sym).bars` (ES-native on the ES chart) via `hlToolBars` — same tool grid + RTH open +
  ES points as the corpus, so the coefficients transfer. Do NOT use `closedCandles` here (SPY-scale, ×rr).

**SELF-CALIBRATING (his "auto/adapting" requirement):** the coefficients are NOT hardcoded — `tools/study-hodlod.py`
now fits them every night (`_predict_block`, RTH-only inputs) and writes `BASERATES.predict`; `tools/bake-hodlod.py`
carries them into the `HODLOD_BASE.predict` literal; `hlBaseNormalise` passes `predict` through so the couriered live
file overrides the literal. So as the corpus grows, the model re-fits itself. Panel literals are the boot fallback.
Verified: baked open30 a=25.9 b=1.44 R²0.30, exante a=40.7 b=0.36, dir30 0.674 (298 sessions).

**⚠ NEXT / OPEN:** the READ layer (HLTAB, AUC 0.879) is the OTHER predictive piece and answers "has the extreme
printed" — a rendered elapsed-time ladder is still unbuilt. The expected model's DIRECTION is only 68% (modest);
adding today's GEX structure (King distance, air pocket) to the range/direction is untested and F-4-gated (don't add
complexity that doesn't earn it — measure first). VERIFY LIVE: expected range should widen on a wide open, and the
`EXPMODEL` basis should read `exante` pre-open, `open30` after ~09:00 CT.

# ⚠⚠ 2026-09-15 — v16.17 + lsDayStats v0.5 + lsDayModel v0.13: THE READ ON THE CHART (still current)

**His ask:** *"fix everything and make the model auto ... lookup how it is self enhancing ... make sure it is a really
good model, lookup all the testing we did ... make sure it is adapting."* Driven by the day-model investigation.

**What the testing actually says (the evidence that drove this — do not re-litigate):**
- **The good model already existed but was STRANDED IN THE BROWSER.** The validated layer is the **HLTAB READ**
  (`tools/model-lodhod.py`, FINDINGS **F-4**): AUC **0.879**, 2-axis (posr × minutes-since-open), regime-STABLE
  (better on volatile days, 0.899), transfers ES↔NQ (**F-7**). Adding a 3rd axis made it WORSE; a 5-feature
  regression is ceremony — **ship the table**. It runs live in the Chrome READ box but was **never written to the
  CSV**, so no RTX plugin could show it. THAT was the gap.
- **The candle is climatology** — `hodlodBaseFor(dow)` → BASERATES.json (298 sessions, per-weekday trimmed mean),
  symmetric, `close=open`. It reports the CENTRE and by design excludes outliers (operator 2026-08-28), so a 2×
  range day (Mon 14 Sep: actual +113.3 vs expected +55.5, IQR 34–74) reads as "unexpected" — correct behaviour,
  a genuine top-decile day, NOT a model failure.
- **DO NOT BUILD a predicted green/red close (F-6):** sign-now already = 83%, extra features never change the call,
  and it's overconfident. So the expected body is the weekday BASE-RATE lean, faint, never a forecast.
- **Self-enhance loop is real but YOUNG:** register.json (10 hypotheses) + `tools/nightly/run.py` (Wilson + shuffle
  null). 8/10 are THIN — only ~11 live-export sessions scored since 2026-09-03. It fills with sessions; can't force it.
- **AUTO is already built:** panel auto-writes the day after the close and retries (v15.71); the "GEX nightly" task
  rebakes BASERATES within ~10 min (v15.68). Nothing to rebuild — VERIFY the scheduled task is installed.

**What shipped (all on origin):**
- **panel v16.17** — (1) NEW `READ` CSV row: `READ,<first>,<posr%>,<cellPct>,<cellN>,<call IN|NOTIN|HOLD>` from
  `lodhodCall(D)` — the HLTAB READ, the model's ADAPTIVE layer, now on the chart (line ~6614). NOTIN is the STRONGER
  call (85% vs 63%). (2) EXPECTED candle now has a BODY: `eClose = O + lean·(rngPts·0.15)`, lean = recent-6 weekday
  (green−red)/n, capped 15% — faint base-rate lean, F-6-honest (line ~6633). DAYEXP close≠open now.
- **lsDayStats v0.5** — parses `READ`, renders it CENTER-JUSTIFIED on the DAY STATS title line ("LOD IN 84%",
  green=IN / amber=NOT IN) via new `textCJ`. (v0.4 red-cell colouring + E-on-top + combined $/pt cells were already
  in source — they ship now on first compile.)
- **lsDayModel v0.13** — MUD box: label on its OWN line above the points ("MUD" / "+113.3" / time / $), actual + E-MUD.

**⚠ NEXT / OPEN (evidence-gated, not started):** recent-regime blend on the expected RANGE (F-4 warns: don't add
complexity that doesn't earn it — hold unless he asks); bake full-corpus weekday greenPct so the body lean isn't n=6
noisy; the elapsed-time READ ladder as its own strip. Verify live: the READ line needs live 1-min bars to populate.

---
_(prior history below)_

# ⚠⚠⚠ IRT INDICATOR ECOSYSTEM — CURRENT FOCUS (2026-09-14). READ `session-state/2026-09-14_resume-v16.09.md` FIRST.

The live work is the IRT (Investor/RT) indicators drawn on his real ES/NQ futures charts, fed by a compact
`GammaProfile.csv` the panel now WRITES. **Four RTX plugins share that one CSV** (each ignores the others' rows):
`lsGammaProfile` (gamma histogram + level rail, BUILT), `lsDayModel` (the day CANDLE, BUILT v0.7), and — source
ready this session, compile+calibrate next — `lsDayStats` (the §10.2 stats strip) + `lsKingTracker` (stepped King
lines: SPX/SPY on ES, QQQ/NDX on NQ). The full CSV schema, the build flow, and what to verify live are in the
2026-09-14 session note.

**⚠ THE DAY MODEL (do not re-discover — a prior context did not know it and it cost time).** TWO evidence bases,
never fused: (1) the EXPECTED **candle geometry** = `hodlodBaseFor(weekday)` → **BASERATES.json / HODLOD_BASE**
(297 ES sessions, per weekday, trimmed mean; firstClock/secondClock, took/gap, range±IQR, wick family). (2) the
**READ/classifier** = `tools/model-lodhod.py` (HLTAB, **AUC 0.879**, posr×minutes) — "has the HOD/LOD printed?".
The classifier is a SEPARATE layer, is NOT the candle, and is NOT written to the CSV. `mockups/hodlod-v2-SPEC.md`.

**⚠ THE KING-SOURCE LESSON.** The gamma nodes read `tapeMapLive('SPXW')` (the CURRENT DOM tape) — NOT
`tapeMap('SPXW')` (serves a STALE saved book after the close; a derived lane at 100% out-votes the real King).
`ladderCellParse` already SIGNS the King; `kingResolve` preserves it — never override the sign.


# ⚠⚠⚠ THE WHAT AND THE HOW — STANDING, EVERY CONTEXT, BEFORE ANYTHING ELSE

**THE WHAT** (`design/PURPOSE.md`, his words): identify the two turning points — the HOD and the LOD — to profit from
the move between them; secondarily the pullback turning points (deflections that resume a trend), relying on gamma
levels: **a gamma node deflects price, and the deflection IS the turning point.** Confusing the pullback deflection
(stay in) with the HOD/LOD deflection (turn around) is the expensive error.

**THE HOW** (`design/PROCESS.md`): the loop ① RECORD → ② EXPORT → ③ PUSH → ④ NIGHTLY → ⑤ REVIEW → ⑥ REGISTRY →
⑦ BUILD → ⑧ INSTALL → ⑨ GATE → ⑩ DASHBOARD / THE READ → ⑪ SCORE. Dashboard = act, Analysis = ask and read (the
registry + TRACK), Testing = trust and promote (the register, the gate, what the ladder renders and why). Every rate
with its n; a scorer must be able to fail; a hypothesis is written before the data and read once at minN; a first
read is never a verdict; every file the panel fetches rides the installer; probe the live panel after every install.
**THE PLAN** is `roadmap/ROADMAP.md` and the 🗺 Roadmap tab (v15.59 the ⚙ Architecture + 🗺 Roadmap tabs · v15.60 📌 Open Items · v15.61 the ladder floor · v15.62 the mockups' look + 📚 Learn · v15.63 the dashboard conversation · v15.64 the second dashboard conversation + roadmap/ and archive/ recovered · v15.65 the PATTERN columns + the Kings' colours · v15.66 THE TAPE (the whole book every bar every market) · v15.67 the setups and patterns SCORED + the complete architecture (⚙ ⑥–⑨, design/ARCHITECTURE.md) · v15.68 THE LOOP CLOSES ON THE CLICK (the GEX nightly task on his machine; the nightly writes the Analysis tab) · v15.69 THE OBJECTIVE OUTCOMES turn / stay in + the Learn rules carry the record · v15.70 💡 REC + the Data Analysis process named and pinned + markets.json · v15.71 the deflection candidate score · v15.72 score the READ · v15.73 the TAP record). **The WHAT and the HOW are INSIDE THE APP from v15.59** (⚙ Architecture tab, rendered from `learning/plan.json` = `tools/plan-seed.py`; the seed, the file and the docs are pinned equal by `test_v1559.js`). Edit the plan in `tools/plan-seed.py`, run it, re-splice `PLAN_SEED`, update the docs.
**Tighten and harden the machinery over time** — PROCESS.md §5 is the standing backlog; add to it when something
breaks, and say what broke in LESSONS.
**MATCH SKYLIT, ALWAYS (his standing rule, 2026-09-07):** *"We should match with skylit always in order to have a source
of truth to compare against."* What the panel and the IRT export draw mirrors what Skylit shows for the same setting;
a deliberate difference is named, never silent (DECISIONS 2026-09-07). The one designed difference — the 2-minute King
latch — was closed by his "match skylit" (v15.88): the latch is OFF, one number (`KING_LATCH_MS`) brings it back.

# ⚠⚠⚠ THE DATA ANALYSIS PROCESS — named by him 2026-09-04; `design/DATA-ANALYSIS-PROCESS.md`; READ IT FIRST

CAPTURE → ANALYSIS → TESTING → LEARNING → REC → DASHBOARD → SCORE. One definition end to end. **Since v15.71 he has no
step at the close: the panel writes the day itself** (after the close, 15:01 CT and later, retried every 10 minutes
until the file is confirmed in the repo folder; any earlier day it missed outside market hours, write-if-absent); the
💾 is the override and the footer's **💾 DUE** chip is the one click Chrome may need for the folder permission after a
reload ("Allow on every visit" there, once). The "GEX nightly" task on his machine runs the nightly within ~10 minutes
of the file; the nightly writes the log, the registry (Analysis), the Learn rules' verdicts, and the Rec file; his ✓ /
✗ on Rec rides the next day file; nothing on the face changes except through Rec. The review (a session) is the one
stage that still waits for one. Eight tabs, the final set. Every market-specific number is in `learning/markets.json`.
`test_data_analysis_process.js` pins all of it. His expectation, verbatim: "just click on the save button once a day
probably eod, and from that point on you take over from data, analysis, testing, learning all the way to the Rec tab,
which is where we will discuss what to implement." — then: "the next step is to automatically have the application
trigger the save button instead of me clicking it … instead of 5pm can you just modify so it is after market hours."

# ⚠⚠ 2026-09-11 (latest) — v15.99: THE KING CHART, FIXED FOR REAL — DRAW FROM kingDay.moves, NOT THE FROZEN CENSUS

**Panel is v15.99.** v15.98 (the previous "fix") made the chart show NOTHING and he was rightly frustrated:
*"you cant even show the king movements today that we have been recording for weeks."* v15.98 had switched the chart to read
the King-rolls CENSUS `krOf(book)` — and that census had **frozen 8 days stale**.

**How it was diagnosed (NOT guessed — this is the whole point).** Instead of shipping a fourth blind fix, I had him paste one
zero-install console command that dumped all three King stores from `localStorage`. The result, 2026-09-11:
- `gpts_kingday_v1` → **KINGDAY.SPY** `day 2026-9-11`, **5 rolls today** (6 points). **KINGDAY.SPXW** `day 2026-9-11`, seed
  only, **0 rolls** — the SPX King genuinely held one strike all day (so a flat SPX line is CORRECT).
- `gpts_kingraw_v1` (the census v15.98 used) → **`day 2026-09-03`** — 8 days stale. `gpts_kingtrack_v1` → also `2026-09-03`.

**⚠⚠ THE CORRECTED KEY TRUTH (this REPLACES the v15.98 note's "use the census" guidance, which was WRONG).** There are three
King stores, and the right one for "what happened all day" is the humble one I dismissed:
- `KINGHIST.seq` — dense per-3-min — but **IN-MEMORY ONLY, wiped on every tab reload** (which the install steps force). Useless
  for a whole day.
- `KRAW`/`krOf` (the census) — survives reload, BUT its writer `ktTick`/`krTick` is gated by `recorderBlind()`+RTH and **has
  not fired since 2026-09-03**. Do NOT rely on it being fresh.
- `KINGDAY.moves` (`gpts_kingday_v1`) — **survives reload AND written on the UNGATED SPY sample path.** THIS is the source.

**v15.99 fix:** `kingSteps(book)` now reads `kingJourney(book)` = `kingDay(book).moves` first, `krOf(book)` as a fallback so
it can never blank again. A book with only its seed draws a **flat held line** (not blank). A plain roll tally sits beside the
taps: *"SPY K — 5 rolls today · SPX K — held all day."* Second-of-day bar-matching unchanged. `test_v1596` 18; suite 166/176.

**⚠ OPEN / NEXT BUILD (named, not dropped):** the census `ktTick`/`krTick` has not written since 2026-09-03. The **"held /
broke" tap stats and the Atlas-comparable roll count depend on it**, so the next step is to find why it stopped firing — is
`ktTick` (line ~26974 caller) reached? is `recorderBlind()` stuck true? is `ladderKings(EB,sym)` returning empty? Do this
BEFORE enriching the King study. One thing at a time; the chart draws first (done in v15.99).

**⚠ SCALE unchanged:** SPY King exact (× rr), SPX King approx (× dispScale, `~`).
**OPEN with him:** he still wants to VERIFY the chart's kings against Atlas. The panel's kings ARE Skylit/Atlas's data; I
CANNOT browse his Atlas tab from the cloud (not targetable; a second Atlas tab double-records). Verify via his console dump vs
Atlas, or after the close on a full live day.

# ⚠⚠ 2026-09-11 — v15.98: (SUPERSEDED by v15.99) THE KING CHART READ THE ROLL CENSUS (krOf)

**⚠ This build's central decision was WRONG and is corrected above.** v15.98 switched `kingSteps` to `krOf(book)` (the census)
believing it was the Atlas-comparable authoritative store — but the census was frozen 8 days stale, so the chart drew NOTHING.
The lesson (LESSONS v15.99): "authoritative" is worthless if the store isn't being written; check a store holds TODAY's data
before trusting it, and prefer the ungated reload-surviving store (KINGDAY). Kept here as history; do not follow its guidance.

# ⚠⚠ 2026-09-11 — v15.97: THE KING CHART FIX — THE STEP LINES WERE FLAT

**Panel is v15.97.** On v15.96 he sent a screenshot: **"why dont you show any king movements and deflections … there should
be step lines going up and down throughout the day."** The King lines drew **flat** (SPX flat at the top, SPY flat at the
bottom), no rolls, no markers.

**Cause:** the King journey (`kingDay(book).moves`) stamps each move with `Date.now()` (ms); the candle `.t` field is not
reliably that same unit/epoch, so `kingAt(book, bar.t)` matched no move and fell back to ONE strike per line. Both journeys
were fine (SPY, and SPXW via the explicit `sampleTapeHistory('SPXW')`) — only the per-bar lookup was wrong. **A line that
drew-but-flat proved the source existed; the bug was the transform.**

**Fix (v15.97):** `kingChartHtml` steps the Kings by **second-of-day** — every candle carries `.so` reliably. A local
`kingSteps(book, scale)` converts each move's age to a second-of-day (`ctNowSecOfDay() − (Date.now() − move.t)/1000`) and, per
bar, takes the latest move with `so ≤ bar.so`. No absolute-ms or timezone comparison. Both lines now step through the day and
the ▲/✕ markers (which read the lines) land right. SPY exact (×rr), SPX approx (×dispScale, `~`), unchanged.
**VERIFY (v15.97):** on the King chart the two King lines should now walk up/down with the rolls, not sit flat, and taps should
appear where price met a stepped line. If a line is still flat, its journey genuinely has one entry today (reseeded on a
reload) — it fills as the King rolls.

# ⚠⚠ 2026-09-11 — v15.96: THE KING CHART (price vs the SPX / SPY Kings, as steps)

**Panel is v15.96, companion v1.19 (unchanged).** His ask: **"a 3 min candle chart that display spy and spx king lines as
steps, tracking their movements and when and where they deflected or price broke through."** Picked **mockup C**, and set the
order: **the view first, the factor stats as data accrues** ("first lets get the study done that you have and then we will
enrich it").

**What shipped — a new view in the node-ladder section**, a toggle (`CFG.ladderView`: grid ⇄ kingchart, persisted, default
grid). `kingChartHtml(sym)` / `kingChartSvg(...)`: ES 3-minute candles (`closedCandles('SPY')` × ratio), **SPX King (gold
solid) + SPY King (cyan dashed) as STEP lines** from `kingAt(book, bar.t)` (the recorded `kingDay(book).moves` journey),
**deflect (▲) / break (✕) markers** computed the doctrine way — the WICK tests within ~1 ATR, the CLOSE decides — a **ledger**
beside it, then **sourced insights** (doctrine + one `prov · n=4`) and an **n-gated hold-rate-by-factor shell**
(`recording · n=0` until filled). All real recorded data; no rate without its n.

**⚠ SCALE — the one honest limit:** the **SPY King is exact** (`kingAt('SPY')`×rr); the **SPX King is approximate** — its
strike journey is recorded but the SPX→display ratio journey is NOT, so the current `dispScale` is applied back and the SPX
line wears **`~`**. Exact = record the ratio per bar (enrichment). Do not "fix" this by pretending it's exact.

**THE FACTOR BACKLOG is `design/KING-STUDY.md`** — his full list for why a King holds/breaks: roll up/down · growth/decline ·
pika-stack support · polarity · level confluence · trend vs 50-MA · time of day · above/below open · **SPX·SPY·QQQ book
confluence ("at least one matters a lot")** · distance. Each becomes a factor the tap record (v15.97) splits hold-rate on, at
n ≥ 15 with a Wilson low. Build the factors as the data accrues — NOT before.

**Tests:** `test_v1596` 17 (the wick-tests/close-decides detection, the `~` scale, the n-gated shell, the toggle end to end).
**VERIFY ON HIS PANEL DURING RTH (v15.96):** toggle the node-ladder section to "King chart" — the candles, the two King step
lines (SPY exact, SPX `~`), and the ▲/✕ markers where price met each King. Watch the SPX-King scale especially, and whether
the markers land where he'd call the deflects/breaks.

# ⚠⚠ 2026-09-10 — v15.95: THE SETTINGS PANEL, TRIMMED

**Panel is v15.95, companion v1.19 (unchanged — ONE link).** His ask, after he selected 1m for the IRT cadence and then
couldn't scroll the ⚙ panel to a save button: **"there are a lot of things on the settings that are old and not in use.
review what needs to be removed and its associated code in the javascript file."**

**First, the "save button" confusion:** there is none, and none is needed — every ⚙ control persists on click (its handler
calls `saveCfg()` → localStorage immediately). The panel had just outgrown its height; the cleanup is the scroll fix.

**Removed (each traced to zero consumers FIRST, then cut with its code):** the legacy BO-pullback signal block — `BO Pullback`
(`CFG.boPb`), `BO Followthrough Req` (`CFG.ftReq`), `Signal Type` (`CFG.dir` + the `segBtn()` builder) — the tool's original v8
purpose, inert since it became the HOD/LOD + deflection panel (each wrote a flag no render reads); **Compact node cells**
(`CFG.compact`, superseded by the v15.63 ladder-grid); the **`pbNode` alert row** (the one alert with no `fireAlert('pbNode')`
— it could never fire). Header **"BO Pullback Config" → "Tapereader config"**; gear tooltip rewritten.
**Kept, because each has a live consumer** (a trace, not a guess): **Node Thresh** (→ `MIN_STRENGTH`, renamed off "BO", now #1)
and **Trend + Trend MA** (the trend engine + the nightly recommender, now #2); the six alerts that fire (feedStale, kingRoll,
inplayAccum, dissipate, absorption, trap). **Nothing on the face changed.**

**Tests:** `test_v1595` 22 — pins the removals as CODE constructs (no assignment / no rendered label — tolerant of records that
name them) and the keepers as present-and-wired. No existing test referenced a removed control (they were dead).
**VERIFY AT A CALM TIME (v15.95):** install off the open (a reload restarts his recorder). On the live panel: ⚙ opens to
"Tapereader config", scrolls to its foot, and the BO Pullback / Followthrough / Signal Type / Compact rows are gone; Node
Thresh, Trend, the IRT block and the six alerts remain and still work.

# ⚠⚠ 2026-09-10 — v15.94: THE CANDLE LIQUIDITY MAP · TOOK FROM THE OPEN · PFH/PFL · SWEPT + TARGET + NEXT DRAW

**Panel is v15.94, companion v1.19 (unchanged — ONE link).** Three things, all from 2026-09-10, all on the ⓪a daily candle.
His words: **"fix it"** (the candle showed 4h30 beside LOD 10:27 — *"the difference between the open and 10:27 is not 4h30"*)
· **"you must ensure you are accurately tracking the overnight low, prior day low, the prior full low, the weekly low … and
their corresponding highs … indicate which levels were swept and targetted for their liquidity … prior day poc, vah, val,
cw0, pw0 … shown when swept or targetted on the daily candle"** · **"targeted is the other extremity … a green bar that swept
overnight low and then went to (prior full high) the PFHI, which is the target"** · then the mockup: **"this is good, just
make sure the swept text doesn't overlap … fix and build"**.

**(1) The took under each extreme.** Was the leg AFTER the extreme (the LOD's was `P.lcMin`, 10:27 → close = 4h30) drawn under
the LOD clock, so it read as the LOD's distance from the open, which it was not. Now the number under each extreme is the
**TOOK FROM THE OPEN**: first = `D.took` (`hodLod`'s `(firstT-openSec)/60`), second = `D.took + D.gap`. (dayCandleSvg:
`afterHod = firstUp ? tookFirst : tookSecond`, `afterLod` the mirror.)
**(2) PFH / PFL = the prior FULL Globex session.** New `priorFullHL(dayStr)` folds the prior RTH day (`futSessionBars(1).rth`,
which now returns `byDay[k].key`) with that day's own overnight (`overnightHL(key)`), giving the prior full high/low — his
**PFHI / PFLO** — distinct from PDH/PDL (prior RTH only). Added tier 1 (`LEVEL_TIER`) and to `sweepLevelsToday`.
**(3) One axis: SWEPT + TARGET + NEXT DRAW.** `candleDraws(sym, D, dayStr)` finds the **TARGET** (cyan ring) — the un-swept
level the SECOND extreme tagged within tol `max(2,|px|·0.0004)` (his green day: swept ONL → targeted PFHI) — and the **NEXT
DRAW** (grey ring), the nearest un-swept level clearly beyond the second extreme. Swept + target + draw build one `items[]`
through the **same** STEP anti-overlap stacking (so nothing overlaps — his ask), swept wins a label collision (`seen[]`),
**CW0 / PW0** now draw on the candle too. Header **SWEPT / TARGET ▸**.

**Tests:** `test_v1594` 20 (4 mutants); `test_v1578` / `test_v1580` / `test_nodeat` re-pinned to took-from-open · PFH/PFL tier 1
· SWEPT / TARGET ▸, the 18-crowd rebuilt on 18 **distinct** levels (the new `seen[]` dedupe collapses a same-label crowd).
**⚠ Pre-existing red, NOT this build (red since ≤ v15.93, harness drift on removed/renamed functions):** `test_tapeking`,
`test_expiry_profile`, `test_node_map`, `test_sma_cont`, `test_v1126_process`. Leave them unless he asks — they are not v15.94.
**VERIFY AT A CALM TIME (v15.94):** installing reloads the Atlas tab and **a reload restarts his recorder**, so install OFF
the open — after the opening range settles. On the live candle: the number under each extreme is its took from 08:30 (not a
leg after); a swept day shows SWEPT / TARGET ▸ with ticks for swept levels and cyan/grey rings for the target/next draw, none
overlapping; PFHI/PFLO appear when swept or targeted.

# ⚠⚠ 2026-09-09 — v15.93: THE WICK OPEN = THE RTH OPEN ("RTH") · SAME YAHOO FEED, ONE STABLE OPEN

**Panel is v15.93, companion v1.19 (unchanged — ONE link).** He compared his FuturesPulse tool (a Replit app, Elliott-wave
+ an A/E row like the panel's) against the panel and caught a wick% mismatch: his tool **9%**, the panel **15%**. Verbatim:
*"why not just have NDX King… also why is the put wall 7635…"* (v15.92) then, on the wick%: *"double check why your values
for wick% and other dont match"* → *"check it.. it is being displayed"* → *"the tool gets data from yahoo finance"* → **"RTH"**.
**What it was.** Every A-row field matched to the minute EXCEPT wick%. wick% = |open − HOD| / range; both agree the HOD
(7665.00) and range (36.25), so the whole gap is the **session open**. His tool opened on the **08:28** print (7661.75 →
9%), the panel on **08:27** (7659.50 → 15%, the v15.87 "tools" mirror) — both PRE-open, and his tool's open minute was not
stable (08:27 on 09-08, 08:28 on 09-09). Both tools pull the SAME Yahoo ES=F feed; the difference was never the data, only
which minute each called "the open". **He chose the true RTH open — Yahoo's 08:30:00 print (7660.75 → 12% on 09-09).**
**What changed.** `HL_TOOL_A` (panel) + `LOAD_A` (tools/study-hodlod.py) 08:27 → 08:30 — the first RTH bar is 08:30–08:32,
its open IS the session open, pre-open leaves the extremes, **every clock is unchanged** (only wick%, the wick reclaim and
the green/red open move). `MIN_BARS` 386 → 383 keeps the same 295 sessions. BASERATES re-folded (ES 295, NQ 195),
`HODLOD_BASE` re-baked — E row and A row one definition. On 09-08 the A row now reads wick% **14** (open 7711.50), MD
**$1,962.50** — the numbers the panel had BEFORE v15.87 chased his tool.
**⚠ The standing rule "match his tool" is bounded by his tool being consistent** (DECISIONS 2026-09-09 v15.93): where his
tool's own output can't be reproduced by a stable rule, the canonical market definition wins — his explicit call (he chose
RTH over mirroring the 9%). A future context must not "restore" the 08:27 open to match a one-day sample.
**VERIFY AT THE OPEN (v15.93):** the A row's wick% reads off the 08:30 open (on a day like 09-09, ~12% not 15%); the A-row
hover says "the OPEN is the RTH open (the 08:30:00 print, the bar 08:30–08:32)"; after his nightly the E row's wick fields
re-fold on the RTH open (the Data/Analysis tabs' ⟳ lines carry today). `__gptsDebug` — `hlBase()` and the A row.
**NEXT, in order:** (a) verify at the open (v15.93 above; the v15.92 / v15.91 lists below still apply); (b) one real ✓ on
Rec (R-33) and his ✓/✗ on R-34/R-35/R-1/R-2/R-4/R-5/R-6; (c) **v15.94 THE TAP RECORD, element 1** — asked before built
(104 rows WAIT on it); (d) readers for the 15 READY rows (K4.1/K4.2 first); (e) Q12 ~09-15; (f) seasonality (v15.95).

# ⚠⚠ 2026-09-09 — v15.92: KINGS + WALLS ("there are way too many levels on my irt charts") · NDX KING BY NAME · THE NQ WALLS · ⚙ IRT › LINES

**Panel is v15.92, companion v1.19 (unchanged — ONE link).** His ruling, verbatim: *"i realize you are tracking the kings, but
there are way too many levels on my irt charts. i want to reduce it to kings along with the put and call wall only for both
the ES and NQ. you can continue your study."* Before it, on the D rows: *"why not just have NDX King as the label and its
gamma levels could just be XG2 etc. similar to what we are doing for ES. i dont see why any king needs to have 100%. also
why is the put wall 7635 when insider finance has it at 7600. is this a conversion or an error?"* — the list was stated
line by line with that morning's numbers and he said *"ok"*.
**The file is EIGHT lines:** `EPU26` SPXW KING · SPY KING · CW0 · PW0 — `ENQU26` QQQ KING · NDX KING · CW0 · PW0 (21 the tick
before). Everything else the export computed since v14.20 — XG2–5, SG2–5, G2–5, FLIP0, the D rows — is STILL COMPUTED
AND LATCHED every tick ("you can continue your study": the panel, the record and the studies keep every level) and not
written while `CFG.irt.lines` is `kw` (the default; a config without the key is `kw`). **⚙ IRT › Lines: Kings + walls /
everything** — one click writes every family again exactly as v15.91 did (a family comes back by name, no build).
**NDX KING** = the NDX options book's own King: `futDerBookKing('NQ1', ['NDXP','NDX'])` reads the NQ1 payload's NDXP book (the
weeklies — the SPXW analog), its largest |dollars| row at Skylit's price (29355.41 = strike 29330 that morning), bare
label, full gold / purple by polarity, width 3, FRONT payload first, held day-scoped under `NDX`. It replaces the D-NDX
KING row (the everything-mode D list dedupes against it). **The NQ walls** = InsiderFinance's QQQ 0DTE walls (721 / 714 that
hour, `ifChain('QQQ').dte0.lv`) at Skylit's QQQ ratio via `nqPx`, red / green, width 2, `IF_STALE_MIN`, a ±15% scale check
against their spot — NQ never had walls in the file; the NDX chain is NOT fetched (his word would add it: a companion
change, both links). **The shade says the book on both charts:** the index book's King full, width 3 (SPXW · NDX); the ETF
book's King the lighter pair, width 2 (SPY · **QQQ — the one visible change to a line he had; offered, agreed**).
**The 7635 put wall — answered, not changed:** PW0 is the 0DTE wall (SPX 7630 that hour; to-Friday 7630; all-expiry 7500 —
the number their header shows) at Skylit's SPXW ratio, 7630 × 1.0006963 = 7635.25 on the tick. Not an error.
**Probes:** `__gptsDebug.irt()` → `preview` (the header + 8 lines), `last.lines` ("Kings + walls"), `last.esKept`
("SPXW KING, SPY KING, CW0, PW0"), `last.ndxWhy` ("live · NDXP 29330 at 29355.41 (front payload, N s old)" / "held Nm …"),
`last.nqIfWhy` ("0DTE from the QQQ chain (CW0 721, PW0 714, 9m old)"), `last.gWhy` ("on the panel, not in the file (Kings +
walls) — live (XG2 …)"). ⚠ NDX KING and the NQ1 prices need the NQ1 self-fetch, which runs only while the Atlas tab is
VISIBLE (`ensureFeeds` → `panelVisible()`); a tab left in the background holds the day's last (`held Nm`) — that is the
design (v14.74), not a fault.
**VERIFY AT THE OPEN (v15.92):** FlexLevelsExport.csv has 8 data lines (`__gptsDebug.irt().last.rows === 8`); on the NQ chart
`NDX KING` sits where Atlas draws the NDX book's 100% node and `QQQ KING` is the lighter yellow; `CW0` / `PW0` on both
charts (the NQ pair near QQQ 721 / 714 × 41.1); no XG / SG / G / FLIP0 / D line anywhere; ⚙ IRT shows `Lines` with Kings +
walls lit; clicking everything rewrites the file with 21-ish lines at once and Kings + walls brings it back to 8.
**NEXT, in order:** (a) verify at the open (v15.92 above; the v15.91 / v15.90 lists below still apply); (b) one real ✓ on Rec
(R-33) and his ✓ / ✗ on R-34 / R-35 / R-1 / R-2 / R-4 / R-5 / R-6; (c) **v15.93 THE TAP RECORD, element 1** — asked before
built (104 rows WAIT on it); (d) readers for the 15 READY rows (K4.1 / K4.2 first); (e) Q12 on ~09-15; (f) seasonality
(v15.94, after R-33).

# ⚠⚠ 2026-09-09 — v15.91: THE D ROWS BY THEIR BOOK, DEDUPED ("i'll go with your recommendation") · THE NDX BOOK ON HIS NQ CHART · THE NIGHTLY APPENDS FIRST

**Panel is v15.91, companion v1.19 (unchanged — ONE link).** v15.90 was reviewed LIVE on his panel (13:5x–14:4x CT): the
registry landed (184 · WAITING 111 · READY 15 …), his machine's nightly re-ran with the new code at 10:25 CT (H5 BLOCKED by
its join, H7 WITHDRAWN, feats in the log) — and two things were caught: **his sweep corpus read 284, not 290** (the
nightly's step order: the sweep tables ran before the append wrote the night files — fixed, pinned) and **the D rows
drew every level twice on FRONT** (Atlas's FRONT list IS the two books' own rows — verified 5 for 5 against the chart).
His decision: *"i'll go with your recommendation"* → the D rows are written only when no line within half a point is
already in the file, and each carries its BOOK, resolved from Skylit's own ratios (D-SPX KING · D-NDX2 · D-QQQ3; Dn when
no ratio resolves it). His question: *"i am seeing a derived king on the nq. what is it?"* → **the NDX options book's
King projected onto NQ** (29355.41 / 1.00087 = 29330 — NDXP and NDX share the ratio; Atlas's NQ1 chart is three books:
QQQ · NDXP · NDX); the file had never carried NDX; now it does, as D-NDX KING with its next nodes, beside QQQ KING and
G2–G5. The morning's "7658 — 100%" was a single-ruler list from a different window (the chart was not on FRONT then).
**VERIFY AT THE OPEN (v15.91):** on the NQ chart `D-NDX KING` at Atlas's 100% NDX label and NO D line on QQQ KING / G2–G5;
on the ES chart NO D line at all while the selector is FRONT (`irt().last.dWhy` says "every row of Atlas's list is
already on the chart"); `__gptsDebug.futDerRows('NQ1').front.rows[0]` → `src:'NDX', strike:29330-ish, lbl:'D-NDX KING'`;
after his machine's nightly the Data tab's sweep corpus reads 290+ (the order fix) and the Analysis ⟳ lines carry today.
**NEXT, in order:** (a) verify at the open (v15.91 above; the v15.90 list below still applies); (b) one real ✓ on Rec
(R-33) and his ✓ / ✗ on R-34 / R-35 / R-1 / R-2 / R-4 / R-5 / R-6; (c) **THE TAP RECORD, element 1** (v15.93 — v15.92 took
Kings + walls the same afternoon) — asked before built (104 rows WAIT on it); (d) readers for the 15 READY rows (K4.1 /
K4.2 first); (e) Q12 on ~09-15; (f) seasonality (v15.94, after R-33).

# ⚠⚠ 2026-09-09 — v15.90: THE LEARNING PROCESS, HARDENED ("yes, fix everything") + D-KING · D2..D5 IN THE IRT FILE ("we need to match atlas … FRONT")

**Panel is v15.90, companion v1.19 (unchanged — ONE link). v15.84 → v15.90 unverified at an open (the lists below).** He
installed v15.89 (the footer read v15.89 at 13:5x CT; the Data tab was on his panel) and then asked the question this
build answers: *"harden the learning process … did you integrate the data tab … make sure the entire learning process
makes sense … studies that are useless or pointless … is everything upto date"*. The audit is
**`session-state/AUDIT-2026-09-09.md`** — read it: nine findings, what was done for each, what is still open. His answer to
the audit: *"yes, fix evertyhing and make sure it all makes sense and is integrated and working together and aligned to
my objectives/purpose. make sure you fix any and all issues."* — the one whole-build mandate he has given; it covered
the cuts. Mid-build, on the ES1 chart's "7658 — 100%": *"why do i not see the 7658 in irt .. are you exporting it"* →
*"we need to match atlas. are you saying that the level was a derived level"* → *"FRONT"*.
**What v15.90 is:** (1) the sweep corpus APPENDS (the nights → `<day>-night.csv`; study-sweeps reads the vendor + every
CSV, the four-day gap rule; 290 sessions and one more a night; H7 had read n = 0 for thirteen sessions); (2) THE REGISTRY
IS THE MACHINE'S OUTPUT — every study carries `needs:{corpus, n}`; coverage.json counts every corpus (price 295 · nq 195 ·
sweeps 290 · book 13 · ledger 148 · kingroll 170 · gate 231 · tap 0 · vix 0 · calendar 0); results.py sets WAITING (have /
need / ETA) or READY (data on hand, no reader) on every row it has no number for, composes the scripted sentences from
SWEEPS.json nightly, reads dir.kingRoll from the day files (D3.3 60% n=170); **184 studies: WAITING 111 · READY 15 · READ
15 · THIN 12 · SHIPPED 13 · REGISTERED 10 · REFUSED 3 · NULL 2 · CUT 2 · OPEN 1**; the thirteen level-name studies are
H2.L (NULL — 2 of 36 names clear the control: chance); S1.4 / S7.3 CUT; H7 WITHDRAWN on the register (F-23); H5 judged by
its join (0 of 3 extremes coincide with a ledger tap — BLOCKED with its own n); (3) the docs say what runs (nine tabs,
the nightly's real steps, PROCESS §4 today, the roadmap 15.90 / 15.91 the tap record / 15.92 seasonality); (4) **D-KING ·
D2..D5** — the merged derived list Atlas draws on the futures chart, FRONT window, in the IRT file on EPU26 and ENQU26
(white 2 / grey 1, solid), held day-scoped, with `__gptsDebug.futDerRows('ES1')` to hold against his chart.
**VERIFY AT THE OPEN (v15.90, after the v15.88 / v15.89 lists):** `__gptsDebug.futDerRows('ES1')` — `front.rows[0].k` must
equal the "100%" label on his ES1 chart (7658 that morning) and D-KING on the IRT chart must sit on it; `irt().last.dWhy`
says "live (D-KING …)"; the Analysis tab's H2 shows H2.L NULL with the machine's table under it and no row says OPEN or
READ NEXT; Testing ② shows H7 WITHDRAWN and H5 BLOCKED "0 of N extremes … coincide"; the Data tab ④ has have · need · ETA
per corpus; after his machine's first nightly the Analysis tab's ⟳ lines carry "nightly 2026-09-09" and the sweep
corpus reads 291. **And one real ✓ on Rec (R-33)** — the path has never been exercised on his machine.
**Records this build:** CHANGELOG v15.90, LESSONS v15.90 (seven), DECISIONS 2026-09-09 v15.90, FINDINGS F-24, INVENTORY
§0x, DATA-ARCHITECTURE (the counts, the night files), DATA-ANALYSIS-PROCESS (nine tabs, rules 11–12), PROCESS (the
nightly, §4, §5), the plan / roadmap / architecture regenerated, AUDIT-2026-09-09.md, `.gex-config.json` 2026-09-09d.
**NEXT (as written after v15.90 — v15.91 followed the same afternoon; see the block above):** (a) verify at the open; (b) his ✓ / ✗ on **R-33 / R-34 / R-35** and R-1 / R-2 / R-4 / R-5 / R-6;
(c) **THE TAP RECORD, element 1 (v15.92 then; v15.93 since Kings + walls)** — the universe + the two controls per `design/TAP-RECORD.md` §2a, ASKED before
built (104 rows WAIT on it); (d) readers for the 15 READY rows — K4.1 / K4.2 first (the King's attraction: PURPOSE §3B);
(e) Q12 on ~09-15; (f) the seasonality mockup (v15.93 then, v15.94 since; after R-33).

# ⚠⚠ 2026-09-09 — v15.89: THE 🗄 DATA TAB, BEFORE ANALYSIS — WHAT WE HOLD · WHAT THE STUDIES NEED · WHAT IS MISSING (AND WHAT YAHOO CAN FILL) — MOCKUP B + THE RECOMMENDATIONS SECTION

**Panel is v15.89, companion v1.19 (unchanged — ONE link at install). v15.84 → v15.89 are unverified at an open — the
v15.88 checklist below still applies, plus: the 🗄 Data tab sits between Dashboard and 📊 Analysis; ① shows 19 cells
(08-17 → 09-08) with the tape row green on 09-07 and 09-08; ② Skylit's row counts today's bars; ③ localStorage ≈ 3.4 MB
/ 10 MB and IDB snaps ≈ 2,700 · feat ≈ 63,600 · defl ≈ 150 · tape ≈ 600 (his 09-09 numbers); ⑤'s "prior week not
fetched" row is GONE once `__gexif.futWeek()` has rows; ⑦ lists R-33 · R-34 · R-35 first.** Probe:
`__gptsDebug.showData(true)` · `__gptsDebug.coverage()` (asOf, days.length) · `__gptsDebug.dataIdb()`.
**His ask, whole:** *"i want you to build a data tab and place it before the analysis tab. Tell me what should the data
tab have? I am thinking it should give me a snapshot summary of the data we currently have, what we need for the
studies, recommendations and more. If there is missing data that we can obtain from yahoo, it should mention that. what
are your recommendations for this tab? can you provide mockups for it."* Two mockups from real numbers (A sources-first ·
B coverage-calendar-first, `mockups/mockup-data-tab-A/B.png`, `tools/mockup-data.py`); **his choice: "I like B, I also
want recommendations section. build"** — built as B with ⑦ RECOMMENDATIONS · data.
**What it is:** read-only, seven sections on the panel's chrome, each a question — ① COVERAGE (a cell per day file on
GitHub, six signal rows: day file · ES night · NQ night · CSV complete · tape · nightly log; the hover has the counts) ·
② THE SOURCES live (Skylit · IF · Yahoo ×5 · GitHub raw · the nightly — what, how much, how fresh) · ③ THE RECORD ON THIS
MACHINE (localStorage against its quota with the bar; IDB snaps · feat · defl · tape counted live) · ④ WHAT THE STUDIES
ARE WAITING FOR (the registry by corpus — the tap record 104 · the price corpus 41 · the book 43 · the Learn gauge · the
King roll · the ledger; have vs need) · ⑤ THE GAPS and what Yahoo can fill (computed rows that disappear as stores fill;
**DAILY bars · HOURLY bars · ^VIX1D in green = obtainable from Yahoo → R-33 · R-34 · R-35**; the chains' history, QQQ's
node dollars, bid/ask volume in grey = Skylit / IF / IRT only) · ⑥ THE PIPELINE'S CLOCK today · ⑦ RECOMMENDATIONS · data
(the DATA rows of Rec, proposals first, title + one sentence; his ✓ / ✗ stay on Rec). **The repo side is counted by the
nightly** — `tools/nightly/coverage.py` → `learning/coverage.json` after the futures step; the panel fetches ONE file
(`gpts_coverage_v1`, with the 10-minute pipeline check and at boot) and reads the live half itself. DATA-ARCHITECTURE §6d.
**R-33 · R-34 · R-35 (DATA, proposed, by the review) — nothing is fetched until his ✓:** a daily-bar courier (ES=F / NQ=F
1-day, the full history — his seasonality charts, the weekday / 10-week ranges); an hourly backfill (730 days, once — the
HOD / LOD clock on ~500 sessions instead of 295); ^VIX1D daily in the VIX courier (the EM band backtestable).
**Found while building:** (a) the tab's first render was BARE — the mockup's styles live under `.g3pan` (the Rec / Items
chrome), not the Analysis chrome; the headless screenshot caught it (LESSONS v15.89: a tab is rendered and looked at
before it is called built). (b) **origin moved while I built**: his machine's 07:55 nightly (after the v15.88 install)
rewrote BASERATES · SWEEPS · the examples · the studies · the 09-08 log and put a SECOND machine row on Rec —
**RN-hour.2-held: H2 09:30–10:30 held 23 of 30 = 77% (Wilson low 59%) vs 44%** — one session's classes; register it out of
sample, do not quote it as a rate. Merged: origin's Rec file + `rec-seed.py` + `splice-seed.py`, then `--keep-mine`.
(c) **the guard could not see files his machine ADDED** (the 09-07 day file, `data/tape/2026-09-07` + `09-08`, the 09-07
log — on origin for two days, absent from the cloud's tree): `origin-guard.py` now fetches his machine's additions before
the check (selftest). The cloud's coverage.json: **19 day files · ES 295 / NQ 195 · sweeps 284 · book 13 · 195 studies ·
tape 09-07, 09-08**; his machine's next nightly re-counts it.
**NEXT, in order (as written after v15.89; v15.90 followed the same day — see the block above):** (a) verify at the open (v15.84–89); (b) his ✓ / ✗ on **R-33 / R-34 / R-35** (the Yahoo fills) and R-1 /
R-2 / R-4 / R-5 / R-6 on Rec; (c) **v15.90 THE TAP RECORD, element 1** — the universe (SPX KING / XG2–5, SPY KING / SG2–5 on
ES; QQQ KING / G2–5 on NQ) + the two controls, per `design/TAP-RECORD.md` §2a, ASKED before built; (d) Q12 on ~09-15;
(e) the seasonality mockup (v15.91, after R-33); (f) RN-hour.2-held — the register's out-of-sample count, not a build.

# ⚠⚠ 2026-09-09 — v15.88 + COMPANION v1.19: THE AGENDA, BATCHED — AHI · ALO · LHI · LLO · THE KING FOLLOWS SKYLIT · NQ'S NIGHT + THE WEEKLY LEVELS · THE NQ CORPUS · THE HOUR ON EVERY DEFLECTION · ⓪a PER MARKET · F-23

**Panel is v15.88, companion v1.19 (BOTH links at install). v15.84 → v15.88 are unverified at an open — the checklist is
below.** His method today: **"1 issue at a time"**, then **"lets discuss other things also to put more issues in the
build"** — each still one message, one decision. The agenda (his "lets go over all the issues step by step, one by one")
is DONE except what waits by design: **the tap record is the NEXT build (v15.89, his "your recommendation")**; Q12 (top 3
vs 5) reports ~09-15 (a reminder fires in this cloud session); the seasonality charts wait for a mockup (v15.90); R-1 /
R-2 / R-4 / R-5 / R-6 sit on the Rec tab for his ✓ / ✗. **Item 4 closed without a build:** *"there should be no qqq lines
in es. for NQ, just use G because we dont have to distinguish it like for the es which has both spy and spx."*
**His decisions, verbatim, in this build:** *"just use whatever is standard"* → Asia 17:00–02:00 CT, London 02:00–08:30
CT (AHI / ALO / LHI / LLO, tier 1; LDNH / LDNL renamed) · *"match skylit"* → the King latch OFF, the rail and the export
follow Skylit's crown on the same tick · *"yes"* → companion v1.19: NQ's full night, the weekly 5-minute bars (PWH / PWL /
WPOC drawn for the first time), NQ's base rates couriered · *"i'll go with your recommendation"* → the NQ vendor corpus
parsed (NQ BASERATES 195 = 188 + Yahoo; four shared days identical to the tick) · *"i agree i think you should also tag
other hours"* → the five uncircled first-hour turns are legs (E005 m1–m4, E006 m1, ⚐), every leg carries its hour, R-3
implemented (hour:1–7 · clock:first / mid / last in both twins) · *"your recommendation"* ×2 → the tap record next, no
patch tier · *"this build"* → ⓪a per market, after my correction (there was no NQ E row; the section read ES on every
chart — now the NQ chart reads NQ, with NO baked NQ literal: "no base for this market yet" until the courier delivers).
**F-23 (found, not decided):** the sweep corpus's ONH / ONL included the session's own post-close bars — a look-ahead
that removed failed sweeps. Corrected: ONL 22% n=125 · ONH 16% n=154 (were 29% / 26%); F-14's first-30-minutes edge is
gone (21% n=249 vs 18%); the flush (35% n=94 vs 23%) and the slow reclaim (32% n=96 vs 23%) stand at ~+10pp. Quote the file's numbers with n.
**Numbers withdrawn this build:** ONL 29% / ONH 26%; "the first 30 minutes matter 27% vs 18% n=180"; the flush 40% n=86;
the slow reclaim 40% n=90; the shallow poke 86% n=228. And a bug fixed on the way: `measureBarsRaw` sorted its day keys
lexically (the v15.79 bug's twin) — from the 10th of a month the A row would have stood on the 8th's bars.
**Found on GitHub while building (v15.87's first hours on his machine):** the tick re-ran the nightly every ten minutes
(the one-second fence read the run's own BASERATES as older than the log) — `STALE_S` an hour now; and the installer does
not carry `data/futures/` so his corpus rebuilt from four day files (289 sessions) — every day file now (295 after this
install's first run). His machine's 21:35 outputs are merged into this build's seeds (a first machine row on Rec, RN-teach-L5).
**VERIFY AT THE OPEN (v15.84–88):** footer v15.88, companion 1.19 (`__gexif.futWeek()` returns ES / NQ rows; `hlBaseNq()`
195 sessions after its first fetch); the candle's labels wear AHI / ALO / LHI / LLO and PWH / PWL / WPOC when swept; the
export's SPXW KING moves with Skylit's crown (no lag); on the NQ chart the ⓪a header reads *NQ 1-min*, the A row NQ's
numbers in NQ dollars, the E row NQ's after the courier; the Learn tab's hour column and the ⚐ legs; the Analysis H2 table
has ALO / AHI / LLO / LHI / WPOC rows, ONL 22%; `LASTFEED.SPY.j.expirations.length` = 1 and ~120 rows; `feedRejects().SPY.win`
climbing; the A row's open = the 08:27 minute's open (v15.87); the Analysis tab's asOf 09-08 (the nightly's re-run).
**OPEN THREAD (2026-09-09, after v15.88) — RESOLVED BY v15.89 (B, with ⑦; see the block above): THE 🗄 DATA TAB, before Analysis — his ask:** *"i want you to build a data tab
and place it before the analysis tab. Tell me what should the data tab have? I am thinking it should give me a snapshot
summary of the data we currently have, what we need for the studies, recommendations and more. If there is missing data
that we can obtain from yahoo, it should mention that. what are your recommendations for this tab? can you provide
mockups for it."* Two mockups delivered (`mockups/mockup-data-tab-A.png` sources-first · `-B.png` coverage-calendar-first;
`design/mockup-data-tab.html`; generator `tools/mockup-data.py`, every number from the repo and his browser's stores).
Proposed sections: ① the sources live (Skylit · IF · Yahoo ×5 · GitHub · the nightly; coverage · freshness · what each
feeds) · ② the record on this machine (localStorage 3.4 MB / 10 MB, IndexedDB snaps 2,699 · feat 63,597 · defl 149 · tape
610) · ③ the record on GitHub (coverage per session: day file · snaps · node events · ES / NQ bars · corpus CSV · tape ·
nightly) · ④ what the studies are waiting for (by corpus, with have / need: the tap record 85 studies · the price corpus
41 · the book 43 · blind reads 0 of 5 · kingRoll 0 of 30 · H6 2 of 40) · ⑤ the gaps and what Yahoo can fill (green =
obtainable: DAILY bars years back for his seasonality charts — RECOMMENDED; HOURLY bars 2 years for the HOD/LOD clock on
~500 sessions — RECOMMENDED; ^VIX1D optional; grey = Skylit / IF / IRT only) · ⑥ the pipeline's clock. Read-only, no
claims. **Awaiting: A or B; then the two Yahoo recommendations (daily, hourly) yes / no; then build (v15.89 becomes the
Data tab or the tap record — his order).**
**NEXT, in order (as written after v15.88; the Data tab took v15.89 — the tap record is v15.90):** (a) verify at the open; (b) **THE TAP RECORD, element 1** — the universe (SPX KING / XG2–5, SPY
KING / SG2–5 on ES; QQQ KING / G2–5 on NQ) + the two controls, per `design/TAP-RECORD.md` §2a, asked before built; (c) Q12
on ~09-15; (d) the seasonality mockup; (e) his ✓ / ✗ on R-1 … R-6; (f) the King latch is gone — if a flap day ever hurts
his chart, one number brings it back.

# ⚠⚠ 2026-09-09, small hours — v15.87: THE TOOL GRID ("tools") · THE CORPUS APPENDS ITSELF ("yes") · THE NIGHTLY RE-RUNS AFTER AN INSTALL — AND THE CLOBBER, TOLD

**Panel is v15.87, companion v1.18. Delivered after midnight (installv1587.bat); v15.84 → v15.87 are ALL unverified on
his panel at an open — verify at 08:30 CT 2026-09-09 (list below).** We were going through his agenda "one by one
including open items so we resolve everything" — 14 items; items 1 and 2 are resolved by this build; item 3 is next.
**Item 1, the wick (his words, three rounds):** *"how is there a wick when the hod occurred at 8:30 … if the hod was at
8:30, then it never went above the open, so how can there be a wick?"* → *"lookup the days candle using yahoo or any
online resource"* → *"are you saying that the 14% of the session was done in 1 minute, creating a wick"* → parked → then
his screenshots: *"i checked with other souce and this is what it shows. see pics. You must be doing something wrong or
there is a disconnect. figure it out."* His tool's row for 09-08: HOD 8:33 · Took 3 · BOP 3 · Wick 6 · W.End 8:36 ·
Wick% 6 · MUD 6h24 · LOD 3:00pm · HL Rng 45.5 · MD $2,138. **The disconnect was the OPEN**: his tool's 3-minute bars are
stamped by END, so the bar labelled 8:30 is 08:27–08:30 and opens at **7715.00**; the panel took the 08:30 minute's
7711.50. Same bars: 14% on one grid, 6% on his. Asked which — **"tools"**. Built: `hlToolBars` folds the minutes into
his grid before `hodLod` / `gdActual`; the study folds the same way; the A row's hover states the grid; his row
reproduces exactly (`test_v1587` §2). **Item 2, the corpus (his 2026-09-08 question):** *"are you also saving the daily
stats from yahoo … do you have an indicator below … When will learning occur."* The append existed and nothing called
it → **"yes"** → the nightly now runs append-futures → study-hodlod → BASERATES (ES 295 sessions = the vendor's 284 +
the Yahoo days with provenance; NQ 11, Yahoo only) after the sweeps; `tools/bake-hodlod.py` re-bakes `HODLOD_BASE`
from the file at build time; **the indicator is `BASERATES.corpus.last`** (09-08 now; 09-09 after tomorrow's nightly).
**THE CLOBBER, and a wrong statement of mine:** his machine's nightly ran 09-08 at 15:05; the 15:21 / 17:11 / 18:01
installers (v15.84–86, built from a clone that predated the run) pasted the **09-07** results / studies /
recommendations / examples back over it, on his disk and on GitHub. I had told him the nightly's outputs "rode up
with your push" — they did not. Fixed at the only time that matters: `tick.py` re-runs the nightly when an output is
older than the log (the installer's mtime 0) or `results.asOf` < the log's day. **This build ships the 09-07 outputs
as GitHub holds them; his machine re-runs 09-08 within 10 minutes of the install** (asOf 09-07 < log 09-08) — check
the Analysis tab's asOf. Also fixed: Yahoo's live-quote row (14:49:41, volume 0) dropped by the harvest; the 09-08
tail (14:44–14:59) was read from the live companion store (`data/futures/2026-09-08-tail.json`).
**Numbers withdrawn by the grid:** the E row's took 33.5 m / rng 61.4 pts (now 34.7 / 60.6, 295 sessions); Fridays
~19 m → ~20 m, Tuesdays ~46 → ~48; the A row's 14% for 09-08 → 6%. The ladder is unchanged (42/56/67/76/84).
**OWED FROM HIM:** the first-hour turns he did not circle (E005's boundary); the Asia / London hours (17:00–02:00 /
02:00–08:30 CT proposed). **ANSWERED 2026-09-09 (agenda item 4, no build):** the NQ labels stay **G2–G5** — *"for NQ,
just use G because we dont have to distinguish it like for the es which has both spy and spx"*; no QQQ line on ES (as built). **OWED TO HIM:** Q12 on ~09-15 (reminder set); the
NQ vendor corpus parser (`NQ TestingData.txt`, tab / ISO — NQ's BASERATES is Yahoo-only until then).
**VERIFY AT THE OPEN (v15.84–87):** footer v15.87; the A row's open = the 08:27 minute's open and its hover says
*Grid: tool*; `__gptsDebug.hodLod().grid === 'tool'`; `LASTFEED.SPY.j.expirations.length` = 1 and ~120 rows, steady;
`feedRejects().SPY.win` climbing; the IRT file's XG / SG labels; the Learn tab's E005 / E006; the Analysis tab's
asOf 09-08 after the re-run; `hodlodBase().n` 295 (296 after the 09-09 nightly).
**THE AGENDA, remaining, one at a time (his "lets go over all the issues step by step"):** 3 the patch-build tier
(PARKED 2026-09-09 — "next issue"; I0909patch on the Open Items tab) · 4 DONE (G stays; no QQQ on ES) · 5 the Asia / London hours → then AHI / ALO /
LHI / LLO (LOCKED; the NQ courier has no overnight bars) · 6 the first-hour turns (E005's boundary) · 7 the tap record
element 1 · 8 R-1…R-6 · 9 the King latch · 10 WH / WL / weekly POC (companion v1.19) · 11 Q12 (~09-15) · 12 done
(the Architecture line for v15.84 is in) · 13 verify at the open · 14 LDNL/LDNH. The seasonality charts are v15.88.

# ⚠⚠ 2026-09-08, late night — v15.86: HIS TRAINING — TEN CIRCLES (E005 ES · E006 NQ), THE LIQUIDITY LEVELS BESIDE THE GAMMA LEVELS, L10 · L11 · L12; Q12 OPEN (top 3 vs 5, due ~09-15); AHI · ALO · LHI · LLO LOCKED

**Panel is v15.86, companion v1.18. His panel read v15.85 at 20:5x CT (the screenshot's footer); v15.86 delivered after
that (installv1586.bat) — the Learn tab's two new sections are the only face change.** His words, in order: **"tell me
simply what the recommendation is"** (→ keep 5) · **"ok.. i need you to record and let me know in about a week from
now.. keep it as an open item"** (→ OPEN-QUESTIONS Q12; a reminder fires in this cloud session 2026-09-15 08:30 CT) ·
**"next, i need to continue training you .. here are 6 deflections identified by the circles. each should be counted as
1 deflection. you should analyze the time and price and gamma node with all its details. in the future, you should be
able to identify them like i am identifying them, and score them to predict the deflection in advance. see also if
there were any type of patterns as well as levels there like ONH ONL, PDH, PDL, Asia High Asia Low, London High and
London Lo, prior day poc, vah, val. i was not tracking sweeps of the asia and london highs and low, but we should do
it. they can be ALO, AHI, LLO, LHI. you will find that the liquidity levels and gamma levels may increase the
probability of deflection, so you must look at both and track both … first you must get really good at
identification of deflections and levels and both as well as patterns like rug, rrug, piku stack, barney stack etc..
only after you put all of this together over time will you start giving me valid predictions. Especially after adding
hod lod statistics as well as things like volume. so make sure you track, score over time to get to this level of
competence and reach my intended purpose of the project, which you should always keep in mind."** · **"here are 4
examples from the NQ to also help you learn"**.
**What the ten circles were (E005 / E006 in `learning/deflections/`, the numbers there):** ES — the +γ SPX King 7700 as
the ceiling ×3 (10:18 · 12:06 · 13:18; lower highs, rejections 13.75 → 8.5 → 12; the King $88M → $197M then −$100M/15m
after the third tap); the −γ King 7680 gone THROUGH twice with the turn at the +γ node beneath (13:48 at ONL/LLO
7687.50 → 7670 / SPY 765; 14:59 the LOD 7672.25 on the fresh +γ stack 7665 / 7670, +$47M/15m); 14:33 the new +γ King
7690 over the −γ 7685 / 7680 (a rug) on the SMA-50. NQ — the −γ 718 as the lid on the opening drive; the −γ Kings
719 / 718 as floors at the PDC 29569 and the prior POC 29512, overshot 16 / 28 pts and reclaimed; the +γ 720 pair as
the afternoon ceiling. **Rules:** L10 (a −γ King is not the floor; the overshoot to the +γ node), L11 (a +γ King ceiling
decays by the tap, gone when the King bleeds), L12 (a liquidity level at a −γ King gets swept before the turn) — all
PROPOSED, one day; L1 n=24 CONFIRMED; the `liq` factor. **The panel's own `defl` ledger matched none of his six** (21
rows of ±0.50 SPY wobbles) — the tap record (R-25) is the fix, and §4b of its design now says what the circles taught
it (the overshoot, the sweep depth, the King's $ after the tap; QQQ needs dollars).
**THE QUESTION ASKED (one):** the first-hour turns he did NOT circle — ES 08:45 (7701), 09:03 (7680 at the ONL/LLO
sweep with the 7675 −98% King), 09:39 (7685), 09:51 — and the NQ LOD 09:09 at the PDL sweep: skipped on purpose (the
book still forming; the opening excursion) or are they deflections too? His answer sets the boundary of the corpus.
**OWED FROM HIM:** that answer; the NQ labels (G / QG); the Asia / London boundaries (proposed 17:00–02:00 / 02:00–08:30
CT). **OWED TO HIM:** Q12 on ~09-15 (the per-rank table over a week); the tap record's element 1 when he says go.
**VERIFY AT THE OPEN:** footer v15.86; `LASTFEED.SPY.j.expirations.length` = 1 and ~120 rows, steady; `feedRejects().SPY.win`
climbing; the snaps' node count no longer flipping bands; the IRT file's XG / SG labels; the Learn tab's E005 / E006.
**NEXT, in order, each its own ask:** (a) his answers above; (b) the tap record, element 1 (design §2a); (c) verify live;
(d) AHI / ALO / LHI / LLO on the SWEPT line and the candle (LOCKED — after his word on the hours; the NQ courier
needs overnight bars); (e) WH / WL / weekly POC (companion v1.19); (f) v15.87 SEASONALITY TRACKED (mockup first);
(g) R-1…R-6; (h) the King latch; (i) LDNL/LDNH; (j) the projection as a scored read — only if he asks.

# ⚠⚠ 2026-09-08, night — v15.85: XG2–XG5 · SG2–SG5 (R-26) · THE TAP RECORD DESIGNED (R-25, paper) · v15.84 + v15.85 BOTH UNVERIFIED UNTIL THE OPEN

**Panel is v15.85, companion v1.18. Delivered (installv1585.bat) after the close; v15.84 (F-22) was delivered forty
minutes earlier and he may install either — v15.85 contains v15.84.** His words tonight, in order: **"double check"**
(→ F-22, v15.84, above) · **"are you running a study on the gamma levels deflection …"** / **"the purpose of the study is
to identify what makes gamma levels work as deflectors of price for the purpose of trading reversals from levels"** ·
**"yes"** (to the design) · the A-row question (*"no wick … but the wick % is 14%. How is that possible"* — answered from
the raw ES bars: the HOD was the 08:30 bar's high, 6.25 pts above the open = 14% of the 45.5-pt range; W.End 8:31,
BOP 1m, wick 1m; wick% is a PRICE ratio, his 08-28 definition; the row and the candle are right) · **"the spy gama
level cand be SG2 SG3 etc., and the spx levels can be XG2 and XG3 etc.. make this change. also note the qqq king
should only be on the nq not the es"** (→ v15.85; the QQQ King has been NQ-only since v15.80, read on his live file
and pinned again) · **"is it sufficient to have the top 3 instead of the top 5. can you look at the deflections today"**
(→ answered from a hand read, below).
**What shipped (v15.85, R-26).** XG2–XG5 (SPX book), SG2–SG5 (SPY book); the signed % as before; Kings and colours
unchanged; `irtGHeld` accepts XG / SG (ES) and G (NQ) — an old S2 hold is dropped, the next book re-latches; the gear
line says `SG rows`. `test_v1585.js` 16 · 6/6 mutants; `test_v1583`, `test_v1576`, `test_irt_export` §4t, `test_em_band`,
`test_v1581` re-pinned. **OPEN WORD (asked): the NQ symbol's QQQ lines — `G2–G5` as today, or `QG2–QG5`.**
**The tap record — R-25, designed, NOTHING BUILT: `design/TAP-RECORD.md`.** His question is the registry's: 110 of 193
studies name `corpus: tap record` and it does not exist. The design: one row per tap of an EXPORTED line (book · rank ·
signed % at Skylit's price) plus two controls (`rest` = the other 0DTE nodes ≥ 20%; `mid` = the midpoint between
adjacent lines, the doctrine's "imaginary support"), the tap on ES 3-minute bars (±5 ES / ±20 NQ, from outside, tapNo),
the conditions at the tap (lifecycle, growth into the tap and day-over-day, King distance, gatekeepers, the pocket
behind, the other book's line within 5, his key levels, approach speed and flush, regime, trinity, King roll, the
clock, before / after the first extreme), the outcome in trading terms (broke / mfe / mae / reach / R; DEFLECT = not
broke and ≥ 10 pts away within 20 bars · PIN · BREAK; wasExtreme; resumed), an IDB store `taps` mirrored into the day
file, the nightly's table by book × rank and by condition beside the control, H11 pre-registered (fresh + growing >
tested or shedding > midpoint). **Built one element at a time on his word, in the order §5 lists: the universe first.**
The clean sample begins 2026-09-09 (F-22); the King rows could be back-filled from the archive (window-proof), the
G / S rows cannot.
**The top-3 question — one day, a hand read, not a verdict** (in the page: `snaps[].tri.SPXW.top` / `tri.SPY.top` —
the ladder's own ranked lists, window-proof — at Skylit's price via `xm.SPXW.px` / `xm.SPY.px`, against the ES 3-minute
bars, the design's definitions): **34 taps; ranks 1–3: 22 taps, 15 DEFLECT, 7 PIN, 0 BREAK; ranks 4–5: 12 taps, 3
DEFLECT, 6 PIN, 3 BREAK (all three).** The HOD (7717.75, 08:30) = the two Kings (SPY 770 at 7718.5, SPX 7720); the LOD
(7672.25, 14:59) = SPY 765 at 7671 (rank 3 at 14:51, 2 at 14:57) with SPX 7665 (rank 5) at the same price. Two turns
needed a rank-4/5 line (09:51 SPY 767 r5 +47% 12.7 pts; 13:48 SPY 765 r4 +24% 18.7 pts). Ranks move all day (SPY 765:
3 → 1 → 5 → 4 → 3). Told him: keep 5 for two weeks while the record accumulates, then decide with n. ⚠ The SPX ratio
drifted 1.00044 → 1.00093 today (+3.4 → +7.2 pts); a flat 1.0007 mis-zoned two taps — use the per-snapshot `xm` price.
**VERIFY AT THE OPEN (both builds, through his tab):** footer v15.85; `__gptsDebug.LASTFEED.SPY.j.expirations.length`
= 1 and ~120 rows, steady across three reads a minute apart; `feedRejects().SPY.win` climbing; today's snaps'
`nodes.length` no longer flipping bands; the IRT file: XG / SG labels, the SPY rows against Skylit's SPY orbs at the
same second; the QQQ King on ENQU26 only.
**NEXT, in order, each its own ask:** (a) his answer on the NQ labels (G / QG); (b) the tap record, element 1 — the
universe and the two controls (design §2a); (c) verify v15.84/85 live; (d) WH / WL / weekly POC (companion v1.19);
(e) v15.86 SEASONALITY TRACKED (mockup first); (f) R-1…R-6 await his ✓/✗; (g) the King latch under "match Skylit
always"; (h) LDNL/LDNH; (i) the projection's target / regime as a scored read — only if he asks. **Tonight:** the day
file's `featSource` (from 08:39) and tomorrow's nightly log are the last proof of v15.82; tomorrow's file is the first
with a one-window SPY book.

# ⚠⚠ 2026-09-08, evening — v15.84: THE PANEL'S OWN SPY / QQQ BOOK IS ONE WINDOW AND ONE BREADTH — THE 0DTE CHAIN, EVERY STRIKE (F-22). NOT YET VERIFIED ON HIS PANEL.

**Panel is v15.84, companion v1.18 (unchanged). Built and delivered (installv1584.bat) after the close; the first
verification is tomorrow's session — see the probe below.** His word: **"double check"** (the IRT file against Skylit
at the same second, after "check" at 14:18). The SPX side was exact to the tick (SPXW KING 7687.00 = 7680 −100 · G2
7681.75 +86 · G3 +50 · G4 +43 · G5 7697.00 +32). The SPY side printed **SPY KING 760 at 15:21 and 766 at 15:23**, S rows
whose signs and sizes did not match Skylit's SPY book (766 at −$49M in the panel's copy, +$849M on Skylit's 0DTE orb).
**Cause, F-22, two halves:** the panel's own SPY / QQQ book (`selfFetch`) copies the LAST gex/levels URL seen and kept
its `exp_mode` / `exp_count` / `nodes` — (1) the app's, which on an ES chart with the projection on rotates THREE
windows (`nodes=5 current/1` · `nodes=5 next_n/2` · `nodes=60 next_n/4`); (2) the panel's OWN, because every request it
makes goes through the `window.fetch` it hooked, so the expiry sets (`nodes=500` in the 0DTE / week / next_n-6 windows)
both reset that URL and hand their responses to `onFeed`. Measured: `LASTFEED.SPY` at 15:38 CT = 3 expirations, FIVE
rows; the recorded SPY node count per bar from 13:45: 30 · 3 · 12 · 4 · 5 · 4 · 11 · 12 · 5 · 4 · 11 · 4 · 28 · 5 · 13;
the IDB snaps: the same ≤6 / 26+ mix, **34–61 band flips a session, every session since 08-26** (FINDINGS F-22 has the
table). **The King series survive** (the largest 0DTE node leads every window: 767 → 766 at 14:51 today); **every
node-level read, ledger and feature record before v15.84 was taken on a shifting universe** — F-21 (acm 15% vs dec 15%)
is flagged "measured on a mixed-window book", the node grade likewise. I had told him "the day's record is fine" on the
strength of the steady King — withdrawn in LESSONS v15.84.
**What shipped.** `selfFetch` pins `exp_mode=current&exp_count=1&nodes=500` (replaced or appended) for gamma and
vanna; `onFeed` refuses a multi-expiration gamma payload once a single-expiration book is held —
`FEED_REJECTS[sym].win` / `winT`, `__gptsDebug.feedRejects()`, logged once and every 50th time; nothing held → accepted
(never blind at boot); no `expirations` field on either side → the guard stands down. The never-history-over-live guard
is unchanged behind it. No face change. `test_v1584.js` 28 · 13/13 mutants. Roadmap: v15.83 shipped and verified
(14:18 CT), **v15.84 this build**, seasonality → v15.85, the rest +1. DECISIONS 2026-09-08 (v15.84), LESSONS v15.84,
INVENTORY §0r, SKYLIT-FEEDS § The endpoint, config 2026-09-08i.
**VERIFY TOMORROW (first thing, through his tab):** `__gptsDebug.LASTFEED.SPY.j.expirations.length` = 1 and
`levels[last].l.length` ≈ 120 and STAYING so across three reads a minute apart; `__gptsDebug.feedRejects().SPY.win`
climbing (every week / wk7 set refused); `__gptsDebug.dumpRecorder()` → today's SPY snaps' `nodes.length` no longer
flipping bands; the IRT file's SPY KING and S rows against Skylit's SPY orbs at the same second. If the row count still
flips, the app is requesting SPY itself (a SPY chart) — the recorded limitation, not a regression.
**Not fixed, recorded (DECISIONS):** on a SPY / QQQ chart the app's own five-row payload still arrives through the hook;
whether the week / wk7 sets should feed anything now that only the expiry profile reads them.
**HIS STUDY QUESTION (asked 15:3x CT, answered, his "yes" to the design):** *"are you running a study on the gamma
levels deflection. basically testing spx and spy and qqq top 5 gamma levels and their ability to deflect and if they do
deflect, under what conditions and if they dont deflect, under what conditions."* — *"the purpose of the study is to
identify what makes gamma levels work as deflectors of price for the purpose of trading reversals from levels."*
Answered honestly: the node ledger / the deflection ledger / the L-rules exist and are scored nightly, but nothing tags
a level by BOOK and RANK (SPX KING / G2–G5, SPY KING / S2–S5, QQQ on NQ — the lines he trades from), the outcome is
"continued 0.3 within 10 bars" not a trader's reversal, the QQQ book on NQ is not scored, and until today every sample
was afternoon-only (F-20) and — as of tonight — mixed-window (F-22). Proposed **R-25 · THE LEVEL STUDY**: one row per
touch of an EXPORTED level (the 13 ES + 5 NQ lines), stamped book · rank · signed % · the conditions at the touch
(fresh / tested, growth, King distance, time of day, regime, before / after the first extreme, day colour, confluence
with the other book within N points), scored in trading terms (reversal points before violation, violation points,
held / broke, minutes to the reversal extreme), nightly, on Analysis as "which levels deflect, under what conditions",
with n. He said **"yes"** to writing the design — **THE DESIGN IS THE NEXT DELIVERABLE, on paper, one element at a time,
before anything is built; the clean sample starts 2026-09-09 (F-22).**
**NEXT, in order, each its own ask:** (a) the R-25 design (his "yes"); (b) verify v15.84 live at the open; (c) WH / WL
/ weekly POC (companion v1.19, two weeks); (d) v15.85 SEASONALITY TRACKED (mockup first); (e) R-1…R-6 await his ✓/✗;
(f) the King latch under "match Skylit always"; (g) LDNL/LDNH; (h) the projection's target / regime as a scored read —
only if he asks. **Tonight:** the day file's `featSource` (from 08:39) and tomorrow's nightly log are the last proof of
v15.82; tomorrow's file is the first with a one-window SPY book.

# ⚠⚠ 2026-09-08, early afternoon — v15.83: THE SPY BOOK'S TOP FIVE IN THE IRT EXPORT · THE SIGNED % ON EVERY NODE LINE · THE NODE LINES WEAR THEIR POLARITY (R-24).

**Panel is v15.83, companion v1.18 (unchanged). INSTALLED AND VERIFIED on his panel 14:18 CT 2026-09-08:** footer
v15.83, 18 rows, 13 of 13 ES rows on Skylit's prices — SPXW KING 7695.50 gold w3 · G2 -100% 7685.50 purple (the crown
flapping: 7680 at 100% while the 2-minute latch holds 7690 — the open latch decision, seen live) · G3 +61% yellow ·
G4 -46% purple · G5 +30% yellow · SPY KING 7690.75 light yellow w2 · S2 +48% 768 · S3 +28% 766 · S4 +26% 769 · S5 +18%
765 (all palest yellow — no negative SPY node yet to show the palest purple). v15.82 was verified at 12:08 CT (below).** His words, from an IRT screenshot with no
7690 line: *"do my levels in irt match the levels in skylit. for example, i dont see a 7690 level"* → the SPX side
matched to the tick; the 7690 was **SPY 767**, a SPY-book node the file never carried (R-13 was the SPX top-5) →
*"that spy node is important because it is a big node.. i currently have top 5 nodes for spx. Im thinking of having the
top 5 for spy as well .. what is your suggestion?"* → my suggestion was "export what Skylit draws" (P20 then) → he set
NODES = 5 → *"is that showing me the top 5 from both spx and spy"* (yes: `nodes=5` per borrowed book, read off his tab)
→ **"ok lets go with this. the spy gamma node lines can be beige"** → **"also add the % to each of the node lines
except the king"** → **"add a + or - too so i know the polarity"** → **"infact, i want you to redo the color schemes
for the nodes, using yellow and its derivatives for positive nodes and purple and its derivatives for negative gamma
nodes."**
**What shipped.** **S2–S5** — the SPY book's next four by |v| after its King (`irtSpyTop`, from the same payload the SPY
KING row reads; no strength floor: NODES = 5 draws the top five whatever their size), width 1, solid, Skylit's ES1
price, held day-scoped under `GS`, `IRT_LAST.sWhy`, on the ETF symbol at the SPY strikes. **The signed %** on every node
line (`irtPctTag`: `G3 -56%`, `S2 +55%`, the NQ symbol's `G2 +44%`); the Kings bare. **The polarity scheme**
(`irtNodeCol`, `IRT_COLORS.gpos 245,215,110 · gneg 185,150,250 · spos 250,238,190 · sneg 225,208,252`): yellow
family = +gamma, purple family = −gamma, the shade = the book (SPXW / QQQ nodes one step lighter than their King, the
SPY nodes the palest); white is gone from the node rows; beige (his first word) superseded within the hour — do not
bring it back; the IF rows keep his 08-28 colours. `irtGHeld` accepts S labels; the gear's IRT line adds `S rows …`;
the helpers are typeof-guarded inside `irtBuildCsv`. Today's file would read: SPXW KING 7705.50 · G2 +77% 7710.50 ·
G3 -44% 7685.50 · G4 +29% 7700.50 · G5 -21% 7680.50 · SPY KING 7690.50 (767) · S2..S5 at 768 / 769 / 770 / … ·
CW0 · PW0 · FLIP0 — 13 ES rows. `test_v1583.js` 38 · 16/16 mutants; `test_v1576` 1g/1m re-pinned; the roadmap —
v15.82 shipped, **v15.83 this build**, seasonality → v15.84.
**Verified 14:18 CT (above) from the panel's file; his eyes on the IRT chart are the last word on the shades. Not
asked: the strike in the label (`S2 767 +55%`) — his call if he wants it.
**NEXT, in order, each its own ask:** (a) WH / WL / weekly POC — the companion's window must reach the prior week
(v1.19: two weeks), then PWH/PWL/WPOC draw by name; (b) v15.84 SEASONALITY TRACKED (mockup first; the corpus append into
the nightly — it ends 08-21); (c) the Rec proposals R-1…R-6 await his ✓/✗; (d) the King latch under "match Skylit
always" (his call); (e) LDNL/LDNH on the SWEPT line — mine, ask; (f) the projection's target / regime as a recorded,
scored read — only if he asks. **Tonight:** the day file's `featSource` (from 08:39) and tomorrow's nightly log with
morning rows are the last proof of v15.82.

# (earlier) 2026-09-08, midday — v15.82: THE OUTCOMES EXPORTED FROM THE ARCHIVE (R-22, F-20's fix).

**Panel is v15.82, companion v1.18 (unchanged). INSTALLED AND VERIFIED on his panel 12:08 CT 2026-09-08:** footer v15.82;
`featHealth().perSymbol.SPY` = queue 1,364 records / 29 bars (10:39 → 12:06, the shed window) · archive 2,800 · **merged
3,271 records / 69 bars, 08:39 → 12:06 — the whole session so far, no gap**. (A probe one minute after the reload read
archive 0: the boot loader was still walking the 60,543 archived records — IDB `feat` by date: 08-19 5.3k … 09-04 9.1k,
09-08 2.8k at noon.) v15.81 was verified at 11:31 CT (below). Two other tabs were open on his Chrome at 12:08: an NQ1
/atlas tab (a second panel on the same store — flagged twice) and a Skylit stage page (inert). His words, on the outstanding list I gave him
(R-22 first): *"lets go with your recommendation"*.
**What shipped.** The day file's `feat` and the live `featStats` read **the localStorage queue ∪ the IndexedDB archive**
(`featMergeRecs`: by `key|t`, the LS copy wins — it is the live one; sorted by t); `repoUpsertFeat` keeps the in-memory
`FEAT_ARCHIVE` current on every mirror write (it was boot-time only); `repoFeatDay(date, cb)` reads the IDB feat store
by date; `repoExportDay` writes `feat` = the union, **`featSource`** = `{ls, archive, merged: {n, from, to} per symbol,
note}` and the matrix rebuilt from the union — a harness without `repoFeatDay` degrades to the queue alone;
`featHealth()` shows `archive` / `merged` / `mergedBars` / `lsSpan` / `mergedSpan`; `tools/day-digest.py` carries
`featSource` and names a file without it as pre-v15.82. **Not done, on purpose:** the snapshot mirror stays in
localStorage (the forward labeller, the node-born seed and the day line read it synchronously; the archive already has
every bar) and the shedder is untouched (the v14.76 lesson). **Why it matters:** every nightly and review since 08-25
scored the last 45–130 minutes of each session (F-20); from tonight's file the whole session is in `feat`, and the
far-side-on-gamma study (blocked on ~40 clean sessions) can start counting. Forward-only — the nine old files stay as
they are. `test_v1582.js` 36 · 13/13 mutants; `test_v1571` green through the degrade path; `test_v1581` 0a loosened;
the version pins moved; the roadmap — v15.81 shipped, **v15.82 this build**, seasonality → v15.83, score → v15.84,
READ → v15.85.
**Verified 12:08 CT (above). Still to see:** tonight's day file with `featSource.merged.SPY.from` at ~08:39 CT and
thousands of records (the queue alone would be ~1,500 from ~13:xx); tomorrow morning's nightly log scoring morning
rows for the first time since 08-25; the digest's `featSourceNote` on the new file.
**NEXT, in order, each its own ask:** (a) WH / WL / weekly POC — the companion's window must reach the prior week
(v1.19: two weeks), then PWH/PWL/WPOC draw by name; (b) v15.83 SEASONALITY TRACKED (mockup first; the corpus append into
the nightly — it ends 08-21); (c) the Rec proposals R-1…R-6 await his ✓/✗; (d) the King latch under "match Skylit
always" (his call); (e) LDNL/LDNH on the SWEPT line — mine, ask; (f) the projection's target / regime as a recorded,
scored read — only if he asks.

# (earlier) 2026-09-08, late morning — v15.81: THE IRT EXPORT ON SKYLIT'S OWN FUTURES PRICES (R-23) · F-20 solved (R-22 proposed).

**Panel is v15.81, companion v1.18 (unchanged). INSTALLED AND VERIFIED on his panel 11:31 CT 2026-09-08** (he ran
installv1581.bat at 11:02 CT after a stray double-click of installv1579.bat at 11:00 — the CDN's 5-minute cache then
showed Tampermonkey "Reinstall 15.79" until ~11:09; told him to delete the old installers): footer v15.81; the gear line
"9 of 9 ES rows on Skylit's ES1 prices (2 s old · SPXW ratio 1.000670 · SPY ratio 10.027050)" and "5 of 5 NQ rows on
Skylit's NQ1 prices (52 s old · QQQ ratio 41.118860)" — the NQ book by the self-fetch while the chart is on ES; the file
vs Skylit's derived rows the same minute: SPXW KING 7705.25 vs 7705.15 · G2 7710.25 vs 7710.15 · G3 7685.25 vs 7685.14 ·
G4 7700.25 vs 7700.15 · SPY KING 7710.75 vs 7710.90 (the ratio moved a hair in the minute between) · CW0 7700.25 vs
7700.15 — every line within the 0.25 tick, no '~'. `__gptsDebug.futDer()` shows ES1 (SPY · SPXW · SPX) and NQ1 (QQQ ·
NDXP · NDX — two books more than the doc knew; noted, unused).** His words: *"I added the projecttion feature in skylit. i want you to look it up and
see how it is calculated and how we can use it. I also see percentages next to it … also look at the levels in skylit
and the levels you are sending me in the export, are they the same or is there a computing error. Can this be resolved
using the projected levels which look like current levels?"* → the reading → one question → **"yes, i want them to
match skylits own ES1 prices"** → *"why weren't they done this way before?"* (answered plainly: a mid-August model that
ES has no book, the ES1 feed counted and discarded by the observer, the rule a day old, the rulers never compared at the
same second).
**What shipped.** (1) **The futures books Skylit publishes are read:** `gex/levels?symbol=ES1` / `NQ1` — no book of
their own, `derived[]` = the SPY / SPXW / SPX (QQQ) books with every strike ALREADY at the futures price by Skylit's
live ratio (SPY 770 → 7720.05 at 10.026042, SPXW 7700 → 7704.45 at 1.000578, 10:25 CT) — kept in `LASTFUTDER` by the
observer (never history over live, `futDerNewestT`), self-fetched once a minute in the 0DTE window while the export is
on (`selfFetchFut`; the app asks for ES1 itself on the ES chart, NQ1 needs the fetch). (2) **Every ES and NQ row in the
FlexLevels file at that price:** `skylitFutPx(futSym, book, strike)` → the derived row's k (k / ratio ≈ strike) or
strike × the same ratio (a node below the cut, the IF flip); rows carry `es`; the futures symbol writes it without the
'~'; SPXW KING · G2–G5 · SPY KING · CW0 / PW0 / FLIP0 (an SPX chain, on the SPXW ratio — one ruler) · QQQ KING and its
G rows on NQ1. The panel's own basis is the fallback (no payload, or older than five minutes), tilde-tagged as before;
the King latch stores the source strike so a held King keeps Skylit's price; `IRT_LAST.skyWhy` / `nqSkyWhy` and the
gear's IRT status line say which ruler the file is on; `__gptsDebug.futDer()`. The read/record pipeline never consumes
these payloads. (3) **The projection, understood and recorded** (SKYLIT-FEEDS § THE ES1 BOOK AND THE PROJECTION
FEATURE): "Forward-projected GEX zones past the last candle" — per column the alive expirations of the next-4 basket
merged per strike with |GEX| × dissolve × 1/√(days to expiry, floored at ½), top three labelled with strength against
the column's strongest (100%) — NOT the 0DTE %King (SPXW 7705: 94% on the tape, 82% in the projection); plus a target
price (the |v|-weighted mean strike: the centre path / target dot), a std-dev cone and a regime word (compressed /
balanced / directional / unstable). Read from the app's chunk 988, verified against the drawn labels. Not on the face,
not in the file — his call if ever. (4) **F-20 SOLVED** — the localStorage budget: 09:53 → 10:44 CT shed 0 → 1, the
queue's first bar 08:36 → 09:00, recorder 3,542 of 3,600 KB with one day in it (today ≈ 1.8 M chars, half a snapshot
mirror the IDB archive already holds); the exported snaps come from the archive, the exported feat from the shed
queue. FINDINGS F-20 CONFIRMED, F-10c amended, the review's `dataHealth.mechanism` delivered. **R-22 (proposed, NOT
built — ask): export feat from the archive, stop mirroring snaps into localStorage.** (5) The aligned E/A rows (the
second v15.80 installer) ride in this installer too. `test_v1581.js` 51 · 26/26 mutants; `test_v1580` 6b, `test_v1570`
1d re-pinned; the version pins moved; the roadmap — v15.81 this build, seasonality → v15.82, score → v15.83, READ →
v15.84.
**Verified 11:31 CT (above).** Still to look at with him on the face (not probed): the aligned E/A rows, the candle's
axis on today's live session, the solid IRT lines on his IRT chart itself (the file is right; the chart is his eyes).
**NEXT, in order, each its own ask:** (a) R-22 — the outcomes exported from the archive (F-20's fix; every nightly and
review scores an afternoon-only sample until it ships); (b) WH / WL / weekly POC — the companion's window must reach
the prior week (v1.19: two weeks), then PWH/PWL/WPOC draw by name; (c) v15.82 SEASONALITY TRACKED (mockup first);
(d) LDNL/LDNH on the SWEPT line — mine, ask; (e) the King latch under "match Skylit always" (his call); (f) the
projection's target / regime as a recorded, scored read — only if he asks.
**OPEN, unchanged:** the two day-line wrinkles; the Rec voice + bigger type awaiting ✓; R-11 SWEPT on NQ; the SDK folder
approval; hlNodeAt compares the SPY King and the ATR unconverted against ES extremes (rr=1 on ES bars since v15.08 —
HodN/LodN mostly "—"; noted, not touched); a second Atlas tab (NQ1) was open at 10:45 CT — two panels on one recorder
store; flagged to him.

# (earlier) 2026-09-08, the open — v15.80: THE CANDLE ON THE PRICE AXIS · ONE SOURCE FOR THE SHOWN DAY · HIS KEY LEVELS · the review of Friday.

**Panel is v15.80, companion v1.18 (unchanged). Installed? v15.79 was live on his panel at the open (probed through
his tab: "DAY · TUE 8 SEP · developing", $12,659 under MUD — the 10× bug); installv1580.bat carries v15.76 → v15.80 —
one double-click, the link, reload.** His words, on the v15.79 candle: *"the levels that are swept are the only ones
that should be indicated. EMH and EML are not levels and they should be aligned based on the y axis which should be
the price axis so the candle should show where it swept the level. do you understand. Do you indicate how much time it
took to reclaim the Open in the candle prior to going to the 2nd extreme. Also a MUD of 0m doesn't make sense. double
check the values they are incorrect."* Mockup (Friday, from the real face) → *"much better but the king is not a key
level. the key levels are PDH, PDL, ONL, ONH, WH, WL, Prior day POC, VAH, VAL, Weekly Poc"* → *"I dont want IBL IBH
PDC. you can keep CW0 and PW0. and POC is the prior day poc. VAH and VAL is also prior day VAH and prior day VAL"* →
*"since the market is open you should show today"* → today's candle from his own courier rows → *"build"*. → R-17.
**What shipped.** (1) **The price axis:** every swept KEY level (`LEVEL_TIER` tier 1 = exactly his list — PDH PDL ONH
ONL · prior-day POC VAH VAL · PWH PWL WPOC by name for the day the panel can compute them) drawn as a tick at its own
price with the name and the minute it swept; green reclaimed · red broke · amber being tested; opened-beyond not
drawn; collisions push a line apart with a leader; the reversal names gone; MUD and the reclaim line (`↩ 8:45 · 9m` =
W.END · BOP, his question 4) beside the open tick, left of the bar. (2) **The values:** the parked candle had measured
the recorder's FRAMES (Friday: open 767.39 at 09:46, MUD 0m) while its sweeps read the ES bars (7742, HOD 08:36, MUD
106m) — `measureBarsRaw` in a replay now serves the courier's ES bars for the SHOWN day first (truncated at the parked
minute; ≥ 30 bars; frames otherwise); `closedCandles` keeps the frames for the band / ATR / trend (`replayFrameBars`).
**The MUD dollars were 10× on the ES chart since v15.08** (converted twice) — once now. (3) **His key levels:** the
King → tier 2; EMH/EML, PDC and the IB out of the sweep set; CW0/PW0 stay; the READ's no-sweep sentence says so.
(4) **ONH/ONL are the full Globex night** for the first time (the day-key split had put the evening on yesterday's
key: 509 morning bars, always the PMH/PML stub) — `overnightHL` = the prior calendar key's evening (≥ 17:00) + the
session's morning, FULL when both halves exist (≥ 60 / ≥ 300; the Tuesday after Labor Day had 61 + 509 = 570).
(5) **A key with no RTH bars is not a session:** `futSessionBars(off, day, calendar)` walks over a holiday key
unless `calendar` (overnightHL) — so today's PDH/PDL/profile are Friday's, not Monday's empty key. (6) **The IRT export drops the QQQ King converted onto ES** — *"qqq should only be converted for nq"* (R-18); **every IRT
line solid** (R-19); **G2–G5 on the NQ symbol** from the QQQ book, the ES rule mirrored, held under GQ (R-20).
(7) **The A row under the E row** (today's actuals, the same columns), **MU / MD** for MUD (MU
green to a HOD, MD red to a LOD) on both rows, the candle and the DAY table, HOD cells green / LOD cells red, and
**the expectation for the day** as the E row's first badge — GREEN DAY / RED DAY from the v14.91 opening-range call,
`DAY ?` before it fires, the recent-six count in its hover (R-21). **Then, after the first v15.80 installer:** *"the
rows should be aligned. the firt badge can simply be E Tue and A Tue or something like that so the entire row is
symmetrical"* → **the two rows are ONE thirteen-column grid** (`.g3eag`, rows `display:contents`), tags `E · TUE` /
`A · TUE` (`E · ALL` pooled; the n, the basis, the date, "so far / as it closed" in the hovers), `DAY ?` / `1ST ?`
placeholders so both rows are always thirteen cells, `eagFit()` (after layout, in render) returning the rows to
wrapping below ~900 px (his panel is 943; measured in Chromium at 943 and 700). **installv1580.bat was REBUILT with
it — same name; he must run the second one.** Mockup `mockups/mockup-e-a-rows-aligned.png`. (7b) **THE PROJECTION
QUESTION (2026-09-08, ~10:30 CT, answered, nothing built):** *"I added the projection feature in skylit. i want you
to look it up and see how it is calculated and how we can use it … also look at the levels in skylit and the levels
you are sending me in the export, are they the same or is there a computing error. Can this be resolved using the
projected levels …"* — SKYLIT-FEEDS.md § "THE ES1 BOOK AND THE PROJECTION FEATURE" has the whole of it: the ES1
gex/levels feed is ALL derived (SPY · SPXW · SPX, pre-converted to ES by Skylit's live ratio; the observer already
sees 195 of them a day and drops them); the projection merges the next 4 expirations per strike with
|GEX| × 1/√days-to-expiry, the label % = strength against the column's strongest (100%), plus a target price, a cone
and a regime word (read from chunk 988, verified against the drawn labels); the export's SPXW/SPY rows were the SAME
strikes in the SAME order as Skylit's, placed 0.5–1.5 pt apart because our SPX→ES basis is the IF spot on a 3-minute
clock and Skylit's is its own spot every minute. **Proposed, awaiting his answer: read the export's ES prices straight
from the ES1 derived rows ("match Skylit always").** If yes → its own version (the plan's next must move with it). (8) **THE REVIEW
(stage ⑤) ran** — `review/2026-09-08.json` delivered over the bridge (the panel reads `review/<lastTradingDay>.json`,
so a review is named for the day it is WRITTEN): the headline is FINDINGS **F-20** — the feature queue holds only the
LAST 15–30 bars of every session since 08-25 (13:39→14:57 on Friday) — **and the mechanism was found live the same
morning: the localStorage budget.** Two readings through his tab: 09:53 CT recorder 2,656 KB (3 days), shed 0, the queue
from 08:36; 10:44 CT recorder 3,542 of the 3,600 KB budget (one day left), **shed 1**, the queue's first bar **09:00**,
the LS snapshots' first 09:06. Today's day alone ≈ 1.8 M chars — feat 903 K (1,524 records × ~474 B) + a snapshot
mirror 897 K that the IndexedDB archive already holds. The exported `snaps` come from the archive (complete), the
exported `feat` from the localStorage queue (shed) — the "not the budget" inference had read the archive's snapshots as
proof. FINDINGS F-20 CONFIRMED + F-10c amended + the review's `dataHealth.mechanism`. **The fix is R-22 on Rec
(proposed, NOT built — ask): export feat from the archive (FEAT_ARCHIVE / repoUpsertFeat) merged with the queue, and
stop mirroring snaps into localStorage.** Forward-only: nothing before the fix back-fills; Next Stop 67% (n 153; B 92% > C 61%); node grade INVERTED (A 32% n 65 < B 35% < C 54%); **F-21** the ledger:
acm 15% deflect (n 234) vs dec 15% (n 239), event type does not separate BREAK (10–17%, n 2,257); the HOD/LOD table's
late cells 43–50% live vs 99–100 (n 23/30) → **H10** registered forward (pick `lodhodCell`, `judge_lodhod` in run.py,
study H1.5); no proposal clears the bar; `tools/review-pool.py` + `tools/review-write-2026-09-08.py`. `test_v1580.js`
114 · 49 of 49 mutants; `test_irt_export` 131; re-pinned `test_replay`, `test_nodeat`, `test_v1554`, `test_v1572`, `test_v1557`, `test_v1578`,
`test_v1577` 3c/3o, `test_v1579` 1d, the version pins. Mockups `mockups/mockup-candle-axis{,-row,-today}.png`, `mockups/mockup-e-a-rows-{today,aligned}.png`.
**NOT yet verified on his machine:** the axis on today's live candle (ONL 7687.5 at its price; $1,263 under MD; KING
and CW0 gone from the candle, still in the SWEPT line); the A row under the E row with RED DAY on both, the columns
aligned (E · TUE / A · TUE); the IRT file with
no dashed line, no QQQ row on EPU26, four G rows on ENQU26; the closed state tomorrow pre-open standing on TODAY's ES bars
(open 7711.5, HOD 08:30, not the frames); ONH/ONL named as such (the night full); the H10 row on Testing ①.
**NEXT, in order, each its own ask:** (0) his answer on the ES1-derived prices for the export (R-23, above); (b) R-22 —
the outcomes exported from the archive (F-20's fix; the learning layer's biggest open item — every nightly and review
scores an afternoon-only sample until it ships); (a) WH / WL / weekly POC — the companion's window must reach the
prior week (v1.19: two weeks), then PWH/PWL/WPOC draw by name; (c) v15.81 SEASONALITY TRACKED (the corpus append + his two charts; mockup first);
(d) LDNL/LDNH on the candle's SWEPT line — mine, ask; (e) the King latch under "match Skylit always" (his call).
**OPEN, unchanged:** the two day-line wrinkles; the architecture's integrations list lacks the IRT export; the Rec
voice + bigger type awaiting ✓; R-11 SWEPT on NQ; the SDK folder approval; hlNodeAt compares the SPY King and the ATR
unconverted against ES extremes (rr=1 on ES bars since v15.08 — HodN/LodN mostly "—"; noted, not touched).

# (earlier) 2026-09-08, pre-open — v15.79: THE E ROW BELOW THE LADDER, BIGGER · THE CANDLE'S SWEEPS ARE THE SHOWN DAY'S.

**Panel is v15.79, companion v1.18 (unchanged). Installed? v15.77 reached GitHub via his installer; v15.78's installer
was delivered ~02:50 CT (not confirmed run); installv1579.bat carries v15.76 → v15.79 — one double-click, the link,
reload.** His words: *"move the hod expected stats above the replay below the node ladder and the daily candle and
make the font slightly bigger because it is very small and cant read it"* → mockup at 9 px → *"make the font bigger
and add some spacing so it takes up the row, show mockup"* → 10.5 px spread across the row, shown at 1000 and 1300 px
→ *"build"*. Mid-build: *"on the daily candle you should indicate the levels that the hod and lod swept."* → R-16.
**What shipped.** The E row: built by `secDay` into `SECDAY_EROW`, emitted once by `secLoc` after `secFrame` (the
King cards, the ladder, the candle slot) and before its SET line — under the ladder block, above the replay strip;
values 10.5 px, labels 8.6, chips 9, `space-between`. **The shown day's sweeps:** the candle's labels and the SWEPT
line read `sweepEventsShown` — live, today's; in a replay or the closed state's park, the SHOWN day's
(`futSessionBars(offset, dayStr)` anchored on it; `overnightHL` / `priorProfile` / `sweepLevelsToday` /
`sweepEventsToday` take the day; inside the courier's 5-day window, else none). Why: before the open the courier's
newest day is today with no RTH bar, so the parked Friday candle wore no labels and the SWEPT line said "none yet"
about Friday's face. **Fixed on the way:** `futSessionBars` sorted its unpadded day keys as strings (the 10th before
the 8th — from this Thursday "today" would have been the 9th's bars). **The installer:** mockups ride by last commit,
six of them (v15.78 was at 8.16 of 8.39 MB). `test_v1579.js` 35 · 11 of 11 mutants; `test_v1564` §6, `test_v1577`
4b/4e, `test_v1578` re-pinned. Render `design/render-v1579-face.png`; mockups `mockup-e-row-moved.png`,
`mockup-e-row-big.png`.
**NOT yet verified on his machine:** the row under the ladder at his width and scale; the parked Friday candle with
Friday's sweeps before 08:30 (and the SWEPT line saying the same); then the release at 08:30 with the first live
minute and today's own sweeps accruing on both.
**OPEN, unchanged:** the corpus does not grow (v15.80's first step, with his two seasonality charts); the two day-line
wrinkles; the architecture's integrations list lacks the IRT export; the King latch under "match Skylit always" (his
call); the Rec voice + bigger type awaiting ✓; R-11 SWEPT on NQ; the SDK folder approval.

# (earlier) 2026-09-08 — v15.78: THE DAY CANDLE BESIDE THE LADDER, WITH THE SWEEP LABELS.

**Panel is v15.78, companion v1.18 (unchanged). Installed? v15.77's installer was delivered ~02:10 CT and not
confirmed run; installv1578.bat carries v15.76 → v15.78 — one double-click, the link, reload.** His words, on a
screenshot of his panel with the space to the right of the ladder circled: *"do you see the black space on the right
of the app. see pic. I want the daily candle that is developing to be displayed there. you have the code for this and
the labels also already."* Mockup from the real face → *"you have to add the sweep labels. after doing that, build."*
→ R-15.
**What shipped.** The DAY table's own candle (`dayCandleSvg`, now with a frame argument `{w,h}`; without it the 98×137
candle is unchanged, pinned) beside the node ladder at the ladder's height: `.g3ladcdl`, a flex sibling of the grid
inside `.g3f2` (that flex line IS the black space: the grid is a fixed-column flex item that does not grow), filled by
`candleFit()` after `ladderFit()` in `render()` — the free width the grid's line leaves, measured; under
`CDL_MIN_W`=110 nothing changes; else min(free−4, 260) wide (the −4 is the slot's borders: a candle built at exactly
the free width wrapped under the grid) and the grid's rendered height minus the 17-px header; hidden again if it still
wrapped. Marks: HOD/LOD with their clocks and the leg after each, open/close ticks, MUD and its dollars, the PT line,
the shape spine; LEFT of the bar the reversal names (ON the wick); RIGHT the **SWEPT line's labels** (THROUGH it):
`sweepEventsToday`, tier 1–2, the SWEPT line's colours and hover words, ≤7 a side, a *SWEPT ▸* caption. The header:
*DAY · FRI 4 SEP · as it closed* / *developing*. `test_v1578.js` 44 · 16 of 16 mutants; verified in Playwright at
1000 / 760 / 1300 px (on · hidden · capped). Renders `design/render-v1578-face.png` (sweep labels STUBBED from his
9/4 SWEPT line — the harness has no futures bars) and `design/render-v1578-narrow.png`; mockup
`mockups/mockup-day-candle.png`.
**NOT yet verified on his machine:** the candle on his panel (his width ~1300 css px at his 1.25× scale leaves ~270 px:
the column should show) with the real SWEPT labels of the day; the E row (v15.77) live on a Tuesday; the G rows
(v15.76) on his ES chart; the closed state's release with the first live minute.
**(v15.79) the installer's mockups now ride by last commit, six of them — the 8.16-of-8.39 MB squeeze is resolved.**
**OPEN, unchanged:** the corpus does not grow (v15.79's first step); the two day-line wrinkles; the architecture's
integrations list lacks the IRT export; the King latch under "match Skylit always" (his call); the Rec voice + bigger
type awaiting ✓; R-11 SWEPT on NQ; the SDK folder approval.

# (earlier) 2026-09-08 — v15.77: THE E ROW ON TOP OF THE HOD LINE, PER WEEKDAY — his seasonality.

**Panel is v15.77, companion v1.18 (unchanged). Installed? v15.76's installer was delivered 2026-09-07 evening and
NOT confirmed run (ask); installv1577.bat carries v15.76 + v15.77 — one double-click, the link, reload.** His words,
on a screenshot of HIS OWN tool's A/E strip (not ours — its vocabulary Rly · Done · PB · Num · Ret · Risk · Ext · Tgt
· Rwd · Dur is in no version of this repo; v14.71 verified our wick family AGAINST that panel): *"I want to see the
expected row on top of the HOD at the top. I think it takes into account the day from the data, so if it is tuesday,
it averages Tuesdays. I dont need [the eleven] … show me mockup."* Two mockups rendered from the real face (all
sessions n=284 · the weekday n=55): *"use mockup 2, because it compares friday with fridays in the past and mondays
with past mondays … this is a type of seasonality. you should also be tracking the ranges and other things based on
seasonality (day of week)."* — with his tool's two charts (red vs green by weekday, last 6; daily range by weekday,
last 5–6, vs a 10-week average). → R-14 built; the charts are v15.78 (mockup first).
**What shipped.** `.g3erow` / `hlERowHtml` above the HOD/LOD read line: *E · FRI n=55* · the weekday's recent colour
(*3/3 EVEN*, a count of the last six, STALE after 14 days) · *1ST HOD* (the weekday's LOD-first share before bars) ·
HOD ~8:49a · took ~19m · BOP ~19m · wick ~44m · W.End ~9:13a · wick% ~32% · MUD ~2h50 · LOD ~12:36p · HL gap ~3h26 ·
HL rng ~63.7pts · $3,187 — every ~ a trimmed mean over the SHOWN day's weekday (`hodlodBaseFor(dow)`;
`BASERATES.json` `byWeekday` from `study-hodlod.py` `expected_block`/`weekday_blocks`, the pooled block unchanged;
baked `HODLOD_BASE.byDow`; `hlBaseByDow` floor 40; `hlDowOf` null on weekends; `hlDayShown` = the replay's / the
park's day, else the recorder's). The LADDER (hold rates) stays pooled; the read's hover prose names the same basis
("expected gap", no longer "median"). `test_v1577.js` 51 · 14 of 14 mutants; `test_futbars` + `test_v1564` re-pinned.
Render `design/render-v1577-top.png`; mockup `mockups/mockup-e-row.png`.
**Measured on the corpus (n=55–60 per weekday):** Fri ~19 m to the first extreme vs Tue ~46 m; Thu ~70 pts range vs
Tue ~54; Wed LOD-first 42% vs Tue 60%. Weekday AND first extreme → n≈24: not split.
**⚠ FOUND: the corpus does not grow.** The architecture's hodlod ③–④ (`append-futures.py` → `study-hodlod.py`) are
wired into no task; `data/futures/ES/` is empty; the corpus ends 2026-08-21, so the "recent six" are that date's.
v15.78's first step is wiring both into `tools/nightly/run.py` (his machine), then his two charts on Analysis.
**NOT yet verified on his machine:** the E row live at the top on Tuesday 09-08 (expected *E · TUE n=60 · … HOD/LOD
~9:15a · took ~46m …*, the chip for Tuesdays *3/3 EVEN*, then *1ST …* once the first bar prints); the v15.76 G rows on
his ES chart; the closed state's release at 08:30 with the first live minute; the first stamped session.
**OPEN, unchanged:** the two day-line wrinkles on 9/4's SAVED segment (the late sweep's 00:02 mark; the trimmed
recorder's bar count); the architecture's integrations list lacks the IRT export; the King latch under "match
Skylit always" (his call); the Rec voice + bigger type awaiting ✓; R-11 SWEPT on NQ; the SDK folder approval.

# (earlier) 2026-09-07 Labor Day, afternoon — v15.76: G2–G5 IN THE IRT EXPORT — the rest of Skylit's top-5, white, beside the Kings, CW0, PW0 and FLIP0.

**Panel is v15.76, companion v1.18 (unchanged). Installed? v15.75 IS installed and verified on his panel (⏸ CLOSED,
Fri 4 Sep at 14:58, 100 frames); installv1576.bat carries v15.76 — one double-click, the link, reload.** His words:
*"i want to update the irt export so it exports the top 5 levels for the spx also. G1 - G5. all should be white. in
addition to this, it already exports the kings, cw0, pw0 and the flip."* → R-13.
**The one question and his answer.** Skylit's top-5 includes the King (NODES = 1/3/5/10/15/20 top-N by |%King|,
measured in SKYLIT-FEEDS.md; NODES=1 is the King alone), and the King already has its gold line in the file. Asked
which five, he asked *"What does skylit do"* — answered from the setting map — and chose **"Mirror Skylit: G2–G5"**: the
King's slot is the SPXW KING line, the four that follow are **G2 G3 G4 G5, white, width 1, solid**, no G1.
**What shipped.** In `irtBuildCsv`, after the SPXW KING row: rank the SAME `T.pct` the King row read by |%King| (sign
ignored; ties toward the lower strike), drop the EXPORTED King's strike (the latched crown — a flapping new 100% node
shows as G2, never vanishes), write the next four into `rows[]` so they ride EPU26 (0.25 tick, the `~` rule) and SPY
(cents) like every row; one conversion closure `spxRow` for the King and the G rows (v14.14 precedence: live ES chart →
dispScale/R, else undScale, else dispScale/R); held for the session like the Kings (`irtGLatch`/`irtGHeld`, key
`gpts_irt_kings_v1` under `G`, day-scoped); `IRT_LAST.gWhy` (*live (G2 7630 85%, …)* · *held Nm* · *no tape and
nothing latched today*); the probe is `__gptsDebug.irt()` (preview shows the first 8 lines; `last.gWhy`).
`test_irt_export.js` 129 (was 108; §4t) · 14 of 14 mutants — the survivor of the first run exposed a fixture that
could not tell the shared conversion from a re-derived one (LESSONS v15.76 (2)); `test_em_band.js` re-pinned.
**NOT yet verified on his machine:** the four white lines on his ES chart after the v15.76 reload and one export (the
gear's ⇩ Export now, or the 180 s tick) — expected in `FlexLevelsExport.csv`: SPXW KING, G2, G3, G4, G5, SPY KING,
CW0, PW0, FLIP0 on EPU26 (each again on SPY), QQQ KING on ENQU26, QQQ KING ~ on EPU26. On the closed state the four
come from Friday's parked book, exactly like the King.
**OPEN, found live on 9/4's day line (not folded into this build — his request was the export):** (a) SAVED reads
"00:02 · 25 bars": the late sweep's `dayWritten` mark (00:02 CT, RULE 3 write-if-absent found the file present) is
preferred over the first write (15:01 auto) — the evidence should be the first write; (b) `dayBarCount` reads the
trimmed localStorage recorder (25) instead of the day file / IndexedDB (101). Both are `dayLineState` evidence bugs;
fix together, pin in a `test_v1577.js`, ask nothing. (c) The architecture's integrations list (plan-seed `system.integrations`,
pinned at 5 by test_v1567 7a) has no row for the IRT FlexLevels export — an OUTBOUND integration; add it as the sixth when
plan-seed is next touched, with the pin moved.
**THE FIRST DECISIONS ARE HIS (unchanged):** the six proposed Rec rows — R-1 first; the Rec voice + bigger type
(mockup `mockups/mockup-rec-why-gain.png`, R-10 reserved) awaits his ✓ — numbering now v15.77+ (the candidate score is
v15.77 in the plan; the voice takes the next free number when he approves). Open, in his order: (3b) GATE / AIR POCKET
in PATTERN; (4) SWEPT on ES and NQ (R-11 drafted); (5) SWEPT majors only. The SDK folder approval (from his desktop)
opens the second project.

# (earlier) 2026-09-07 Labor Day morning — v15.75: THE CLOSED STATE — the dashboard stands on the last session when the market has not opened.

**Panel is v15.75, companion v1.18 (unchanged). Installed? v15.74 IS installed; installv1575.bat carries v15.75 — one
double-click, the link, reload.** His words at the panel, 08:3x CT on Labor Day: *"the application doesn't seem to
support a frozen state from when it was open so i can really work on it … there is no node ladder"* → R-12, *"ok.. fix"*.
Measured through his Chrome tab (read-only, the sanctioned probe): every feed arriving, the clock past 08:30, the day
line "recording · 0 bars", the face empty, a drag on Friday's strip handed back to LIVE by the stale-day guard.
**What shipped.** The session signal is the gamma payload's own minute series (`levels[].t`, 390 minutes of the last
session — Friday 14:59 CT on the holiday; today's minute on a trading day): `liveBookToday()`. `closedState()` →
`replayAutoPark()` parks on the last weekday with frames at its LAST frame (`REPLAY.auto`), the whole face as it was,
under the CLOSED bar and the ⏸ CLOSED badge; the recorder stays blind (nothing to record); `replayAutoRelease()` returns
to LIVE with today's first live minute, silently; his drag / ◀ ▶ make it his replay; LIVE out of it stands for the day.
The stale-day guard now requires the live book (a holiday no longer evicts) and never touches an auto-park; the day
line reads the last session on a holiday. `test_v1575.js` 52 / 14 of 14 mutants. Render `design/render-v1575-face.png`.
**The design conversation of 2026-09-06/07 is in `session-state/2026-09-07_design-notes-purpose-tags-flow.md`** — READ
IT: the two purpose tags (HOD/LOD · DEFLECTION, final), the Rec voice (why · what you gain · serves) and bigger type
awaiting his ✓ (mockup `mockups/mockup-rec-why-gain.png`; R-10 reserved), absorption = delta at price (the footprint),
the sources (Skylit per-bar toggles unmeasured; StoneX/CQG WebAPI; **the IRT RTX SDK** — the documented path, the
folder approval pending on his desktop), the second project's purpose draft in his words, SWEPT on NQ (R-11 proposed).
**VERIFIED on his panel after the v15.75 reload (read-only probe):** v15.75, `closed:{closed:true, why:'no session so
far today', day:'2026-09-04'}`, badge ⏸ CLOSED, the bar text, 100 frames at idx 99, King cards + 15 ladder rows.
**NOT yet verified:** Tuesday 09-08: the release at 08:30 with the first live minute, then the first stamped session
and the automatic close.
**THE FIRST DECISIONS ARE HIS (unchanged):** the six proposed Rec rows — R-1 first. Open, in his order: (3b) GATE /
AIR POCKET in PATTERN; (4) SWEPT on ES and NQ (R-11 drafted); (5) SWEPT majors only. The candidate score → v15.77.

# (earlier) 2026-09-05 small hours — v15.74: THE LOG SURVIVES A RELOAD (+ v15.74b: THE ORIGIN GUARD — the installer had overwritten his machine's run).

**Panel is v15.74, companion v1.18 (unchanged). Installed? v15.73 IS installed (his push 22:2x CT); installv1574.bat
carries v15.74 — one double-click, the link, reload.** His first sight of v15.73 (22:3x CT): *"why hasn't the analysis
started"* — the line read "analysis overdue — is the GEX nightly task installed?" while `learning/log/2026-09-04.json`
was on GitHub (his machine's run, 22:35 CT, `ranOn: his machine`, pushed 22:36). Cause: `ANALYSIS_NIGHTLY` lived only
in memory; the pipeline throttle (`P.t`) survives a reload and refused the refetch for up to ten minutes. Fix: the log is
kept in `gpts_nightly_v1` and restored at load (`nightlyLoad` / `nightlySave`); `test_v1574.js` 29 / 5 of 5 mutants,
with the scene (Saturday small hours, a stored log, a fresh pipe stamp → analysis GREEN "22:35 · your machine · 81
taps") and the pre-fix lie kept beside it. The plan: v15.73 shipped, v15.74 this, the candidate score → v15.75 (→ v15.77 after v15.75 the closed state and v15.76 the Rec voice).
**Lesson (LESSONS v15.74):** ask of every variable a status reads, "what survives a reload?" — a status and the timer
that refreshes its source must share a lifetime, or the status lies for the timer's length.
**v15.74b (tools, the same night) — "double check .. look at analysis": he was right.** The v15.74 installer, built
from a clone that had never pulled, put the CLOUD's copy of the 9/4 log (19:44) back over his machine's run (22:35) —
the Analysis tab named the cloud. The numbers were identical; only `ranOn` / `ranAt` differed. His task re-ran at 23:45
(the extracted log has mtime 0, so the day file reads newer) and the sync pushed it at 23:46 — GitHub carries his
machine's run again. Fix: `tools/origin-guard.py` before every commit (adopts his machine's newer copies of the files
it writes, refuses a real conflict); `build-installer.py` refuses to build over anything left to adopt. This clone
adopted the seven files. `test_origin_guard.js` (8). BUILD-CHECKLIST §1a · PROJECT-CONSTANTS L-O · the skill's SAVE §3.
**VERIFIED on his machine (over the bridge, `tools\gex-nightly.log`):** three clean UTF-8 runs for 9/4 — 21:55, 22:35,
23:45 CT — the same numbers each time (81 taps · H5 READY 78 · resume 21/28 = 75%).
**NOT yet verified on his machine:** the day line after the v15.74 reload (ask him what it shows — expected: 9/4 ·
saved 15:01 · analysis 23:45 · your machine · 81 taps); Monday's first stamped session; the automatic close.
**THE FIRST DECISIONS ARE HIS (unchanged):** the six proposed Rec rows — R-1 first, R-2 second. Open, in his order:
(3b) GATE / AIR POCKET in PATTERN; (4) SWEPT on ES and NQ; (5) SWEPT majors only. H5 READY at 78 — the join rides the
candidate-score build (v15.75).

# (earlier the same night) — v15.73: THE DAY LINE — the process reporting on itself at the bottom of the panel.

**Panel is v15.73, companion v1.18 (unchanged). Installed? v15.72 IS installed (his push at 20:0x CT); the "GEX nightly"
task IS installed and ran at 19:21 CT; installv1573.bat carries v15.73 — one double-click, the link, reload.**
**His words, and the standing expectation (PURPOSE §3b):** "my expectation is that from now on i will not have to click
on the save button and that i will only goto the rec tab to discuss recommendations with you, everything else will be
done automatically" · then: "there needs to be some message at the bottom that tells me that says something like 9/4 -
data saved analysis complete, testing complete, recommendations made. something descriptive" → mocked up in three
moments (the session · the file written · everything done), "i like it." → built.
**What shipped (v15.73).** `dayLineState` / `dayLineHtml` (`.g3pline`) between the AFTER HOURS bar and the footer, both
render paths: the date, then saved · analysis · testing · learning · rec with evidence and colour (green done · amber
expected, not yet · red overdue with the cause · grey not knowable); during the session "data · recording · N bars";
yesterday's completed line until today's first bar; a download-only day "not saved"; the hover is the sentence; every
fact from what the panel already fetches; `fmtCT`; `__gptsDebug.dayLine()`. R-9 implemented; the plan (v15.72 shipped,
v15.73 this, the score → v15.74 — since v15.74 is the reload fix, → v15.75); DATA-ANALYSIS-PROCESS.md names the line; INVENTORY §0i; the config (2026-09-05b).
`test_v1573.js` 44 / 14 of 14 mutants. Mockup `mockups/mockup-day-line.png`; render `design/render-v1573-face.png`.
**NOT yet verified on his machine:** the line live on Monday, segment by segment after 15:01; the first stamped session
(kroll, king:floor:up); the automatic close (writtenBy auto).
**THE FIRST DECISIONS ARE HIS (unchanged):** the six proposed Rec rows — R-1 (record the reads) first, R-2 second. **Also
open, in his order:** (3b) GATE / AIR POCKET in PATTERN; (4) SWEPT on ES and NQ; (5) SWEPT majors only. H5 READY at 78 —
the join rides the candidate-score build (v15.75 since the reload fix took v15.74).

# (earlier the same night) — v15.72: THE FACE (his three asks + the leaked border) · THE ROLLING FLOOR / CEILING REGISTERED (H8 / H9).

**Panel is v15.72, companion v1.18 (unchanged). Installed? v15.71 IS installed (his push at 19:08 CT, GitHub at 15.71;
the "GEX nightly" task NOT yet set up — no `tools\gex-nightly.log`); installv1572.bat carries v15.72. Two double-clicks
still: the installer, then `setup-gex-nightly.bat` once.**
**What happened tonight, in his order.** (1) He checked the install: GitHub had v15.71; today's file had been written at
15:01 by the panel itself (v15.65 was on the panel, not v15.64). (2) "the after hours message to the left is bad choice …
put it on the bottom … make the king badges bigger … the font in the node ladder" → measured (the cards sat at 462 of
649 px beside the chip in one wrapping flex row), mocked up from the real panel at 673 / 760 px, approved ("yes .. build").
(3) "why do the rugs and the barneys have a yellow in them" → the strikes under the block are yellow because he asked
for it in v15.65; "there is yellow in the rectangle right before the purple" → a BUG: the dead `.g3pb` pullback rule's
amber left border leaking into the v15.65 pattern block of the same name — fixed and pinned. (4) "when the king rolls up
and is below price it may be creating a floor (support) and be bullish and vice versa" → the doctrine's rolling floor on
the King (FOLLOWING); registered the same night as H8 / H9 with the stamp field it needs. (5) "have you run the process
to save, analyze etc.?" → the nightly ran in the cloud on the panel's 15:01 file (his task had not fired), outputs written
into his repo folder over the bridge; F-19 got its second day (turn 1 of 28, resume 21 of 28 = 75% low 57%, held 35 of 78).
**What shipped (v15.72).** `afterHoursChipHtml` (the bar at the bottom, both render paths; gone from `secFrame`);
`.g3kz` full row + the card sizes; the ladder 9 px with `56 68 118 66 94 68 70 70 70` and the bar at 112; the dead `.g3pb`
rule removed, `border-left:0`; `kingRollsNow` → `kroll` on every ledger event; four classes in `PAT_CLASSES` /
`tapClasses` and `patterns.py` (pinned equal); K2.6 / K2.7 (studies-seed, REGISTERED, sourced from the classes in
results.py); H8 / H9 in `learning/register.json` + `PREREG_SEED` + `HYP_STUDY` (pick `pat`, outcome `resume`, base
`dir:up` / `dir:dn`, minN 30, since 2026-09-08, judgedBy nightly) read by `run.py judge_pat` from the pattern table;
`.gitattributes` fixed (`* -text`; the task scripts CRLF again); `Claude outputs/` ignored; the Analysis mockup generator
draws the nightly's count line; R-8 implemented on Rec; the plan (v15.71 shipped, v15.72 this, the score → v15.73).
`test_v1572.js` 63 / 12 of 12 mutants. Render `design/render-v1572-face.png`; mockup `mockups/mockup-after-hours-bottom.png`.
**VERIFIED after his install (v15.72b, the same night):** GitHub at v15.72; `.gitattributes` on origin, the task scripts
CRLF; the "GEX nightly" task ran on his machine at 19:21 CT (first run) — its one wart, a cp1252 print of ≥ in the pattern
report ("patterns threw: 'charmap'"), fixed: UTF-8 stdout in run.py / tick.py, PYTHONUTF8 in the .bat, written over the
bridge. His words after the install (PURPOSE §3b): no click on the save, only the Rec tab; R-LINK (the Tampermonkey link
as a clickable link) in the constants. **NOT yet verified:** the panel widening once for the ladder; Monday: the first stamped
session — `kroll` on a tap, `king:floor:up` in the nightly's table, the automatic close (writtenBy auto), the nightly task
(once he runs `setup-gex-nightly.bat`).
**THE FIRST DECISIONS ARE HIS (unchanged):** the six proposed Rec rows — R-1 (record the reads) first, R-2 second. **Also
open, in his order:** (3b) GATE / AIR POCKET in PATTERN; (4) SWEPT on ES and NQ; (5) SWEPT majors only. **H5 is READY at
78 events** — the join is part of the candidate-score build (v15.73), not a session's afternoon.

# (earlier the same night) — v15.71: THE SAVE RUNS ITSELF. No click at the close; a missed day written the next morning; the 💾 a chip.

**Panel is v15.71, companion v1.18 (unchanged). Installed? NOT YET — he has installed nothing since v15.64:
installv1571.bat carries v15.65 → v15.71; two double-clicks (the installer, then `setup-gex-nightly.bat` once).**
**How it came about.** His ask: "automatically have the application trigger the save button instead of me clicking
it … if the save button has not been pressed and the time is 5pm or later, trigger it. if the save button for the
previous day has not been triggered and the time is during non market hours, trigger it." My read of the record
FIRST: the last seven day files were exported at 15:01–15:03 CT by the v10.44 auto-export — his click was a second
write; what was missing was the retry after 16:00, the missed-day write, and the honest failure (the silent path
downloaded and counted the day as saved). He approved the three rules + the DUE chip with one change: "instead of 5pm
can you just modify so it is after market hours". Built and committed the same night.
**What shipped.** One writer `repoExportDay(date, silent, by, done)`; RULE 1+2 `repoAutoExportTick` (after the close,
weekdays, no upper bound, 10-minute retry until `saveState()` = saved); RULE 3 `repoLateSweep` / `repoLateSaveTick`
(outside market hours, 10-day look-back, write-if-absent, kv `dayWritten:<day>`, the tape's sweep along); the refusal
on every path (no bars → nothing written — the 08-29 / 08-30 weekend files were the old export's); `writtenBy`
click · auto · late in the file; SAVED = confirmed in the folder (the silent download is gone; blocked instead:
permission · no folder · write failed); `autosavePermTick` asks (never requests) every 10 minutes and re-arms the
rules on a grant; the click `repoClickExport` requests the permission FIRST, synchronously, on `DATA_DIR_H` cached at
boot, then today, then the earlier days; the 💾 chip (idle · saved · pending · 💾! · 💾 DUE(+n) · nodata) with the
question-first hover; `autosaveTick()` before the 08:30 gate. Docs: DATA-ANALYSIS-PROCESS ("none, since v15.71"),
PROCESS ② / §5b, PURPOSE §3b, INVENTORY §0g, ARCHITECTURE / the ⚙ tab, the skill, the config (`autosave`, 2026-09-04h),
the plan (v15.70 shipped, v15.71 this, the score → v15.72). `test_v1571.js` 90 / 16 of 16 mutants; R-7 on Rec, implemented. Mockup
`mockups/mockup-autosave-chip.png`. Render probe: the chip renders on the 09-03 replay ("pending" after the close).
**NOT yet verified on his machine:** the install; the first close with the tab open and no click → `data/<day>.json`
with `writtenBy: "auto"` on GitHub within ~12 minutes, the nightly's log after it; the chip live (`__gptsDebug.autosave()`);
whether his Chrome already holds "Allow on every visit" (the daily 15:01 exports say the grant persists — if the chip
shows 💾! or DUE after a reload, one click and the three-way prompt settle it).
**THE ONE THING HE STILL DOES:** keep the Atlas tab open through the close (a closed browser runs no timer); if it was
closed, the next open outside market hours writes the day (rule 3) — pre-market, so the nightly runs before the open.
**THE FIRST DECISIONS ARE HIS (unchanged):** the six Rec rows — R-1 (record the reads) first, R-2 second. **Also open,
in his order:** (3b) GATE / AIR POCKET in PATTERN; (4) SWEPT on ES and NQ; (5) SWEPT majors only.

# (earlier the same night) — v15.70: 💡 REC; THE PROCESS NAMED AND PINNED; learning/markets.json.

**Panel is v15.70, companion v1.18 (unchanged). Installed? NOT YET — installv1570.bat carries v15.65 → v15.70; two
double-clicks (the installer, then `setup-gex-nightly.bat` once).** Built tonight on his "ok build out everything":
`learning/recommendations.json` (the review's R-1…R-6 real rows; the nightly's machine rows from pre-registered
conditions in `tools/nightly/recommend.py`; his decisions from the day files' `reco`); the panel's 💡 Rec tab (`recBlock`,
✓ / ✗ → `gpts_reco_v1` → the export's `reco`; fetched at boot and on the 10-minute check; `REC_SEED`); `design/DATA-
ANALYSIS-PROCESS.md` + `test_data_analysis_process.js`; `learning/markets.json` (SPY live; NQ · GC · CL price only;
`patterns.py` reads the tolerances; the panel's constants pinned equal); PLAN (the eighth tab, `process`, stages ⑤ / ⑦,
storage, roadmap v15.70 this build, the candidate score → v15.71), the skill's load order, PROCESS.md's banner, the
config (`process`, `rec`, 2026-09-04g). `test_v1570.js` 31 / 18 of 18 mutations. Render `design/render-v1570-rec.png`.
**NOT yet verified on his machine:** the install; the Rec tab with its six rows; a ✓ → "your ✓ · rides the next 💾" →
after the next 💾 and the nightly, the row's status in the file → APPROVED with the day.
**THE FIRST DECISIONS ARE HIS:** R-1 (record the reads) is the one I would take first — it is the missing chain for
"directional prediction" and "reads"; R-2 (the count-to-test draft) second. Then R-3 / R-4 / R-5 / R-6 in his order.
**A gold market** is a configuration entry once a gamma book source exists (a GLD chain or a Skylit GLD ladder — to
confirm); the price half (GC bars) is already couriered.
**Also still open, in his order:** (3b) GATE / AIR POCKET in PATTERN; (4) SWEPT on ES and NQ; (5) SWEPT majors only.

# (earlier the same night) — v15.69: THE OBJECTIVE OUTCOMES (turn · stay in) ON EVERY TAP; THE LEARN TAB CARRIES THE RECORD.

**Panel is v15.69, companion v1.18 (unchanged). Installed? NOT YET — installv1569.bat carries v15.65 → v15.69 (GitHub was
at v15.64 when built); two double-clicks: the installer, then `setup-gex-nightly.bat` ONCE (Python 3.13.15 is on his PATH).**
**THE STANDING REQUIREMENT, his words tonight (PURPOSE §3b — read it):** "a trading decision support system that is
data driven … data capture, analysis testing back to dashboard for everything that is displayed on the dashboard
including hod lod time, nodes, setups, directional prediction, reads and more" · "the learn tab should also be
updated" · "the entire data, analysis and testing process results in learning and it is from the learning that can
know something and make a decision based on what you see on the dashboard". The chain ends at KNOWLEDGE; the face
draws from it; knowledge has degrees (confirmed · provisional · doctrine · descriptive) and the face must show which.
**The reflection he asked for ("see if we are missing anything … then build") found:** (a) every held rate scored a
proxy ("continued 0.3 SPY in ten bars"), never the objective — built tonight; (b) the Learn rules were confirmed by
taught legs only, the record never asked — built tonight; (c) the face's READS are shown and never recorded, so
"directional prediction" and "reads" have no chain at all — NEXT; (d) a count never becomes a test by itself — the
auto-draft into the register, out of sample from the next session — after (c); (e) recommendations with his tick, the
live pre-tap read joined to the tested number — after that. Order agreed in principle; he has not yet said "go" on (c).
**What shipped (v15.69).** `patterns.py`: TURN / RESUME per tap from the day file's own SPY bars (h / l per 3-min bar),
pre-registered (TURN_TOL 0.50 SPY, first tap per node per day, ≥ 20 bars), `turn` / `resume` cells per class on the
nightly's table (`log.patterns.rows[].turn/resume`, `outcomes`); `results.py`: the Analysis lines read "held · turn ·
resume"; `RULE_SOURCES` → each Learn rule's evidence + verdict (agrees / contradicts / thin / measured / not measured),
written into `learning/deflections/examples.json` and `results.json.rules`; `learn-seed.py` merges; LEARNING.md prints
it; the panel: Testing ⑦ two extra columns on the nightly's table, Learn ② the record under each rule (status untouched),
LEARN_SEED + PLAN_SEED re-spliced; six King-condition classes in both twins. `test_v1569.js` 26 / 15 of 15 mutations.
**THE FIRST READ (F-19, one day, 19 episodes, old names):** turn 1 of 19 (5%) · resume 13 of 19 (68%) · held 24 of 51
(47%). A tap is rarely THE turn — the base every class must beat; the HOD/LOD question is WHICH tap.
**NOT yet verified on his machine:** the two double-clicks; after the next 💾: the log with ranOn "his machine", the
turn / resume columns on Testing ⑦, the record's line under each Learn rule.
**HIS OPEN ITEMS, IN HIS ORDER (unchanged):** (3b) GATE / AIR POCKET in PATTERN; (4) SWEPT on ES and NQ; (5) SWEPT
majors only (PDC — ask). The candidate score is v15.70 and should be built from tested rules, not hand weights.

# (earlier, late evening) — v15.68: THE LOOP CLOSES ON THE CLICK. THE NIGHTLY RUNS ON HIS MACHINE AND WRITES THE ANALYSIS TAB.

**Panel is v15.68, companion v1.18 (unchanged). Installed? NOT YET — installv1568.bat carries v15.65 → v15.68 (GitHub
was at v15.64 when it was built); he has TWO double-clicks this time: the installer, then `setup-gex-nightly.bat`
ONCE (it registers the "GEX nightly" Windows task; needs Python 3 on the PATH — he has 3.13.15).**
**How it came about (the step-by-step conversation, his words):** "Obviously it is triggered by the save button (disk
icon). Then you take that data and perform your analysis and update the analysis tab, which gives you ideas to test.
is that how it goes" → my answer: no — nothing ran after the click until I opened a session, and the Analysis tab's
registry changed only when a review edited the seed → "this seems like a gap. when i click on the save button and the
push happens, you have everything to start the analysis" → "i envision clicking on the save, the data getting saved
and the analysis occurring and the analysis tab being updated" → "so can you update that so you do what you need to
do". Python 3.13.15 is on his PATH (`python --version`).
**What shipped.** (1) `setup-gex-nightly.bat` (run once) → the "GEX nightly" task: `tools\gex-nightly.bat` every 10
minutes, hidden (`gex-nightly-hidden.vbs`), a lock, `python` / `py -3`, no PowerShell → `tools/nightly/tick.py` runs
`run.py` ONLY when the newest `data/<day>.json` is newer than `learning/log/<day>.json` (exit 3, silent, otherwise;
"today" = the newest day file, never `%DATE%`). (2) `tools/nightly/results.py`: the nightly writes the registry —
`STUDY_SOURCES` maps 21 study rows (the King by book S0.1–S0.4 / K1.1 / K1.2, floor / ceiling K2.1 / K2.2, growing /
fading and ±γ at the King S0.5 / S0.7, rugs S1.1 / S1.2, stacks S7.1 / S7.2, the register's verdicts via `HYP_STUDY`) →
`learning/results.json` + `learning/studies.json` patched: a VERDICT (a rate at n ≥ 15, or cleared / refused /
ready) sets `result` · `status` (READ · THIN · REFUSED · READ NEXT) · `by:'nightly'` · `asOf`; a thin row KEEPS the
review's sentence and carries the count so far as `nightly`. `studies-seed.py` merges results.json (the seed owns the
questions, the nightly the numbers — L-U). `run.py`: `ranOn` / `ranAt` in the log, atomic writes, the sweep table's
relative path, unknown flags refused (`--help` used to run it). New classes in both twins: `king:floor · king:ceil ·
king:grow · king:fade · king:pos · king:neg`. (3) The panel: `pipeCheck` re-fetches the registry every 10 minutes;
`studyRowHtml` tags a nightly result "· by the nightly, <date>" (blue) and shows the count so far as "⟳ …"; the ⚙
NIGHTLY box says "ran on his machine / cloud". Stage ④, the nightly component, storage, PROCESS ④ / §5b, DATA-
ARCHITECTURE §3, `.gex-config.json` (nightly, 2026-09-04e) follow; roadmap v15.67 shipped, v15.68 this, the candidate
score → v15.69. Already on the 09-03 registry: H1.3 → READ NEXT (H5 ready at 51); F2.1 / F5.2 / F6.1 keep their
sentences with "H2 thin: n=1 of 30" beside them. `test_v1568.js` 45 / 21 of 21 mutations.
**NOT yet verified on his machine — after the two double-clicks:** `setup-gex-nightly.bat` prints python 3.13, "task
GEX nightly created", and tick's decision; after the next 💾, within ~12 minutes `tools\gex-nightly.log` has one
"nightly ran" line, GitHub has `learning/log/<day>.json` with `ranOn: "his machine"` and `learning/results.json`, and
the ⚙ tab's NIGHTLY box reads "ran on his machine"; Analysis rows read "by the nightly, <day>". If `tools\gex-nightly.log`
says PYTHON NOT FOUND, the task's environment lacks the PATH entry — the fix is the `py -3` launcher or a full path.
**What still waits for a session:** the REVIEW (⑤) — a READ into a hypothesis, items answered, FINDINGS, the seed's
questions. Candidate: a scheduled cloud session bound to his computer. Not built; not discussed yet.
**HIS OPEN ITEMS, IN HIS ORDER (unchanged):** (3b) GATE / AIR POCKET in PATTERN (Academy definitions first); (4)
SWEPT on ES and NQ; (5) SWEPT majors only (PDC — ask). Then v15.69 the deflection candidate score.

# (earlier the same evening) — v15.67: THE SETUPS AND PATTERNS SCORED; THE COMPLETE ARCHITECTURE ON THE ⚙ TAB AND IN design/ARCHITECTURE.md.

**Panel is v15.67, companion v1.18 (unchanged). Installed? NOT YET CONFIRMED — installv1567.bat carries v15.65 + v15.66 +
v15.67 (GitHub was still at v15.64 `c61d6df` when it was built); he has not yet said he ran v15.65/66 either.** His ask,
verbatim: "you need to score these setups and patterns to get proper probabilities and insights. build and make sure you
are updating the architecture document and tab in the application which should be a complete architecture / design of
the application's process, including the integrations with inside finance and yahoo data that is used to get hod lod
statistics daily."
**What shipped.** (1) Every new tap in the deflection ledger is STAMPED at the tap with what the PATTERN columns showed
at that strike, per book (`pat:{spx,spy,qqq}` — pika / barney stack named or member, rug / rrug, NEW, growth, polarity),
by the face's own functions (`tapPatternStamp` → `gridBookNodes` / `gridSetups` / `nodeIsNew` / `nodeGrowth`), and the
Kings it touched (`kings`). (2) **THE BUG FOUND ON THE WAY (LESSONS v15.67, L-T, F-18):** the ledger is in the BOOK'S
units (SPY 768) and the ladder in the CHART'S (7712); `kingsAtStrike` compared them raw since v15.63, so `kings` was `[]`
on every tap and the per-book King tally never counted a King. `tapDisp(sym, k)` converts once with the ladder's own
ratios; a pre-v15.67 row says nothing about the King (`tapClasses` claims King-none only for a stamped tap). (3) The
table — `patternTable`: every tap · King SPX/SPY/QQQ/any/none · pika / barney per book · rug / rrug per book · SPX NEW /
growing / fading / +γ / −γ / none · the old detector's names · UP / DOWN — held / broke / pending, a rate at n ≥ 15 with
its Wilson low, `thin (n=…)` under — **Testing ⑦ THE PATTERNS**, live (today + the IDB archive `DEFL_ARCH`) and from the
nightly (`log.patterns`, `tools/nightly/patterns.py`, the Python twin pinned equal by test_v1567 3l). (4) **The
architecture:** `PLAN.system` in `tools/plan-seed.py` → ⚙ Architecture ⑥ THE SYSTEM (nine components) · ⑦ THE
INTEGRATIONS (Skylit · InsiderFinance · Yahoo · ForexFactory · GitHub — how · what · keys · notes) · ⑧ THE HOD/LOD
STATISTICS, DAILY (eight steps, the corrected rates only) · ⑨ STORAGE, and `design/ARCHITECTURE.md` from the same seed
(one source, pinned). Roadmap: v15.66 shipped, v15.67 this build, the candidate score → v15.68 (…v15.75).
`test_v1567.js` 57 / 19 of 19 mutations. Renders `design/render-v1567-patterns.png` · `render-v1567-architecture.png`.
**What the numbers say today (09-03, the only register-era day, OLD names, none stamped):** every tap 24/51 = 47%
(low 34%); Floor 11/18 = 61% (low 39%); King 2/6, Gate 6/11, Rug 2/4, Ceiling 3/12 thin; UP 52% (n=31), DOWN 40%
(n=20). Not pattern rates in the face's sense — 0 pika/barney under the old names while the face showed stacks.
**NOT yet verified live — probe after he installs:** Testing ⑦ renders with his ledger; `__gptsDebug.stamp(774)` on a
live page returns `disp` ≈ SPY×10, `kings` non-empty when a King sits there, `pat.spx` with `st`/`rug`; after a day
of taps `__gptsDebug.patterns().stamped > 0`; ⚙ Architecture shows ⑥–⑨ with nothing clipped. Then the v15.66 checks
(`__gptsDebug.tape()`, `data/tape/<day>/` after the 💾).
**HIS OPEN ITEMS, IN HIS ORDER (unchanged):** (3b) GATE / AIR POCKET / other doctrine shapes in PATTERN — each needs its
Academy definition named first (the gate still scores under the OLD detector's name); (4) SWEPT on ES and NQ; (5) SWEPT
majors only (PDC — ask). Then v15.68 the deflection candidate score (the stamps are its factors).
**Small debt:** `python3 tools/nightly/run.py --help` RUNS the nightly (no argument parsing) — add a guard.

# (earlier, late afternoon) — v15.66: THE TAPE. THE WHOLE BOOK, EVERY BAR, EVERY MARKET, IN DAILY FILES.

**Panel is v15.66, companion v1.18 (unchanged).** His ask: "you must store the whole day … save the entire tape in daily
files for each market". Until v15.65 the record held 90 SPXW strikes per bar and only the 8 strongest SPY/QQQ strikes
(the `tri` top-8 since v11.2) — a sketch, not a tape (LESSONS v15.66). Now every closed bar: SPXW's full velocity table
(~286 strikes, Skylit's dollars + d5/d15/d60/d1d) and the whole Trinity ladders for SPY · QQQ · VIX (every strike as
%King + the King's $K → dollars) into IndexedDB (`repo.tape`, db v4); written ONCE at the close beside the day file by
the same 💾 / auto-export as `data/tape/<day>/SPXW·SPY·QQQ·VIX.json` (shared strike list, aligned rows); a captured
day never written is written on the next boot / 💾, once. NOT periodic — his question, answered: each write is a 2 MB
blob the sync task pushes; IndexedDB is the safety net, the file the courier. `tools/nightly/tape.py` (load · dollars
· coverage), `run.py` logs the coverage. `test_v1566.js` 39 / 11 of 11 mutations. DATA-ARCHITECTURE §6b, PROCESS ②.
**HIS ONE STEP IS UNCHANGED:** 💾 at the close (📁 must point at the DATA folder — the tape lands in `data/tape/`).
**NOT yet verified live — after he installs and the next close:** `__gptsDebug.tape()` shows four books with bars
accruing from the first closed bar after the reload; after the 💾, `data/tape/<day>/` has four files, the sync task
pushes them, `python3 tools/nightly/tape.py <day>` prints ~130 bars, SPXW ~280 strikes, SPY/QQQ ~100.
**HIS OPEN ITEMS, IN HIS ORDER (unchanged):** (3b) GATE / AIR POCKET / other doctrine shapes in PATTERN; (4) SWEPT on ES
and NQ; (5) SWEPT majors only (PDC — ask). Then v15.67 the deflection candidate score.

# (earlier the same afternoon) — v15.65: THE PATTERN COLUMNS, ONE PER BOOK; THE KINGS IN THEIR OWN COLOURS.

**Panel is v15.65, companion v1.18 (unchanged).** Built item by item after v15.64 was verified live: (2) SETUP → three
PATTERN columns, SPX · SPY · QQQ — a PIKA (yellow) / BARNEY (purple) block, the word in black, the book's OWN strikes in
yellow under it even on a converted-price row; RUG / RRUG blocks with their two strikes; the full name in the hover; a
SPY / QQQ pattern off its King lands on the ladder row nearest its converted price within one SPXW strike
(`gridPatternHost`) — v15.63/64 never drew those at all (LESSONS v15.65). (1) His heatmap claim in the brain (P-heat).
The NOW row in a white hue; the Kings in their own colours — SPX orange · SPY blue · QQQ cyan — rows, chips and strip
cells; the pulse is a brightness pulse now. Mockup approved first: `mockups/mockup-pattern-columns.png`; the render
`design/render-v1565-face.png` (08-28 13:12: a SPY RUG 772/771 and a QQQ PIKA 715–716 the old face hid).
`test_v1565.js` 25 / 10 of 10 mutations. **NOT yet verified live — probe after he installs:** every pattern in its
book's column, a SPY/QQQ block on a non-King row when one exists, the white NOW row, the three King colours on the rows
AND the strip, nothing clipped at 673 px.
**HIS OPEN ITEMS, IN HIS ORDER, NOT YET DISCUSSED ONE AT A TIME:** (3b) PATTERN also to carry GATE (the gatekeeper),
AIR POCKET and "other things you learned going over the skylit documentation" — each needs its Academy definition
named before it is built (the doctrine gate); (4) SWEPT on ES **and NQ** — NQ bars are already couriered hourly
(`NQ=F`, 5-day window): ONH/ONL, PDH/PDL/PDC, prior-day POC/VAH/VAL computable; CW0/PW0 only as the QQQ book scaled to
NQ (a bearing); (5) SWEPT majors only — prior-day POC/VAH/VAL, **prior-week POC** (a nightly product: not in the live
5-day window), ONH/ONL, PDH/PDL, CW0/PW0; drop IBH/IBL and EMH/EML; PDC not named by him — ask. Then v15.66 the
deflection candidate score.

# (earlier the same day) — v15.64: THE SECOND DASHBOARD CONVERSATION. NEW AND THE STACKS RE-CALIBRATED; THE LOST DIRECTORIES RECOVERED.

**Panel is v15.64, companion v1.18 (unchanged).** Built from his eleven corrections of the v15.63 face, given one per
message and built on "okay build". WHAT CHANGED AND WHY (the CHANGELOG has the full reasoning):
· **NEW** = a strike observed BELOW the threshold earlier today, crossed within 20 bars, and GREW (×2 its size at the
  crossing, or +20% of itself over the window). v15.63 called first sight a birth, so every row was NEW. Store
  `gpts_nodeborn_v2` (`{t,mag,pct}` per strike + a below-set); `bornFromSnaps` seeds it from the day file (bar 0 and
  the first bar after a > 10-min gap are never births; absent from the top list = small); live, every context row and
  every tape strike under the cut is a below-observation, and `nodeBornTouch` latches only after one. Chip `NEW 9b ×2.3`
  or `NEW 6b +26%` — the figure that carried it. Constants ⚖ hand-set: `GRID_NEW_BARS` 20 · `GRID_NEW_GROW_X` 2 ·
  `GRID_NEW_GROW_PCT` 20. Calibration `tools/study-gridtells.py` (rule C): 0–2 chips/bar after 10:00 on his four days;
  catches 7775 · 7720 (08-28), 7700 (08-27), 7670 (08-31); CANNOT catch 7755 (09-03), 7705 (08-28), 7665 (08-31) — at
  the tap they sat at 12–18% of a King that had grown (the moving denominator). Recorded in the constants' comment.
· **STACKS** = runs of same-sign MEMBERS on adjacent strikes, a member ≥ 30% of the King (`GRID_STACK_MIN_PCT`; a
  thinner node breaks the run — study S6, the cut applied BEFORE the run is built, NOT S2 "every node in the run"),
  the biggest ≥ 40%; named ONCE on the biggest member (`PIKA STACK` yellow / `BARNEY STACK` purple), the other members
  bracketed (`stk` rows: a coloured left bar + ┃). Median 1 stack/bar (08-31: 2), max 3–5, ≥ 1 on 90–99% of bars. The
  rugs take price's side: RUG needs price under the yellow, REVERSE RUG above it (`gridSetups(nodes,{px})`).
· **The QQQ King draws LIVE**: `gridDisp()` reads `readTrinityHeaders()` when there is no replay frame — `STATE.QQQ.price`
  is never set under the SPX pin, so the third layer had only ever drawn in replay and in the mockup.
· **The face**: ⓪a (READ line + SWEPT) at the TOP, the replay strip at the BOTTOM (`replayBarHtml('warn')` at the top,
  `('strip')` after the column; no-arg = both). READ line = its two facts; the range clause and the timing prose are
  in its hover. SWEPT = names only, grouped `making LOD: POC · VAL · IBL · making HOD: PDH`, the latest sweep's side
  first, colour = state (green reclaimed · red broke · amber testing · dim opened-beyond), time/price/status in each
  name's hover. The tally line OFF the face (counts in each King cell's hover; `tallyLineHtml` kept). King cells: a
  `▲ ROLLED UP` / `▼ ROLLED DOWN` badge above ABOVE/BELOW. NOW row bright with a white bar (wins inside the zone);
  King rows gold with a 2.4 s pulse, gated by the gear's motion switch and `prefers-reduced-motion`; grid columns
  re-balanced from Chromium measurements.
· **The seventh directory (L-Q)**: `archive/v15.53/` and `roadmap/` never reached GitHub (committed in the cloud, never
  in the manifest). Recovered: `tools/recover-archive.py` (155 blocks from git `bbd2f1a`, `INDEX.md` marked a
  reconstruction), the 26 retired tests moved to `archive/v15.53/tests/` and DELETED from his tree by the installer
  (`RETIRED_DEL`), the manifest walks `roadmap/` + `archive/`, `plan-seed.py` writes `roadmap/ROADMAP.md`,
  `PRODUCT-ROADMAP.md` carries a STALE banner + the PARKED block, `tools/splice-seed.py` splices a seed.
**DELIVERY, LEARNED TONIGHT (standing):** from the Claude-in-Chrome side panel the installer card cannot be opened or
downloaded ("This file type cannot be opened"). When the session is linked to his computer, write the SAME
`installvNNNN.bat` into `C:\Dev\gex-signal-tapereader\` with `device_commit_files` (request that one folder first)
and he double-clicks it there — the folder where every earlier installer already sits. The .bat now extracts with a
`for /f skip` copy: **`more +N` stops at 65,535 lines** (v15.64 was the first build past it, 66,123) and sat on
"Extracting payload..." for good (landmine L-S). **INSTALLED AND VERIFIED LIVE 2026-09-04 ~11:55 CT:** GitHub `c61d6df`;
his panel v15.64 — ⓪a at the top, the strip at the bottom, three King rows with the QQQ King drawn live (≈7729), one
NEW chip (`NEW 8b +98%`), three stack names with brackets, the pulse running, no tally text, the SWEPT grouping
(`making LOD POC · VAL · EML · PDC · IBL`), no swallowed render errors.
**Verified**: `test_v1564.js` 84 / 21 of 21 mutations; `test_v1563` re-pinned where the tells changed; smoke clean;
Chromium renders of the shipped script on 2026-09-03 12:48 and 2026-08-28 10:00 (`design/render-v1564-face.png`).
**Verified live** (above). Still to watch over a full session: the NEW count through the day (expect 0–2 per bar),
and the gear's motion switch stopping the pulse. **NEXT (v15.65):** the deflection candidate score (the
L-rules as register features scored by the ledger — the gauge's predict part), the per-book King rows on Testing,
the H5 join. **Doctrine to put in the brain as TOLD claims** (learn-seed): velocity mode "appearing"; core-concepts
"rapid accumulation acts like a magnet"; the 2025-10-09 SPX/SPY/QQQ case study "downside nodes popped up and grew
significantly … floors beginning to grow at 670 … wait for the floor to hit to play the bounce"; the TSLA case — done
this build (`new` factor re-defined in the seed). Open for him: the review's time of day.

# (earlier the same day) — v15.63: THE DASHBOARD CONVERSATION, FEATURE BY FEATURE, SHIPPED.

**Panel is v15.63, companion v1.18.** His decisions are the face now: the node row = NEW · ⇄ ROLL · ▲ GROWTH (% of the
node, no dollars, window 5/15/60 in the gear, under test S6.6) · SETUP per book in the patternpedia's colours (pika
yellow · barney purple · rug red · reverse rug green; stacks by adjacent strikes with mass; rugs adjacent, no floor
within 3 strikes); the three Kings as rows of one KING ZONE + the strip (price · ABOVE/BELOW · GROWTH · ROLLED); the
per-book King tally from the ledger (`kings[]` per tap; counts until n ≥ 15); the READ box untouched; the SWEPT line
(rates in the hover); the DAY table and the taps list OFF the face; MARK/STATE gone; levels in their own column.
`CFG.ladderGrid` / `CFG.dayRead` off = v15.62. Replay-aware. Reference: design/mockup-king-strip.html v3b,
design/render-v1563-face.png (the shipped script on 2026-09-03 12:48). `test_v1563.js` 65 / 14 of 14 mutations.
NEXT (v15.64): the deflection candidate score (the L-rules as register features scored by the ledger — the gauge's
predict part), the per-book King rows on Testing, the H5 join. Open for him: the review's time of day.

**HOW TO ASK HIM TO DO ANYTHING (2026-09-04, standing):** numbered steps — Step 1, Step 2 … — one action per step,
what he should see, the word to send back. Never inside a paragraph. (skills/gex/SKILL.md, STANDING PROJECT RULES.)

**THE END OF DAY (2026-09-03 evening, skill 1a-00e):** his one step is 💾 Save. The desktop bridge is how the files
move: `get_device_info` → `device_request_folder_access` on `C:\Dev\gex-signal-tapereader` (once per session) →
`device_stage_files` the day file → `tools/nightly/run.py` → the review → `SendUserFile` + `device_commit_files` the
log / tables / brain into his repo folder → the "GEX sync" Windows task (every 2 min) pushes → the panel fetches.
The cloud can `git fetch origin` (works) but never push. Done tonight for 2026-09-03 (and the unpushed 09-01, 09-02).
**His ask 2026-09-03:** the KING DEFLECTION is the bread-and-butter setup — S0.1–S0.7 in the registry (per book: SPX,
SPY, QQQ; growth into the tap; the rolled King; ±γ), the `king` factor + L9 in the brain; the ledger's per-book King
rows are the v15.63 measurement. "piku stack" = S7 Pika cloud · cluster.

**THE LEARNING DOC** (v15.62, `learning/deflections/LEARNING.md` — READ IT ON LOAD, skill 1a-00d): his chart screenshots
are lessons. BLIND CALL FIRST, then look the nodes up in the record (`tools/node-lookup.py <day> <strike> hh:mm-hh:mm`;
today's book on his machine is `gpts_nodeevents_v1` via the Chrome tab), then the example + the rule go into
`tools/learn-seed.py` → run → re-splice `LEARN_SEED` → build. The gauge (0–100) cannot flatter: 4 today, all breadth.

---

# ⚠⚠ 2026-09-03 (latest) — v15.62: THE MOCKUPS' LOOK IS THE PANEL'S LOOK · 📚 LEARN.

**Panel is v15.62, companion v1.18.** (1) His third ask for the mockups' look: the Analysis / Testing / Learn tabs now
render in the mockup generators' OWN classes (.subj .hd .sec/.sech/.secb .sc .rs .note table .row .flow .foot) with
their own stylesheet — `tools/panel-css.py` scopes the `.pan…` rules of `mockup-from-studies.py` + the EXTRA of
`mockup-testing.py` under `#gpts-body .g3pan`; `PANEL_CSS` pinned equal; the Analysis skeleton for subject K pinned
equal to the generator's (`test_v1562 2e`); the mockup page's scale control (1× / 1.55× / 2.1×, `gpts_tabscale_v1`) in
the tab's foot; the old "Did the dashboard tell the truth?" header is gone; `ensureV3Css()` runs in `panOpen()` too.
**A change to the look is made in the mockup generator first, then spliced.** (2) 📚 Learn: `tools/learn-seed.py` →
`learning/deflections/examples.json` (the tab, `LEARN_SEED`) + `LEARNING.md` (the doc, read on load, skill 1a-00d) +
`img/E00n.png` (his screenshots, on the installer manifest). Four TAUGHT examples (E001 Sep 3, E002 Aug 31, E003 Aug 28,
E004 Aug 27), each leg checked in the record with `tools/node-lookup.py` (snaps' vend rows + nodeEvents + ES bars);
eight rules: L1 growth into the tap · L2 fresh node at the extreme · L6 stack CONFIRMED; L3 (magnet, unconditioned)
REFUTED by the −γ King of Aug 31 and kept; L8 (+γ magnet) · L4 side flip · L5 −γ accelerates away · L7 time of day
PROPOSED. **The gauge is 4/100** — identify 0 (no blind read yet), predict 0 (the scorer is v15.63), breadth 3.5.
He said: "i think you have enough to start with and analyze for now." Open: which node he meant by "new node just
before 11 am" in E001; what "piku stack" means. **He has not installed since v15.58** — installv1562.bat carries 15.59–62.

# (earlier) — v15.61: THE LADDER FLOOR. "why are there only 3 strikes."

**Panel is v15.61, companion v1.18.** Probed his live panel at 12:40 CT: 100 SPXW strikes, 16 non-zero, exactly THREE at
or above `CFG.nodeThresh` (20): 7750 100% · 7745 45% · 7740 28% (then 11 / 8 / 6 / 5%). The ladder drew the book
faithfully — three bars — and was useless to the eye. **Fix is DISPLAY only:** `LAD_MIN_ROWS=8`; `ladderSubPiles(B,sym,have)`
appends the next-strongest non-zero sub-threshold strikes to `RAILPS_DRAW` (never to `RAILPS`/`emPiles`) as `sub:true`
rows — dimmed outline (`.g3ldbar.sub`), real %King, hover "context row — not a node: not recorded, not scored, no role".
The engine's node set, the recorder, the READ's top-5 and the rolls are unchanged; a zero strike is never filler.
`test_v1561.js` (9; 4 of 4 mutations). Plan re-numbered: v15.62 score the READ · v15.63 the TAP record · … · v15.67 the
pullback outcome. He had NOT yet installed v15.60 when v15.61 was built — installv1561.bat carries both. **He has not
answered the dashboard-inventory question yet** (what to modify / delete / enhance, DASHBOARD-INVENTORY §4) — it is asked
in the chat; the answer goes to `learning/items.json` / the plan.

# (earlier) — v15.60: 📌 OPEN ITEMS (issues, questions) + ENHANCEMENT REQUESTS ON THE ROADMAP.

**Panel is v15.60, companion v1.18.** Six tabs. Issues / questions / enhancements typed on the panel → `gpts_items_v1` →
the day export `items` → `run.py ingest_items` → `learning/items.json` (SEEN) → **the review answers IN THAT FILE**
(status ANSWERED / FIXED / PLANNED as vX / DECLINED, `answer`, `link`, `answeredOn`) → the panel shows the answer under
the item. **A context that loads this project must read `learning/items.json` and `learning/requests.json` and answer
what is SEEN** — that is the review's job (stage ⑤), and it is how his words come back as work. Plan re-numbered:
v15.62 score the READ · v15.63 the TAP record.

# (earlier) — v15.59: THE ⚙ ARCHITECTURE AND 🗺 ROADMAP TABS. THE WHAT, THE HOW AND THE PLAN ARE IN THE APP.

**Panel is v15.59, companion v1.18.** Two new tabs rendered from `learning/plan.json` (`tools/plan-seed.py`; `PLAN_SEED`
pinned equal by `test_v1559.js`, which also pins every stage to PROCESS.md and every version to ROADMAP.md). Architecture =
the objective + the loop as LIVE STATUS (a red stage = where the loop is broken today) + the tabs + the rules + hardening.
Roadmap = NEXT (v15.60 score the READ) · AFTER THAT (v15.61 TAP record · v15.62 nightly reads the queue · v15.63 definitions
+ shipped-artifact test · v15.64 face manifest · v15.65 pullback outcome) · SHIPPED · HIS DECISIONS · CONSTRAINTS. **To
change the plan: edit tools/plan-seed.py, run it, re-splice PLAN_SEED, update ROADMAP.md / PROCESS.md — the test fails
until all three agree.**

# (earlier) — v15.58: THE READ RANKS RECLAIMED SWEEPS FIRST; A GAP OPEN IS NOT A SWEEP. INSTALLER MANIFEST FIXED.

**Panel is v15.58, companion v1.18.** Live probe of his panel found: (1) the installer manifest never carried
learning/*.json, learning/log, data/es-1min/SWEEPS*.json — fixed by glob + `test_installer_manifest.js`; until he runs
the rebuilt installer the Analysis tab shows the seed ("registry not fetched"); (2) the READ put gap-open "breaks" at the
top — now reclaimed → broke → opened beyond; (3) the Testing tab's ①②④⑤⑥ now carry the mockup's bodies (register
columns, gate summary, stores, nightly head, suite stamp from `learning/suite.json`). **Next: v15.59 the TAP record.**

# (earlier) — v15.57 SHIPPED: EM EDGES · VWAP + BANDS · DEVELOPING PROFILE · LONDON · HVL/MAGNET · THE TWO-LINE RULE.

**Panel is v15.57, companion v1.18 (unchanged).** Five level families added to the sweep read (his approval), `LEVEL_TIER`
+ the two-line rule on ⓪a, the corpus at 32 level types / 116 cells. Finding: interior levels (VWAP/bands/today's value
area) are NOT the extreme (3–13% vs 16–28% control) — pullback candidates; P5.1 measures resume. **Next: v15.58 the TAP
record.**

# (earlier) — v15.56 SHIPPED: THE BOOK'S LEVELS IN THE SWEEP READ · COMPANION v1.18.

**Panel is v15.56, companion v1.18.** CW0/PW0/CW/PW and the King join the sweep levels (`bookLevelsNow`, `dispToEs`,
side by position); the READ's node clause checks the sweep against the King / top-5 / walls inside ±0.50 SPY (±5 ES)
and quotes H6's own comparison from the book table (`tools/study-sweeps-book.py` → `data/es-1min/SWEEPS-BOOK.json`,
9 sessions, all thin, grows per export; H6 judged from it, THIN not BLOCKED). The overnight is honest: `overnightHL().full`;
a courier stub is PMH/PML; companion v1.18 fetches ES without the UTC trim so ONH/ONL become the overnight from the next
poll. `RATE_MIN_N=15`. **Next: v15.57 the TAP record.**

# (earlier) — v15.55 SHIPPED: THE ANALYSIS TAB BY SUBJECT · TRACK · THE READ FROM THE STATS.

**Panel is v15.55.** Analysis renders `learning/studies.json` by subject (K S D F P H X; the live sections are the
evidence bodies of H1/F1/F5/D2; H2 carries the sweep table from `data/es-1min/SWEEPS.json`); one TRACK field per
subject (`gpts_requests_v1` → day export `requests` → nightly → `learning/requests.json` → a study row with
`req:<id>` reports back); ⓪a has THE READ FROM THE STATS (today's sweeps on the courier's ES bars against the
sweep table, the node clause from the deflection ledger, the register's word — node-conditioned rate UNMEASURED
until H6); Testing in loop order ⓪–⑥ with the `kill.negGammaWide` FLAG. Register has H6/H7 (nightly-judged).
**The registry is edited in `tools/studies-seed.py`, never in the JSON or the HTML.** F-14 in FINDINGS: the
level's name does not matter; the flush, the clock and the speed do. **Next: v15.56 the TAP record** (95 OPEN
studies and H6 wait on it), then the READ NEXT queue one study per night (K4.1 first).

# (earlier the same day) — THE ANALYSIS TAB IS NOW "BY SUBJECT". DESIGN DONE.

He chose alternative A (by subject) and asked for subcategories "like a trader", an extensive sweeps section, and
the Testing tab redesigned to match. Delivered, all design, no userscript change: **`learning/studies.json`** (the
registry: K KINGS · S SETUPS · D DIRECTION · F DEFLECTION MECHANICS · P PULLBACK DEFLECTIONS · H HOD/LOD · X CONTEXT
— 45 subsections, 170 studies, each with `decides`), **`tools/study-sweeps.py` → `data/es-1min/SWEEPS.json`** (the
sweep study, run: the level's NAME does not matter vs a fresh-low control; the clock, depth and reclaim speed do;
node unmeasured = H6), **`design/ANALYSIS-TESTING-BY-SUBJECT.md`** (the design + v15.55 build list + his decisions),
mockups from the registry (`design/mockup-analysis-by-subject-standalone.html`, `design/mockup-testing-tab-standalone.html`).
**Next: build v15.55** — TAP record · Analysis renders studies.json · Testing renders register/gate/dashboard/record/
nightly/suite by subject · write H6/H7 to register.json · nightly runs study-sweeps.py and one READ NEXT per night ·
ladder sweep line (base rate + n, no node claim). Mockups are generated: edit `tools/studies-seed.py`, not the HTML.

# ⚠⚠ 2026-09-03 — READ THIS BEFORE THE REST. NO CODE SHIPPED; THE PLAN CHANGED.

**Panel is v15.54** — THE WORKFLOW IS CLOSED: design/ARCHITECTURE-E2E-WORKFLOW.md is the spec and the status; the gate (⑤b gates), one register (learning/register.json), the nightly in one command (tools/nightly/run.py, self-tested), verdicts read back into Analysis ④, both tabs in workflow order, four hot readers memoised per frame. Its first real turn is the first session exported on this build. Next: v15.55 merges (accumulation → one, registry 48 → 28). Before that, **v15.53** — THE SIMPLIFICATION, PART 1: 31,063 → 26,830 lines, eight defects fixed (four on ⓪a), 4,233 lines in `archive/v15.53/` with reasons; his decisions recorded in DECISIONS.md (integrations untouched, dark-pool lifecycle archived, window pop-out only, ladder stays on top). Next: v15.54 merges + registry 48→28. Previously **v15.52** (2026-09-03 evening: v15.51 made the grader able to fail; v15.52 made it VISIBLE — Testing ⑤b CAN THE SCORER FAIL, ⑧ PRE-REGISTERED read-once, the gate counts SESSIONS for a to-close feature, ⓪a shows today\u2019s deflection ledger). Next: v15.53 dashboard hierarchy + enrolled TESTING/DEFLECTED/BROKE. Before that, nothing was built today. What happened is that **Q11 was asked of DATA for
the first time**, and the answers moved the build order. Five documents, one study tool and a
pre-registration were added; the userscript was not touched.

## The four findings, in ascending order of how much they matter

**1 · The corpus exists — 11 days of it — and it was never where anyone looked.**
`day.defl` is evicted nightly (localStorage only; there is no `repoUpsertDefl`). But the FEATURES
ledger IS archived to IndexedDB: **8,647 records, 2026-08-19 → 2026-09-02**, incl. `node` 1151,
`reaction` 970, `defl_ant` 669, `defl.trigger` 313, `lodhod` 362. Exported to
`data/corpus/feat-2026-08-19_2026-09-02.json`.

**2 · The touch is a coin, and now two independent samples say so.**
`held 49/94 episodes = 52.1% [42–62%]` against the 8-session hand study's 56% break. `hit` IS the
held/broke label. **⚠ 1,151 rows are 94 EPISODES** — 12.2 rows each, overlapping forward windows.
~40 cells searched, 2 flagged, chance predicts 1.9. **Nothing survived.**

**3 · The label is the wrong SHAPE for the question.**
**62 of 94 episodes carry internally contradictory labels** — same node, same day, held on some bars
and broke on others. `hit` answers *"did price move favourably over the next 10 bars FROM THIS BAR"*,
re-asked every bar. Q11 asks about a **discrete test event**. `day.defl` dedups by strike per fresh
tap and is the right shape — which is why `repoUpsertDefl` is job #1, on better grounds than "the
corpus is small".

**4 · ⚠⚠ THE SELF-IMPROVEMENT LOOP IS COMPLETE AND MEASURES NOTHING.**
`lodhod` — the ONLY surface with a real backtest (284 sessions, **AUC 0.879**) — scored
**362/362 = 100.0%** over 4 live sessions, **including 5 of 5 in the cell where the table predicts
0–19%**. Its scorer asks whether price travelled **the whole session range in 30 minutes**. It never
does. Corrected to the right threshold it reads 97.5% and is **still flat across every cell** —
because **a bar-level scorer cannot test a session-level claim at any threshold.**

    claim OK · record OK · score BROKEN · aggregate reads the vacuum · gate clears a tautology
    · transport DOWN (53 commits unpushed, 403) · promote never legitimately fired

See `roadmap/FINDINGS-the-loop-measures-nothing.md`.

## ⚠ THREE CORRECTIONS I MADE IN ONE DAY — do not repeat them

    "months of labelled deflections may exist"   -> none did; eviction deletes past days first
    "so the corpus does not exist"               -> it did, 11 days, under a DIFFERENT KEY
    "7,500 records are being discarded"          -> correct DECLINES of non-events (all 2,706 checked)

**Every one was stated before the one-command check that refuted it.** And `study-nodeatextreme.py`'s
own header records a PAST context making the same class of error — reading `tri.<SYM>.king`, one
crown, and concluding "no node was there" from a one-line view of a full book. **I then did the same
thing by analysing `feat` alone and never opening `snaps`.**

## ⚠ WHAT I HAD NOT USED (and a new context should)

`snaps` in IndexedDB carries **`tri.<SYM>.top`, the full RANKED node book for SPY/QQQ/SPXW/VIX, every
bar**. Also untouched: `gpts_nodeevents_v1` (3.2 MB), the four king-tracking stores, `gpts_promo_v1`,
`gpts_nodehist_v1`, `gpts_peak_v1`. **`tools/` already holds ~40 `study-*` scripts** — among them
`study-nodeatextreme.py` and `study-kingdeflect.py`, which already ask "was the extreme at a node"
and "did a king cause the deflection". **Check `tools/` before proposing an analysis as new.**

## What was added today

    design/spec-v16-dashboard-deflection.md      the dashboard build spec (DRAFT, he approved the look)
    design/mockup-v16-dashboard.html             true-608px mockup on his real 2026-09-02 ladder
    roadmap/INTEGRATION-what-we-already-have.md  HLTAB is 2/3 of Q11; sweeps were measured in OPEN AIR
    roadmap/FINDINGS-Q11-first-corpus.md         the analysis, incl. the retraction in §7
    roadmap/FINDINGS-the-loop-measures-nothing.md  finding 4 above
    roadmap/PREREGISTER.md                       H1-H5 fixed BEFORE the data that tests them
    tools/study-corpus-episodes.py               episode clustering + Wilson + a multiple-comparison ledger

## Next, in order

1. **`repoUpsertDefl`** — event-level persistence. **H5 is blocked until it accumulates.**
2. **Move the `lodhod` scorer to the CLOSE** — `lod/hod/lodT/hodT` are already recorded per bar; the
   true first-printed extreme is computable in-panel at 15:00 without the nightly.
3. **One ATR geometry** — retire `LVL_INPLAY_PTS=3` and `reactDefence`'s `bd>3`. ⚠ Both are fixed
   constants in **DISPLAY space** (verified: `tradeNodes().es` is display, every caller agrees), so
   both take `atr(sym) × scaleUsed × DEFL_NEAR`. This fills the empty MARK column.
4. **Then** the dashboard from `spec-v16` — with the rule that spec did NOT have:
   **⚠⚠ nothing goes on the face unless a test that COULD HAVE FAILED says it is true.**

⚠ **And the dashboard hierarchy in the mockup is BACKWARDS.** It leads with the deflection ladder
(measured null) and buries ⓪ a (AUC 0.879, the only validated edge, and literally his stated
objective). **The HOD/LOD verdict belongs on top; the ladder supports it until H5 pays.**


---

## 0 · WHY THIS APPLICATION EXISTS — read `design/PURPOSE.md` BEFORE ANYTHING ELSE

**Find the day's HOD and LOD early enough to trade the move between them.** Secondarily, find the
**pullback** turning points that resume a trend rather than end it. (Operator, 2026-09-02, in full
and in his own words in `design/PURPOSE.md`; pinned by `test_purpose.js`.)

**The mechanism he is trading:** a **gamma node deflects price, and the deflection IS the turning
point** — either a trend reversal at the extreme (the HOD/LOD) or a pullback reversal that leads to
**continuation**. ⚠ **Confusing those two is the expensive error: they call for opposite trades.**

    ⓪a HOD/LOD          MEASURES the day — the turning points and their base rates
    the node ladder     watches the king, which ATTRACTS price as well as DEFLECTS it
    ⇄ · Δ15m · STATE · ROC   tracks gamma MOVEMENT: where · how much · what it means · as a rate

**Why the right-hand columns exist:** gamma **building** on a pullback may be the node that causes
the reversal and the continuation; gamma building at a high or low may be the node that causes the
HOD/LOD deflection. They answer **"is a deflection being built right now, and where?"** — the
leading signal for the turning points above.

⚠ **JUDGE EVERY PROPOSAL, PRIORITY AND CUT AGAINST THAT FILE.** If the roadmap, DECISIONS or
DEPENDENCIES conflict with it, **PURPOSE wins and the other is wrong.** ⚠ And the mechanism is a
**hypothesis he is trading, not a proven law** — keep it checkable (base rates, A over E), never
assert it.

---

## ⚠ 0 · THE CARRY-FORWARD CONTRACT

**This note is rewritten IN FULL every build. Anything not re-typed is GONE, silently.**

⚠⚠ **AND IT FAILED EXACTLY THAT WAY AGAIN.** The previous note carried `v15.09` in its header —
which is all `test_savedone` checks — while its body still described **v14.80**. Nine versions never
reached it and every guard stayed green. Snapshot kept as `session-state/2026-08-31_resume-v14.80.md`
so the failure is legible. **A version-keyed guard is satisfied by a stamp, not by content: when you
bump the header, rewrite the body in the same edit.**

Read in this order, in full, before anything else:

0. ⚠⚠ **`session-state/LESSONS.md`** — the failure-pattern register, then the per-build log newest
   first. It names results that have been **WITHDRAWN**; quoting one is the most expensive thing a
   fresh context can do, because it looks like knowledge.
1. **`session-state/CHAT-HISTORY.md`**, the CURRENT-CONTEXT entry — what was *said*.
2. **`session-state/LOCKED-ITEMS.md`** — agreed-but-unbuilt work. Check every build.
3. **`session-state/OPEN-QUESTIONS.md`** — so you do not re-ask what he already answered.
4. **`design/DATA-ARCHITECTURE.md`** — who can reach what.
5. **`skylit-docs/FINDINGS.md`** — F-1…F-16.

**THE LOAD CLONES FULL** (never `--depth 1`).
⚠⚠ **A COMMIT IS NOT A PUSH.** The cloud gets a 403 from the git proxy. `installvNNNN.bat`, run on
his machine, is the only route to GitHub — and the git record confirms every release on origin was
committed and pushed by **him**, never by a session.

He works **one item at a time** and expects you to **discuss before building**. He has caught more
real defects than the test suite has. **When he pushes back, he is usually right.**

---

## 1 · THE STANDING BUSINESS REQUIREMENT (his words)

> "I am a trader and need to know where to take trades from and where price is going, so basically I
> need to know potential support and resistance especially if it is weakening and new support and
> resistance is forming as well as where price is going."

And the frame for ⓪a, which he had to tell me and which reorganised the whole section (2026-08-30):

> "do you realize that i am taking the model of the daily bar and trying to measure the movements in
> it from open to close"

> "so the daily candle and its contstruction will be my mental model for daytrading using all of
> these measurements … its very important that this feature be a world class feature but it will
> require your help in constant refinement via the use of llm to identify additional datapoints and
> measurements to better prediction"

**What he trades, stated 2026-08-30 and now wired that way:**

    STRUCTURE    nodes · kings · walls · flip      SPXW / SPY / QQQ.  ES has no book of its own.
    MEASUREMENT  HOD · LOD · candle · EFF · GD/RD  ES's own 1-minute bars.  (`measureBars`)

> "its the es that i am trading but using spxw nodes" · "we are using other markets to get things
> like their kings because ES doesn't have its own book, so we use other tapes"

---

## 2 · WHERE WE ARE — v15.50, and what the face carries

**Panel v15.50 · companion v1.17.** Suite **142 green / 6 baseline red** (`expiry_profile`,
`node_map`, `sma_cont`, `tapeking` (needs jsdom), `trendbadge`, `v1126_process`).

### ⚠⚠ THE REPLAY SLIDER IS THE NEW THING, AND IT HAS NOT BEEN SEEN LIVE YET

A 30px strip under the tabs: day stepper, a track whose ticks are the frames that actually exist,
the clock, and a LIVE / ⟲ REPLAY badge. **Dragging it rewinds the whole panel** — ladder, kings,
nodes, the frame and ⓪a all read the book recorded at that minute. ◀ ▶ step the day, so a Saturday
reaches Friday.

**It is the existing stale-book path with a different source, not a second renderer.** `tapeMap(sym)`
has served a stored book since v14.55; replay points that branch at any frame in
`gpts_repo_v1.snaps` — **2,149 SPY frames over 18 days**, back to 2026-08-11, which is NOT bounded by
the 3,600 KB localStorage budget.

    tapeMap        replay -> the frame's book · stale -> the latch · else -> live
    measureBars    replay -> bars built FROM THE FRAMES, truncated at the parked one
    recorderBlind  replay -> TRUE.  Nine write paths inherit it. This is the D-10 guard.

**Four rules it is built on, and none may be quietly relaxed:**
1. a vendor row belongs to the book whose KING it sits nearest **in log space** — SPY 767 and QQQ 716
   are seven percent apart and no magnitude rule splits them;
2. **refuse, never fall through** — an empty book beats live numbers under a REPLAY badge;
3. **replay never writes**;
4. the handle **snaps to a frame that exists** — 13:01 gives you the 13:00 book, labelled 13:00.

⚠ **THE PER-BAR OPEN IS RECONSTRUCTED** (previous close); high/low/close are recorded and exact.
`approxOpen` carries it, so WICK%, BODY and the GREEN/RED call inherit the approximation knowingly.
⚠ **A replayed ladder is as deep as the frame was stored.** `VEND_MAX_ROWS` went 40 → 90 for that
reason, but it **cannot enrich the 18 days already recorded** — at 40 rows those hold ~19 SPXW
strikes, down to 4% of King, which covers everything the ladder draws and not the grey minors.

**v15.11 fixed the two things his first drag exposed.** All three crowns now come from the frame
(`ladderKings` was reading TODAY's latch and TODAY's `LASTFEED` — mislabelling, not thinness), and
the whole accumulation layer replays through `slicesFor()`, so BUILDING/STEADY/FADING, the day peak
and the DEFENDING/ABANDONING marks come from the REAL rule fed a sequence rebuilt from the frames.

⚠ **STILL CANNOT REPLAY, and both are stated on purpose:** the **roll arrows** (`ROLL_LATCH` is a
stateful RTH accumulator, not in a frame — replaying it is its own build), and the **gamma profile**,
which is not on the live face either (removed v14.81 at his request; do not reinstate it under cover
of "make replay like live").

⚠⚠ **v15.12 — THE EM PIN IS PER CHART FAMILY, AND THAT IS WHY ES WORKS AGAIN.** His pin was captured
on the SPY chart (`rr:1`, `em:3.49` in SPY points) and on an ES chart was judged against an ES-scale
floor of 7.7, healed away as implausible, and fell back to an expired $1.70 straddle — so the band
refused and **the ladder, which lives inside that section, went with it.** The record is now keyed
`sym|cash` / `sym|fut`, and a family with no pin SEEDS from the other using `emK`, the straddle in
the BOOK's own points. ⚠ Pins written before v15.12 have no `emK` and use the ratio rescale, which
is flagged `seedApprox`. **A stored value in DISPLAY units is a trap whenever the display can change.**

⚠⚠ **v15.13 — THE LADDER WAS 105px WIDER THAN HIS PANEL AND HE HAD NEVER SEEN THE RIGHT EDGE.**
`.g3ladwrap` measured 640 against 535 with scrollLeft 0, and the roll lane is at x 620-640 — so the
arrows shipped at v15.09 into the one strip he could not see, with the ROC column and most node bars
beside them. `ladderFit()` now grows the panel by exactly the overflow. ⚠ **The width was logged as
"his call" since v14.54; that applies to WHICH COLUMNS MATTER, never to whether the panel can show
the columns that exist.** The arrows also REPLAY now, by re-running the live `rollScan` over the
frames, and `rollsLive()`/`velOk()` no longer exclude a replayed bar.

⚠⚠ **v15.14 — THE KING LANE NOW DRAWS THE CROWN'S JOURNEY.** Its renderer was always complete; it
had nothing to draw. The track recorded MIGRATIONS but never an ORIGIN, so a crown holding one strike
all day gave an empty array and the "no migration recorded" placeholder — **an empty series and a
series with one long run mean opposite things.** The first observation is seeded now, and in replay
the journey is rebuilt from `tri.<book>.king` under the same `KT_DWELL` rule. Measured 2026-08-31:
**8 SPXW migrations, 10 SPY.** ⚠ That is the RECORDED trinity crown at dwell 2 — a DIFFERENT
instrument from the latched crown measured at "SPXW 0 durable moves" on 08-28 over a truncated
window. Neither number refutes the other; say which one you mean.

⚠⚠ **v15.15 — THE SEAM LESSON, AND IT IS THE MOST IMPORTANT THING IN THIS NOTE.** Every defect from
v15.10 to v15.15 was the same shape: a consumer reaching around a replayed seam to a LIVE source.
`tapeMap`, `ladderKings`, `slicesFor`, `velOk`, `rollsLive`, `ktOf`, `closedCandles`, `tapeSync`.
The last one cost four symptoms at once — `emBand` read LIVE candles while the nodes came from the
frame, and `emPiles` clips piles to that band, so a 13:12 book against a 21:00 band left ONE pile:
one node bar, no states, nothing for `rollScan`. **"I swapped the source" is not a finished thought
until every reader of that source is enumerated.** Before claiming replay works: grep the render path
for `LASTFEED`, `STATE[`, `VEL`, `ctTodayStr` and the latch keys.
⚠ Also: a missing `k` on the replayed velocity rows made `rollScan` compare `undefined===undefined`
and discard EVERY roll as "the same strike" — 2,406 sightings, zero drawn, nothing thrown.

⚠ **STILL LIVE-SOURCED IN REPLAY: the ladder's LEVELS.** `ifLadder` reads `ifChain` (the live IF
payload); the NODES replay, the LEVELS do not. Frames store `lev` and `deriv`, so it is buildable.

⚠⚠ **v15.16 — THE SINGLE MOST IMPORTANT THING TO UNDERSTAND ABOUT REPLAY.** `SK_MIN_STRIKES = 20`
is a LIVE-PARSE health floor ("below 20 the DOM changed"). His recorded frames hold a MEDIAN OF 17
SPXW strikes, so it refused **120 of 129 bars**, and a `skPiles` refusal returns NO PILES — no nodes,
no statuses, nothing for `rollScan`, one strike at 100%. Every symptom he reported, from one constant.
`SK_MIN_STRIKES_REPLAY = 5` now judges a recorded book; the live floor stays 20.

⚠⚠ **AND THE PATTERN BEHIND EVERY REPLAY BUG IN v15.10-v15.16:** `rollsLive` (RTH-only), `velOk`
(live harvest), `tapeSync` (live votes), `closedCandles` (live candles), `SK_MIN_STRIKES` (live-parse
health). **Every threshold carries an implicit claim about where its input came from, and replay
changed the provenance without changing the thresholds.** Before claiming any replayed surface works:
list every early return, floor and freshness check between the source and the pixels, and ask of each
what it assumes. Skipping that read cost six builds.

⚠⚠ **STILL LIVE IN A REPLAYED FACE — AND MEASURED, SO NOBODY PROMISES WHAT CANNOT BE DELIVERED.**
Traced 2026-09-01 against a real frame; **neither is fully recoverable from the days already stored:**

    dispScale   ⚠ WITHDRAWN AT v15.19 — THIS WAS WRONG. The basis IS in every frame: px / xm.SPXW.px
                (764.86 / 7677.55 = 0.099775). `ifLadder` reads it from the frame in replay now. The
                stated evidence ("a frame has no ES price") was true and did not bear on the claim.
    the LEVELS  the frame's `lev` holds cr/cr0/ps/ps0/mag at SPY SCALE (767/765) — the SPY book's
                walls, NOT the SPX chain rows the ladder draws (PDH, CW0, FLIP…). Those come from
                ifLadder.rows + sessionLevels and are not in a frame at all.

**So a replayed LEVEL set is not obtainable from the 18 recorded days.** Recording `ifLadder.rows`
per frame would fix it going forward. ⚠ Do not tell him the levels will replay on old days.

⚠⚠ **v15.17 — TWO MORE, AND BOTH WERE FOUND BY MEASURING RATHER THAN GUESSING.**
**The arrows were TRUE and wrong to show:** four genuine roll pairs between strikes the ladder does
not draw (7625->7650 on an $82K shed) while the KING's own 7675->7670 at $22.4M was missing. The live
latch scans `tradeNodes()`; replay scanned every stored strike. **Reusing a function is not
reproducing the call — match the INPUT UNIVERSE too.**
**"Cannot scroll" was not a scroll bug:** panel 1016px in a 557px window, top -307, and
`body.scrollHeight === clientHeight`. The content fits the panel; the panel does not fit the screen.
`panelFit()` clamps it. Third costume of the v12.2/v12.5 lesson.

⚠⚠⚠ **v15.50 — `roadmap/DEFLECTION-ROADMAP.md` IS NOW THE BUILD ORDER.** `load gex` step 1a-01.
21 items across Dashboard / Analysis / Testing + the foundation, pinned by `test_roadmap.js` (35).
⚠⚠ **ITS HEADLINE: THE TOUCH ITSELF HAS NO EDGE.** 79 deflections / 25 breaks, **56% break**,
mirror-image excursions (+0.92/+0.26 vs +0.29/+0.86), **t=+0.41** top-5 and **t=−0.32** kings —
BOTH NULL. The panel can say a node was TESTED; it cannot say which way it resolves, **and that is
the entire trade**. `OPEN-QUESTIONS` **Q11** is the product and IS NOT BUILT.
⚠ **EVERY DASHBOARD FEATURE IS COSMETIC UNTIL Q11 HAS AN ANSWER.**
⚠⚠ **MEASURED ON THE TESTING TAB: `ANSWERED (0)` · `TESTING (83)`** across 21 families — the loop has
never resolved one question, and G5's families (`node.tap.*`, `node.pol.*`, `drift.conf`, `kill.*`)
are **unanswerable by construction** (n=0 always), not pending.
⚠⚠ **Q11 IS BLOCKED ON HIM, NOT ON ENGINEERING:** precision is unmeasurable until **one session is
labelled exhaustively** (recall is 100% of checkable marks), and the **re-arm distance** is his
judgement. ⚠ Do not tune it to make a number look good.
⚠⚠⚠ **DO NOT BUILD THE LIVE DEFLECTION SURFACE FIRST** — a confident face over a t=0.41 signal is
exactly what `PURPOSE §4.4` forbids. **The one exception is the MARK fix** (correctness:
`LVL_INPLAY_PTS=3` is fixed chart-points while deflection is ATR-scaled).

⚠⚠ **v15.49 — "THE COLUMNS ARE MISSING" WAS THE HEADER ROW SCROLLING OFF THE TOP.** All eleven
headers existed, were positioned right and were inside the horizontal view; `scrollTop 15.15`,
`header.topRel -15`, `insideView FALSE`. The header was `position:relative` inside a box that
**v15.28 made OPEN on the expected-move band** rather than at the top — so it is scrolled from the
first render on most days. ✅ Now `position:sticky;top:0` + `z-index` + an **opaque** background
(without the last two the rows scroll THROUGH it, which is worse than the original fault).
⚠ **CHECK WHAT IS VISIBLE BEFORE CHECKING WHAT EXISTS** — I probed horizontal scroll first because
that was my model of "missing columns". The answer was vertical.
⚠⚠ **THE MARK COLUMN IS GENUINELY EMPTY AND WAS UNDIAGNOSABLE.** Zero marks with a row **0.5 points**
from price and `LVL_INPLAY_PTS=3`; the same code marks five rows in the harness. The catch discarded
BOTH a null return and a throw. ✅ Now `swallow('levelMarker')` + `__gptsDebug.mark()` records
strike, disp, now, distance, threshold and verdict per row, reset each render. **READ IT NEXT
RELOAD — do not guess a third time.**
⚠⚠⚠ **MEASURED, NOT CHANGED: `LVL_INPLAY_PTS=3` IS SCALE-DEPENDENT.** 3 SPY points = THREE strike
gaps (harness marks 5 rows at once); 3 ES points = under HALF a gap. **One constant, two entirely
different tests.** Not touched — it redefines what IN PLAY means, which is HIS call.

⚠⚠⚠ **v15.48 — `sessionPhase(now)` TAKES A **DATE** AND I PASSED IT SECONDS, THREE TIMES.**
It does `new Date(now.toLocaleString(...))`. A NUMBER has `toLocaleString`, so `48000` → `"48,000"`
→ **Invalid Date** → every field NaN, **and nothing threw**. Measured 13:30 CT mid-RTH:
`deps.rthNow FALSE`, `idleMin null`.
⚠⚠ **SO BOTH GUARDS WERE INERT — INCLUDING THE ONE THAT MATTERED: the v15.45 replay stale-day guard
had NEVER ONCE FIRED.** The protection written the day he lost a morning of recording would not have
saved the next morning. ✅ One `liveSessionPhase()` = `sessionPhase(new Date())` serves all three.
⚠ **`new Date()` IS the wall clock** — no `ctOffsetSec`, no `%86400` arithmetic. Never rebuild it.
⚠⚠ **THE STUBS WERE KINDER THAN THE REAL FUNCTION — SECOND BUILD RUNNING** (v15.46 was the first).
They now THROW on a non-Date. **A double that accepts what the original refuses tests the double.**
⚠⚠ **THE PROOF IT WORKS WAS 65 FAILING ASSERTIONS** — the moment the clock was fixed the guard began
evicting both render harnesses, which park a past day on purpose. They now SATISFY the guard by
setting `RP_STALEGUARD` to today (the same state a real panel reaches), never patch it out.
⚠ **v15.47's RTH cut was "keep what I can verify" and deleted every CLOCKLESS bar**, emptying whole
series (the v15.24 blackout). Now "drop what I can **refute**" — only provably pre-open bars go. The
anchor guard still refuses a clockless bar; it is simply not ERASED on the way there.
⚠⚠⚠ **2026-09-02's EM BAND IS UNRECOVERABLE AND IT IS MY FAULT.** `capMin 299` — pinned at 13:29,
`em 9.66` against a 48.25-point day, price 22 above the expected high. My v15.46/47 faults stopped it
pinning at 08:30; by 13:29 the 0DTE straddle had decayed, so the pin is the REMAINDER, not the
EXPECTATION. Flagged `est`/`over` on the face. **Nothing recorded the open's straddle — it is gone.**
Tomorrow's is correct from the first bar.

⚠⚠⚠ **v15.47 — THE BAND'S SERIES BEGAN BEFORE THE OPEN, SO THE ANCHOR WAS NEVER THE OPEN.**
v15.46 fixed the units and the band STILL refused. Measured 13:20 CT: `MB.day '2026-9-2'` passed,
but **`cs[0].so 28800 = 08:00 CT, o 7640`** — the ES courier's window opens at 08:00 (`FUT_WIN_A`)
and `measureBars`' futures branch buckets by DAY **without cutting to RTH**. The first RTH bar was
index 30, 08:30, **o 7650.5**.
⚠⚠ **THE GUARD WAS RIGHT TO REFUSE.** `out.anchor` and `rec.openU` both read `cs[0]`, so relaxing
the check would have anchored the day on **7640 instead of 7650.5 — 10.5 points low, silently.**
✅ **CUT THE SERIES TO RTH WHERE IT IS BUILT**, so `cs[0]` IS the open and every reader (anchor,
openU, openSo, hiWater/loWater) shares one definition of the session. `emBand` now agrees with
`hodLod`, which had filtered `b.so<openSec` all along. Idempotent on the cash and replay paths;
pre-open it empties `cs`, which correctly falls to the prior-close anchor (v11.50).
⚠ **TWO INDEPENDENT FAULTS PRODUCED ONE MESSAGE** and I shipped a fix for the first. **RE-MEASURE
AFTER SHIPPING** — "it should work now" is not a measurement.
⚠ **ONLY A BEHAVIOURAL ASSERTION CAUGHT IT**: removing the cut survives every grep, because the line
is still there. `test_em_band` w4 runs a courier-shaped series and reads the anchor.
⚠⚠ **FOUND, NOT FIXED: `deps.rthNow` reported FALSE at 13:20 CT with `idleMin:null`** — the v15.43
wall-clock helper is not resolving the live phase, so the session-aware staleness is NOT engaging.
Harmless today (idle 0 grades as before). **Fix next.**

⚠⚠⚠ **v15.46 — THE WARM-UP GUARD READ `t` IN THE WRONG UNITS AND REFUSED THE BAND ALL DAY.**
He reloaded mid-session: *"nothing displayed, no ladder"*. `emBand.ok FALSE`, why *"warm-up: candle
window or ratio is not yet today's"* — **zero ladder rows**, with EVERY input healthy (FUTMODE.live,
futBars 1m old, tape 100 strikes, no render errors, `sessionBody` reading today correctly).
⚠⚠ **TWO PRODUCERS DISAGREE ABOUT WHAT `t` MEANS:** `closedCandles` stores `t: realMs` but passes
NAIVE SECONDS to `naiveDayStr`; `measureBars/ES` stores `t: r[0]*1000` (REAL ms). `naiveDayStr`
multiplies by 1000, so on a futures chart it got ms and returned a **year in the fifty-eight
thousands** — the comparison was STRUCTURALLY UNSATISFIABLE and `capOK` was permanently false.
⚠ **IT ONLY FIRES ON A FRESH CAPTURE** — a carried-over pin skips the branch — so it hid from v15.23
until the first new-day capture, then took the panel out on the first render.
⚠⚠⚠ **AND THE HARNESS WAS COVERING FOR IT:** `test_em_band` stubbed `naiveDayStr(ms)` while the real
one takes SECONDS. **A STUB KINDER THAN THE FUNCTION IT DOUBLES IS A COVER-UP** — 648 green
assertions on a panel that would not draw. The stub now matches; fixtures carry `day`.
✅ **ASK THE PRODUCER, NEVER RE-DERIVE.** No conversion works at the call site (right for one branch,
wrong for the other, and wrong for cash only after 19:00 CT when the UTC day rolls). `measureBars`
NAMES the day it selected; each bar carries the day it was kept for; compared NUMERICALLY because
formats differ (`2026-9-2` vs `2026-09-02`). No day named ⇒ decline to judge, never block.
⚠ The guard still refuses a stale ES day (the branch takes the LAST day present — v15.23's real
reason), a first bar before 08:30, and a bar with no clock.

⚠⚠⚠ **v15.45 — A REPLAY PARKED ON YESTERDAY SAT THROUGH FOUR HOURS OF A LIVE SESSION AND THE PANEL
RECORDED NOTHING.** He asked, mid-morning: *"are you recording .. market is open"*. Measured: strip
`◀ Tue 1 Sep ▶ … 13:57 ↺ REPLAY` while the real clock was Wednesday 12:54 CT, RTH — and the store
held **ZERO frames for 2026-09-02**. `recorderBlind()` gates all nine write paths in replay, so the
whole morning was never captured. ⚠ **THE BADGE SAID WHICH MODE; IT NEVER SAID THE COST.**
⚠ **NOT persisted state** — `REPLAY` is in memory, so a reload would have cleared it. The tab simply
never closed. **A SESSION-BOUNDARY bug, not a storage one.**
✅ `replayStaleDayGuard()` — a replay of a **PREVIOUS** day, still open once a **live RTH** session
has begun, hands itself back **ONCE** and says so. Rewinding **TODAY** is deliberate and untouched.
✅ A red **⚠ NOT RECORDING** banner whenever replay is on during live RTH, naming the session being
missed, and **the banner itself is the click target** that returns to live.
⚠⚠ **WALL CLOCK, NOT `sessionPhase()`** — parked on yesterday it returns `rth:true` for a session
that ended, so the guard would have agreed with the state it exists to detect. Third build running
that this trap appeared (v15.43 deps, v15.45 guard ×2).
📏 **Recovered live and verified:** frames 0 → 1 → 2 at 12:55 / 12:57, 90 vend rows each.

⚠⚠ **v15.44 — THE LADDER COLUMNS RIGHT OF `NOW` ARE ONE NARRATIVE, IN HIS WORDS (2026-09-02):**
*"the arrow column shows the movement of gamma rolling from one strike to another, the delta profile
shows how much gamma is moving, the state says it in words by classifying it, the roc gives you a
percentage."* **WHERE → HOW MUCH → WHAT IT MEANS → AS A RATE.** Do not reorder these without him.
    S@2 · Y@28 · LEVEL@56 · PRICE@104 · NODE·%KING@140 · NOW@226 · MARK@294
      · Δ15m@400 · ⇄@452 · STATE@500 · ROC 15m@558          LAD_W 618 → 608
⚠ **OPEN QUESTION, ASKED NOT ASSUMED:** he said "move the roll arrows to the RIGHT of the delta
profile" and then described the group with the ARROW FIRST. The build follows the explicit spatial
instruction (⇄ right of Δ15m); if he meant the narrative order, it is two constants.
✅ **THE ROLL WORDS COLUMN IS RETIRED** — "the arrows are suppose to show the roll, the from node
(little circle) and the to node (arrow head)." It was ONE FACT TOLD TWICE, 32px apart.
⚠ **NOTHING LEFT THE RECORD:** every sentence the chip's hover carried (the mass rule, the distance
cap, the receive ratio, "not conservation of mass", "does not say price will go there") moved to the
lane's hover, and `test_replay_face` q5 fails the build if any goes missing.
✅ Lane 20px → **44px** (the width v15.09's sketch asked for, squeezed only to fit the 640 cap).
⚠⚠ **THE Δ BARS HANG *LEFT* FROM THEIR AXIS.** `LAD_DAX` is the axis, `DAX-DMAX` is where the ink
starts. My first cut placed it at 344 and a full-width negative bar reached 288, inside the chute at
292 — `test_ladder` g4 caught it. **A column that grows toward its neighbour cannot be placed by its
own left edge.**
✅ New guard `g1b`: every ladder constant must be **FINITE** — `v()` on a deleted constant returns
NaN and comparisons against NaN pass silently, so a removed column can keep certifying its own layout.

⚠⚠⚠ **v15.43 — THE DEPS DOT WAS RED ~17 HOURS A DAY.** At 04:52 CT: `if.SPX 386m · if.QQQ 385m ·
fut.courier 426m · irt.export 294m` all STALE and `if.usable` FAIL "running blind" — **every one the
correct overnight state.** ⚠ `if.usable`'s OWN comment forbids exactly this ("would cry wolf every
session"); it guarded the SPX pin and never the clock.
✅ **THE RULE IS "WAS IT FRESH WHEN THE SESSION ENDED"** — staleness measured back from the CLOSE.
A feed last seen 14:59 is healthy at 04:52; one last seen 10:00 still FAILS and still shows its TRUE
age. **The age is never hidden; only the clock it is judged against changes.**
⚠⚠⚠ **NEVER CALL BARE `sessionPhase()` FOR ANYTHING ABOUT THE LIVE WORLD** — it is REPLAY-AWARE by
design (v15.18). Measured while parked at 14:06 it returned `rth:true · POWER HOUR` at a real
04:52. `depsSessionIdleMin()` builds a second-of-day from `Date.now()` and passes it EXPLICITLY.
⚠ Pre-open it walks back to the previous TRADING day (Monday counts the weekend). **Holidays are not
modelled** — errs toward noise, never silence.
✅ **CONFIRMED LIVE THIS SWEEP** (all previously unverified): replayed `dispScale 1.0019` (not
0.0998) on a real futures chart — closes the v15.40 unknown; EM band all one ruler (v15.41 holding);
roll lane 11 arrows; king runs 2.1–10.2px; `1ST TP · HOD` / `2ND TP · LOD`; both candles red from one
call; zero render errors.
⚠ **RAISED, NOT FIXED:** a king run priced outside the drawn frame is SILENTLY dropped — the ladder
names what it drops on its "off frame" line, the king lane has no equivalent.

⚠⚠⚠ **v15.42 — THE KING LANE WAS NEVER EMPTY; IT WAS ONE PIXEL WIDE.** The axis ran open → WALL
CLOCK, so at 19:50 the crown merely *still there* took **57% of a 24px lane** and five real
migrations drew at **1.0–2.5px**. Measured widths `1.0 · 2.5 · 1.0 · 2.5 · 2.4 · 11.4`.
⚠⚠ **A DEFECT PROPORTIONAL TO ELAPSED TIME IS INVISIBLE WHEN YOU BUILD IT AND OBVIOUS IN THE
EVENING.** Test at more than one hour — the test checks 15:00, 19:50 AND midnight.
⚠ **WHEN HE SAYS "EMPTY", MEASURE THE ELEMENT GEOMETRY BEFORE THE DATA.** I went to the data first,
twice. The lanes were drawn, populated and correct — and unreadable.
✅ The axis now ends at the **last closed bar** (data, not clock arithmetic); shrinks only, only once
RTH is over. Long runs gain ~1.7×; the "still there" run drops 57% → 19%.
⚠ **RESIDUAL, NOT FIXED:** a 15-minute run out of 390 is still sub-pixel in 24px. Floored at 1px so
it is faint, never absent.
⚠⚠ **THE ROLL LANE: `rollsLive()` ASKED `rth` WHILE THE REST OF THE FACE ASKED `showingStaleBook()`.**
After the close the panel serves the close-of-session book — nodes, states, ROC — and blanked that
same book's ROLLS. **One face, two opinions about which session is on screen.** Now `rollsLive()`
also returns true for the frozen book; `rollLatched()` still refuses live arrows over a replayed bar.
✅ **AN EMPTY ROLL LANE NOW NAMES ITS SILENCE** — four distinct cases, four sentences. `return ''`
had made "no rolls today", "retired at the close" and "the latch is empty" identical.

⚠⚠⚠ **v15.41 — A REPLAY PIN OUTLIVED THE REPLAY AND FLATTENED THE LADDER.** Measured LIVE after he
rewound: pin `SPY|fut = {openU:761.79, rr:1, fam:'replay', replay:true}` → band `low 729.29 /
high 794.29` (SPY) against `now 7647.50 / hiWater 7673.75` (ES). `emRailBounds` STARTS the frame at
`B.low`, so the rail spanned **6,944 points** and every row collapsed onto one line.
⚠⚠ **ONE UNGUARDED WRITE, FOUR OVER-GUARDED READS.** `replayEmPin()` (v15.24) builds the pin; four
heals refuse to repair a `replay:true` pin (v15.26, correct but UNSCOPED); and the **ratio heal**
(v14.19) does `S.sym[emKey]=rec` with **no replay guard at all** — it PERSISTED the replay pin into
the LIVE key where nothing could repair it. Each rule individually correct; **none asked whether the
replay was still happening.**
✅ **`rpPin(r) = r.replay && replayOn()`** — *exempt RIGHT NOW*, not *born in replay*. **All FIVE**
sites call it, the write included. ⚠ **When one condition is restated at five call sites, the bug is
the five.**
✅ **AND A GUARD THAT TRUSTS NO FIELD:** an anchor and a price on one chart cannot be a factor of
two apart. Overrides every exemption, disclosed as `rulerOff`. **v11.65, v15.12, v15.24, v15.26 and
this one would all have tripped it.**
⚠ **COMPARE THE RAW SERIES VALUES** — my first cut scaled `rec.openU` and `test_em_band` went red
("openU is scaled in exactly TWO places"). It was right: scaling first compares two numbers AFTER
applying the very ratio in doubt.
⚠ **I ASKED HIM TO REWIND, AND THE REWIND WROTE THE PIN.** When a report follows an instruction I
gave, suspect the instruction.

⚠⚠⚠ **v15.40 — THE REPLAY LADDER WAS EMPTY BECAUSE OF A SCALE, NOT A CAPTURE.** He reported it
THREE TIMES as "you are not capturing the state". ⚠ **THE STATE WAS CAPTURED PERFECTLY EVERY
MINUTE.** Measured replaying 2026-09-01 14:21: the frame held 36 SPXW strikes, seven clearing 20% of
King, **ALL SEVEN inside the band** (7630 −100, 7625 +77, 7610 +52, 7635 −48, 7620 −44, 7615 +39,
7650 +21). `replayLadder` returned `dispScale 0.099775 === undScale 0.099775` — the CASH scale in
both slots — so SPXW 7630 drew at **761.28 on a ladder framed 7615..7680**, ~6,880 points below it.
`inFrame()` refused every node. ⚠ **`dispScale` IS THE CHART SCALE; `und/spx` IS THE UNDERLYING ONE.
Their EQUALITY is the alarm** — it is only correct on a cash chart.
⚠⚠ **THE FIX WAS ALREADY IN THE FILE, FORTY LINES BELOW**, v15.06, in capitals: "THE FIX IS ONE
SCALE, NOT A BETTER FALLBACK." `replayLadder` was written in v15.18, AFTER it. **A lesson written as
prose guards only the function it sits in.**
⚠ **BEFORE BELIEVING A CAPTURE COMPLAINT, READ BACK THE STORED RECORD** — one probe would have
settled this on any of the three occasions.
✅ Replayed basis = today's ES/SPY ratio (no frame records an ES print), DISCLOSED as
`scaleSrc:'replay:fut:ratio-today'`; no ratio ⇒ cash, and it says so. Level rows still refused.
✅ **1ST TP · LOD / 2ND TP · HOD** — the headings name the turn. They said "1ST HOD" until v15.33, so
the rename had silently dropped it. The second is named only once it has PRINTED.
📏 **MEASURED, NOT FIXED — the day does not fit.** Store holds 14:00→15:00, **24 frames**. Each is
**26,551 bytes, 64% of it `feat`** (the learning vector, which nothing that DRAWS reads); `vend` —
what the replay ladder is rebuilt from — is 10%. A full RTH day = 130 × 26.5KB = **3.44MB against a
3.6MB budget (96%)**, so any growth evicts the morning. Dropping `feat` from the frame ⇒ ~1.2MB.
**Two budgets is the clean form: the replay slice must not be evicted by the learning payload.**

⚠⚠⚠ **v15.39 — `emBand` MULTIPLIES EVERY PRICE IT RETURNS BY `emRr`, AND UNTIL NOW SAID NOTHING.**
Measured 2026-08-31: bars high **769.88**, `EB.hiWater` **772.28**, ratio **1.0031195570**. And
`scaleUsed` reads **1**, so a caller checking for a conversion is told there is none. ⚠ **THE LADDER
IS EM SPACE; `hodLod`/`sessionBody` ARE BAR SPACE.** Anything moving between them must multiply.
I walked into this INSIDE the fix for it — v15.39b put bar prices on the EM rail, the body hung
below its own wick (jsdom) and the expected move collapsed to 1% of the view (real Chromium), which
is v15.28's exact fault reintroduced. ⚠ **THE COLOUR IS SCALE-INVARIANT; THE COORDINATES ARE NOT** —
share the FACT, convert the COORDINATE. `emBand` now publishes `emRr`; use it, never derive it.
⚠⚠ **THE ORIGINAL DEFECT:** the NOW-column candle drew `EB.open`→LIVE TAPE (RED) while ⓪a drew
`hodLod.open`→last CLOSED bar (GREEN), on the same session. **The panel was FROZEN and the NOW
candle was still following the after-hours tape** — `recorderBlind()` gates every WRITE and gated no
READ. ⚠ The day was FLAT (+0.50 on 52.25) so **the disagreement was 6× the body**.
✅ `sessionBody(sym)` now owns the session's open/close/hi/lo; both candles read it. `__gptsDebug.sessionBody()`.
📏 **MEASURED over 284 sessions:** median body 43% of range; **13% of sessions have a body smaller
than the 3.25pt error** — one day in eight it decided the colour.
⚠ **FOUND, NOT FIXED:** the panel carries TWO session highs — `emBand.hiWater` and `hodLod.hod` —
consistent only through `emRr`. Now labelled, not yet unified.

⚠⚠⚠ **v15.38 — THE FUTURES-GAMMA WORK IS PARKED, NOT ABANDONED, AND NOT YOURS TO START.**
`design/spec-futures-gamma-markets.md` — gamma levels for **CL · NG · GC · E6 (· HG pending)** from
the real CME chains, free, into the IRT export. Operator, 2026-09-01: *"hold this implementation
detail somewhere... we will come back to it once the application with the current markets is
optimal."* ⚠ **DO NOT START IT, AND DO NOT RE-RESEARCH IT.** `load gex` step 1a-1 routes you there.
⚠ **v15.38 CHANGED NO PANEL CODE** — version string only; the installer is the delivery channel.
⚠ **THE THREE TRAPS THE SPEC EXISTS FOR** (each measured, each would ship a wrong number):
`<ROOT>*1` (nearest) vs `<ROOT>*0` (most active) — **`*1` puts GOLD and COPPER on SEPTEMBER**, months
he does not trade; the DTN root map is **hand-written, never derived** (copper is `CPE` not `HGE`,
euro is `E6` with no suffix and Barchart's root is `E6` so `6EU26` 404s); and **the chain is not in
the raw HTML** — 466,227 bytes, `"strike":` appears ZERO times, which is why delivery is a reader
userscript rather than a fetch.
⚠ **THE NEGATIVES, which cost the most to establish:** Skylit returns **zero snapshots** for every
futures symbol **including ES1** (SPY returns 390); InsiderFinance is equity/ETF-only; the ETF
conversion (FXE/GLD/USO/UNG) was **dropped and is explicitly NOT a fallback**.
⚠ **OI IS PUBLISHED ONCE A DAY BY THE EXCHANGE — there is nothing to poll.** True for the paid
vendors too. One pull after the open is correct, not a compromise. **Do not build a poller.**
✅ `test_parked_specs.js` (36) fails the build if any of it is deleted. 24 mutations, 24 caught —
**after a fix**: two passed initially because I asserted words appeared *somewhere* in a document
that deliberately states the warning twice.

⚠⚠⚠ **v15.37 — FOUR HEADER LAMPS, ONE PER EXTERNAL FEED: `IRT · IF · YF · FF`.** He asked for
indicators for **Yahoo Finance (YF)** and **ForexFactory (FF)** and added *"All of this integration
better be mentioned somewhere. check where it is mentioned."* ⚠ **THE AUDIT:** Yahoo was in
`DEPENDENCIES.md` §2 but the heading never said Yahoo, and it had no lamp. **ForexFactory was in TWO
CODE COMMENTS AND NOTHING ELSE** — no doc section, no `deps()` item, no lamp, since v14.38.
⚠⚠ **§0 of that very file says every dependency here fails silently and must be written down.**
Writing the warning is not obeying it; `test_deps.js` now enforces all four steps (item · lamp ·
test · SECTION HEADING) and is **76 assertions**, up from 36.
⚠⚠ **FF IS COUNTED, NOT AGED** — it is delivered once a day, so an age reads "FF 340m" on a healthy
calendar by mid-session. It shows the EVENT COUNT, and **`0ev` is a real, healthy answer**; the
green dot is what says the courier ran. `cal.ff` tests the `day` STAMP, never the count — a
count-based check calls every quiet day broken.
⚠ **BOTH FAIL INTO A PLAUSIBLE FACE:** stale Yahoo bars still have a high and a low (the ⓪a candle
draws a WRONG one), and a missing calendar removes a caveat rather than blanking a section.
📏 **MEASURED in real Chromium at 673px:** the four lamps take 185px, ending x=337; right-hand
controls start x=529 — **192px slack**, panel floor is 652. Labels pinned to ≤3 chars.

⚠⚠⚠ **v15.36 — THE KING LANE IS NOT A CENSUS AND I QUOTED IT AS ONE.** He asked "for each type of
king, how many rolls were there" and I read the numbers off `KTRACK`, the **king lane** — which is
dwell-filtered to 20 minutes **because he asked for it to be** (v15.23, "too erratic"). Throwing
changes away is the lane's JOB. ⚠ **Median ratio census : lane over his own 8 recorded sessions =
×3.0.** ⚠ And the count still CLIMBS as sampling gets finer (15m→6, 9m→7, 6m→9, 3m→12 for SPXW), so
**every number this file can produce is a FLOOR** and Atlas, recomputing continuously, reads at or
above all of them. ⚠ **THE HONEST FORM IS "AT LEAST N".** ⚠ MEASUREMENT WITHDRAWN: any earlier
per-king roll count — those were lane counts.
✅ **NOW SHIPPED:** `gpts_kingraw_v1` — one entry per crown CHANGE, ~15s cadence, **all three books**
(QQQ included: excluding it from the DRAWING had silently excluded it from the ANSWER).
`__gptsDebug.kingTrack()` returns `rolls` (census) *and* `migrations` (lane), and the lane tooltip
says both. `replayKingRaw()` rebuilds the same shape from 3m frames and **declares its coarser
basis**. ⚠ **AND A REAL BUG FELL OUT:** `ktTick` had NO book-depth floor — a half-loaded first paint
has a king and it is noise; the dwell was absorbing it. `krTick` reuses `SK_MIN_STRIKES`, the same
floor `skPiles` and the LASTBOOK latch use. ⚠ **A tolerant consumer makes an unguarded producer look
correct.**

⚠⚠⚠ **v15.35 — THE FREEZE BADGE PRINTED A 1970 TIMESTAMP AND IT LOOKED REAL.** `LB.ts` is epoch
MILLISECONDS, `fmtClock(ts)` does `new Date(ts)` (also ms), and the call site divided by 1000 in
between: `1788296340000 → 1788296340 → 1970-01-21 → **10:44 am CT**`. A book latched at **14:59:00**
displayed as "frozen 10:44 am".
**THE LATCH WAS ALWAYS CORRECT** — SPXW king 7630, 100 strikes, ts 14:59:00, exp 2026-09-01. Only the
label lied, and 10:44 am is plausible enough to be believed. ⚠ **A disclosure that lies is worse than
no disclosure**: silence prompts a question, a wrong number ends one. ⚠ And the comment two lines
above says the badge "names the SESSION and the CLOCK TIME" — it did, and still printed 1970.
✅ **EVERYTHING ELSE POST-CLOSE CHECKED CORRECT:** lamps in the header (`IRT 1m · IF 1m`), AFTER HOURS
chip, ⓪a below the ladder, 1ST TP / 2ND TP, brighter labels, zero render errors. `deps` has one
failure left — `if.SPY: missing expected move` — which is EXPECTED under the v15.06 SPX pin.

⚠⚠ **v15.34 — THE FEED LAMPS ARE IN THE HEADER** beside the version chip (they cost a 13px row on
the top strip). `render()` paints `#gpts-hdrlamps` each pass. ⚠ Their CSS is scoped to `#gpts-panel`,
NOT `#gpts-body` — a selector scoped to a parent encodes a LOCATION, and locations change.
✅ **THE CLOSE-OF-DAY FREEZE, VERIFIED LIVE at 14:57 on 2026-09-01:** `CFG.lastBook true`,
`gpts_lastbook_v1` holding SPXW king 7625 / 100 strikes stamped that second, recorder at 31 frames.
⚠ **TWO STORES, TWO ANSWERS ABOUT THE SAME DAY:** the RECORDER started at 13:39 (panel reloaded, and
it only runs while open) so the slider covers 13:39→close; the KING TRACK is a separate day-keyed
latch that SURVIVED the reload and starts at 08:30. Say which store when reporting coverage.
📌 **2026-09-01 KING ROLLS (his question):** SPXW 5 recorded migrations / 4 drawn after the 20-minute
dwell; SPY 3 recorded / 2 drawn; **QQQ NOT TRACKED AT ALL — `KT_BOOKS` is SPXW + SPY only**, though
the ladder draws a QQQ crown. Atlas itself publishes no roll count, only a live King-distance chip
per symbol, so there is nothing on their side to reconcile against.

⚠⚠⚠ **v15.33 — MY OWN `deps()` CHECK CALLED IRT BROKEN WHILE IRT WAS WORKING.** It reported
`irt.build: nothing to write` while `IRT_LAST` held `{rows:6, how:'file', inPlace:true, err:null}`.
v15.22 re-ran `irtBuildCsv()` AS A PROBE, and that rebuild depends on live inputs (the IF ladder, the
ES ratio, the latched crown), so one unlucky instant reads as a dead export.
**A HEALTH CHECK MUST OBSERVE THE SYSTEM, NOT PERTURB IT** — it reads `IRT_LAST.rows` now and probes
only when no export has run this session. ⚠ Found only because I read the live state before building
the lamp; shipping the lamp on that check would have glowed red at him all day over nothing.

⚠ **TWO FEED LAMPS ON THE TOP STRIP** (`feedLampsHtml`), both from the SAME `depsHealth()` the footer
dot and `__gptsDebug.deps()` read — never a second opinion:
    IRT 2m   the panel WRITING king levels to the file IRT polls
    IF 3m    InsiderFinance ARRIVING — the age of the FRESHEST usable chain
Each states its AGE: a green dot with no number is a claim you cannot check.

⚠ **LAYOUT:** ⓪a HOD/LOD is mounted BELOW the ladder (the mount moved, `secDay()` untouched);
the columns are headed **1ST TP / 2ND TP** with the extreme's identity moved to the hover; the ⓪a
labels went `#6c7889` → `#9fb0c4` (3.1:1 → ~7.4:1 on the card — his "dark grey" was a real number).

⚠⚠⚠ **v15.32 — EVERY BUILD MESSAGE CARRIES TWO BLOCKS THE BUILDER PRINTS. PASTE BOTH.**
Operator, twice: *"i dont see the tamper monkey links or save confirmations."* Scroll to the END of
`python3 tools/build-installer.py` output and copy:

    ==== PASTE THIS WITH THE INSTALL FILE ====     both Tampermonkey links, CHANGED/UNCHANGED
    ==== SAVE CONFIRMATION — PASTE THIS TOO ====   chat history · lessons · changelog · resume note

The save list is read from `git show --stat HEAD`, so it cannot be written from memory, and each
mandated file is marked `saved` or **`MISSING`**. `skills/gex/SKILL.md` step 0-bis requires both.
⚠⚠ **THE LINKS HAVE PRINTED SINCE v14.3 AND STILL WENT MISSING FOR SEVERAL BUILDS**, because pasting
them lived only in a context's head. **The only rules this project keeps are the ones something
prints or something fails on** — and I read that sentence in LESSONS §0 this session and still
let the step lapse.

⚠⚠ **v15.31 — THE STRIKES WERE NEVER MISSING; THEY WERE BELOW `nodeThresh` (20% of King).**
Measured: 100 SPXW strikes on the tape, ELEVEN drawn. The filter is right — a NODE is a strike with
mass — but a price axis with holes reads as data loss. Every in-frame strike now draws a 1px tick at
18% opacity, width = its own %King. ⚠ **A GAP IS INFORMATION**: grid drawn with no bar = no dealer
mass between two levels, the air pocket, previously invisible because the row was absent.
⚠ **THE DAY IS A CANDLE BEHIND THE NOW COLUMN** — wick = `hiWater`..`loWater`, body = `open`..now,
green when up. Every number is one the ⓪a section already measures and the band is anchored on, so
the candle and the band can never describe different sessions.
⚠ **THE VIEW MUST HOLD THE WHOLE DAY** as well as the band — asserted by c3 the way L7b asserts the band.
⚠ **IBH/IBL ARE OFF THE LEVEL RAIL** (both call sites) and still MEASURED by `sessionLevels`.

⚠⚠ **v15.30 — THE GRIP WAS CAPPED AT 560 WHILE HIS PANEL WAS 673.** The first pixel of drag snapped
it DOWN and pinned it, so widening was impossible and the gesture read as dead. 560 was right when
the ladder was 588px; `ladderFit()` has been growing the panel past it for builds. `panelWidthBounds()`
is a FUNCTION now — floor = the ladder's own width (narrower just hides columns), ceiling = the
viewport — so it can be executed instead of grepped. ⚠ **A rule the operator's hands touch is
behaviour and gets a test that runs it.**
⚠ **THE TAPS COLUMN IS RETIRED AND THE ROLL LANE HAS ITS SLOT (x 344).** `LAD_W` came DOWN 640 → 618.
The tap COUNT survives in the STATE hover — removing a badge must never remove the measurement.
⚠ **"AT ALL TIMES SHOW EH TO EL"** is asserted in a real browser as a SPAN (L7b), not as two separate
labels: checking each edge alone passes a view holding one and not the other.
⚠⚠ **THREE BUILDS RUNNING, A LITERAL IN A TEST WAS WHAT BROKE.** r11 pinned `LAD_ROLL=620`, r12
pinned `LAD_W=640`, and both failed on a change that honoured what they were protecting.
**A position is a decision; a property is a fact. Assert the property.**
⚠ And I tripped the documented `val()` landmine myself: `typeof LAD_W==='number'` earlier in the file
than the declaration IS read as the declaration. Write `'number'===typeof X`.

⚠⚠⚠ **v15.29 — THERE IS A REAL BROWSER IN THE CONTAINER. USE IT FOR ANY LAYOUT QUESTION.**

    node tools/render-face.js <day> <hh:mm> --page     # standalone doc: panel CSS + body
    node tools/measure-ladder.js                        # lays it out in Chromium and measures
    node test_ladder_layout.js                          # 7 assertions, all in a real browser

**jsdom has NO layout engine** — every box measures 0, `scrollTop` never moves, `max-height` does
nothing — so every layout property was a `[GREP]` and the greps were guarding a clamp THAT DID NOT
WORK. On the first real-browser run, v15.28's EL label rendered at **299..312 in a 300px window**.
Three compounding faults, none visible without layout:
1. the pill's `top` is its **CENTRE** (`height:13px; transform:translateY(-50%)`) and I had **guessed
   11** for a box the stylesheet declares as 13 — one line away in the same file;
2. the **frame is not the view** — the window opens on the band and is shorter than the content;
3. the **header row shares the scroll box**, so `max-height:viewH` gives the ladder `viewH − 12`.
⚠ **A container's height is the sum of what is IN it.** ⚠ **`[GREP]` is a debt, not a resolution.**

⚠⚠⚠ **v15.28 — THE LADDER OPENS ON THE EXPECTED MOVE AND SCROLLS TO THE REST. HIS SPEC, VERBATIM:**
"at the open the ladder should be drawn from the expected move low to the expected move high and then
from that point on should adjust its height based on price movement taking out either side as well as
allowing me to scroll up and down."
    content = every node at its true price, ONE coordinate system, nothing clipped (v15.04's lesson)
    window  = EL..EH, widened by price taking out either side, +4% air
    scroll  = the wrapper scrolls vertically; applied ONCE per row-set so it never fights a manual scroll
⚠⚠ **AND THE REASON IT WAS NEEDED: IN REPLAY THE BAND WAS A FIVE-PIXEL SLIVER.** `feat.emband` is
recorded in CHART units while the frame's `px` is the UNDERLYING price, and the replay pin carried
`rr:1` — so band 7661..7730 sat beside its own `now` of 764.49 and the frame spanned **6,986 points**.
**The ratio was in the frame all along: recorded anchor ÷ the series' own open.** Fourth build running
that a mixed ruler was the fault.
⚠ **THE EXPECTED LOW WAS NEVER MISSING** — it was drawn at `top:300px` in a 300px frame. And that is
the NORMAL case: `emRailBounds` starts the frame AT the band, so both labels land on an edge by
construction. The label is clamped inside by its own height; the RAIL stays on the true row.
✅ **AND THE ARITHMETIC, VERIFIED ON HIS LIVE v15.27:** ES open 7647 ± 32.5 → EH 7680 / EL 7615, drawn
exactly. Rows spread 30.7→239 with a 19px median gap.

⚠⚠⚠ **v15.27 — `EB.scaleUsed` HAS TWO MEANINGS. DO NOT CHANGE WHAT IT CONTAINS.**
TEN call sites multiply an UNDERLYING-book value by it to reach chart space (the SPY King flag, the
prior-day levels, the dark-pool prints, `levelMarkerOf`). The BAND needs the scale of the series IT
measures. Those were the same number only while the band measured the underlying book — v15.24 moved
it to the ES series (scale 1) and v15.26 made the pin agree, so **PDH drew at 768 on a ladder of ES
strikes** and twelve rows crushed into six pixels.

    out.scaleUsed   = UNDERLYING book → this chart (~10.0353)   ← the contract. Never repurpose it.
    out.seriesScale = the band's own series scale (1 for ES bars) ← use this inside emBand

⚠⚠ **AND v15.26's GEOMETRY GUARD PASSED THE FAULT IT WAS WRITTEN FOR.** y2 asserted min-to-max
spread; his ladder had twelve rows in 6px and one outlier at 636 → spread 635px → green.
**A RANGE IS NOT A DISTRIBUTION.** y3b (median gap ≥6px) and y3c (no tenth holds >70% of rows) are
pinned against his ACTUAL measured tops, with an assertion that the old test passes them.
⚠⚠ **TWO BUILDS IN A ROW WENT OUT LIVE-BROKEN (v15.24 blank, v15.26 scrambled), both with a green
suite, both found by him in one glance. For any change to a scale, a unit or a shared field: RENDER
THE FACE AND LOOK AT IT** — `node tools/render-face.js <day> <hh:mm>` exists and I did not use it.

⚠⚠⚠ **v15.26 — I SHIPPED A BLANK LADDER AT v15.24 AND THE WHOLE SUITE WAS GREEN. READ THIS BEFORE
CHANGING WHERE ANY NUMBER COMES FROM.** v15.24 moved the band's anchor to `measureBars()` (ES bars,
already chart-scale, rr 1) and left the STORED pin's `rr: 10.0353` from the derived SPY series.
`useRr` preferred the stored one, so `hiWater = 7673 × 10.0353 = 76,986`, the rail frame spanned
~69,000 points, and **all thirteen rows drew at `top:639.7px` of a 640px frame**. Audit ok, zero
render errors, 134 green, blank panel.
**A SCALE STORED IN ONE SERIES' UNITS IS MEANINGLESS AGAINST ANOTHER** — third instance (v11.65,
v15.12). The pin now records `src` (the series) and is REBUILT from `emK` when the series or scale
changes. ⚠ Replay pins are exempt, like the v15.24 heal — forgotten twice in three builds, caught
both times by the cross-examination against `feat.emband`.
⚠⚠ **AND THE SUITE COULD NOT SEE IT because every assertion checked PRESENCE.** `test_replay_face`
y1-y5 now assert the GEOMETRY: rows spread, at distinct heights, the band spanning points not tens
of thousands. **Presence is not legibility.**
⚠ **WHEN HE REPORTS A DISPLAY FAULT, SCREENSHOT FIRST.** Every debug surface reported healthy —
correctly — while the face was empty.

⚠⚠⚠ **v15.25a — THE DELTA PROFILE'S REAL EDGE, MEASURED. DO NOT OVERSTATE IT AGAIN.**
`tools/study-deltacadence.js`, 13 sessions (08-17..08-31): **BUILDING is 52.8% against a 50% coin**
at a 30-minute horizon, and it does NOT improve with a bigger move (+4-10% → 52.1%, +100%+ → 52.5%).
**The only differentiator is DISTANCE: within 25 points of spot 56.9% (n=1266) vs 51.7% further out
(n=3999).** The measured parts of this section are the ROLL and SPENT (19/19 pass-throughs), not the
15m change. ⚠ His business requirement is support/resistance prediction — say 53% when it is 53%.

⚠⚠ **v15.25b — THE STATE IS HELD FIVE MINUTES BEFORE IT CHANGES (`LVL_HOLD_MIN=5`).** Measured:
hold 0m → 14.1% of reads change state, 52.8% edge · **5m → 9.3%, 52.2%** · 10m → 5.6%, **50.6% (a
coin)**. Five removes a third of the churn for nothing; ten takes the signal. ⚠ Replay is EXEMPT —
the slider jumps between minutes and a per-strike cache would carry state across a two-hour leap.

⚠⚠ **v15.25c — THE ROLLS WERE ALWAYS DRAWN AND NOBODY COULD READ THEM.** Four real rolls
(7645→7665 $18M, 7650→7665 $16M …) as stepped paths in a 20px column at the far right, no strike
named. Each row now carries `⇢7675` (amber, leaving) or `⇠7650` (blue, arriving). ⚠ **"It renders"
is not "it is readable".** Its 32px came from the ROC column, which lost the 5m at v15.23.

⚠ **THE AMBER LINE crossing pills was the EM rail drawn INSIDE the pill chute** at the band edge's
true price, while its label steps away to clear the crowns. Two dashed segments now stop either side
of the chute. ⚠ The line was in the right place and the wrong column.

✅ **EH/EL ARITHMETIC VERIFIED on his pin:** `openU 759.5653 × rr 10.0353 = 7622.4`, `± em 32.31` →
**7655 / 7590**, matching the face exactly. The width IS InsiderFinance's 0DTE ATM straddle captured
at the open; it is added to and subtracted from the open. The ANCHOR was the fault (v15.24).

⚠⚠⚠ **v15.24a — THE CROSS-EXAMINATION IS THE STRONGEST TEST THIS PROJECT HAS. USE IT.** Every frame
records what the LIVE face was reading — `tri.<book>.top`, `tri.<book>.king`, `feat.emband` — so a
replayed render is checked against THE RECORDING, not against my expectation. `test_replay_face`
does this (x1-x6) and caught a regression on its first run: v15.23's own heal was overwriting the
replayed pin, 771.74 recorded vs 769.34 drawn. **Both plausible; only the recording knows.**

⚠⚠ **v15.24b — WHY REPLAY COULD NOT CAPTURE THE DAY.** Measured on his recording: 34 frames, and
**EIGHT carried no `tri`, no `vend`, no `px`** — empty shells the slider offered as seekable ticks,
so the handle landed on one and the face went blank. The recorder refuses to write them now, and
`replayLoadDay` drops the ones already on disk (`replayUsable`), reports the count, and distinguishes
"the frames carry no book" from "none were recorded".
⚠ His recording that day also STARTED AT 09:03, not 08:30 — the recorder only runs while the panel is
open. Not a defect; a limit to state when the slider will not go back further.

⚠⚠ **v15.24c — A RULE ENFORCED ONLY AT WRITE TIME CANNOT FIX WHAT IS ALREADY STORED.** Twice in one
build: the king lane's dwell (KTRACK already held the day's flickers — his lane still showed **23
runs** after v15.23) and the empty frames. Both now enforced where the data is READ.
⚠ And the replay rebuild NO LONGER filters as it builds: **one rule, one place, both paths.** Two
copies looked like belt and braces and were invisible to mutation.

⚠⚠ **v15.24d — THE BAND ANCHORS ON `measureBars()`, NOT `closedCandles()`.** On a futures chart the
underlying series is DERIVED — rebuilt from ES through a moving basis — so the same 08:30 bar read
759.5653 and later 761.9526. The band drew 7590-7655 against a real ES open of 7647 with price at
7663, ABOVE the expected high. **A value recomputed from a moving input is not a record of anything.**

⚠⚠ **v15.23a — THE EM BAND WAS ANCHORED ON YESTERDAY'S OPEN AND NOBODY COULD SEE IT.** Measured
on his panel: pin `openU 768.6968 · rr 10.0353` → anchor **7714**, against a real ES open of **7647**
(courier) and a SPY first candle of **761.93**. Price sat below EL all day and EL wore the ⤓.
Three stacked failures: the warm-up guard tested **`cs[0].time`, a field the candles do not have**
(they carry `t`/`so`), so it never fired; the capture then ran at 08:30:08 against an array that had
not rolled to today and stored `openSo:null`; and the self-heal REQUIRES `openSo` to be a number, so
a badly-captured pin was permanent for the session. ⚠ The width was always InsiderFinance's 0DTE
straddle and the anchor was always meant to be the open — **the width was right, the open was
yesterday's.**
⚠ `_openSec` must stay at FUNCTION scope: inside the capture branch it hoists as `undefined` on
every render where a pin exists, and the heal silently cannot run.

⚠⚠ **v15.23b — `KT_DWELL` WAS A COUNT READ BY TWO LOOPS AT DIFFERENT RATES.** Live it ticks per
render (seconds → ~6s of probation); in replay it walks 3-minute frames (→ 6 minutes). One name, two
rules, and the live king lane was effectively unfiltered — "too erratic". Now **`KT_DWELL_MIN=20`**,
a DURATION honoured by the clock in both paths. Measured over **11 sessions, 08-17 to 08-31**:

    dwell   SPXW median [min-max]   SPY
      0m    5 [0-15]                5 [0-11]     ← what he was seeing
     20m    2 [0-4]                 3 [0-5]      ← shipped
     30m    2 [0-4]                 2 [0-4]      ← starts erasing real moves

⚠ n=11, one instrument, one three-week window. ⚠ A duration also means a GAP in the recording cannot
promote a flicker, which a count could never notice.

⚠ **THE ROC COLUMN IS 15m ONLY** (matching the Δ column). The 5m is still COMPUTED and still decides:
TURN needs 5m and 15m to agree and both to have flipped against the hour. BUILDING/WEAKENING are 15m.
All three windows remain in the hover.

⚠⚠⚠ **v15.22a — THE DEPENDENCIES HAVE A LIVE CHECK, AND `load gex` MUST READ
`design/DEPENDENCIES.md`.** Operator-mandated: *"it is fundamental to the application."*
InsiderFinance (call wall, put wall, expected move), the ES 1-minute courier, the IRT export and the
recorder ALL live outside this script and ALL fail silently. **A green suite has never meant they
are up** — the suite runs in Node, where none of them exist. Before diagnosing any missing surface:

    __gptsDebug.deps()      ← run this FIRST. The `deps` dot on the footer carries the same verdict.

⚠⚠ It found a fault on its first run: the stored **SPY chain was 15,328 minutes old** (payload
2026-08-21) with a **null expected move**, while SPX and QQQ were three minutes old — and its own
`stale` flag read **false**, because that flag is written once and never re-evaluated. **Freshness is
a question about the clock NOW.** ⚠ A stale SPY is EXPECTED under the v15.06 SPX pin, so the overall
verdict asks "is there a usable book", not "is every symbol fresh" — a check that is red every day is
a check nobody reads.

⚠⚠ **v15.22b — "LOD IN 74% · HOD after 1:30 — 80%" NOW DRAWS. IT NEVER HAD.** The far-side model
shipped at v14.72 gated on `D.secondT > D.clock` — but **`secondT` is the later of the two RUNNING
extremes**, and a session has a running high and low within two bars. Measured: at 08:45 on
2026-08-31, `secondT` was 08:39. The gate was false from the third bar of every session, so the
far-side line never drew, "both extremes in — the range is set" printed all day about an unfinished
range, and the "% of the range" clause never appeared. **Three clauses, one wrong idea, eight
builds** — and `test_hodlod` u8/u9 DEMANDED that gate, holding it in place.
All three are gated on the table's own IN call now, and the floor is anchored on `D.clock` rather
than `ctNowSecOfDay()` (with the wall clock, a replayed 10:00 and 14:12 gave the same floor).
⚠ A floor past the close prints as a refusal, not a time.

⚠ **THE COMPANION LINK SHIPS WITH EVERY BUILD NOW.** The panel's has printed since v14.3 and the
companion's never did, so every build that changed the companion told him to update it and handed him
no way to.

⚠⚠ **v15.21 — `hlPT` WAS SUBTRACTING A SPY PRICE FROM AN ES PRICE, LIVE.** The face printed
**PT 6895.0pts** and **OF BAR 35818%** (and `LC RNG 6894.2`): `hodLod` has measured true ES bars
since v15.08 while `hlPT`, which consumes hodLod's own output, still read `closedCandles()` — the
SPY book. 7661 − 766 = 6895. Both now read `measureBars(sym).bars`.
⚠⚠ **AND THE TEST DEFENDED IT.** `test_hodlod` p7/p7b asserted in a comment that "closedCandles() is
the UNDERLYING book" — true before v15.08, false after — and passed throughout. **A source grep
freezes the model that was true when it was written; when a value's SOURCE changes, the tests that
describe that source are part of the change.**

⚠⚠ **THE LADDER'S COLUMN HEADERS WERE NEVER LOST — NEVER CARRIED OVER.** v14.46 replaced the rail +
node profile with the ladder and the profile's header row (`g3ndhd`) went with the old surface. Ten
columns unlabelled for seven builds, because **a missing label throws nothing and greps as nothing.**
Now: S · Y · LEVEL · PRICE · NODE %KING · NOW · MARK · TAPS · Δ15m · STATE · ROC. ⚠ Every x and width
is the column's own `LAD_*` constant, and the test asserts THAT, not just the words.

✅ **THE ⓪a CANDLE, ANSWERED:** it is **RTH-only** (`hodLod` skips `so < 08:30 CT`, clamps to 15:00),
and on a FUTURES chart it **backfills the whole session** — the courier asks `interval=1m&range=5d`,
so opening at noon still fills from 08:30. ⚠ It was up to an HOUR stale: `FUT_POLL_MS` was hourly,
chosen when these bars only fed a nightly corpus. **Companion v1.17 polls every 5 minutes inside RTH,
hourly outside.** ⚠ **The reason a constant was chosen can expire without the constant looking wrong.**
⚠ On a CASH chart there is NO backfill — `measureBars` falls back to `closedCandles()`, which holds
only what the panel has seen since it opened. Stated, not fixed; the courier carries no cash bars.

⚠⚠ **v15.20 — THE READ IS OFF (`CFG.read=false`) AND THE FIRST REMOVAL TOOK THE WRONG LINE.**
He asked at v15.10 ("take out the read … where it say Range day - Trinity"); something else went and
`.g3tread` — *"EVENT day · Trinity 2-of-3 … KING 7664 (brake) holds"* — stayed, so he had to say it
again. It is a SETTING, not a deletion ("I might come back to it later"): every producer still runs
and the check is `===true`, so a config stored before the key existed leaves it OFF.
⚠ **When the instruction names what is ON SCREEN, confirm against the screen** — `render-face.js`
exists now, so there is no excuse for removing the wrong thing twice.

⚠⚠ **AND `__gptsDebug.audit()` WAS INVENTING A FAULT AND THEN CRASHING.** It read `body.innerText`,
a rendering-dependent property that a layout-free DOM returns `undefined` for, tested the string
"undefined" against itself, reported *"the face prints undefined somewhere"*, and threw on `.split`.
`itxt()` falls back to `textContent`. **The auditor was the broken thing, and only became visible
once the face could be rendered in a test.**

✅ **LIVE CHECK, v15.19 on his panel, 2026-09-01 08:58 CT, market open:** audit ok / 0 violations ·
0 render errors · velocity harvest ok, 1212 objects of 2678 scanned · 17 ladder rows · storage
2,753KB of 10,240 (27%), 0 shed, 0 quota hits · panel 651px in an 837px window, body scrollHeight
962 / client 621 — **it scrolls.** The replay strip and tab bar are present and the day selector
reads `Tue 1 Sep`.
✅ **The v15.18 arrow fix is doing live work:** on his own book that morning, 7615 (`cur −4,939,537`,
`d15 +77,796`) and 7655 (`cur −3,927,477`, `d15 +249,912`) both read as RECEIVING by sign while
their mass was FALLING. Exactly the inversion, live.

⚠⚠⚠ **v15.19 — READ THIS BEFORE TOUCHING ANY REPLAYED SURFACE. THERE IS NOW A TEST THAT DRAWS.**
`node tools/render-face.js 2026-08-31 14:12` renders the REAL userscript in jsdom, parked on a real
recorded minute, and prints what the body contains plus every swallowed error. `test_replay_face.js`
is the same harness with 36 assertions. **Run it before claiming any replayed surface works.** Nine
defects in a row were found by the operator instead of by me for one reason: every other test in this
project executes a FUNCTION, a refusing section is swallowed by design, and so a broken replay and a
quiet one are the same picture.

⚠⚠ **v15.19a — ONE REFUSAL UPSTREAM WAS HIDING SIX SURFACES.** `emBand` pins the day's expected move
once from the LIVE 0DTE straddle, keyed to the session shown; a replayed day has no such record, so it
fell through to today's chain, which after hours does not quote, and returned `no EM`. **The ladder,
the node states, the percentages, the king lanes, the roll arrows and the ROC column all live inside
that section.** The band IS recorded — `feat.emband` = `{ok, em, open, k, est}` — and replay now pins
from it. ⚠ When several unrelated surfaces vanish at once, **walk UP to the nearest thing they share.**

⚠⚠ **v15.19b — THE CLOCK IS A LEAK CLASS WITH NO SEAM TO CATCH IT.** `Date.now()` is not a feed, so
no freshness gate or provenance check sees it. `sessionPhase()` said **AFTER HOURS · EM EXPIRED** on a
14:12 Monday bar — and that branch RETIRES the target, the budget and **the roll arrows** — and the
king lane's axis ran from the replayed open to tonight, crushing a session's journey into 6.5px
(measured: run starts 4.0→10.5 instead of 4.0→21.3). Both now use **`clockNow()`**. ⚠ A wrong
position is not a missing element, and a test that COUNTS elements cannot tell the two apart.

⚠⚠ **v15.19c — CORRECTION TO THIS NOTE: `dispScale` IS RECOVERABLE. The earlier claim was wrong.**
A frame carries `px` (764.86) and `xm.SPXW.px` (7677.55); their ratio IS the basis (0.099775), stored
every minute since the recorder began. My evidence — "a frame has no ES price" — was true and
irrelevant to the question asked. ⚠ **A negative finding needs the same standard of proof as a
positive one**; this one sat in the file the next context is required to trust.
⚠ **THE LEVELS REMAIN ABSENT AND THAT IS DELIBERATE.** `ifLadder` in replay returns the frame's scale
and NO rows: today's PDH/CW0/FLIP drawn over a past session is the mislabelling this project keeps
paying for. Recording `ifLadder.rows` per frame fixes it going forward only.

⚠ **THE ROC COLUMN DRAWS IN REPLAY** as `rp*` — this panel's own change in MASS, italicised, with a
hover that says so. `p15` still means SKYLIT's `percent15Min` and nothing else may ever be written to
that name.

⚠⚠ **"I CANNOT SCROLL" IS SETTLED IN CSS, NOT IN JS.** Reported at v12.2, v12.5 and v15.17; every time
the body scrolled correctly and the PANEL was taller than the window. The panel now carries
`max-height: calc(100vh - 16px)` in its own style, which holds with no code running. `panelFit()`
stays only for what CSS cannot do: pulling a panel dragged off the top edge back into view.

✅ **AND WHAT HE ASKED FIRST, ANSWERED BY MEASUREMENT:** HOD/LOD, the candle and the DAY columns DO
replay — 09:00 / 11:30 / 14:12 give LOD 08:48 / 09:30 / 09:30, HL RNG 4.4 / 5.1 / 5.1, EFF — / 34% /
23%, KING 763.28 / 768.10 / 765.77.

⚠⚠ **v15.18 — A THRESHOLD'S DENOMINATOR HAS A PROVENANCE TOO, AND THAT IS THE TENTH INSTANCE.**
Four of five levels read **SPENT** on a replayed bar. `levelStateOf` divides the frame's mass by
`peakOf(k)` — and `peakOf` returned the LIVE WHOLE-DAY peak, so 14:12 was being judged against a
high water mark set at 15:59. Measured on 2026-08-31 at 14:12:

    strike      |cur|    peak->14:12   ret     peak WHOLE DAY   ret
    7675     81988795      81988795   1.00          115827347   0.71
    7685     34840580      34840580   1.00          503965848   0.07   <- at its OWN peak, called SPENT

`replayPeakOf(k)` is the max `|cur|` across frames **up to `REPLAY.idx`**, memoised on `day|idx`; a
strike no earlier frame carried returns `null`, not a fabricated peak. ⚠ The seam covers not only
the DATA a computation reads but **the scale it is judged against** — walk the constants and the
accumulators, not just the feed.

⚠⚠ **v15.18b — THE ROLL ARROWS WERE POINTING BACKWARDS ON HALF THE BOOK, LIVE AND IN REPLAY.**
`rollScan` tested the SIGNED delta (`src.d15 < -$40K`), which is right on the positive side and
inverted on the negative one, where a strike GAINING mass carries a negative delta:

    7675  |cur| 59.6M -> 82.0M   d15 -22.4M   GAINED   <- was called the SOURCE
    7670  |cur| 40.2M -> 18.2M   d15 +22.0M   SHED     <- was called the RECEIVER

The face drew `7675 -> 7670`; the mass went `7670 -> 7675`, into the King. Both tests now measure
`|cur| - |cur - d15|`. Over 129 recorded frames: **446 old arrows, 310 new, 40 of them previously
drawn REVERSED**, 15 identical. ⚠ The v11.34 note "receivers gained 2.8x, 8.6x…" was measured under
the inverted rule — **do not quote it again as evidence about direction.**
⚠⚠ **AND THE STATES COULD ONLY EVER SAY TWO THINGS.** Every branch but SPENT is gated on `p5`/`p15`
— SKYLIT's percents — which a recorded row does not carry. So replay could reach only SPENT,
WEAKENING-via-roll-source and HOLDING: *"2 weakening and everything else spent"* was the complete
list of things it could say, not a reading of the market. Replayed rows now carry `rp5/rp15/rp60`,
**deliberately not `p5/p15/p60`** — the ROC column credits those to Skylit and their sign convention
is not observable from a recording. `rp15` is ours and its convention is stated: the change in MASS.

⚠⚠ **THE ONE RULE BEHIND ALL THREE OF THIS BUILD'S DEFECTS: `cur` IS SIGNED, `|cur|` IS MASS.**
SPENT's denominator, `rp15`, and the arrows were the same confusion in three places. When a quantity
is signed, decide once whether the code means the value or its magnitude — and say which in the name.

⚠ **`tools/audit-replay-face.js` (new)** prints, for any minute of any recorded day, each node's
mass, its peak-to-then, its state and the arrows, so a claim about the face can be CHECKED. Run it
before answering any "does this make sense" question:
`node tools/audit-replay-face.js 2026-08-31 14:12` ⚠ **its clock is CENTRAL, because the panel's is**
(`ctNow` uses America/Chicago; `replaySec` subtracts 5h) — an audit on ET silently compares 15:12's
book against 14:12's claim, which cost half an hour this build.

⚠⚠ **AND THE RULE THAT COMES OUT OF IT: AN ABSENT MEASUREMENT IS NOT A ZERO.** `nodeTapCount()`
returns 0 both for "never touched" — which the face reports as *a quiet death* — and for "this run
does not track taps". Replay scored every strike a confident zero and every one earned the clause.
Taps are now `null` with `tapsKnown=false`, DECAYING requires a zero that is actually KNOWN, and the
face says the count is not recorded per frame. **Whenever a default is the same token as a real
measurement, the code has no way to be honest.**
⚠ The assertion for this was written as a SOURCE GREP and **survived mutation** — `if(false){
tapsN=null; … }` leaves the text intact. Fourth recorded instance in this project. It now executes
`levelStateOf` in both modes.

✅ **AND WHAT THE CHECK CONFIRMED AS CORRECT** — record confirmations, not only faults: the node
profile is exact at 14:12 strike for strike, and "only five nodes" is every SPXW strike at or above
the 20% threshold, the same filter the live ladder applies.

**FIRST THING TO DO: drag the slider on the live panel.** v15.19 was seen working LIVE (above); the
REPLAY side of v15.17-v15.19 has still never been dragged on his real panel. Check the ladder, kings and ⓪a move together, the
clock reads the parked bar, ◀ reaches Friday, and `__gptsDebug.storage()` shows no new writes.

### the rest of the face

**⓪a DAY is three columns beside the candle** — 1ST · 2ND · DAY, each `label / actual / expected`,
1px rules between. The candle is on the LEFT and its height is **DERIVED**:
`DAYCOL_HD(16) + DAYCOL_N(9) × DAYCOL_ROW(13)`.

    1ST   SLvl · HodN|LodN · TIME · TOOK · BOP · WICK · W.END · OF BAR · MUD
    2ND   TLvl · HodN|LodN · TIME · PT TOOK · PT · PTWICK · (blank) · OF BAR · PTMUD
    DAY   GD/RD · PTN · HL GAP · HL RNG · HL $ · LC GAP · LC RNG · EFF · BODY

⚠ The blank at index 6 of the 2ND column is HIS deliberate gap. Do not fill it.

**The version now sits in the header beside `Tapereader`**, reading `GPTS_VERSION`.
**The READ row is OFF the face** (v15.10, his call — "I might come back to it later").
⚠ `emRead()` is **still called**: `test_em_band` §30 executes it and greps for forecast vocabulary,
so deleting the call would leave that ban guarding nothing.
**TREND is off the face since v14.90**; `secBias()` is kept because `bias.confirm` still records.
**The GAMMA PROFILE was removed at his request (v14.81) — do not propose rebuilding it.**
**The panel is PINNED to the SPX book** (`CFG.mkt`), and says so when the pin disagrees with the
chart: `◉ SPY book (chart: QQQ)`.

## 3 · THE NUMBERS ON THE FACE — every one with its n and its date

⚠ **Never quote a rate from this project without its n and its date.** Several are one-day samples
and at least one was contradicted by a later day.

| what | value | n · window | status |
|---|---|---|---|
| ⓪a **cell** rates (`HLTAB`) | AUC 0.879, calibrated at every decile | 44,302 obs · 284 sessions · 2025-06-02→2026-08-21 | PROVISIONAL, F-4/F-11 |
| ⓪a **IN** decision (`inHit`) | **63%**, median 9:20 | n=284 | CONFIRMED, F-12 |
| ⓪a **NOT-IN** decision | **85%**, median 8:40 | n=230 | **the STRONGER of the two**, F-11/F-12 |
| far side · touch | AUC 0.826, Brier 0.147 | 388,494 obs · 197 sessions | PROVISIONAL, F-14 |
| far side · timing | first passage, AUC 0.692; `T ~ (d/σ)²` does 95% of it | 7,168 arrivals | PROVISIONAL, F-15 |
| **GREEN/RED** (`GD_META`) | **76%** on the **80%** of days it speaks, base 51%, z=8.8, CI 71–82 | n=282 · 2025-06-02→2026-08-21 | PROVISIONAL, no forward test |
| EFF expected (`EFF_META`) | **68%**, a **MEDIAN** | 284 sessions | a ratio takes a median |
| PT / LC (`PT_META`) | PT ~49m / 19.8pts · LC ~103m / 12.5pts | n=283 | **side-specific**: PT after a LOD 24.0pts, after a HOD 17.0 |
| deflections (`DEFL_META`) | 79 deflect / 25 breaks, **56% break** | 8 sessions · calibrated 2026-08-29 | detection only — see §5 |

⚠⚠ **THREE WITHDRAWN NUMBERS. NEVER QUOTE THEM:** the ⓪a IN call at **92%** (hindsight side
selection — `inHindsight:92` is kept only as a label); SUCCESSION **76% crowned within 20 bars** (does
not reproduce at any horizon; 23% at 30m against the DRAWN crown); "crowns beat chance by **17pp**"
(`study-kingdeflect.py` measured crowns where it should have measured nodes).

⚠ **DO NOT RE-PROPOSE**, all measured and recorded: sweeps (48%, below their own base), momentum
divergence (−0.0004), NQ divergence (−0.0014), IB30/IB60, the 50-SMA, open-reclaimed, the 60-minute
breakout, the daily ATR, the overnight range, volume, day-of-week, gap, prior-day and overnight level
identity, prior-day POC/VAH/VAL (gx-009, CLOSED NEGATIVE — the **sham beat the real level** twice),
the prior day's colour (AUC **0.500**), red/green from the open, a 5-feature logistic, or a narrow
high-probability timing box.

---

## 4 · WHAT IS SETTLED — do not re-litigate any of this

- **⓪a is one daily candle.** Rows are legs of it; `OF BAR` sums to 100% and that total is the check
  the decomposition is honest. **WICK% is not a ratio — it is where the OPEN sits in the bar.**
- **PTWICK = PT TOOK + PT BOP** (Q1, answered 2026-08-30): the first-extreme shape anchored on the
  SECOND extreme, as WICK is anchored on the open. **PT BOP and PT W.END were dropped** — em-dash
  most days.
- **ENHANCE HIS LAYOUT, DO NOT REDESIGN IT.** *"i dont want to deviate too much from what i have."*
  A HIGH/LOW rewrite and a per-leg rewrite were both offered and both rejected.
- ⚠⚠ **HE HAS ASKED THREE TIMES FOR A TWO-SIDED TIME WINDOW AT A HIGH NUMBER** and it cannot be built
  honestly: ±15 min lands **15%**, ±30 lands **24%**, and a two-sided window must be **3.6 HOURS**
  wide to reach 80% (197 sessions, F-13/F-15). "after X — 80%" is a **one-sided floor**; the MIDDLE
  HALF is the honest two-sided answer and it is **50%**. Show him those three numbers rather than
  re-deriving the refusal.
- **Deflection geometry is FINAL:** approach **1.0 ATR**, penetration **1.5 ATR**, triggered on the
  **WICK**; the **CLOSE** classifies deflect vs break. **One price event is ONE deflection**, never
  one per node. A **pullback is the extreme of its own 30-minute neighbourhood** — median 3/session.
- **The node universe is a RANK (top few by dollars), not a %King threshold.**
- ⚠⚠ **The node selects WHERE; the price action decides WHAT.** Node 764 on 2026-08-24: one
  deflection, two breakdowns. **Never score nodes as reliable-or-not** — it would look like a finding.
- **Levels are excluded by being mid-range, and IB is excluded BY NAME.** A level price traded through
  is near neither extreme, so one rule does both jobs; a distance test would readmit IB on any day it
  sat on a wick.
- **Ladder:** `LAD_W=640` — raised once from 618 for the roll lane, argued in the open, and the
  assertion now says *"640 IS NOW THE CAP. The next column that wants width argues for it here."*
  The chute is **price's alone** (v14.82 put names in it and he rejected it). Name→price gap is 2px,
  deliberate. `ladderRolls` is retired; its lane is the King columns.
- **Two books, never averaged.** Skylit = FLOW; InsiderFinance = OI×gamma. **Name both units out loud
  before comparing two numbers.**
- **Scale:** SPX and ES are **ten points apart** — a basis, not a conversion. **SPY is 10.04× away and
  every scale failure of 2026-08-30 had SPY in the display path.** `displayScale()` is the single
  source; `hodLod` takes its scale FROM `measureBars`, never from the ratio.

---

## 5 · HOW IT IMPROVES — the part he cares most about

1. **DATA.** Every feature self-declares once in the FEATURES registry and is recorded per bar; the
   `farside` record carries each level's **node identity** (`kind`, %King, polarity, role) — the
   gamma dataset nobody has collected.
2. **ANALYSIS.** `docs/LLM-NIGHTLY-BRIEF.md` carries the ⭐ section: score the touch call by decile,
   score the ≤20% NO call separately, run the gamma test **with a DENSE distance control** (F-16 —
   a sparse control invented a +12-point effect that vanished), and **propose** a new `FARSIDE.json`,
   never apply one.
3. **ADOPTION WITHOUT A BUILD.** Companion v1.16 couriers `BASERATES.json` and `FARSIDE.json` from
   raw GitHub; the panel **validates** them (≥120 sessions, every rated cell n≥60, monotone) and keeps
   the baked-in copy if the payload fails. ⚠ **Monotonicity is not evidence** — two synthetic sessions
   once produced `57/80/100/100/100`.
4. **THE NIGHTLY LOOP** (`tools/nightly/`): **the LLM proposes, the harness disposes.** 8 hypotheses
   are **pre-registered** — locked before the data to test them exists, which is stronger
   pre-registration than anything obtainable later. `subset_null()` is the control that matters: it
   killed four false PROVISIONALs at 82–84% that were mutually contradictory, and it exists because a
   FILTER inherits the edge it filters.
5. **THE UNTESTED INPUT.** `gpts_vix_daily_v1` holds 503 daily ^VIX closes, wired into nothing.
   Implied vol is the one volatility measure that is not a slower copy of what the panel computes.

⚠⚠ **AND THE CONSTRAINT THAT GOVERNS ALL OF IT:** the ES corpus is **284 sessions of PRICE ONLY**.
The gamma book has **~10 recorded sessions**, several of them collapsed. **Every shipped model is
price-only. Nothing using the gamma book can be tested for months.** That is not a reason to wait —
it is why the loop starts by accumulating and pre-registering.

---

## 6 · WHAT TO DO NEXT, IN ORDER

0-KING. ⚠ **VERIFY v15.96 THE KING CHART on his panel during RTH** — toggle the node-ladder section to "King chart": ES
   candles, SPX King (gold, `~` approx scale) + SPY King (cyan, exact) as steps, ▲ held / ✕ broke markers. Does the SPX line
   sit where his IRT SPX King is, and do the markers match where he'd call the deflects/breaks? **Then enrich the King study
   by the factors in `design/KING-STUDY.md`** (roll, growth, pika-stack, polarity, confluence, trend vs 50-MA, time of day,
   above/below open, SPX·SPY·QQQ confluence) as the tap record (v15.97) accrues — one factor at a time, each n-gated.
0. ⚠ **VERIFY v15.94 + v15.95 ON THE LIVE PANEL (after he installs at a calm time — a reload restarts his recorder).**
   v15.94 candle: the number under each extreme is its TOOK from the 08:30 open (not the leg after); a swept day shows
   **SWEPT / TARGET ▸** with swept ticks and cyan/grey target / next-draw rings, none overlapping; PFHI/PFLO appear when swept
   or targeted. v15.95 ⚙: opens to "Tapereader config", scrolls to its foot, and the BO Pullback / Followthrough / Signal Type
   / Compact rows are gone; Node Thresh, Trend, the IRT block and the six alerts stay and work. **Then the next planned build is
   v15.96 — THE TAP RECORD** (R-25, `design/TAP-RECORD.md`; the candle map took 15.94 and the settings trim took 15.95, so the
   tap record and everything after it moved +1 on the roadmap). Discuss it ONE element at a time, mockup first, as always.
1. ⚠⚠ **WATCH THE REPLAY SLIDER ON A LIVE SESSION.** It is unit-tested (73 assertions, 20 mutations)
   and smoke-clean, and it has never been dragged. The things to look at, in order: do the ladder,
   the kings and ⓪a move **together**; does the clock read the parked bar; does ◀ reach Friday; and
   does `__gptsDebug.storage()` show **no new writes** while it is engaged.
2. **Q11 — the ex-ante deflect/break discriminator.** Still the only open question that matters.
   Detection is finished and recall-verified; the touch itself has no edge (mirror legs, 56% break).
   ⚠ 2026-08-31 gave the sharpest case yet: of his six circled deflections the rule caught **five**,
   and the miss (10:54) is structural — on a 5.15-point day the test band is 0.81 points and price
   sat inside it 95% of the time, so 28 contiguous bars collapse into ONE visit and swallow the turn.
   The pullback-first framing catches only four of six. **Neither unit reproduces his set.**
3. **`DEFLECT_ZONE` is still 0.50, fixed and symmetric, in 22 places.** The finalised ATR geometry
   governs only `hlNodeAt` and its hover. A deliberate, separately-tested change.
4. **The day-export gap (FINDINGS F-10c).** `buildDayExport` carries `day.feat` but not
   `FEAT_ARCHIVE`, so resolved outcomes older than the queue never reach the repo — the only thing
   the nightly review reads. ⚠ What trims the queue to ~29 bars is **still unknown**; two mechanisms
   were named confidently and both were wrong. Measure with `__gptsDebug.featHealth()` first.
5. **Ask him Q3** (sweep levels: furthest, or all?) and **Q4** (what is the "nd" contract?).
6. **The implied-vs-realized σ study** — `gpts_vix_daily_v1` holds 503 daily closes, wired to nothing.
7. **ITEM 18 Tier 1/2**, and the **2026-09-16** backfill deadline for the 2026-07-18→08-14 hole.

⚠⚠ **THE IRT PIPE IS SOLVED — DO NOT RE-LITIGATE IT.** IRT's Remote File field reads a `file://` URL
**once, on Apply — it does NOT poll.** Standing config, verified live: `irtserve.bat` running with
autostart, BOTH charts on `http://127.0.0.1:8000/FlexLevelsExport.csv`, Check Every 1 Minute, feed on
`gex`. ⚠ Never run that server against a panel older than v14.74.

## 7 · HOW TO WORK WITH HIM

1. **ONE AT A TIME.** State one item, its fix, ask, **STOP.** This is the most-violated rule in the
   project and breaking it has cost more rework than any bug. If a reply is taking shape with three
   headings and a "which do you prefer" — delete it.
2. **Do not build until he says build.** He says it plainly.
3. **SHOW MOCKUPS FIRST**, rendered headless at his panel width with the pairwise overlap audit.
   ⚠ **Publish design mockups as ARTIFACTS** — he comments on delivered file cards and those comments
   do not reach us; the only reason one set was ever read is that he screenshotted them.
4. **TEST BEFORE YOU BUILD.** A measurement has changed the build after it was already described.
5. ⚠⚠ **DELIVER EXACTLY ONE FILE.** His words, 2026-08-15 and restated 2026-08-27: *"you are
   supposed to just give me an install file."* One `installvNNNN.bat`, dash-free and dot-free, **plus the
   Tampermonkey links as text**, plus **tell him to click them**: run the .bat → wait ~5 min for the
   CDN → **CLICK THE LINK** → **reload the Atlas tab**. Tampermonkey's default update check is once a
   day, so the click is the reliable step. "Reinstall" means he already has it — that is correct.
6. **VERIFY THE INSTALLER BY DECODING IT** before sending. It has silently dropped whole directories
   three times, including `tools/nightly/` with the pre-registered hypothesis bank.
7. **Bump BOTH version strings** (`@version` and `GPTS_VERSION`) and the four test pins.
8. **One edit, one write, verify.** A multi-edit script that aborts writes nothing.
9. **MUTATE EVERY NEW ASSERTION INDIVIDUALLY.** "The suite is green" has never once caught a fake
   assertion; mutation has caught every one. Delete the whole construct — a condition mutation does
   not test a presence assertion, and a mutation narrower than the assertion tests nothing.
10. **End every build message with `✅ SAVE DONE`** naming what was updated.

---

## 8 · DOCTRINE THAT MUST NOT BE LOST

- **Absence of data is not a reading.** Thin cells refuse; they do not guess. And **degrade toward
  silence, not toward noise** — but **never hide real data to avoid drawing it badly** (v15.04/05).
- **A well-formed number is not a supported one.** Monotone ≠ evidence.
- **Measure the question the FACE actually puts**, and state WHEN each variable was read.
- **A matched control must SPAN its range densely**, or it invents an effect.
- **%King ranks at one instant; DOLLARS compare two moments.** A moving denominator cannot measure
  change.
- **Does it POINT or does it CONDITION?** Gamma, vanna, VIX term structure and DEX all condition.
- **Gamma tells you HOW price moves, never WHICH WAY.**
- **Before concluding data is absent, enumerate the keys of what you already hold.**
- **A count that disagrees with how the thing behaves in life is a defect in the counter.**
- **Anything unproven ships labelled unproven and scored nightly.**
