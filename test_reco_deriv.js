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
ok(d.cw===780, 'call wall = largest +γ above spot');
ok(d.pw===775, 'put wall = largest mass below spot');
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
ok(/try\{ h\+=recoTestsHtml\(\)/.test(src), 'recoTestsHtml called in testingBlock');
ok(/@version\s+10\.46/.test(src), 'version 10.46');
ok((src.match(/^function render\(\)/gm)||[]).length===1 && /\}\)\(\);\s*$/.test(src), 'file shape rule 2.4');
console.log('test_reco_deriv: '+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
