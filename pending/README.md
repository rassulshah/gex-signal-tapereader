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
   Not in this patch, same latent pattern: lsDayModel (font guard) and lsKingTracker (`dialogReady`) also read settings only in
   the callbacks. Their defaults happen to equal his settings, so it does not show; a non-default setting would.
