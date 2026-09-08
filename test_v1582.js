// test_v1582.js — (v15.82) THE OUTCOMES EXPORTED FROM THE ARCHIVE (R-22, F-20's fix). Operator, 2026-09-08: "lets go with
// your recommendation". The localStorage budget sheds the outcome queue from the second hour of every session (measured
// live: 08:36 → 09:00 by 10:44 CT). Every resolved record was already mirrored to IndexedDB, but the in-memory archive was
// boot-time only, featStats read the LS queue ALONE for a day LS still held, and the export wrote that queue. Now the
// archive is kept current, one merge rule (featMergeRecs — LS ∪ archive, LS wins per record) serves featStats and the
// export, repoFeatDay reads the archive for the date, and the day file says what came from where (featSource).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
const sleep=(ms)=>new Promise(r=>setTimeout(r,ms));

// ---------- 0. the version ----------
ok(/@version\s+15\.(8[2-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[2-9]|9\d)';/.test(src), '0a v15.82 or later in both spots');

// ---------- 1. the merge rule ----------
{
  eval(ex('featMergeRecs'));
  const L=[{key:'a',t:3,bar:3,resolved:true,hit:1},{key:'b',t:3,bar:3,resolved:false},{key:'a',t:4,bar:4}];
  const A=[{key:'a',t:1,bar:1,resolved:true,hit:0,id:'x'},{key:'a',t:2,bar:2,resolved:true},{key:'a',t:3,bar:3,resolved:true,hit:0},{key:'b',t:2,bar:2,resolved:true}];
  const M=featMergeRecs(L, A);
  ok(M.length===6, '1a LS ∪ archive by key|t: 3 + 4 with one overlap = 6', M.length);
  ok(M.map(r=>r.key+'|'+r.t).join(' ')==='a|1 a|2 b|2 a|3 b|3 a|4', '1b sorted by t then key', M.map(r=>r.key+'|'+r.t));
  ok(M.find(r=>r.key==='a'&&r.t===3).hit===1, '1c on the overlap the LS copy wins (it is the live one)');
  ok(featMergeRecs([], A).length===4 && featMergeRecs(L, []).length===3 && featMergeRecs(null, null).length===0, '1d either side empty → the other; both empty → []');
  ok(featMergeRecs([{key:'a'},{t:1},null], [{key:'z',t:1}]).length===1, '1e records without key or t are dropped, null tolerated');
  ok(featMergeRecs([{key:'a',t:1,hit:1},{key:'a',t:1,hit:0}], [{key:'a',t:1,hit:2}]).length===1 && featMergeRecs([{key:'a',t:1,hit:1},{key:'a',t:1,hit:0}], [])[0].hit===1, '1f a duplicate inside the queue itself is one record — the first copy');
}

// ---------- 2. the archive kept current in memory ----------
{
  global.FEAT_ARCHIVE={};
  let puts=[]; const stubOpen2=(cb)=>cb({ objectStoreNames:{ contains:()=>true }, transaction:()=>({ objectStore:()=>({ put:(r)=>puts.push(r.id) }) }) });
  const built=new Function('repoOpen','FEAT_ARCHIVE', ['featArchiveMerge','repoUpsertFeat'].map(ex).join('\n')+'\nreturn { repoUpsertFeat:repoUpsertFeat };')(stubOpen2, FEAT_ARCHIVE);
  const repoUpsertFeat=built.repoUpsertFeat;
  repoUpsertFeat('SPY','2026-09-08',[{key:'a',t:1,bar:1,resolved:true,hit:1},{key:'b',t:1,bar:1,resolved:true,hit:0}]);
  ok(FEAT_ARCHIVE.SPY && FEAT_ARCHIVE.SPY['2026-09-08'].length===2 && FEAT_ARCHIVE.SPY['2026-09-08'][0].id==='SPY|2026-09-08|a|1' && FEAT_ARCHIVE.SPY['2026-09-08'][0].sym==='SPY', '2a repoUpsertFeat writes the in-memory archive (id, sym, date like the loader)', FEAT_ARCHIVE.SPY['2026-09-08'][0]);
  ok(puts.length===2, '2b …and still mirrors to IndexedDB');
  repoUpsertFeat('SPY','2026-09-08',[{key:'a',t:1,bar:1,resolved:true,hit:1,atClose:true},{key:'c',t:2,bar:2,resolved:true}]);
  const A=((FEAT_ARCHIVE.SPY||{})['2026-09-08'])||[];
  ok(A.length===3 && A.find(r=>r.key==='a'&&r.t===1) && A.find(r=>r.key==='a'&&r.t===1).atClose===true, '2c a re-resolved record REPLACES its archived copy (by key|t), a new one is appended', A.length);
  const src2=ex('repoUpsertFeat');
  ok(/try\{ featArchiveMerge\(sym, date, recs\); \}catch\(eM\)\{\}/.test(src2) && src2.indexOf('featArchiveMerge')<src2.indexOf('repoOpen('), '2d memory first, the async IDB write after');
  repoUpsertFeat('SPY','2026-09-08',[{key:'a',t:1,bar:1},{t:9},null]);
  ok((((FEAT_ARCHIVE.SPY||{})['2026-09-08'])||[]).length===3, '2e records without key or t are not archived; null tolerated');
}

// ---------- 3. repoFeatDay reads the archive for one date ----------
{
  const rf=ex('repoFeatDay');
  ok(/objectStore\('feat'\)\.index\('date'\)/.test(rf) && /IDBKeyRange\.only\(date\)/.test(rf) && /\(out\[r\.sym\]=out\[r\.sym\]\|\|\[\]\)\.push\(r\)/.test(rf), '3a repoFeatDay walks the feat store by the date index into {sym:[recs]}');
  ok(/if\(!db\)\{ cb\(\{\}\); return; \}/.test(rf) && /if\(!db\.objectStoreNames\.contains\('feat'\)\)\{ cb\(\{\}\); return; \}/.test(rf) && /\}catch\(e\)\{ cb\(\{\}\); \}/.test(rf), '3b no db / no store / a throw → cb({}) — the export never waits forever');
  // run it against a cursor stub
  const rows=[{id:'SPY|d|a|2',sym:'SPY',date:'2026-09-08',key:'a',t:2},{id:'SPY|d|a|1',sym:'SPY',date:'2026-09-08',key:'a',t:1},{id:'QQQ|d|a|1',sym:'QQQ',date:'2026-09-08',key:'a',t:1}];
  // ⚠ built in its OWN scope (new Function), not eval'd here: a sloppy-mode eval hoists the declaration into this
  // module's scope and shadows the global stubs section 5 installs (the first run of this file found that out).
  const stubOpen=(cb)=>cb({ objectStoreNames:{ contains:()=>true }, transaction:()=>({ objectStore:()=>({ index:()=>({ openCursor:()=>{ const req={}; setTimeout(()=>{ let i=0; const step=()=>{ if(i<rows.length){ const r=rows[i++]; req.onsuccess({ target:{ result:{ value:r, continue:step } } }); } else req.onsuccess({ target:{ result:null } }); }; step(); },0); return req; } }) }) }) });
  const rfd=new Function('repoOpen','IDBKeyRange', ex('repoFeatDay')+'\nreturn repoFeatDay;')(stubOpen, { only:(d)=>({ only:d }) });
  let got=null; rfd('2026-09-08', (o)=>{ got=o; });
  (async()=>{ await sleep(20);
    ok(got && got.SPY && got.SPY.length===2 && got.SPY[0].t===1 && got.QQQ && got.QQQ.length===1, '3c the records come back per symbol, sorted by t', got&&Object.keys(got));
  })();
}

// ---------- 4. featStats reads LS ∪ archive per day ----------
{
  const fsrc=ex('featStats');
  ok(/var arr=\(typeof featMergeRecs==='function'\) \? featMergeRecs\(\(\(days\[dk\]\|\|\{\}\)\.feat\|\|\{\}\)\[sym\]\|\|\[\], arch\[dk\]\|\|\[\]\)/.test(fsrc), '4a featStats merges the LS queue with the archive for EVERY day (it used to take LS alone for a day LS still held — the live face saw the shed queue)');
  ok(/if\(!arr\.length\) arr=arch\[dk\]\|\|\[\];/.test(fsrc), '4b …and a day with no LS at all still reads the archive (unchanged)');
}

// ---------- 5. the export writes LS ∪ archive and says what came from where ----------
{
  const rx=ex('repoExportDay');
  ok(/var _getFeat=\(typeof repoFeatDay==='function'\)\?repoFeatDay:function\(d, cb\)\{ cb\(\{\}\); \};/.test(rx) && /_getFeat\(date, function\(FA\)\{/.test(rx), '5a repoExportDay reads the archive for the date through repoFeatDay — degrading to the queue alone when the function is absent (a harness, never the panel)');
  ok(/payload\.feat=merged;/.test(rx) && /payload\.featSource=src;/.test(rx) && /payload\.matrix=buildFeatureMatrix\(\{ feat:merged \}\);/.test(rx), '5b the file carries the merged queue, featSource, and a matrix rebuilt from the merged queue');
  ok(/\}\);   \/\/ repoFeatDay \(v15\.82\)\n  \}\);\n\}/.test(rx+'\n'), '5c the callback closes where repoDay\'s did — the writer\'s tail is untouched');
  // run the writer end to end with stubs: LS queue = the last two bars, archive = the morning
  global.TODAY='2026-09-08'; global.AUTOSAVE={}; global.DAY_WRITTEN={}; global.REPO_LAST_SAVE=null; global.SAVED_TODAY=null; global.TAPE_LAST_WRITE=null;
  const written={};
  global.repoDay=(d,cb)=>cb({ date:d, snaps:{ SPY:[{t:1},{t:2},{t:3},{t:4}] } });
  global.buildDayExport=(d)=>({ schema:'x', date:d, feat:{ SPY:[{key:'a',t:3,bar:3,resolved:true,hit:1},{key:'a',t:4,bar:4,resolved:false}] }, matrix:[] });
  global.repoFeatDay=(d,cb)=>cb({ SPY:[{key:'a',t:1,bar:1,resolved:true,hit:0},{key:'a',t:2,bar:2,resolved:true,hit:1},{key:'a',t:3,bar:3,resolved:true,hit:0}], QQQ:[{key:'a',t:1,bar:1,resolved:true}] });
  global.buildFeatureMatrix=(day)=>Object.keys(day.feat).map(s=>({ sym:s, n:day.feat[s].length }));
  global.rulesDoc=()=>({asOf:'2026-09-04'}); global.modelStamp=()=>'m'; global.recorderLoad=()=>({days:{}}); global.VERSION_STR=()=>'15.82';
  global.tapeExportDay=()=>{}; global.repoKvGet=(k,cb)=>cb(k==='dataDir'?{ getFileHandle:(nm)=>Promise.resolve({ createWritable:()=>Promise.resolve({ write:(t)=>{ written[nm]=t; return Promise.resolve(); }, close:()=>Promise.resolve() }) }), queryPermission:()=>Promise.resolve('granted') }:null);
  global.repoKvSet=()=>{}; global.localStorage={ setItem:()=>{}, getItem:()=>null }; global.pipeNoteSave=()=>{}; global.render=()=>{}; global.repoDownload=()=>{};
  global.dayWrittenMark=()=>{}; global.autosaveBlock=()=>{}; global.swallow=()=>{}; global.ctTodayStr=()=>'2026-09-08'; global.dayHasData=()=>true;
  for(const n of ['dayWrittenMark','autosaveBlock','pipeNoteSave','saveState','ctNow','ctAfterClose']){ if(typeof global[n]!=='function') global[n]=()=>({}); }
  eval(ex('featMergeRecs')+'\n'+ex('repoExportDay'));
  let done=null; repoExportDay('2026-09-08', true, 'auto', (o)=>{ done=o; });
  (async()=>{ await sleep(40);
    const p=JSON.parse(written['2026-09-08.json']||'{}');
    ok(p.feat && p.feat.SPY && p.feat.SPY.length===4 && p.feat.SPY.map(r=>r.t).join(',')==='1,2,3,4', '5d the file\'s SPY queue is the morning from the archive + the queue: bars 1,2,3,4 (the queue alone held 3,4)', p.feat&&p.feat.SPY&&p.feat.SPY.map(r=>r.t));
    ok(p.feat.SPY.find(r=>r.t===3).hit===1, '5e the overlap (bar 3) is the LS copy');
    ok(p.feat.QQQ && p.feat.QQQ.length===1, '5f a symbol only the archive holds is carried too');
    ok(p.featSource && p.featSource.ls.SPY.n===2 && p.featSource.archive.SPY.n===3 && p.featSource.merged.SPY.n===4 && p.featSource.merged.SPY.from===1 && p.featSource.merged.SPY.to===4 && /LS wins per record/.test(p.featSource.note), '5g featSource says what came from where and the span each covers', p.featSource);
    ok(Array.isArray(p.matrix) && p.matrix.find(m=>m.sym==='SPY').n===4, '5h the matrix is rebuilt from the merged queue (4 rows, not 2)', p.matrix);
    ok(p.snaps && p.snaps.SPY.length===4 && p.writtenBy==='auto', '5i the snaps and the rest of the payload are untouched');
    // the archive absent (a harness, or IDB gone): the queue alone, as before
    global.repoFeatDay=(d,cb)=>cb({});
    repoExportDay('2026-09-08', true, 'auto', ()=>{});
    await sleep(40);
    const p2=JSON.parse(written['2026-09-08.json']||'{}');
    ok(p2.feat.SPY.length===2 && p2.featSource.archive.SPY.n===0 && p2.featSource.merged.SPY.n===2, '5j no archive → the queue as before, featSource says archive 0');
  })();
}

