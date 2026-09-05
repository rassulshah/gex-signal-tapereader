// ============================================================================================
// test_v1566.js — (v15.66) THE TAPE: the whole book, every bar, every market. Operator, 2026-09-04: "in order for you to
//   do proper analysis for the day as part of the end of day review, you must store the whole day … save the entire
//   tape in daily files for each market … in a folder like the data folder". Executed against fakes of VEL, the Trinity
//   ladders, IndexedDB and the File System Access folder handle — never grepped where it can be run.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,500):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const decomment=s=>s.replace(/\/\*[\s\S]*?\*\//g,'').replace(/(^|[^:\\'"])\/\/[^\n]*/g,'$1');
const TAPE_FNS=['tapeBook','tapeReadSPXW','tapeReadTrinity','tapeCaptureBar','tapePersist','tapeRestore','tapePruneDay','tapeLateSave','tapeFileJson','tapeFilesFrom','tapeFilesToday','tapeWriteFiles','tapeMarkWritten','tapeExportDay','tapeSweepUnwritten','tapeHealth','tapeCaptureTick'];
const build=(g,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('TAPE_BOOKS')+exVar('TAPE_MAX_BARS')+exVar('TAPE_KEEP_DAYS')+exVar('TAPE')+exVar('TAPE_LAST_WRITE')+'\n'+TAPE_FNS.map(ex).join('\n')+'\n'+(tail||''));

// ---- a fake IndexedDB: enough of the API for the module (put / openCursor / index / delete), synchronous callbacks ----
function fakeIdb(){
  const stores={ tape:new Map(), kv:new Map() };
  const mkStore=(name)=>({
    put(rec){ stores[name].set(rec.id||rec.k, rec); return {}; },
    get(k){ const r={}; setTimeout(()=>{ r.onsuccess&&r.onsuccess({target:{result:stores[name].get(k)}}); },0); return r; },
    delete(k){ stores[name].delete(k); },
    openCursor(){ const r={}; const vals=[...stores[name].values()]; setTimeout(()=>{ let i=0; const step=()=>{ if(i<vals.length){ const v=vals[i++]; r.onsuccess({target:{result:{value:v, primaryKey:v.id, continue:step}}}); } else r.onsuccess({target:{result:null}}); }; step(); },0); return r; },
    index(nm){ return { openCursor(range){ const r={}; const vals=[...stores[name].values()].filter(v=>!range||v[nm]===range.v); setTimeout(()=>{ let i=0; const step=()=>{ if(i<vals.length){ const v=vals[i++]; r.onsuccess({target:{result:{value:v, primaryKey:v.id, continue:step}}}); } else r.onsuccess({target:{result:null}}); }; step(); },0); return r; },
      openKeyCursor(range){ const r={}; const vals=[...stores[name].values()].filter(v=>!range||v[nm]===range.v); setTimeout(()=>{ let i=0; const step=()=>{ if(i<vals.length){ const v=vals[i++]; r.onsuccess({target:{result:{primaryKey:v.id, continue:step}}}); } else r.onsuccess({target:{result:null}}); }; step(); },0); return r; } }; }
  });
  const db={ objectStoreNames:{ contains:(n)=>!!stores[n] }, transaction:(n)=>({ objectStore:(m)=>mkStore(m||n) }) };
  return { db, stores };
}
const sleep=(ms)=>new Promise(r=>setTimeout(r,ms));

