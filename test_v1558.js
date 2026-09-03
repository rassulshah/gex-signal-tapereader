// ============================================================================================
// test_v1558.js — (v15.58) THE TESTING TAB AS MOCKED (design/mockup-testing-tab.html) + the READ ranks sweeps first.
//   ① register columns (predict / refute if / n-minN bar / verdict) · ② the gate summary · ④ the stores · ⑤ the nightly
//   head · ⑥ the suite stamp. Rendered in jsdom with the real script; functions run with stubs.
// ============================================================================================
const fs=require('fs'), vm=require('vm');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
// a threshold ("held < 45%", "<= 30%", "covers 24%") is a prediction, not a rendered rate; a rate needs its n
const bareP=s=>{ const out=[]; const re=/(\d+)%/g; let m; const txt=String(s).replace(/<[^>]+>/g,' ').replace(/&gt;/g,'>').replace(/&lt;/g,'<'); while((m=re.exec(txt))){ const head=txt.slice(Math.max(0,m.index-12), m.index); const tail=txt.slice(m.index, m.index+70); if(/[<>]=? ?$|covers $|base $|\(base $/.test(head)) continue; if(!/n=|\(n |n \d|sessions\)|n\s*\d+\/\d+|eff n/.test(tail)) out.push(head.slice(-8)+'|'+tail.slice(0,30)); } return out; };
{
  const { JSDOM }=require('jsdom');
  const dom=new JSDOM('<!doctype html><html><body></body></html>',{ url:'https://app.skylit.ai/atlas', pretendToBeVisual:true }); const win=dom.window;
  win.matchMedia=q=>({matches:false,media:q,addListener(){},removeListener(){},addEventListener(){},removeEventListener(){}});
  win.requestAnimationFrame=cb=>0; win.cancelAnimationFrame=()=>{};
  const store={}; Object.defineProperty(win,'localStorage',{ value:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{store[k]=String(v);}, removeItem:k=>{delete store[k];}, clear:()=>{}, key:i=>Object.keys(store)[i]||null, get length(){return Object.keys(store).length;} }, configurable:true });
  win.indexedDB=undefined; win.setInterval=()=>0; win.clearInterval=()=>{}; win.setTimeout=()=>0; win.clearTimeout=()=>{}; win.fetch=()=>new Promise(()=>{});
  store['gpts_studies_v1']=fs.readFileSync('learning/studies.json','utf8');
  store['gpts_register_v1']=fs.readFileSync('learning/register.json','utf8');
  const W=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS.json','utf8')); store['gpts_sweeps_v1']=JSON.stringify({corpus:W.corpus,lookup:W.lookup,ledger:W.ledger,cells:W.cells});
  store['gpts_suite_v1']=JSON.stringify({ schema:1, version:'15.58', at:'2026-09-03T18:00Z', files:135, green:126, red:9, redFiles:['test_expiry_profile.js'] });
  const i0=src.indexOf('(function(){'), i1=src.lastIndexOf('})();'); const ctx=vm.createContext(win); win.window=win;
  vm.runInContext(src.slice(i0+'(function(){'.length, i1), ctx, { filename:'gex.user.js' });
  const run=c=>vm.runInContext(c,ctx);
  run('buildPanel()'); run('RENDER_ERRS.length=0'); run('__gptsDebug.showTesting(true)');
  const html=run('elBody.innerHTML'); const errs=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
  ok(errs.length===0,'1a the Testing tab renders with nothing swallowed',errs);
  ok(/<th[^>]*>hyp<\/th>[\s\S]*?subject · study[\s\S]*?claim[\s\S]*?predict[\s\S]*?refute if[\s\S]*?n \/ minN[\s\S]*?verdict/.test(html),'1b ① THE REGISTER has the mockup’s columns: hyp · subject·study · claim · predict · refute if · n/minN · verdict');
  const h2row=(html.match(/<tr[^>]*>(?:(?!<\/tr>)[\s\S])*?>H2<\/td>[\s\S]*?<\/tr>/)||[''])[0];
  ok(/F · F2\.1/.test(h2row) && /a node.s 2nd test holds MORE often than its 1st/.test(h2row) && /tap(&gt;|>)=1 held (&gt;|>) 60%/.test(h2row) && /tap(&gt;|>)=1 held (&lt;|<)= 55%/.test(h2row),'1c H2 shows its study, prediction and refutation from the fetched register',h2row.replace(/<[^>]+>/g,'|').slice(0,240));
  ok(/width:0%[^>]*><\/i><\/span> <span[^>]*>0\/30/.test(html),'1d the n / minN bar (0/30 today)');
  ok(/H6<\/td><td[^>]*>H · H2\.7/.test(html) && /H7<\/td><td[^>]*>H · H2\.8/.test(html),'1e H6 / H7 rows with their studies');
  ok(/predicted-low band[\s\S]*?predicted-high band[\s\S]*?n per band[\s\S]*?verdict/.test(html) && /<td[^>]*>dir<\/td>/.test(html) && /<td[^>]*>node<\/td>/.test(html) && /<td[^>]*>decision<\/td>/.test(html),'1f ② THE GATE summary: dir / node / decision rows with both bands, Δ, n per band, verdict');
  ok(/EVERY FEATURE · the live bands/.test(html),'1g …with the full live band table under it');
  ok(/<th[^>]*>store<\/th>[\s\S]*?size[\s\S]*?fields present[\s\S]*?missing for the OPEN studies/.test(html) && /feat \(IDB\)/.test(html) && /TAP record/.test(html) && /the book corpus/.test(html),'1h ④ THE RECORD: the stores table (feat · defl · TAP · ES 1-min · the book corpus · kingRoll)');
  ok(/WHAT UNLOCKS WHEN/.test(html),'1i …with the unlock thresholds under it');
  ok(/last run[\s\S]*?reads next[\s\S]*?H1 at 40 · H2 at 30[\s\S]*?refreshes[\s\S]*?SWEEPS\.json \(116 cells, 284 sessions\)/.test(html),'1j ⑤ THE NIGHTLY head: last run · reads next · refreshes · ledger');
  ok(/THE SUITE · self-test/.test(html) && /135 files/.test(html) && /126 green/.test(html) && /9 red/.test(html) && /test_expiry_profile\.js/.test(html) && /SELF-TEST · the synthetic day/.test(html),'1k ⑥ THE SUITE shows the stamped run (files · green · red · which) then the self-test');
  ok(bareP(html.slice(html.indexOf('① THE REGISTER'), html.indexOf('① THE REGISTER')+9000)).length===0,'1l no bare % in the register block',bareP(html.slice(html.indexOf('① THE REGISTER'), html.indexOf('① THE REGISTER')+9000)));
  const order=['ANALYSIS','TRACKED','REGISTER','GATE','DASHBOARD','NIGHTLY','① THE REGISTER','② THE GATE','③ ON THE DASHBOARD','④ THE RECORD','⑤ THE NIGHTLY','⑥ THE SUITE'].map(k=>html.indexOf(k));
  ok(order.every(i=>i>=0) && order.every((v,i)=>i===0||v>order[i-1]),'1m the loop strip then ①…⑥ in loop order',order);
}
{
  const s=fs.readFileSync('tools/run-tests.sh','utf8');
  ok(/learning\/suite\.json/.test(s) && /"green":%d/.test(s) && /redFiles/.test(s),'2a run-tests.sh stamps learning/suite.json with files / green / red / redFiles');
  ok(fs.existsSync('learning/suite.json') && typeof JSON.parse(fs.readFileSync('learning/suite.json','utf8')).green==='number','2b …and the stamp exists in the tree (so it rides the installer)');
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
