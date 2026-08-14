# GEX SIGNAL TAPEREADER — SESSION RESUME NOTE
_Trigger phrase: user says **"load gex"** → read this file first and come fully up to speed, then continue the conversation from "WHERE WE LEFT OFF"._

_Last saved: 2026-08-12 · Current shipped version: **v10.24.1** (Node Map v1 + regime/Gatekeeper/Rug; Rug polarity VERIFIED after-hours 2026-08-12 -> Rug flag LIVE). 9 new tests + 3 existing green.

---

## 0. WHAT THIS PROJECT IS
A **Tampermonkey userscript** ("GEX Signal Tapereader") that overlays a trading-signal panel onto **Skylit** (app.skylit.ai) for SPY/QQQ 0DTE gamma-exposure (GEX) tape reading. It reads the option-wall / dealer-exposure tape from the Skylit DOM and renders a dashboard: **King** (largest-abs-exposure strike = settlement magnet), **Trend**, **S/R Bias** (srBattle force model), **Breakout (BO)**, and **node accumulation** (Building/Fading strikes). The user (Rassul) is an active intraday trader refining this into a coherent, self-improving decision tool.

- Live script runs in the browser on: `https://app.skylit.ai/atlas?symbol=SPY&interval=3...`
- The panel re-renders every poll tick; candles are 3-min (CANDLE_S=180).

## 1. WHERE EVERYTHING LIVES
**AI Drive root:** `/GEX-Signal-Tapereader/`
- `current/gex-signal-tapereader.user.js` — the LIVE deployed script (what the user installs). Currently v10.22, md5 `ffdf1bd4107aff94c085f3d1fe7e46a6`.
- `releases/` — dated version snapshots + a `pre-vNNN` snapshot before each bump.
- `changelog/CHANGELOG.md` — human changelog, newest on top.
- `session-state/latest-resume-note.md` — THIS FILE (authoritative resume).
- `daily-data/` — INBOX for the end-of-day review: `gex_YYYY-MM-DD.json` files land here.
- `roadmap/PRODUCT-ROADMAP.md` — product roadmap (NOW/NEXT/LATER/EXPLORING).
- `mockups/analysis_tab_mockup.html`, `mockups/roadmap_board_mockup.html` — the design mockups (open in browser).
- Other older docs: master-spec.md, teaching-spec.md, workflow.md, design/, probes/, backups/.

**Sandbox working dir:** `/home/user/gex/`
- `v10.js` — the working copy (edit here). Keep in sync with `current/`. md5 `677fb213...`, `node --check` passes.
- `gex-signal-tapereader.user.js` — local mirror of v10.js.
- Tests (all pass): `test_king.js` (expect 15 "true"), `test_srbattle.js`, `test_confluence.js`, `test_fallback.js` (PASS), `test_recorder.js` (PASS).
- Mockup sources: `analysis_tab_mockup.html`, `roadmap_board_mockup.html`.
- NOTE: sandbox is EPHEMERAL. If `/home/user/gex/v10.js` is gone, re-copy from `/mnt/aidrive/GEX-Signal-Tapereader/current/gex-signal-tapereader.user.js`.

## 2. DEPLOY / BUILD RITUAL (follow every version bump)
1. Edit `/home/user/gex/v10.js`. Run `node --check v10.js`.
2. Run regressions: `node test_king.js | grep -c "true$"` (=15), `node test_fallback.js|tail -1` (PASS), `node test_recorder.js|tail -1` (PASS), `node test_confluence.js|tail -1`, `node test_srbattle.js`.
3. Bump version in 3 places: `@version` header (line ~4), `console.log('[GPTS] vX part1 loaded')` (~line 253), footer `'<span>feed vX</span>'` (~line 4057).
4. Copy v10.js → `current/gex-signal-tapereader.user.js`; snapshot `pre-vNNN` (old) + `dated_feature_vNNN.user.js` (new) into `releases/`.
5. Prepend a changelog entry to `changelog/CHANGELOG.md`; bump version in this resume note.
6. `aidrive_tool get_readable_url` on the current script → give the user the link + "replace the script and hard-reload."

