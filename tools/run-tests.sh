#!/usr/bin/env bash
# Run the whole suite. `cp current/... v10.js` first — the harness reads v10.js, not current/.
# BUILD-CHECKLIST §2: full suite green, 6 baseline reds expected (pre-v13.8).
cd "$(dirname "$0")/.." || exit 1
# ⚠ v10.js IS GENERATED, NEVER COMMITTED. It was tracked despite being in .gitignore for its whole
# life, and the committed copy had drifted to v11.48 — 42 versions and 6,555 lines behind current/.
# Anyone running `node test_ladder.js` straight from a fresh clone was testing v11.48 and getting a
# green suite that said nothing about the shipped panel. A stale-but-green suite is the dangerous
# state; a missing file is loud. Untracked as of v14.56, and regenerated here on every run.
cp current/gex-signal-tapereader.user.js v10.js
pass=0; fail=0; failed=""
for f in test_*.js; do
  if node "$f" >/dev/null 2>&1; then pass=$((pass+1)); else fail=$((fail+1)); failed="$failed $f"; fi
done
echo "SUITE: $pass green / $fail red  (of $((pass+fail)) files)"
[ -n "$failed" ] && { echo "RED:"; for f in $failed; do echo "  $f"; done; }
exit 0
