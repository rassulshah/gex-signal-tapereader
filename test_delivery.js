// ============================================================================================
// test_delivery.js — THE TWO STANDING OPERATOR RULES, ENFORCED BY A RED BUILD
//
// ⚠ WHY THIS FILE EXISTS. On 2026-08-27 the operator had to ask for both of these, and one of them
// he had already mandated on 2026-08-15:
//
//     "you are supposed to just give me an install file."
//     "are you saving chat history like you are supposed to"
//
// The first failed because THREE DOCUMENTS DISAGREED and a context followed the wrong one:
//     tools/build-installer.py    printed "DELIVER THESE TWO FILES (primary)"
//     PROJECT-CONSTANTS.md        called the zip+applier pair "the primary delivery"
//     skills/gex/SKILL.md         said "ship ONE self-contained installer"
// He got three attachments. The banner was wrong, not the rule.
//
// Prose is exactly what lost ITEM 18 out of the resume note. So the fix is not a nicer sentence in
// one more place — it is an assertion that goes RED when any of the sources stops saying it, the
// same mechanism test_chat_history.js uses. A checklist line can be skipped; a red build cannot.
// ============================================================================================
const fs = require('fs');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); }
                          else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
const read = p => { try { return fs.readFileSync(p, 'utf8'); } catch (e) { return ''; } };

const BUILDER  = read('tools/build-installer.py');
const CONSTS   = read('session-state/PROJECT-CONSTANTS.md');
const SKILL    = read('skills/gex/SKILL.md');
const CONFIG   = read('.gex-config.json');
const RESUME   = read('session-state/latest-resume-note.md');
const CHECKLIST= read('tools/BUILD-CHECKLIST.md');

// ---- 1 · every source that a context reads must state the ONE-FILE rule ----------------------
ok(BUILDER.length && CONSTS.length && SKILL.length && CONFIG.length && RESUME.length && CHECKLIST.length,
   'd0 all six sources are present and readable');

const QUOTE = /just give me an install file/i;
ok(QUOTE.test(BUILDER),   'd1a the builder carries the operator quote, so nobody edits the banner back');
ok(QUOTE.test(CONSTS),    'd1b PROJECT-CONSTANTS carries it');
ok(QUOTE.test(SKILL),     'd1c the skill carries it');
ok(QUOTE.test(CONFIG),    'd1d .gex-config.json carries it — this is what a load reads FIRST');
ok(QUOTE.test(RESUME),    'd1e the resume note carries it');
ok(QUOTE.test(CHECKLIST), 'd1f the build checklist carries it');

// ---- 2 · the builder must not tell a context to send two files ------------------------------
// ⚠ This is the actual regression that happened. The banner outranked the rule because it was the
// thing printed at the moment of delivery, when nobody re-reads the skill.
ok(!/DELIVER THESE TWO FILES/i.test(BUILDER),
   'd2a the builder no longer prints a TWO FILE delivery banner');
ok(/DELIVER EXACTLY ONE FILE/.test(BUILDER),
   'd2b ...it prints a ONE FILE banner instead');
ok(/_DELIVER\s*=\s*'installv%s\.bat'/.test(BUILDER),
   'd2c ...and emits a dash-free, dot-free filename, because downloads strip both');
ok(/fallback only/i.test(BUILDER),
   'd2d the zip+applier pair is labelled FALLBACK where it is printed, not "primary"');

// ⚠ the pair must still BE BUILT — it is the proven escape hatch for the day the self-extractor
// fails on his machine again, which it did three ways in one day on 2026-08-25.
ok(/ZIPNAME\s*=\s*'gexdrop%s\.zip'/.test(BUILDER) && /BATNAME\s*=\s*'applygex%s\.bat'/.test(BUILDER),
   'd2e ...but it is still BUILT — one file is a DELIVERY rule, not a reason to delete the fallback');

