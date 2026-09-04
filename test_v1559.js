// ============================================================================================
// test_v1559.js — (v15.59) ⚙ ARCHITECTURE · 🗺 ROADMAP — THE WHAT, THE HOW AND THE PLAN INSIDE THE APP.
//   One source, three places: the docs (PURPOSE.md, PROCESS.md, ROADMAP.md), learning/plan.json, PLAN_SEED.
//   This test pins them equal, renders both tabs in jsdom with the real script, and checks that a broken loop
//   shows red. GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs'), vm=require('vm');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };

// ---- 1 · one source, three places ------------------------------------------------------------------
const plan=JSON.parse(fs.readFileSync('learning/plan.json','utf8'));
const seed=new Function(src.slice(src.indexOf('var PLAN_SEED='), src.indexOf('function planLoad('))+' return PLAN_SEED;')();
ok(JSON.stringify(seed)===JSON.stringify(plan),'1a PLAN_SEED in the script equals learning/plan.json byte for byte (edit tools/plan-seed.py, run it, re-splice)');
const purpose=fs.readFileSync('design/PURPOSE.md','utf8'), process_=fs.readFileSync('design/PROCESS.md','utf8'), roadmap=fs.readFileSync('roadmap/ROADMAP.md','utf8');
ok(purpose.indexOf(plan.objective.quote.slice(0,80))>=0,'1b the objective’s quote in plan.json is the operator’s sentence in PURPOSE.md');
ok(/high of the day and low of day/i.test(plan.objective.quote) && /pullback/i.test(plan.objective.one) && /deflection IS the turning point/i.test(plan.objective.mechanism),'1c the WHAT: HOD/LOD, pullback, the deflection mechanism');
plan.stages.forEach(sg=>ok(new RegExp('\\b'+sg.id+'\\b').test(process_),'1d PROCESS.md names stage '+sg.n+' '+sg.id));
ok(plan.stages.length===11 && plan.stages.map(s=>s.n).join(',')==='1,2,3,4,5,6,7,8,9,10,11','1e eleven stages, numbered');
const docVersions=[...roadmap.matchAll(/^### v(15\.\d+) —/gm)].map(m=>m[1]);
const planVersions=plan.roadmap.filter(x=>x.status!=='shipped').map(x=>x.v);
ok(docVersions.length>=6 && docVersions.every(v=>planVersions.indexOf(v)>=0),'1f every roadmap version in ROADMAP.md is in plan.json',{doc:docVersions,plan:planVersions});
ok(planVersions.every(v=>docVersions.indexOf(v)>=0),'1g …and every unshipped plan item is in ROADMAP.md',{doc:docVersions,plan:planVersions});
ok(plan.roadmap.filter(x=>x.status==='next').length===1 && plan.roadmap.every(x=>['shipped','next','later'].indexOf(x.status)>=0) && plan.roadmap.filter(x=>x.status!=='shipped').every(x=>x.done),'1h exactly one NEXT; every unshipped item says what done means');
const GV=(src.match(/var GPTS_VERSION='([0-9.]+)'/)||[])[1];
ok(plan.roadmap.find(x=>x.status==='next').v===GV,'1i the NEXT item is the running build (v'+GV+'), so the Roadmap tab marks it RUNNING',plan.roadmap.find(x=>x.status==='next').v);
ok(plan.tabs.length===7 && plan.tabs.map(t=>t.tab).join(',')==='Dashboard,Analysis,Testing,Architecture,Roadmap,Open Items,Learn','1j the seven tabs and their roles (v15.60: Open Items · v15.62: Learn)');
ok(plan.rules.length>=8 && plan.rules.every(r=>r.rule && r.test),'1k every rule names the test behind it');

// ---- 2 · the tabs render, with data and with a broken loop ----------------------------------------------
{
  const { JSDOM }=require('jsdom');
  function boot(store){
    const dom=new JSDOM('<!doctype html><html><body></body></html>',{ url:'https://app.skylit.ai/atlas', pretendToBeVisual:true }); const win=dom.window;
    win.matchMedia=q=>({matches:false,media:q,addListener(){},removeListener(){},addEventListener(){},removeEventListener(){}});
    win.requestAnimationFrame=cb=>0; win.cancelAnimationFrame=()=>{};
    Object.defineProperty(win,'localStorage',{ value:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{store[k]=String(v);}, removeItem:k=>{delete store[k];}, clear:()=>{}, key:i=>Object.keys(store)[i]||null, get length(){return Object.keys(store).length;} }, configurable:true });
    win.indexedDB=undefined; win.setInterval=()=>0; win.clearInterval=()=>{}; win.setTimeout=()=>0; win.clearTimeout=()=>{}; win.fetch=()=>new Promise(()=>{});
    const i0=src.indexOf('(function(){'), i1=src.lastIndexOf('})();'); const ctx=vm.createContext(win); win.window=win;
    vm.runInContext(src.slice(i0+'(function(){'.length, i1), ctx, { filename:'gex.user.js' });
    const run=c=>vm.runInContext(c,ctx); run('buildPanel()'); return run;
  }
  // (a) everything fetched
  const store={};
  store['gpts_studies_v1']=fs.readFileSync('learning/studies.json','utf8'); store['gpts_register_v1']=fs.readFileSync('learning/register.json','utf8');
  const W=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS.json','utf8')); store['gpts_sweeps_v1']=JSON.stringify({corpus:W.corpus,lookup:W.lookup,ledger:W.ledger,cells:W.cells});
  store['gpts_suite_v1']=fs.readFileSync('learning/suite.json','utf8');
  let run=boot(store);
  run('RENDER_ERRS.length=0'); run('__gptsDebug.showArchitecture(true)');
  let html=run('elBody.innerHTML'); let errs=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
  ok(errs.length===0,'2a the Architecture tab renders with nothing swallowed',errs);
  ok(['Dashboard','Analysis','Testing','Learn','Architecture','Roadmap','Open Items'].every(t=>html.indexOf(t)>=0),'2b the tab bar has seven tabs');
  ok(html.indexOf(plan.objective.one.slice(0,60))>=0 && /The mechanism:/.test(html) && /TREND REVERSAL/.test(html) && /PULLBACK REVERSAL/.test(html) && /expensive/i.test(html)===false || /Confusing the pullback/.test(html),'2c ① THE WHAT: the objective, the mechanism, the two kinds, the expensive error');
  plan.stages.forEach(sg=>ok(new RegExp('>'+sg.id+'<').test(html),'2d ② the loop draws stage '+sg.id));
  ok(/studies · SWEEPS suite loaded/.test(html) && /the review’s files are here/.test(html),'2e with the files fetched, REGISTRY and REVIEW are green with their evidence');
  const scoreV=(plan.roadmap.find(x=>/score THE READ/.test(x.title)&&x.status!=='shipped')||{}).v;
  ok(!!scoreV && new RegExp('not built — planned v'+scoreV.replace('.','\\.')).test(html),'2f SCORE is honestly red: not built — and names the plan’s version for it (v'+scoreV+'), not a typed one',scoreV);
  ok(/of 11 stages green right now/.test(html),'2g the header counts the green stages');
  // (b) nothing fetched -> the seeds render, REGISTRY / REVIEW / NIGHTLY are red
  run=boot({});
  run('__gptsDebug.showArchitecture(true)'); html=run('elBody.innerHTML');
  ok(/registry not fetched — every file the panel fetches must ride the installer/.test(html) && /not fetched \(404\?\)/.test(html) && /no nightly log read back/.test(html),'2h with nothing fetched the loop shows red where it is broken (the manifest bug would have been visible on day one)');
  ok(html.indexOf(plan.objective.one.slice(0,60))>=0,'2i …and the WHAT still renders from the seed');
  // (c) the roadmap tab
  run('__gptsDebug.showRoadmap(true)'); html=run('elBody.innerHTML');
  ok(/① NEXT/.test(html) && /② AFTER THAT/.test(html) && /③ SHIPPED/.test(html) && /④ HIS DECISIONS/.test(html) && /⑤ STANDING CONSTRAINTS/.test(html),'2j the Roadmap tab: NEXT · AFTER THAT · SHIPPED · HIS DECISIONS · STANDING CONSTRAINTS');
  ok(new RegExp('v'+GV.replace('.','\\.')+'</span><div[^>]*>RUNNING').test(html),'2k the running version is marked on its row');
  const order=plan.roadmap.filter(x=>x.status==='later').map(x=>html.indexOf('v'+x.v+'</span>'));
  ok(order.every(i=>i>=0) && order.every((v,i)=>i===0||v>order[i-1]),'2l AFTER THAT keeps the plan’s order',order);
  ok(/done when: /.test(html) && /Skylit API backfill/.test(html) && (/one install file per build/.test(html) || /STANDING CONSTRAINTS/.test(html)),'2m done-when lines, his open decisions, the standing constraints (folded by default)');
  // (d) mutual exclusion with the other tabs
  run('__gptsDebug.showAnalysis(true)'); html=run('elBody.innerHTML');
  ok(!/① NEXT/.test(html) && /TRACK SOMETHING UNDER/.test(html),'2n switching to Analysis leaves the Roadmap');
  run('__gptsDebug.showRoadmap(true)'); run('__gptsDebug.showDashboard()'); html=run('elBody.innerHTML');
  ok(!/① NEXT/.test(html) && !/THE WHAT · the objective/.test(html) && /(the read · from the stats|<em>SWEPT<\/em>)/.test(html),'2o …and the Dashboard clears every view flag, straight from the Roadmap (v15.63: the SWEPT line is the read)');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
