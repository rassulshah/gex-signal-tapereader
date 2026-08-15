# RESUME NOTE — 2026-08-15 (late) — v10.44 BUILT + delivered; next = v10.45 Testing tab

**Baseline: v10.44** delivered as `gex-v10.44-install.bat` (self-contained installer: places files,
registers the GEX-data-push scheduled task, commits+pushes). Repo `C:\Dev\gex-signal-tapereader`,
GitHub `rassulshah/gex-signal-tapereader` (raw URL for TM:
https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-signal-tapereader.user.js).
**v10.46 SHIPPED**: derived GEX factors (deriveFactors→snap.deriv: net-sign, zero-gamma, HHI,
imbalance, call/put wall, ranks) + Testing tab ⑥ Recommended-tests (22 research-backed hypotheses,
📗/📙/📕 tags, ✅/⏳ readiness). Research: Barbon-Buraschi, pinning, SpotGamma/MenthorQ, Skylit VEX.
**v10.45 SHIPPED**: 🧪 Testing tab (question library, hypothesis builder + presets + __gptsHypo, pattern
miner studyMine, insights rule engine, coverage strip). STILL PENDING (next): Analysis-tab Insights
block + regime GATE (suppress trend/conf/King-verdict claims in chop). **Next build target: v10.46**
= regime gate + Analysis Insights. See CHANGELOG v10.44 entry for exactly what shipped; §8 below for the plan.
User's first-run to-dos after installing 10.44: click 📁 in the footer once to pick
C:\Dev\gex-signal-tapereader\data ("Allow on every visit"); confirm the scheduled task exists
(Task Scheduler → "GEX data push"). Verify footer "feed v10.44 · rec ●".

(Original session header follows.)
Repo: `C:\Dev\gex-signal-tapereader`.

## 0. What happened this session (one paragraph)

Element-by-element King-area walkthrough (user request: "1 by 1") turned into a foundational
reframe: **nodes are magnets — they attract (pull) and repel (push); all indicators must be
built around these qualities, split into DESCRIPTIVE (what the field is doing) and PREDICTIVE
(what it will do, always ⚖/📊-tagged and nightly-scored).** Mid-walkthrough the user simplified
scope: the entire King area (console + path chart + projected chart) comes OUT of the UI for
now; the Dashboard becomes a single column (Deflections → Node Map); the Node Map is the
primary magnet surface. All King/projection/episode recording continues silently.

## 1. v10.44 SCOPE — LOCKED DECISIONS

### UI
1. **Single column**: Deflections → Node Map. King console, King path sparkline, and
   King path·projected ALL removed from Dashboard. Footer gains a small "rec ●" marker
   showing background recording is alive.
2. **IDENTITY column (final)**: 👑 King · 🚪 Gate · **▔ Ceil / ⛰ Flr** = nearest strong
   magnet (strength ≥15–20% of King mass — pick constant, ⚖) above/below price = the live
   range (Step 3 "define the range"). Roles stack ("👑 King · Flr"). Other nodes: ★ Mag
   (strong) / Mag (minor). **Sup/Res vocabulary retired** (it smuggled predictions).
   🍆 Barney/Cell purple −γ identity kept.
3. **Purple −γ convention (Skylit-matching)**: −γ King renders PURPLE everywhere it appears
   (user chose "everywhere": tile/pill/sparkline/nodemap — now mainly Node Map row since
   console is hidden). +γ/default stays gold. Polarity tag text can drop where color carries it.
4. **ACTIVITY column (final vocabulary + priority)**:
   `Pull` (w/ toward-share %) → `BOw` (at node; covers watch AND initial break, per user) →
   `BO·FT` (the ONLY breakout chip displayed; internal TST/CONF/GO tracking continues for
   scorecard only) → `Defl ↑/↓` (confirmed event; after a few bars hands off to) →
   `Push` (w/ toward-share %) → outcome echoes (`broke ↑/↓`, `held`, `FBO` — user chose KEEP).
   Priority on overlap: fresh Defl > BO·FT > BOw > Push > Pull > echo.
   Push off a node BELOW price = green (bullish bounce); above = red.
