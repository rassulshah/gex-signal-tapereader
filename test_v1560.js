// ============================================================================================
// test_v1560.js — (v15.60) 📌 OPEN ITEMS (issues, questions) + enhancement requests on the Roadmap.
//   One store, one path (the TRACK path): typed → export `items` → nightly → learning/items.json → the review's
//   answer → back under the item. Functions run with stubs; both tabs render in jsdom; the nightly ingests a
//   planted day file; the manifest carries items.json. GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs'), vm=require('vm'), cp=require('child_process'), os=require('os'), path=require('path');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));
const PAL={ ink:'#e6edf3', sub:'#8b98a9', line:'#1e2530', card:'#12161f', gold:'#e3c341', blue:'#7cc7ff', amber:'#f2b45a', longAccent:'#2ec27e', shortAccent:'#f0616d' };
const esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');

// ---- 1 · the store ----------------------------------------------------------------------------------
{
  const store={};
  const g={ localStorage:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{ store[k]=String(v); } }, ITEMS_KEY:'gpts_items_v1', ITEMSFILE_KEY:'gpts_itemsfile_v1', ITEM_KINDS:{ issue:'ISSUE', question:'QUESTION', enhancement:'ENHANCEMENT' }, ctTodayStr:()=>'2026-09-03' };
  const T=build(g,['itemsLoad','itemsSave','itemsAdd','itemsRemove','itemsExport','itemsFileLoad','itemsMerged','itemsClosed'],'return { load:itemsLoad, add:itemsAdd, remove:itemsRemove, exp:itemsExport, merged:itemsMerged, closed:itemsClosed };')(g);
  ok(T.add('bug','the ladder is blank')===null && T.add('issue','ab')===null && T.load().length===0,'1a an unknown kind or a too-short text is refused');
  const i1=T.add('issue','  the ladder   is blank after 15:00  '); const q1=T.add('question','why is PDC a magnet?'); const e1=T.add('enhancement','a King-only ladder view');
  ok(i1 && i1.kind==='issue' && i1.text==='the ladder is blank after 15:00' && i1.status==='NEW' && /^I/.test(i1.id) && q1.kind==='question' && e1.kind==='enhancement','1b issue / question / enhancement are stored, trimmed, dated, stamped NEW');
  ok(T.add('issue','the ladder is blank after 15:00')===null && T.load().length===3,'1c the same text under the same kind is not added twice');
  let M=T.merged(); ok(M.length===3 && M.every(m=>m.status==='NEW' && m.local),'1d merged, nothing exported: all NEW');
  const ex1=T.exp('2026-09-03'); ok(ex1.length===3 && ex1.every(r=>r.id&&r.kind&&r.text) && !('exported' in ex1[0]),'1e the export carries id / kind / text / date and stamps the records');
  M=T.merged(); ok(M.every(m=>m.status==='IN THE EXPORT'),'1f after the export the status says so');
  // the review's file comes back
  store['gpts_itemsfile_v1']=JSON.stringify({ schema:1, asOf:'2026-09-04', items:[
    { id:q1.id, kind:'question', text:q1.text, status:'ANSWERED', answer:'PDC breaks on first touch 44–56% (n=215/240): it is crossed, not defended.', link:'H2.10', answeredOn:'2026-09-04' },
    { id:i1.id, kind:'issue', text:i1.text, status:'FIXED', answer:'latched book after the close', link:'v15.61', answeredOn:'2026-09-04' },
    { id:e1.id, kind:'enhancement', text:e1.text, status:'PLANNED', answer:'as v15.67', link:'v15.67' },
    { id:'Ifile', kind:'question', text:'a question that exists only in the file', status:'DECLINED', answer:'out of scope: not the objective' } ]});
  M=T.merged();
  const byId={}; M.forEach(m=>byId[m.id]=m);
  ok(byId[q1.id].status==='ANSWERED' && /n=215\/240/.test(byId[q1.id].answer) && byId[q1.id].link==='H2.10','1g a question comes back ANSWERED with its evidence and link');
  ok(byId[i1.id].status==='FIXED' && byId[i1.id].link==='v15.61' && byId[e1.id].status==='PLANNED','1h an issue FIXED with the version; an enhancement PLANNED');
  ok(byId['Ifile'] && byId['Ifile'].local===false && byId['Ifile'].status==='DECLINED' && T.closed('DECLINED') && T.closed('FIXED') && T.closed('ANSWERED') && !T.closed('PLANNED') && !T.closed('SEEN'),'1i an item only in the file appears (not local); ANSWERED / FIXED / DECLINED are closed, PLANNED and SEEN are not');
  T.remove(i1.id); ok(T.load().length===2 && T.merged().some(m=>m.id===i1.id && !m.local),'1j removing locally keeps the file’s record');
}

