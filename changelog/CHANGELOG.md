## v10.52 — 2026-08-18 — end-to-end PIPELINE INDICATOR + automatic review read-back

**Why.** There was no way to see whether a day's data reached the nightly review, or whether a review came
back. Two real breaks: v10.50's footer redesign silently DROPPED the old `saved ✓` text, and the panel
could only receive a review through a console call, so a review sitting on GitHub never appeared.

**Footer is now a 4-stage pipeline** (replaces the 3 health dots; feed/vex liveness folded into the `rec`
hover, nothing lost): `● rec · ● saved · ● pushed · ● review`, each a coloured dot with a question-first
hover. green ok · amber warn · red bad · grey unknown.
- `saved` distinguishes **repo-folder save** from **download-only** (`dl`, amber) — a download never reaches
  the repo, so the review would never see it. That state is now persisted so it survives a reload.
- `pushed` = today's `data/<date>.json` is on GitHub. `review` = last session's review exists AND parsed.

**CT-vs-UTC date bug fixed.** `saveState()` compares against the CHICAGO trading date. Comparing to the UTC
date produced a false "not exported today" on 2026-08-17 (8pm CT = next-day UTC).

**Automatic review read-back.** `pipeCheck()` (max one remote check per 10 min, cached in `gpts_pipe_v1`,
skipped when the tab is hidden, fail-soft) fetches `review/<lastTradingDay>.json` from the raw URL, parses
it into `ANALYSIS_REVIEW`, and re-renders — replacing the manual `__gptsDebug.setReview` call. Analysis ⑥/⑦
and the pre-open brief now fill themselves. 404 falls back one weekday.

**Also:** `gex` skill gains a **REVIEW** procedure (the nightly methodology + honesty rules + a delivery
cascade: device bridge → Google Drive transport → chat), so the scheduled task just invokes the skill and
the method lives in one versioned place. `review/` created with a README. Nightly rescheduled to 06:33 CT.

**Tests:** test_pipeline_indicator.js (105) — saveState classification incl. the CT/UTC case, pipeCheck
caching + fail-soft, stage colours, one-line render. Suite green except the 4 pre-existing stale.

## v10.51.2 — 2026-08-18 — Steps 1-5 restored (small clickable line above the King badge)

v10.50 retired the ①②③④⑤ icons and folded their doctrine "into element hovers" — but the STEP_TEXT
content (all five Skylit method pages) was left in the file with NO CALLER, i.e. unreachable dead code.
The per-element hovers explain individual elements; they do not carry the 5-step framework, which the
project docs call the app's governing workflow.

Restored as a tiny line ABOVE the SUP/King/RES cluster: `Steps 1 2 3 4 5`, 8px, each numeral separately
clickable and opening ITS OWN popover through the existing `.gs-ico` / `data-gstep` delegation in
`wireStepIcons()` (machinery was never removed). Deliberately on its own row so the pill alignment below
is untouched. Hover names the step (1 Magnets · 2 King · 3 Range · 4 Gatekeepers · 5 Flow).

## v10.51.1 — 2026-08-18 — drift bar FIX: correct scale + two readable lanes

**Bug (found live).** The bar drew both ±1σ bands across the **Flr..Ceil** domain — which does not contain
them. On 2026-08-18 (GEX band 769.04-771.68, σ 1.32; VEX centre 774.16; price 772.68) that clamped VEX to
0-100% and GEX to 66%, so the two overlapping 50%-opacity bands smeared into one wash and only the white
price tick was legible. The READ was correct (SPLIT — gamma centred below price, vanna above); the DRAWING
was wrong.

**Fix.** Domain is now the UNION of both bands and price, padded 8% — so the bands always fit and their
relative positions are true. Same live numbers now render GEX 6.9-39.8% (left of price), VEX 48.3-93.1%
(right), price 52.2% — a readable split picture. Bands moved into TWO STACKED LANES (gold GEX on top,
purple VEX below, 3px each) instead of overlaying in one lane, each with a brighter centre tick for its
VWAP, and the white price line spans both lanes. Per-lane hovers name the band and centre.

**Also confirmed live:** VEX capture IS working (VVWAP 774.16 with the v10.49 auth self-fetch) — the earlier
"LASTVEX null" reading was a probe artifact (LASTVEX is not exposed on __gptsDebug), not a capture failure.

Suite green except the 4 pre-existing stale (node_identity, node_role_badge, nodemap, tapeking/jsdom).

## v10.51 — 2026-08-18 — Direction engine: SMA-50 PRIMARY, GEX/VEX drift CONFIRMS or DIVERGES

Replaces the v10.50 weighted-sum lean with a HIERARCHY (user-directed). The 50-SMA five-state machine IS
the trend; drift never chooses direction — it grades confidence.

**Confirmed trend (`up`/`dn`):** direction = the trend, always. Base score 3 (a confirmed trend alone is a B).
Drift agreeing adds +2 (AGREE-*, bands overlap) or +1 (LEAN-*, same side no overlap) → `confirmed`.
Drift opposing subtracts 2 AND hard-caps the grade at **C** → `divergence` (price up while the book leans
down is a caution, not a strong read). Drift SPLIT/NONE → `trend-only`.
**No confirmed trend (`flat`/`up-broken`/`dn-broken`/`na`) — TENTATIVE (user choice):** drift supplies a
PROVISIONAL lean so the panel still reads on rangebound days, but the grade can never exceed **C**. The two
broken states vote 0 for now — we do not yet know whether a broken uptrend continues or reverses; the
recorder will answer that.
All v10.50 hard caps preserved: mid-range → C, chop → C + SIDE, SIDE can't be A, power/open-drive cap odds.

