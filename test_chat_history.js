// (2026-08-27) THE CHAT HISTORY MUST BE REGENERATED ON EVERY BUILD, AND THIS FAILS THE BUILD IF IT WAS NOT.
//
// Operator-mandated: "everytime you make me a build i want this file updated with the chat history of
// the context ... this should ensure you are upto date." He then asked the question that produced this
// file: "i also want to make sure that the next context ... doesn't forget to make updates to the chat
// history ... so i dont have to tell future context sessions to save chat history every time."
//
// ⚠ A CHECKLIST ITEM IS NOT A PRECAUTION. `tools/BUILD-CHECKLIST.md` and the skill both say to run the
// generator, and both are just prose that a hurried context can skip — which is exactly how ITEM 18
// fell out of the resume note and cost a whole session. The only thing in this project that has ever
// reliably stopped a recurrence is a TEST THAT GOES RED. So:
//
//   THE VERSION STAMPED IN CHAT-HISTORY.md MUST EQUAL GPTS_VERSION.
//
// Bump the version without regenerating the history and this test fails, the suite goes red, and the
// build stops. There is no way to ship a build with a stale history and a green suite.
//
// It also pins the WIRING — config, skill, checklist — so a later edit cannot quietly unhook the file
// from the load procedure and leave the generator orphaned.
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const read=(p)=>{ try{ return fs.readFileSync(p,'utf8'); }catch(e){ return null; } };

const HIST='./session-state/CHAT-HISTORY.md';
const h=read(HIST);
ok(h!==null, 'session-state/CHAT-HISTORY.md exists');
ok(read('./tools/chat-history.py')!==null, 'tools/chat-history.py exists (it is GENERATED, never hand-written)');

if(h){
  // ---- the two tiers -------------------------------------------------------------------------
  ok(h.indexOf('<!-- CURRENT-CONTEXT -->')>=0, 'it carries a CURRENT-CONTEXT marker');
  ok(h.indexOf('<!-- EARLIER-CONTEXTS -->')>=0, 'it carries an EARLIER-CONTEXTS marker');
  const cur=h.split('<!-- CURRENT-CONTEXT -->')[1]||'';
  const curBlock=cur.split('<!-- EARLIER-CONTEXTS -->')[0]||'';

  // ---- ⚠⚠ THE GATE: version stamp must match the shipping version --------------------------
  // This is the whole point of the file. Everything else here is hygiene.
  const src=read('./v10.js')||read('./current/gex-signal-tapereader.user.js')||'';
  const vm=src.match(/GPTS_VERSION\s*=\s*'([^']+)'/);
  const ver=vm?vm[1]:null;
  ok(ver!==null, 'GPTS_VERSION is readable from the panel source', ver);
  if(ver){
    const stamped=curBlock.match(/^##\s+\S+\s+·\s+v([0-9][\w.\-]*)/m);
    ok(stamped!==null, 'the CURRENT-CONTEXT entry carries a version stamp', stamped&&stamped[1]);
    ok(stamped!==null && stamped[1]===ver,
       'the chat history was regenerated for THIS version — run `python3 tools/chat-history.py` '+
       '(stamped '+(stamped?stamped[1]:'none')+', shipping '+ver+')');
  }

  // ---- the operator's own words are present and verbatim ------------------------------------
  const prompts=(curBlock.match(/^\*\*OPERATOR:\*\*/gm)||[]).length;
  ok(prompts>0, 'the current entry records at least one operator prompt', prompts);
  ok(curBlock.indexOf('> ')>=0, 'operator prompts are quoted verbatim, not paraphrased');

  // ---- the three hand-written sections must be FILLED, not left as placeholders --------------
  // The generator emits italic placeholders. Shipping them means nobody wrote the judgment down,
  // and judgment is the one part a transcript cannot supply.
  ['### DECISIONS','### SHIPPED','### OPEN AT CLOSE'].forEach(function(sec){
    ok(curBlock.indexOf(sec)>=0, 'the current entry has a '+sec.replace('### ','')+' section');
  });
  // ⚠⚠ (v15.10) SCOPED TO THE SECTION BODY, NOT THE WHOLE ENTRY. These three searched the entire
  // current block, and the block CONTAINS THE TRANSCRIPT — so the moment a reply quoted the
  // placeholder text ("...still has its three summary sections as literal placeholders"), the guard
  // fired on a file that was correctly filled in. That is the comment-contains-the-token family
  // that has produced nine false results here, arriving one level up: the RECORD quotes the thing
  // the guard looks for. Read the slice from the heading to the next heading instead.
  var sectionBody=function(head){
    var i=curBlock.indexOf(head); if(i<0) return '';
    var rest=curBlock.slice(i+head.length);
    var j=rest.indexOf('\n### ');
    return j<0?rest:rest.slice(0,j);
  };
  ok(sectionBody('### DECISIONS').indexOf('_Fill in before committing')<0, 'DECISIONS was filled in, not left as the placeholder');
  ok(sectionBody('### SHIPPED').indexOf('_Version + what actually changed')<0, 'SHIPPED was filled in, not left as the placeholder');
  ok(sectionBody('### OPEN AT CLOSE').indexOf('_What the next context must pick up')<0, 'OPEN AT CLOSE was filled in, not left as the placeholder');
}

// ---- THE WIRING IS PINNED TOO ------------------------------------------------------------------
// A generator nobody reads is worse than no generator: it costs effort every build and pays nothing.
// These three assertions make it impossible to silently unhook the file from the load procedure.
// ⚠ PARSE THE JSON, DO NOT SUBSTRING IT. The first cut of this test asserted
// `cfg.indexOf('session-state/CHAT-HISTORY.md')>=0` — which passes even when the file is deleted
// from `projectFiles`, because the `chatHistory` block below also contains that same path. Caught by
// mutation-testing the assertion, which is precisely why BUILD-CHECKLIST §2 demands it: an assertion
// that cannot fail is worse than no assertion, and this one could not fail.
let cfg=null; try{ cfg=JSON.parse(read('./.gex-config.json')||'{}'); }catch(e){ cfg=null; }
ok(cfg!==null, '.gex-config.json parses');
if(cfg){
  const pf=(cfg.canonicalFiles&&cfg.canonicalFiles.projectFiles)||[];
  ok(pf.indexOf('session-state/CHAT-HISTORY.md')>=0,
     '.gex-config.json lists it in canonicalFiles.projectFiles', pf.length);
  ok(!!cfg.chatHistory && typeof cfg.chatHistory==='object',
     '.gex-config.json carries the chatHistory contract block');
  ok(!!cfg.chatHistory && /CHAT-HISTORY\.md/.test(String(cfg.chatHistory.onLoad||'')),
     'the chatHistory block says to read it ON LOAD');
  ok(!!cfg.chatHistory && /chat-history\.py/.test(String(cfg.chatHistory.onBuild||'')),
     'the chatHistory block says to regenerate it ON BUILD');
  ok(/CHAT-HISTORY\.md/.test(String(cfg.loadInstruction||'')),
     'the loadInstruction itself names it, so a load cannot skip it');
}

const skill=read('./skills/gex/SKILL.md')||'';
ok(skill.indexOf('CHAT-HISTORY.md')>=0, 'the gex skill names it in the LOAD procedure');
ok(skill.indexOf('chat-history.py')>=0, 'the gex skill names the generator in the SAVE procedure');

const chk=read('./tools/BUILD-CHECKLIST.md')||'';
ok(chk.indexOf('chat-history.py')>=0, 'BUILD-CHECKLIST.md requires regenerating it every build');

console.log('\n'+pass+' pass / '+fail+' fail');
process.exit(fail?1:0);
