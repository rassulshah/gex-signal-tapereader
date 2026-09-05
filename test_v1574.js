// ============================================================================================
// test_v1574.js — (v15.74) THE LOG SURVIVES A RELOAD. Operator, 2026-09-04 22:3x CT, minutes after installing
//   v15.73: "why hasn't the analysis started" — the day line read "analysis overdue — is the GEX nightly task
//   installed?" while learning/log/2026-09-04.json (his machine, 22:35 CT) was on GitHub. ANALYSIS_NIGHTLY lived only
//   in memory; the pipeline's 10-minute throttle (P.t in localStorage) survived the reload and kept the fetch from
//   running again. The log is now kept in gpts_nightly_v1 and restored at load. Functions run with stubs; the
//   restore is executed, not grepped; GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=m.index, d=0, started=false; for(; i<src.length; i++){ const c=src[i]; if(c==='{'){d++;started=true;} else if(c==='}'){d--; if(started&&d===0){i++;break;}} } return src.slice(m.index,i); }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const LOG=JSON.parse(fs.readFileSync('learning/log/2026-09-04.json','utf8'));
const mkStore=(init)=>{ const store=Object.assign({},init||{}); return { store, localStorage:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{ store[k]=String(v); }, removeItem:k=>{ delete store[k]; } } }; };

ok(/@version\s+15\.74/.test(src) && /var GPTS_VERSION='15\.74';/.test(src),'0a v15.74 in both spots');

