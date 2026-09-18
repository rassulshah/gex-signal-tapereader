# lsDayStats — the eyes-on checklist (Gate B, the part no test can see)

Use with the **expected picture** `tools/gp-regress.py --indicator daystats` prints (`A_cells`, `E_cells` — the twelve
cells exactly as `DayStatsLogic.h` formats them). Screenshot within a minute of `ASOF`; transcribe into `irt.json`
(`{"A":[...12], "E":[...12]}`, null for a cell you cannot read).

## The strip
- [ ] Row A reads the runner's `A_cells` cell for cell: `1ST <HOD|LOD> <clock> <price on the chart's contract>` · TOOK · BOP · WICK · W.End · W% · MUD · MUDt · `2ND ...` · HL Gap · `$<usd>  <pts>p`.
- [ ] Row E reads `E_cells`: every expectation carries `~`; the 2ND cell carries the ladder (`39% last hr`) when the CONDE row has it; `=` only on a read-in basis (older CSVs).
- [ ] **A and E differ in the 1ST clock and TOOK** once the READ is in (16.36) — identical values are the bug he caught.
- [ ] Before `hodLod` is ready there is no A row (a blank row, not a copy of E).

## Colours
- [ ] A row: 1ST green when HOD first (red when LOD first); 2ND the opposite; MUD red after a first HOD, green after a first LOD.
- [ ] No **STALE** badge.

## What to write down
Screenshot file names, every unticked line with what was seen instead, the operator's words.

## DS 0.11 + panel 16.45 — the READ line's second half
- [ ] With the 1ST called IN, the title line reads "HOD IN 96%  ·  LOD after HH:MMam 80%" before that clock; the clock = 08:30 + CONDE field 9 minutes; then "after <p50> 50%", then "39% last hr", "29% last 30", "any minute"; after the close "LOD IN <A row's 2ND clock>".
- [ ] NOT IN / HOLD calls show only the first half, as before.

## DS 0.12 + panel 16.46 — the second extreme's read
- [ ] Title line: "HOD IN 96%  ·  LOD IN <p>%" once the READ has a call; p from the CSV's READ2 row; "· if not, ~HH:MM (50%)" only while p < 50 (HH:MM = the export's ASOF + the arrival minutes); after the close "LOD IN <A row's 2ND clock>".
- [ ] p rises through the morning as price leaves the running extreme; it drops back when price revisits it (as at 10:00 on 2026-09-17).
- [ ] READ2's src reads `baked` until the first nightly refit lands (then `nightly`); the runner's expected picture prints `second_read`.


## 0.13 (2026-09-17) — the panel's place, both clocks after the close
- [ ] Corner = Top-center: the block sits on the pane's midline, its left edge never left of the pane (a narrow pane overflows on the RIGHT).
- [ ] After the close: "HOD IN 8:33am   ·   LOD IN 9:09am" — both halves clocks, green.
- [ ] LOD-first day: the line starts with LOD (his rule: "if the lod occurs, it should be before the HOD and vice versa").
- [ ] The READ line with "· if not, ~10:09am (50%)" is not clipped at 300 px.
- [ ] E row HL RNG is model-sized (09-17 re-derived: ~$2445 ~48.9p, not ~$8625 ~172.5p) — the 16.47 EM scale fix.

## 0.14 (2026-09-17) — one model on both halves
- [ ] 8:45–9:03: the left half is the HLTAB cell ("HOD 40%"), no right half; from 9:06 both halves read "X IN p%" from the model.
- [ ] The left half carries "· if not, ~clock (50%)" while its p < 50, exactly like the right half.
- [ ] Tone: green at ≥ 70, amber at ≤ 30, white between — from the model's p, not the old IN / NOT IN call.
- [ ] E row 1ST / TOOK are the stage's median (a top-tercile open reads ~8:51am · ~21m), never the A row's clock (16.48: no +orclock).
- [ ] After the close both halves are clocks, green.
