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

