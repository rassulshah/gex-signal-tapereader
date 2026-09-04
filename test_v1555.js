// ============================================================================================
// test_v1555.js — (v15.55) THE ANALYSIS TAB BY SUBJECT · THE TRACK FIELD · THE STATS READ · TESTING
//   design/ANALYSIS-TESTING-BY-SUBJECT.md. Every function is EXTRACTED AND RUN with stubs; both tabs and
//   the face are RENDERED in jsdom with the real script; the registry, the register and the sweep table
//   are checked as files. GPTS_SRC points the harness at a mutated copy (the mutation pass below).
// ============================================================================================
const fs=require('fs'), cp=require('child_process'), vm=require('vm');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
function exVar(n){ const m=new RegExp('\\nvar '+n+'=').exec(src); if(!m) throw new Error('var not found: '+n);
  let i=src.indexOf('[',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='[')d++; else if(src[k]===']'){ d--; if(!d) return 'var '+n+'='+src.slice(i,k+1)+';'; } } }
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));
const PAL={ ink:'#e6edf3', sub:'#8b98a9', line:'#1e2530', card:'#12161f', gold:'#e3c341', blue:'#7cc7ff', amber:'#f2b45a', longAccent:'#2ec27e', shortAccent:'#f0616d' };
const esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
const W=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS.json','utf8'));
const Wslim={ corpus:W.corpus, lookup:W.lookup, ledger:W.ledger, cells:W.cells };
// the house rule, as a function: every % in a string must carry an n within 40 characters after it
const bareP=s=>{ const out=[]; const re=/(\d+)%/g; let m; const txt=String(s).replace(/<[^>]+>/g,' '); while((m=re.exec(txt))){ const tail=txt.slice(m.index, m.index+60); if(!/n=|\(n |n \d|sessions\)|n\s*\d+\/\d+/.test(tail)) out.push(tail.slice(0,40)); } return out; };

// ---- 1 · sweepScan: the corpus definition, on synthetic bars ------------------------------------
{
  const g={ SWEEP_RECLAIM_MAX:30, SWEEP_IB_BARS:60 };
  const scan=build(g,['sweepScan'],'return sweepScan;')(g);
  const bar=(o,h,l,c)=>[0,o,h,l,c,1];
  const flush=[bar(101,102,100.5,101), bar(101,101,95,96)].concat(Array.from({length:7},()=>bar(96,97,90,96))).concat([bar(96,101,96,100.5), bar(100.5,102,100,101)]);
  let e=scan(flush,100,true,0);
  ok(e && e.status==='reclaimed' && e.sweep===1 && e.reclaim===9 && e.depth===10 && e.speed===8,'1a a flush: swept on bar 1, back inside on bar 9, 10 pts deep, speed 8',e);
  const poke=[bar(101,102,100.5,101), bar(101,101,99.5,100.2), bar(100.2,101,100,100.8)];
  e=scan(poke,100,true,0);
  ok(e && e.status==='reclaimed' && e.speed===1 && Math.abs(e.depth-0.5)<1e-9,'1b a poke: one bar through, next close back inside — speed 1, depth 0.5',e);
  ok(scan([bar(101,102,100,101), bar(101,102,100,101)],100,true,0)===null,'1c a low EXACTLY at the level is not a sweep (strictly through)');
  const acc=[bar(101,102,100.5,101)].concat(Array.from({length:36},()=>bar(99,99.5,98,99)));
  e=scan(acc,100,true,0);
  ok(e && e.status==='accepted' && e.reclaim===null,'1d no reclaim in 30 bars with 36 elapsed -> ACCEPTED',e);
  e=scan([bar(101,102,100.5,101)].concat(Array.from({length:10},()=>bar(99,99.5,98,99))),100,true,0);
  ok(e && e.status==='pending','1e no reclaim yet, only 10 bars elapsed -> PENDING',e);
  const hi=[bar(99,99.5,98,99), bar(99,104,99,103), bar(103,106,102,103), bar(103,104,99,99.5)];
  e=scan(hi,100,false,0);
  ok(e && e.status==='reclaimed' && e.ext===106 && e.depth===6 && e.speed===2,'1f the high side mirrors: extremum 106, depth 6',e);
  ok(scan(flush,100,true,5)===null || scan(flush,100,true,5).sweep>=5,'1g the start index is honoured (IB sweeps begin after the IB)');
}

