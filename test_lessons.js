// ============================================================================================
// test_lessons.js — THE LESSONS FILE MUST BE CURRENT BEFORE A BUILD SHIPS.
//
// Operator, 2026-08-30: "i need you to either use a document you already have or create a lessons
// learned document which you update every time there is a build. You will then also make it a part
// of the load gex procedure so future contexts know the lessons."
//
// ⚠⚠ WHY THIS IS A TEST AND NOT A CHECKLIST LINE. This project has run the experiment already.
// `latest-resume-note.md` went SEVEN builds stale, was fixed, then went FOUR more stale the same
// day — while `CHAT-HISTORY.md` stayed current the whole time for exactly one reason: a test went
// red when it wasn't. The rule is in PROJECT-CONSTANTS in the operator's own terms:
//
//     A rule enforced by a test is followed. A rule enforced by a checklist is followed until it
//     is busy.
//
// ⚠ AN EMPTY ENTRY IS ALLOWED AND A MISSING ONE IS NOT. A build that produced no lesson writes
// "no new lesson" against its version. That is a fact. A skipped entry is a silence nobody can
// tell apart from an oversight — which is precisely how ITEM 18 was lost for 24 versions.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

const ver=(src.match(/GPTS_VERSION\s*=\s*'([^']+)'/)||[])[1];
ok(!!ver, 'x0 GPTS_VERSION is readable', ver);

const L=(()=>{ try{ return fs.readFileSync('./session-state/LESSONS.md','utf8'); }catch(e){ return null; } })();
ok(L!==null, 'x1 session-state/LESSONS.md exists');

// ---- 1 · IT CARRIES THIS BUILD ---------------------------------------------------------------
ok(L && L.indexOf('v'+ver) > -1,
   'x2 LESSONS.md has an entry for the CURRENT panel version — every build writes one', ver);

