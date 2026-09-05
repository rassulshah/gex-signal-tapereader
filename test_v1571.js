// ============================================================================================
// test_v1571.js — (v15.71) THE SAVE RUNS ITSELF. Operator, 2026-09-04: "the next step is to automatically have the
//   application trigger the save button instead of me clicking it. can the application self trigger using a time. for
//   example, if the save button has not been pressed and the time is 5pm or later, trigger it. if the save button for the
//   previous day has not been triggered and the time is during non market hours, trigger it." — "instead of 5pm can you
//   just modify so it is after market hours.. besides this i approve .. build"
//   Three rules through one writer, on the panel's clock (Chicago): rule 1+2 today after the close until confirmed;
//   rule 3 every earlier day with bars and no file, outside market hours, write-if-absent; SAVED = confirmed in the
//   folder (the silent download is gone); a timer asks for the grant but never requests it — the click carries it.
// ============================================================================================
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,400):''));} };
const src=fs.readFileSync(process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js','utf8');
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index), d=0, j=i; for(;j<src.length;j++){ const ch=src[j]; if(ch==='{')d++; else if(ch==='}'){ d--; if(d===0) break; } } return src.slice(m.index,j+1); }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const sleep=(ms)=>new Promise(r=>setTimeout(r,ms));
const VARS=['AUTOSAVE_RETRY_MS','AUTOSAVE_LATE_DAYS','AUTOSAVE','DAY_WRITTEN','DATA_DIR_H','REPO_AUTO_TRIED','REPO_LAST_SAVE','TAPE_LAST_WRITE'];   // SAVED_TODAY's line carries a comment, declared by hand below
const FNS=['ctMarketHours','ctAfterClose','dayWrittenMark','autosaveBlock','repoDays','repoAutoExportTick','repoLateSweep','repoLateSaveTick','autosavePermTick','autosaveTick','autosaveBoot','repoClickExport','autosaveState','autosaveTip','repoExportDay'];
// a harness: the globals the module reads come in as `g`; the module's own vars and functions are the real source
const build=(g,tail,fns)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+VARS.map(exVar).join('\n')+'\nvar SAVED_TODAY=null;\n'+(fns||FNS).map(ex).join('\n')+'\nvar window={__gptsDebug:{}};\n'+(tail||''));
// a Chicago clock: weekday (1–5) or weekend, seconds of day
const clock=(dow, sec)=>({ ctNow:()=>({ getDay:()=>dow, getTime:()=>1e12, setDate(){}, getDate:()=>4 }), ctNowSecOfDay:()=>sec });
const H=(h,m)=>h*3600+(m||0)*60;

ok(/@version\s+15\.71/.test(src) && /var GPTS_VERSION='15\.71';/.test(src),'0a v15.71 in both spots');

// ---- 1 · the clock -----------------------------------------------------------------------------------------------------------
{
  const f=(dow,sec)=>{ const g=clock(dow,sec); return build(g,'return { mh:ctMarketHours(), ac:ctAfterClose() };',['ctMarketHours','ctAfterClose'])(g); };
  let r=f(3,H(10)); ok(r.mh===true && r.ac===false,'1a Wednesday 10:00 CT: market hours, not after the close',r);
  r=f(3,H(15,0)+30); ok(r.mh===false && r.ac===false,'1b 15:00:30: the session is over, the close rule waits for 15:01',r);
  r=f(3,H(15,1)); ok(r.mh===false && r.ac===true,'1c 15:01: after the close',r);
  r=f(3,H(16,30)); ok(r.ac===true,'1d 16:30: STILL after the close — the v10.44 auto-export gave up at 16:00, this one never does',r);
  r=f(5,H(21)); ok(r.ac===true && r.mh===false,'1e Friday 21:00: after the close, not market hours',r);
  r=f(6,H(15,30)); ok(r.ac===false && r.mh===false,'1f Saturday: never "after the close" (no session), never market hours',r);
  r=f(0,H(10)); ok(r.ac===false && r.mh===false,'1g Sunday 10:00: not market hours',r);
  r=f(1,H(8,29)); ok(r.mh===false,'1h Monday 08:29: pre-market is outside market hours (rule 3 may run)',r);
  r=f(1,H(8,30)); ok(r.mh===true,'1i Monday 08:30: the session',r);
  const gb={ ctNow:()=>{ throw new Error('no clock'); }, ctNowSecOfDay:()=>0 }; const broken=build(gb,'return { mh:ctMarketHours(), ac:ctAfterClose() };',['ctMarketHours','ctAfterClose'])(gb);
  ok(broken.mh===true && broken.ac===false,'1j a clock fault reads as market hours and not after the close — nothing writes on a broken clock',broken);
}

