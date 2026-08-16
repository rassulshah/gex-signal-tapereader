# RESUME NOTE — 2026-08-16 (late) — v10.47 PHASE A BUILT (candidate); NEXT = verify live Mon, then Phase B

**Baseline: v10.47 candidate** (`@version 10.47`, footer "feed v10.47"), delivered as
`gex-v10.47-phaseA-install.bat` (places userscript + tests + docs, commits+pushes; user then updates
Tampermonkey from the raw URL). NOT yet verified on live tape — user was away 15 min; built while away.
**Verify Monday at open:** (1) header cluster + ①②③ visible above READ; (2) READ is one paragraph with a
verdict word and reads like the locked texts; (3) Node Map header is one CONT/REVERSAL sentence; (4) on a
sync failure only the one-line banner shows and the app still renders; (5) Flr/Ceil = the biggest node
each side (matches Skylit), "★ Mag · next" beyond them; (6) gate icon is the castle arch; (7) Defl cards
say "Defl · Gate" etc. Known-unfinished: RSHUF state (Phase B) so the sentence never says RESHUFFLING yet.
Phase A code map: `kingHeaderBlock`, `syncBannerHtml`, `readBlock44` (rewritten), `nodeMapSentence` /
`nodeMapSentenceHtml` / `_nmAcc` helpers, `pickEdge` inside `nodeMapModel`, `gateSvgSm`, deflectionBlock
card block. Test: `test_read_v1047.js`.

(Design-session note follows — still the spec for Phases B–D.)

**Baseline: v10.46** (userscript `@version 10.46`, footer "feed v10.46"), deployed. Repo
`C:\Dev\gex-signal-tapereader`, GitHub `rassulshah/gex-signal-tapereader`, TM raw URL:
https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-signal-tapereader.user.js
Data export `data/YYYY-MM-DD.json` at 15:01 CT + local scheduled task "GEX data push" — WORKING.

**This session (2026-08-15/16, weekend, Fable):** full design pass, ONE ITEM AT A TIME, of the
whole tool against Skylit's "How to read and use Heatseeker" (5 steps), Core Concepts, Execution
Doctrine, Charts-First, Trinity, Best Practices, FAQs. Everything below is LOCKED by the user.
Build spec = this note + `mockups/gex-v10.46-dashboard-mockup.html` (Dashboard + Analysis tabs;
Testing tab mockup still to do at start of Phase D).

## 0. STANDING RULES ADDED THIS SESSION (also in skills/gex/SKILL.md + developer-kickoff.md)
- **DISCUSS ONE ITEM AT A TIME.** One element per message, ask, wait, move on. Never dump the
  whole list of open items + fixes in one reply. (User had to say it twice — do not repeat.)
- **Abbreviations everywhere on the panel** (limited horizontal space): Defl, Ceil, Flr, Acm, Dec,
  Gate, Mag, BOw, FT, RShuf. Full words ONLY inside the READ paragraph and Node Map sentence.
- Plain language in sentences: never "mass"; say "the bigger nodes above/below", "support/
  resistance". Name every level. Numbers (%) only when Acm/Dec is the point.
- Gatekeeper icon = the white castle-gate `gateSvg()` — NOT 🚪.
- Bull/bear lean lives in the READ, never as a verdict pill.
- "Deflection" is the word for the event; no trade language (buy/sell/size/stop) anywhere.

## 1. LOCKED BUILD SCOPE — 17 ITEMS + 5 REFINEMENTS (v10.47, phased)

### PHASE A — DASHBOARD (build first)
1. **Header restored**: ★SUP | 👑 King pill (King strike + offset arrow, divider, gate-svg + Gate
   strike + offset) | ★RES, then the `5-STEP ①②③` icon row. Code exists in `kingBlock()` L6613–6705;
   split top slice into `kingHeaderBlock()`, call in `render()` before READ. NO verdict pill, NO charts.
2. **Sync gate → one-line banner**: at `render()` tape-sync gate (L7987) emit ONE red header line
   `⚠ STRUCTURAL READ OUT OF SYNC WITH TAPE` (full outOfSyncBlock detail in hover) and FALL THROUGH
   to the normal layout. No dimming. Footer keeps "⚠ tape".