// ---- 2b · THE TAMPERMONKEY LINKS ARE PART OF THE DELIVERY --------------------------------
// ⚠ Operator-mandated 2026-08-24 and restated 2026-08-27: "make sure you give me tampermonkey links
// when you give me install files." On 2026-08-27 installv1458.bat went out with NO links and he ran
// a build with a known unit bug for an hour, because Tampermonkey's default update check is ONCE A
// DAY and nothing prompted him to force it. The links used to print sixty lines below the filename,
// in a block easy to scroll past. They must print INSIDE the delivery block, welded to the filename.
{
  // ⚠ (v15.22) THE BOUND IS ABOUT THE BLOCK, NOT ABOUT ITS LENGTH. At {0,1400} this stopped
  // matching the moment the companion's raw URL and the comment explaining it were added — a change
  // that PUT MORE OF WHAT THIS TEST WANTS inside the block, and it read as the block disappearing.
  // A character budget standing in for "these lines are together" fails on exactly the edits it
  // should pass. Bounded by the block's own end marker instead.
  const blk = /==== DELIVER EXACTLY ONE FILE[\s\S]*?round-trip/.exec(BUILDER);
  ok(!!blk, 'd2f the delivery block is identifiable in the builder');
  ok(blk && /raw\.githubusercontent\.com/.test(blk[0]),
     'd2g the Tampermonkey raw URL prints INSIDE the delivery block, not sixty lines below it');
  ok(blk && /Companion/.test(blk[0]),
     'd2h ...and the companion is named too, so only what CHANGED gets relinked');
  ok(blk && /CLICK THE LINK/.test(blk[0]),
     'd2i ...and it says to CLICK the link — TM auto-update is once a day, the click is reliable');
  ok(blk && /Reinstall/.test(blk[0]),
     'd2j ...and that a Reinstall prompt means he already HAS the build, not that it failed');
}
ok(/tampermonkey links when you give me install files/i.test(CONFIG),
   'd2k the config a load reads FIRST carries the operator quote about the links');
ok(/CLICK THE LINK/i.test(CHECKLIST) && /once a day/i.test(CHECKLIST),
   'd2l the build checklist carries the click rule and why');

// ---- 3 · PROJECT-CONSTANTS must not still call the pair the primary delivery -----------------
ok(!/The primary delivery is now zip/.test(CONSTS),
   'd3 PROJECT-CONSTANTS no longer calls the zip+applier pair "the primary delivery"');
ok(/THE ONE-FILE RULE/.test(CONSTS),
   'd3b ...and states the rule under a heading a skim will hit');

// ---- 4 · the chat history rule, stated where a BUILD will see it -----------------------------
// He should never have to ask whether this was done. The test_chat_history.js version gate is the
// enforcement; these assertions make sure the INSTRUCTION to run it survives a doc rewrite.
const CH = /CHAT-HISTORY|chat.?history/i;
ok(CH.test(CHECKLIST) && /chat-history\.py/.test(CHECKLIST),
   'd4a the build checklist names the generator command');
ok(/every build/i.test(CHECKLIST) && CH.test(CHECKLIST),
   'd4b ...and says it runs on EVERY build');
ok(/everyBuild/.test(CONFIG) && /test_chat_history\.js FAILS THE BUILD/.test(CONFIG),
   'd4c the config states the rule AND names the red-build enforcement');
ok(/must never have to ask/i.test(CONFIG) || /must never have to ask/i.test(RESUME),
   'd4d ...and records that the operator should never have to ask whether it was done');
ok(/Run it LAST|run it LAST|AFTER the final exchange/i.test(RESUME + CHECKLIST),
   'd4e ...and that it runs LAST, or the tail of the session is missing from it');

// ---- 5 · the enforcement it points at must actually exist ------------------------------------
ok(fs.existsSync('test_chat_history.js'),
   'd5a the chat-history gate this rule cites is a real test file, not a claim');
ok(fs.existsSync('tools/chat-history.py'),
   'd5b ...and the generator it names actually exists');

// ---- (v15.22) BOTH LINKS, EVERY BUILD --------------------------------------------------------
// Operator, 2026-09-01: "why is there not companion link.. you should give me that because a change
// is required." The panel's raw link has printed since v14.3 and the companion's never did — so on
// every build that changed the companion he was told to update it and handed no way to. The rule
// was never about the panel; it was about being able to install what changed.
{
  const bi = fs.readFileSync('./tools/build-installer.py', 'utf8');
  const paste = bi.slice(bi.indexOf('PASTE THIS WITH THE INSTALL FILE'));
  ok(/current\/gex-signal-tapereader\.user\.js/.test(paste),
     'L1 the delivery block carries the PANEL\'s raw link');
  ok(/current\/gex-if-levels\.user\.js/.test(paste),
     'L2 ...and the COMPANION\'s, which it never did before v15.22');
  // and in the terminal block too, which is what a context reads while writing the message
  const term = bi.slice(bi.indexOf('PASTE THESE WITH IT, EVERY TIME'), bi.indexOf('PASTE THIS WITH THE INSTALL FILE'));
  ok(/current\/gex-if-levels\.user\.js/.test(term),
     'L3 ...in both places, so forgetting requires ignoring both');
}