// ---- 2 · rule 1+2: today, after the close, until confirmed ---------------------------------------------------------------------
{
  let NOW=1e12; class D extends Date{ static now(){ return NOW; } }
  const calls=[]; let SAVED=false;
  const g=Object.assign({ Date:D, TODAY:'2026-09-08', saveState:()=>({code:SAVED?'saved':'none'}), repoExportDay:(d,s,by)=>{ calls.push([d,s,by]); } }, clock(2,H(15,1)));
  const f=build(g,'return { tick:repoAutoExportTick, A:()=>AUTOSAVE, tried:()=>REPO_AUTO_TRIED };',['ctAfterClose','repoAutoExportTick'])(g);
  f.tick(); ok(calls.length===1 && calls[0][0]==='2026-09-08' && calls[0][1]===true && calls[0][2]==='auto','2a 15:01, not saved: one silent export of today, by auto',calls);
  ok(f.tried()==='2026-09-08' && f.A().tries===1,'2b the attempt is recorded (REPO_AUTO_TRIED — the pending state reads it) and counted',[f.tried(),f.A().tries]);
  NOW+=5*60000; f.tick(); ok(calls.length===1,'2c five minutes later: no second attempt yet (10-minute retry)',calls.length);
  NOW+=6*60000; f.tick(); ok(calls.length===2 && f.A().tries===2,'2d eleven minutes after the first: the retry — it keeps trying until the file is confirmed',[calls.length,f.A().tries]);
  SAVED=true; NOW+=11*60000; f.tick(); ok(calls.length===2,'2e saved (confirmed in the folder): no further attempt',calls.length);
  // before the close and on a weekend: nothing
  const g2=Object.assign({}, g, clock(2,H(14,59))); const calls2=[]; g2.repoExportDay=(d,s,by)=>calls2.push(d); g2.saveState=()=>({code:'none'});
  build(g2,'repoAutoExportTick();',['ctAfterClose','repoAutoExportTick'])(g2); ok(calls2.length===0,'2f 14:59: the session is not over — nothing');
  const g3=Object.assign({}, g, clock(6,H(15,30))); const calls3=[]; g3.repoExportDay=(d,s,by)=>calls3.push(d); g3.saveState=()=>({code:'none'});
  build(g3,'repoAutoExportTick();',['ctAfterClose','repoAutoExportTick'])(g3); ok(calls3.length===0,'2g Saturday 15:30: no session, no write (the 08-29 / 08-30 weekend files are what this stops)');
  const g4=Object.assign({}, g, clock(4,H(20,0))); const calls4=[]; g4.repoExportDay=(d,s,by)=>calls4.push(d); g4.saveState=()=>({code:'download'});
  build(g4,'repoAutoExportTick();',['ctAfterClose','repoAutoExportTick'])(g4); ok(calls4.length===1,'2h a day exported as a DOWNLOAD only is not saved — the rule writes it into the folder (the download never reaches git)');
}