// ---- 2 · statsRead: the sweep against the table, the node clause, the alignment -----------------
{
  const mk=(evs, defl, W_, night)=>{
    const g={ PAL, RATE_MIN_N:15, g3esc:esc, sweepsLoad:()=>W_, sweepsBookLoad:()=>null, bookLevelsNow:()=>({ walls:[], king:null, top5:[] }), tapZoneEs:()=>5, sweepEventsToday:()=>evs, recorderLoad:()=>({}), recorderDay:()=>({ defl:{ SPY:defl } }), ANALYSIS_NIGHTLY:night||null };
    g.LEVEL_TIER=g.LEVEL_TIER||new Function(src.slice(src.indexOf('var LEVEL_TIER='), src.indexOf('function levelTier('))+' return LEVEL_TIER;')(); return build(g,['levelTier','rateTxt','pctOf','ctrlTxt','statsRead'],'return statsRead("SPY");')(g);
  };
  const deepSlowEarly={ level:'ONL', side:'LOD', px:7660, at:'08:41', atBar:11, epoch:1700000000, bucket:'08:30-09:00', ext:7650, depth:10, speed:8, status:'reclaimed' };
  let R=mk([deepSlowEarly],[],Wslim);
  ok(R.lines.length===1 && R.aligned===3 && R.of===3,'2a early + deep + slow -> 3 of 3 measured conditions favour the sweep',{a:R.aligned,of:R.of});
  const t=R.lines[0].txt;
  ok(/EARLY \(08:30-09:00\)/.test(t) && /DEEP: flushes past 8 pts printed it 40% \(n=86\)/.test(t) && /SLOW reclaim: 6–30 bars printed it 40% \(n=90\)/.test(t),'2b each condition quotes ITS rate with n from the table',t.slice(0,300));
  ok(/by name: ONL 29% \(n=113\) vs 28% control \(n=284 sessions\) — the level’s name adds nothing/.test(t),'2c the level by name, against the fresh-low control, says the name adds nothing',t.slice(-200));
  // two levels swept by the same wick are ONE line, and the deeper excursion is the side's candidate
  const pdl={ level:'PDL', side:'LOD', px:7663, at:'08:41', atBar:11, epoch:1700000000, bucket:'08:30-09:00', ext:7650, depth:13, speed:9, status:'reclaimed' };
  const later={ level:'IBL', side:'LOD', px:7655, at:'10:40', atBar:130, epoch:1700007000, bucket:'10:00-11:30', ext:7652, depth:3, speed:2, status:'reclaimed' };
  const hod={ level:'ONH', side:'HOD', px:7700, at:'09:10', atBar:40, epoch:1700002400, bucket:'09:00-10:00', ext:7703, depth:3, speed:1, status:'reclaimed' };
  R=mk([deepSlowEarly,pdl,later,hod],[],Wslim);
  ok(R.lines.filter(l=>l.kind==='sweep').length===2 && /^ONL \+ PDL swept 08:41 · 10\.00 pts through ONL · back inside in 9 bars/.test(R.lines[0].head) && /by name: ONL 29% \(n=113\)[^·]*· PDL 23% \(n=87\)/.test(R.lines[0].txt) && /^ONH swept 09:10/.test(R.lines[1].head),'2c2 one line per excursion, one excursion per side: ONL+PDL merged (deepest wins over the later IBL poke), ONH on the other side',R.lines.map(l=>l.head));
  ok(/at NOTHING/.test(R.lines[0].node) && R.lines[0].decides==='SIZE','2d no deflection within 10 min -> "at NOTHING"');
  ok(bareP(R.lines[0].head+' '+t+' '+R.lines[0].node).length===0,'2e no bare % anywhere in the line',bareP(t));
  const poke={ level:'PDL', side:'LOD', px:7660, at:'10:12', atBar:102, epoch:1700000000, bucket:'10:00-11:30', ext:7658.5, depth:1.5, speed:2, status:'reclaimed' };
  R=mk([poke],[{ t:1700000000*1000+4*60*1000, strike:7660, name:'floor', cont:null }],Wslim);
  ok(R.aligned===0 && R.of===3 && /SHALLOW: pokes of ≤ 3 pts printed it 14% \(n=228\) — NOT the extreme 86% \(n=228\) of the time/.test(R.lines[0].txt),'2f a shallow quick late poke -> 0 of 3, and says the poke is NOT the low 86% (n=228)',R.lines[0].txt.slice(0,200));
  ok(/a deflection was recorded 4 min from the sweep at 7660 \(pending\)/.test(R.lines[0].node) && /UNMEASURED \(H6/.test(R.lines[0].node),'2g a deflection within 10 min -> the node clause (v15.56: the book-now check comes first), and with no book table the rate says UNMEASURED (H6)',R.lines[0].node);
  const accepted={ level:'PDH', side:'HOD', px:7700, at:'09:05', atBar:35, epoch:1700000000, bucket:'09:00-10:00', ext:7712, depth:12, speed:null, status:'accepted' };
  R=mk([accepted],[],Wslim);
  ok(/the level BROKE\. PDH breaks on first touch 34% \(n=170\)/.test(R.lines[0].txt) && R.lines[0].decides==='STOP','2h an accepted sweep says the level BROKE with its first-touch break rate and n',R.lines[0].txt);
  R=mk([deepSlowEarly],[],null);
  ok(/has not been fetched — no rate is quoted/.test(R.lines[0].txt) && R.of===0,'2i no table fetched -> no rate is quoted, nothing counted');
  R=mk([],[],Wslim);
  ok(R.lines.length===1 && /No level has been swept/.test(R.lines[0].txt) && R.lines[0].decides==='WAIT','2j nothing swept -> WAIT, one honest line');
  R=mk([], [{ t:1, strike:7681, name:'ceiling', cont:1, key:'k' }], Wslim, { hypotheses:[{ id:'H2', verdict:'thin', n:7, minN:30 }] });
  ok(R.lines.some(l=>l.kind==='register' && /7681/.test(l.txt) && /THIN · n 7\/30/.test(l.txt) && /73% \(n=22\) vs 47% \(n=70\)/.test(l.txt)),'2k the register line names the last deflection and H2’s state from the nightly, exploratory numbers with n',R.lines.map(l=>l.txt));
  const pend={ level:'ONL', side:'LOD', px:1, at:'08:31', atBar:1, epoch:1, bucket:'08:30-09:00', ext:0, depth:1, speed:null, status:'pending' };
  R=mk([pend],[],Wslim);
  ok(R.lines.length===1 && /ONL is being tested NOW — swept 08:31/.test(R.lines[0].txt) && R.lines[0].decides==='WAIT' && R.of===0,'2l a PENDING sweep is named as being tested now, and nothing is counted yet',R.lines[0]);
}

// ---- 3 · TRACK: requests ------------------------------------------------------------------------
{
  const store={};
  const g={ localStorage:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{ store[k]=String(v); } }, REQUESTS_KEY:'gpts_requests_v1', ctTodayStr:()=>'2026-09-03', studiesFlat:()=>[{ id:'K7.1', status:'OPEN', req:'RX' }], STUDY_STATUS_COL:{ OPEN:'#6c7889' } };
  const T=build(g,['requestsLoad','requestsSave','requestsAdd','requestsRemove','requestStatus','requestsExport'],'return { load:requestsLoad, add:requestsAdd, remove:requestsRemove, status:requestStatus, exp:requestsExport };')(g);
  ok(T.add('K','')===null && T.add('K','ab')===null && T.load().length===0,'3a empty / too short is refused');
  const r=T.add('K','  ONL sweep   into a top-5 node before 09:30  ');
  ok(r && r.subj==='K' && r.text==='ONL sweep into a top-5 node before 09:30' && r.status==='NEW' && r.date==='2026-09-03' && /^R/.test(r.id),'3b a request is trimmed, dated, stamped NEW, given an id',r);
  ok(T.add('K','ONL sweep into a top-5 node before 09:30')===null && T.load().length===1,'3c the same text under the same subject is not added twice');
  ok(/NEW — rides in the next Save/.test(T.status(r).label),'3d status: NEW until exported');
  const ex1=T.exp('2026-09-03');
  ok(ex1.length===1 && ex1[0].id===r.id && ex1[0].text===r.text && !('exported' in ex1[0]),'3e the export carries id/subj/text/date and stamps the record as exported',ex1[0]);
  ok(/IN THE EXPORT · 2026-09-03/.test(T.status(T.load()[0]).label),'3f status after export: in the export, the nightly reads it');
  ok(/STUDIED as K7\.1 · OPEN/.test(T.status({ id:'RX' }).label),'3g a study row carrying req:<id> reports STUDIED as <study id>');
  T.remove(r.id); ok(T.load().length===0,'3h remove works');
}

