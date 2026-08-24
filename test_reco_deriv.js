// v10.46 tests — derived GEX factors + recommended-tests section.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++;} else {fail++; console.log('  FAIL:',m);} }
function grab(name){const i=src.indexOf('function '+name+'(');if(i<0)return '';let d=0,j=src.indexOf('{',i);for(let k=j;k<src.length;k++){if(src[k]=='{')d++;if(src[k]=='}'){d--;if(!d)return src.slice(i,k+1);}}}
eval(grab('deriveFactors'));

const nodes=[{k:780,pct:30,pos:true},{k:777,pct:72,pos:false},{k:775,pct:100,pos:false},{k:772,pct:20,pos:true},{k:770,pct:40,pos:false}];
let d=deriveFactors(nodes,776,775);
ok(d && d.ns===-1, 'net-GEX sign negative when -γ mass dominates');
ok(d.ag===262, 'abs gamma strength = Σ|mass| (262)');
// (v11.7) THE WALL RULE CHANGED, deliberately. It used to read "largest +γ above spot", which required
// pos===true on the call side while the put side accepted any sign. On this very fixture that asymmetry
// picks 780 (+30) over 777 (-72) — it skips a ceiling more than twice as heavy purely because the gamma
// there is negative. A negative-gamma ceiling is still a ceiling; it just behaves differently when broken.
// So both sides are now chosen the same way — heaviest mass on that side of spot — and polarity is
// RECORDED (cwPos/pwPos) rather than used as a filter.
ok(d.cw===777, 'call wall = heaviest mass above spot, sign not required');
ok(d.cwPos===false, 'and its negative polarity is reported, not used to disqualify it');
ok(d.pw===775, 'put wall = largest mass below spot');
ok(d.pwPos===false, 'put wall polarity recorded too');
ok(d.basis==='pct', 'this fixture carries no abs series, so the %King basis is used');
ok(d.nNat===5 && d.nSkipped===0, 'all five nodes are native');
ok(d.ranks[0].k===775 && d.ranks.length<=6, 'GEX ranks: King first, capped');
ok(d.reg==='negGamma', 'regime label from sign/zero-gamma');
ok(typeof d.hhi==='number' && d.hhi>0 && d.hhi<=1, 'HHI concentration in (0,1]');
ok(typeof d.imb==='number' && d.imb>=-1 && d.imb<=1, 'above/below imbalance in [-1,1]');
// zero-gamma crossing: build a book that flips sign
let n2=[{k:770,pct:60,pos:false},{k:772,pct:20,pos:false},{k:778,pct:50,pos:true},{k:780,pct:40,pos:true}];
let d2=deriveFactors(n2,775,778);
ok(d2.zg!=null && d2.zg>778 && d2.zg<780, 'zero-gamma interpolated where cumulative flips ('+d2.zg+')');
ok(deriveFactors([],776,775)===null && deriveFactors(null,776,775)===null, 'null-safe on empty/missing');

// wiring: recorded per bar + section present
ok(/deriv:\(function\(\)\{ try\{ return deriveFactors\(nodes/.test(src), 'deriv recorded in snapshot');
ok(/var RECO_TESTS=\[/.test(src) && /function recoTestsHtml\(\)/.test(src), 'recommended-tests data + renderer');
ok(/⑥ Recommended tests/.test(src), 'section ⑥ rendered');
ok((src.match(/th:'VEX \(vanna\)'/g)||[]).length>=4, 'VEX hypotheses included');
ok(/th:'Confluence'/.test(src) && /th:'Accumulation'/.test(src), 'confluence + accumulation themes');
// (v10.54 GROUP 5) the research-curated list is exploration, not the learning loop, so
// it moved into the Testing tab's collapsible DETAIL section.
ok(/det\+=recoTestsHtml\(\)/.test(src), 'recoTestsHtml called in testingBlock (DETAIL section)');
// (v12.0) VERSION PINS ARE NUMERIC NOW. These were regex alternations listing every allowed major
// ('10.4x|10.5x|11.x'), so every major bump broke three unrelated suites at once — 11 -> 12 did it
// again. Parse the version and compare; a floor is what the assertion actually means.
function verAtLeast(src, min){
  var m=/@version\s+([0-9]+)\.([0-9]+)/.exec(src); if(!m) return false;
  var a=parseInt(m[1],10), b=parseInt(m[2],10);
  var p=String(min).split('.'), A=parseInt(p[0],10), B=parseInt(p[1]||'0',10);
  return (a>A) || (a===A && b>=B);
}
ok(verAtLeast(src,'10.46'), 'version is at least 10.46', (/@version\s+\S+/.exec(src)||[])[0]);
ok((src.match(/^function render\(\)/gm)||[]).length===1 && /\}\)\(\);\s*$/.test(src), 'file shape rule 2.4');
console.log('test_reco_deriv: '+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