// ---- 3 · the one writer ----------------------------------------------------------------------------------------------------------
const mkWriter=(o)=>{
  o=o||{};
  const files=o.files||{}; const written={}; const downloads=[]; const log={ perm:[], req:[], tape:[], kv:{}, ls:{}, pipe:[], renders:0 };
  const perm=()=>o.perm||'granted';
  const handle=(o.handle===null)?null:{
    getFileHandle:(nm,opt)=>{ if(!(opt&&opt.create) && !(nm in files) && !(nm in written)) return Promise.reject(new Error('NotFoundError')); return Promise.resolve({ createWritable:()=>Promise.resolve({ write:(t)=>{ if(o.writeFails) return Promise.reject(new Error('NotAllowedError')); written[nm]=t; return Promise.resolve(); }, close:()=>Promise.resolve() }) }); },
    queryPermission:()=>{ log.perm.push('q'); return Promise.resolve(perm()); },
    requestPermission:()=>{ log.req.push('r'); return Promise.resolve(o.grantOnRequest?'granted':perm()); }
  };
  const g={
    TODAY:o.today||'2026-09-08',
    repoDay:(d,cb)=>cb({ date:d, snaps:(o.bars===0)?{}:{ SPY:[{t:1},{t:2}] } }),
    buildDayExport:(d)=>(o.empty?{ schema:'x', date:d, empty:true, refused:'no bars' }:{ schema:'x', date:d, feat:{} }),
    rulesDoc:()=>({asOf:'2026-09-04'}), modelStamp:()=>'m', recorderLoad:()=>({days:{}}), VERSION_STR:()=>'15.71',
    tapeExportDay:(d,s)=>{ log.tape.push([d,s]); },
    repoKvGet:(k,cb)=>cb(k==='dataDir'?handle:null), repoKvSet:(k,v)=>{ log.kv[k]=v; },
    localStorage:{ setItem:(k,v)=>{ log.ls[k]=v; }, getItem:(k)=>log.ls[k]||null },
    pipeNoteSave:(d,how)=>{ log.pipe.push([d,how]); }, render:()=>{ log.renders++; },
    repoDownload:(n,t,s)=>{ downloads.push(n); }
  };
  const f=build(g,'return { w:repoExportDay, A:()=>AUTOSAVE, DW:()=>DAY_WRITTEN, RLS:()=>REPO_LAST_SAVE, ST:()=>SAVED_TODAY, setTape:(v)=>{ TAPE_LAST_WRITE=v; } };',['dayWrittenMark','autosaveBlock','repoExportDay'])(g);
  return { f, written, downloads, log };
};
{
  // (a) the auto path, granted
  let W=mkWriter(); let done=null; W.f.w('2026-09-08', true, 'auto', (ok_)=>{ done=ok_; });
  async function run(){
    await sleep(20);
    const p=JSON.parse(W.written['2026-09-08.json']||'{}');
    ok(p.writtenBy==='auto' && p.snaps && p.snaps.SPY && p.snaps.SPY.length===2 && !p.late,'3a the auto path writes the full day file with writtenBy auto and the IndexedDB snaps',Object.keys(p));
    ok(done===true && W.log.ls.gpts_last_export==='2026-09-08' && W.log.pipe[0][1]==='repo folder' && W.f.ST()==='2026-09-08','3b …and only the SUCCESS callback writes the evidence: gpts_last_export, the pipe note, SAVED_TODAY',[done,W.log.ls,W.log.pipe]);
    ok(W.f.DW()['2026-09-08'] && W.f.DW()['2026-09-08'].how==='repo folder' && W.f.DW()['2026-09-08'].by==='auto' && W.log.kv['dayWritten:2026-09-08'],'3c the day is marked written, in memory and in kv');
    ok(W.f.A().blocked===null && W.f.A().lastWrite && W.f.A().lastWrite.by==='auto' && W.downloads.length===0,'3d nothing blocked, nothing downloaded');
    // (b) the grant is missing, silent: NO download, blocked
    W=mkWriter({ perm:'prompt' }); done=null; W.f.w('2026-09-08', true, 'auto', (ok_)=>{ done=ok_; }); await sleep(20);
    ok(W.downloads.length===0 && Object.keys(W.written).length===0 && done===false,'3e silent + grant missing: NOTHING is written and NOTHING downloads (the v10.5x silent failure mode is gone)',[W.downloads,Object.keys(W.written)]);
    ok(W.f.A().blocked && W.f.A().blocked.why==='permission' && W.f.A().blocked.date==='2026-09-08' && W.f.A().perm==='prompt' && W.log.req.length===0,'3f …the rule is BLOCKED on the permission, and requestPermission was never called from the timer (v14.53)',W.f.A().blocked);
    ok(!W.log.ls.gpts_last_export && W.log.pipe.length===0,'3g …and no evidence was written');
    // (c) the click, grant missing → requestPermission inside the click path → granted → written
    W=mkWriter({ perm:'prompt', grantOnRequest:true }); done=null; W.f.w('2026-09-08', false, 'click', (ok_)=>{ done=ok_; }); await sleep(20);
    ok(W.log.req.length===1 && W.written['2026-09-08.json'] && JSON.parse(W.written['2026-09-08.json']).writtenBy==='click' && done===true && W.log.renders>=1,'3h the click asks for the permission and writes, writtenBy click, and renders',[W.log.req.length,done]);
    // (d) no folder picked: silent → blocked "no folder", no download; click → the legacy download
    W=mkWriter({ handle:null }); W.f.w('2026-09-08', true, 'auto'); await sleep(20);
    ok(W.downloads.length===0 && W.f.A().blocked && W.f.A().blocked.why==='no folder','3i no folder + silent: blocked "no folder", nothing downloads on its own',[W.downloads,W.f.A().blocked]);
    W=mkWriter({ handle:null }); W.f.w('2026-09-08', false, 'click'); await sleep(20);
    ok(W.downloads.length===1 && W.downloads[0]==='2026-09-08.json','3j no folder + his click: the download, as before (he asked for a file and gets one)',W.downloads);
    // (e) the refusal: no bars, on every path
    W=mkWriter({ empty:true, bars:0 }); done=null; W.f.w('2026-09-08', true, 'auto', (ok_)=>{ done=ok_; }); await sleep(20);
    ok(Object.keys(W.written).length===0 && W.downloads.length===0 && done===false && W.f.A().note && /no recorded bars/.test(W.f.A().note.why) && W.log.tape.length===0,'3k a day with no recorded bars is never written, never downloaded, and the tape is not touched — on the auto path',[Object.keys(W.written),W.f.A().note]);
    W=mkWriter({ empty:true, bars:0 }); W.f.w('2026-09-08', false, 'click'); await sleep(20);
    ok(Object.keys(W.written).length===0 && W.downloads.length===0 && W.f.RLS() && W.f.RLS().how==='refused','3l …and on the click: refused, and REPO_LAST_SAVE says so (saveState treats refused as no save)',[W.downloads,W.f.RLS()]);
    // (f) the localStorage day evicted but bars in IndexedDB: written, marked partial, no `empty`
    W=mkWriter({ empty:true }); W.f.w('2026-09-08', true, 'auto'); await sleep(20);
    const pp=JSON.parse(W.written['2026-09-08.json']||'{}');
    ok(pp.partial && !('empty' in pp) && !('refused' in pp) && pp.snaps.SPY.length===2,'3m the localStorage day evicted but bars in IndexedDB: written with `partial`, without the refusal fields',Object.keys(pp));
    // (g) a previous day (rule 3): today's evidence untouched, marked late
    W=mkWriter(); W.f.w('2026-09-05', true, 'late', ()=>{}); await sleep(20);
    const pl=JSON.parse(W.written['2026-09-05.json']||'{}');
    ok(pl.writtenBy==='late' && pl.late && /rule 3/.test(pl.late.why) && !W.log.ls.gpts_last_export && W.log.pipe.length===0 && W.f.ST()===null && W.f.DW()['2026-09-05'].by==='late','3n a previous day: writtenBy late, marked, and TODAY’s evidence (gpts_last_export · pipe · SAVED_TODAY) untouched',[pl.writtenBy,W.log.ls,W.f.ST()]);
    // (h) a retry does not rewrite a tape already written this session; a click always does
    W=mkWriter(); W.f.setTape({ day:'2026-09-08', how:'repo folder', t:1 }); W.f.w('2026-09-08', true, 'auto'); await sleep(20);
    ok(W.log.tape.length===0,'3o an auto retry after the tape was written this session does not rewrite the 2 MB tape (git history)',W.log.tape);
    W=mkWriter(); W.f.setTape({ day:'2026-09-08', how:'repo folder', t:1 }); W.f.w('2026-09-08', false, 'click'); await sleep(20);
    ok(W.log.tape.length===1 && W.log.tape[0][0]==='2026-09-08','3p his click writes the tape again',W.log.tape);
    // (i) the write itself fails, silently: blocked with the error, no download
    W=mkWriter({ writeFails:true }); W.f.w('2026-09-08', true, 'auto'); await sleep(20);
    ok(W.downloads.length===0 && W.f.A().blocked && W.f.A().blocked.why==='write failed','3q a failed silent write blocks with the error instead of downloading',W.f.A().blocked);
    W=mkWriter({ writeFails:true }); W.f.w('2026-09-08', false, 'click'); await sleep(20);
    ok(W.downloads.length===1,'3r a failed write on his click falls back to the download, as before',W.downloads);
    await part4();
  }
  run().catch(e=>{ fail++; console.log('FAIL 3 harness threw '+e.stack); finish(); });
}