**READ** gains relation-aware wording: confirmed / divergence ("Uptrend, but GEX and VEX lean down —
divergence, lower confidence") / tentative / trend-only. Direction hover rewritten question-first.

**Recording (feeds the future weight optimizer, changes no behaviour):** `dir.trend5` records the FULL
five-state value (uncollapsed) so up-broken/dn-broken earn their own measured hit-rates; `dir.drift`;
`dir.relation` (measures whether the hierarchy beats trend alone). Recorded but NOT voting: `dir.struct`,
`dir.kingRoll`, `netGamma`, `dir.trendFast` (SMA 10 + 20, so windows can be compared empirically).
**FCHIST** (`gpts_flrceilhist_v1`) starts sampling Flr/Ceil strikes per bar so multi-session ROLLING becomes
computable later — rolling is NOT computed or voted yet (Academy: rolling is day-over-day across map
updates; 2 consecutive = signal, 3 = confirmation).

**Analysis — "Direction factors"** table: rows per five-state, per drift verdict, and per relation, each with
n · rate · **vote split ↑/↓** · baseline-adjusted expectation · lift · MFE/MAE, plus a `⚠1-way` flag. The vote
split is mandatory: on 2026-08-11 structure voted DOWN 46/49 on a down day and would otherwise have looked
like 71% edge. Verified against that artifact — the row reads 100% but lift +6 and flags one-way.

**Tests:** test_dir_hierarchy.js (80) — drift never flips a confirmed trend, divergence → C, tentative → C,
caps intact; test_direction_grade rewritten (68); test_feature_enrollment (613). Suite clean except the 2
pre-existing stale (node_identity, node_role_badge) + 2 environmental crashes (nodemap, tapeking/jsdom).

## v10.50.1 — 2026-08-18 — trend confirmation 16/20 → 15/20

`TREND_DOM` 16 → **15** (user). 15 of 20 closed 3-min bars on one side of the continuous SMA-50
(±0.25 ATR band) now confirms a directional trend. This also corrects a long-standing comment error:
the old constant was documented as ">=75%" but 16/20 is 80%; 15/20 is the true 75%. Slightly earlier
trend confirmation, so `up` / `dn` are reached sooner and the `up-broken` / `dn-broken` transition
states trigger on a 15-bar loss of dominance. Comments + test_sma_cont global synced. Suite green
except the 5 pre-existing stale.

## v10.50 — 2026-08-17 — DASHBOARD REDESIGN: one voice per decision, exceptions not defaults

Full element-by-element review of the whole dashboard, implemented. Display-only redesign — all
data/analysis/testing/learning enrollment intact (test_feature_enrollment 429✓).

**READ — single direction voice.** Direction grade merged inline with the verdict (`↑ BULLISH B`, no ⚖).
New 3-beat sentence style: WHERE · STATE+LEAN · POTENTIAL — e.g. "At King 773. Support building with GEX
and VEX leaning up. Potential bounce to 776." Templates for bounce/reject/cont/split (test_read_voice 14✓).
Invalidation off the READ (on the decision line). Regime no longer shown (input to the grade, in its hover
only). Removed: standalone direction line, readWhy block, legacy BULLISH/BEARISH body, "↩ King behind" line.

**DRIFT — one line + thin bar.** ~7px bar under the line: gold GVWAP±σ, purple VVWAP±σ, white price line.
Hover simplified ("Which way do GEX & VEX lean? Both above price — supporting higher prices.").

**DEFLECTION ZONES — the single ladder.** Legacy Node-Map ladder retired; zones are the one node list.
In-play card = 3 rows with the DECISION folded into row 3 (`bounce play · entry 773 · tgt 776(air) · inval
<772`) + TAKE/PASS gated to real setups (grade≥B, cell≠stand-aside). Polarity = colored `g` (yellow +γ /
purple −γ). Reaction = ✓/✗. Confluence = S/Q/V (SPXW added, display-only from the trinity header, honest S–
when absent). Acm horizons renamed 15m/session. ACTIVITY tag (Pull/Push/Defl/BO·FT) folded onto every row.
%King from the gamma feed. Dropped: sparkline, "· px", the gray Dir/Node-grade legend, grade tier ⚖ (→ hover).

**RETIRED:** node-map sentence, legacy ladder rows, duplicate in-map ★SUP/👑/★RES header, step icons ①–⑤
(doctrine folded into element hovers), regime chip, legacy "Deflections" list, snapback line, air-pocket
line (→ `(air)` tag on tgt AND inval), range chip (→ `⚠ OUT · range redefining` exception flag only).

**KEPT as exceptions/health:** session badge (highlight power/OPEX), model-heat (cold-only), pre-open brief
(one line), footer = 3 health dots (feed · vex · rec) + version. Question-first hovers on every element.

**Tests:** +test_read_voice (14), +test_zone_row (24); updated test_read_v1047/layout_2col/accum_canon
(m15/session)/node_grade/feature_enrollment. Suite green except the 5 pre-existing stale.
**Verify live (next open):** the 3-beat READ, drift bar, single graded ladder with folded decision + take/pass,
✓/✗ reaction on a tap, colored g, S/Q/V, footer health dots.

## v10.49.1 — 2026-08-17 — coherence fixes (live-verify caught 3)

**1 · Drift SPLIT bug.** `driftRead` required same-side-of-price AND band-overlap → tight/offset bands
forced a false SPLIT even with both centres above price. Now: SAME side = AGREE on direction (dir set);
band-overlap only decides conf (`UP·conf`) vs plain lean (`UP`). SPLIT reserved for opposite sides.
Regression pinned in test_drift_read (same-side disjoint → LEAN-UP, opposite → SPLIT).

**2 · Two-voice READ.** `readBlock44` computed its OWN BULLISH/BEARISH verdict, contradicting the spine
head (DN vs BULLISH on the same panel). It now takes the verdict word from `directionGrade` (the spine)
when available; legacy lean-based verdict only as fallback (unit-test scope). One direction voice.

**3 · King graded C.** `nodeGrade` ignored magnitude, so the dominant +γ King scored C on tap/roc alone.
Added a dominance input (Academy absolute-value rule): King / %King≥70 → +1, trivial <25 → −1.

Suite green except the 5 pre-existing stale. v10.49 → 10.49.1 (3 spots).

## v10.49 — 2026-08-17 — MENTAL-MODEL DASHBOARD: two-grade READ + decision · deflection-quality zones · full 5-layer enrollment (candidate — verify live)

**A · Auth fix (blocking).** v10.48 self-fetch 401'd: Skylit's `gex/levels` needs an in-memory `Authorization: Bearer <JWT>`.
Now captured off real requests (fetch Headers/object/Request + XHR setRequestHeader) into `LASTAUTH` and replayed by `selfFetch`.
VEX is now ACTUALLY captured continuously. Footer shows `vex ⏳` until first auth is seen.

**B · FEATURES registry (the enrollment mechanism, user rule 2026-08-17).** `registerFeature({key,label,record,outcome,fwd,questions,rule})`;
5 consumers: recorder (`snap.feat`, `recorderDay.feat[sym]`, `resolveFeatureOutcomes` idempotent/forward-only w/ MFE/MAE),
Analysis scorecards, question seeds, `RULES` (`learning/rules.json`, `gpts_rules_v1`, fail-soft), export. 11 features enrolled:
dir, drift, node, decision, acm, defl_ant, reaction, act, rshuf, roll, gateHour. `test_feature_enrollment.js` (429) enforces it.

**C · Spine.** `directionGrade` (drift·structure·range·regime; MID-RANGE and CHOP hard-cap to C) · `nodeGrade`
(polarity·tap·rocNow·rocDay·confluence) · `decisionCell` = 3×3 DECISION_MATRIX (descriptive words only). Cached per bar (`spineOf`).
Grades render ⚖ until RULES promote them 📊 (n≥20).

**D · Drift line** under the header: `↗ Drift UP·conf · G773.9 V775.0` (GVWAP/VVWAP ±σ, normalized per feed; AGREE when same
side of px AND bands overlap). Pinned to live 773 numbers in `test_drift_read.js`.

**E · Descriptive trade frame** per in-play zone: `zone 773±.25 · inval <772 · tgt 776 (air)` (vocabulary locked; tgt capped at King;
path from air-pocket/cluster). Echoed in the decision line. Nothing prescriptive.

**F · Deflection-quality zones** replace the ladder body: in-play node full (identity · +γ clean/−γ sharp · tap · GRADE; row2 =
Acm day/now · Q/V · frame), top-3 others one line each with grade; %King from the feed; `reactionQuality` chip (⚡conf/⚡weak) at the tap;
`▶ setup` anticipation when approaching a ≥B node; legend. Ladder retained as fallback when no node qualifies.

**G · READ** = head `↑ UP B · Node 773 A− ⚖` + why + DECISION line; `sessionBucket` badge (open-drive/morning/midday/afternoon/power,
OPEX) feeds dir; `modelHeat` badge (model warm/cold from last 10 resolved grades); odds ONLY from promoted rules; CHOP ⇒ SIDE/C.

**H · TAKE/PASS** buttons on the in-play zone → `recorderDay.act[sym]` (selection-quality stat in Analysis). No P&L.
**I · accumCanon** = ONE Acm source (now ≈6m, day = since open via `gpts_acmday_v1`) used by sentence, zones and nodeGrade —
Acm/Dec contradiction eliminated. **J · Pre-open brief** line (before 08:30 CT / `__gptsDebug.brief()`).
**K · Analysis** prepends FEATURES scorecards (rate·n·MFE/MAE; dir/node BY GRADE with A>B>C monotone check; 3×3 decision cells;
act selection quality); Testing seeds questions + 9 miner factors; `learning/rules.json` 38 ⚖ rules incl. KILL LIST
(kill.tap3 / kill.midrange / kill.noConf / kill.negGammaWide); `docs/LLM-NIGHTLY-BRIEF.md` (nightly, proposals only).

**Tests.** 8 new files, 696 assertions, all PASS. Suite green except the 5 pre-existing stale. `test_read_v1047` pin → 10.49.
**Verify live:** VEX captured while on GEX (`__gptsDebug.LASTVEX.SPY`); drift line; two-grade READ + decision; zones with grades;
TAKE/PASS writes; footer `feed v10.49`.

## v10.48 — 2026-08-17 — GEX/VEX dual-capture + mode-independent King & ladder

**Problem.** Skylit only sends the `data_type` for what you DISPLAY. `onFeed` routed `combined`
into the gamma cache (`LASTFEED`), and `tapeMap()` reads the DOM tape — which shows the displayed
book. So VEX display gave a false out-of-sync, and GEX+VEX display gave a unanimous-but-WRONG King
(the 780 from A.4). You could never hold clean GEX and clean VEX at once.

**Fix — capture is now decoupled from display.**
- `onFeed(sym,feed,j,viaSelf)`: `vanna`→`LASTVEX`, `gamma`→`LASTFEED`, **`combined` is ignored for
  the caches** (no more contamination). `LASTDISP[sym]` records what you're displaying (hook only).
- `selfFetch(sym,type)` + `ensureFeeds()` (5s interval, tab-visible + URL guard): self-fetches
  whichever mode the display ISN'T showing, off the last real `gex/levels` URL (same auth/query),
  throttled per (sym,type), 503s swallowed. Net ≤1 extra request/cycle. `LASTFEED` (gamma) and
  `LASTVEX` (vanna) now stay fresh continuously regardless of the toggle → VEX is always captured
  for analysis.
- `feedStructMap(sym)` builds a `kingResolve`-shaped map straight from `extractWalls(LASTFEED.j)`
  (King node 100%, others signed by polarity). `tapeMap()` returns it whenever the display isn't
  pure GEX (and as the unreadable-tape fallback), so the WHOLE panel — King, ladder %s, accumulation,
  Flr/Ceil — reads pure gamma in GEX, VEX, or GEX+VEX. The 3-vote reconciler then agrees cleanly
  (no false out-of-sync).
- Footer states it plainly when relevant: `SPY:gamma·feed (disp VEX)`.
- `FEED_STALE_MS` 60s → 12s (matches the 5s keep-alive; tightens the footer live/stale line).

**Tests.** `test_mode_king.js` (29): feedStructMap build, onFeed routing, King-from-gamma-feed under
combined AND vanna display (DOM used only in pure GEX), unreadable-tape fallback, ensureFeeds
stale-mode selection + guards. Full suite green except the 5 pre-existing stale (layout_2col,
node_identity, node_role_badge, nodemap, tapeking/jsdom). `test_read_v1047.js` version pin → 10.48.

**Verify live:** flip GEX → VEX → GEX+VEX; King stays 776 (or whatever GEX says) in all three, footer
shows `gamma·feed (disp …)` off-GEX, no out-of-sync banner. `__gptsDebug.LASTVEX.SPY` populated while
displaying GEX.

## v10.47 — 2026-08-16 — PHASE A SHIPPED (dashboard) · Phases B–D still planned

**Phase A.4 (LIVE FIX 2026-08-17 08:55 CT):** tape sync tripped all morning — Skylit now renders the strike ladder as a real `<table>`; `findTapeTable().validKingRow()` collapsed each `<tr>` to one token so the strike→$K adjacency never matched. Now validates per row (strike in td[0], King $K in a later td). Verified in-page: King 775, 50 rows, 776=97%. Also: feed was `combined` (GEX+VEX toggle) with King 780 — user must run the heatmap in **GEX** mode for the model; VEX is captured separately.

**Phase A.3 (approved mockup `mockups/gex-v10.47-dashboard-mockup.html`):** ladder STATE is plain colored text ("Acm ▲12%", no pill); LIFE is "● T2" (stage letter + taps, no circle). Mockup file added.

**Phase A.2 (same evening):** READ and Node Map sentence cut to BARE BONES per user — READ e.g. "BEARISH. Down to King 772. Gate 774.50 held 2×. Sup 771 steady, Res 773 steady. King steady, −γ. 50% at this distance 📊. Watch 773." Node Map e.g. "CONT thru Gate 774.5 → King 775.38: Dec ▼8%, King 775.38 Acm ▲12% pulling. Sup 773.25 Acm ▲19%." / "REV at Ceil 776.5: Acm ▲14%, held 1×, 777.75 Acm ▲9% behind. Sup 773.25 steady." / "DEFL ↑ off Gate 774.5: 2nd tap, Acm ▲9%." Verdict words abbreviated CONT / REV / DEFL / TBD. 3rd+ tap always flips to CONT with the warning (unless the node is already Dec).

**Phase A.1 (2026-08-15 evening, after first live look):** banner text → "⚠ Out of sync" (one short line); King pill falls back to the model King (no "Waiting on tape…" while the ladder has a King); ①②③ now tiny icons INSIDE the ★SUP / 👑 / ★RES pills, 5-STEP row removed; READ has no "READ ▸" label, compressed wording, 4-line clamp (full text + provenance in hover); Node Map sentence only renders when a node is engaged (no "NO NODE IN PLAY"), 3-line clamp, ⑤ icon rides on the column header; ladder = CSS grid 96/66/78/1fr, one line per node, nowrap, smaller pills; Flr/Ceil labels outrank Rug labels; price divider tighter; Deflections "none".

**Phase A built (candidate — verify live Monday):**
- `kingHeaderBlock()` restored above READ: ★SUP | 👑 King (+ castle-gate row) | ★RES + 5-STEP ①②③ (top slice of the unrendered `kingBlock()`; no charts, no verdict pill).
- Tape-sync gate → `syncBannerHtml()`: ONE red line "⚠ STRUCTURAL READ OUT OF SYNC WITH TAPE" (detail in hover), app renders normally underneath (was a blocking panel).
- `readBlock44()` rewritten: ONE plain paragraph. Verdict word (BULLISH/BEARISH/SIDEWAYS/TBD) from King side+strength, Flr/Ceil state, srBattle; destination + distance; gate in between + tap record (3rd-tap warning); Support/Resistance state; King "getting heavier — dealers pulling price up" / "bleeding"; ONE odds sentence (King pull by distance + hour, dropped in CHOP); ONE watch level; range-position wording (near the floor / midpoint / ceiling); "already reached once today"; purple-node notes. Regime gate: CHOP ⇒ SIDEWAYS.
- Node Map header = `nodeMapSentence()`: CONTINUATION / REVERSAL / DEFLECTED / TBD / NO NODE IN PLAY at the engaged node (fresh Defl > BOw > Push > Pull≤1.5) with the WHY in accumulating/decreasing vocabulary (locked templates), 3rd-tap flip, polarity note. Range/Trend chips + imbalance line + crossover banner removed from the header (range/pattern in hover).
- Flr/Ceil REDEFINED (Skylit): `pickEdge()` = LARGEST node per side (≥`FLRCEIL_EDGE_PCT`=40% ⚖, King excluded unless only strong node; far-edge guard `FLRCEIL_FAR`=6). Strong nodes beyond the edge = "★ Mag · next" (`isNext`).
- Gate icon everywhere = castle-gate svg (`gateSvgSm` in row pills; 🚪 removed). Pull/Push chips without toward-share % (in hover). Defl cards: "Defl · Gate/Ceil/Flr/King…", context chips = Acm/Dec %, ±γ, Nth tap (red at 3rd+).
- Tests: `test_read_v1047.js` (32) + 3 tests updated; suite green except 5 pre-existing stale tests (layout_2col, node_identity, node_role_badge, nodemap, tapeking/jsdom) that were already failing on v10.46.
- NOT in Phase A (Phase B+): reshuffle detector (RShuf chip/sentence), rolling, chart levels, MFE/MAE, gate-hour, GEX/VEX VWAP, Analysis rebuild, Testing pipeline, rules.json, nightly review.

**Design session (2026-08-16) — locked scope for the whole learning pipeline:**

Full ONE-AT-A-TIME design pass against Skylit's 5-step guide + doctrine docs. LOCKED scope, phased:
**A Dashboard**: header cluster restored (★SUP | 👑/gate | ★RES + ①②③), sync gate → one-line banner
(app renders), READ = one D-style paragraph (BULLISH/BEARISH/SIDEWAYS/TBD; locked texts), range
position as wording, regime gate (chop→SIDEWAYS, odds dropped), Flr/Ceil = largest bounding node
(Skylit) + Gate/next-target/cluster classes, Node Map header = one CONT/REVERSAL/RSHUF sentence
(accumulating/decreasing vocabulary), 3rd-tap warning, polarity in why, King-already-reached, Defl
cards with Step-5 context chips (abbrev.). **B Recorder**: reshuffle detector, rolling Flr/Ceil,
gate-defl hour, chart levels (VWAP, PDH/L/C, PM H/L, IB30, POC/VAH/VAL), MFE/MAE, regime on outcomes,
GEX/VEX VWAP computed internally. **C Analysis**: "did the dashboard tell the truth" — 6 sections in
dashboard order + Setup Performance bars; nightly LLM review OPTION 1 (no cloud token; review/ file
committed by local task). **D Testing**: question queue w/ lifecycle + auto conditional refinement +
walk-forward promotion; learning engine `learning/rules.json` read by the panel; confidence tiers;
event tag. Deferred: cross-index SPY/SPXW/QQQ(+VIX). Spec: session-state/latest-resume-note.md +
mockups/gex-v10.46-dashboard-mockup.html. Rules added: one-item-at-a-time, panel abbreviations.

## v10.46 — 2026-08-15 — derived GEX factors + Recommended-tests section (research-backed)

**Research (2 web passes, sourced):** Barbon–Buraschi gamma fragility (the one hard result: +gamma→
mean-revert/low-vol, −gamma→momentum/wide range), Ni–Pearson–Poteshman expiration pinning,
SpotGamma/MenthorQ level defs, Skylit VEX ("GEX governs range, VEX governs drift"). VEX/vanna
directional claims are mechanistic but UNBACKTESTED — flagged 📕 so the tab measures rather than trusts.

**Derived GEX factors (`deriveFactors`, recorded per bar as `snap.deriv`)** — computed from the
strike tape we already parse (pct + polarity): net-GEX sign & magnitude, zero-gamma level + regime
(pos/neg-gamma), abs gamma strength Σ|mass|, gamma concentration (HHI), above/below imbalance,
call wall (largest +γ above) / put wall (largest mass below), GEX ranks 1–6. Unit-tested. Feeds the
Testing miner as new factors; gamma-only, so it works today with no new tape reads.

**Testing tab ⑥ Recommended tests** (`RECO_TESTS` + `recoTestsHtml`): 22 curated hypotheses grouped
by theme (GEX regime, level, concentration, imbalance, accumulation, time-of-day, VEX, DTE gate,
confluence, expiration, end-of-day), each tagged 📗 evidenced / 📙 plausible / 📕 folklore, with the
data it needs and ✅ runnable-now vs ⏳ unlocks-at-open (VEX / multi-symbol / VIX-term). Sources noted.

**Tests:** `test_reco_deriv.js` (18) + suite updated — 7 files, 192 assertions, all green.
**Deferred to v10.47 (needs live/toggled tape, scheduled for Mon open):** VEX (vanna) capture from the
Skylit VEX toggle, multi-symbol full ladders (QQQ/SPXW/VIX), VIX term structure, regime GATE,
Analysis Insights block.

## v10.45 — 2026-08-15 — 🧪 Testing tab: hypothesis engine over the data repository

**Frame.** A third tab (**🧪 Testing**, `TESTING_VIEW`, `window.__gptsDebug.showTesting`) beside
Dashboard and Analysis. It reads the v10.44 IndexedDB repository (`gpts_repo_v1`) and does one thing:
measure the PAST. Everything carries an n; nothing predicts. Five blocks.

**① Question library.** The recurring hypotheses — King pulls 30m; King pull at 2 strikes; King pull
11am CT; non-King mass repels; contender ≥60% repels; Acm wall reached / leak rate; net-force —
each a rate bar with %, n, and a ⚖/📊 tag. Values come from the study store (`studyLoad`, run by
`studyRun`); 08-15 baseline until the local repo has run once.

**② Hypothesis builder.** Preset recipe chips ("King PULL zone · 11am → toward King", "Regime CHOP →
up?") each compose a WHEN→outcome query over the repo and show rate vs baseline + lift. Console API
`window.__gptsHypo({when:[{f:'kzone',v:'pull'}],outcome:'toward'})` for ad-hoc runs.

**③ Pattern miner (`studyMine`).** Auto-scans single + pairwise factor buckets (kzone, kside, hour,
regime, nearest-node state, nearest-node strong/weak) against the outcome, min n=30, ranked by lift,
with a combos-tested count and a multiple-testing caution — rows are leads ⚖ until they survive the
nightly re-run. Cached in `localStorage gpts_mine_v1`.

**④ Insights & recommendations.** A rule engine over the study emitting four buckets: what the data
says / change the product / improve the testing / hypotheses to test next.

**⑤ Data coverage strip (`repoCoverage`).** Days · bars · symbols · which fields exist and since when.

**Nightly.** The LLM review (day export → GitHub) consumes the same store, so page and review agree.

**Still pending (10.45/10.46):** the Analysis-tab **Insights** block and the **regime gate** (suppress
trend/confluence/King-verdict direction claims in chop) — both carried over, not yet built.

## v10.44.1 — 2026-08-15 — hotfix: tape finder (heatmap sidebar was stealing the match)

**Bug (user screenshot, 23:43 CT):** STRUCTURAL READ SUPPRESSED, "tape $K tag —", recurring.
`findTapeTable()` matched the `chart-heatmap-sidebar` container — it also carries "Strike" + a
$K TOTAL (e.g. `-$262,131K`) + 50+ rows — instead of the real tape column, so no King $K ROW was
found and the sync gate (correctly) refused to show a wrong anchor.
**Fix:** (a) reject `heatmap` containers by class; (b) `validKingRow()` requires the $K cell to sit
in a strike row (a strike token immediately precedes it in cell order) — a lone $K total no longer
qualifies; (c) accept the compact trinity per-symbol column fingerprint (`SPY$…King…`, no "Strike"
header) with an SPY preference. Safe for market hours (the classic ladder still matches via the
Strike+expiry branch; validKingRow passes on "775 $1,252,620K"). NOTE: after-hours Skylit renders a
collapsed column that may still not fully parse — the definitive check is at market open.

# CHANGELOG — GEX-Signal-Tapereader

## v10.44 — 2026-08-15 — MAGNET FRAME: single-column dashboard, Node Map rebuilt, data repository

**Doctrine (from the design session):** nodes are magnets — they PULL and PUSH; every indicator is
DESCRIPTIVE (field now) or PREDICTIVE (⚖ hand-set until n≥20 → 📊 measured, nightly-scored).

**UI (user-directed simplification).** King console + King path chart + projected chart REMOVED
from the Dashboard (all still computed/recorded; `kingBlock()` no longer rendered). Panel is ONE
column: **READ ▸ → ⚡ Deflections → ⑤ Node Map**; default width 690→440 (one-time migration).

**READ ▸ (`readBlock44`)** cites only measured/tagged magnet claims: King distance + ZONE
(ORBIT ≤1 / PULL 1.5–3 / OUT >3) with the toward-King rate at that distance and this hour;
Range Flr–Ceil inside/OUT with each boundary's state, episode and "walls like this held X%";
non-King mass lean (repels 📊 57%); regime tag (chop/mixed/trend, efficiency ratio). No legacy
King-verdict/confluence/Break-through claims (all ran contrarian on the 4 test days).

**Node Map (all 5 fields per the locked spec).** IDENTITY: 👑 King · 🚪 Gate · **▔ Ceil / ⛰ Flr**
= nearest strong magnet (≥`FLRCEIL_MIN_PCT`=15% of King) above/below = the live range (Step 3);
★ Mag / Mag for the rest; Sup/Res retired; roles stack (King · Flr); **−γ identity purple**
(Skylit convention) incl. a −γ King. STATE: **Acm / Dec / Steady** (Diss→Dec) + the node's ▲/▼%
vs its session open (persisted), ±15% bright; ±γ text tag dropped (color carries it). ACTIVITY:
**Pull tw% → BOw → BO·FT → Defl ↑/↓ → Push tw% → echoes** (priority fresh Defl > BO·FT > BOw >
Push > Pull > echo); only BOw/BO·FT chips ever shown (chain retired); Push off a node below price
= green. Header **Range chip**. Every chip carries the episode timeline in its hover.

**Episode engine (`nodeEpisode`, pure).** Per node: zone, toward-share (% of last 10 closes
moving nearer), last tag, crossings, state. Recorded per bar as `snap.ep`.

**FT redefined (both directions):** full-hold OR two consecutive directional closes beyond the
node with the 2nd progressing (`s.ftLenient`). Applies to the machine and the scorecard.

**Data repository.** IndexedDB `gpts_repo_v1` (unbounded; migrates the localStorage recorder once;
mirrors the last 12 snaps per bar so outcome back-fills land). New per-bar fields: `xm`
(cross-market headers SPY/QQQ/SPXW/VIX from the Skylit sidebar — confluence was untestable with
0 QQQ bars), `ep` (episodes), `rg` (regime tag). **Daily export** `data/YYYY-MM-DD.json` at 15:01
CT into the repo folder via File System Access API (📁 one-time pick) with download fallback;
`install.bat` registers a scheduled task (weekdays 15:30, run-if-missed) that commits+pushes
`data/`. Footer: `rec ● · saved hh:mm ✓ · 💾 · 📁`.

**Study module (`studyRun`)** = the 08-15 test battery as a nightly job over the repository
(King pull by distance/hour, others-repel, contender-repel, wall-reached by state, net-force,
episode Pull/Push scoring); cached for READ/hovers; 08-15 baseline (4d/391 bars) until first run.

**Fixes.** `hitKing` labeler (fired 1–2%): uses tape King strike, rejects magnitude-as-King,
counts range crossings. `parseKingDollarsK` explicit `Math.abs` (live signed $K seen);
`parseKingDollarSign` exposed as candidate polarity source. %KCH baseline persisted per
day+symbol (`gpts_kd_open_v1`) — survives reload.

**Tests:** `test_magnet_v1044.js` (36) + suite updated for BOw vocabulary — 5 files all green.
**Docs:** `docs/MAGNET-FIELD-GUIDE.html` (new). NEXT (10.45): 🧪 Testing tab, Analysis Insights,
regime gate.

## v10.44-PLAN — design record 2026-08-15 (superseded by the shipped entry above)

Design session outcome; full spec in `session-state/latest-resume-note.md`, build mockup
`design/nodemap_v1044_mockup.html`.

**UI (simplification, user-directed):** King console + King path chart + projected chart
REMOVED from Dashboard (recording continues silently, footer "rec ●"). Single column:
Deflections → Node Map. Node Map becomes the primary magnet surface:
- IDENTITY: 👑 King · 🚪 Gate · **▔ Ceil / ⛰ Flr** = nearest strong magnet (≥15–20% King
  mass) above/below price = the live range (Step 3). Roles stack. Others: ★ Mag / Mag.
  **Sup/Res vocabulary retired.** −γ nodes (incl. a −γ King) render PURPLE (Skylit convention).
- STATE: **Acm / Dec / Steady** (Diss→Dec; Acm kept — Step 5 doctrine) + per-node ▲/▼% vs
  session open (`Dec ▼29%`), threshold-colored. ±γ text DROPPED — purple carries polarity.
  STRIKE·% and LIFE unchanged.
- ACTIVITY: `Pull tw%` → `BOw` → `BO·FT` (only BO chip; chain display removed) →
  `Defl ↑/↓` → `Push tw%` → echoes (broke/held/FBO). Priority: Defl > BO·FT > BOw > Push >
  Pull > echo. **FT redefined (both directions): full-hold OR two consecutive progressing
  closes beyond.** Range chip + range-redefinition echo on Ceil/Flr break+FT.

**Data layer:** %KCH = King-$ %change vs TRUE open (persisted baseline, survives reload);
parseKingDollarsK → explicit Math.abs (LIVE-VERIFIED signed $K exists: −$27,399K; sign =
candidate polarity source). Episode engine per node (PULL → PIN/BREAKING/BLOCKED → Defl →
PUSH·after-tag/-break/-block), per-bar `snap.ep` with conditions-at-contact, nightly
`episodeScorecard()` incl. PREDICT-PUSH forecast arm, %KCh day-direction study
(checkpoints × King-position × polarity), LLM nightly review must answer why/what-preceded/
what-to-change. Everything ⚖ until n≥20 → 📊.

**Test battery (in-page, 4 days/391 bars):** net-force sum RETIRED (50%); King pulls 55% (69% at 2
strikes; 74% 11am CT, 66% 2pm), non-King mass repels 57%, contender ≥60% repels 58%, Acm walls
reached 15% vs Fading 23%; legacy King-verdict/conf/Break-through ran contrarian → regime gate
(10.45). hitKing labeler bug found (1-2% fires) → fix. ADDED to 10.44: DIST→ZONE (ORBIT/PULL/OUT)
+ hour gate; READ ▸ citing measured rates; IndexedDB data repository + daily JSON export + QQQ/VIX
recording; test battery as nightly module. v10.45: 🧪 Testing tab (question library, hypothesis
builder, pattern miner, insights/recs, coverage) + Analysis insights + regime gate.

**Shelved with return-spec:** ATTRACTION tile v2, %KCH tile flip, King charts (return validated
once scorecards mature). Net-force chip retired on evidence.

## v10.40 — 2026-08-14 — KING PATH v2 (Batch 2): analyzer + narrative-first layout + gutter

**LAYOUT (approved mockup).** The "KING PATH · today · drift · rolls · verdict" header row
is REMOVED. The narrative — old bottom verdict line, expanded into the King Analyzer read —
now leads the section. Drift + rolls moved INSIDE the chart (top-left overlay chip), the
verdict pill to the top-right overlay, session times to the bottom-left overlay. Net: one
full row saved, chart effectively taller.

**GUTTER (the label-collision fix).** `kingSparkline` reserves a 46px right gutter
(padR 4→46, W 236→262) with a divider. The price PILL and the 👑King label render in the
gutter at their own Y (anti-collision separation kept), with tick marks into the plot. The
dashed price line ends at the plot edge — it can never run under its label again. The
signed offset-vs-King stays (small line under the pill; test_kingpath enforced it).

**KING ANALYZER (`kingAnalyzer` + `kingReadHtml`).** Descriptive line: King strike,
polarity (+γ friction / −γ fuel dealer mechanics), K$ magnitude + session change, distance,
eVA value band + inside/outside. Predictive line, priority-ordered and TAGGED
(⚖ Academy / 📊 measured-with-n): Overshoot→Beach-Ball watch ⚖ · SUCCESSION WATCH
(contender ≥60% → 📊 76% King rolls to it within 20 bars, n=148) · gravity gate (≤3
strikes; beyond → pull explicitly "unsupported" 📊) · approach/ETA (📊 63% vs 47%) with
POWER-phase PIN WINDOW ⚖ · outside-value imbalance = don't-fade 📊 (n=25) · K$
bleed/build ⚖. Chips: phase+mins-to-close, taps·crossings, succession, K$Δ, QQQ King
alignment ✓/✗ (feed-derived). Provenance footer names the 4d/324-bar base.

**DRIFT DEMOTED.** 3-bar King drift tested 50.0% vs next-30m direction (n=68) — coin flip.
It survives as a DESCRIPTIVE overlay chip only, and says so in its tooltip.

New pure helpers (all unit-tested): `evaBandFromPct`, `successionFromPct`,
`kingTapsCross`, `sessPhaseCT`, `kingApproach`. `KD_TRACK` follows session King-$
(first/last/peak). Analyzer is null-safe: missing inputs suppress claims, never invent.

**Tests:** `test_king_analyzer.js` (26 assertions: helpers + layout/demotion guards).
test_kingpath caught the dropped offset label — restored. Full suite 25/26 green.
NEXT (Batch 3): Analysis-tab King stats recomputing the backtest tables nightly.

## v10.39 — 2026-08-14 — KING DATA LAYER (Batch 1) + indicator backtest results

**BACKTEST FIRST.** Before choosing which King indicators to build, all candidates were
tested in-page against the 4 recorded days (324 usable bars, out10 = next-30m outcome).
Small n — this RANKS candidates, it does not validate them. Results:

| Candidate | Result | Verdict |
|---|---|---|
| Contender >=60% of King | King rolled TO THAT STRIKE within 20 bars **112/148 (76%)**, median 4 bars | BUILD — headline ("King Succession Watch") |
| Convergence velocity | approaching -> **63%** continue toward King (n=161) vs receding 47% (n=148) | BUILD |
| Distance gravity | toward-King edge at <=3 strikes (54/59/60%), FLIPS beyond (47%, 0/3 at 5+) | BUILD as a <=3-strike gravity gate |
| eVA (70% exposure band) | inside -> 57% rotation (n=260); **outside -> continuation, NOT reversion** (revert 36%, n=25) | BUILD — outside-value = imbalance, do-not-fade read |
| King drift (3-bar) | next-30m direction agreement **50.0%** (n=68) — coin flip | DEMOTE to descriptive-only chart chip |
| Naked Kings / IB×King / one-way rolls | supportive anecdotes (n<=4 days) | TRACK, grade after data accumulates |
| K$ momentum / polarity | UNTESTABLE — never recorded | THIS BATCH fixes that |

**SHIPPED (data layer only, no UI change):**
- `recNode()` now records **`pos`** (gamma polarity) and **`abs`** (magnitude) per node —
  the long-committed KEYSTONE. Unblocks Academy Art.4 (net-gamma regime), Art.7
  (day-over-day rolling), Art.9 (real-vs-hedge), plus contender/K$ backtesting.
- New `parseKingDollarsK()`: the King row's dollar figure (e.g. `$996,886K`) is now
  PARSED, exposed as `tapeMap().kingKd`, and recorded per bar as snapshot **`kd`**.
  Live 2026-08-14: K$ bled $1,397,016K -> $996,886K (−29%) intraday while price stalled
  below — the strongest leading signal on the board, previously discarded.
- Captured in BOTH tape paths (tr/td and div-grid), null-safe, guarded.

Taps/crossings/dwell, IB, HOD/LOD, eVA and naked-King ledgers are DERIVABLE offline from
the already-recorded px/tking/nodes series — deliberately NOT duplicated into capture.

**Tests:** new `test_king_data.js` (14 assertions incl. sync-guards on both capture paths).
Full suite 24/25 green (test_tapeking needs jsdom). Batches ahead: B2 = King Path v2 UI +
analyzer (approved mockup, drift demoted, VIX-confirm chip pending ladder spike);
B3 = Analysis-tab King stats recomputing these tables as n grows.

## v10.38b — 2026-08-14 — L0B TAPE RECONCILIATION: consensus gate, fail-closed display

Follow-on to the v10.38 King fix. The parse defect was fixable; the ARCHITECTURE that
let it ship silently was not addressed by fixing it. One parser was the sole authority
on the King, so a single defect inverted the structural anchor with nothing to contradict it.

**THREE INDEPENDENT PATHS.** The King is now derived three ways that fail differently:
1. `kingFromTapeTag()` — Skylit's own `$K` marker in the rendered DOM
2. `kingFromFeed()` — largest `|v|` in the raw network payload
3. `kingFromTapeMax()` — largest `|%King|` in the parsed tape map

A parse bug breaks (3) but not (1) or (2). A stale feed breaks (2) but not (1) or (3).
A Skylit DOM change breaks (1) and (3) but not (2). **No single fault can take a majority.**

**CONSENSUS REQUIRED.** `reconcileVotes()` (pure, 47 assertions) needs >=2 agreeing paths.
Outcomes: unanimous / majority / no-consensus / single-source / no-source. A single source
is explicitly NOT consensus — it cannot corroborate itself.

**FAIL-CLOSED DISPLAY.** `render()` calls `tapeSync('SPY')` before any %King-derived block.
Without consensus it renders `outOfSyncBlock()` — naming the reason and showing all three
votes side by side — INSTEAD of the King badge and Node Map. Structural data is now
suppressed rather than shown wrong. Aligns with LEARNING-SPEC S0: "NEVER fabricate a
number the data can't support." A confident panel built on a wrong anchor is exactly that.

**PARSE INVARIANTS FEED THE GATE.** A `kingConflict` from `kingResolve()` forces the gate
closed even when the three votes agree — a flagged parse is never treated as healthy.

**RECURRENCE TRACKING.** `RECON_STATE` counts consecutive failures per symbol. At
`RECON_FAIL_ESCAL` (3) the fault is marked RECURRING and the panel says so. A bounded log
(20 records) retains reason + all three votes + timestamp per failure so a repeating fault
is diagnosable rather than guessed at. The streak resets on any healthy read.

**OPERATOR DIAGNOSTICS.** `__gptsDebug.syncReport()` returns verdict, all three paths,
agree/disagree lists, streak, recurring flag and the full failure log.
`__gptsDebug.setTapeGate(false)` reverts to legacy behaviour if the gate ever misfires;
`CFG.tapeGate` defaults true.

**PROOF.** `test_tape_sync.js` replays the real 2026-08-14 board — tag 780, feed 780,
broken parser 775 — and asserts the reconciler returns **780 with the parser still
broken**, flags the dissenter by name, and never crowns 775. This layer would have
caught the incident on the first render.

Full suite 23/24 green (test_tapeking skipped — needs jsdom).


## v10.38 — 2026-08-14 — CRITICAL: tape/tapereader King desync fixed + negative-gamma strikes recovered

**Found live during market hours (SPY, 09:30 CT).** The panel was reporting `King 775 · 42%`
while the Skylit tape tagged **780** with `$1,252,620K` and the feed independently showed 780
at 3.71e9 vs 775 at 8.49e8 — 780 dominant by 4.4x. The structural anchor was inverted, and
it was being RECORDED that way.

**ROOT CAUSE (two defects in the same function).**

1. *Cross-expiry read.* The King row prints the DOLLAR figure instead of `100%` (King ==
   largest absolute exposure == 100% by definition). `firstStrengthPct()` returned null on
   that cell, and the loop then fell back to `cells[2]` — a DIFFERENT EXPIRATION COLUMN —
   assigning the King the next expiry's 3-4%. `kingResolve()` saw 775 at 45% beating the
   "4%" King and fired `maxpct-override`, crowning the wrong strike. Every downstream read
   inherited it: node roles, %King normalisation, target ladder, gatekeeper geometry,
   regime, READ narrative.
2. *Signed %King discarded.* %King carries gamma POLARITY. `firstStrengthPct()` accepted
   only UNSIGNED values, treating every signed one as a change chip — so EVERY
   NEGATIVE-GAMMA STRIKE was silently dropped from the tape map (774 `-1%`, 777, others).

**FIX.**
- New `tapeCellPct()` replaces `firstStrengthPct()` in the tape reader. Takes the FIRST
  percentage in a cell as %King (sign preserved); any later percentage is the growth chip
  and is ignored. A cell containing `$K` returns 100 unconditionally.
- Path A never reads `cells[2]`. One column, one expiry, no crossing.
- `kingResolve()`: **the `$K` tag is authoritative and can no longer be demoted.** The
  `maxpct-override` branch is removed. If a parsed percentage disagrees with Skylit's own
  King tag, the PARSE is wrong, not the tag. `maxpct` remains only for the no-tag case.
- Two hard invariants, both flagged rather than silently absorbed:
  `king-not-100` (tagged King didn't parse to 100) and `rival-at-or-above-king`
  (some other strike met or exceeded 100). Invariant 2 scans every strike, not just the
  max — a tie leaves the max pointing at the King itself and would slip through.

**TESTS.** New `test_tape_king.js` — 37 assertions, fixtures captured verbatim from the
live Skylit DOM. Includes SYNC-GUARDS that fail if `cells[2]` fallback or `maxpct-override`
is ever reintroduced. Verified end-to-end against the real 2026-08-14 board:
King 780 @ 100%, 775 @ 45% (not King), 777 present, 774 retained at -1.
Full suite 22/22 green (test_tapeking skipped — needs jsdom).

**NOTE.** Recorded snapshots store both `king` (feed) and `tking` (tape), so days captured
before this fix are recoverable — the raw values were never lost, only the resolution.

**DOC GAP.** `design/architecture-design.md` Layer 0A described the tape bridge without ever
specifying the cell layout (two percentages: %King then growth; King row prints dollars).
That omission is why the bug survived. Should be documented there.


## v10.37 — 2026-08-14 — King badge carries gatekeeper · Deflections one-line strip · Gatekeeper section removed

**King badge redesign (kingBlock):** the single gold pill is now two stacked rows.
- TOP: 👑 crown + King strike + signed offset vs price ('+2↑' / '−3↓', green above / red below).
- BELOW: white gate icon + GATEKEEPER strike + its signed distance from price (was the SPY price).
  No gatekeeper => a dimmed gate + '–' placeholder (keeps row height; means clear path to King).

**Gatekeeper section REMOVED:** the standalone gatekeeperBlock() render call and the inline
'🚪 Gatekeeper …' Node-Map header line are both gone — that info now lives in the King badge.
(gatekeeper() detector + gatekeeperBlock() function are left defined but unused, for safety.)

**Deflections strip redesign (deflectionBlock):**
- Header collapsed to ONE line: '⚡ Deflections  N live' (saves vertical space for the Node Map).
- Removed the 'unlock n≥…' message entirely.
- Cards now sit in a HORIZONTAL scroll strip, NEWEST-LEFT (sorted by fewest bars-since-tap),
  strip starts scrolled fully left.
- Per-card data: node type (setup name) + strike + direction + confluence chips.
  Grade stays HIDDEN ('● rec nX') until the setup crosses its auto-tuned unlock sample size.

**Tests:** full suite 22/22 green. Mockup: mockups/king_badge_and_deflections_v10.37.html.

## v10.36 — 2026-08-14 — Deflection Signals section (above Node Map) + honest data-earned grading

**New section: ⚡ Deflection Signals** — rendered ABOVE the Node Map (accumBlock).
A deflection = price taps a node and reverses away (reuses deflectionAt, >=DEFLECT_CONFIRM bars).
Rows are sorted by setup priority and show: strike, direction, setup name, confluence chips, and a grade.

**Setup classification (classifyDeflection):** King(90) > Gate(85) > Rug/Reverse-Rug(80) >
Pika/Barney(60) > Floor/Ceiling(50). The BO·FT-retest flavor (breakout+follow-through, pullback
back to the node, then deflect) STACKS on top (+8 prio, ⭑ marker) — the user's marquee case.
FBO (false-breakout) flavor tagged via nodeOutcome.

**Honest grading (data-earned, not predicted):**
- recordDeflections(sym) — logs each NEW confirmed deflection into recorderDay.defl[sym],
  de-duped per (setupKey@strike) within the forward window. Wired into the snapshot cycle.
- labelDeflectionOutcomes — after DEFL_FWD_BARS (10) closed bars, marks whether price CONTINUED
  in the deflect direction by >= DEFL_CONT_PTS (0.30 strikes).
- deflStats — aggregates per-setup continuation rate across all recorded days.
- Grade is HIDDEN (dashed "● recording  n=x/N") until the setup crosses its unlock sample size.
  Unlock N is AUTO-TUNED from observed daily setup volume (deflUnlockN ~= 3 trading days, floor 5,
  cap 25) — recommended after ~2 weeks of real data rather than guessed up front.
- Grade thresholds: A+ >=75% · A >=68% · B >=58% · C >=45% · D <45%.

**Tests:** test_defl_signals.js added (28 assertions: unlock-N, grade thresholds, classification
priority + BO·FT/FBO stacking, forward continuation scoring, stats aggregation). Full suite 22/22 green.

**Mockup:** mockups/deflection_signals_mockup_v10.36.html (3-panel: day-1 hidden grade,
matured earned grade, Analysis performance bars).


Implements the mocked & approved Node Map redesign (mockups: nodemap_final_redesign,
nodemap_redesign_v2_with_bo_outcome).
• 4 fixed ZONES per row, column headers up top so rows align:
  1. IDENTITY — new nodeRolePill(): ONE pill merging role ICON (★/👑/🚪/🧶) + WORD
     (King/Gatekeeper/Rug/Pika/Barn/Flr/Ceil). Kills the old duplication (icon in strike
     column AND a separate word badge on the right).
  2. STRIKE·% — merged ("775 · 23%"), King strike gold.
  3. STATE — Acm/Diss/Steady + γ polarity grouped.
  4. ACTIVITY·LIFE — ONE event by priority: DEFLECTION (Defl ↑/↓) > live BO chain
     (BO·FT·TST·CONF·GO) > resolved outcome (broke↑/↓ / held / FBO) > attracting; PLUS the
     lifecycle DOT.
• Lifecycle tag → compact DOT (single letter T/U/D; Fresh shows nothing; % + note in hover).
• Tests: nodeRolePill assertions added to test_node_role_badge.js. Suite 21/21 green.

NEXT (user-requested, NOT yet built): a dedicated "DEFLECTION SIGNALS" SECTION — a
standalone panel aggregating all confirmed deflection events across detectors (King
deflection, Gatekeeper deflection, Rug/Reverse-Rug, Floor/Ceiling), each with the node
type, direction, and CONFLUENCE (e.g. BO + pullback + deflection at one strike = stronger).
The per-node deflection detection (v10.34) + this v10.35 activity zone are the inputs;
the section is the digest of "what just became tradeable."

## v10.35 — 2026-08-13 — Node Map row REDESIGN (4 zones)

## v10.34 — 2026-08-13 — DEFLECTION detector + FBO relabel

SOURCE OF TRUTH: Skylit Academy (execution-doctrine: "enter at the direct tap, deflection
plays out"; core-concepts: magnet/deflection).
• NEW deflectionAt(): a DETECTED reversal off a node — price taps within DEFLECT_ZONE
  (0.50 SPY/QQQ; docs ±0.50 / ±5 SPX), reverses away by >=DEFLECT_AWAY(0.45), sustained
  >=DEFLECT_CONFIRM(2) closed bars. Reports {dir:+1 bounce-off-floor / -1 reject-off-ceiling,
  awayPts, bars, pos}.
  - EVENT REPORT, NEVER a prediction — fires only AFTER the reversal confirms. Honors the
    locked honesty red line (the app never predicts deflect/break).
  - Multi-bar confirmation per BUILD-PLAN (a one-bar 'did it reject' test mis-calls break-then-reverse).
  - Polarity FLAVOR (not a gate): +gamma node = deflection expected; -gamma = counter-character (noted in hover).
  - Distinct from BO outcomes: deflection = clean bounce/rejection; 'held' = never broke; FBO = broke then reversed.
• UI: DEFLECTION takes PRECEDENCE in the Node Map Activity zone ("Defl ↑/↓" + hover naming
  the node: King/Gatekeeper/Floor/Ceiling). Rationale (user): a setup is "meaningless until
  there is a deflection" — so the confirmed deflection is the headline event on the row.
• FBO: false-breakout outcome marker relabeled 'false break' -> 'FBO' (user).
• Attached per-node as L.deflection in nodeMapModel.
• Tests: NEW test_deflection.js (11 assertions incl. up/down/-gamma/multi-bar-confirm/
  still-sitting/never-tapped); step5 + nodemap tests updated. Suite 21/21 green.
NOTE: this is the detection+report of deflection only. The Node Map UI ZONE REDESIGN
(4-zone layout, role-pill de-dup, lifecycle dot, BO/outcome consolidation) was MOCKED and
APPROVED but NOT yet built — it is the next UI build (mockups: nodemap_redesign_v2).

## v10.33 — 2026-08-13 — Node Lifecycle: Fresh/Tested/Delivered/Decaying + tap-probability (Academy pattern)

SOURCE OF TRUTH: Skylit Academy "Node Lifecycle" (marked KEY in the doctrine).
• NEW node tap counter (updateTaps/nodeTapCount): counts DISTINCT taps — a tap = wick
  within TAP_TOL(0.20) of the strike, then price LEAVES by >=TAP_AWAY(0.60), then RETURNS.
  One long sit != many taps. Persisted per trading day. (Fixes the previously dead,
  never-populated 'touches' field.)
• NEW nodeLifecycle(): FRESH (0 taps, ~80% 1st-tap) -> TESTED (1, ~66%) -> DELIVERED
  (2+, ~33%, graveyard) -> DECAYING (weakening with no interaction). Academy tap-reaction
  probabilities (TAP_PROB 80/66/33) surfaced as a FACTUAL annotation, not a trade call.
• UI: nodeLifecycleTag on Node Map rows — Fresh/Tested/Used/Decay + probability. Fresh
  shown only on King/Gatekeeper/strong nodes (avoids badge spam on minor untouched nodes).
• Charts-First safe: reports how many times price tested a level, never buy/sell.
• Tests: NEW test_lifecycle.js (16 assertions incl. distinct-tap + no-inflation-on-sit);
  nodemap stub added. Suite 20/20 green.
NOTE: Real-vs-Hedge (cross-session GROWTH vs decay) is the SEPARATE later half of Node
Lifecycle — needs the committed recorder-schema history; not in this build.

## v10.32 — 2026-08-13 — Air Pocket / Liquidity Vacuum detector (Academy pattern)

SOURCE OF TRUTH: Skylit Academy "Air Pockets, Liquidity Vacuums & Velocity Mode."
• NEW airPocketDetect(): a low-exposure GAP between two significant nodes = a fast
  PATHWAY (trade THROUGH it, target the node on the far side), never a target itself.
  - Air Pocket vs Liquidity Vacuum by gap width RELATIVE to the board's grid step
    (adapts SPY 1-strike vs QQQ/SPX wider). Sparse-board fallback to an absolute floor.
  - Flags the pocket ADJACENT to spot (Academy Velocity-checklist Q1) + reports the
    pathway's far-side target nodes. Structural read, NOT a buy/sell signal.
  - Tunables: AIRPOCKET_GAP_MULT=2.5, AIRPOCKET_VACUUM_MULT=4.0, AIRPOCKET_MIN_STRIKES=2.0.
• UI: compact "⚡ Air Pocket lo–hi · pathway → up/dn" note in the Node Map header,
  shown only when a pocket sits adjacent to spot; full detail in the hover.
• This completes 3 of 4 ingredients of the Academy's "dangerous combo" checklist
  (we already have polarity + velocity/rapid flags; air pocket was the missing brick).
• Tests: NEW test_airpocket.js (14 assertions incl. relative-spacing + sparse-board
  edge cases); nodemap stub added. Suite 19/19 green.
• CLEANUP: removed the v10.31-debug feed/candle field probes (audit complete — verified
  the scraped candle prop carries only OHLC; volume/CVD live on separate chart series,
  logged as a roadmap candidate to widen the fiber scraper).

## v10.31 — 2026-08-13 — Detector polarity hardening (Skylit Academy = source of truth)

SOURCE OF TRUTH recorded: Skylit Academy (skylit.ai/learn) is now the authoritative
reference for all detector logic; on conflict with FAQ/patternpedia/older code, the
Academy wins (see SOURCE-OF-TRUTH.md). Full Academy (11 articles) mirrored to skylit-docs/learn/.

• CLUSTER = PIKA CLOUD, positive-gamma only. Academy: "Pika = POSITIVE gamma specifically."
  clusterDetect now filters w.pos===true. Fixes clusters/stacks over-firing on -gamma days
  (root cause of the "too much double-stack on multiple strikes" report).
• NEW Barney detector: dense NEGATIVE-gamma region (Academy: "a different animal — Barney").
  Surfaced with its own 'Barn' node badge (instability/acceleration zone), not mislabeled as a pin/chop cluster.
• DOUBLE-STACK polarity-gated to +gamma (STACK_POS_ONLY=true, tunable) — a bounce shelf is a
  +gamma support behavior. Kept (verified a real Skylit concept via FAQ). Final rule pending live review.
• Badge vocabulary aligned to Academy: Cluster 'Clst' -> 'Pika'; added 'Barn'.
• Tests: test_cluster_stack.js now polarity-aware (+7 assertions incl. Barney), role-badge +2,
  nodemap stub added. Suite 18/18 green.

Note: Double-Stack final polarity rule to be locked after reviewing live before/after counts.

## 2026-08-13 — v10.30: DETECTOR HARDENING (Double-Stack over-fire fix + Rug adjacency/strength) + roadmap
- **DOUBLE-STACK over-fire FIXED (user-reported).** On SPY's dense 1-pt grid the old detector chained any run of consecutive >=25% strikes into one giant "double stack" and badged every strike. Now: a Double-Stack is EXACTLY a comparably-strong adjacent PAIR — (a) exactly 2 nodes (a run of 3+ is a Cluster, not a stack), (b) MUTUALLY EXCLUSIVE with Cluster (cluster members are skipped), (c) STRENGTH-BALANCED (weaker >= STACK_BALANCE=0.5 × stronger, so a dominant King next to a marginal node is not a "stack"). doubleStackDetect now takes the cluster result to enforce exclusivity.
- **Significance floor raised 25% → 40% (CLUSTER_SIG_PCT).** A "fortress" node must be substantial vs the King; 25% let ordinary mid-strength strikes all qualify on a dense board.
- **RUG / REVERSE-RUG hardened.** (a) The yellow ceiling must now sit DIRECTLY over the purple node — within RUG_ADJ=3.0 strikes — matching the doctrine's tight cap-over-accelerant stack (was: any purple anywhere below, which over-fired). (b) Both anchors must be STRONG (>=RUG_ANCHOR_PCT=40), not just clear the 20% cascade floor. (c) Fixed a latent bug: the Reverse-Rug mass test referenced purpleCeil.v which walls never carry (always undefined → silent pct fallback); now compares pct-scale masses correctly.
- **Thresholds (all tunable, sync-guarded by tests):** CLUSTER_SIG_PCT=40, CLUSTER_BAND=3.0, CLUSTER_MIN_N=3, STACK_GAP=1.0, STACK_BALANCE=0.5, RUG_SIG_PCT=20, RUG_ANCHOR_PCT=40, RUG_ADJ=3.0.
- **ROADMAP:** added the NOW-priority "Detector correctness pass (v10.30)" and the follow-on "Analysis-tab metric review (v10.31, AFTER detector hardening)" per user — harden the Dashboard detectors BEFORE scoring their metrics.
- **TESTS:** test_cluster_stack rewritten (20 checks incl. over-fire regression + mutual-exclusivity); test_rug rewritten (13 checks incl. far-purple, weak-anchor, weak-reverse-floor rejections). Full suite 18/18 green.
## 2026-08-13 — v10.29: Node Map decluttered — callout lines → badge hovers, "regime"→"Pattern", final abbreviations
- **REMOVED the Node Map header callout LINES** (they duplicated per-node badges / drifted off-doctrine):
  * **Pattern instruction line** ("Stand aside — no clean edge." / "Fade the edges" / "Enter on pullbacks") — the header Pattern BADGE already names it; stance lives in the badge hover.
  * **S/R Imbalance net-read + crossover banner** — NOT part of Skylit's Step-5 (which is per-node Acm/Diss/Reshuffling, already on the rows); the net bull/bear synthesis edged toward a deflect/break call the doctrine forbids. Removed (srBattle engine retained for other consumers).
  * **RUG callout** and **Double-Stack / Cluster callouts** — fully covered by the per-node RugC/RugF (RRugC/RRugF) / DStk / Clst badges.
- **DETAIL MOVED TO BADGE HOVERS.** The per-node pattern badges now carry the live detail in their tooltip: Rug → geometry (ceil over floor) + forming/candidate/unconfirmed state + targets; Double-Stack / Cluster → span + node-count + meaning. (New row fields rugDetail/stackDetail/clusterDetail feed nodeRoleBadge.)
- **"regime" → "Pattern"** — renamed to match Skylit's Patternpedia vocabulary (Trend / Whipsaw / Rainbow Road are Patterns, not "regime"). Badge unchanged; wording corrected.
- **FINAL ABBREVIATIONS** (full names in hovers): Double-Stack→**DStk**, Cluster→**Clst**, Rug ceiling/floor→**RugC/RugF**, Reverse-Rug→**RRugC/RRugF**, Ceiling→**Ceil**, Floor→**Flr**. (Acm/Diss node-status tags already shipped in v10.26.)
- **TESTS:** test_node_role_badge updated to the final labels (priority order unchanged). Full suite 18/18 green.
## 2026-08-13 — v10.28: step-number icons on Node Map (⑤) & Gatekeeper (④), + space-saving abbreviations
- **⑤ STEP-5 ICON in the Node Map header** — the Node Map header title now leads with the clickable ⑤ icon (opens the "Step 5 — Map the Flow" popover), matching ①②③ in the King header and ④ in the Gatekeeper section. The 5-step spine is now fully numbered end-to-end.
- **④ moved BEFORE the gate icon** in the Gatekeeper section (was gate+④, now ④+gate) so the step number leads the section like the others.
- **ABBREVIATIONS (space fix — full names kept in tooltips):**
  * per-node role/setup badges: Double-Stack → **2Stk**, Cluster → **Clu**, Rug-ceiling → **Rug-Ce**, Rug-floor → **Rug-Fl**, Reverse-Rug variants → **RRug-Ce / RRug-Fl**.
  * Node Map callouts: "⬛⬛ Double-Stack N–N — strong-bounce shelf" → "⬛⬛ 2Stk N–N · bounce shelf"; "▦ Cluster N–N (N nodes) — pin/chop zone" → "▦ Clu N–N · pin/chop" (node-count moved into the tooltip). Frees up the cramped text/badge columns in the ladder.
- **TESTS:** test_node_role_badge updated to the new abbreviated labels (priority ordering unchanged). Full suite 18/18 green.
## 2026-08-13 — v10.27.1: layout fixes on the v10.27 batch (leftover ladder, header fit, King Path legibility)
- **Node Map: REMOVED the leftover old two-sided ladder.** accumBlock() was still rendering the pre-Step-5 ladder ("778 Ceiling · 24% [Steady]", "776 King · 38%" + ↳ sparkline rows) BELOW nodeMapBlock() — a duplicate of the new Step-5 identity ladder. accumBlock is now just the Node Map wrapper (everything after the nodeMapBlock() call deleted).
- **King 3-magnet header: FIT FIX.** ★SUP / 👑King / ★RES badges overflowed the panel width. Shrunk fonts (side strike 14→12.5px, King strike 15→13px, labels 8→7.5px, offset arrow 14→12px), padding, radius, and gap; row now uses justify-content:space-between + width:100% + flex:0 1 auto;min-width:0 so the three badges spread inside the panel instead of spilling off both edges.
- **King Path: LEGIBILITY FIX.** (a) King & price labels stacked when King≈price — now they auto-separate vertically to a guaranteed ≥11px gap (split around their midpoint). (b) The price line was lost under the gold King staircase — restyled to stroke-width 1.4, dash 4 3, opacity 0.85 (stale 0.35), + a blue price marker on the left end, so the two lines read distinctly even a strike apart.
- **TESTS:** test_kingpath updated for the new price-line style (dash 4 3, stale opacity 0.35). Full suite 18/18 green.
## 2026-08-13 — v10.27: BO section removed→per-node BO tag + 14-bar breakout gate + Gatekeeper magnitude-driven + S/R Imbalance folded into Node Map + badge/chip cleanup
- **GATEKEEPER now MAGNITUDE-DRIVEN (definition fix).** Was: NEAREST significant node between price and King. Now: the DOMINANT blocker = largest |%King| on the path (the doc's "second-highest node between price and King" / "compare vs the 2nd highest-value node" / "far in excess of nodes beyond it"). Tiebreak on EXACT-equal magnitude only → the node nearer PRICE (doctrine's price-anchored ≤5-pt validity). Fixes the case where a nearer-but-weaker node wrongly won over the stronger blocker (e.g. picks 779@41% over 778@14%). test_gatekeeper +2 assertions (stronger-farther beats weaker-nearer; exact tie→nearer price).
- **BO / SPY Signals standalone section REMOVED → per-node BO tag.** The breakout-pullback lifecycle now rides on the Node Map row it belongs to as a compact chain tag (BO / BO·FT / BO·FT·TST·CONF·GO), colored by direction (long green / short red), tooltip explains each stage. State machine (runMachine/newSetup/STATE.setups) unchanged — only the grid rendering is gone. Reclaims vertical space. (nodeMapBlock → setupTagForNode)
- **NEW 14-BAR BREAKOUT QUALITY GATE.** A BO only fires if the breakout bar ALSO prints a new N-bar extreme: a 14-bar HIGH for upside breakouts / 14-bar LOW for downside breakdowns (window INCLUSIVE of the breakout bar). Symmetric (longs + shorts). Filters weak/noise pokes through a node not backed by genuine range expansion. Tunable const BO_HL_LOOKBACK=14. +test_bo_14bar (8 assertions incl. constant sync-guard).
- **S/R IMBALANCE standalone section REMOVED → folded into the Node Map header.** The Step-5 net-flow read ("Bearish/Bullish imbalance — resistance X building, support Y fading") + the tradeable crossover banner (▼ BEARS / ▲ BULLS TAKING OVER) now render as Node Map header lines. srBattle engine unchanged (render-cached; same value). Keeps the flow thesis, drops the section overhead.
- **Redundant strongest-Sup/Res chip row REMOVED from the Node Map** — duplicated the King 3-magnet header (★SUP ← 👑 → ★RES). Snapback "↩ King N behind" warning retained.
- **SUP/RES header badge overflow FIX** — the ★SUP/★RES side badges clipped their label + %King outside the pill (rigid height:42px + 3 stacked rows). Switched to min-height:42px + line-height:1 so content sizes cleanly inside the border. Matches the King badge behavior.
- **TESTS:** full suite **18/18 suites green** (17 prior + test_bo_14bar), no regression. symSignalsHdr/signalGrid left defined but unreferenced (dead, harmless).
- **NOTE ON NUMBERING:** this is the Step-5 consolidation release; the VEX/Analysis-tab/LLM bundle originally sketched as "10.26/10.27" is still unbuilt — renumber that to v10.28+ next session.
## 2026-08-13 — v10.26: Step 5 node identity (three-axis) + role/setup badge (pill removed) + Cluster/Double-Stack detectors
- **NODE STATUS (Step 5 flow, doc-vocab):** each node now shows Building/Fading/Steady mapped to **Acm** (green, strengthening) / **Diss** (red, weakening) / **Steady** (grey). The RAPID flag = the doc's **Reshuffling** state → 🔥 (rapid Acm) / ❄ (rapid Diss), tooltip ties to the doc word. (nodeStatusTag)
- **NODE TYPE (gamma polarity):** **+γ** positive-gamma (pinning/mean-revert, yellow) vs **−γ** negative-gamma (accelerant/breakout, purple) — surfaced per-node from the existing `pos` field. (nodeTypeTag)
- **REMOVED the predictive verdict pill (Bounce/Pullback/Break-through)** — it violated the attraction-only honesty rule (map makes NO deflect/break call). The `verdict` field still exists in the model but is NOT rendered.
- **ROLE/SETUP BADGE replaces it (factual, never predicted):** King > Gatekeeper > Rug-ceiling/floor > Double-Stack > Cluster > Floor/Ceiling (priority order; single top badge, secondary role in tooltip). (nodeRoleBadge)
- **TWO NEW DETECTORS (user's node-type list):**
  * **clusterDetect** — ≥3 significant nodes (≥25% King) within CLUSTER_BAND=3 strikes → PIN/CHOP region (blue). Callout ▦ + per-node badge.
  * **doubleStackDetect** — 2+ adjacent significant nodes within STACK_GAP=1 strike → strong-BOUNCE shelf (green). Callout ⬛⬛ + per-node badge.
  * Thresholds: CLUSTER_SIG_PCT=25, CLUSTER_BAND=3.0, CLUSTER_MIN_N=3, STACK_GAP=1.0 (all tunable; test has a sync-guard).
- **King bug fix (from v10.25) intact** — extractWalls returns the STRIKE of max |exposure|, not the magnitude.
- **TESTS:** +test_node_identity (17), +test_node_role_badge (19), +test_cluster_stack (15). Full suite **17/17 suites green**, no regression.
- **NOT YET DONE (next session):** live verification of v10.26 on real tape; Analysis-tab scoring layer (item 3); tape reconciliation pass; daily-file→LLM loop. See BUILD-PLAN v10.26 section + RESUME-NEXT-SESSION.md.
## 2026-08-12 — v10.25: 5-step posture layer (live panel) + CRITICAL King-selection bug fix
- **5-STEP INFO-ICON SYSTEM** (①–⑤): click-to-open popovers carrying the Skylit "how-to-read-heatseeker" step text, wired via delegated handler + re-appended popover on each render.
- **STEP 1 (header)**: 3-MAGNET cluster centered — green ★SUP (left) · 👑King (middle) · red RES (right). Old trend badge REMOVED (regime/instruction line carries the trend read).
- **STEP 2**: King framed as EOD settlement anchor only (EOW/swing wording dropped — this is a day-trading tool).
- **STEP 4 (gatekeeper area)**: own section below King header; white castle-gate (portcullis) SVG icon, no spelled-out "GATEKEEPERS" title; lists gatekeeper node(s) + strength ratio via existing v10.24 gatekeeper() engine.
- **STEP 5 (sharpened Node Map, ATTRACTION-ONLY)**: accumulation ONLY attracts — NO deflect/break prediction. Per-node stage: attracting → "at node · watch BO" handoff. Resolved OUTCOME echo (report, not prediction) from the BO state machine: broke ↑ · broke ↓ · held (clean reject) · false break (broke then reversed).
- **CRITICAL KING BUG FIX**: extractWalls() was returning the max exposure MAGNITUDE (e.g. 6.6e8) as the King instead of the STRIKE — leaked into S.king/PREVKING and shown as a nonsense King disconnected from the tape. FIXED: King = strike of largest ABSOLUTE |exposure| (so a dominant NEG-gamma node can be King). Confirmed against day_811.json: 67 snaps had magnitude-as-king; tking (correct source) was always a real strike.
- **TESTS**: +test_step5_attraction (16), +test_king_strike (9). Full suite 14/14 green, no regression.
## 2026-08-12 — v10.24.1: Rug polarity VERIFIED (after-hours) — Rug/Reverse-Rug flag now LIVE
One-time polarity verification done against the live ladder (after-hours; feed still populated). Confirmed sign(pos=n.d>0) == Skylit ladder color convention (positive=yellow, negative=purple), so the Rug detector's polarity basis is sound.
- EVIDENCE (SPY ladder DOM, signed %King = polarity): 772=−85% (purple/neg), 773=+90% / 779=+95% / 774=+77% (yellow/pos), 769=−17% (neg). Signs cluster exactly where color convention predicts; no inversion.
- CROSS-SYMBOL CONFIRM: QQQ 727=−$13,835K (large negative/purple) with positive nodes above = the documented yellow-over-purple Rug geometry appearing NATIVELY under our sign convention. An inverted sign would render this upside-down; it doesn't.
- ACTION: flipped RUG_POLARITY_VERIFIED=true. Rug/Reverse-Rug now SHOWS its flag (was computed-but-hidden in v10.24). All other detectors unchanged. test_rug updated to assert shown=true. 9/9 new suites + 3 existing green.
NOTE: raw feed JSON not fetchable out-of-tab (Authorization header, not just cookie) — verification done from the rendered ladder, which is the authoritative feed render.

## 2026-08-12 — v10.24: Node Map v1 + intraday detectors (regime / Gatekeeper / Rug) + effectiveness capture
Shipped as one release (user locked). Build order: Node Map structure -> regime -> Gatekeeper -> Rug. 9 new test suites + 3 existing green.
- **NODE MAP v1** (Issue I): reshapes PROJ into a two-sided, price-anchored dealer-positioning map. Consumer of futureStructureSummary rows (not a new engine). Marks strongest floor/ceiling (★, size+build-rate+nearness blend), the King (👑), per-level verdict (Bounce/Pullback/Break-through/Forming, directional-meaning colors), travel-emphasis side (trend badge + momentum, both sides always shown), and against-King "magnet behind" note. Renderer nodeMapBlock() replaces the abbreviated PROJ row. Layout + verdict wording user-approved (rendered mockup).
- **GEX-STRUCTURE REGIME** (gexRegime): whole-board Trend / Whipsaw / Rainbow Road on current node structure, reverse-engineered from Patternpedia + annotated examples. TREND = one-sided mass skew ≥1.8×; WHIPSAW = 2 edges + hollow middle (edge/mid ≥2×); RAINBOW = ≥4 prominent both-sign interleaved, full middle. DISTINCT from the SMA price-trend badge (kept). Surfaces as a header chip + regime-as-instruction line (fade edges / stand aside / enter on pullbacks). Tunable consts REGIME_*.
- **GATEKEEPER** (gatekeeper): nearest high-|value| node between price and King; strength ratio = |gk| / |next non-King node beyond|. Ratio ≥1.8× => Reversal-likely verdict + King decoy discount. Early-session rejections weighted higher. Absolute value ranks (polarity only flavors).
- **RUG / REVERSE-RUG** (rugDetect): the polarity-gated namesake. Yellow(pos)-over-purple(neg) stack, neg floors below, no positive floor => bearish nosedive (mirror = squeeze). Uses per-node pos (feed d). VERIFY-ONCE: RUG_POLARITY_VERIFIED=false gates the SHOWN flag until sign(pos)==ladder color is confirmed live; detector still computes + logs evidence. Confirms with build-rate (downside growing + ~zero upside).
- **EFFECTIVENESS CAPTURE** (Issue I5 mandate): recorder now logs sig.nodemap every 3m bar — regime, gatekeeper, rug, strongest levels, and per-level verdicts — so forward-outcome (out5/out10) hit-rate is computable later by the Analysis tab + LLM loop. Records from day one.
NEW TESTS: test_nodemap, test_gexregime, test_gatekeeper, test_rug.
NOTE: Rug flag stays hidden (shown=false) until the one-time polarity verification; everything else is live.

## 2026-08-12 — v10.23: batch fix (A/B/C/D/F/G/H) — King header rework + de-flicker + continuous SMA
FIXES-FIRST build order per user. All 5 new test suites + 3 existing suites green; no regression.
- **C SMA-50 continuous**: added multi-session close series (convertFiberCandlesCont / S.contCloses) + contSMA/contSMAAtTodayIdx. smaVal + trendVerdict now chart-aligned & populated from the open (kills "need more bars"). Option-A sanity: fall back to today-only + SMA_CONT_FLAG if continuous drifts >5% from spot.
- **D S/R Imbalance** (renamed from "S/R Bias"): reframed to DIVERGENCE of build-RATES near price (imbalanceMetric); badge names the mechanism ("Bearish imbalance — resistance 773 building, support 769 fading"); de-flickered via dead-band + asymmetric hysteresis + 3m bar-close commit (SRB_STATE). Crossover derived from committed state.
- **H node-ladder de-flicker**: accumulationStateFor now judges accumTrend on the CLOSED portion of absSeq (excludes live last point) with a rapid-override (ACC_RAPID_ROC) for genuine fast moves. Node badge COLOR corrected to directional-meaning: res-building=red, res-fading=green, sup-building=green, sup-fading=red, steady=grey (stateColor).
- **A King Path**: price line now ALWAYS renders (was silently dropped when price fell outside the strike-only window — the reported bug); folds price into the axis before padding, clamps+caret if off-range, last-known fallback (dimmed) on null px. Labels: spot-price "771.9 (-1.1)" at right end + King strike "👑773" on the gold dot. Canvas +33% taller (H 84→112).
- **B King badge offset**: format now SIGN left, number, ARROW right → "+1↑" / "−1↓" (offset axis only; drift stays in the King Path).
- **F READ removed**: deleted the readBlock() render call; King header auto-promotes to the top of the panel. readBlock/structuralReadHtml left defined as dead code.
- **G King header**: TREND BADGE (left) + King node (right) matched pair. Trend badge = King-matched pill, stacked (state code top / dominant "↓16/20" count below, state-colored arrow) + slope tick (↗/↘/→) centered right, colored by slope direction. "warming up" (no digits) on na.
NEW TESTS: test_sma_cont, test_sr_imbalance, test_node_flicker, test_kingpath, test_trendbadge.

## 2026-08-12 — v10.22: King table-selection fix (FLOW BUCKET popup decoy) + largest-|value| cross-check + year-header guard

DATA-INTEGRITY FIX (found in a live-market review, 2026-08-12 ~09:5x CT). The panel showed King 775 while the tape's true GEX King was 773 (96% %King, $207,395K, adjacent to spot 772). Root cause: findTapeTable() returned the FIRST DOM container carrying a "Strike" header + a "$...K" cell. The "FLOW BUCKET / Top contracts" flow popup ALSO matches that pattern, sits earlier in the DOM, and its $K dollar tag lands on 775 — so whenever that popup is open the parser latched onto the wrong table and crowned the wrong strike. Every King consumer (King header/path/journey, S/R bias, edges, regime classifier, AND the recorder that persists to daily-data) read from that corrupted map, so the bug silently poisoned exports too. This is the 2nd King-integrity bug of the same class (see v10.20 recorder fix) — the source of truth is the tape.

FIXES:
- findTapeTable(): (a) HARD-REJECT the flow popup (FLOW BUCKET / Top contracts / Pick range end markers); (b) REQUIRE the GEX-ladder fingerprint — ISO expiry-date column headers (20\d\d-\d\d-\d\d) OR a deep strike list (>=15 strike rows); (c) among survivors pick the one with the MOST strike rows (the real ladder can never lose to a small popup). Still keyed off rendered data, not CSS classes.
- kingResolve() cross-check: docs say King = largest ABSOLUTE dealer exposure. If the $K-tagged strike disagrees with the strike carrying the largest parsed |%King| by >=5, trust the DATA (override to max-|value|), set kingConflict + remember kingTagged for diagnostics. A single mis-placed $K cell can no longer silently crown the wrong strike. kingSrc reports 'dollar' | 'maxpct' | 'maxpct-override'.
- King row no longer clobbers its real %King with 100 (773 now reads its true 96, not 100) in both Path A (<tr>) and Path B (div grid); 100 kept only as a last-resort fallback when no % is readable.
- Year-header guard: a bare 4-digit year token (e.g. "2026" from an expiry column) can no longer be mistaken for a strike and pollute the node map.

TESTS: test_tapeking.js NEW — reproduces the exact 2-table scenario (flow popup King$K=775 + GEX ladder King$K=773) and the mis-tagged-$K case; 12/12 PASS (picks ladder, crowns 773, preserves 96%, no year pollution, cross-check override + flags). Regressions still green: test_analytics.js (real 8/11 edges incl. fade-support->down 64%/sw73%), test_render.js (Whipsaw regime + all reads, RENDER-OK).

Files: current/gex-signal-tapereader.user.js (v10.22), releases/2026-08-12_king-table-selection-fix_v10.22.user.js, releases/2026-08-12_pre-v1022_v10.21.user.js.

## 2026-08-12 — v10.21: Doc-derived coherent Analysis tab (regime classifier + King behavior + accumulation/dissipation/combined edges + loader + tooltips)

Rebuilt the Analysis tab around the Skylit Heatseeker methodology (read from docs.skylit.ai: Core Concepts, 5-step How-to-Read, Best Practices, Limitations, Pitfalls, FAQs, and Patternpedia). The tab now tells ONE causal argument top-to-bottom instead of scattered stats.

NEW analytics core (pure fns over a day-export, validated against the real 2026-08-11 capture; A_* namespace):
- A_kingBehavior: King path/core (dwell-weighted, ignores 1-bar outliers), net drift, rolls (up/dn, avg size, lead/lag), offset posture (pull up/down), reach rate + time-to-reach + gap convergence, and the PIN metric (close vs day King within ~1pt zone + early/late timing per the doc's drive-off-vs-pin rule).
- A_accumEdge('accum'/'fade'): does building support-below->up / resistance-above->dn (accumulation), and fading support-below->dn / resistance-above->up (DISSIPATION is directional). Reports dir-hit vs baseline (lift), swing-hit, and MFE/MAE payoff, split support vs resistance, with n.
- A_combinedEdge: trapdoor (res build + sup fade -> dn), liftoff (sup build + res fade -> up), compression (both build -> range), dual-vs-single, net-flow polarity.
- A_regime: composes the above into the docs' day-types — Trend (leading one-way rolls + drift) / Whipsaw (flip-flop King in a tight core) / Rainbow Road (scattered heavy nodes, no edges) / Mixed / Forming — each with plain-language why. 8/11 correctly classifies Whipsaw (769–771 core, 7↑/7↓ rolls).

Analysis-tab render:
- Regime chip headline (color by regime) + why. King-behavior step (anchor) with reach/pin. Accumulation, Dissipation, and Combined edge steps — REGIME-AWARE (range days show a "directional edges are low-signal here" caveat). Every item carries a coherence tooltip naming its role in the King->price->S/R->nodes story (context-not-signals posture per Best Practices).
- LOADER: __gptsDebug.loadDay(json)/loadReview(obj)/clearLoaded() + an in-tab "📂 Load day" button, so a past day's export renders here (all render fns routed through A_day()). Legacy 7-step signal scorecard + LLM review preserved below as "Signal scorecard & review".
- Day-grade badge shrunk (28px->18px, 84px->52px) and demoted below the regime chip.

Roadmap captured in DOCS-DERIVED-SPEC.md. v10.22 (needs API key, reminder set 8/12): VEX/GEX overlap + persist VEX from LASTVEX (already hooked, unused), SPX/SPY/QQQ tri-confluence gate, gatekeeper/velocity/node-class, Rug tri-confluence, SSE feed migration (Skylit moved to /api/stream). v10.23: live Dashboard regime banner + Rug-forming alert + multi-day topping/bottoming.

Files: current/gex-signal-tapereader.user.js (v10.21), releases/2026-08-12_regime-king-edges-loader-tooltips_v10.21.user.js, releases/2026-08-12_pre-v1021_v10.20.user.js. Tests: analytics_v1021.js, test_analytics.js (PASS), test_render.js (RENDER-OK, 10/10).

## 2026-08-12 — v10.20: recorder tapeKingStrike fix + King-arrow offset + BO/S-R reorder + Analysis scroll

(Backfilled changelog entry — this shipped but was never logged.)
- RECORDER FIX (the reason v10.20 exists): recorder + sig now use tapeKingStrike() instead of S.king, which was storing raw dealer EXPOSURE (e.g. 528,568,656) instead of the strike in 67/88 snaps on 8/11. Caught by the first end-of-day data-integrity review. hitKing labeling corrected by the same fix going forward.
- King header: arrow now shows the offset NUMBER (e.g. ↑2) with tabular-nums, not a bare arrow.
- Layout: BO / SPY Signals moved ABOVE the S/R Bias node ladder.
- Analysis tab wrapped in its own vertical scroll container so all steps are reachable in a short panel.

Files: current/gex-signal-tapereader.user.js (v10.20), releases/2026-08-12_kingarrow-bo-order-analysisscroll_v10.20.user.js, releases/2026-08-12_pre-v1020_v10.19.user.js

## 2026-08-12 — v10.19: Remove S/R Bias balance bar + King header redesign

- REMOVED the S/R Bias balance bar (the "◀ support 50% · no edge · resistance 50% ▶" block). Per spec, the srBattle read now lives in the READ synthesis at top; the tradeable crossover banner and the node ladder + PROJ strikes below are kept (the ladder IS the S/R nodes the READ refers to). Only the bar was removed.
- KING HEADER redesign: removed the "👑 King" title text; badge is now CENTERED; crown moved INSIDE the badge next to the King price. Offset shown as an ARROW only (↑ when King above price, ↓ when below), and NOTHING when King == price. Fixed the inverted color: King ABOVE price = green (magnet pulling up), BELOW = red (was showing red when above). Removed the "+N" plus-sign entirely.
- King Path hysteresis coloring confirmed live (from v10.18): line color only flips when the King reclaims the prior pivot by ≥2 strikes, so single-strike wiggles no longer recolor a clear trend (the mixed-segment screenshot was a pre-v10.18 build).

Files: current/gex-signal-tapereader.user.js (v10.19), releases/2026-08-12_srbar-removed-kingheader_v10.19.user.js, releases/2026-08-12_pre-v1019_v10.18.user.js

## 2026-08-12 — v10.18: The agreed Dashboard redesign (READ, 16/20 trend machine, trend-gated breakouts, King hysteresis)

Implemented the decisions we specced but hadn't yet coded:

1. **READ replaces MIXED/NO EDGE + 4 badges.** New readBlock() at the top: a plain-language synthesis of the THREE kept signals (King, Trend, S/R) — no vote, no badges, no MIXED/NO EDGE. Leads with the dominant piece (confirmed trend > broken trend > King lean > S/R), names conflict in plain words ("Signals mixed — …"), left border colored by net lean (green/red/amber). CONTEXT voter dropped (proved wrong on 2026-08-11). Old confluenceStrip() left defined but no longer rendered.
2. **Trend = 16/20 five-state machine.** trendVerdict rebuilt: Uptrend / Uptrend-broken / Downtrend / Downtrend-broken / No-trend / NA. 16 of 20 bars (>=75%) confirms a direction; "broken" is the mandatory middle step (uses TREND_LAST memory) styled amber/caution. TREND_DOM 15→16.
3. **Trend-gated breakouts + 50-MA filter.** trendOkFor upgraded: long-break allowed only in Uptrend OR Downtrend-broken; short-break only in Downtrend OR Uptrend-broken; AND the bar must close beyond the 50-MA. Wired into the BO state machine (runMachine). breakoutConviction() tags high (confirmed) vs early (broken).
4. **King sparkline hysteresis.** Line now colored by a running regime that only flips when the King reclaims the prior pivot by >=2 strikes (BUF=2) — single-strike wiggles no longer recolor a clear trend. Roll dots keep their own local up/down color.

Verified: 5-state machine (up17→up, break→up-broken, 16-below→dn, chop→flat), gating (short-ok/long-blocked in downtrend even above MA), hysteresis (1-wiggle stays colored, real reversal flips), READ has no MIXED/badges. Regressions: king 15, fallback/recorder PASS, confluence intact.

Files: current/gex-signal-tapereader.user.js (v10.18), releases/2026-08-12_read-trend16-gating-hysteresis_v10.18.user.js, releases/2026-08-12_pre-v1018_v10.17.user.js

## 2026-08-12 — v10.17: Analysis tab as trigger + narrow-view responsive fixes

- Removed the standalone 📥 Save Day button from the Dashboard footer. The Analysis tab is now the trigger: an in-tab banner shows "Today's data isn't saved yet · [📥 Save & prep review]" when unsaved, and "✓ Saved gex_DATE.json — drop into daily-data/" once saved this session (SAVED_TODAY flag). No surprise auto-download on tab open — the save is an explicit tap.
- Responsive SVGs: timeline + convergence now size to the actual panel width (_bodyW from elBody.clientWidth), render at width:100% when they fit and only horizontal-scroll when bars exceed the width. Fixes overflow in narrow sidebar.
- Timeline visibility fixes: S/R dominance band now a visible green/red tint (opacity 0.13) instead of near-black; price line rendered distinctly under the King (thicker, lighter); auto-padded Y-range so a flat King and 771.xx price no longer collapse onto one line; compact single-row legend (King/price/sup/res) that fits.
- Reduced Analysis step indent 30px → 14px for more room in narrow view.

Files: current/gex-signal-tapereader.user.js (v10.17), releases/2026-08-12_analysis-tab-trigger-responsive_v10.17.user.js, releases/2026-08-12_pre-v1017_v10.16.user.js

## 2026-08-12 — v10.16: ANALYSIS TAB — in-app end-of-day review dashboard

- NEW tab bar at top of panel: **Dashboard | 📊 Analysis** (toggle via window.__gptsDebug.showAnalysis()).
- NEW analysisStats(sym): computes review metrics LIVE from the recorder's labeled snapshots (sig vector + out5/out10) — no LLM needed for the numbers. Direction hit % (30m), reversal-catch %, King-target %, per-signal accuracy (King/Trend/S/R/Confluence), confluence-outcome matrix by alignment count, and multi-strike node lifecycle (net drift per strike).
- NEW analysisBlock(): the 7-step narrative tab — (1) relationship timeline SVG (King+price+S/R dominance band, horizontal scroll, crossover markers), (2) King↔price convergence SVG, (3) node lifecycle (vertical scroll), (4) confluence-outcome matrix, (5) per-signal scorecard, (6) worked/missed/why, (7) discoveries + ranked recs. All from real data.
- LLM narrative panels (steps 6–7 + lead/lag notes) show an honest "Awaiting review" state until a review is loaded via window.__gptsDebug.setReview(json); numeric panels always render from tape.
- Day Grade derived from direction-hit% until an LLM grade is supplied.
- Scrollbars: whole tab body scrolls; timeline + convergence scroll horizontally; node lifecycle scrolls vertically.

Design source: mockups/analysis_tab_mockup.html. Files: current/gex-signal-tapereader.user.js (v10.16), releases/2026-08-12_analysis-tab_v10.16.user.js, releases/2026-08-12_pre-v1016_v10.15b.user.js

## 2026-08-12 — v10.15b: DATA LAYER — signal-vector capture + forward-outcome auto-labeler + daily export

Foundation for the self-improving review loop (Stage 1 of 3).

- recordNodeSnapshot now also captures a per-bar SIGNAL VECTOR `sig`: trend{state,up,win,ma,slope}, king{cls,word,drift,score,magnet,offK}, srb{dom,cross,supF,resF,supPct,floor/ceil + fade flags}, breadth{net,dir,mag}, conf{dir,word,score,aligned,bull,bear,declared}. Each signal is guarded so one failure never blocks the snapshot.
- NEW labelForwardOutcomes(): once ≥5 / ≥10 newer bars exist, back-fills out5 (15m) and out10 (30m) forward outcomes on each older snapshot — {mfe, mae, net, pxEnd, hitKing, revUp, revDn, n}. Idempotent, only fills null slots. Storing BOTH horizons makes the ~1-bar bounce/pullback lag measurable rather than guessed.
- NEW buildDayExport()/saveDayToFile(): self-describing daily payload (schema gex-day-export/v1 + legend + horizons) exported as gex_YYYY-MM-DD.json.
- NEW footer "📥 Save Day" button → downloads today's labeled JSON. Drop into AI Drive /GEX-Signal-Tapereader/daily-data/ for the scheduled end-of-day review workflow.
- Created AI Drive folder /GEX-Signal-Tapereader/daily-data/ as the review workflow's inbox.

Files: current/gex-signal-tapereader.user.js (v10.15), releases/2026-08-12_datalayer-forwardlabels_v10.15b.user.js

## 2026-08-12 — v10.15: King header stacked King/price badge

- Replaced the 3-badge header cluster (gold King price + distance chip "↑3 above" + net-drift chip "↓4 net") with ONE stacked pill: King strike on top, current SPY price below it (thin divider between), and a signed strikes-apart offset (e.g. +2 / −6) to the right.
- Offset = round(King − price). Color: King ABOVE price = red (overhead resistance), BELOW = green (support beneath), equal = gold/neutral — same convention as the sparkline dots.
- King price is the large emphasis line (14px gold); SPY price is the muted second line (11px); offset sized 13px so +5/−5 reads clearly.
- Net-session-drift is still fully described in the KING PATH sparkline tooltip and the drift pill below, so no signal was lost by dropping the header net chip.

Files: current/gex-signal-tapereader.user.js (v10.15), releases/2026-08-12_king-stacked-badge_v10.15.user.js, releases/2026-08-12_pre-v1015_v10.14.user.js

## 2026-08-11 — v10.14: srBattle S/R force engine + crossover flag (validated on real tape)

Replaced the S/R Bias bar's engine (static netPositioning, which only summed
BUILDING nodes and stayed green all the way down a grind) with srBattle() — a
DISSIPATION-DOMINANT support-vs-resistance FORCE model, designed and validated
against the user's real recorder dump (2026-08-11 PM turns).

Core logic: supportForce/resistanceForce = Σ proximity-weighted node build-rate
per side, with a HEAVY extra penalty (rate*1.5) when the NEAREST level (|k-px|<=1)
is Fading — because the tape showed the real turn is marked by the in-play level
DISSIPATING, not by breadth of far building nodes (support keeps rebuilding one
strike lower in a grind, so breadth stays green). Symmetric: a bear-pullback high
reads resistance-dominant (floor giving way + ceiling building); a bounce low
reads support-dominant (ceiling fading + floor rebuilding).

Validation (baked into test_srbattle.js from the actual dump):
- 1:30-1:42 grind (floor rebuilding lower): stays SUPPORT — correctly NO false
  short into rebuilding support.
- 1:48 breakdown (770 floor net -52): flips RESISTANCE + "BEARS TAKE OVER".
- Circle-5 pullback high (~14:45): RESISTANCE throughout.
- Bounce low C3 (~13:27): SUPPORT (ceiling fading, floor rebuilding +47).

UI: the S/R Bias bar is now the FORCE split (green support% vs red resistance%),
headline = Support/Resistance-dominant, plus a CROSSOVER banner — "▼ BEARS TAKING
OVER (pullback-high short trigger)" on SUP→RES, "▲ BULLS TAKING OVER (bounce
starting)" on RES→SUP. Tooltip shows the raw forces + nearest floor/ceiling state.

Confluence BREADTH contributor now reads srBattle.dom (dissipation-aware) instead
of the crude Building/Fading count; a fresh crossover intensifies its weight.
netPositioning kept ONLY as the separate CONTEXT vote (whole-board static tilt)
and setupGrade. Per-render memo (RENDER_SEQ + SRB_CACHE) so srBattle's crossover
state isn't corrupted by being called twice (bar + confluence) per render.

Known limitation (documented in tooltip): bounce-low confirmation can lag the
exact low by ~1 bar (the floor often still reads Fading at the bottom tick, flips
to Building a bar later) — it's a confirmation tool, not a bottom-picker.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
17/17; confluence 4 scenarios OK; srBattle 4 real-tape turns reproduced.

Files: current/gex-signal-tapereader.user.js (v10.14), test_srbattle.js,
srbattle_findings.md, releases/2026-08-11_srbattle-crossover_v10.14.user.js,
releases/2026-08-11_pre-v1014_v10.13.user.js.

## 2026-08-11 — v10.13: CONFLUENCE engine — one coherent thesis across all reads

Added a top CONFLUENCE strip that integrates the four independent signals into a
single directional thesis, so the panel tells ONE story (King lean → price
trigger → S/R entry → context confirms) instead of four scattered readouts.

confluence(sym) scores four weighted contributors, each voting bear(-1)/bull(+1)/0:
- LEAN (King, w1.2): kingVerdict().dir — the structural anchor.
- TRIGGER (Price, w1.2, +15% if a live setup agrees): trendVerdict state + a
  live BO/short setup — price must confirm the move.
- CONTEXT (Board, w0.8): netPositioning().dir — net board S/R tilt.
- BREADTH (Nodes, w up to 1.2): MULTI-NODE — nodeBreadth() counts ALL nearby
  nodes: resistance building overhead + support dissipating below = bearish;
  support building below + resistance dissipating above = bullish. One node is
  weak; several agreeing across strikes scales the weight (net/3, capped).
A direction is DECLARED only at >=3/4 aligned (weighted sign); otherwise
"MIXED / NO EDGE" — the tool stays quiet when signals conflict.

Top strip shows: ▲/▼/◆ thesis word + "N/4 aligned" + per-contributor chips
(LEAN/TRIGGER/CONTEXT/BREADTH each ✓ agrees / ✗ disagrees / – neutral, a
disagreeing one lights red so you see WHY conviction is <4/4) + an action line
("Short rallies into overhead resistance; magnet target 771" / "Buy dips…" /
"Signals conflict — stand aside"). Full explanation in the hover.

Section order: CONFLUENCE strip → King (LEAN) → S/R Bias (CONTEXT+BREADTH) →
BO (TRIGGER).

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
17/17; new test_confluence.js drives 4 scenarios (full bearish 4/4 = the user's
example with 2 resistance building + support fading; breadth-alone stays mixed;
conflict; full bullish 4/4) — all behave correctly.

Files: current/gex-signal-tapereader.user.js (v10.13), test_confluence.js,
releases/2026-08-11_confluence-engine_v10.13.user.js,
releases/2026-08-11_pre-v1013_v10.12.user.js.

## 2026-08-11 — v10.12: remove Trend section, King on top

Section order is now King → S/R Bias → BO (Trend section removed). The Trend
read is now carried by the King verdict + sparkline (dealer positioning) at the
top; structural levels live in S/R Bias below. structuralReadHtml() is left
defined but no longer rendered (harmless; can be deleted later).

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
17/17. No orphaned state (structuralReadHtml was purely presentational).

Files: current/gex-signal-tapereader.user.js (v10.12),
releases/2026-08-11_remove-trend-king-top_v10.12.user.js,
releases/2026-08-11_pre-v1012_v10.11.user.js.

## 2026-08-11 — v10.11: fix King path painted one color; per-segment coloring

Bug: v10.10 stroked the ENTIRE King staircase in the single CURRENT verdict
color, so a King that was bearish (red) most of the session but currently flat
showed a fully yellow line — hiding the real history.

Fix: each staircase segment is now stroked by ITS OWN local direction — green
where the King stepped UP, red where it stepped DOWN, gray for flat baseline
holds. The trailing hold-to-now keeps the last move's color. The line now
genuinely shows "red for most of the day, flat recently". The under-fill was
changed to a neutral gray tint (direction-agnostic context, no longer implying
a single verdict). Roll dots + gold current-King dot unchanged.

The current-verdict color still drives the Bull/Bear/Neutral PILL and the
magnet-target line (those are point-in-time reads) — only the LINE is now
historical/per-segment. verdictCol kept in kingSparkline signature but ignored
for the stroke.

QA: node --check OK; ends })(); FALLBACK + RECORDER PASS; King render test 17/17
(now asserts red segments present + line NOT single-yellow); segment-color unit
test confirms bearish session => red strokes, uptrend => green, verdict color
ignored for stroke.

Files: current/gex-signal-tapereader.user.js (v10.11),
releases/2026-08-11_king-persegment-color_v10.11.user.js,
releases/2026-08-11_pre-v1011_v10.10.user.js.

## 2026-08-11 — v10.10: King bull/bear verdict + verdict-colored path + magnet read

New kingVerdict() combines three independent King signals into ONE directional
call, shown as a colored pill next to the drift label and echoed on a new
magnet-target read line; the sparkline is now colored by that verdict.

Signals combined (drift-dominant):
- DRIFT (primary): net first→current King migration. Dealers re-centering
  exposure higher = bullish structural pressure; lower = bearish. ±3 cap, 1.6x.
- MAGNET (secondary): King vs price. Price gravitates to the King into expiry,
  so King ABOVE price = upward pull (bullish lean, target above), BELOW =
  downward pull, AT = pinned/no pull. ±1x.
- STABILITY (conviction, not sign): pinned ≥20m + no drift + no rolls =
  range-pinned (magnet strong, fade extremes; conviction ×0.6); ≥2 rolls in 60m
  = trending (momentum, go with drift; ×1.15).
Verdict: Bullish / Bearish / Neutral, plus a regime word (trending /
range-pinned / settling). When drift and magnet disagree it flags "mixed" in the
tooltip (and calls Mixed if the net score is weak).

UI:
- Bull/Bear/Neutral pill next to "drift up/down · N rolls" in the King-path title.
- Sparkline LINE + a faint fill tint now colored by the verdict (green bull /
  red bear / yellow neutral-flat), not just raw slope.
- New magnet-target line under the chart: "◉ Bullish · King magnet pulls price
  up toward 771 (2 strikes above)" or "Price is AT the King — pinned…". This is
  the actionable price-prediction output.
- Full drift+magnet+stability reasoning in the pill / chart tooltips.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
render test 15/15; kingVerdict unit test across 5 scenarios (bull/bear/flat/
mixed/trending) behaves correctly.

Files: current/gex-signal-tapereader.user.js (v10.10),
releases/2026-08-11_king-verdict-color_v10.10.user.js,
releases/2026-08-11_pre-v1010_v10.9.user.js.

## 2026-08-11 — v10.9: trend counter conditional, King moved under Trend + taller

1. Trend bar counter (e.g. "14/20 bars") now shows ONLY for a directional trend
   (Uptrend / Downtrend). On Sideways or Trend N/A there is no bias, so the
   count was noise — it's omitted; the clause just reads "Sideways." / "Trend
   N/A.".
2. Section order changed to Trend → King → S/R → BO (King moved up directly
   under Trend so the two directional reads sit together at the top).
3. King sparkline given more height (H 46 → 84) and switched to
   width:100%/height:auto with xMidYMid-meet so it fills the panel width and
   renders ~2x taller, making the drift staircase far more legible. Dots stay
   round (no distortion).

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
test 11/11; trend-counter logic test confirms Up/Dn show the count, Side/NA hide it.

Files: current/gex-signal-tapereader.user.js (v10.9),
releases/2026-08-11_king-under-trend-taller_v10.9.user.js,
releases/2026-08-11_pre-v109_v10.8.user.js.

## 2026-08-11 — v10.8: King path redesigned as a stepped time sparkline

Replaced the King-path chip row with an inline SVG **staircase sparkline** of
King STRIKE (y) vs real CLOCK TIME (x) across the cash session (8:30–15:00 CT).

Why stepped-on-a-time-axis: the King `moves` array is event-based (one entry per
confirmed roll), so equal-spaced chips misrepresented pace — a 2h hold and a
30-second double-roll looked identical. Plotting against real time makes a long
hold a long flat segment (truth) and a roll burst a tight cluster (instability).
The King is a discrete strike, so a smoothed line would imply values that never
existed; a staircase (hold flat, vertical jump at the roll timestamp, flat to
'now') is the honest shape. The silhouette IS the trend: climbing = bullish
migration, descending = bearish, flat = pinned, sawtooth = chop.

Details:
- Line colored by net drift (green = net higher, red = net lower, gray = flat).
- Roll vertices = small dots (green up / red down); current King = gold dot.
- Faint dashed line = current price, for King-vs-price convergence at a glance.
- y-axis extreme labels (top/bottom strike) + x-axis labels (first-move/open
  time on the left, "now · pinned Nm" on the right).
- Title row shows "drift up/down/flat · N rolls". Full explanation in hover.
- Consecutive same-strike moves collapsed; left edge anchored to session open
  (or first move), right edge to now.
- Single-session only (moves resets daily); a multi-day King trend would need
  the recorder feed.

New helpers: sessionBoundsCT() (8:30/15:00 CT epoch ms today) and kingSparkline().
Removed the now-unused horizontal auto-scroll snippet + #gpts-kingpath scrollbar
CSS.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
test rewritten (SVG present, stepped H/V path, gold dot, dashed price line, red
bearish line, drift-down label) 11/11. Standalone kingSparkline unit test also
passed (net drift, dup-collapse, flat-day).

Files: current/gex-signal-tapereader.user.js (v10.8),
releases/2026-08-11_king-sparkline_v10.8.user.js,
releases/2026-08-11_pre-v108_v10.7.user.js.

## 2026-08-11 — v10.7: drop the LOCAL tile from S/R Bias

Removed the LOCAL scope tile from the S/R Bias block. Its "nearest building
support vs resistance" read (relativeRead) duplicated information already shown
two ways: (a) the two S/R node rows straddling the price divider ARE those
nearest levels with their build states, and (b) the Trend sentence already
states the same near-term call with direction + targets. LOCAL was also the
noisiest scope (single-strike flicker) and read "Neutral" most of the time.

Also dropped the now-redundant BOARD tile: the board tilt is fully carried by
the header badge (Support-heavy N×) + the balance bar (green support% vs red
resistance%). The bar's label row now shows support% · dip-buy/fade bias ·
resistance% inline. Net effect: the block collapses to header badge → balance
bar, reclaiming a full tile row of height, with zero information loss.

relativeRead() is kept — it still feeds the "trap / clean forms" alert.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS;
King 10/10. No orphaned identifiers (locVerd/locTip/supTx/resTx removed).

Files: current/gex-signal-tapereader.user.js (v10.7),
releases/2026-08-11_drop-local-tile_v10.7.user.js,
releases/2026-08-11_pre-v107_v10.6.user.js.

## 2026-08-11 — v10.6: header cleanup, target color fix, S/R+Bias merge

1. **Removed the "D·S" setup-grade tag** from the panel header (grade element
   kept but permanently hidden so nothing referencing it breaks).
2. **Fixed T2 color.** Both T1 and T2 now use ONE rule: yellow if the target is
   a STRETCH (too far at the recent pace), else green if ABOVE price (long
   target), else red if BELOW price (short target). Previously T2 used a reach
   scale that painted an in-reach downside target (e.g. 769) green — now a
   downside target is red (or yellow if stretched). Exact distance stays in the
   badge tooltip.
3. **Trend body: abbreviations + trend-first.** The Read/Trend sentence now uses
   "Sup" / "Res" instead of Support/Resistance, and leads with a colored trend
   clause (Uptrend / Downtrend / Sideways (+bar count)) BEFORE the S/R text.
4. **Merged the two headers into one "S/R Bias".** The former separate "⚖ Bias"
   sub-header is gone; the board-tilt headline badge (Support-heavy /
   Resistance-heavy N×) now rides on the single S/R Bias header. The balance bar
   + BOARD/LOCAL tiles render below it unchanged.
5. **King path: subtle → connector between nodes.** Faint (PAL.sub, 55% opacity,
   8px) with zero horizontal padding + a slight negative margin so it sits in
   the existing gap and does NOT widen node spacing.
6. **Removed the per-day King roll-count badge** from the King header (knowing
   it's "42" carries no signal). Current King, distance, and net drift remain;
   the roll count still appears in the King-path hover tooltip.

QA: node --check OK; ends })(); single render(); undefined-id scan clean;
FALLBACK + RECORDER PASS; King test 10/10 (roll-count badge absent, → present).

Files: current/gex-signal-tapereader.user.js (v10.6),
releases/2026-08-11_header-target-color-srbias_v10.6.user.js,
releases/2026-08-11_pre-v106_v10.5.user.js.

## 2026-08-11 — v10.5: UI layout overhaul (Trend · S/R · King · BO)

Application structure is now **Trend → S/R → King → BO** (render order changed to match).

1. **Read → "Trend".** Section header renamed.
2. **T1/T2 are now color-coded, distance text row removed.** T1 keeps GEOMETRY
   color (green = above price / bullish, red = below / bearish). T2 now encodes
   REACH as color: green = in reach, yellow = stretch, red = far. Direction of
   T2 is inferred from T1's color (per user). Exact distance/pace lives in each
   badge's hover tooltip. The old "T1 +x/ystr | T2 ..." distance row is gone.
3. **Projected S/R moved into the S/R (ACM) section and abbreviated.** Now a
   single dashed "PROJ · Sup <k> <pct>% · Res <k> <pct>%" row (was the verbose
   two-line "STRONGEST ACCUMULATION" block + the Trend-section PROJECTED row).
   Same strongestAccumulator() data; just abbreviated and relocated.
4. **King section redesign.**
   - Removed the PINNED / SESSION / LAST HR quick-stat tiles.
   - Removed the "KING READ · Resistance at 770 is holding while 769…" sentence.
   - KING PATH reversed to **oldest → newest** with the current King on the
     RIGHT (gold), and the row auto-scrolls to the right end on every render so
     the latest is always in view.
   - Path chips now show the **price only**; time + roll direction moved to each
     chip's hover tooltip. Frees space for 5–6 chips per row.
5. **ACM → "S/R".**
   - Removed the in-play header badge (the "GK 772 / Accumulating" pill).
   - "Strikes that matter" filter: instead of a fixed 3-above/3-below ladder
     padded with idle Steady nodes, it now shows ONLY nodes that are actively
     Building or Fading, plus the in-play node and the King (structural
     anchors). Capped at 5/side with the new scrollbar for overflow.
6. Section order (above).
7. **Vertical scrollbar on the S/R ladder** (max-height 220px, thin dark
   theme). The SPY price divider is sticky so it stays in view while rows
   scroll.
8. **"SPY Signals" header → "BO"** (breakout-pullback tracker), Clear button
   retained.

QA: node --check OK; ends `})();`; single render(); undefined-identifier scan
clean; FALLBACK + RECORDER regressions PASS; King redesign test rewritten to
assert removed tiles absent, path oldest→current, price-only chips, scroll
container present — all pass.

Files: current/gex-signal-tapereader.user.js (v10.5),
releases/2026-08-11_trend-sr-king-bo-layout_v10.5.user.js,
releases/2026-08-11_pre-uilayout_v10.4.user.js.

## 2026-08-11 — v10.4: merged NET + Mixed into one "Bias" block

### Why
The "NET support-heavy 2.1x" line and the "Mixed" line were the SAME support-vs-
resistance question at two zoom levels (whole-board tilt vs nearest-levels edge),
rendered as two separate bordered rows. Redundant + "Mixed" was unclear. Merged them.

### New ⚖ Bias block (replaces the two rows)
- Section header "⚖ Bias" with a headline badge: "Support-heavy 2.1×" /
  "Resistance-heavy N×" / "Balanced" (colored by board tilt).
- Balance BAR: green (support/below) vs red (resistance/above), widths from
  netPositioning below/above totals (e.g. support 68% / resistance 32%).
- Two scope tiles:
  - BOARD = whole-board tilt -> "dip-buy bias" / "fade-short bias" + the N× ratio.
  - LOCAL ~N str = nearest building support vs resistance -> "Support edge" /
    "Resistance edge" / "Neutral" (relabeled from Long-friendly/Trap/**Mixed**),
    with the raw "sup 771 +0 · res 772 -1 /36m" build-rates.
- Full explanations kept in per-element tooltips.

### Relabels (display only; internal cls unchanged so nothing downstream breaks)
- Local verdict: Long-friendly -> "Support edge"; Trap risk -> "Resistance edge";
  Mixed -> "Neutral" (per user).

### Verification
- node --check OK; undeclared-identifier scan CLEAN; single render(); ends })();.
- Regression: DOM-tape parser PASS; recorder PASS.
- Bias-block logic test (live scenario): headline 2.1×, bar 68/32, BOARD dip-buy,
  LOCAL "Neutral", raw rates rendered, "Bias" label — PASS.
- @version 10.3 -> 10.4; banner + footer v10.4.

### Files
- current/gex-signal-tapereader.user.js  (v10.4)
- releases/2026-08-11_bias-merged_v10.4.user.js
- releases/2026-08-11_pre-biasmerge_v10.3.user.js  (prior, safety)

## 2026-08-11 — v10.3: UI cleanup + King section redesign/enhancement

### Header / Read (space savings)
- Grade badge "D · Short" -> "D·S" (one line; Long shows "X·L").
- Read target badges drop the T1/T2 prefixes -> value+role only (e.g. "771 K",
  "769 Fl"), keeping the trend badge. No more 2-line wrap.

### King section — full redesign + enhancement (was cluttered/confusing)
Header badges now on ONE line: value-only King (was "K 771" -> "771"); distance badge
fixed (the confusing "-> 0 below" now reads "at price" when distance is 0, else
"^ N above" / "v N below" with the arrow meaning SIDE, not roll dir); NEW net-drift
badge ("vN net" colored, = first King -> current); roll count is just the number
(was "23x today" -> "23").

NEW enhancements (all from the persisted KINGDAY roll timestamps):
- KING READ line: one-line structural interpretation, e.g. "Drifted down 775->771
  (bearish migration), now pinned 41m at price. Settlement gravity right here and
  holding — range-pin behavior; a confirmed roll away is what would start a trend leg."
  (descriptive + structural implication; no trade advice). Stable-pin logic takes
  precedence over "unstable" so the sentence never contradicts itself.
- Quick-stat tiles: PINNED (time held at current King) / SESSION (start->now drift) /
  LAST HR (roll velocity).
- Roll path redesigned: MOST-RECENT FIRST (left), older rolls scroll right, single
  non-wrapping row, CONSECUTIVE DUPLICATES COLLAPSED (kills the 775->788->775 clutter),
  current strike gold + "now·<pinned>" pinned, green ^ up / red v down + timestamps.

### QA hardening (after the v10.2 pkTip/pkMark runtime crash)
- Added an acorn-based undeclared-identifier scan to the verification pass; v10.3
  reports SCAN CLEAN (no runtime ReferenceError risks in the new King code).

### Verification
- node --check OK; single render(); ends })();.
- Undeclared-identifier scan: CLEAN.
- Regression: DOM-tape parser PASS; recorder PASS.
- New king-logic test (realistic KINGDAY dataset): net drift, at-price distance,
  pinned 41m, session 775->771, last-hr 2 rolls, dedup path, coherent READ line — PASS.
- @version 10.2 -> 10.3; banner + footer relabeled v10.3.

### Files
- current/gex-signal-tapereader.user.js  (v10.3)
- releases/2026-08-11_king-redesign-ui_v10.3.user.js
- releases/2026-08-11_pre-uicleanup_v10.2.user.js  (prior, safety)

## 2026-08-11 — v10.2: render crash fix (pkTip / pkMark undefined)

### Problem (why v10.1 was STILL idle)
Deployed v10.1 still showed idle. Live console revealed the REAL cause: a runtime
ReferenceError thrown every tick inside rowHtml -> accumBlock -> render:
  "ReferenceError: pkTip is not defined" (and a second, pkMark, on the compact path).
render() threw before it could draw King/ACM/Read, so the panel could never populate
regardless of feed/tape data. These were LATENT bugs in the Skylit-ACM base (variables
used but never declared); node --check can't catch them (runtime-only), which is why
they slipped through prior syntax checks.

### Fix
- Defined `pkTip` (the peak-drawdown tooltip fragment) BEFORE its use in the tip string,
  from the same r.state.fromPeak data that pkTxt uses.
- Defined `pkMark` (inline peak marker: green ★ when a Building node sits at/near its
  session peak, fromPeak<=3%) before its two uses (compact + full row). Defensive:
  empty string when data absent.
- Added a static undefined-identifier scan (acorn) to the QA pass; it now reports CLEAN
  (only false positive: the named IIFE 'walk' inside tapeCells, which is valid).

### Verification
- node --check OK; single render(); ends })();.
- acorn undeclared-identifier scan: CLEAN (no real undefined refs).
- Regression: DOM-tape parser snapshot PASS; recorder PASS; wallsFromTape fallback PASS.
- @version 10.1 -> 10.2; banner + footer relabeled v10.2.

### Note
This crash masked whether the v10.1 feed-hook/fallback works — with render fixed, the
panel should now populate AND the footer will reveal the data path (SPY:combined = XHR
hook caught the feed; SPY:tape = DOM fallback carrying it; SPY:idle = neither, needs
further diagnosis).

### Files
- current/gex-signal-tapereader.user.js  (v10.2)
- releases/2026-08-11_renderfix-pkTip-pkMark_v10.2.user.js
- releases/2026-08-11_pre-renderfix_v10.1.user.js  (prior, safety)

## 2026-08-11 — v10.1: feed-hook + DOM-tape fallback (fixes idle panel)

### Problem
v10.0 deployed correctly (title "Tapereader", "Clear", feed v10.0, 15× today) but the
panel went idle again: Read NA, King "Waiting on tape/K –", ACM empty, footer SPY:idle.
Diagnosis on live app: gex/levels API returns 200 (26 calls seen) AND the SPY heatmap
tape table IS present/readable ($388,977K king at 771) — yet LASTFEED stayed empty.
Root cause: (a) the feed hook only wrapped window.fetch, so Skylit's XHR-delivered (or
early/first) gex/levels responses were never captured; (b) refreshSym HARD-BAILED when
LASTFEED was empty, so even though the tape was readable the panel never populated.

### Fix (both, per user)
1. XHR hook: installFeedObserver now also wraps XMLHttpRequest.prototype open/send
   (captures the URL at open, parses responseText at load), alongside the existing
   fetch hook. Feed is captured regardless of transport / install timing.
2. DOM-tape fallback: new wallsFromTape(sym,px) synthesizes a walls[] structure from
   the visible Skylit tape (tapeMap): %King per node, King=100, pos=support/resistance
   by side vs price (abs/net null — not on the tape). refreshSym now: reads fiber price
   first; uses the network feed when live; ELSE falls back to wallsFromTape and still
   runs the machine. Panel populates off the visible tape even with no feed hook at all.
3. Footer status: shows "SPY:tape" (blue) when running off the DOM fallback, "SPY:<feed>"
   (green) on live feed, "SPY:stale"/"SPY:idle" otherwise — no longer misleading.

### Verification
- node --check OK; single render(); ends })();.
- Regression: DOM-tape parser BOTH live snapshots PASS; recorder PASS.
- New fallback test: wallsFromTape builds 8 walls from a shim tape, King=771 @100,
  below-price nodes = support, above-price = resistance. PASS.
- @version 10.0 -> 10.1; banner + footer relabeled v10.1.

### Files
- current/gex-signal-tapereader.user.js  (v10.1)
- releases/2026-08-11_xhrhook-tapefallback_v10.1.user.js
- releases/2026-08-11_pre-feedhook-fix_v10.0.user.js  (prior, safety)

### Next
- DEPLOY v10.1; expect panel to POPULATE (King/ACM/Read fill; footer "SPY:tape" if the
  network hook still misses, or "SPY:combined" if the XHR hook now catches it).
- If it now reads SPY:combined, the XHR hook was the fix; if SPY:tape, the fallback is
  carrying it — either way the panel is live.

## 2026-08-11 — v10.0: CONSOLIDATED MERGE (all versions unified)

### Why
Two divergent builds existed: (a) canonical current v9.2 — had the WORKING div/grid
DOM-tape reader + the DATA-layer recorder + side-aware badges, but was UI-lighter;
(b) /Skylit ACM Project/ — feature-rich (grade badge, King-journey timeline, N× today,
In-play + NET support-heavy read, Absorb state, Gatekeeper) but had the OLD broken
tr-only DOM reader and no recorder. A mockup showed the union of both plus a couple
of items. v10 UNIFIES everything into one file.

### Merge mechanics (chosen for lowest risk)
- Base = the feature-complete /Skylit ACM Project/ file (already has grade/king-journey/
  In-play/NET/Absorb/Gatekeeper and the VISIBLE King-tracker timeline).
- Transplanted my 3 self-contained v9.2 fixes INTO it:
  1. WORKING DOM-tape reader (div/grid Path B: tapeCells/leadTok/leadSignedPct,
     signed nearest-expiry %King, $K king detection) — replaces ACM's broken tr-only
     reader that would show idle on today's Skylit layout.
  2. DATA-layer recorder (gpts_recorder_v7): per-closed-bar node snapshots + setup
     outcome events for SPY+QQQ, 10-day rolling, quota-guarded; wired into tick() and
     runOutcome(); exports __gptsDebug.dumpRecorder()/dumpRecorderJSON()/clearRecorder().
  3. Side-aware ACM badges: building SUPPORT=green, building RESISTANCE=yellow (gold),
     Fading=red, Steady=blue — in stColor/stateColor, in-play header badge, and the
     STRONGEST callout.

### Mockup items
- Title set to "Tapereader" (compact, one line) per request — NOT "GEX Tape Reader".
  Grade badge (A·Long etc) sits beside it, exactly like the mockup.
- Header heights ~20% smaller (title bar 7->5px/13->12px; section headers 11->9px,
  padding 4->3px) — matches the v9.2 compaction.
- VISIBLE King-tracker timeline: already present in the ACM base (kingBlock renders the
  color-coded, time-stamped roll chain from the persisted KINGDAY journey — green up,
  red down, gray hold, each with fmtClock timestamp) + "N× today" move count. Confirmed
  included; no rebuild needed.

### Config / data safety
- Kept ACM's built-in v7->v8 config migration (loadCfg migrates gpts_cfg_v7 -> _v8 on
  first load, settings carry over). King journey persists under gpts_kingday_v1. My
  recorder adds gpts_recorder_v7. No existing keys wiped.

### Verification
- node --check OK; single render(); ends })();.
- DOM-tape parser regression: BOTH live snapshots ALL PASS in the merged file.
- Recorder regression: PASS (snapshot throttle, node fields, outcome event + context).
- Feature audit: title/Clear/grade/setupGrade/King-timeline/N×today/In-play/
  netPositioning/STRONGEST/absorptionAt/Gatekeeper/side-aware-gold/tapeCells/
  leadSignedPct/recordNodeSnapshot/recordOutcomeEvent/dumpRecorder all present.
- @version -> 10.0; banner + footer relabeled v10.0.

### Files
- current/gex-signal-tapereader.user.js  (v10.0 consolidated)
- releases/2026-08-11_consolidated-merge_v10.0.user.js
- releases/2026-08-11_pre-v10-merge_v9.2.user.js       (prior canonical, safety)
- releases/2026-08-11_skylit-acm-source_v9.1.user.js   (archived merge source)

### Next / open
- DEPLOY v10.0 to Tampermonkey; verify: "Tapereader" title + grade badge, compact
  headers, King-tracker timeline chain populates, green/yellow ACM badges, panel not
  idle (feed v10.0), and __gptsDebug.dumpRecorder() fills after a few bars.
- The old /Skylit ACM Project/ file is now SUPERSEDED — treat /GEX-Signal-Tapereader/
  as the single source of truth going forward.
- Analytics/prediction consumer of the recorder JSON = next project phase.

## 2026-08-11 — v9.2: UI compaction, side-aware ACM badges, DATA-layer recorder

### UI / space savings
- Title "Gex Signal Tapereader" -> "Tapereader" (single line, nowrap).
- Header button "Clear All" -> "Clear".
- Header heights reduced ~20%: top title bar padding 7px->5px, title font 13->12px;
  section headers (Read/King/ACM/SPY Signals) font 11->9px, padding 4px->3px,
  margins 5/3 -> 4/2. Applied to sectionHdr, sectionHdrRight, symSignalsHdr.

### ACM badge color now SIDE-AWARE (support vs resistance)
- Accumulation (Building) badge/strip/arrow color:
  - building SUPPORT (below price) = GREEN (unchanged, PAL.longAccent)
  - building RESISTANCE (above price) = YELLOW (PAL.gold)  [NEW]
  - Dissipating = red, Steady = blue (unchanged)
- stColor()/stateColor() made side-aware; in-play header badge and STRONGEST
  ACCUMULATION callout updated to match (resistance accumulator now yellow, not red).

### DATA layer recorder (feeds LLM analytics / prediction) — NEW, additive
- New storage key gpts_recorder_v7 (the 7 existing gpts_*_v7 keys untouched).
- Two streams per trading day, 10-day rolling retention:
  - snaps[sym]: once-per-closed-3m-bar snapshot of the whole node picture — price,
    King (feed + tape), in-play node, and every tracked node's strike/role/side/
    %King(feed)/tape-%King(signed)/state/net/rapid/roll/short-history. Throttled on
    lastClosedB so exactly one per closed bar. Hard cap 200/sym/day.
  - events[sym]: one row when a setup RESOLVES (T1/T2/FAILED/EXPIRED) via runOutcome,
    with setup facts (strike/dir/attempt/targets/bars/duration/boPct) + node context
    at resolution (in-play + nearest-strike node). _recorded guard logs each once.
    Hard cap 300/sym/day.
- Symbols: SPY + QQQ. All writes quota-guarded (drop oldest day on QuotaExceededError);
  wrapped in try/catch so the recorder can never break render or the state machine.
- Debug exports: __gptsDebug.dumpRecorder() (returns DB + logs summary),
  dumpRecorderJSON() (stringified), clearRecorder() (wipes recorder key only).
- Wired in tick(): recordNodeSnapshot('SPY'/'QQQ') after recordSession();
  recordOutcomeEvent() at the end of runOutcome().

### Verification
- node --check OK; file shape preserved (single render(), single IIFE, ends })();).
- All 7 legacy gpts_*_v7 keys intact; gpts_recorder_v7 added additively.
- Offline recorder test (localStorage+STATE+futureStructureSummary shims): no-op with
  no closed bar; 1 snapshot/closed bar/sym; same-bar throttle (no dupes); new bar ->
  new snapshot; full node fields captured; outcome event captured with context. PASS.
- Regression: both live DOM-tape parser snapshots still ALL PASS (no regression).
- @version 9.1 -> 9.2; footer + load banner relabeled v9.2.

### Files
- current/gex-signal-tapereader.user.js  (v9.2)
- releases/2026-08-11_pre-ui-recorder_v9.1.user.js   (pre-change snapshot)
- releases/2026-08-11_ui-badge-recorder_v9.2.user.js (this release)

### Next / open
- DEPLOY v9.2 to Tampermonkey; visually confirm compact header, one-line title,
  yellow resistance-accumulation badges, green support ones.
- Analytics: build the consumer that reads dumpRecorder() JSON and joins node
  pre-conditions to outcomes for the LLM prediction layer (out of scope this fix).
- Still pending from prior ladder: verify GO->T1/T2/FAILED/EXPIRED on live closed
  bars; no stray third target; same-tick persistence; failed never overrides a hit.

## 2026-08-11 — live verification (post-fix, no code change)

### Verified on live app.skylit.ai/atlas (SPY, 3m)
- Panel now POPULATES after a fresh reload: King "K 771", ACM nodes (775 Ceiling
  40%, 773.5 Cluster, 773 Cluster, King 771 = 100/100), footer "SPY:combined".
- Cross-checked against the live heatmap tape: King $-cell at 771 (matches K 771);
  775 first-% = 40% (matches ACM "Ceiling · 40%"). Consistent.
- DOM reader re-validated offline against TWO live snapshots (king top-of-table and
  king mid-table with heavy negatives): both ALL PASS (king + every checked strike,
  incl. negatives like 774=-18, 773=-82, 772=-7). count 23-24.
- No userscript runtime errors in console (only benign Chrome ext "message channel
  closed" noise, unrelated to GPTS).

### Architecture clarification (important for future debugging)
- runMachine() BAILS if there is no network feed: `LASTFEED[sym]` from the gex/levels
  XHR/fetch hook. King/ACM/walls/%King are built from that NETWORK feed (STATE.walls).
- The DOM tape reader (readTapeFromDOM, the div/grid fix) is used by livePctAt() to
  OVERRIDE %King so ACM matches the visible tape for real strikes, plus node-history
  and King-identity cross-check. It is a supplement, not the primary gate.
- Therefore the original "all idle" had TWO causes: (a) network feed not yet captured
  (primary gate), and (b) DOM reader broken for the div/grid layout (fixed this day).
  The panel recovering coincided with the feed arriving; the fix keeps ACM %s tape-
  accurate and King identity aligned.
- Note: for a real strike the ACM % can differ from the tape's first-column number,
  because walls use ABSOLUTE strength (n.v/king) while the tape first column is a
  SIGNED net-change. For cluster half-strikes (e.g. 773.5) there is no tape key, so
  the wall value is used. Both behaviors are by design.

## 2026-08-11 — DOM tape-reader fix (div/grid layout) — v9.1

### Problem
The Tampermonkey panel loaded correctly (feed v9.1) but every section sat idle:
Read "NA / Flat market", King "Waiting on tape… / K –", ACM "No nodes under
accumulation yet", SPY Signals "No active setups", footer "SPY:idle". Root cause:
Skylit Atlas now renders the heatmap strike table as a CSS-grid of <div> cells,
NOT <tr>/<td> rows. readTapeFromDOM() only ever handled the <tr> path (the promised
div fallback was a never-implemented stub), so table.querySelectorAll('tr') returned
0, no rows parsed, count stayed 0, tapeMap() returned null, and LASTFEED/content
never populated. This is validation task #1 ("live DOM-tape reading matches current
Skylit layout"): it no longer matched.

### Fix (readTapeFromDOM + helpers)
- Kept Path A (<tr>/<td>) unchanged for forward/backward compatibility.
- Added Path B for the current div/grid table:
  - findTapeTable() now also accepts a signed King dollar cell (−$…K / $…K, real or
    unicode minus) and a larger subtree budget (<800 nodes).
  - New tapeCells(): walks the grid and returns each TOP-MOST value cell's leading
    token, deliberately NOT descending into it. Skylit nests each column's change
    delta (e.g. "+3%") as a child INSIDE the value cell whose own text is the real
    value (e.g. "98%"), so the leading token = the true nearest-expiry %King and the
    nested chip is ignored.
  - New leadSignedPct(): reads the nearest-expiry %King WITH sign, because Skylit's
    first-column %King can legitimately be negative (e.g. 774 = −39%). The old
    unsigned firstStrengthPct() would have skipped the negative and mis-read a later
    column; leaf-flattening would have grabbed the nested chip.
  - King row detected by the $…K cell; row locked so a trailing chip can't overwrite
    the King's 100.

### Verification
- node --check: OK. File shape preserved: single render(), single IIFE, ends `})();`,
  all 7 gpts_*_v7 storage keys unchanged, @version 9.1.
- Offline end-to-end test of the REAL extracted functions against a DOM shim modeled
  on the live SPY table (own-text value + nested change-chip child): king=773,
  count=24, and every checked strike correct incl. negatives (774→−39, 772→−4) and
  King (773→100). Edge cases: no-table page returns null gracefully (idle, no crash).
- Live DOM used as ground truth: SPY heatmap table ref showed value cell "98%" with
  nested child "+3%", confirming the own-text-leads structure the parser relies on.

### Files
- current/gex-signal-tapereader.user.js  (overwritten with fix)
- releases/2026-08-11_pre-domreader-fix_v9.1.user.js   (old current, safety snapshot)
- releases/2026-08-11_domreader-div-grid-fix_v9.1.user.js  (fixed snapshot)

### Not changed / still open
- Path A (<tr>) still uses unsigned firstStrengthPct (pre-existing behavior; not
  exercised by live Skylit). Could be made sign-aware later for parity.
- LASTFEED (footer status) is fed by the gex/levels network hook, a SEPARATE path
  from the DOM reader; if the footer still shows idle after deploy while content
  populates, investigate the fetch/XHR hook timing next.
- Deployment to Tampermonkey remains a separate explicit follow-up (paste current/).

## 2026-08-11 — load + verification session (no code change)

### What happened
- New context window ran a full canonical load of all eight `/GEX-Signal-Tapereader/` files.
- Verified `current/gex-signal-tapereader.user.js` is BYTE-INTACT (md5 c6a54bbcce5a2896d863d79453d55286), `node --check` passes, final line is `})();`.
- Baseline confirmed: v9.1 uploaded-sync; `gpts_*_v7` keys intact; Trinity symbols (SPY, QQQ, SPXW, VIX) preserved.
- Confirmed approved patch state: `runOutcome(sym, last)` wired; `assignTargets()` restricted to T1/T2.

### Saved this session
- Refreshed `session-state/latest-resume-note.md` (added last-opened marker, md5 integrity note, explicit "ignore /Skylit ACM Project/" instruction; next-step list unchanged).
- Appended this changelog entry.

### Intentionally NOT changed
- `current/gex-signal-tapereader.user.js` (no edits — load/verify only)
- `master-spec.md`, `teaching-spec.md`, `design/architecture-design.md`, `workflow.md`, `developer-kickoff.md` (no rule/doctrine/architecture/contract change)
- No `releases/` snapshot (no meaningful code change to snapshot)

### Next
- Live-validation checklist per resume note (DOM-tape stability → GO outcome resolution → no third target → same-tick persistence → failed-never-overrides → accumulation regressions), then recorder-schema expansion beginning with time/session-truth capture.

---

## 2026-08-11 — uploaded userscript synced into canonical project state

### Sync result
- Synced the uploaded `gex-signal-tapereader.user.js` into `current/gex-signal-tapereader.user.js`
- Saved release snapshots before and after sync:
  - `releases/2026-08-11_pre-upload-sync_v9.1.user.js`
  - `releases/2026-08-11_uploaded-sync_v9.1.user.js`

### Notable code-state differences from the previous canonical baseline
- Preserved the Step 1 / Step 2 outcome patch (`runOutcome(...)` still wired; two-target rule still enforced)
- Added DOM-tape reading / tape-alignment logic for live `%King` and king identity
- Added short-horizon node-history strips and king-roll memory/tracking
- Added a more explicit dip-tolerant / absolute-value-based accumulation detector
- Included denser UI presentation changes such as combined signal-grid direction and broader drag handling

### Documentation updates
- Refreshed `session-state/latest-resume-note.md` for the uploaded-sync baseline
- Refreshed `design/architecture-design.md` to describe the DOM-tape / node-history / accumulation design direction
- Updated `master-spec.md` with the current intake/design emphasis note

### Next validation focus
- validate the DOM tape reader against the live Skylit layout
- validate live outcome resolution and persistence behavior
- then continue recorder-schema expansion from the new canonical baseline

---

## 2026-08-11 — design-document continuity added to save protocol

### Save-protocol enhancement
- Added a required persistent design document at `design/architecture-design.md`
- Updated the save routine so `save` / `save all` / `save everything` must ensure the design document exists and refresh it when architecture/design understanding changes
- Added the design document to startup/restart loading so a brand-new context window loads rules, code, state, history, and architecture intent together

### New artifact created
- Created `design/architecture-design.md` as the restart-safe app architecture/design explainer

### Purpose
- Make project restarts safer by preventing architecture/design intent from being trapped only in code or scattered notes

---

## 2026-08-10 — full shorthand-command hardening across Genspark surfaces

### Command-contract updates
- Hardened `save`, `update` / `claude update`, and deployment-prep commands in addition to the prior `load gex` hardening
- Applied the rules explicitly across all Genspark environments, including the Chrome extension
- Added blocking completion criteria and proof-of-completion reporting for each shorthand command family
- Added stop conditions so the assistant must say a command is incomplete rather than bluffing success when required reads, writes, or output delivery did not happen

### Required proof after each command family
- load: loaded files, baseline/version, approved patch state, next concrete step
- save: updated files, snapshot status, current canonical version/label, unchanged files, deploy-next prompt
- update: incoming code received, baseline before/after, changed files, snapshot status, unresolved diffs, deploy-next prompt
- deployment-prep: canonical source used, current version/label, inline code vs requested URL

---

## 2026-08-10 — command-contract hardening for Chrome extension

### Workflow / bootstrap rules updated
- Hardened `load gex` and related shorthand commands so they are blocking bootstrap commands, not loose intent phrases
- Extended the rule explicitly to all Genspark environments, including the Chrome extension
- Added mandatory post-load confirmation requirements: loaded files, current baseline/version, approved patch state, and next concrete step
- Added stop condition when any canonical startup file was not actually read
- Added `changelog/CHANGELOG.md` to the required startup read checklist everywhere the load procedure is defined

### Purpose
- Prevent partial project loads that sound correct but do not actually read the full canonical project state before continuing

---

## 2026-08-10 — v9.1 minimal recorder patch (candidate)

### Code changes applied to canonical current source
- Added `runOutcome(sym, last)` above `runMachine(sym)`
- Wired `runOutcome(sym, last)` into `runMachine(sym)` after setup progression and before `syncLog(sym)`
- Enforced the two-target rule in `assignTargets()` by changing `slice(0,3)` to `slice(0,2)`

### Behavioral intent
- Outcome engine is observational-only and closed-bar based
- Intrabar touch resolves T1 / T2
- Failed = close back through strike before any target hit
- Expired = no target hit by end of day (15:00 CT)
- Touch wins over same-bar reclaim
- Once a target is hit, failure no longer applies

### Documentation updates
- Updated `master-spec.md` with the canonical minimum ten-group recorder schema
- Updated `session-state/latest-resume-note.md` to reflect the new current baseline and next validation steps

---

# GEX Signal Tapereader — Changelog

## 2026-08-10

### v9.1 candidate save — header cleanup + live-market plan refresh
- Removed the duplicated inner `Read` and `ACM` labels from the status cards and then removed the left-side inner labels entirely so the chip headers are tighter and the outer section header alone names the module
- Preserved file shape (`render()` count, single IIFE, final `})();`) and all existing `gpts_*_v7` storage keys
- Updated the master spec to reflect the UTC-first timing model, the four-layer current-node / future-structure / state / output architecture emphasis, and the live-market capture mission
- Replaced the session-state note to reflect that UTC canonical timing is now architecturally fixed and should be validated live rather than treated as the main pending bug
- Added `live-market-plan-2026-08-10.md` as the latest Claude-ready live-market collection plan

### `code` alias added
- Added `code` as an accepted deployment-prep alias meaning: give the full deploy-ready userscript inline for copy/paste into Tampermonkey
- Updated `workflow.md` and `developer-kickoff.md` so `code` is treated the same as `give me code`

### Inline deploy-code preference added
- Set the default deployment behavior to paste the full userscript inline in chat for direct copy/paste into Tampermonkey unless the user explicitly requests a URL, link, or hosted file instead
- Updated `workflow.md`, `developer-kickoff.md`, and `master-spec.md` to document the inline-code-first rule for deployment-prep commands such as `give me code`

### External sync/update workflow added
- Added `update` and `claude update` as accepted project commands
- Defined a cross-assistant sync flow for pasting newer Claude-generated userscript code back into the Genspark project
- Clarified that externally supplied code can be treated as the authoritative incoming sync payload for that update operation, after which AI Drive resumes its role as the canonical store
- Updated `workflow.md` and `developer-kickoff.md` so syncs now overwrite `current/gex-signal-tapereader.user.js`, refresh `session-state/latest-resume-note.md`, append a factual changelog entry, and optionally save a release snapshot when the external update is meaningful
- Updated `master-spec.md` to document the cross-assistant sync rule

## 2026-08-09

### Shorthand command workflow added
- Added accepted project-load aliases to the operating docs:
  - `load`, `open`, `retrieve`, `continue`, and `get` forms for the GEX project / Tapereader
  - added the shorter alias `load gex` (and parallel short forms for open/retrieve/continue/get)
- Added accepted save aliases:
  - `save`, `save all`, `save everything`
- Added accepted deployment-prep aliases:
  - `prepare the script`, `prepare script`, `give me script`, `give me code`, `give me the code`, `give me the tampermonkey script`, `give me the javascript`, `prepare deploy copy`
- Defined default behavior for each shorthand command in `developer-kickoff.md` and `workflow.md`
- Clarified that `master-spec.md` and `teaching-spec.md` should also be updated when a session changes persistent rules, verified state, doctrine, or conceptual mapping
- Added the post-save behavior: after a save command, ask whether the user also wants the deploy-ready Tampermonkey script

### Project storage scaffold created
- Created persistent AI Drive project structure under `/GEX-Signal-Tapereader/`
- Added canonical folders:
  - `current/`
  - `session-state/`
  - `changelog/`
  - `releases/`
  - `probes/`

### Canonical project files imported
- Saved current userscript as `current/gex-signal-tapereader.user.js`
- Saved restart-safe handoff as `master-spec.md`
- Saved doctrine/teaching reference as `teaching-spec.md`
- Saved current resume note as `session-state/latest-resume-note.md`

### Verified current development baseline
- Current live code baseline: `v9.0`
- Structural-read layer integrated and previously verified live
- Current next engineering target: `v9.1`
- Immediate next coding task: **time-unit reconciliation**

### Operating model established
- AI Drive is now the persistent source of project continuity
- Tampermonkey remains the deployment/runtime target
- Future sessions should load from:
  1. `master-spec.md`
  2. `teaching-spec.md`
  3. `current/gex-signal-tapereader.user.js`
  4. `session-state/latest-resume-note.md`

---

## Changelog rules
- Append newest entries at the top of the file or as a new dated section
- Record only factual project changes
- Include version target when a code change is made
- Note whether a file is a candidate, verified build, or release snapshot
- Do not use the changelog as a substitute for the session-state note## v10.55 — 2026-08-18 — TREND / MAGNET / PULLBACK-NODE engine · rolling · FUTURES mode · engine-ready data · QQQ parity · SPXW confluence

**The mental model (user-taught).** A trend is an alternation of MAGNETS (the node price rallies TO) and PULLBACK NODES
(the node that forms on the counter-move and price DEFLECTS off — the level to sell from in a downtrend, buy from in
an uptrend). Lower-low (magnet) / lower-high (PB), each governed by a node; PB nodes APPEAR AFTER the move and ROLL
lower after each leg. The 50-SMA confirms the trend; rolling ceilings ARE the successive pullback nodes.

**Leg engine (`legEngine`)** per bar: dir from the SMA five-state · phase RLY/PB · magnet (capped at King) · PB ZONE
prediction while rallying ("expect a pullback node to form above, below 775.5 — sell level") · PB DETECTION the
moment a meaningful node appears/grows in the zone lower than the last · ROLL count (2 = signal, 3 = confirmed) ·
rolled-off levels lose target status, vacated zone tagged air · invalidation on close through the PB ("lower-high
broken") · a PB forming AGAINST the trend resets and flags weakening. Uptrend = mirror. NODEHIST records the nearest
ceiling/floor per bar so it is testable. `test_leg_engine` recovers 776 → 775.5 → 775 (synthetic replay of the
user's 08-17 sequence — no real 08-17 export exists in the repo).

**Surfaced** in the READ (RLY/PB/confirmed sentences in the user's vocabulary), the ⚑ "Pullback node formed" banner,
zone rows (`PB · 2nd lower`, `MAG · target`, dimmed `rolled off`), the decision line at a PB ("sell-side deflection ·
tgt magnet 773 · inval above PB 775.0", still behind contact + R:R gates), and the direction hover. The roll is a
score factor INSIDE the trend-primary hierarchy (+1 confirmed aligned / −1 against) — it never flips direction.
Multi-session rolling from FCHIST votes only at ≥3 sessions ("needs 3, have N" until then).

**FUTURES mode.** Chart symbol auto-detected each render (title/header). ES/MES → SPY tape · NQ/MNQ → QQQ tape ·
anything else → "No options tape for GC — levels unavailable" (never invented). Live EMA ratio from the futures
price ÷ underlying (footer `ES/SPY 10.068 (live)`); fallback last-good → constant with `≈` on EVERY converted level.
ALL displayed levels convert (King, SUP/RES, gate, zones, magnet/PB, drift, entry/tgt/inval, R:R, contact band);
only the futures value is shown. Underlying price + candles reconstructed by 1/r so trend/in-play/drift/R:R keep
working on a futures chart. Recording stays in underlying strikes. ⚙ override auto|SPY|ES|NQ.

**Engine-ready data.** `buildFeatureMatrix` → `matrix` in the export (one row per bar per sym, every feature +
regime + model stamp, four outcome labels + MFE/MAE). New non-voting predictors: timeToClose, barOfDay,
distToKing/Magnet, pbActive, rollCount, sessionRangePos, dayNet, PDC-rel, EVENT_TAG (⚙: FOMC/CPI/OPEX/half-day).
**QQQ parity** (spine/zones/leg/features run for the active underlying). **SPXW confluence** parsed from the trinity
ladder → `S` scored +1 when present, `S–` honestly otherwise.

Tests: leg_engine 57 · futures_mode 62 · roll_factor 54 · feature_matrix 42 · spxw_confluence 26 · qqq_parity 41;
feature_enrollment 772. Suite green except the 4 pre-existing stale.