3. **READ = ONE PARAGRAPH, D-style** (`readBlock44()` rewrite). Verdict word first
   (BULLISH / BEARISH / SIDEWAYS / TBD), then: destination + distance · what's in between + its
   record · support vs resistance state · King state ("getting heavier — dealers pulling price
   up") · ONE odds sentence 📊 · ONE watch level. Locked texts:
   - BULLISH: "BULLISH. Price is going up toward the King at 775.38, about a strike away. The
     Gate at 774.50 is in between and it has already held twice. Support at 774 is building
     while Resistance at 776.50 is fading. The King is getting heavier — dealers are pulling
     price up. This has worked 60% of the time at this distance, 74% in this hour. Watch the
     floor at 773.25: breaking it changes the read."
   - SIDEWAYS: "SIDEWAYS. Price is inside 773.25–776.50, near the midpoint. The King at 775.38
     is steady, neither side is building. Watch 776.50 and 773.25: a break with follow-through
     sets direction."
   - TBD: "TBD. The King at 775.38 is above price and pulling up, but Resistance at 776.50 and
     777.75 sit right above it and are building — they block the way. Support at 773.25 is
     steady, not helping. Watch the gate at 774.50: a hold means the King wins, a break means
     the resistance wins."
   - BEARISH mirrors BULLISH. Destination = King, or Flr/Ceil/next target when King isn't it.
4. **Range position** as wording only ("near the midpoint / near the floor / near the ceiling"),
   pos=(px−lo)/(hi−lo); no separate line, no %.
5. **Regime gate**: `regimeTag()`=chop → verdict forced SIDEWAYS, odds sentence dropped.
8. **Flr/Ceil REDEFINED (Skylit-consistent)**: on each side the LARGEST node (≥ ~40% of King, ⚖)
   over the whole visible map, King excluded unless only strong node. Guard: if largest is >~6
   strikes out and a ≥threshold node is closer, closer = working edge, far = next target.
   Classes: strong node BETWEEN price and edge = **Gate**; BEYOND edge = **next target** ("★ Mag ·
   next"); adjacent comparable = **cluster** (edge is a zone "776.50–777.75"). Header ★SUP/★RES,
   Range, READ all use the same two levels.
11. **3rd-tap warning**: on 3rd+ arrival Node Map sentence flips ("has held twice — a third tap
    usually fails (~33%)"); tap count red on the Defl card. (Review may later say: suppress.)
12. **Polarity in the why**: −γ node → sentence says reaction fast/momentum; deflection off −γ
    is counter-character.
13. **King already reached today** → READ says so instead of presenting it as fresh destination.
- **Node Map header = ONE plain sentence** replacing Range/Trend chips + imbalance line:
  `⑤ CONTINUATION through 774.50 toward the King at 775.38 because the gate is decreasing (▼8%),
  while the King above is accumulating (▲12%) and pulling harder. Support at 773.25 is
  accumulating too so the floor under the move is firm.`
  `⑤ REVERSAL likely at 776.50 — the ceiling is accumulating (▲14%) and has held twice, and
  777.75 behind it is accumulating as well, so resistance is stacking. Nothing below price is
  decreasing, so a deflection here would have support to fall back on.`
  `⑤ RESHUFFLING — 776.50 decreasing, 774.50 and 773.25 accumulating in the last 15 minutes; no
  call until the map settles.` Also TBD / NO NODE IN PLAY. Vocabulary: accumulating / decreasing.
- **Ladder rows**: 4 columns kept (Identity | Strike | State | Activity·Life); STATE is the loud
  column; Pull/Push chips WITHOUT toward-share %; "★ Mag · next" beyond edges; SPY divider.
- **Defl cards**: "Defl · Gate" / "Defl · Ceil", time, chips = Step-5 context at the moment
  (Acm ▲9% / Dec, +γ/−γ, Nth tap, settled / RShuf), grade once n≥20 else "● rec n".

### PHASE B — RECORDER
7. Gate-deflection **hour bucket** recorded (no UI).
9. **Rolling**: record Flr/Ceil strike per bar; Ceil→lower / Flr→higher flagged; READ may say
   "the ceiling has rolled down from 777 to 776.50" (bearish evidence).
10. **Reshuffle detector**: short-window rate of change across nodes (⚖ e.g. ≥3 nodes >20% in
    15m); RShuf state → Node Map sentence + verdict held TBD + Defl-card chip; recorded.
14. **Chart levels recorded per bar + per node (nearest + distance)**: VWAP, PDH/PDL/PDC,
    PMH/PML, IB30 H/L, POC/VAH/VAL. (User enabled on Skylit: VWAP, Session Levels = Prev Day
    H/L/C + IB30 + Premarket H/L; Volume Profile single profile. Swing/Asia/London/Opens OFF.
    Keep our own SMA-50 trend machine — no chart SMA.) Read via TradingView study series (fiber).
18. **Yahoo Finance HTF/ITF data (item 18, locked 2026-08-16)**: userscript-side fetch of
    `https://query1.finance.yahoo.com/v8/finance/chart/{SPY|QQQ|^SPX|^VIX|ES=F}?interval=..&range=..`
    (intervals 1m…1wk; 1m ≤7d, intraday ≤60d, daily unlimited; no key; chart endpoint does NOT need
    the cookie/crumb that quoteSummary does). Try plain `fetch` first, fall back to `GM_xmlhttpRequest`
    (needs `@grant GM_xmlhttpRequest` + `@connect query1.finance.yahoo.com`; verify unsafeWindow access
    still OK). New Layer-0 source `htfFeed` → cache `gpts_htf_v1` → `STATE.htf` → `snap.htf` in the
    export. Tier 1 daily/weekly at boot: prior week/month H/L/C, 20/50/200 DMA, daily ATR(14), gap vs
    ATR, position in weekly range. Tier 2 60m/15m hourly: 1h/4h trend + swings, 1h ATR. Joins item 14
    as "nearest chart level" per node; READ may cite an HTF level when a node sits on it. Cloud sandbox
    CANNOT reach Yahoo (blocked) — the browser fetches, the cloud reads the exported day file. Degrade
    silently if the endpoint changes. User's console check pending (CORS yes/no).
- **Naming**: `learning/rules.json` = the **tape reader mental model** (user, 2026-08-16): beliefs with
  evidence (rate, n, MFE/MAE, regime), mechanism note, confidence; demotable; the READ is the model
  narrating. It describes, never acts.
- Refinement 1: **MFE/MAE** on every outcome (magnitude, not just hit rate).
- Refinement 2: **walk-forward promotion** (must hold on 3 NEW sessions).
- Refinement 3: **regime tag on every rule/outcome**; regime = first miner split.
- Compute **GEX VWAP + VEX VWAP internally** (Σk·v/Σv + bands) from walls + LASTVEX → GEX/VEX
  overlap confluence (Best Practices #5). No chart toggle needed.
- Options **Flow** subpane: NOT this build (confirmation later). User's own settings advised.

### PHASE C — ANALYSIS TAB (mockup done) — "did the dashboard tell the truth today?"
① READ verdicts table (time, verdict, to, +30/+60m, reached, watch held/broke; hit rate vs quoted).
② Node Map calls (CONT/DEFL/RSHUF, node, because, resolved; rate by call type; 3rd-tap misses).
③ Deflections today + **Setup Performance bars** (deflStats; incl. "3rd+ tap"; A+≥75 A≥68 B≥58
   C≥45; unlock n≥20). ④ Flow (RShuf episodes, rolls + did price follow, Acm/Dec flips).
⑤ King one-liner + projection scorecard folded in. ⑥ Review (LLM worked/missed/start-tracking).
15. **Nightly review = OPTION 1** (user chose; NO token in cloud — GitHub token deleted): scheduled
    task in this app reads public repo raw URLs (today + prior days + rules.json), writes
    `review/YYYY-MM-DD.json` (worked/missed/why, start-tracking, threshold proposals, contradictions),
    delivers to user; **extend local "GEX data push" task to also commit `review/`** (ships in
    installer). `analysisBlock()` ⑥ fetches review file from raw URL or shows "awaiting review".
    Code changes stay PROPOSALS.

### PHASE D — TESTING PIPELINE + LEARNING ENGINE
16. **Question queue with lifecycle**: sources = ⑥ library, miner leads, nightly review, user;
    `proposed → testing → answered(📊,n) → refined | parked`. Auto-refinement: answered question
    with lift & n → spawn children adding one factor (`when:[...]` AND), keep children with
    positive lift over parent at n≥20, chain shown (52%→66%→71%→78%). Promotion: n≥20 AND
    3 nightly re-runs AND walk-forward. READ cites promoted only. Question library re-based on
    dashboard dependencies (King pull dist/hour, Flr/Ceil hold by state, Gate defl by tap, CONT
    by regime/hour, REV by hour, after-RShuf, after-roll, VWAP/level proximity). Miner factors +=
    call type, tap, polarity, RShuf, VWAP band, nearest level, range pos. New section
    "Leads → promoted".
17. **Learning engine = `learning/rules.json`** in repo: promoted rules {condition, rate, n, MFE/MAE,
    regime, mechanism note (LLM), promoted date, last re-verified}. Written nightly, read by
    userscript at boot next to `studyLoad()`; READ odds / Node Map why / Defl grades / chips read
    rules; decay: 3 runs under bar → demoted; steers what to test next. Never rewrites code.
- Refinement 4: **event tag** (FOMC/CPI/OPEX/half-day) one-click footer at open → day file.
- Refinement 5: **confidence tiers** on panel: 📊 promoted · ◐ lead · ⚖ hand-set.
- LLM roles: nightly review (why/mechanism/contradictions/new fields) + question generation.
  Everything else deterministic.

### DEFERRED (own build after A–D)
6. **Cross-index agreement**: SPY · SPXW · QQQ (+ VIX if readable) — Trinity sidebar is ON and
   shows all four Kings → multi-symbol parser (was scheduled 08-17). READ sentence "QQQ agrees /
   diverges — lower confidence"; Trinity 3/3 · 2/3 · divergent.

## 2. SKYLIT CONSISTENCY CHECK (done 08-16) — verdict
Consistent: magnets, abs-value rule, polarity, King=EOD anchor, gatekeeper def, tap decay 80/66/33
(`TAP_PROB`), lifecycle tags, descriptive-only. Fixed by items 8/9/10/4/11/12/13. Deferred: 6,
GEX/VEX overlap (compute internally), hedge nodes (low).

## 3. NEXT CONCRETE STEP
Phase A build. Before coding: confirm nothing else queued (standing rule), then implement A against
the mockup, unit tests, ship installer with TM raw-URL step. Then B → C → D.

## 4. WORKFLOW AGREEMENTS (standing) — unchanged + §0 above
Ask first · mockups first · one at a time · installer .bat delivery · model routing (delegate
mechanical work down) · every element has hover text for a new reader.
