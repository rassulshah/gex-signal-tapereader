# Skylit Heatseeker — Documentation Archive

Saved copy of the Skylit Heatseeker documentation (docs.skylit.ai), captured **2026-08-12** for use in developing and improving the GEX Signal Tapereader userscript and its Analysis tab. Self-contained: all page text is Markdown and all referenced heatmap images are downloaded to `images/` with links rewritten to local relative paths.

Source of truth for the app's methodology. Every detector/verdict/read in the app should trace to a principle here (see the project's LEARNING-SPEC).

## Contents

### core-concepts/
- `core-concepts.md` — Nodes, polarity (positive/green-yellow = smooth; negative/blue-purple = wicky), **absolute value ranks (sign/color only flavors)**, magnets, King nodes (pin late / drive-off early), Gatekeeper nodes, midpoints, price delivery & **node retest decay (1st touch strongest / 2nd ~66% / 3rd+ ~33%)**, rate of change (rapid accumulation/unwind), rolling ceilings(bearish)/floors(bullish), Hedge nodes, **air pockets** (polarity sets violence), OPEX, Power Hour.

### read-the-heatmap/
- `how-to-read-and-use-heatseeker.md` — the **5-step framework**: (1) Identify Magnets, (2) Spot the King (session destination), (3) Define the Range (fade edges, avoid midpoints), (4) Watch Gatekeepers (+ **SPX/SPY/QQQ must agree or stand aside**), (5) Map the Flow (accumulation/dissipation/reshuffle — trade AHEAD of the reshuffle). This is the app's governing workflow.

### patternpedia/ (the detectors)
- `pattern-trend.md` — King far + one-sided skew, trend-side increases while counter-side fades; enter on pullbacks.
- `pattern-the-whipsaw.md` — >=2 high-value edge nodes + few nodes between; fade edges, avoid middle.
- `pattern-rainbow-road.md` — many scattered both-sign nodes, no clear range; stand aside.
- `pattern-the-gatekeeper.md` — high-value node between price and King; strength ratio vs the node beyond; reversal when >>1; failed test => reshuffle.
- `pattern-rug-setup.md` — **yellow (positive) node stacked above purple (negative) with no floor below** => violent nosedive when the yellow unwinds. Reverse-Rug = mirror. (Polarity IS the pattern.)
- `case-study-speculative-and-decoy-nodes.md` — far-OTM speculative/decoy nodes; feeds the King "decoy discount".
- `topping-patterns-bottoming-patterns.md` — **VEX (not GEX)** multi-day matrix: little upside accumulation + gatekeeper wall + downside stair-step across forward expirations = higher-timeframe TOP (bottom = mirror).
- `the-ten-commandments-of-using-heatseeker.md` — discipline/risk rules.

### best-practices-and-methodology/
- `best-practices.md` — **context not signals**; stay fluid; **asymmetric R:R over win-rate**; don't fight the map; **GEX/VEX confluence**; **SPX/SPY/QQQ confluence**.
- `limitations.md` — not 100%; low-liquidity/OPEX distortions; reshuffles; context > single signal.
- `common-pitfalls-and-mistakes.md`
- `faqs.md`
- `examples-and-case-studies.md`

### images/
44 annotated heatmap-ladder screenshots referenced by the pages above (the ground-truth examples used to reverse-engineer the detectors). Referenced from the `.md` files via `../images/<id>.png`.

## How the app maps to these docs
See the project batch spec (`/GEX-Signal-Tapereader/`): the 5-step framework governs the guided posture output; Core Concepts + Patternpedia define the King Tracker, Node Map, and pattern detectors (Gatekeeper / Trend / Rug / Whipsaw / Rainbow / Topping-Bottoming); Best Practices/Limitations define the "context-not-signals", tri-index-gate, and honest-degradation posture.

_Captured for offline/LLM reference. Not for redistribution — © Skylit._
