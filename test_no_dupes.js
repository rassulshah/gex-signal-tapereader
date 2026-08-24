// (v11.40) NO DUPLICATE TOP-LEVEL FUNCTION NAMES.
//
// Two function declarations with one name in one scope: the LATER one wins, silently, and every caller
// of the earlier one starts receiving a different shape with no error anywhere. This has now happened
// three times in this project. The third was mine: v11.31 added `ifNum(x)` as a display formatter while
// `ifNum(txt,label)` already existed as the manual-InsiderFinance parser — so all five parser callers
// were handed the formatter, passed it a string, and got NaN. The manual-entry panel was quietly broken
// for nine releases and nothing threw.
//
// Two collisions predate this and are inert only because the shadowed copy has no live callers:
//   trendBadgeHtml(sym) ~964  hidden by  trendBadgeHtml() ~5036   — different arity
//   nodeBreadth(sym,k)  ~1451 hidden by  nodeBreadth(sym)  ~14828 — different return shape
// They are pinned so they stay visible and so anything NEW fails here instead of in a caller.
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

[['./v10.js','the panel'],['./current/gex-if-levels.user.js','the companion']].forEach(function(f){
  const src=fs.readFileSync(f[0],'utf8');
  const names={}; const re=/^function\s+([A-Za-z_$][\w$]*)\s*\(/gm; let m;
  while((m=re.exec(src))!==null){ names[m[1]]=(names[m[1]]||0)+1; }
  const dupes=Object.keys(names).filter(n=>names[n]>1).sort();
  // (v11.98) THE LIST IS EMPTY NOW, AND IT SHOULD STAY EMPTY.
  // `nodeBreadth` was declared twice — the later one-argument version won for the whole file and the
  // earlier two-argument one was unreachable. That is the exact shape of the `ifNum` collision that
  // shipped broken for nine releases. `trendBadgeHtml` was declared twice with no caller on either.
  // ⚠ A name appearing here is not a style problem: it is a function that looks live, reads live, and
  // never runs — while its twin silently answers every call.
  const known = [];
  const fresh = dupes.filter(n=>known.indexOf(n)<0);
  ok(fresh.length===0, 'no NEW duplicate function name in '+f[1], fresh);
  ok(JSON.stringify(dupes)===JSON.stringify(known),
     'the known collisions in '+f[1]+' are still exactly the ones we know about', dupes);
});
// and the specific regression that motivated this file
{
  const src=fs.readFileSync('./v10.js','utf8');
  ok(/function ifNum\(txt, label\)/.test(src), 'the manual-IF parser keeps the ifNum name it had first');
  ok(/function dispNum\(x\)/.test(src), 'and the display formatter is dispNum');
  ok(!/function ifNum\(x\)/.test(src), 'nothing redeclares ifNum with the formatter signature');
}
console.log('\n'+pass+' pass / '+fail+' fail');