// ---- 4 · the registry as data ----------------------------------------------------------------------
{
  const S=JSON.parse(fs.readFileSync('learning/studies.json','utf8'));
  const flat=[]; S.subjects.forEach(sj=>sj.subsections.forEach(ss=>ss.studies.forEach(x=>flat.push(Object.assign({subj:sj.key,sub:ss.key},x)))));
  ok(S.schema===1 && S.subjects.length===7 && S.subjects.map(s=>s.key).join('')==='KSDFPHX','4a seven subjects, K S D F P H X',S.subjects.map(s=>s.key));
  const ids=flat.map(x=>x.id); ok(new Set(ids).size===ids.length && ids.length>=170,'4b study ids are unique, '+ids.length+' studies');
  const VOC=['SHIPPED','READ','READ NEXT','THIN','OPEN','REFUSED','REGISTERED','BLOCKED','DRAFT'];
  ok(flat.every(x=>VOC.indexOf(x.status)>=0),'4c every status is in the vocabulary',flat.filter(x=>VOC.indexOf(x.status)<0).map(x=>x.id));
  const bad=flat.filter(x=>x.result && /\d%/.test(x.result) && !/n=/.test(x.result));
  ok(bad.length===0,'4d every result with a % carries an n',bad.map(x=>x.id));
  ok(flat.every(x=>x.decides && /SIZE|SIDE|TARGET|STOP|SKIP|TIME|LEVEL|WAIT/.test(x.decides)),'4e every study says what it decides at the tap');
  const seed=new Function(src.slice(src.indexOf('var STUDIES_SEED='), src.indexOf('var STUDY_STATUS_COL='))+' return STUDIES_SEED;')();
  const seedSubs=[]; seed.subjects.forEach(sj=>sj.subsections.forEach(ss=>seedSubs.push(ss.key)));
  const fileSubs=[]; S.subjects.forEach(sj=>sj.subsections.forEach(ss=>fileSubs.push(ss.key)));
  ok(seedSubs.join(',')===fileSubs.join(',') && seed.subjects.map(s=>s.key).join('')===S.subjects.map(s=>s.key).join(''),'4f the panel’s seed and the file agree on every subject and subsection key — one registry, not two',{seed:seedSubs.length,file:fileSubs.length,keys:seed.subjects.map(s=>s.key).join('')});
  const h2=flat.filter(x=>x.sub==='H2'); ok(h2.length>=15 && h2.some(x=>/PDC/.test(x.q)) && h2.some(x=>/POC \/ VAH \/ VAL/.test(x.q)) && h2.some(x=>/Pre-market/.test(x.q)) && h2.some(x=>/WEEK/.test(x.q)) && h2.some(x=>/Opening range/.test(x.q)),'4g the sweeps subsection covers PDC, the profile, pre-market, the prior week and the opening range ('+h2.length+' studies)');
  ok(flat.some(x=>x.id==='H6.5' && /LID/.test(x.q)),'4h his example — the lid after a big-node rejection from the LOD — is a study (H6.5)');
  // SWEEPS.json: the lookup the panel reads
  ok(W.lookup && W.lookup.level.ONL.n===113 && Math.round(100*W.lookup.level.ONL.rate)===29 && W.lookup.depth.deep.n===86 && W.lookup.speed.flush.n===90 && W.lookup.clock['08:30-09:00'].n===180,'4i SWEEPS.json carries the lookup with the numbers the findings quote',W.lookup&&W.lookup.level.ONL);
  ok(['PDC-','PDC+','VAL','VAH','POC-','POC+','PML','PMH','PWL','PWH','OR5L','OR15H'].every(k=>W.lookup.level[k] && W.lookup.level[k].n>0),'4j the extended level set is in the lookup');
  // the register carries H6 / H7, written with prediction and refutation
  const R=JSON.parse(fs.readFileSync('learning/register.json','utf8'));
  const h6=R.hypotheses.find(h=>h.id==='H6'), h7=R.hypotheses.find(h=>h.id==='H7');
  ok(h6 && h6.blocked && h6.pick==='sweepNode' && h6.judgedBy==='nightly' && /> 40%/.test(h6.predict) && /<= 30%/.test(h6.refuteIf),'4k H6 (sweep × node) is in the register, blocked on the tap record, judged by the nightly');
  ok(h7 && h7.pick==='sweepEarly' && h7.since==='2026-08-21' && /> 24%/.test(h7.predict) && /<= 18%/.test(h7.refuteIf),'4l H7 (the early sweep) is in the register, read only on sessions after the corpus');
  // the seed pins to the file
  const seedR=new Function(exVar('PREREG_SEED')+' return PREREG_SEED;')();
  ok(seedR.length===R.hypotheses.length && seedR.every((h,i)=>R.hypotheses[i].id===h.id && R.hypotheses[i].pick===h.pick && R.hypotheses[i].minN===h.minN && !!R.hypotheses[i].blocked===!!h.blocked),'4m the panel seed and register.json agree on all '+seedR.length+' rows');
  // the nightly judges the sweep rows
  const r=cp.spawnSync('python3',['tools/nightly/run.py','--selftest'],{encoding:'utf8'});
  ok(r.status===0 && /H6\s+THIN.*sweep-at-a-node events/.test(r.stdout) && /H7\s+THIN.*after 2026-08-21/.test(r.stdout) && /planted effect found: True/.test(r.stdout),'4n run.py judges H6 (thin, from the book table — v15.56) and H7 (thin, sessions after 2026-08-21) and still finds the planted effect',(r.stdout||'').split('\n').filter(l=>/H6|H7|SELFTEST/.test(l)).join(' | '));
  const rq=JSON.parse(fs.readFileSync('learning/requests.json','utf8'));
  ok(rq.schema===1 && Array.isArray(rq.requests),'4o learning/requests.json exists for the nightly to append to');
}

