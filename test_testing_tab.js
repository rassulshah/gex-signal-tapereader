// v10.45 tests — Testing tab: factor extraction, pattern miner, hypothesis engine,
// insights rule engine, tab wiring. Pure logic extracted from the source (repoAll stubbed).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,msg){ if(c){pass++;} else {fail++; console.log('  FAIL:',msg);} }
function grab(name){const i=src.indexOf('function '+name+'(');if(i<0)return '';let d=0,j=src.indexOf('{',i);for(let k=j;k<src.length;k++){if(src[k]=='{')d++;if(src[k]=='}'){d--;if(!d)return src.slice(i,k+1);}}}

// --- wiring guards (string checks, no eval) ---
ok(/var TESTING_VIEW=false;/.test(src), 'TESTING_VIEW state var added');
ok(/window\.__gptsDebug\.showTesting=function/.test(src), 'showTesting handler');
ok(/if\(TESTING_VIEW\)\{/.test(src), 'render branch for Testing');
ok(/tab\('\\uD83E\\uDDEA Testing', TESTING_VIEW/.test(src), 'third tab in the tab bar');
ok(/if\(ANALYSIS_VIEW\)\{ TESTING_VIEW=false; ARCH_VIEW=false; ROADMAP_VIEW=false; \}/.test(src) && /if\(TESTING_VIEW\)\{ ANALYSIS_VIEW=false; ARCH_VIEW=false; ROADMAP_VIEW=false; \}/.test(src), 'tabs mutually exclusive (v15.59: five views, each clears the other four)');
ok(/window\.__gptsHypo=function/.test(src) && /window\.__gptsMineRun=function/.test(src) && /window\.__gptsHypoRun=function/.test(src), 'console + preset APIs exposed');
// (v10.54 GROUP 5.2) the Testing tab was rebuilt around the learning loop: question
// queue -> proposals -> challengers -> kill list -> self-test -> coverage. The v10.45
// hypothesis builder / pattern miner / research list are kept in a DETAIL section.
ok(/function testingBlock\(\)/.test(src), 'testingBlock exists');
// (v15.55) the tab in LOOP order — design/ANALYSIS-TESTING-BY-SUBJECT.md §3
[['\u2460','THE REGISTER'],['\u2461','THE GATE'],['\u2462','ON THE DASHBOARD'],['\u2463','THE RECORD'],['\u2464','THE NIGHTLY'],['⑥','THE SUITE']].forEach(function(p){
  // the source writes some glyphs as \uXXXX escapes and some as characters; accept either, and a title prefix
  var esc=p[0].split('').map(function(c){ var cc=c.charCodeAt(0); return cc>127?('\\u'+cc.toString(16).padStart(4,'0')):c; }).join('');
  ok(src.indexOf("'"+p[0]+"','"+p[1])>=0 || src.indexOf("'"+esc+"','"+p[1])>=0, 'testing section '+p[0]+' '+p[1]+' is rendered');
});
ok(/PATTERN MINER/.test(src) && /Hypothesis builder|hypothesis/i.test(src), 'the v10.45 exploration tools survive in DETAIL');
ok(/gpts_mine_v1/.test(src), 'miner result cached');
// (v12.0) VERSION PINS ARE NUMERIC NOW. These were regex alternations listing every allowed major
// ('10.4x|10.5x|11.x'), so every major bump broke three unrelated suites at once — 11 -> 12 did it
// again. Parse the version and compare; a floor is what the assertion actually means.
function verAtLeast(src, min){
  var m=/@version\s+([0-9]+)\.([0-9]+)/.exec(src); if(!m) return false;
  var a=parseInt(m[1],10), b=parseInt(m[2],10);
  var p=String(min).split('.'), A=parseInt(p[0],10), B=parseInt(p[1]||'0',10);
  return (a>A) || (a===A && b>=B);
}
ok(verAtLeast(src,'10.40'), 'version is at least 10.40', (/@version\s+\S+/.exec(src)||[])[0]);
ok((src.match(/^function render\(\)/gm)||[]).length===1 && /\}\)\(\);\s*$/.test(src), 'file shape rule 2.4 (one render, closes cleanly)');

// --- pure logic: stub repoAll + localStorage, run miner/hypothesis on synthetic rows ---
let ROWS=[]; global.repoAll=cb=>cb(ROWS);
global.localStorage={_d:{},getItem(k){return this._d[k]||null;},setItem(k,v){this._d[k]=v;}};
eval(src.match(/var T_FACTORS=\[[^\]]*\];/)[0]+grab('_tRows')+grab('_tKing')+grab('_tFacts')+grab('studyMine')+grab('studyMineLoad')+grab('studyHypothesis'));

// facts
let f=_tFacts({px:776,tking:778,t:new Date('2026-08-14T16:00:00Z').getTime(),out10:{net:0.3},nodes:[{k:777,pct:100,st:'Building'}]});
ok(f.dir==='up' && f.towardKing==='yes' && f.kzone==='pull' && f.kside==='above' && f.hour==='11-12' && f.nearState==='Acm' && f.nearStrong==='strong', 'factor extraction ('+JSON.stringify(f)+')');
let f2=_tFacts({px:776,tking:776.5,t:Date.now(),out10:{net:-0.2},nodes:[]});
ok(f2.kzone==='orbit', 'kzone orbit at <=1 strike');

// build 100 bars: king 2 above, 70% toward (=up)
const base=new Date('2026-08-14T16:00:00Z').getTime();
for(let i=0;i<100;i++){ const net=(i%10<7)?0.3:-0.3; ROWS.push({sym:'SPY',px:776,tking:778,t:base+i*180000,out10:{net:net,mfe:Math.max(0,net),mae:Math.min(0,net)},rg:{tag:'trend'},nodes:[{k:778,pct:100,st:'Building'},{k:774,pct:20,st:'Fading'}]}); }

let hres=null; studyHypothesis({when:[{f:'kzone',v:'pull'}],outcome:'toward'}, r=>hres=r);
ok(hres && hres.rate===70 && hres.n===100, 'hypothesis pull->toward = 70% (n=100) ('+JSON.stringify(hres)+')');
let hres2=null; studyHypothesis({when:[{f:'kzone',v:'orbit'}],outcome:'up'}, r=>hres2=r);
ok(hres2 && hres2.n===0, 'hypothesis with no matching rows => n=0, no crash');

let mres=null; studyMine(m=>mres=m);
ok(mres && mres.n===100 && mres.base===70, 'miner runs: n=100, base up 70% ('+(mres?mres.base:'null')+')');
ok(mres && mres.top.length>0 && mres.top.every(x=>x.n>=30), 'miner rows all n>=30');
ok(mres && typeof mres.combosTested==='number' && mres.combosTested>0, 'miner reports combosTested (multiple-testing)');
ok(studyMineLoad() && studyMineLoad().n===100, 'miner result cached + reloadable');
ok(mres && mres.top.every(x=>typeof x.score==='number'), 'miner rows ranked by score');

// min-n gate: 20 rows => miner returns null
ROWS=ROWS.slice(0,20); let mnull='sentinel'; studyMine(m=>mnull=m);
ok(mnull===null, 'miner returns null below 30 bars');

console.log('test_testing_tab: '+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
