# pending/ — agreed changes, coded and tested, NOT BUILT

A patch here is complete work the operator has agreed to but asked NOT to build yet ("don't create build yet, I am still
considering other changes"). It lives here, not in `plugin/*.cpp`, because the GEX build task on his machine compiles any
change to the plugin sources within two minutes — putting it in `plugin/` IS building it.

To ship a patch, when he says so:

    git apply pending/<file>.patch
    python3 tools/regress.py all --syntax        # must be ALL GREEN
    # then the normal build: CHANGELOG, resume note, snapshot/guards, commit, deploy, "close IRT"
    git rm pending/<file>.patch                  # in the same commit — a shipped patch does not stay here

If a patch no longer applies (other work touched the same lines), re-create it from the notes below rather than forcing it.

## 2026-09-19_gp073-ds016.patch — GP 0.73 + DS 0.16

1. **GP 0.73 — the tape strip's spacing, one rule for both rails.** The % starts exactly 2 character-spaces after the widest
   strike on that rail (`gpl::tapeCols`, `gpl::TAPE_GAP_CHARS = 2`), measured with `getTextWidth` at the strip's font. The
   strip width is measured from both books (`measureTape`) instead of `font x 7 + 8`, and shared by both rails. The % is
   left-justified on both strips now (the SPX right edge goes ragged — agreed). Operator: "add 1 space ... in the spx rail and
   remove 2 spaces in the spy rail" -> "make spy and spx consistent" -> "lets go with your recommendation" (N = 2).
   Mockup: `design/gp-tape-gap-mockup.html`. Gate B +5 (98).
2. **DS 0.16 — Day Stats opened at Top-left although the dialog said Top-center.** IRT can call parmsLoad before a restored
   instance's values exist; the font guard skipped readSettings and the constructor's Top-left drew until an Apply.
   `syncSettings()` (the same guard) now runs at the top of every draw() and in all three callbacks. New `test_plugin_settings.js`
   (4) in the daystats Gate A.
3. **GP 0.73 — labels centred ON their price.** IRT's rect text lands ~0.45 x font below its y (measured in 0.55); every label
   at a bar's price (the tape strip, the % beside a bar, role / pattern / level tags) drew 4-5 px under the bar. Hidden at Auto
   thickness, visible at Medium — operator 2026-09-19: "the node is slightly above, it is not aligned, both SPY and SPX".
   `textLJc` / `textRJc` shift by the measured offset; the depth pill re-centred on y. `test_plugin_settings.js` 5-8.
4. **GP 0.73 — the SPY strip carries the ES price.** "762 7717  −96%": the SPY strike, one space, the ES price the bar sits at
   (whole points) in a dimmer ink (C_DIMES — he picked the variant), then 2 characters to the %King. Each strip is now measured
   from its own book (`measureOne`, `tw[0]` main / `tw[1]` SPY rail) and the level lines, King lines and bands stop at each
   strip's own edge (`stripL` / `stripR`). Book = SPY single rail shows the ES price too; the SPX strip does not.
5. **DS 0.16 — MUD is the move in dollars followed by (points).** "$413 (8.3p)" / "~$1,197 (23.9p)": |2nd extreme − open|,
   the leg MUDt times. A row from the chart's own RTH open and 2nd extreme (`chartSession`, stamp-aware per DayModel 0.16 —
   on an end-stamped chart the 08:30 bar is the pre-open bar); E row from DAYSE's 2nd extreme vs DAYEXP's open. Money gets a
   thousands comma (HL RNG too). MUDt unchanged — it is HL GAP − BOP by his 09-12 definition; Friday: 3h30 − 2h21 = 1h09,
   the LOD-to-HOD time is HL GAP. Gate B 70 (+10). Mockup: design/ds-mud-spyes-mockup.html ("looks good").
   Not in this patch, same latent pattern: lsDayModel (font guard) and lsKingTracker (`dialogReady`) also read settings only in
   the callbacks. Their defaults happen to equal his settings, so it does not show; a non-default setting would.
