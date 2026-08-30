// ============================================================================================
// test_recordcurrent.js — THE RECORD MUST BE AS NEW AS THE WORK.
//
// Operator, 2026-08-30: "how could you forget that you have to save that every build ?"
//
// ⚠⚠ THE HONEST ANSWER IS THAT THE EXISTING GUARDS COULD NOT CATCH IT. test_savedone and
// test_chat_history are VERSION-KEYED: they assert the current GPTS_VERSION appears in the file.
// On 2026-08-30 three separate bodies of work landed under an already-recorded v14.95 — the node
// source failures, the trinity study, and the whole LESSONS.md register — and BOTH tests stayed
// green with none of it written down. A version-keyed check is blind to every commit that does not
// bump the version, which is most of them.
//
// So this one keys on COMMITS, not versions: if a commit touched the panel, the tools, the tests or
// session-state, then CHAT-HISTORY.md must have been written in that same commit or a later one.
//
// ⚠ THE RECORD FILES ARE EXCLUDED FROM "WORK", or the test would be satisfied by its own subject.
// ============================================================================================
const { execSync } = require('child_process');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const git=(a)=>execSync('git '+a,{cwd:__dirname,encoding:'utf8',stdio:['ignore','pipe','ignore']}).trim();

// ⚠ A TEST THAT SILENTLY SKIPS IS THE FAILURE MODE THIS FILE EXISTS TO CLOSE. No git = FAIL.
let haveGit=false;
try{ git('rev-parse --git-dir'); haveGit=true; }catch(e){}
ok(haveGit, 'r0 this is a git repo and git is runnable — the check cannot be skipped into silence');

if(haveGit){
  const RECORD = ['session-state/CHAT-HISTORY.md'];
  const LESSON = ['session-state/LESSONS.md'];
  // everything whose change obliges an entry. The record files themselves are NOT work.
  const WORK = ['current', 'tools', 'changelog', 'session-state', 'data', 'design', 'skylit-docs'];

  const lastTouch = (paths, exclude) => {
    const ex = (exclude||[]).map(p=>`':(exclude)${p}'`).join(' ');
    try{ return git(`log -1 --format=%H -- ${paths.join(' ')} ${ex}`); }catch(e){ return ''; }
  };
  const workCommit   = lastTouch(WORK, RECORD.concat(LESSON));
  const recordCommit = lastTouch(RECORD, []);
  const lessonCommit = lastTouch(LESSON, []);

  const isAtLeastAsNew = (a, b) => {   // is `b` the same as `a`, or a descendant of it?
    if(!a || !b) return false;
    if(a===b) return true;
    try{ execSync(`git merge-base --is-ancestor ${a} ${b}`,{cwd:__dirname,stdio:'ignore'}); return true; }
    catch(e){ return false; }
  };

  const subj = (h)=>{ try{ return git(`log -1 --format=%s ${h}`).slice(0,60); }catch(e){ return h; } };

  ok(!!workCommit,   'r1 found the last commit that changed the project');
  ok(!!recordCommit, 'r2 found the last commit that changed CHAT-HISTORY.md');

  ok(isAtLeastAsNew(workCommit, recordCommit),
     'r3 CHAT-HISTORY.md is as new as the newest project change — no work is unrecorded',
     { work: subj(workCommit), record: subj(recordCommit) });

  ok(isAtLeastAsNew(workCommit, lessonCommit),
     'r4 ...and so is LESSONS.md', { work: subj(workCommit), lessons: subj(lessonCommit) });

  // ⚠ AND THE WORKING TREE MUST BE CLEAN OF UNCOMMITTED PROJECT CHANGES, or the check above is
  // measuring history while the actual change sits unstaged. This is how "it passed" and "it
  // shipped" come apart.
  let dirty='';
  try{ dirty = git('status --porcelain -- ' + WORK.join(' ')); }catch(e){}
  const dirtyLines = dirty ? dirty.split('\n').filter(Boolean) : [];
  ok(dirtyLines.length===0,
     'r5 no uncommitted project changes — the record cannot be verified against work that is not in',
     dirtyLines.slice(0,5));
}

console.log('test_recordcurrent: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