// ---- 2 · both tabs render; the add path; the export ------------------------------------------------------
{
  const { JSDOM }=require('jsdom');
  const store={};
  store['gpts_itemsfile_v1']=JSON.stringify({ schema:1, asOf:'2026-09-04', items:[{ id:'Iq1', kind:'question', text:'why is PDC a magnet?', status:'ANSWERED', answer:'because it is crossed, not defended (44–56% first-touch breaks, n=215/240)', link:'H2.10', answeredOn:'2026-09-04' },{ id:'Ie1', kind:'enhancement', text:'a King-only ladder view', status:'PLANNED', answer:'as v15.67', link:'v15.67' }]});
  const dom=new JSDOM('<!doctype html><html><body></body></html>',{ url:'https://app.skylit.ai/atlas', pretendToBeVisual:true }); const win=dom.window;
  win.matchMedia=q=>({matches:false,media:q,addListener(){},removeListener(){},addEventListener(){},removeEventListener(){}});
  win.requestAnimationFrame=cb=>0; win.cancelAnimationFrame=()=>{};
  Object.defineProperty(win,'localStorage',{ value:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{store[k]=String(v);}, removeItem:k=>{delete store[k];}, clear:()=>{}, key:i=>Object.keys(store)[i]||null, get length(){return Object.keys(store).length;} }, configurable:true });
  win.indexedDB=undefined; win.setInterval=()=>0; win.clearInterval=()=>{}; win.setTimeout=()=>0; win.clearTimeout=()=>{}; win.fetch=()=>new Promise(()=>{});
  const i0=src.indexOf('(function(){'), i1=src.lastIndexOf('})();'); const ctx=vm.createContext(win); win.window=win;
  vm.runInContext(src.slice(i0+'(function(){'.length, i1), ctx, { filename:'gex.user.js' });
  const run=c=>vm.runInContext(c,ctx); run('buildPanel()');
  run('RENDER_ERRS.length=0'); run('__gptsDebug.showItems(true)');
  let html=run('elBody.innerHTML'); const errs=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
  ok(errs.length===0,'2a the Open Items tab renders with nothing swallowed',errs);
  ok(/① ISSUES · open/.test(html) && /② QUESTIONS · open/.test(html) && /③ CLOSED/.test(html) && /id="gpts-item-issue"/.test(html) && /id="gpts-item-question"/.test(html) && /\+ Issue/.test(html) && /\+ Question/.test(html),'2b ISSUES and QUESTIONS each with a field and an add button; CLOSED under them');
  ok(/answers read back 2026-09-04/.test(html),'2c the header says when the review’s answers were read back');
  run('document.getElementById("gpts-item-issue").value="the READ shows a gap open as a sweep"; __gptsDebug.itemsAdd("issue");');
  run('document.getElementById("gpts-item-question").value="does the King roll before the LOD?"; __gptsDebug.itemsAdd("question");');
  html=run('elBody.innerHTML');
  const local=JSON.parse(store['gpts_items_v1']);
  ok(local.length===2 && local[0].kind==='issue' && local[1].kind==='question' && /the READ shows a gap open as a sweep/.test(html) && /does the King roll before the LOD\?/.test(html) && /NEW/.test(html),'2d + Issue and + Question store and render the item as NEW');
  run('__gptsDebug.showItems(true); TAB_SECTIONS["oi3"]=true; render();'); html=run('elBody.innerHTML');
  ok(/why is PDC a magnet\?/.test(html) && /THE ANSWER · 2026-09-04/.test(html) && /44–56% first-touch breaks, n=215\/240/.test(html) && /ANSWERED · H2\.10/.test(html),'2e a question answered in the file shows under CLOSED with THE ANSWER, its date and link');
  // the roadmap tab carries the enhancement field and the PLANNED item
  run('__gptsDebug.showRoadmap(true)'); html=run('elBody.innerHTML');
  ok(/⊕ REQUEST AN ENHANCEMENT/.test(html) && /id="gpts-item-enhancement"/.test(html) && /\+ Enhancement/.test(html) && /PLANNED · v15\.67/.test(html) && /a King-only ladder view/.test(html),'2f the Roadmap tab: the enhancement field, and a PLANNED request with its version');
  run('document.getElementById("gpts-item-enhancement").value="show the READ on the ladder itself"; __gptsDebug.itemsAdd("enhancement");'); html=run('elBody.innerHTML');
  ok(JSON.parse(store['gpts_items_v1']).length===3 && /show the READ on the ladder itself/.test(html),'2g + Enhancement stores and renders under the Roadmap');
  // the export carries them
  const exp=JSON.parse(run('JSON.stringify(itemsExport("2026-09-03"))'));
  ok(exp.length===3 && exp.map(r=>r.kind).join(',')==='issue,question,enhancement','2h itemsExport returns the three rows the day file carries');
  ok(/items:\(function\(\)\{ try\{ return itemsExport\(dk\); \}/.test(src),'2i buildDayExport writes `items`');
  run('__gptsDebug.showItems(true)'); run('__gptsDebug.showDashboard()'); html=run('elBody.innerHTML');
  ok(!/① ISSUES/.test(html) && /(the read · from the stats|<em>SWEPT<\/em>)/.test(html),'2j the Dashboard clears the Open Items view, straight from it');
}

// ---- 3 · the nightly ingests, the manifest carries ------------------------------------------------------
{
  const tmp=fs.mkdtempSync(path.join(os.tmpdir(),'items-')); const dayFile=path.join(tmp,'2026-09-03.json');
  fs.writeFileSync(dayFile, JSON.stringify({ schema:'gex-day-export/v1', date:'2026-09-03', snaps:{}, items:[{ id:'Itest1', kind:'issue', text:'planted issue', date:'2026-09-03', t:1 },{ id:'Itest2', kind:'enhancement', text:'planted enhancement', date:'2026-09-03', t:2 }] }));
  const py=`import sys, json, io, importlib.util, os
spec=importlib.util.spec_from_file_location('run', 'tools/nightly/run.py'); m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
m.ITEMS='${tmp.replace(/\\/g,'/')}/items.json'
D=json.load(open('${dayFile.replace(/\\/g,'/')}'))
n=m.ingest_items([('2026-09-03', D)]); n2=m.ingest_items([('2026-09-03', D)])
J=json.load(open(m.ITEMS)); print(json.dumps(dict(n=n, n2=n2, items=[(x['id'],x['kind'],x['status'],x['seenIn']) for x in J['items']], asOf=J['asOf'])))`;
  const r=cp.spawnSync('python3',['-c',py],{encoding:'utf8'});
  let J=null; try{ J=JSON.parse((r.stdout||'').trim().split('\n').pop()); }catch(e){}
  ok(r.status===0 && J && J.n===2 && J.n2===0 && J.items.length===2 && J.items[0][2]==='SEEN' && J.items[0][3]==='2026-09-03','3a the nightly copies new items into items.json as SEEN, once (idempotent)',(r.stderr||'').slice(-200)||J);
  ok(fs.existsSync('learning/items.json') && JSON.parse(fs.readFileSync('learning/items.json','utf8')).schema===1,'3b learning/items.json exists in the tree');
  const m=cp.spawnSync('python3',['tools/build-installer.py','--list'],{encoding:'utf8'});
  ok((m.stdout||'').split('\n').indexOf('learning/items.json')>=0 && /pipeFetch\(PIPE_RAW_BASE\+'\/learning\/items\.json'\)/.test(src),'3c the panel fetches learning/items.json and the installer carries it');
  const plan=JSON.parse(fs.readFileSync('learning/plan.json','utf8'));
  ok(plan.tabs.some(t=>t.tab==='Open Items') && plan.roadmap.find(x=>x.v==='15.60' && x.status==='shipped') && /items/.test(plan.stages[0].what),'3d the plan knows the sixth tab and this build');
}
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