(async()=>{
ok(/@version\s+15\.(6[6-9]|[7-9]\d)/.test(src) && /var GPTS_VERSION='15\.(6[6-9]|[7-9]\d)';/.test(src),'0a v15.66+ in both spots');

// ---- 1 · the two readers -------------------------------------------------------------------------------------------
{
  const VEL={ '7750':{k:7750,cur:273.2e6,d5:1e6,d15:7e6,d60:20e6,d1d:30e6,exp:'2026-09-08'}, '7745':{k:7745,cur:123.6e6,d5:0.4e6,d15:12.3e6,d60:15e6,d1d:16e6,exp:'2026-09-08'},
              '7600':{k:7600,cur:120000,d5:0,d15:0,d60:1000,d1d:2000,exp:'2026-09-08'},           // dust — kept: the whole table, not the 90 biggest
              '772':{k:772,cur:5e6,d5:0,d15:0,d60:0,d1d:0,exp:'2026-09-08'},                        // a SPY row that leaked into the harvest: same expiry, still today's → kept by the reader (vend keeps it too); the book file is SPXW by source
              '7800':{k:7800,cur:9e6,d5:0,d15:0,d60:0,d1d:0,exp:'2026-09-12'} };                    // a far expiry: excluded
  const ladders={ SPY:{ king:773, kingKd:390226, count:3, pct:{'773.00':100,'772.00':-45,'771.00':-38}, vel:{'773.00':2.5} }, QQQ:{ king:718, kingKd:257257, count:2, pct:{'718.00':100,'715.00':-27}, vel:{} } };
  const g={ VEL, VEL_META:{ts:1788784200000}, velOk:()=>true, laddersByDollar:()=>ladders, STATE:{SPY:{price:7702.5}}, recorderBlind:()=>false, repoOpen:(cb)=>cb(null), repoKvGet:(k,cb)=>cb(null), repoKvSet(){}, repoDownload(){}, VERSION_STR:()=>'15.66', TODAY:'2026-09-08' };
  const f=build(g,'return { tapeReadSPXW, tapeReadTrinity, tapeCaptureBar, tapeFileJson, TAPE:()=>TAPE, tapeHealth };')(g);
  const s=f.tapeReadSPXW('2026-09-08');
  ok(s && s.n===4 && s.rows['7750'][0]===273200000 && s.rows['7600'][0]===120000 && !s.rows['7800'],'1a SPXW: every row of today\'s expiry, dust included, the far expiry excluded — [cur,d5,d15,d60,d1d] rounded to dollars',s&&Object.keys(s.rows));
  ok(s.rows['7745'].length===5 && s.rows['7745'][2]===12300000 && s.ts===1788784200000,'1b …the five vendor figures in order, and the harvest timestamp');
  const y=f.tapeReadTrinity('SPY');
  ok(y && y.n===3 && y.king===773 && y.kd===390226 && y.rows['773'][0]===100 && y.rows['773'][1]===2.5 && y.rows['772'][0]===-45 && y.rows['772'][1]===null,'1c SPY: every Trinity strike as [pct, vel|null], with the King and its $K for the bar',y);
  ok(f.tapeReadTrinity('VIX')===null && f.tapeReadTrinity('QQQ').n===2,'1d a pane the page is not showing reads null; QQQ reads');
  const g2=Object.assign({},g,{ velOk:()=>false });
  ok(build(g2,'return tapeReadSPXW("2026-09-08");')(g2)===null,'1e SPXW is null while the harvest is not healthy — never a stale table written as the tape');
}

// ---- 2 · the capture: per bar, every book, throttled, blind in replay, restored across a reload ------------------------
{
  const VEL={ '7750':{k:7750,cur:273.2e6,d5:1e6,d15:7e6,d60:20e6,d1d:30e6,exp:'2026-09-08'} };
  const ladders={ SPY:{ king:773, kingKd:390226, count:1, pct:{'773.00':100}, vel:{} }, QQQ:{ king:718, kingKd:257257, count:1, pct:{'718.00':100}, vel:{} }, VIX:{ king:20, kingKd:1000, count:1, pct:{'20.00':100}, vel:{} } };
  const idb=fakeIdb(); let blind=false;
  const g={ VEL, VEL_META:{ts:1}, velOk:()=>true, laddersByDollar:()=>ladders, STATE:{SPY:{price:7702.5, lastClosedB:1788784200000}}, recorderBlind:()=>blind, repoOpen:(cb)=>cb(idb.db), repoKvGet:(k,cb)=>{ const r=idb.stores.kv.get(k); cb(r?r.v:null); }, repoKvSet:(k,v)=>{ idb.stores.kv.set(k,{k,v}); }, repoDownload(){}, VERSION_STR:()=>'15.66', TODAY:'2026-09-08', IDBKeyRange:{ only:(v)=>({v}) } };
  const f=build(g,'return { tapeCaptureBar, tapeCaptureTick, tapeRestore, tapeHealth, TAPE:()=>TAPE };')(g);
  const B1=1788784200000, B2=B1+180000;
  ok(f.tapeCaptureBar(B1, B1+5000, '2026-09-08')===true && f.tapeCaptureBar(B1, B1+9000, '2026-09-08')===false,'2a one capture per closed bar — the second call on the same bar is a no-op');
  const H=f.tapeHealth();
  ok(H.day==='2026-09-08' && Object.keys(H.books).sort().join()==='QQQ,SPXW,SPY,VIX' && H.books.SPXW?.bars===1 && H.books.SPY?.strikesLast===1,'2b all four books captured for the bar',H);
  ok(idb.stores.tape.size===4 && idb.stores.tape.get('2026-09-08|SPY|'+B1)?.kd===390226 && idb.stores.tape.get('2026-09-08|SPXW|'+B1)?.rows['7750'][0]===273200000,'2c …and each book\'s bar is mirrored into IndexedDB under day|book|bar, never localStorage',[...idb.stores.tape.keys()]);
  blind=true; ok(f.tapeCaptureBar(B2, B2, '2026-09-08')===false && f.tapeHealth().books.SPXW.bars===1,'2d nothing is captured while the panel is replaying (recorderBlind)'); blind=false;
  ok(f.tapeCaptureBar(B2, B2, '2026-09-08')===true && f.tapeHealth().books.SPXW.bars===2 && idb.stores.tape.size===8,'2e the next bar adds a second record per book');
  // a reload: a fresh module restores today's bars from the fake IndexedDB
  const f2=build(g,'return { tapeRestore, tapeHealth, tapeCaptureBar, TAPE:()=>TAPE };')(g);
  ok(f2.tapeHealth().books.SPXW===undefined,'2f a fresh load starts empty…');
  await new Promise(r=>f2.tapeRestore('2026-09-08', r)); await sleep(5);
  const H2=f2.tapeHealth();
  ok(H2.restored===true && H2.books.SPXW?.bars===2 && H2.books.SPY?.bars===2 && H2.books.SPXW?.lastBar===B2,'2g …and tapeRestore brings the morning back from IndexedDB',H2);
  ok(f2.tapeCaptureBar(B2, B2, '2026-09-08')===true && f2.tapeHealth().books.SPXW?.bars===2,'2h re-capturing the current bar after a reload replaces, never duplicates');
  // the tick: restore first, then capture on the closed bar
  const f3=build(g,'return { tapeCaptureTick, tapeHealth, TAPE:()=>TAPE };')(g);
  f3.tapeCaptureTick(); await sleep(5); f3.tapeCaptureTick();
  ok(f3.tapeHealth().restored===true && (f3.tapeHealth().books.SPXW?.bars||0)>=2,'2i tapeCaptureTick restores on its first call and captures on the next');
}

// ---- 3 · the file: one shared strike list, one aligned row per bar, the units named ---------------------------------------
{
  const g={ VERSION_STR:()=>'15.66', repoOpen:(cb)=>cb(null), repoKvGet:(k,cb)=>cb(null), repoKvSet(){}, repoDownload(){}, STATE:{}, TODAY:'2026-09-08', VEL:{}, VEL_META:{}, velOk:()=>true, laddersByDollar:()=>({}), recorderBlind:()=>false };
  const f=build(g,'return { tapeFileJson, tapeFilesFrom };')(g);
  const recs=[ { id:'d|SPXW|2', day:'2026-09-08', book:'SPXW', bar:2, t:2, px:7704, n:2, ts:9, rows:{ '7750':[6e6,1e6,2e5,3e5,4e5], '7745':[1e6,0,0,0,0] } },
               { id:'d|SPXW|1', day:'2026-09-08', book:'SPXW', bar:1, t:1, px:7702.5, n:2, ts:9, rows:{ '7750':[5e6,1e5,2e5,3e5,4e5], '7600':[120000,0,0,1000,2000] } } ];
  const J=f.tapeFileJson('2026-09-08','SPXW',recs);
  ok(J.schema===1 && J.book==='SPXW' && J.day==='2026-09-08' && J.src==='skylit' && J.f.join()==='cur,d5,d15,d60,d1d' && /15\.66/.test(J.writtenBy),'3a the SPXW file names itself: schema, book, day, source, the five fields, the writer');
  ok(J.strikes.join()==='7600,7745,7750' && J.bars.length===2 && J.bars[0].bar===1 && J.bars[1].bar===2,'3b one shared strike list (sorted), the bars in time order');
  ok(J.bars[0].v[0][0]===120000 && J.bars[0].v[1]===null && J.bars[0].v[2][0]===5e6 && J.bars[1].v[0]===null && J.bars[1].v[1][0]===1e6,'3c each bar\'s row is aligned to the strike list, null where the strike was absent that bar',J.bars.map(b=>b.v));
  ok(J.bars[0].px===7702.5 && J.bars[0].ts===9 && J.bars[0].king===undefined && /today/.test(J.unit.cur),'3d the bar carries the price and the harvest stamp; the unit says dollars, today\'s expiry');
  const y=f.tapeFileJson('2026-09-08','SPY',[{ id:'d|SPY|1', day:'2026-09-08', book:'SPY', bar:1, t:1, px:7702.5, n:2, king:773, kd:390226, rows:{ '773':[100,2.5], '772':[-45,null] } }]);
  ok(y.f.join()==='pct,vel' && y.bars[0].king===773 && y.bars[0].kd===390226 && y.strikes.join()==='772,773' && y.bars[0].v[0][0]===-45 && /pct\/100/.test(y.unit.kd),'3e the SPY file: [pct, vel] per strike, the King and its $K per bar, the unit line saying how dollars are derived');
  const files=f.tapeFilesFrom('2026-09-08', recs.concat([{ id:'d|QQQ|1', day:'2026-09-08', book:'QQQ', bar:1, t:1, px:7702.5, n:1, king:718, kd:1, rows:{'718':[100,null]} }]));
  ok(Object.keys(files).sort().join()==='QQQ.json,SPXW.json' && JSON.parse(files['SPXW.json']).bars.length===2,'3f tapeFilesFrom: one file per book present, JSON text');
}

// ---- 4 · the writer: tape/<day>/<BOOK>.json under the day-file folder; download when there is no folder; marked once ---------
{
  const idb=fakeIdb(); const written={}; const dirs=[]; const downloads=[];
  const fileHandle=(path)=>({ createWritable:()=>Promise.resolve({ write:(txt)=>{ written[path]=txt; return Promise.resolve(); }, close:()=>Promise.resolve() }) });
  const dirHandle=(path)=>({ getDirectoryHandle:(nm,o)=>{ dirs.push(path+'/'+nm+(o&&o.create?'+':'')); return Promise.resolve(dirHandle(path+'/'+nm)); }, getFileHandle:(nm)=>Promise.resolve(fileHandle(path+'/'+nm)), queryPermission:()=>Promise.resolve('granted') });
  idb.stores.kv.set('dataDir',{k:'dataDir', v:dirHandle('data')});
  const g={ VERSION_STR:()=>'15.66', repoOpen:(cb)=>cb(idb.db), repoKvGet:(k,cb)=>{ const r=idb.stores.kv.get(k); cb(r?r.v:null); }, repoKvSet:(k,v)=>{ idb.stores.kv.set(k,{k,v}); }, repoDownload:(n,t)=>{ downloads.push(n); }, STATE:{SPY:{price:1}}, TODAY:'2026-09-08', VEL:{ '7750':{k:7750,cur:1e6,d5:0,d15:0,d60:0,d1d:0,exp:'2026-09-08'} }, VEL_META:{ts:1}, velOk:()=>true, laddersByDollar:()=>({ SPY:{king:773,kingKd:1,count:1,pct:{'773.00':100},vel:{}} }), recorderBlind:()=>false, IDBKeyRange:{ only:(v)=>({v}) } };
  const f=build(g,'return { tapeCaptureBar, tapeExportDay, tapeWriteFiles, TAPE_LAST_WRITE:()=>TAPE_LAST_WRITE };')(g);
  f.tapeCaptureBar(1788784200000, 1, '2026-09-08');
  f.tapeExportDay('2026-09-08', true); await sleep(20);
  ok(Object.keys(written).sort().join()==='data/tape/2026-09-08/SPXW.json,data/tape/2026-09-08/SPY.json','4a the 💾 writes tape/<day>/<BOOK>.json inside the same folder as the day file',Object.keys(written));
  ok(dirs[0]==='data/tape+' && dirs[1]==='data/tape/2026-09-08+','4b …creating tape/ and the day folder as needed',dirs);
  ok(JSON.parse(written['data/tape/2026-09-08/SPXW.json']||'{"bars":[{"v":[[0]]}]}').bars[0].v[0][0]===1e6 && (idb.stores.kv.get('tapeWritten:2026-09-08')||{v:{}}).v.how==='repo folder' && (f.TAPE_LAST_WRITE()||{}).day==='2026-09-08','4c the file is the tape, and the day is marked written in kv');
  // no folder: the files download, named tape-<day>-<BOOK>.json
  const idb2=fakeIdb(); const g2=Object.assign({},g,{ repoOpen:(cb)=>cb(idb2.db), repoKvGet:(k,cb)=>{ const r=idb2.stores.kv.get(k); cb(r?r.v:null); }, repoKvSet:(k,v)=>{ idb2.stores.kv.set(k,{k,v}); } });
  const f2=build(g2,'return { tapeCaptureBar, tapeExportDay };')(g2);
  f2.tapeCaptureBar(1788784200000, 1, '2026-09-08'); f2.tapeExportDay('2026-09-08', true); await sleep(20);
  ok(downloads.sort().join()==='tape-2026-09-08-SPXW.json,tape-2026-09-08-SPY.json' && (idb2.stores.kv.get('tapeWritten:2026-09-08')||{v:{}}).v.how==='download','4d without a folder handle the files download, and the day is still marked',downloads);
  ok(/tapeExportDay\(date, silent\)/.test(ex('repoExportDay')),'4e repoExportDay — the 💾 and the 15:01 auto-export — calls tapeExportDay for the same day');
  ok(/tapeCaptureTick\(\)/.test(decomment(ex('tick'))),'4f the tick captures every closed bar');
}

// ---- 5 · the late save: a day captured and never written is written on the next boot / the next 💾, once -------------------
{
  const idb=fakeIdb(); const written={};
  const fileHandle=(path)=>({ createWritable:()=>Promise.resolve({ write:(txt)=>{ written[path]=txt; return Promise.resolve(); }, close:()=>Promise.resolve() }) });
  const dirHandle=(path)=>({ getDirectoryHandle:(nm)=>Promise.resolve(dirHandle(path+'/'+nm)), getFileHandle:(nm)=>Promise.resolve(fileHandle(path+'/'+nm)), queryPermission:()=>Promise.resolve('granted') });
  idb.stores.kv.set('dataDir',{k:'dataDir', v:dirHandle('data')});
  // yesterday's tape sits in IndexedDB, never written; the day before was written; today is empty
  idb.stores.tape.set('2026-09-05|SPXW|1', { id:'2026-09-05|SPXW|1', day:'2026-09-05', book:'SPXW', bar:1, t:1, px:1, n:1, rows:{'7750':[1,0,0,0,0]} });
  idb.stores.tape.set('2026-09-04|SPXW|1', { id:'2026-09-04|SPXW|1', day:'2026-09-04', book:'SPXW', bar:1, t:1, px:1, n:1, rows:{'7750':[1,0,0,0,0]} });
  idb.stores.kv.set('tapeWritten:2026-09-04',{k:'tapeWritten:2026-09-04', v:{how:'repo folder'}});
  const g={ VERSION_STR:()=>'15.66', repoOpen:(cb)=>cb(idb.db), repoKvGet:(k,cb)=>{ const r=idb.stores.kv.get(k); cb(r?r.v:null); }, repoKvSet:(k,v)=>{ idb.stores.kv.set(k,{k,v}); }, repoDownload(){}, STATE:{}, TODAY:'2026-09-08', VEL:{}, VEL_META:{}, velOk:()=>true, laddersByDollar:()=>({}), recorderBlind:()=>false, IDBKeyRange:{ only:(v)=>({v}) } };
  const f=build(g,'return { tapeRestore, tapeHealth };')(g);
  await new Promise(r=>f.tapeRestore('2026-09-08', r)); await sleep(20);
  ok(Object.keys(written).join()==='data/tape/2026-09-05/SPXW.json','5a on boot the unwritten day (09-05) is written; the written one (09-04) is not written again',Object.keys(written));
  ok((idb.stores.kv.get('tapeWritten:2026-09-05')||{v:{}}).v.how==='repo folder','5b …and marked');
  ok(f.tapeHealth().books.SPXW===undefined && f.tapeHealth().day==='2026-09-08','5c today starts empty — an old day is never mistaken for today');
  // retention: only the last TAPE_KEEP_DAYS older days stay in IndexedDB
  const idb2=fakeIdb(); for(let d=1; d<=8; d++){ const day='2026-08-'+String(20+d).padStart(2,'0'); idb2.stores.tape.set(day+'|SPY|1',{ id:day+'|SPY|1', day, book:'SPY', bar:1, t:1, px:1, n:1, rows:{'773':[100,null]} }); idb2.stores.kv.set('tapeWritten:'+day,{k:'tapeWritten:'+day, v:{how:'repo folder'}}); }
  const g2=Object.assign({},g,{ repoOpen:(cb)=>cb(idb2.db), repoKvGet:(k,cb)=>{ const r=idb2.stores.kv.get(k); cb(r?r.v:null); }, repoKvSet:(k,v)=>{ idb2.stores.kv.set(k,{k,v}); } });
  const f2=build(g2,'return { tapeRestore };')(g2);
  await new Promise(r=>f2.tapeRestore('2026-09-08', r)); await sleep(30);
  const left=[...idb2.stores.tape.keys()].map(k=>k.split('|')[0]).sort();
  ok(left.length===5 && left[0]==='2026-08-24','5d older days beyond TAPE_KEEP_DAYS (5) are pruned from IndexedDB — the files are the record',left);
}

// ---- 6 · the store, the nightly, the records ---------------------------------------------------------------------------
{
  const ro=ex('repoOpen');
  ok(/indexedDB\.open\(REPO_DB_NAME, 4\)/.test(ro) && /createObjectStore\('tape',\{keyPath:'id'\}\)/.test(ro) && /createIndex\('day','day'/.test(ro),'6a the repo database is version 4 with a `tape` store keyed day|book|bar and indexed by day');
  // the upgrade handler creates the store on a database that lacks it — executed
  const created=[]; const fakeDb={ objectStoreNames:{ contains:()=>false }, createObjectStore:(n)=>{ created.push(n); return { createIndex(){} }; } };
  const req={}; const g={ REPO_DB:null, REPO_DB_NAME:'x', indexedDB:{ open:()=>req } };
  new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('repoOpen')+'\nrepoOpen(function(){});')(g);
  req.onupgradeneeded({ target:{ result:fakeDb } });
  ok(created.indexOf('tape')>=0 && created.indexOf('snaps')>=0 && created.indexOf('defl')>=0,'6b …and the upgrade creates it beside the older stores',created);
  ok(fs.existsSync('tools/nightly/tape.py') && /def load\(day, root=ROOT\)/.test(fs.readFileSync('tools/nightly/tape.py','utf8')) && /def dollars\(book, bar, k\)/.test(fs.readFileSync('tools/nightly/tape.py','utf8')),'6c the nightly has a reader (tools/nightly/tape.py: load · dollars · coverage · --selftest)');
  ok(/tape_cov/.test(fs.readFileSync('tools/nightly/run.py','utf8')) && /tape=tape_cov/.test(fs.readFileSync('tools/nightly/run.py','utf8')),'6d run.py reports the tape coverage in the nightly log');
  ok(/__gptsDebug\.tape=function/.test(src) && /__gptsDebug\.tapeExport=function/.test(src),'6e the debug surface: bars per book today, and a manual export');
  const da=fs.existsSync('design/DATA-ARCHITECTURE.md')?fs.readFileSync('design/DATA-ARCHITECTURE.md','utf8'):'';
  ok(/data\/tape\/<day>\/<BOOK>\.json|data\/tape\//.test(da) && /repo\.tape|`tape` store|IndexedDB/.test(da),'6f DATA-ARCHITECTURE names the tape files and the store');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.66/.test(cl) && cl.indexOf('## v15.66')<cl.indexOf('## v15.65'),'6g the CHANGELOG has the v15.66 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.66 — a sketch had been called the tape/.test(ls.slice(logAt)),'6h the lesson log carries the v15.66 entry');
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
})();