// ---- 5 · the dashboard table, the gate text, the loop strip ------------------------------------
{
  const mk=(night)=>{ const g={ PAL, g3esc:esc, RULE_UNLOCK_N:20, ANALYSIS_NIGHTLY:night, featGated:(k)=>(k==='dir'?{gated:false}:(k==='node'?{gated:true,why:'CANNOT DISCRIMINATE'}:{gated:false,thin:true})), ruleLocalRate:()=>({effN:2}), rulesLoad:()=>({ rules:{ 'dir.A':{}, 'kill.tap3':{}, 'x':{} } }), ruleTier:(id)=>(id==='x'?'📊':'⚖') };
    return build(g,['gateStateTxt','rulesTierCounts','dashboardRulesHtml','panSection','panNote','panRow'],'return { html:dashboardRulesHtml("SPY"), tc:rulesTierCounts(), g:[gateStateTxt("dir").txt,gateStateTxt("node").txt,gateStateTxt("decision").txt] };')(g); };
  let D=mk(null);
  ok(/kill\.negGammaWide[\s\S]*?FLAG/.test(D.html) && /contradicts H3/.test(D.html),'5a with no verdict on H3, kill.negGammaWide is FLAGGED against the registered null');
  D=mk({ hypotheses:[{ id:'H3', verdict:'refused' }] });
  ok(/kill\.negGammaWide[\s\S]*?STANDS/.test(D.html) && !/>FLAG</.test(D.html),'5b when H3 is REFUSED the rule STANDS');
  ok(D.g[0]==='CLEAR' && /GATED — CANNOT DISCRIMINATE/.test(D.g[1]) && D.g[2]==='⛔ until 30/band','5c gate text: clear / gated with why / thin',D.g);
  ok(D.tc.total===3 && D.tc.earned===1 && D.tc.hand===2 && /3 rules on the ladder, 1 earned/.test(D.html),'5d rule tiers are counted from the live rules, not typed',D.tc);
  ok(bareP(D.html).length===0,'5e no bare % in the dashboard table',bareP(D.html));
}

