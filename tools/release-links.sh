#!/usr/bin/env bash
# (v11.52) PRINT THE TAMPERMONKEY BLOCK. Run this as the LAST step of every build and paste the output.
# Step 9 of the checklist has said "always send the link" since v11.9 and it kept getting missed, because
# a reminder competes with everything else at the end of a build. A step that PRODUCES the message cannot
# be forgotten the same way — if the block is missing, the step was not run.
#
# It marks which script actually CHANGED against origin/main. Sending a link for an unchanged script
# makes Tampermonkey offer "Reinstall" instead of "Update", which reads exactly like a failed push.
set -u
RAW="https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current"
git fetch origin main -q 2>/dev/null || true

ver(){ grep -m1 '@version' "$1" | tr -d '\r' | awk '{print $NF}'; }
changed(){ git diff --quiet origin/main -- "$1" 2>/dev/null && echo no || echo yes; }

TV=$(ver current/gex-signal-tapereader.user.js)
CV=$(ver current/gex-if-levels.user.js)
TC=$(changed current/gex-signal-tapereader.user.js)
CC=$(changed current/gex-if-levels.user.js)

echo "**Tampermonkey — update ONLY what changed:**"
echo
if [ "$TC" = yes ]; then
  echo "- **Tapereader v$TV** (changed) — $RAW/gex-signal-tapereader.user.js"
else
  echo "- Tapereader v$TV — UNCHANGED, do not reinstall"
fi
if [ "$CC" = yes ]; then
  echo "- **Companion v$CV** (changed) — $RAW/gex-if-levels.user.js"
else
  echo "- Companion v$CV — UNCHANGED, do not reinstall"
fi
echo
echo "Then RELOAD the Atlas tab — installing does not affect an already-open page."
[ "$TC" = no ] && [ "$CC" = no ] && echo "(neither script changed — this was a docs/tooling build, nothing to update)"
exit 0