// ---- 4 · rule 3: the earlier days, write-if-absent, outside market hours ----------------------------------------------------------
async function part4(){
  const mkSweep=(o)=>{
    o=o||{};
    const exported=[]; const marks={};
    const g=Object.assign({
      TODAY:'2026-09-08', ctTodayStr:()=>'2026-09-08', ctDateStr:(d)=>'2026-08-29',
      repoDays:(cb)=>cb(o.days||['2026-08-20','2026-09-03','2026-09-04','2026-09-05','2026-09-08']),
      repoKvSet:(k,v)=>{ marks[k]=v; }, repoOpen:(cb)=>cb(null),
      repoExportDay:(d,s,by,done)=>{ exported.push([d,s,by]); if(o.exportFails){ done(false); return; } DW[d]={how:'repo folder',by:by}; done(true); },
      tapeSweepUnwritten:(t,s)=>{ g._tape=[t,s]; }, render:()=>{ g._renders=(g._renders||0)+1; }
    }, clock(o.dow!=null?o.dow:2, o.sec!=null?o.sec:H(15,5)));
    const DW={};
    const files=o.files||{ '2026-09-04.json':1 };
    const h=(o.noFolder)?null:{ getFileHandle:(nm)=>((nm in files)?Promise.resolve({}):Promise.reject(new Error('NotFoundError'))), queryPermission:()=>Promise.resolve(o.perm||'granted') };
    const f=build(g,'DATA_DIR_H=__g._h; DAY_WRITTEN=__g._dw; return { sweep:repoLateSweep, tick:repoLateSaveTick, A:()=>AUTOSAVE, DW:()=>DAY_WRITTEN };',['ctMarketHours','dayWrittenMark','autosaveBlock','repoLateSweep','repoLateSaveTick'])(Object.assign(g,{_h:h,_dw:DW}));
    return { f, exported, marks, g, DW };
  };
  let S=mkSweep(); let r=null; S.f.sweep(true, (x)=>{ r=x; }); await sleep(30);
  ok(r && r.due===3 && S.exported.map(e=>e[0]).join()==='2026-09-03,2026-09-05' && S.exported.every(e=>e[1]===true && e[2]==='late'),'4a the sweep: 09-03 and 09-05 have bars and no file → written silently, by late; 09-04 has a file; 09-08 is today; 08-20 is past the look-back',[r,S.exported]);
  ok(S.DW['2026-09-04'] && S.DW['2026-09-04'].how==='found' && S.marks['dayWritten:2026-09-04'] && r.found.join()==='2026-09-04','4b a day whose file exists is marked "found" — write-if-absent: a good file is never overwritten',[S.DW['2026-09-04'],r.found]);
  ok(S.g._tape && S.g._tape[0]==='2026-09-08' && S.g._tape[1]===true && S.f.A().lateDue===0,'4c the tape’s own sweep rides along, and nothing is left due',[S.g._tape,S.f.A().lateDue]);
  S=mkSweep({ perm:'prompt' }); r=null; S.f.sweep(true, (x)=>{ r=x; }); await sleep(30);
  ok(S.exported.length===0 && r && r.blocked==='permission' && S.f.A().blocked && S.f.A().blocked.why==='permission' && S.f.A().lateDue===3,'4d the grant missing: nothing written, blocked on the permission, 3 days shown as due (the chip says +3)',[r,S.f.A().lateDue]);
  S=mkSweep({ noFolder:true }); r=null; S.f.sweep(true, (x)=>{ r=x; }); await sleep(30);
  ok(S.exported.length===0 && r && r.skipped==='no folder','4e no folder picked: the sweep skips',r);
  S=mkSweep(); S.DW['2026-09-03']={how:'repo folder'}; S.DW['2026-09-04']={how:'found'}; S.DW['2026-09-05']={how:'repo folder'}; r=null; S.f.sweep(true, (x)=>{ r=x; }); await sleep(30);
  ok(S.exported.length===0 && r.due===0,'4f days already marked written are not even looked at',r);
  // the tick: never inside market hours; throttled
  let NOW=5e6; class D extends Date{ static now(){ return NOW; } }
  S=mkSweep({ sec:H(10) }); S.g.Date=D; let f2=build(S.g,'DATA_DIR_H=__g._h; DAY_WRITTEN=__g._dw; return { tick:repoLateSaveTick, A:()=>AUTOSAVE };',['ctMarketHours','dayWrittenMark','autosaveBlock','repoLateSweep','repoLateSaveTick'])(S.g);
  f2.tick(); await sleep(30); ok(S.exported.length===0,'4g 10:00 on a weekday: rule 3 never runs inside the session',S.exported);
  S=mkSweep({ sec:H(7,30) }); S.g.Date=D; f2=build(S.g,'DATA_DIR_H=__g._h; DAY_WRITTEN=__g._dw; return { tick:repoLateSaveTick, A:()=>AUTOSAVE };',['ctMarketHours','dayWrittenMark','autosaveBlock','repoLateSweep','repoLateSaveTick'])(S.g);
  f2.tick(); await sleep(30); ok(S.exported.length===2,'4h 07:30 pre-market: the missed days are written before the open (so the nightly runs before the session)',S.exported.length);
  NOW+=60000; f2.tick(); await sleep(30); ok(S.exported.length===2,'4i …and not again a minute later (10-minute cadence)');
  S=mkSweep({ dow:0, sec:H(11) }); S.g.Date=D; f2=build(S.g,'DATA_DIR_H=__g._h; DAY_WRITTEN=__g._dw; return { tick:repoLateSaveTick };',['ctMarketHours','dayWrittenMark','autosaveBlock','repoLateSweep','repoLateSaveTick'])(S.g);
  f2.tick(); await sleep(30); ok(S.exported.length===2,'4j Sunday: non-market hours, the sweep runs',S.exported.length);
  await part5();
}

