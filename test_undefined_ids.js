// ============================================================================================
// test_undefined_ids.js — AN UNDECLARED IDENTIFIER IS A RUNTIME BOMB WITH NO COMPILE ERROR.
//
// 2026-08-30: v14.91 shipped `dayCandleSvg(SYM, D, PTL)` inside secDay, whose parameter is `sym`.
// `SYM` is declared NOWHERE. `node --check` passed. 47 assertions in test_nodeat passed. The whole
// ⓪a section vanished from the operator's panel and the mount's own catch swallowed it silently.
//
// ⚠⚠ EVERY EXISTING TEST FOR THAT SECTION IS A SOURCE GREP. Greps prove text is present; they
// cannot prove it RUNS. Two of the three bad references sat inside try/catch and returned null
// forever — HodN/LodN/PTN and the GREEN/RED call would have read em-dash indefinitely with nothing
// failing anywhere. One typo, three broken features, zero red tests.
//
// This scans function bodies for identifiers that are neither declared in the file nor JS builtins.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

// everything the file itself declares
const declared=new Set();
// ⚠ COMMA-SEPARATED DECLARATORS. `var GDc=null, GDa=null;` declares BOTH. A parser that takes only
// the first name reports the second as undeclared — which this one did on its first run, flagging
// `GDa` as a bug when it was the lint that was wrong. Walk the whole declaration statement.
// ⚠ AND DECLARATIONS SPAN LINES. `var str=..., pOpen=...,\n    pNow=...;` declares pNow on the
// CONTINUATION line. Stopping at \n missed it and the lint reported a live function as broken —
// the third parsing bug in this lint, all of them false ALARMS. A lint's own parser needs the same
// scrutiny as the code it polices, or it trains you to ignore it.
for (const m of src.matchAll(/\b(?:var|let|const)\s+([^;]{0,600})/g)){
  let depth=0, cur='';
  for(const ch of m[1]){
    if('([{'.includes(ch)) depth++;
    else if(')]}'.includes(ch)) depth--;
    if(ch===',' && depth<=0){ cur=''; continue; }
    if(depth<=0 && ch==='=') { const n=cur.trim().match(/^[A-Za-z_$][\w$]*$/); if(n) declared.add(n[0]); cur='~'; continue; }
    if(cur!=='~') cur+=ch;
  }
  const first=m[1].split(/[,=;]/)[0].trim().match(/^[A-Za-z_$][\w$]*$/);
  if(first) declared.add(first[0]);
  for(const d of m[1].split(',')){
    const n=d.trim().split('=')[0].trim().match(/^[A-Za-z_$][\w$]*$/);
    if(n) declared.add(n[0]);
  }
}
for (const m of src.matchAll(/\bfunction\s+([A-Za-z_$][\w$]*)\s*\(([^)]*)\)/g)){
  declared.add(m[1]);
  m[2].split(',').map(s=>s.trim()).filter(Boolean).forEach(p=>declared.add(p));
}
for (const m of src.matchAll(/\bfunction\s*\(([^)]*)\)/g))
  m[1].split(',').map(s=>s.trim()).filter(Boolean).forEach(p=>declared.add(p));
for (const m of src.matchAll(/\bcatch\s*\(\s*([A-Za-z_$][\w$]*)\s*\)/g)) declared.add(m[1]);
for (const m of src.matchAll(/\bfor\s*\(\s*(?:var|let|const)?\s*([A-Za-z_$][\w$]*)\s+(?:in|of)\b/g)) declared.add(m[1]);

const BUILTIN=new Set(['window','document','console','Math','JSON','Date','Object','Array','String',
 'Number','Boolean','RegExp','Error','Promise','Map','Set','WeakMap','localStorage','sessionStorage',
 'setTimeout','setInterval','clearTimeout','clearInterval','parseInt','parseFloat','isNaN','isFinite',
 'encodeURIComponent','decodeURIComponent','fetch','navigator','location','requestAnimationFrame',
 'MutationObserver','performance','RE','Infinity','NaN','undefined','null','true','false','this','arguments',
 'typeof','instanceof','new','return','if','else','for','while','var','let','const','function','try',
 'catch','throw','break','continue','switch','case','default','delete','in','of','do','void','GM_xmlhttpRequest','unsafeWindow','TextEncoder','Intl','btoa','atob','URL','Blob','crypto']);

function bodyOf(name){
  const i=src.indexOf('function '+name+'(');
  if(i<0) return '';
  let d=0,st=false;
  for(let j=i;j<src.length;j++){ const c=src[j];
    if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0) return src.slice(i,j+1);} }
  return '';
}

// ⚠ Scan the sections that RENDER — a throw in any of them silently blanks a whole block, because
// every one is mounted inside a swallow().
const SCAN=['secDay','secLoc','secFrame','dayCandleSvg','gdRead','gdActual','hlNodeAt','deflKings','deflNodeAt','hlPT','hlLevelHit','ifLadder'];
let bad=[];
for(const fn of SCAN){
  const b=bodyOf(fn);
  if(!b){ bad.push(fn+': NOT FOUND'); continue; }
  const stripped=b.replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'')
                  .replace(/'(?:\\.|[^'\\])*'/g,"''").replace(/"(?:\\.|[^"\\])*"/g,'""')
                  .replace(/`(?:\\.|[^`\\])*`/g,'``')
                  // ⚠ REGEX LITERALS TOO. `/Mag/.test(x)` is not a reference to `Mag`. Leaving them
                  // in made the lint report six phantom identifiers in secLoc on its first run — and
                  // a lint that cries wolf is a lint nobody keeps.
                  .replace(/([=(,:[!&|?{;]\s*)\/(?![*\/])(?:\\.|\[(?:\\.|[^\]\\])*\]|[^\/\\\n])+\/[gimsuy]*/g,'$1RE');
  for(const m of stripped.matchAll(/\b([A-Za-z_$][\w$]*)\b/g)){
    const id=m[1];
    if(declared.has(id)||BUILTIN.has(id)) continue;
    // skip property accesses  .foo  and object keys  foo:
    const before=stripped.slice(Math.max(0,m.index-1), m.index);
    const after=stripped.slice(m.index+id.length, m.index+id.length+1);
    if(before==='.'||after===':') continue;
    bad.push(fn+': '+id);
  }
}
bad=[...new Set(bad)];
ok(bad.length===0, 'u1 no undeclared identifiers in the rendering path', bad.slice(0,10));

// the specific regression, pinned by name
const SD=bodyOf('secDay');
ok(!/\bSYM\b/.test(SD), 'u2 secDay uses its own `sym` parameter, never a bare SYM');
ok(/dayCandleSvg\(sym,/.test(SD), 'u3 the candle is called with sym');
ok(/gdRead\(sym\)/.test(SD) && /gdActual\(sym\)/.test(SD), 'u4 the GREEN/RED call is passed sym');
ok(/hlNodeAt\(sym,/.test(SD), 'u5 the node engine is passed sym');

console.log('test_undefined_ids: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