// ---------- 6. the probe ----------
{
  const fh=src.slice(src.indexOf('window.__gptsDebug.featHealth=function(){'), src.indexOf('window.__gptsDebug.featHealth=function(){')+2600);
  ok(/archive:A\.length, merged:M\.length, mergedBars:Object\.keys\(mb\)\.length, lsSpan:tsOf\(a\), mergedSpan:tsOf\(M\)/.test(fh), '6a featHealth reports the archive beside the queue: archive, merged, mergedBars, the spans');
}

// ---------- 7. the record ----------
setTimeout(()=>{
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r22=R.rows.find(r=>r.id==='R-22');
  ok(r22 && r22.status==='implemented' && r22.version==='15.82', '7a R-22 on Rec, implemented in v15.82', r22&&[r22.status,r22.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '7b REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.82/.test(cl) && cl.indexOf('## v15.82')<cl.indexOf('## v15.81'), '7c the CHANGELOG has the v15.82 entry on top');
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const r82=P.roadmap.find(r=>r.v==='15.82'), seas=P.roadmap.find(r=>/SEASONALITY TRACKED/.test(r.title));
  ok(r82 && /ARCHIVE/i.test(r82.title) && (r82.status==='next'||r82.status==='shipped') && seas && +seas.v>15.82 && P.roadmap.filter(r=>r.status==='next').length===1, '7d the plan: v15.82 is this build; the seasonality moved past it', r82&&[r82.status, seas&&seas.v]);
  const seedP=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedP)===JSON.stringify(P), '7e PLAN_SEED equals the file');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.82/.test(ls.slice(logAt>=0?logAt:0)), '7f the lesson log carries the v15.82 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(8[2-9]|9\d)/.test(rn.slice(0,600)) && /featSource/.test(rn), '7g the resume note is at v15.82 or later and names featSource');
  const fd=fs.readFileSync('skylit-docs/FINDINGS.md','utf8'); const f20=fd.slice(fd.indexOf('## F-20'), fd.indexOf('## F-20')+1200); ok(/FIXED in v15\.82/.test(f20), '7h FINDINGS F-20 says it is fixed in v15.82');
  console.log('test_v1582: '+pass+' passed, '+fail+' failed');
  process.exit(fail?1:0);
}, 200);