## 3. KEY CODE MAP (v10.js line numbers ~, will drift)
- Palette `PAL` ~line 2142 (bg #0b0e14, card #12161f, line #1e2530, longAccent/green #2ec27e, shortAccent/red #f0616d, gold #e3c341, sub #8b98a9, amber #f2b45a, blue #4a90d9).
- `trendVerdict(sym)` ~446 — returns {state,up,win,ma,slope}. TREND_WINDOW=20, trendMA=50, band=0.25*ATR.
- `srBattle(sym)` ~940 — DISSIPATION-DOMINANT S/R force model. Returns {dom,cross,supForce,resForce,supPct,resPct,nearFloor{k,st,fading},nearCeil{...}}. cross='bears' (SUP→RES, pullback-high short) / 'bulls' (RES→SUP, bounce). SRB_PREV memoizes prev dom.
- `kingVerdict(mv,kingK,px,now)` ~3461 — returns {dir,cls,word,regime,drift,magnet,score,...}.
- `kingBlock()` ~3503 — the King panel. HEADER now = stacked King/price badge (v10.15).
- `nodeBreadth(sym)` ~4069, `confluence(sym)` ~4090 (4-voter → thesis), `confluenceThesis()` ~4141.
- `futureStructureSummary(sym)` — returns {above[],below[],inPlay} node rows with .state.label (Building/Steady/Fading), .net, .pct, .role.
- RECORDER (DATA layer): keys `gpts_recorder_v7` (RECORDER_KEY ~34), `gpts_slices_v7` (SLICE_KEY ~27). `recordNodeSnapshot(sym)` ~1317, `labelForwardOutcomes()` NEW ~1349-area, `buildDayExport()/saveDayToFile()` ~2195-area. Debug: `window.__gptsDebug` (dumpRecorder, dumpRecorderJSON, saveDayToFile, buildDayExport, clearRecorder).

## 4. VERSION HISTORY (recent, newest first)
- **v10.15b** — DATA LAYER: per-bar SIGNAL VECTOR capture in snapshots (`sig`: trend/king/srb/breadth/conf) + `labelForwardOutcomes()` back-filling out5(15m)/out10(30m) {mfe,mae,net,pxEnd,hitKing,revUp,revDn} + `buildDayExport()`/`saveDayToFile()` self-describing daily JSON + footer "📥 Save Day" button. Created AI Drive `daily-data/` inbox.
- **v10.15** — King header STACKED badge: King strike on top / SPY price below / signed offset to right (red if King above price=resistance, green if below=support, gold if equal). Replaced old 3-chip cluster (gold price + distance + net-drift).
- **v10.14** — srBattle support/resistance FORCE engine + crossover flag ("▼ BEARS TAKING OVER" / "▲ BULLS TAKING OVER"). Wired into S/R Bias bar + confluence BREADTH. (Validated against real recorder data across pullback highs 12:45/13:50/14:45 and bounce lows; ~1-bar lag known.)
- **v10.13** — CONFLUENCE engine (one coherent thesis across all reads).
- **v10.12** — Removed Trend section; order King → S/R Bias → BO.
- **v10.11** — King path per-segment coloring fix.
- **v10.10** — King verdict colored pill + magnet-target line.
- **v10.8/10.9** — King stepped time sparkline; moved King under Trend, taller.

## 5. USER'S CONFIRMED DECISIONS (locked — do not re-ask)
### Self-improving review loop
1. **Storage:** localStorage live + **AI Drive daily archive** for long-term multi-day corpus.
2. **Review trigger:** **auto-scheduled workflow** after the day's data is saved to AI Drive.
3. **Outcome window:** store **BOTH 5-bar(15m) and 10-bar(30m)** forward outcomes. (DONE in v10.15b.)
4. **Review focus:** predict — direction, support/resistance, buy/sell signals, targets, pullback reversals, trend reversals; improve QUALITY of all. Plus: the **WHY** behind success/failure; improve the **app & its features**; **discover trader-useful patterns**; make **discoveries + ranked recommendations**.
### Storage delivery
- Going with **Option A for now** (script auto-triggers day's download at session close → user confirms browser save → drops into `daily-data/`). ALSO the **paste-to-chat path**: user can paste the recorder JSON (or say "run today's review") and the AGENT writes it to Drive + runs the review (agent HAS Drive access; the userscript does NOT — it can't authenticate to Drive). Zero-touch relay endpoint is in EXPLORING.
### Analysis tab (in-app, driven by LLM review)
- Approved the **7-step narrative** design (see mockup). Wants **scrollbars** + a **widget per section**. Wants all FIVE relationship widgets:
  1. Relationship timeline (King+price+S/R dominance band, horizontal scroll) 2. King↔price convergence tracker 3. Multi-node lifecycle panel (vertical scroll) 4. Confluence-outcome matrix 5. Signal scorecard + lead/lag.
  Plus: What Worked/Missed + Why, Discoveries + ranked Recommendations, Day Grade header, 7-day footer trend.
- Order approved: **story-first** (grade header → timeline → King pull → nodes → did-it-pay → scorecard → why → discoveries/recs). (User said "go with your recommendation.")
### Trend rules (locked in discussion, NOT all coded yet)
- Trend uses SMA-50 on 3-min closes, 20-bar window, band 0.25*ATR.
- **16/20 (≥75%) dominance** rule over ALL 20 bars. Three+ states: **Uptrend, Uptrend-broken, Downtrend, Downtrend-broken, NA.** "Broken" is a MANDATORY middle step before the opposite trend (uptrend→uptrend-broken→downtrend, and inverse). Show "broken" in a distinct **amber/caution** color.
- Remove the **MIXED/NO EDGE** headline and the **4 badges**.
### READ section (renamed concept, discussed, NOT coded)
- Rename the old vote section to **READ**: a plain-language summary of King + Trend + S/R nodes. **No vote** — just informs/sums up the other sections. TRIGGER voter → driven by Trend (option C = three-state). CONTEXT voter → **DROPPED** (netPositioning board-tilt proved wrong on 2026-08-11 tape: read bullish while price fell). Final voter set = **King, Trend, S/R** (3, no context).
### Breakout gating (discussed, NOT coded)
- Only surface **meaningful** breakouts aligned with the trend: downside break allowed when **downtrend OR uptrend-broken**; upside break allowed when **uptrend OR downtrend-broken**. Require close **beyond the 50-MA** (up-break above, down-break below). Apply to BOTH the READ text AND the BO state machine so everything is in sync. Confirmed-trend breakout = high conviction; broken-trend breakout = low/early.
### King sparkline coloring (discussed)
- Color the line by a **running King trend with hysteresis** — flip regime only when King reclaims prior pivot (strictly beyond by ≥1 strike); dots stay colored by their own local direction; cold-start gray until first pivot. (This was the "no minor color shifts until King trend truly changes" request. v10.11 did per-segment; the hysteresis upgrade is still pending.)

## 6. REQUESTED ROADMAP FEATURES (saved in roadmap/PRODUCT-ROADMAP.md)
- **📏 HOD/LOD Tracker (v10.17)** — live high/low of day as reference rails; feeds targets; failed-breakout/range flags; mark on King timeline.
- **🌐 Multi-Symbol — Crude (CL) & Gold (GC) (v10.18)** — per-symbol strike spacing (CL~0.25, GC~1.0), ATR bands, trend MA, session hours; header symbol switcher. Dependency: per-symbol tape feed in Skylit.
- **📤 Export GEX Levels → IRT (v10.19)** — push King/walls/floors/ceilings/HOD-LOD as drawn levels into IRT. OPEN QUESTION: IRT import format (CSV / clipboard / API?).

## 7. VALIDATED MARKET FINDINGS (from 2026-08-11 recorder analysis — reuse, don't re-derive)
- 12:12 turn = **support bounce** (floor net +8→+13), NOT a breakdown. Short only on floor DISSIPATION, not every pullback.
- 1:30–1:42 grind = floor rebuilt lower (net +44), srBattle read SUPPORT.
- **1:48 = the shortable moment**: nearest floor dissipated hard (net −52), srBattle flipped to RESISTANCE → "BEARS TAKE OVER" crossover. This validated the dissipation-dominant srBattle over the old static netPositioning.
- Pullback highs ~12:45(771.65) / 13:50(770.50) / 14:45(771.30): ceilings built (+18,+12,+12), floors fading → srBattle RESISTANCE throughout. Bounce lows mirror it (resistance fades, floor builds → SUPPORT). Detection lags ~1 bar on fast reversals (srBattle needs 2 confirming polls) — candidate fix: 1 poll when force swing >30.

## 8. WHERE WE LEFT OFF (next action)
Just finished saving both design mockups to `mockups/` and giving the user the links. Build sequence AGREED = **DATA-FIRST**:
- **NEXT STEP (not yet started):** (1) wire the **auto-download at session close** (Option A) + create the **scheduled end-of-day review workflow** (reads newest `daily-data/gex_*.json`, runs the LLM review focused on the Section-5 review focus, writes a dated scorecard the Analysis tab reads). THEN (2) build the **Analysis tab** shell in the script against that real review output.
- Last thing said to user: asked whether to proceed with the auto-download + review workflow now. User instead asked to SAVE STATE for a new window. So the immediate resume action is: **confirm we're doing the auto-download + scheduled review workflow (Step 1 of data-first), then start building it.**

## 9. WORKING STYLE / TONE
- User is expert; discuss design before coding when he says "don't code yet" / "explain" / "discuss." When he says "go with your recommendation," pick the strongest option and proceed.
- Always keep pieces COHERENT (his recurring theme): King gives the lean → price follows w/ breakdowns → S/R gives pullback entry → bar shows resistance>support → nodes show accumulation. Node scoring must consider MULTIPLE nodes/states (resistance building overhead while support dissipates).
- Deploy = give a readable URL + "replace the script and hard-reload." Keep changelog + this resume note current.

---

## SESSION UPDATE 2026-08-12 (v10.21 shipped)

WHAT HAPPENED THIS SESSION:
- Loaded gex project; found current was v10.20 (resume note had drifted to v10.19). Backfilled the missing v10.20 changelog entry.
- User uploaded a v10.19 file (stale, byte-identical to our v10.19 release) and the 8/11 gex json (byte-identical to daily-data; it's the FLAWED v10.15 capture — corrupt king exposure, sig on only 7/88 snaps). Gave user the v10.20 install link, then built forward.
- WENT THROUGH THE SKYLIT DOCS with the user page-by-page (Core Concepts, How-to-Read 5-step, Best Practices, Limitations, Pitfalls, FAQs, Patternpedia: Whipsaw/Rainbow Road/Gatekeeper/Decoy case study/Trend/Rug; did NOT read Ten Commandments or web-app nav). Full design synthesis captured in /GEX-Signal-Tapereader/DOCS-DERIVED-SPEC.md — READ THAT FIRST next session.
- KEY DATA CORRECTION: VEX is ALREADY hooked (feedTypeFromUrl handles data_type=vanna -> LASTVEX{SPY,QQQ}) but LASTVEX is a dead-end (never read, never persisted). Atlas toolbar has GEX/VEX/combined/derived toggles. Skylit ALSO moved its feed to SSE (/api/stream?...data_type=gamma) + a Trinity system pulling SPY/QQQ/SPXW/VIX — our polled fetch/XHR hook is likely obsolete on live tape (panel showed "SPY:tape" fallback at EOD; user confirmed panel works). SSE migration is a v10.22 must-do.
- Set a Google Calendar reminder (draft) for 8/12 9:30am CT: "Create Skylit API key (Developer tab)". User will look into the API tomorrow.

BUILT + SHIPPED v10.21 (feed-independent, all validated on real 8/11 data):
- A_* analytics core spliced in ~line 4489: A_kingBehavior, A_accumEdge, A_combinedEdge, A_regime, A_day/A_num/A_pct/A_sideOf, A_tip. A_renderTop() renders the coherent top; A_edgeRow() renders edge rows.
- Regime classifier validated: 8/11 => Whipsaw (769-771 core, 7up/7dn rolls, reach 36%, not pinned/late). Fade support->down = 64% (swing 73%, +19 vs base) is the standout real finding.
- LOADER: __gptsDebug.loadDay(json)/loadReview(obj)/clearLoaded() + in-tab "Load day" button; all analysis render fns routed through A_day() so a loaded past day renders. Legacy 7-step scorecard preserved below as "Signal scorecard & review". Grade badge shrunk + demoted under the regime chip.
- Tests in sandbox (EPHEMERAL) + copied to /GEX-Signal-Tapereader/: analytics_v1021.js, test_analytics.js (PASS), test_render.js (RENDER-OK 10/10), verify_injected.js.

DESIGN DECISIONS LOCKED WITH USER:
- Tab = ADAPTIVE (declares regime, shows regime-appropriate metrics; range days flag directional edges as low-signal). Context-NOT-signals posture (Best Practices). Win-rate != edge -> always show MFE/MAE payoff. Honest degradation (insufficient / range-day caveats), never fabricate.
- Coherence tooltips = option A (role-in-the-story, not definitions).
- Sequencing: v10.21 DOM-now (DONE) -> v10.22 API/SSE (VEX persist+overlap, tri-confluence gate SPX/SPY/QQQ "all agree or stand aside", gatekeeper/velocity/node-class, Rug tri-confluence, SSE migration) -> v10.23 product (live Dashboard regime banner, Rug-forming alert, multi-day topping/bottoming).

NEXT STEP (immediate resume action): TOMORROW during LIVE market hours — (1) live-diagnose whether the feed hook still gets data or needs the SSE migration (can't tell from a dead EOD tape); (2) capture a real data_type=vanna payload to lock VEX node shape; (3) then build v10.22 (VEX persist + overlap + tri-confluence) on confirmed live data. Also: verify next clean export shows king as a real strike (769-776) + populated sig every bar (the v10.20 fix working).

RUG SETUP note (project namesake): detectable in v10.21-era data because node.tp keeps gamma SIGN (yellow +gamma above / purple -gamma below + no floor = rug). Needs clean v10.20+ capture to trust tp. Tri-index rug form = v10.22.

---

## ⭐ TOMORROW'S BUILD DIRECTIVE (user's explicit instruction 2026-08-12 EOD)
User said: "make sure both are built out tomorrow and you are incorporating what you learned from the Skylit docs — that is WHY I shared them: to update the application AND the stats." So tomorrow is NOT optional polish; it's the core deliverable.

BUILD BOTH (from LEARNING-SPEC.md §12 — one detector, two surfaces):
1. **Dashboard READ** must NAME the regime + FLAG forming patterns LIVE (Rug/Reverse-Rug/Gatekeeper/Whipsaw), conservative + "forming" qualifier.
2. **Analysis tab** must carry pattern STATS (Rug, Reverse-Rug, Gatekeeper, cluster/double-stack): occurrence count + forward hit-rate vs baseline + MFE/MAE + n, pooled across days.

MANDATORY READS AT START OF NEXT SESSION (in order):
- **LEARNING-SPEC.md** ← authoritative Skylit-methodology→implementation map. Every new metric/pattern/READ line must trace to a section here. This is the doc the user specifically asked to be kept updated with what was learned.
- **DOCS-DERIVED-SPEC.md** ← the build-split roadmap.
- Then this resume note's SESSION UPDATE 2026-08-12 + WHERE WE LEFT OFF.

SEQUENCING (LEARNING-SPEC §13 TODO): (1) LIVE-diagnose feed during market hours; do SSE migration if the hook is stale (prereq for a trustworthy live READ). (2) Write detectors ONCE (Rug/Reverse-Rug/Gatekeeper+strength-ratio+decoy-discount, cluster/double-stack). (3) Wire detectors into BOTH the READ (live naming) and the tab (pooled stats). (4) VEX persist+overlap, freshness/touch-count weighting, session-phase/OPEX/event flags, tri-confluence gate. Each new stat gets: hit-rate vs baseline + MFE/MAE + n + honest "insufficient" when small.

REMINDER: also keep LEARNING-SPEC.md updated as more is learned (incl. reading the two unread doc pages: Ten Commandments, web-app nav). And create the Skylit API key (calendar reminder set 9:30am CT) — unlocks SPX/velocity/node-class/VEX-clean.
