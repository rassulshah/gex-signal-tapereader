// ============================================================================================
// test_installer_note.js — THE INSTALLER'S "UPDATE THE COMPANION" LINE MUST BE TRUE.
//
// Operator, 2026-08-28: "the companion has a reinstall instead of update."
//
// ⚠⚠ WHY THIS IS A TEST. Tampermonkey offers "Reinstall" when the installed @version already
// matches the URL, and "Update" when it does not. He read that correctly off his own screen while
// my delivery note said "CHANGED, update it too" — for the EIGHTH build running. The companion last
// moved at v14.72 (1.15 -> 1.16); every release since was byte-identical.
//
// THE CAUSE was not the comparison, it was the BASELINE. `changed()` diffs against `origin/main`,
// and this clone's ref was pinned at v14.71 because the cloud cannot PUSH — so nothing here ever
// advanced it, while his machine pushed every installer. `git fetch` was never attempted because
// "the cloud has no GitHub access" was carried as ONE fact when it is TWO: fetch works, push 403s.
// Same shape as the `file://` polling error — one true observation generalised past its evidence.
//
// A wrong reinstall instruction is not cosmetic: he reinstalls the companion mid-session, the
// courier restarts, and the IF chain it feeds goes briefly absent — during RTH, on his face.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./tools/build-installer.py','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

// ---- 1 · the baseline is refreshed BEFORE it is compared ------------------------------------
const iFetch=src.indexOf("'fetch', 'origin', 'main'");
const iDiff =src.indexOf("'diff', '--quiet', 'origin/main'");
ok(iFetch>-1, 'n1 the builder FETCHES origin/main — the fix for the stale baseline');
ok(iDiff>-1,  'n2 ...and still diffs the companion against it');
ok(iFetch>-1 && iDiff>-1 && iFetch<iDiff,
   'n3 ...and the fetch comes FIRST — a diff against an unrefreshed ref is the whole defect',
   [iFetch, iDiff]);

// ---- 2 · a failed fetch must NOT be reported as "CHANGED" -----------------------------------
// ⚠ The old code's `except: return True` defaulted to "changed", which is what printed eight false
// reinstall instructions. An unverifiable answer is stated as unverifiable.
ok(/FETCH_OK\s*=\s*\(?\s*_f\.returncode\s*==\s*0/.test(src),
   'n4 the fetch RESULT is recorded, not assumed');
// ⚠ THIS ASSERTION WAS FAKE ON ITS FIRST WRITING and a mutation caught it: it grepped the WHOLE
// file for "UNVERIFIED", which still matched the string sitting in _compan_verdict() even after the
// line that SETS the marker was deleted. Bind to the `if not FETCH_OK:` branch itself.
const notOk=src.slice(src.indexOf('if not FETCH_OK:'), src.indexOf('elif not changed(COMPAN)'));
ok(notOk.length>0 && /COMPAN_NOTE\s*\+=.*UNVERIFIED/.test(notOk),
   'n5 the FAILED-FETCH BRANCH ITSELF sets the UNVERIFIED marker — never a confident CHANGED',
   notOk.replace(/\s+/g,' ').slice(0,90));
const verdict=(src.match(/def _compan_verdict[\s\S]*?\n\n/)||[''])[0];
ok(/UNVERIFIED/.test(verdict) && verdict.indexOf('UNVERIFIED')<verdict.indexOf("'UNCHANGED' in"),
   'n6 ...and UNVERIFIED is checked BEFORE unchanged/changed, so it cannot be overwritten', !!verdict);

// ---- 3 · both delivery lines go through the same verdict ------------------------------------
// There are TWO places the note is printed (console summary and the markdown block). They drifted
// apart once already; one function now feeds both.
const calls=(src.match(/_compan_verdict\(/g)||[]).length;
ok(calls>=3, 'n7 both printed lines call the SAME verdict helper (def + 2 uses)', calls);
ok(!/UNCHANGED, do not reinstall' if 'UNCHANGED' in COMPAN_NOTE/.test(src),
   'n8 ...and the old inline ternaries are gone, not merely shadowed');

// ---- 4 · the fetch cannot block a build forever ----------------------------------------------
ok(/timeout\s*=\s*\d+/.test(src.slice(iFetch-200, iFetch+300)),
   'n9 the fetch is bounded by a timeout — a build must not hang on a network stall');

// ---- 5 · the PANEL line is measured too, not hardcoded --------------------------------------
// ⚠ It read "(changed)" as a literal for the life of this builder. Usually true, and false on any
// build that only touches tools/ or tests — which is exactly what this build is.
// ⚠ FAKE ON FIRST WRITING, caught by mutation: the regex matched only the CONSOLE line's exact
// spacing, so re-hardcoding the MARKDOWN line ("**Tapereader v%s** (changed)") slipped straight
// through. Bind to every printed Tapereader line instead of one hand-typed shape.
const panelLines=src.split('\n').filter(l=>/print\(.*Tapereader v%s/.test(l));
ok(panelLines.length===2, 'p0 exactly two lines print the Tapereader link', panelLines.length);
ok(panelLines.every(l=>/_panel_verdict\(\)/.test(l) && !/\(changed\)/.test(l)),
   'p1 EVERY printed panel line asks the verdict; none hardcodes "(changed)"',
   panelLines.map(l=>l.trim().slice(0,60)));
ok(/def _panel_verdict/.test(src) && (src.match(/_panel_verdict\(\)/g)||[]).length>=3,
   'p2 ...both printed panel lines call a measured verdict (def + 2 uses)',
   (src.match(/_panel_verdict\(\)/g)||[]).length);
const pv=(src.match(/def _panel_verdict[\s\S]*?\n\n/)||[''])[0];
ok(/changed\(SCRIPT\)/.test(pv), 'p3 ...and it diffs the PANEL, not the companion', !!pv);
ok(/FETCH_OK/.test(pv) && pv.indexOf('FETCH_OK')<pv.indexOf('changed(SCRIPT)'),
   'p4 ...and an unfetched baseline is declared, not guessed at', !!pv);

console.log('test_installer_note: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