// ---- 2 · THE LOG IS NEWEST-FIRST AND ACTUALLY A LOG -------------------------------------------
// A file that only ever gains an entry at the bottom is one a reader stops reaching. Newest first
// is stated in the file and checked here so it cannot quietly invert.
ok(L && /newest first/i.test(L), 'x3 ...and the log declares its order');
const entries = L ? (L.match(/^### v[0-9.]+/gm)||[]) : [];
ok(entries.length >= 2, 'x4 ...and holds more than one build', entries.length);
ok(entries.length && entries[0].indexOf('v'+ver) > -1,
   'x5 ...with THIS build at the top, not appended to the end', entries[0]);

// ---- 3 · NOTHING IS EVER DELETED ---------------------------------------------------------------
// ⚠ The whole disease this project keeps catching is knowledge vanishing by omission. A withdrawn
// number stays, NAMED as withdrawn, so a future context cannot re-derive it and think it is new.
ok(L && /NOTHING IS EVER DELETED/i.test(L),
   'x6 the never-delete rule is stated in the file itself');
ok(L && /WITHDRAWN/.test(L),
   'x7 ...and at least one withdrawn result is recorded rather than erased');

// ---- 4 · IT IS PART OF THE LOAD --------------------------------------------------------------
// The load reads canonicalFiles from .gex-config.json. A lessons file a fresh context never opens
// is a diary, not a control.
const cfg=(()=>{ try{ return JSON.parse(fs.readFileSync('./.gex-config.json','utf8')); }catch(e){ return null; } })();
ok(cfg!==null, 'x8 .gex-config.json parses');
// (2026-08-30) the load list is TIERED now — a flat list of 24 was being "read in full" including a
// 607KB changelog, so nothing was read carefully. LESSONS must be in TIER 0, not merely present:
// being listed somewhere a context never reaches is the same as not being listed.
const CF=(cfg&&cfg.canonicalFiles)||{};
const tier0=CF.tier0_readFirstInFull||[];
ok(tier0.indexOf('session-state/LESSONS.md') > -1,
   'x9 LESSONS.md is in TIER 0 of the load — read first, in full', tier0);
ok(tier0[0]==='session-state/LESSONS.md',
   'x9b ...and it is FIRST in tier 0, because it names the withdrawn results', tier0[0]);
ok(tier0.indexOf('session-state/OPEN-QUESTIONS.md') > -1,
   'x9c ...and OPEN-QUESTIONS rides with it, so nothing he already answered gets re-asked');
// ⚠ a self-check, because "files loaded" proves nothing was absorbed
const sc=(cfg&&cfg.loadSelfCheck&&cfg.loadSelfCheck.questions)||[];
ok(sc.length>=5, 'x9d the load carries a self-check the context must answer before working', sc.length);
ok(sc.every(function(x){ return x.q && x.where; }),
   'x9e ...and every question names WHERE its answer lives, so it can be verified not guessed');
ok(cfg && JSON.stringify(cfg.lessons||{}).indexOf('LESSONS.md') > -1,
   'x10 ...and the config carries an explicit lessons instruction for the load');

// ---- 5 · THE FAILURE PATTERNS TRAVELLED WITH IT ------------------------------------------------
// The patterns were in PROJECT-CONSTANTS and are the most reused thing in the file. They must be
// IN the lessons doc, not referenced from it — a pointer is one more hop a busy context skips.
ok(L && /Mislabeling/.test(L) && /Concluding "absent" from a shallow look/.test(L),
   'x11 the failure-pattern register lives IN this file, not behind a pointer');

// ---- 6 · THE OPEN QUESTIONS REGISTER ---------------------------------------------------------
// ⚠ A question with no home gets re-asked. He answered the wick-family definitions ONCE, in his own
// words, and that answer lived only in a resume note — the mechanism that lost ITEM 18 for 24
// versions. An agent that re-asks something he already answered is spending the one resource this
// project cannot regenerate.
const Q=(()=>{ try{ return fs.readFileSync('./session-state/OPEN-QUESTIONS.md','utf8'); }catch(e){ return null; } })();
ok(Q!==null, 'x12 session-state/OPEN-QUESTIONS.md exists');
ok(Q && /## ✅ ANSWERED/.test(Q),
   'x13 ...and answered questions are KEPT with their answers, not deleted');
ok(Q && /WHO can answer it/.test(Q) && /WHAT IT BLOCKS/.test(Q),
   'x14 ...and every entry must name who can answer it and what it blocks');
ok(Q && /NEVER GUESS AN ANSWER AND BUILD ON IT/.test(Q),
   'x15 ...and the no-guessing rule is stated in the file itself');

// ---- 7 · THE DERIVED LIST CANNOT DRIFT FROM THE TIERS -----------------------------------------
// ⚠ `projectFiles` is read by test_chat_history, test_em_band and test_futbars. Retiering removed it
// and broke all three — the ordinary shape of "changed a structure, forgot its consumers". It is now
// the exact union of the tiers, and this assertion is what stops it becoming a second list that
// silently disagrees with the first.
{
  const u=[].concat(CF.tier0_readFirstInFull||[], CF.tier1_readInFull||[],
                    Object.keys(CF.tier2_readPartially||{}), CF.tier3_referenceOnly||[]);
  const pf=CF.projectFiles||[];
  ok(JSON.stringify(pf)===JSON.stringify(u),
     'x16 canonicalFiles.projectFiles is exactly the union of the tiers, in order',
     {projectFiles:pf.length, union:u.length});
  // and every listed file must actually exist — two entries were listed for weeks and were not there
  const missing=pf.filter(function(f){ try{ fs.accessSync('./'+f); return false; }catch(e){ return true; } });
  ok(missing.length===0,
     'x17 ...and every file in the load list exists — a load that 404s teaches nothing', missing);
}

// ---- 8 · A CONFIG REBUILD MUST NOT DROP ITS SIBLINGS ------------------------------------------
// ⚠⚠ Retiering rebuilt `canonicalFiles` wholesale and silently deleted FOUR sibling keys —
// insiderFinance (which carries the one formula everything is recomputed from), skylitApi,
// analystPackages and loadNote. test_em_band caught insiderFinance; nothing would have caught the
// other three. Replacing a container is not the same as editing it, and the difference is invisible
// in a diff you wrote yourself.
['insiderFinance','skylitApi','analystPackages','loadNote'].forEach(function(k){
  ok(!!CF[k], 'x18 canonicalFiles.'+k+' survived — a rebuilt block keeps its siblings');
});
ok(CF.insiderFinance && /gamma \* openInterest \* 100 \* spot\^2/.test(CF.insiderFinance.formula||''),
   'x19 ...including the GEX formula every derived number is rebuilt from');

console.log('test_lessons: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