// ---- (v15.22) THE SIZE GATE MEASURES THE ARTEFACT, NOT A PROXY FOR IT ------------------------
// It used to cap the RAW TREE at 6MB as a stand-in for the finished .bat, across gzip and base64.
// On 2026-09-01 it refused a 6.3MB tree whose real installer was 2.92MB / 37,490 lines — the same
// size as the one that had shipped an hour earlier and worked. A proxy that blocks a good build
// costs as much as one that passes a bad one.
{
  const bi = fs.readFileSync('./tools/build-installer.py', 'utf8');
  ok(/_BAT_BYTES_CAP/.test(bi) && /_BAT_LINES_CAP/.test(bi),
     'Z1 the hard gate is expressed in the .bat\'s own bytes and lines');
  ok(/_bat_bytes = len\(out\.encode\('ascii'\)\)/.test(bi),
     'Z2 ...measured on the finished text, not estimated from the tree');
  ok(/_PAYLOAD_ADVISORY/.test(bi) && !/_PAYLOAD_CAP/.test(bi),
     'Z3 ...and the raw-tree number is an ADVISORY, which cannot refuse a build');
  // the advisory must still PRINT, or a growing tree becomes invisible
  ok(/PAYLOAD ADVISORY/.test(bi), 'Z4 ...but it still reports, so growth is not silent');
  // ⚠ Z5 SURVIVED ITS FIRST MUTATION: it looked for the `raise` line, which stays exactly where it
  // is when the CONDITION above it is disabled. A refusal nothing can reach is not a refusal, and
  // greping for the throw cannot tell the two apart. Bind to the condition and the throw TOGETHER.
  ok(/if _bat_bytes > _BAT_BYTES_CAP or _bat_lines > _BAT_LINES_CAP:[\s\S]{0,600}?raise SystemExit\('refusing to build an installer that will hang/.test(bi),
     'Z5 and the artefact gate does still refuse — the condition reaches the throw');
}

// ---- (v15.32) THE SAVE CONFIRMATION SHIPS WITH EVERY BUILD ----------------------------------
// Operator-mandated 2026-08-30 and restated 2026-09-01: "i dont see the tamper monkey links or save
// confirmations which you are supposed to give me everytime there is a build telling me the files
// that were saved (eg chat history, lessons learned etc.)"
// ⚠⚠ THE FAILURE WAS MINE, NOT THE BUILDER'S. The Tampermonkey block has printed since v14.3 and I
// stopped pasting it; the record files were never listed at all. A rule that lives only in the
// assistant's head has no mechanism — the same lesson test_lessons and test_chat_history encode.
// So the builder now PRINTS the confirmation, and this pins that it does.
{
  const bi = fs.readFileSync('./tools/build-installer.py', 'utf8');
  ok(/SAVE CONFIRMATION — PASTE THIS TOO/.test(bi),
     'S1 the builder prints a save confirmation block');
  ok(/git', 'show', '--stat'/.test(bi),
     'S2 ...read from the COMMIT, not written from memory — a hand-written list drops what the writer forgets');
  ['CHAT-HISTORY.md', 'LESSONS.md', 'CHANGELOG.md', 'latest-resume-note.md'].forEach(f => {
    ok(bi.indexOf(f) >= 0, 'S3·' + f + ' ...and checks off ' + f);
  });
  // ⚠ BOUND TO THE EXPRESSION, NOT THE WORD. The first cut greped for "MISSING", which also appears
  // in the comment explaining it — so deleting the conditional left the word behind and the
  // assertion green. This project's oldest recurring fault, and I wrote it again.
  ok(/'saved  ' if _p in _files else 'MISSING'/.test(bi),
     'S4 ...marking a mandated file MISSING rather than omitting it silently');
  ok(/COULD NOT READ THE COMMIT/.test(bi),
     'S5 ...and saying so if the commit cannot be read, rather than claiming a save that did not happen');
}

// ===== (v15.64, the install) THE COURIER'S CEILING — `more` stops at 65,535 lines (L-S) ==================
{
  const bi=require('fs').readFileSync('tools/build-installer.py','utf8');
  const hdr=(bi.match(/HDR = \"\"\"[\s\S]*?\"\"\"/)||[''])[0];
  ok(hdr.length>0 && /\(for \/f "usebackq skip=%HDRLINES% delims=" %%L in \("%SELF%"\) do echo\(%%L\)>"%TEMP%/.test(hdr),
     'M1 the payload is copied out with a for /f skip — no line limit');
  ok(!/^\s*more\s+\+/m.test(hdr),
     'M2 …and no `more +N` command remains in the header (it stops at 65,535 lines — v15.64 was the first build past it)');
  ok(/refuses it coming back|`more \+N` extraction is back/.test(bi) && /_re\.match\(r'\\s\*more\\s\+\\\+', l\)/.test(bi),
     'M3 the builder asserts against a `more` command in the rendered header');
  ok(/if l\.strip\(\) else 'rem'/.test(bi), 'M4 blank header lines become `rem`, so the skip count cannot depend on how cmd counts empty lines');
  ok(/fullmatch\(r'\[A-Za-z0-9\+\/=\]\{76\}', l\)/.test(bi), 'M5 …and a header line that looks like a base64 line fails the build');
}

console.log('test_delivery: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