// ---- 5 · the click, the chip, the boot, the wiring --------------------------------------------------------------------------------------
async function part5(){
  // the click carries the permission INSIDE the gesture: requestPermission first, synchronously, then the export
  {
    const order=[]; const g={ TODAY:'2026-09-08', repoExportDay:(d,s,by)=>{ order.push('export:'+d+':'+s+':'+by); }, repoLateSweep:(s)=>{ order.push('sweep:'+s); }, _h:{ requestPermission:()=>{ order.push('request'); return Promise.resolve('granted'); } } };
    const f=build(g,'DATA_DIR_H=__g._h; return { click:repoClickExport, A:()=>AUTOSAVE };',['autosaveBlock','repoClickExport'])(g);
    f.click(); ok(order[0]==='request','5a the click calls requestPermission FIRST, synchronously inside the gesture (v14.78: an IndexedDB callback is too late)',order);
    await sleep(10); ok(order.join()==='request,export:2026-09-08:false:click,sweep:false' && f.A().perm==='granted','5b …then today (not silent, by click), then the earlier days',order);
    const g2={ TODAY:'2026-09-08', repoExportDay:(d,s,by)=>{ order.push('x'); }, repoLateSweep:()=>{} }; order.length=0;
    const f2=build(g2,'return { click:repoClickExport };',['autosaveBlock','repoClickExport'])(g2); f2.click(); ok(order.join()==='x','5c without a cached handle the click still exports (the legacy path picks the folder or downloads)');
    ok(/window\.__gptsRepoExport=function\(d\)\{ repoClickExport\(d\|\|TODAY\); \};/.test(src),'5d the footer 💾 goes through repoClickExport');
  }
  // the chip's state and hover
  {
    const mk=(o)=>{ const g=Object.assign({ TODAY:'2026-09-08', saveState:()=>({code:o.sv||'none', t:o.t||null}) }, clock(2, o.sec!=null?o.sec:H(10))); return build(g,'AUTOSAVE=Object.assign(AUTOSAVE, __g._A||{}); DATA_DIR_H=__g._h===undefined?{}:__g._h; return { st:autosaveState(), tip:autosaveTip(autosaveState()) };',['ctAfterClose','autosaveState','autosaveTip'])(Object.assign(g,{_A:o.A,_h:o.h})); };
    let r=mk({}); ok(r.st.code==='idle' && /^Is today’s data saved in the repo folder\? not yet — the session is still running/.test(r.tip),'5e 10:00, nothing yet: idle; the hover opens with the question',r);
    r=mk({ sv:'saved', t:1, A:{ lastWrite:{ by:'auto', date:'2026-09-08' } } }); ok(r.st.code==='saved' && /yes at .* — written by the panel, after the close/.test(r.tip),'5f saved by the panel: green, and the hover says who wrote it',r.tip.slice(0,120));
    r=mk({ sec:H(15,5), A:{ tries:2 } }); ok(r.st.code==='pending' && /the panel is writing it \(tried 2×\)/.test(r.tip),'5g after the close, not yet confirmed: pending, with the count',r);
    r=mk({ sec:H(15,5), A:{ blocked:{ why:'permission', date:'2026-09-08' } } }); ok(r.st.code==='due' && /ONE CLICK NEEDED/.test(r.tip) && /Allow on every visit/.test(r.tip),'5h blocked on the permission: DUE, and the hover names the one click and "Allow on every visit"',r.tip.slice(0,200));
    r=mk({ sec:H(7), A:{ blocked:{ why:'permission', date:'2026-09-05' }, lateDue:2 } }); ok(r.st.code==='due' && r.st.late===2 && /2 earlier days waiting too/.test(r.tip),'5i pre-market, two earlier days blocked: DUE with the count',r.st);
    r=mk({ sv:'saved', t:1, A:{ blocked:{ why:'permission', date:'2026-09-05' }, lateDue:1 } }); ok(r.st.code==='due','5j today saved but an earlier day blocked: still DUE — a day is missing from the record');
    r=mk({ sec:H(15,5), A:{ blocked:{ why:'no folder', date:'2026-09-08' } } }); ok(r.st.code==='due' && /click 📁 and pick the data folder/.test(r.tip),'5k no folder: DUE, the hover says pick it',r.tip.slice(0,160));
    r=mk({ A:{ perm:'prompt' } }); ok(r.st.code==='perm' && /needs one click today/.test(r.tip),'5l the session running but the grant already known missing: 💾! — said BEFORE the close',r);
    r=mk({ sec:H(15,5), A:{ note:{ date:'2026-09-08', why:'refused — no recorded bars' } } }); ok(r.st.code==='nodata' && /nothing was recorded today/.test(r.tip),'5m after the close with nothing recorded: nodata, not pending forever',r);
    r=mk({ sv:'saved', t:1, A:{ blocked:{ why:'permission', date:'2026-09-08' } } }); ok(r.st.code==='due','5n a block for today outranks a stale saved reading (the block is cleared by the success callback, so they cannot both be true after a write)');
    ok(/data-autosave="'\+asCode\+'"/.test(src) && /asTip=autosaveTip\(asv\)/.test(src) && /asLabel='💾'\+\(asCode==='due'\?' DUE':\(asCode==='perm'\?'!':''\)\)/.test(src),'5o the footer renders the chip: data-autosave, the label 💾 / 💾! / 💾 DUE(+n), the hover from autosaveTip');
    ok(/asCol=\(\{ saved:PAL\.longAccent, pending:PAL\.amber, due:'#f0616d', perm:PAL\.amber \}\)\[asCode\]\|\|PAL\.line/.test(src),'5p …green · amber · red · amber · plain');
  }
  // the permission tick asks, never requests; a grant won by the click un-blocks the rules at once
  {
    const g={ render:()=>{ g._r=(g._r||0)+1; } }; let NOW=1e6; class D extends Date{ static now(){ return NOW; } } g.Date=D;
    const f=build(g,'DATA_DIR_H={ queryPermission:()=>Promise.resolve(__g._p), requestPermission:()=>{ throw new Error("must never be called from a timer"); } }; AUTOSAVE.blocked={why:"permission",date:"2026-09-08"}; AUTOSAVE.lastTry=999; AUTOSAVE.lastLate=999; AUTOSAVE.perm="prompt"; return { tick:autosavePermTick, A:()=>AUTOSAVE };',['autosavePermTick'])(Object.assign(g,{_p:'granted'}));
    f.tick(); await sleep(10); ok(f.A().perm==='granted' && f.A().blocked===null && f.A().lastTry===0 && f.A().lastLate===0 && g._r===1,'5q the grant seen: the block clears, both rules are re-armed, the face re-renders',f.A());
    NOW+=60000; f.tick(); await sleep(10); ok(g._r===1,'5r …and the ask is throttled to the 10-minute cadence');
  }
  // the wiring
  {
    const tick=ex('tick'); const iA=tick.indexOf('autosaveTick()'), iG=tick.indexOf('if(!pastReset())');
    ok(iA>0 && iG>0 && iA<iG,'5s tick() runs autosaveTick BEFORE the 08:30 gate — rule 3 writes a missed day pre-market',[iA,iG]);
    ok(!/^\s*repoAutoExportTick\(\);/m.test(tick) && (tick.match(/try\{ autosaveTick\(\); \}/g)||[]).length===1,'5t the old direct call is gone; one call');
    ok(/function autosaveTick\(\)\{ try\{ autosavePermTick\(\); \}catch\(e0\)\{\} try\{ repoAutoExportTick\(\); \}catch\(e1\)\{\} try\{ repoLateSaveTick\(\); \}catch\(e2\)\{\} \}/.test(src),'5u autosaveTick = the permission ask, rule 1+2, rule 3 — each wrapped');
    ok(/autosaveBoot\(\);/.test(ex('boot')) && /repoKvGet\('dataDir', function\(h\)\{ DATA_DIR_H=h\|\|null;/.test(ex('autosaveBoot')) && /repoKvGet\('dayWritten:'\+d/.test(ex('autosaveBoot')),'5v boot caches the folder handle and the written-day marks');
    ok(/DATA_DIR_H=h; AUTOSAVE\.perm='granted'; AUTOSAVE\.blocked=null; AUTOSAVE\.lastTry=0; AUTOSAVE\.lastLate=0;/.test(ex('repoPickFolder')),'5w the 📁 pick caches the handle and re-arms the rules');
    ok(/S\.how!=='refused'/.test(ex('saveState')),'5x saveState: a refusal is not a save');
    ok(/openKeyCursor\(null,'nextunique'\)/.test(ex('repoDays')),'5y repoDays reads the unique dates off the snaps index without loading a record');
    ok(/AUTOSAVE_RETRY_MS=600000/.test(src) && /AUTOSAVE_LATE_DAYS=10/.test(src),'5z the cadence and the look-back are named constants');
    ok(/window\.__gptsDebug\.autosave=function\(\)/.test(src) && /window\.__gptsDebug\.lateSweep=function\(\)/.test(src),'5aa the probes: __gptsDebug.autosave() and .lateSweep()');
    ok(/not yet — today has not been written\. The panel writes it itself after the close/.test(src),'5ab the pipeline strip’s saved hover names the after-close write');
    const exf=fs.readFileSync('test_export_full.js','utf8'); ok(/4c exactly two writers of gpts_last_export/.test(exf),'5ac test_export_full still counts exactly two writers of the flag (the late path writes none)');
  }
  await part6();
}

// ---- 6 · the process, the plan, the record ----------------------------------------------------------------------------------------------
async function part6(){
  const P=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]);
  const nx=P.roadmap.filter(r=>r.status==='next');
  ok(nx.length===1 && nx[0].v==='15.71' && /THE SAVE RUNS ITSELF/.test(nx[0].title) && P.roadmap.some(r=>r.v==='15.70' && r.status==='shipped') && P.roadmap.some(r=>r.v==='15.72' && /candidate score/.test(r.title)),'6a the plan: v15.70 shipped, v15.71 this build, the score moved to v15.72',nx.map(x=>x.v));
  ok(/write-if-absent/.test(P.stages[1].what) && /the 💾 is the override/.test(P.stages[1].what) && /Allow on every visit/.test(P.stages[1].what),'6b stage ② says the panel writes the day itself, write-if-absent, the 💾 the override');
  ok(P.process.then && /automatically have the application trigger the save button/.test(P.process.then),'6c the process record carries his words');
  const doc=fs.readFileSync('design/DATA-ANALYSIS-PROCESS.md','utf8');
  ok(/The operator's one step:\*\* none, since v15\.71/.test(doc) && /the 💾 remains as the override/.test(doc),'6d DATA-ANALYSIS-PROCESS.md: no step at the close');
  const pr=fs.readFileSync('design/PROCESS.md','utf8');
  ok(/THE PANEL WRITES THE DAY ITSELF/.test(pr) && /WRITE-IF-ABSENT/.test(pr) && /never REQUEST it/.test(pr),'6e PROCESS.md ② EXPORT rewritten');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8'));
  ok(/THE SAVE RUNS ITSELF/.test(cfg.theWhatAndTheHow.autosave||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1571.js')>=0,'6f .gex-config.json names the rules and this test');
  const sk=fs.readFileSync('skills/gex/SKILL.md','utf8');
  ok(/💾 DUE chip/.test(sk) && /none since v15\.71/.test(sk),'6g the skill says there is no step at the close and what DUE means');
  const inv=fs.readFileSync('design/DASHBOARD-INVENTORY.md','utf8');
  ok(/## 0g · v15\.71 — the save runs itself/.test(inv) && /descriptive/.test(inv.split('## 0g')[1].split('## 0f')[0]),'6h the inventory carries the chip, degree descriptive');
  const arch=fs.readFileSync('design/ARCHITECTURE.md','utf8');
  ok(/no manual step at the close since v15\.71/.test(arch),'6i ARCHITECTURE.md: no manual step at the close');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8');
  ok(/## v15\.71/.test(cl) && cl.indexOf('## v15.71')<cl.indexOf('## v15.70'),'6j the CHANGELOG has the v15.71 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## THE LOG');
  ok(/### v15\.71/.test(ls.slice(logAt>=0?logAt:0)),'6k the lesson log carries the v15.71 entry');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r7=R.rows.find(r=>r.id==='R-7');
  ok(r7 && r7.status==='implemented' && r7.version==='15.71' && r7.by==='operator' && /approved in the chat/.test(r7.why||''),'6m the face change has its Rec row: R-7, his own ask, implemented in v15.71 (rule 9: nothing on the face changes except through Rec)',r7&&[r7.status,r7.version]);
  const seedJs=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(R),'6n REC_SEED equals the file');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8');
  ok(/v15\.71/.test(rn.slice(0,600)) && /the save runs itself/i.test(rn),'6l the resume note is at v15.71 and says the save runs itself');
  finish();
}
function finish(){ console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0); }
