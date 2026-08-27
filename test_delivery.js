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
  const blk = /==== DELIVER EXACTLY ONE FILE[\s\S]{0,1400}?round-trip/.exec(BUILDER);
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

console.log('test_delivery: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
