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

## (empty) — 2026-09-19_gp073-ds016.patch was built as GP 0.73 + DS 0.16 on 2026-09-19 and retired.