// ---- 1 · the store: save, load, garbage ------------------------------------------------------------------------
{
  const W=mkStore();
  const T=new Function('__g','var localStorage=__g.localStorage;\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+ex('nightlySave')+'\nreturn { load:nightlyLoad, save:nightlySave };')(W);
  ok(T.load()===null,'1a nothing stored → null (not undefined, not {})');
  T.save(LOG); const back=T.load();
  ok(back && back.date==='2026-09-04' && back.ranOn===LOG.ranOn && back.ranAt===LOG.ranAt && (back.hypotheses||[]).length===LOG.hypotheses.length && back.patterns.events===LOG.patterns.events,'1b the whole log round-trips (date · ranOn · ranAt · hypotheses · patterns)',back&&[back.date,back.ranOn,back.ranAt]);
  ok(Object.keys(W.store).length===1 && Object.keys(W.store)[0]==='gpts_nightly_v1','1c one key: gpts_nightly_v1',Object.keys(W.store));
  T.save(null); T.save('x'); T.save({ranOn:'cloud'});
  ok(T.load() && T.load().date==='2026-09-04','1d a null, a string or a log without a date never overwrites the stored one');
  W.store['gpts_nightly_v1']='{not json'; ok(T.load()===null,'1e a corrupt stored copy reads as none, never throws');
  W.store['gpts_nightly_v1']='"just a string"'; ok(T.load()===null,'1f a JSON string is not a log');
  W.store['gpts_nightly_v1']=JSON.stringify({date:7}); ok(T.load()===null,'1g a log whose date is not a string is not a log');
  const B=mkStore(); B.localStorage.setItem=()=>{ throw new Error('QuotaExceededError'); };
  const T2=new Function('__g','var localStorage=__g.localStorage;\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+ex('nightlySave')+'\nreturn { load:nightlyLoad, save:nightlySave };')(B);
  let threw=false; try{ T2.save(LOG); }catch(e){ threw=true; } ok(!threw && T2.load()===null,'1h a full localStorage: the save swallows the quota error, the load says none');
}

// ---- 2 · the restore at load: ANALYSIS_NIGHTLY is the stored log before any fetch --------------------------------
{
  const decl=exVar('ANALYSIS_NIGHTLY');
  ok(decl==='var ANALYSIS_NIGHTLY=nightlyLoad();','2a the declaration restores from the store (not null)',decl);
  const W=mkStore({ gpts_nightly_v1: JSON.stringify(LOG) });
  const v=new Function('__g','var localStorage=__g.localStorage;\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+decl+'\nreturn ANALYSIS_NIGHTLY;')(W);
  ok(v && v.date==='2026-09-04' && v.ranOn===LOG.ranOn,'2b executed: with a stored log, ANALYSIS_NIGHTLY is that log at load',v&&v.date);
  const v0=new Function('__g','var localStorage=__g.localStorage;\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+decl+'\nreturn ANALYSIS_NIGHTLY;')(mkStore());
  ok(v0===null,'2c with nothing stored it is null, as before');
  const iDecl=src.indexOf('var ANALYSIS_NIGHTLY='), iFn=src.indexOf('function nightlyLoad(');
  ok(iFn>0 && iDecl>0 && Math.abs(iDecl-iFn)<2000 && src.indexOf('function pipeNightlyTry(')>iDecl,'2d nightlyLoad is declared beside the variable, before pipeNightlyTry (hoisting is not relied on across modules)');
}

// ---- 3 · the fetch stores what it fetched --------------------------------------------------------------------------
{
  const W=mkStore(); const calls=[]; let rendered=0, saved=null;
  const g={ localStorage:W.localStorage, PIPE_RAW_BASE:'https://raw', pipeFetch:(u)=>{ calls.push(u); return Promise.resolve({ ok:true, status:200, json:()=>Promise.resolve(LOG) }); },
    pipeSave:(P)=>{ saved=JSON.parse(JSON.stringify(P)); }, pipeReviewLine:(j)=>'line:'+j.date, pipeRender:()=>{ rendered++; }, lastTradingDay:(b)=>'2026-09-0'+(4-(b||0)) };
  const run=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+ex('nightlySave')+'\nvar ANALYSIS_NIGHTLY=null;\n'+ex('pipeNightlyTry')+'\nvar P={}; pipeNightlyTry(P,"2026-09-04",true); return function(){ return { P:P, N:ANALYSIS_NIGHTLY }; };')(g);
  setTimeout(()=>{
    const r=run();
    ok(calls.length===1 && /\/learning\/log\/2026-09-04\.json$/.test(calls[0]),'3a the fetch asks for the day\'s log',calls);
    ok(r.N && r.N.date==='2026-09-04' && saved && saved.nightly==='yes' && saved.nightlyDay==='2026-09-04' && rendered===1,'3b on success: ANALYSIS_NIGHTLY set, the pipe record stamped, the footer re-rendered',[saved,rendered]);
    const stored=W.store['gpts_nightly_v1']; let J=null; try{ J=JSON.parse(stored); }catch(e){}
    ok(J && J.date==='2026-09-04' && J.ranAt===LOG.ranAt && (J.hypotheses||[]).length===LOG.hypotheses.length,'3c …and the log is in gpts_nightly_v1 — the next reload starts from it',stored&&stored.slice(0,80));
    // the 404 path stores nothing and tries the prior day once
    const W2=mkStore(); const calls2=[];
    const g2=Object.assign({}, g, { localStorage:W2.localStorage, pipeFetch:(u)=>{ calls2.push(u); return Promise.resolve({ ok:false, status:404 }); } });
    const run2=new Function('__g', Object.keys(g2).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+ex('nightlySave')+'\nvar ANALYSIS_NIGHTLY=null;\n'+ex('pipeNightlyTry')+'\nvar P={}; pipeNightlyTry(P,"2026-09-04",true); return function(){ return { P:P, N:ANALYSIS_NIGHTLY }; };')(g2);
    setTimeout(()=>{
      const r2=run2();
      ok(calls2.length===2 && /2026-09-03\.json$/.test(calls2[1]) && r2.N===null && !('gpts_nightly_v1' in W2.store) && r2.P.nightly==='no','3d a 404 tries the prior weekday once, stores nothing, and the record says no',[calls2,r2.P.nightly,Object.keys(W2.store)]);
      section4();
    },20);
  },20);
}

// ---- 4 · the scene: a reload with a fresh pipeline stamp — the day line reads the log at once -----------------------
function section4(){
  const decl=exVar('ANALYSIS_NIGHTLY');
  const W=mkStore({ gpts_nightly_v1: JSON.stringify(Object.assign({}, LOG, { ranOn:'his machine', ranAt:'2026-09-05T03:35:11Z' })) });
  const g={ localStorage:W.localStorage, ctTodayStr:()=>'2026-09-05', ctMarketHours:()=>false, ctAfterClose:()=>false, ctNowSecOfDay:()=>1*3600,
    saveState:()=>({code:'none'}), pipeLoad:()=>({ t:Date.now()-60000, saveDate:'2026-09-04', saveHow:'repo folder', saveT:Date.parse('2026-09-04T20:01:00Z'), pushed:'yes', pushedDay:'2026-09-04' }),
    recorderLoad:()=>({ days:{ '2026-09-04':{ snaps:{ SPY:new Array(101).fill(0) } } } }), AUTOSAVE:{}, DAY_WRITTEN:{}, autosaveState:()=>({code:'idle'}),
    learnLoad:()=>({ rules:[] }), recMerged:()=>[{id:'R-1',status:'proposed'}], g3tip:(t)=>' title="'+t.replace(/"/g,'&quot;')+'"', g3esc:(s)=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;') };
  const T=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+decl+'\n'+exVar('DAYLINE_ANALYSIS_LATE_MS')+'\nvar window={__gptsDebug:{}};\n'+['fmtCT','dayBarCount','dayLineState','dayLineHtml'].map(ex).join('\n')+'\nreturn { state:dayLineState, html:dayLineHtml };')(g);
  const st=T.state(); const seg=(k)=>(st.segs.find(x=>x.key===k)||{});
  ok(st.day==='2026-09-04' && seg('saved').state==='g' && /15:01/.test(seg('saved').text),'4a Saturday small hours after a reload: the line is 9/4, saved 15:01 (the pipe record)',[st.day,seg('saved')]);
  ok(seg('analysis').state==='g' && seg('analysis').text==='22:35 · your machine · 81 taps','4b analysis GREEN from the restored log: 22:35 · your machine · 81 taps — not "overdue — is the GEX nightly task installed?"',seg('analysis'));
  ok(seg('testing').state==='g' && /^9 claims/.test(seg('testing').text),'4c testing reads the restored verdicts',seg('testing'));
  // the same scene with nothing stored is the v15.73 lie — pinned so the fix cannot be undone quietly
  const T0=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('NIGHTLY_KEY')+'\n'+ex('nightlyLoad')+'\n'+decl+'\n'+exVar('DAYLINE_ANALYSIS_LATE_MS')+'\nvar window={__gptsDebug:{}};\n'+['fmtCT','dayBarCount','dayLineState','dayLineHtml'].map(ex).join('\n')+'\nreturn { state:dayLineState };')(Object.assign({}, g, { localStorage:mkStore().localStorage }));
  const s0=T0.state(); const a0=(s0.segs.find(x=>x.key==='analysis')||{});
  ok(a0.state==='r' && /overdue/.test(a0.text),'4d (the pre-fix scene, for the record) with nothing stored the line would read overdue — which is what he saw',a0);
  section5();
}

// ---- 5 · the record ------------------------------------------------------------------------------------------------
function section5(){
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=P.roadmap.filter(r=>r.status==='next');
  ok(nx.length===1 && nx[0].v==='15.74' && /SURVIVES A RELOAD/.test(nx[0].title) && P.roadmap.some(r=>r.v==='15.73' && r.status==='shipped') && P.roadmap.some(r=>r.v==='15.75' && /candidate score/.test(r.title)),'5a the plan: v15.73 shipped, v15.74 this build, the score → v15.75',nx.map(x=>x.v));
  const seedJs=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(P),'5b PLAN_SEED equals the file');
  ok(P.system.storage.some(k=>/gpts_nightly_v1/.test(k.key)),'5c the architecture\'s storage table names gpts_nightly_v1');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); ok(R.rows.some(r=>r.id==='R-5' && /v15\.75/.test(r.text)),'5d R-5 points the score at v15.75');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.74/.test(cl) && cl.indexOf('## v15.74')<cl.indexOf('## v15.73'),'5e the CHANGELOG has the v15.74 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.74/.test(ls.slice(logAt>=0?logAt:0)),'5f the lesson log carries the v15.74 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.74/.test(rn.slice(0,600)) && /reload/i.test(rn),'5g the resume note is at v15.74 and names the reload');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/reload/i.test(cfg.theWhatAndTheHow.dayLine||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1574.js')>=0,'5h .gex-config.json names the restore and this test');
  console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
}