// ---- 6 · BOTH TABS AND THE FACE, RENDERED IN JSDOM WITH THE REAL SCRIPT ----------------------------
{
  const { JSDOM }=require('jsdom');
  const dom=new JSDOM('<!doctype html><html><body></body></html>',{ url:'https://app.skylit.ai/atlas', pretendToBeVisual:true });
  const win=dom.window;
  win.matchMedia=win.matchMedia||(q=>({matches:false,media:q,addListener(){},removeListener(){},addEventListener(){},removeEventListener(){}}));
  win.requestAnimationFrame=cb=>0; win.cancelAnimationFrame=()=>{};
  const store={};
  Object.defineProperty(win,'localStorage',{ value:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{store[k]=String(v);}, removeItem:k=>{delete store[k];}, clear:()=>{for(const k in store)delete store[k];}, key:i=>Object.keys(store)[i]||null, get length(){return Object.keys(store).length;} }, configurable:true });
  win.indexedDB=undefined; win.setInterval=()=>0; win.clearInterval=()=>{}; win.setTimeout=()=>0; win.clearTimeout=()=>{}; win.fetch=()=>new Promise(()=>{});
  store['gpts_studies_v1']=fs.readFileSync('learning/studies.json','utf8');
  store['gpts_sweeps_v1']=JSON.stringify(Wslim);
  store['gpts_requests_v1']=JSON.stringify([{ id:'Rtest1', subj:'H', text:'ONL sweep into a top-5 node before 09:30', date:'2026-09-03', t:1, status:'NEW' }]);
  const i0=src.indexOf('(function(){'), i1=src.lastIndexOf('})();');
  const ctx=vm.createContext(win); win.window=win;
  let loadErr=null; try{ vm.runInContext(src.slice(i0+'(function(){'.length, i1), ctx, { filename:'gex.user.js' }); }catch(e){ loadErr=e; }
  ok(!loadErr,'6a the script loads in jsdom',loadErr&&loadErr.message);
  const run=code=>vm.runInContext(code, ctx);
  run('typeof buildPanel==="function" ? buildPanel() : (typeof boot==="function" ? boot() : 0)');
  run('RENDER_ERRS.length=0'); run('render()');
  const face=run('elBody ? elBody.innerHTML : ""');
  const errs0=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
  ok(errs0.length===0,'6b the face renders with nothing swallowed',errs0.map(e=>(e.where||e.w)+':'+(e.msg||e.m)));
  // Analysis
  run('RENDER_ERRS.length=0'); run('__gptsDebug.showAnalysis(true)');
  let html=run('elBody ? elBody.innerHTML : ""');
  const errsA=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
  ok(errsA.length===0,'6c the Analysis tab renders with nothing swallowed',errsA.map(e=>(e.where||e.w)+':'+(e.msg||e.m)));
  ok(['K KINGS','S SETUPS','D DIRECTION','F DEFLECTION MECHANICS','P PULLBACK DEFLECTIONS','H HOD / LOD','X CONTEXT'].every(k=>html.indexOf(esc(k))>=0),'6d the subject strip shows all seven subjects');
  ok(/<span class="n">H2<\/span><span class="t">SWEEPS — the levels that get run before the turn/.test(html) && /ONL → LOD/.test(html) && /29% \(n=113\)/.test(html) && /28% \(n=284\)/.test(html),'6e H is the default subject: H2 carries the sweep table with ONL 29% (n=113) and the control 28% (n=284)');
  ok(/id="gpts-track-H"/.test(html) && /TRACK SOMETHING UNDER H/.test(html) && /Rtest1/.test(html) && /NEW — rides in the next Save/.test(html),'6f the TRACK field and the stored request render under the subject');
  ok(/<span class="n">H1<\/span><span class="t">Is the extreme in/.test(html) && /TODAY’S EVIDENCE · LIVE/.test(html),'6g H1 carries today’s live HOD/LOD evidence under its rows');
  const sweepBlock=html.slice(html.indexOf('THE SWEEP TABLE'), html.indexOf('THE SWEEP TABLE')+12000);
  ok(bareP(sweepBlock).length===0,'6h no bare % in the sweep table',bareP(sweepBlock).slice(0,4));
  run('__gptsDebug.showSubject("K")'); html=run('elBody ? elBody.innerHTML : ""');
  ok(/<span class="n">K1<\/span><span class="t">King deflections by book/.test(html) && /K1\.3/.test(html) && /437 kingRoll records, never read/.test(html) && /id="gpts-track-K"/.test(html),'6i switching to K shows its subsections, rows (a subsection with a result opens by default), result lines and its own TRACK field');
  ok(/<span class="n">K5<\/span><span class="t">King quality/.test(html) && !/K5\.1/.test(html),'6i2 a subsection with nothing measured yet stays folded (its count is in the header)');
  // the Add button path: value in the input -> requestsAdd(subj) reads and clears it
  run('document.getElementById("gpts-track-K").value="does the QQQ King lead the SPX King?"; __gptsDebug.requestsAdd("K");');
  const reqs=JSON.parse(store['gpts_requests_v1']);
  ok(reqs.length===2 && reqs[1].subj==='K' && /QQQ King lead/.test(reqs[1].text),'6j + Add stores the request under the subject',reqs.map(r=>r.subj));
  html=run('elBody ? elBody.innerHTML : ""');
  ok(/QQQ King lead the SPX King/.test(html),'6k …and it renders in the list');
  // Testing
  run('RENDER_ERRS.length=0'); run('__gptsDebug.showTesting(true)');
  html=run('elBody ? elBody.innerHTML : ""');
  const errsT=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
  ok(errsT.length===0,'6l the Testing tab renders with nothing swallowed',errsT.map(e=>(e.where||e.w)+':'+(e.msg||e.m)));
  const order=['ANALYSIS','TRACKED','REGISTER','GATE','DASHBOARD','NIGHTLY'].map(k=>html.indexOf('<b>'+k+'</b>')).concat([['①','THE REGISTER'],['②','THE GATE'],['③','ON THE DASHBOARD'],['④','THE RECORD'],['⑤','THE NIGHTLY'],['⑥','THE SUITE']].map(p=>html.indexOf('>'+p[0]+'</span><span class="t">'+p[1])));
  ok(order.every(i=>i>=0) && order.every((v,i)=>i===0||v>order[i-1]),'6m the loop strip then ①…⑥ in loop order (v15.62: the mockup’s markup)',order);
  ok(/H6<\/b><\/td><td[^>]*>H · H2\.7/.test(html) && /H7<\/b><\/td><td[^>]*>H · H2\.8/.test(html) && /judged by the nightly/.test(html),'6n the register shows H6/H7 with their study ids, judged by the nightly');
  ok(/kill\.negGammaWide/.test(html) && /READ-NEXT QUEUE/.test(html) && /K1\.3/.test(html),'6o ③ carries the flag row, ⑤ carries the read-next queue');
  ok(/2 requests/.test(html) && /2 not yet exported/.test(html),'6p the loop strip counts TRACK requests and how many are not yet exported');
  // the export carries them (buildDayExport refuses with no bars; requestsExport is what it calls)
  const exp=JSON.parse(run('JSON.stringify(requestsExport("2026-09-03"))'));
  ok(exp.length===2 && exp.every(r=>r.id&&r.subj&&r.text),'6q requestsExport returns the rows the day file carries');
  ok(/requests:\(function\(\)\{ try\{ return requestsExport\(dk\); \}/.test(src),'6r buildDayExport writes `requests`');
  // the face: the read from the stats (no courier bars in jsdom -> the honest WAIT line)
  run('__gptsDebug.showDashboard()'); html=run('elBody ? elBody.innerHTML : ""');
  ok(/(the read · from the stats|<em>SWEPT<\/em>)/.test(html) && /(No level has been swept|no key level swept yet today)/.test(html),'6s ⓪a carries the read from the stats — v15.63: the SWEPT line — honest with no courier bars');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