5. **FT (follow-through) REDEFINED — both directions**: existing full-hold rule OR
   **two consecutive directional closes beyond the node with the 2nd close progressing**
   (up: 2 green closes above, 2nd > 1st; down: 2 red closes below, 2nd < 1st).
   Applies to the internal setup tracker too, so BO stats use the display definition.
6. **Range events**: Ceil/Flr break+FT ⇒ range redefines to next strong magnet; echoed in
   map + scored nightly (retest-from-other-side rate). Header chip: "Range 775–777 · inside".

### Background / data layer (ships in v10.44, no UI)
7. **Keep recording everything hidden**: kingAnalyzer, KD_TRACK, snap.kd, snap.proj, King
   path history, projScorecard nightly. Charts return later validated.
8. **%KCH** (renamed from K$ change): percent change of King $ magnitude vs TRUE session
   open, quote-page convention. **Persist the day's opening K$ (keyed date+symbol)** so a
   mid-session reload keeps the real baseline (current in-memory KD_TRACK resets — bug).
9. **Magnitude by design**: parseKingDollarsK must `Math.abs()` explicitly + comment.
   VERIFIED LIVE: Skylit prints SIGNED King dollars (saw `−$27,399K` on 2nd-expiry column).
   Also: the SIGN of the King's own $ figure is a candidate direct polarity source
   (negative $ = −γ) — evaluate vs walls-derived pol.
10. **%KCh day-direction study** (user hypothesis): nightly, compute %KCh at checkpoints
    (10a/11a/12p/2p) × King-position (above/below/at) × polarity → day close direction.
    Buckets ⚖ until n≥20 → live 📊. Data already exists since v10.39 (snap.kd).
11. **Episode engine (per node, not just King)**: state machine PULL → contact
    (PIN / BREAKING / BLOCKED-by-gatekeeper) → Defl → PUSH (sub-labeled after-tag /
    after-break / after-block). Per-crown/per-node episode memory (taps, crossings,
    nearest approach, gatekeeper hit), resets on relocation. Toward-share = % of last
    10 bars closing nearer (baseline 54–60% in-gate; PULLING ≥60, PUSHING ≤40, ⚖).
    **Per-bar record `snap.ep`** with ctx at contact: {kch, pol, phase, gatekeeper, trend side}
    and outcome fill-in. THIS IS THE CORE NEW DATA.
12. **episodeScorecard() nightly** + **PREDICT-PUSH arm** (conditions BEFORE the push:
    PULL≥65% + tag + %KCH bleeding + −γ → push next 3 bars — scored as forecast).
    Every PUSHING sub-label its own bucket. Misses scored too.
13. **Nightly LLM review MUST answer 3 questions** on the episode log: WHY each push/hold
    happened (mechanism) · WHAT conditions preceded (candidate predictors) · WHAT to change
    (windows, thresholds, new buckets). Recs land in Analysis tab. (Candidate finding from
    design review: 10-bar toward-share window too slow for fast pushes; test adaptive
    6-bar in POWER/EARLY.)
