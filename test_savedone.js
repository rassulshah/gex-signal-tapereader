// ============================================================================================
// test_savedone.js — THE HANDOFF CHAIN MUST BE CURRENT BEFORE A BUILD SHIPS.
//
// Operator, 2026-08-28: "after you give me a build, i want a confirmation something like a
// checkmark save done for future context. this tells me that you have updated the chat history and
// any relevant files that a future context would need to proceed if this context was closed."
//
// ⚠⚠ WHY THIS EXISTS AND WHY IT IS A TEST, NOT A CHECKLIST LINE. `latest-resume-note.md` went SEVEN
// builds stale on 2026-08-28 (v14.59 while the panel was v14.66) with zero mentions of the feature
// being built — a fresh context would have rebuilt the model from scratch. It was fixed, and then
// went FOUR builds stale again the same day. Meanwhile CHAT-HISTORY.md stayed current the whole
// time, for exactly one reason: test_chat_history.js goes RED when it is not.
//
// **A rule enforced by a test is followed. A rule enforced by a checklist is followed until it is
// busy.** That was recorded as landmine L-R and then not acted on for four builds. This is the fix.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

const ver=(src.match(/GPTS_VERSION\s*=\s*'([^']+)'/)||[])[1];
ok(!!ver, 'v0 GPTS_VERSION is readable', ver);

const read=p=>{ try{ return fs.readFileSync(p,'utf8'); }catch(e){ return null; } };

// ---- 1 · the two files that carry the VERSION must carry THIS version -----------------------
const hist=read('./session-state/CHAT-HISTORY.md');
ok(hist!==null, 'v1 CHAT-HISTORY.md exists');
ok(hist && hist.indexOf('v'+ver)>-1,
   'v2 CHAT-HISTORY.md is stamped with the CURRENT version', ver);

const note=read('./session-state/latest-resume-note.md');
ok(note!==null, 'v3 latest-resume-note.md exists');
// ⚠ THE ASSERTION THAT WAS MISSING. The resume note is the FIRST file `load gex` reads, in full.
ok(note && new RegExp('panel v'+ver.replace('.','\\.')).test(note),
   'v4 the resume note declares THIS panel version — it went 7 builds stale without this',
   note ? (note.match(/panel v[\d.]+/)||['(none)'])[0] : null);

const chg=read('./changelog/CHANGELOG.md');
ok(chg && chg.indexOf('## v'+ver)>-1,
   'v5 CHANGELOG.md has an entry for this version', ver);

// ---- 2 · the resume note must actually be a briefing, not a stub -----------------------------
{
  const must=[
    ['the standing requirement, verbatim', /I am a trader and need to know/],
    ['what to do next, in order',          /WHAT TO DO NEXT, IN ORDER/i],
    ['how the operator works',             /ONE AT A TIME/],
    ['the one-file delivery rule',         /just give me an install file/],
    ['a pointer to FINDINGS',              /FINDINGS/],
  ];
  must.forEach(([what,re])=>ok(note && re.test(note), 'v6 the note carries '+what));
}

// ---- 3 · the ledgers a future context needs must exist ---------------------------------------
[['LOCKED-ITEMS','./session-state/LOCKED-ITEMS.md'],
 ['PROJECT-CONSTANTS','./session-state/PROJECT-CONSTANTS.md'],
 ['DATA-ARCHITECTURE','./design/DATA-ARCHITECTURE.md'],
 ['FINDINGS','./skylit-docs/FINDINGS.md'],
 ['.gex-config.json','./.gex-config.json']].forEach(([n,p])=>
   ok(read(p)!==null, 'v7 '+n+' is present for the next load'));

// ---- 4 · the convention itself must be written down where a context will read it -------------
{
  const cfg=read('./.gex-config.json')||'';
  const consts=read('./session-state/PROJECT-CONSTANTS.md')||'';
  const chk=read('./tools/BUILD-CHECKLIST.md')||'';
  ok(/save done/i.test(cfg), 'v8 the config states the SAVE DONE confirmation the operator asked for');
  ok(/save done/i.test(consts), 'v9 ...and PROJECT-CONSTANTS carries it as a standing rule');
  ok(/save done/i.test(chk), 'v10 ...and the build checklist requires it');
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
