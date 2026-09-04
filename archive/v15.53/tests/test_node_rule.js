// (v11.45) THE NODE RULE — user-mandated 2026-08-22.
//
//   "any time a trade occurs, it must be off a node. The levels give context but the trade is off a
//    node, preferably a pullback node."
//
// And the bug this exposed: secExec tested `pb.entry`, a property pbEntryPick has NEVER returned — it
// returns `level`. So EXECUTE could not arm AT ALL. A valid node-based entry sat in the object while
// the face showed "no setup", from v11.26 until now, because reading an absent property is not an error.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

// ---- the shape pbEntryPick actually returns ----
{
  const fn=(function(){ const i=src.indexOf('function pbEntryPick'); const j=src.indexOf('\n}\n', i); return src.slice(i,j); })();
  ok(/out=\{ ok:false, level:null/.test(fn),'pbEntryPick returns `level`');
  ok(!/\bout\.entry\b/.test(fn),'and never an `entry` — the field secExec used to read');
}
// ---- secExec now keys off the node ----
{
  const fn=(function(){ const i=src.indexOf('function secExec'); const j=src.indexOf('\n}\n', i); return src.slice(i,j); })();
  // strip comments first — the surviving `pb.entry` is in the note explaining the bug, which should stay
  const code=fn.split('\n').filter(l=>!/^\s*\/\//.test(l)).join('\n');
  ok(!/pb\.entry/.test(code),'secExec no longer READS the nonexistent entry field');
  ok(/pb\.entry/.test(fn),'though the comment recording why it was wrong is kept');
  ok(/hasNode=!!\(pb && pb\.ok && pb\.level!=null\)/.test(fn),'it arms off a NODE');
  ok(/NO NODE — NO TRADE/.test(fn),'and refuses outright when there is no node');
  ok(/which is a level, not a node — context only/.test(fn),
     'naming the level explicitly as context rather than a setup');
  ok(/isPB=!!\(pb && \/pb\/i\.test\(pb\.rule\|\|''\)\)/.test(fn),'a pullback node is identified as such');
  ok(/pullback node/.test(fn),'and labelled on the face when it is one');
  ok(/AGAINST THE CALL/.test(fn),'a node setting up against the SMA is refused');
  ok(/R:R UNDER FLOOR/.test(fn),'the reward-to-risk floor still applies');
  ok(/FADE BLOCKED/.test(fn),'and the regime can still veto a fade');
  // the stop must come off the node's own zone, not an arbitrary distance
  ok(/pb\.zoneLo!=null\?pb\.zoneLo:entry/.test(fn),'the stop sits beyond the NODE ZONE, not a fixed pad');
  ok(/pb\.nextStop/.test(fn),'and the target is the next structural stop');
}
// ---- the step bar agrees ----
{
  const fn=(function(){ const i=src.indexOf('function stepState'); const j=src.indexOf('\n}\n', i); return src.slice(i,j); })();
  // (v13.0) FOUR steps now — EXECUTE is index 3, not 4. The RULE is unchanged: a trade is off a
  // NODE, and `entry` never existed on the object.
  ok(/st\[3\]=!!\(pb && pb\.ok && pb\.level!=null\)/.test(fn),'the last step lights on a node, not on the phantom field');
  ok(!/pb\.entry/.test(fn),'and never on pb.entry, which is the field that did not exist');
  ok(/waiting on a node/.test(fn),'and the waiting line says what it is waiting for');
}
// ---- the rule is recorded where it will be read ----
ok(/THE NODE RULE \(user-mandated 2026-08-22\)/.test(src),'the rule is written at the code it governs');
ok(/A TRADE IS OFF A NODE/.test(src),'stated plainly');
ok(/Levels give CONTEXT/.test(src),'with the distinction that levels are context');

// ---- hovers: the audit gap was labels tipped, values not ----
{
  // The v11.45 audit found 114 fields tipped and 41 not, and fixed it with cell(lab,val,tip).
  // The FRAME rewrite (v11.66-v11.86) replaced that helper. The GAP is what must stay closed, not
  // the helper's name -- so assert that no FRAME cell renders a value outside its tipped element.
  const emk=src.split('<span class="g3emk').slice(1)
    .filter(x=>!x.startsWith("\"','")&&!x.startsWith('" style="visibility:hidden"'));  // (v14.6) skip the spc() literals
  ok(emk.length>=2,'FRAME band cells are rendered as tipped spans', emk.length);
  ok(emk.every(x=>x.startsWith("\"'+g3tip(")||x.startsWith("'+(AH?' g3ahdim':'')+'\"'+g3tip(")),
     'each wrapping label AND value in ONE tipped element, which is what the v11.45 hover audit fixed',
     emk.map(x=>x.slice(0,24)));
  ok(/class="g3r'\+far\+band\+'"'\+g3tip/.test(src),'ladder ROWS carry the tip, so the number is hoverable');
  ok(/class="g3prow"'\+g3tip/.test(src),'the price row is explained');
  ok(/class="g3vd2"'\+g3tip/.test(src),'and the verdict block');
}
console.log('\n'+pass+' pass / '+fail+' fail');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