14. **Hover text everywhere new** (user: "whoever is reading it can understand what this is
    and how to use it") — %KCH hover text drafted in session; every ACTIVITY chip gets a
    full episode-timeline hover ("PULL 12:12→12:41 (tw 71%) · tag 775.38 · PUSH ↑ since").

### Shelved (return-spec ready, do NOT build in v10.44)
- ATTRACTION tile v2 (contact-aware) — mockup `dist_attraction_mockup.html`
- %KCH tile flip (percent headline, $ secondary) — for when console returns
- Net-force field indicator (sum of mass/distance-weighted pulls — explains pins/trends/
  whipsaw; descriptive first, scorable later). DISCUSSED, user approved direction.
- READ-line wording: drop "bleeding/building" words, arrowhead only, at ±15% threshold.

## 2. MAGNET FRAME (doctrine for all future indicators)

Node = magnet with: strength (mass), polarity (±γ = contact personality: +γ sticky/pin,
−γ wicky/accelerant), radius (gravity ≤3 strikes, 📊 n=68 beyond = coin-flip), charge trend
(%KCH per node). Mode is NOT fixed: attract → contact → repel is the lifecycle; the FLIP is
the tradeable moment. Every indicator must be descriptive (field now) or predictive
(⚖/📊 + nightly scoring) — nothing vague in between. User's chart showed 3 worked examples
(2026-08-14): 779 top-push (HOD), 777 mid-trend gatekeeper block (bear-rally rejection,
King never reached), 775 bottom-push (LOD) → afternoon pin 776. Session = chain of episodes.

## 3. NODE MAP — ALL 5 FIELDS DECIDED (2026-08-15, later in session)

- IDENTITY: see §1.2 (Flr/Ceil range pair, 👑/🚪, ★Mag/Mag, purple −γ, Sup/Res retired).
- STRIKE·%: **unchanged** (user: "this is fine").
- STATE: **Acm / Dec / Steady** — words KEPT (Step 5 explicitly names accumulation → Acm is
  doctrine); "Diss" RENAMED "Dec"; each word carries the node's **▲/▼% vs its own session
  open** beside it (`Acm ▲12%`, `Dec ▼29%`), threshold-colored (big bleed bright red, small
  drift dim). **±γ text tag DROPPED — purple/default color carries polarity** (hover explains).
- ACTIVITY: see §1.4–1.5 (Pull/BOw/BO·FT/Defl/Push/echoes; lenient FT both directions).
- LIFE: **unchanged** (Fresh/Tested/Delivered/Decaying — user: "fine").

## 3b. OPEN THREADS — DISCUSSION COMPLETE; NEXT = BUILD v10.44 PER §8 (§8 supersedes)

- **User must be ASKED before build starts** (standing rule) — confirm nothing else queued.
- Constants to lock at build, tag ⚖: Ceil/Flr strength threshold (15% vs 20% of King mass);
  Defl→Push handoff bars; per-node % color thresholds (suggest ±15% bright, like King).
- Build to `design/nodemap_v1044_mockup.html` + §1 + §3. Deliver as ONE self-contained
  installer .bat (base64 payload, git-finder) with Tampermonkey raw-URL hyperlink step.
- After build: rewrite KING-FIELD-GUIDE.html for the single-column reality.
- KING-FIELD-GUIDE.html needs a v10.44 revision AFTER build (King area hidden; Node Map
  magnet vocabulary; new ACTIVITY definitions; FT rule; %KCH; episode engine).
- v10.43 deploy may still be pending on user's machine — confirm before building v10.44
  on top.

## 4. MOCKUPS DELIVERED THIS SESSION (design/ folder)

- `king_polarity_color_mockup.html` — purple −γ King tile variants
- `dist_attraction_mockup.html` — ATTRACTION tile v2, contact-aware 6 states (SHELVED spec)
- `node_episode_mockup.html` — episode engine: Node Map states + snap.ep + nightly loop + LLM review
- `nodemap_v1044_mockup.html` — **THE v10.44 BUILD SPEC**: single column, Flr/Ceil, Pull/Push,
  purple King, range chip, rec● footer

## 5. WORKFLOW AGREEMENTS (standing user instructions)

- Before coding: ASK — user may have more fixes; show MOCKUPS for review first.
- Discuss one element at a time ("1 by 1"), confirm, then move on.
- Every deploy: give instructions; changelog lives in GIT not Drive; Tampermonkey raw-URL
  as clickable hyperlink; install.bat pattern (files land at repo root — avoid the
  nested-folder extraction trap).
- Hover/tooltip explanations on everything new.
- "load gex" / "save" procedures: see skills/gex/SKILL.md (drafted 2026-08-15).
- MODEL ROUTING (user-mandated 2026-08-15): cheapest model that does the job. Delegate
  mechanical/well-specified work (code-to-spec, tests, packaging, search, scraping,
  formatting) to subagents on Opus or lower via the Agent tool `model:`; reserve Fable for
  judgment work (design/architecture, stats interpretation, deciding what to build, review).
  Fable stays the orchestrator/reviewer; lower models do the legwork.
- **DELIVERY (user-mandated 2026-08-15): ONE self-contained installer .bat per delivery**
  — payload base64-embedded in the bat (no zip/extract step), self-decodes, places files,
  commits+pushes via git-finder (git NOT on user PATH; GitHub Desktop bundled git is a
  fallback). User's only action: download + double-click.

## 6. LIVE-DOM FINDINGS (Skylit, 2026-08-14 — useful for parsers)

- Header per tape column: "● King 0.5% ↑" = **distance spot→King in %** (verified SPY+VIX),
  NOT a K$ change. Scrapeable as a DIST cross-check. Amber dot rgb(251,191,36).
- King row cell: `$77,617K` + lucide-STAR svg inline (not an arrow).
- Signed dollars occur: `−$27,399K` (Unicode minus U+2212 — regexes must accept \u2212).
- 777 strike showed −72% purple-highlighted (−γ node) — Skylit purple convention confirmed.
- No K$ %change printed anywhere in the tape — our session calc is the only source.

## 7. TEST BATTERY RESULTS (2026-08-15, in-page on 4 days / 391 bars, out10 = next 30m) — DRIVES v10.44/45

Method: window.__R rows from gpts_recorder_v7; force(k,pct)=sign(k-px)*pct/|k-px| for 0.25<|d|<=3.
CAVEATS: 4 chop days, overlapping 30m windows, polarity only since 08-14, QQQ/VIX = 0 bars recorded.
- Net force (sum of pulls): 50% (n=387); magnitude no size effect (0.46/0.37/0.41); flips 51%;
  disagreements vs King: King right 24/39. => RETIRED as headline. NOT a chip.
- DECOMPOSED: King PULLS 55% (n=299; 62-63% on 3 days, 37% on 08-13 = bleeding King far above =
  crown-death case). NON-KING mass REPELS 57% away (n=350; 66% 08-12). Combined "King pulls,
  others push" 56%; 58% when agree (n=200); conflict ~coin-flip.
- King pull by DISTANCE: 1 strike 50% (n=201) · **2 strikes 69% (n=59)** · 3: 25% (n=8) · 4+: 50%.
  => ORBIT (<=1) / PULL ZONE (1.5-3) / OUT. Pull claims only arm in PULL zone.
- King pull by HOUR (CT): 9a 43% · 10a 46% · **11a 74% (n=42)** · 12p 48% · 1p 48% · **2p 66% (n=38)**.
- Contender >=60%: price moves AWAY 58% (n=113) — second confirmation of repel.
- Level reached within 30m: <=1.5 strk 19% base (n=774); <=0.75 45%. By STATE: Building 15% /
  Steady 20% / Fading 23% => Acm walls hold best. By strength: no effect (20-22%).
- Field density high (non-King mass within 3) => smaller moves 0.35 vs 0.47. Air pocket filled 59%
  (n=158) but SMALLER moves (0.38 vs 0.43).
- LEGACY SIGNALS CONTRARIAN on these days: King verdict bull/bear 42% (n=273; 29% on 08-14) ·
  confluence 38% (n=74) · trend up→up 45%, up-broken→up 34% (n=38) · Node "Break-through" verdict
  8% (n=36, base 19%) · regime dir 45%. Only positive: srBattle dom 58% (n=136). => regime gate needed
  (chop artifact likely) — v10.45. Break-through verdict retired (already, via ACTIVITY redesign).
- BUG: recorder out10.hitKing fires 1-2% of bars — implausible; likely compares to pre-fix
  magnitude-as-King. FIX in v10.44 before any King-reach study.
- Pattern-miner leads (⚖): "King below · Dec · dist 1-2 → DOWN 64% (n=44)"; "trend up-broken → DOWN 66%".

## 8. v10.44 / v10.45 SCOPE (user-approved 2026-08-15) — SUPERSEDES earlier lists where they conflict

v10.44 (build next):
- Node Map redesign (§1, §3) + purple −γ + Pull/Push + Flr/Ceil + Acm/Dec/Steady + %.
- DIST → ZONE: ORBIT (<=1) / PULL (1.5-3) / OUT; hour gate (11a, 2p strong; 9-10a weak) on Pull claims.
- READ ▸ section on Dashboard (single column: READ → Deflections → Node Map) that CITES MEASURED
  RATES from the study store: e.g. "King 2↑ PULL zone · 11a window — 📊 74% (n=42) · Ceil 777 Acm —
  walls like this held 85%". Only measured (📊) or ⚖-tagged claims; no legacy verdicts.
- Node Map hovers carry measured hold/repel rate for that node's state+distance.
- DATA REPOSITORY: IndexedDB store (unbounded; every bar; SPY + QQQ + VIX; nodes pct/state/pol/tape%;
  signals; episodes snap.ep; projections; forward outcomes) — migrate gpts_recorder_v7 on first run.
  DAILY EXPORT (user decision 2026-08-15, git chosen over Drive): script writes data/YYYY-MM-DD.json
  DIRECTLY into the repo folder via File System Access API (one-time folder pick, "allow on every
  visit"); v10.44 installer registers a Windows scheduled task (schtasks, weekdays 15:30 CT, run-if-
  missed at next logon) that git add/commit/push data/ using the git-finder. Footer shows
  "saved 15:01 ✓ · pushed ✓". Manual "💾 save day" button remains as fallback. LLM reads data via
  raw GitHub URLs on load gex / nightly review (repo is public — TM already updates from raw URL).
  Coverage strip (days·bars·symbols·fields-since).
- Record QQQ + VIX every bar (currently 0 bars!). Record realized-range regime tag per bar.
- hitKing labeler fix. %KCH persistence + abs(). Episode engine + episodeScorecard + PREDICT-PUSH.
  %KCh day-direction study. Test battery as re-runnable nightly module (feeds READ + hovers).
v10.45:
- 🧪 TESTING TAB (mockup design/testing_tab_mockup.html): ① question library (nightly rerun, n,
  per-day stability, ⚖/📊, retired rows kept) ② hypothesis builder (WHEN factors → outcome; lift vs
  baseline; save to library) ③ pattern miner (factor×outcome scan, min n=30, multiple-testing
  correction; hits enter library as ⚖) ④ insights & recs (rule engine + nightly LLM: what data says /
  change product / improve testing / next hypotheses) ⑤ data coverage.
- Analysis tab "Insights" block (top measured edges + what changed overnight).
- Regime gate (chop vs trend) suppressing trend/conf/King-verdict direction claims in chop.


## 9. v10.46 SPEC (scheduled 2026-08-17 13:35Z / 8:35 CT — trigger trig_01PXXE978tzzq5Pv4pcmN4gA)

WHY SCHEDULED: the multi-symbol tape parser MUST be built against a LIVE market-hours DOM
(after-hours Skylit collapses the columns; a blind parser risks regressing the working SPY path).

Coverage strip evidence (v10.45, 08-15): "SPY 474 bars", NO QQQ/SPXW/VIX. Fields kd/pos/proj/ep
since 08-14; xm/rg not yet present (added v10.44, no live bars since install). So today ONLY SPY
full ladders are saved; QQQ is in RECORDER_SYMS but captures 0 (its column is never parsed into
STATE['QQQ']); SPXW/VIX not recorded. The `xm` header scrape (readTrinityHeaders) covers all 4
symbols' price/%chg/King-distance but is header-only, not the ladder.

BUILD (user chose: FULL ladders all 4, bundled with regime gate + Analysis Insights):
(a) MULTI-SYMBOL PARSER — the centerpiece. Skylit `.chart-trinity-sidebar` has one column child
    per symbol (SPY/QQQ/SPXW/VIX), each column's text starts with the symbol name. Add an ISOLATED
    per-symbol column parser: find the column whose leading text == sym, run the proven grid parser
    (tapeCells + Path B / kingResolve) scoped to that column subtree. DO NOT touch the SPY path —
    separate code path so SPY can't regress. VIX = distinct type (vol points, not $): record, tag it.
    Wire tapeMap(sym)/refreshSym(sym)/futureStructureSummary(sym) so STATE[QQQ/SPXW/VIX] populate.
(b) RECORDING — RECORDER_SYMS = ['SPY','QQQ','SPXW','VIX']; recordNodeSnapshot each per bar → full
    ladders into IndexedDB repo + daily export. Add CONFLUENCE factors to the Testing miner +
    hypothesis builder: per-symbol King side/distance agreement vs SPY (was untestable = 0 bars).
(c) REGIME GATE — in readBlock44 / claim sites: when regimeTag(cs).tag==='chop', SUPPRESS
    trend/confluence/King-verdict DIRECTION claims (they ran contrarian: 42%/38%/45% on chop days).
    Descriptive magnet claims (pull/repel rates) still show. Tag the gate ⚖.
(d) ANALYSIS-TAB INSIGHTS block — render testingInsights() (already built v10.45: says/change/
    improve/next) inside analysisBlock() via `try{ h+=... }catch(){}` at ~line 7514 pattern.
TEST + SHIP: full test_*.js suite; add test_multisym; bump @version 10.46; changelog + MAGNET-
FIELD-GUIDE; ONE installer .bat (git-finder, base64) + TM raw URL. Model-routing: delegate recon/
tests/docs to Opus subagents, keep parser design+validation on main model.

CAVEAT for the scheduled session: needs the user's browser connected to Skylit (Claude-in-Chrome).
If tabs_context_mcp shows no Skylit tab, DO NOT guess the parser — prep everything else, message the
user to open Skylit, stop. (Autonomous scheduled sessions may lack the browser bridge; that's the
known risk of scheduling this — main-model judgment required at run time.)


## 10. PREDICTION-TARGET FRAMEWORK (user north-star, 2026-08-15) — organizes ALL tests

GOAL of the whole Testing effort: predict PRICE. Three linked targets, 15–30m horizon:
  DIRECTION (up/down) · CONTINUATION (move keeps going) · REVERSAL/DEFLECTION (turns at/off a node).
Already recorded in out5/out10 — no new outcome capture needed:
  DIR = sign(out10.net)
  CONT = sign(out5.net)==sign(out10.net) AND |out10.net|>=|out5.net| AND no rev flag
  REV  = revUp||revDn || sign(out5)!=sign(out10) || confirmed deflection at the interacting node
Magnet mapping: Pull-that-holds = CONTINUATION toward a node; Push/deflection = REVERSAL off it.
PREDICTORS = node attributes (add %King-tier + rapid-accum as first-class miner factors):
  %King tier (King=100/>=60/30-59/15-29/<15) · polarity ±γ · RAPID ACCUMULATION (reshuffle+%chg rate)
  · accumulation state (Building/Fading+%chg) · distance zone (orbit/pull/out)+side · structure
  (zero-gamma regime, HHI, imbalance, walls — snap.deriv) · episode state (Pull/Push/toward-share)
  · (v10.47) VEX field + cross-symbol confluence + VIX own-structure.
=> v10.47: score every factor vs all 3 targets; retag RECO_TESTS by DIR/CONT/REV; the tab reads
   "these predict direction / continuation / reversal."

VIX CLARIFICATION (user, screenshot): VIX is a FULL symbol with its own ladder (King ~15.5, per-
strike mass). We HAVE: VIX spot+direction (header) AND VIX's full gamma ladder. We record it as the
4th symbol + its own deriv + a lightweight risk overlay. The ONLY VIX gap = TERM STRUCTURE
(VIX/VIX3M backwardation) — needs VIX3M, an external feed; that single test is parked "needs
external feed". All other VIX tests (direction, own-King, vanna melt-up) are in.

WORKING DEFAULTS proceeding on (state, refine if user disagrees): the CONT/REV definitions above;
VIX recorded as a full symbol AND surfaced as a risk overlay (both).
