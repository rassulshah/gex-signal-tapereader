# RESUME NOTE — 2026-08-15 — Magnet-frame redesign session (Claude Fable 5)

**Baseline: v10.43 deployed-pending** (user still had to run install.bat / commit / TM update as of last check).
Repo: `C:\Dev\gex-signal-tapereader`. No code written this session — design decisions only.
**Next build target: v10.44** (scope locked below).

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

## 3. OPEN THREADS (resume here)

- **Node Map field discussions remaining: STRIKE·% · STATE (Acm/Diss/Steady + ±γ — resolve
  overlap with purple identity) · LIFE (Fresh/Tested/Delivered/Decaying)**. IDENTITY + ACTIVITY
  are DONE (above). User was reviewing "1 by 1" — continue that style: ONE item, discuss,
  confirm, next. (User pushback early in session for stacking two topics in one reply.)
- Ceil/Flr strength threshold constant (15% vs 20%) — pick at build time, tag ⚖.
- Defl→Push handoff timing ("a few bars") — pick constant, tag ⚖.
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

## 6. LIVE-DOM FINDINGS (Skylit, 2026-08-14 — useful for parsers)

- Header per tape column: "● King 0.5% ↑" = **distance spot→King in %** (verified SPY+VIX),
  NOT a K$ change. Scrapeable as a DIST cross-check. Amber dot rgb(251,191,36).
- King row cell: `$77,617K` + lucide-STAR svg inline (not an arrow).
- Signed dollars occur: `−$27,399K` (Unicode minus U+2212 — regexes must accept \u2212).
- 777 strike showed −72% purple-highlighted (−γ node) — Skylit purple convention confirmed.
- No K$ %change printed anywhere in the tape — our session calc is the only source.
