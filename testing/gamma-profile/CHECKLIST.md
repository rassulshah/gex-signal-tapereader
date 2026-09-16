# lsGammaProfile — the eyes-on render checklist (Gate B, the part no test can see)

Use with the **expected chart** block that `tools/gp-regress.py` prints for the CSV under test. Take the IRT screenshot
within a minute of the CSV's `ASOF`. Tick each line; transcribe what IRT shows into `irt.json` for the runner.

## Placement (scale)
- [ ] The King bar sits on the chart at **ES price + basis** (basis = chart last close − SCALEREF; ~0 when Skylit's ES1 is the charted contract). Read the tape column's SPX strike beside the King and confirm it is the Atlas King.
- [ ] The tape-column strikes read **row-for-row** against the Atlas SPXW ladder (7575, 7580, … same digits).
- [ ] The FLIP tick sits between the two strikes the runner names (`flip_between`) and its label reads `FLIP 0DTE`.
- [ ] The SPY KING line (lsKingTracker / FlexLevels) reads `SPY KING <strike>` and sits at strike × the SPY→ES ratio (audit `ratio.SPY`), i.e. beside the SPXW row ≈ SPY strike × 10.

## Labels (roles and patterns)
- [ ] Exactly one **G** on the board (or `G·C` / `G·F`), on the node the runner names; none anywhere else.
- [ ] **C** on the biggest node above price (unless it is the G·C node), **F** on the biggest below (unless G·F).
- [ ] **K** (or the chosen King name) on the King only.
- [ ] Pattern letters: **B** / **P** only on the named member of a stack, **R** / **RR** on the rug's yellow node; NO lowercase letters on members (0.45+) — the bracket marks the run.
- [ ] A **bracket** (thin bar at the base of the strip, family colour) spans each stack run the runner lists.
- [ ] **CW** / **PW** tags on the wall nodes the runner names, beyond the tip, pink.
- [ ] Rank badges 1–5 on the runner's `top5` (Show = Top 5); sub-threshold bars grey.
- [ ] Air-pocket bands ONLY where the runner lists them (none on the far-OTM tail).

## Colours
- [ ] +γ bars gold, −γ bars magenta, saturation growing with |%King| toward the King's colour; the King in the "Distinct" colour only if that option is chosen.
- [ ] Grey bars = below the Show filter, tape-column text grey to match; the King's tape-column text gold.
- [ ] Support stripe green / resistance stripe red at the base of every role-tagged node (below / above price).
- [ ] Regime swatch: magenta for −gamma, gold for +gamma, white for AT flip, grey for n/a; the line text red-tinted when `!CONFLICT`.
- [ ] Bracket colour = the family colour (gold pika / magenta barney). Air pocket grey, magenta-tinted when the run is net −γ.

## The regime chip
- [ ] Line 1 reads exactly the runner's `regime` string (sign | type | tactic, `!CONFLICT` when flagged). No `â€"`.
- [ ] Line 2 reads `PW <chart> (<spx>)   FLIP <chart> (<spx>)   CW <chart> (<spx>)` in green / white / red; `n/a` where a row is absent.
- [ ] No **STALE** badge (the CSV is < 4 min old); if there is one, stop — the run is not same-moment.

## What to write down
The run file gets: the screenshot file names, every unticked line with what was seen instead, and the operator's words.
