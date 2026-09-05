// ============================================================================================
// test_v1551.js — (v15.51) THE LOOP MEASURES NOTHING: four fixes, each run, each mutation-tested.
//
//   1 · lodhod scored AT THE CLOSE on the session label. The old scorer compared a 30-minute
//       excursion to the WHOLE session range and read 362/362 = 100%, incl. 5/5 in the 0-19% cell.
//       The assertion that matters here is the one where the NEW scorer says 0 and the OLD said 1.
//   2 · toClose: a session-level feature waits for 15:00 CT, then scores on every remaining bar.
//   3 · hitNull: a scorer that THREW and one that DECLINED are no longer the same record.
//   4 · repoUpsertDefl: the event-level ledger survives localStorage eviction (IDB v3, 'defl').
//   5 · ONE geometry: levelMarkerOf and reactDefence both take atr x scaleUsed x DEFL_NEAR.
//       ⚠ NOT claimed: that this fills MARK. LVLMK_LAST showed a row 0.5 from price inside the old
//       threshold and still empty — __gptsDebug.mark() decides that, not this file.
//
// Every function below is EXTRACTED AND RUN with stubs. Set GPTS_SRC to test a mutated copy.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,220):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
// extract `outcome:function(rec, fwd){...}` belonging to a given feature key
function exOutcome(key){
  const at=src.indexOf("key:'"+key+"'"); if(at<0) throw new Error('feature not found: '+key);
  const o=src.indexOf('outcome:function', at); if(o<0) throw new Error('no outcome for '+key);
  let i=src.indexOf('{',o),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return '('+src.slice(src.indexOf('function',o),k+1)+')'; } } }

// ---- 1 · lodhod outcome ----------------------------------------------------------------------
{
  const outcome=eval(exOutcome('lodhod'));
  const LOD={ ok:1, side:'LOD', lod:100.00, hod:104.00, rngPts:4.00 };
  const HOD={ ok:1, side:'HOD', lod:100.00, hod:104.00, rngPts:4.00 };
  let r=outcome(LOD,{ px0:101.0, mae:-0.5, mfe:1.0 });
  ok(r.hit===1,'1a LOD standing at 100, lowest later low 100.5 -> HELD',r);
  r=outcome(LOD,{ px0:101.0, mae:-1.2, mfe:1.0 });
  ok(r.hit===0,'1b LOD standing at 100, a later bar printed 99.8 -> NOT held',r);
  // ⚠ THE TAUTOLOGY. Old scorer: mae(-1.2) > -rngPts(4)*0.999 -> 1. A 1.2-point excursion against a
  // 4-point range "held" even though the LOD was undercut. The new scorer must say 0 here.
  ok(r.hit===0,'1c ...and that is the case the OLD scorer marked HELD (excursion < range) — it can now fail',r);
  r=outcome(HOD,{ px0:103.0, mfe:0.8, mae:-0.3 });
  ok(r.hit===1,'1d HOD standing at 104, highest later high 103.8 -> HELD',r);
  r=outcome(HOD,{ px0:103.0, mfe:1.5, mae:-0.3 });
  ok(r.hit===0,'1e HOD standing at 104, a later bar printed 104.5 -> NOT held',r);
  r=outcome(LOD,{ px0:100.0, mae:0, mfe:2.0 });
  ok(r.hit===1,'1f a later low EQUAL to the standing extreme (within tolerance) still holds',r);
  r=outcome(LOD,{ mae:-0.5, mfe:1.0 });
  ok(r.hit===null,'1g no px0 -> declines (null), never guesses',r);
  r=outcome({ ok:0 },{ px0:1, mae:0, mfe:0 });
  ok(r.hit===null,'1h rec.ok=0 -> declines',r);
  ok(r.atClose===true,'1i the outcome stamps atClose so a reader knows which scorer produced it',r);
  ok(/toClose\s*:\s*true/.test(src.slice(src.indexOf("key:'lodhod'"), src.indexOf("key:'lodhod'")+400)),'1j lodhod is registered toClose');
}

// ---- 2/3 · resolver: toClose + hitNull --------------------------------------------------------
{
  const RUN=(opts)=>{
    const g={};
    g.registerCoreFeatures=()=>{};
    g.RECORDER_SYMS=['SPY']; g.TODAY='2026-09-03';
    g.recorderBlind=()=>false;
    const cs=[]; for(let i=0;i<opts.bars;i++) cs.push({o:100,h:100.5,l:99.5,c:100.1});
    g.STATE={ SPY:{ candles:cs } };
    const day={ date:'2026-09-03', feat:{ SPY:opts.recs } };
    g.recorderLoad=()=>({days:{}}); g.recorderDay=()=>day;
    g.nodeMapModel=()=>null;
    g.featureByKey=(k)=>opts.feats[k]||null;
    g.FEAT_FWD=10; g.DIR_PTS=0.5;
    g.ctNowSecOfDay=()=>opts.sec;
    g.frameOutcome=()=>null;
    g.recorderSave=()=>{}; g.repoUpsertFeat=()=>{}; g.featStatsInvalidate=()=>{};
    g.swallowed=[]; g.swallow=(l,e)=>{ g.swallowed.push(l); };
    const code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('resolveFeatureOutcomes')+'\nresolveFeatureOutcomes("SPY");';
    (new Function('__g',code))(g);
    return { recs:opts.recs, swallowed:g.swallowed };
  };
  const rec=(key,n)=>({ key:key, n:n, px:100, resolved:false, rec:{ ok:1 } });

  // a scorer that THROWS vs one that DECLINES, both resolved on a full window
  let out=RUN({ bars:20, sec:12*3600, feats:{
      thrower:{ key:'thrower', fwd:10, outcome:()=>{ throw new Error('boom'); } },
      decliner:{ key:'decliner', fwd:10, outcome:()=>({ hit:null, mfe:0, mae:0 }) },
      scorer:{ key:'scorer', fwd:10, outcome:()=>({ hit:true, mfe:0, mae:0 }) } },
    recs:[ rec('thrower',5), rec('decliner',5), rec('scorer',5) ] });
  const T=out.recs[0], D=out.recs[1], S=out.recs[2];
  ok(T.resolved && T.hit===null && T.hitNull==='threw','3a a throwing scorer resolves hit:null WITH hitNull="threw"',T);
  ok(out.swallowed.some(l=>/feat\.outcome:thrower/.test(l)),'3b ...and the throw is REPORTED via swallow(), not eaten by the empty catch',out.swallowed);
  ok(D.resolved && D.hit===null && D.hitNull==='declined','3c a declining scorer resolves hit:null WITH hitNull="declined"',D);
  ok(S.resolved && S.hit===1 && S.hitNull===undefined,'3d a scoring scorer carries no hitNull',S);

  // toClose: before the close it must NOT resolve, even with a full 10-bar window available
  out=RUN({ bars:40, sec:13*3600, feats:{ lod:{ key:'lod', fwd:10, toClose:true, outcome:(r,f)=>({ hit:1, mfe:f.mfe, mae:f.mae }) } },
            recs:[ rec('lod',5) ] });
  ok(out.recs[0].resolved===false,'2a toClose: NOT resolved at 13:00 CT even though 35 bars are available',out.recs[0]);
  // after the close it resolves on EVERY remaining bar, full window, atClose
  out=RUN({ bars:40, sec:15*3600+60, feats:{ lod:{ key:'lod', fwd:10, toClose:true, outcome:(r,f)=>({ hit:1, mfe:f.mfe, mae:f.mae }) } },
            recs:[ rec('lod',5) ] });
  const L=out.recs[0];
  ok(L.resolved===true && L.atClose===true,'2b toClose: resolved after 15:00 CT and stamped atClose',L);
  ok(L.fwdUsed===35,'2c ...on ALL 35 remaining bars, not the 10-bar default',L.fwdUsed);
  ok(L.partial===false,'2d ...and it is a FULL window by definition, never partial',L.partial);
  // a normal feature is unchanged: short window before the close stays pending
  out=RUN({ bars:8, sec:12*3600, feats:{ n:{ key:'n', fwd:10, outcome:()=>({hit:1}) } }, recs:[ rec('n',5) ] });
  ok(out.recs[0].resolved===false,'2e a normal 10-bar feature with 3 bars available at noon stays pending (unchanged)');
}

// ---- 4 · repoUpsertDefl + IDB v3 --------------------------------------------------------------
{
  const puts=[]; let opened=null;
  const g={};
  g.repoOpen=(cb)=>cb({ objectStoreNames:{ contains:(n)=>n==='defl' },
                        transaction:()=>({ objectStore:()=>({ put:(r)=>puts.push(r) }) }) });
  let code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('repoUpsertDefl')+
    "\nrepoUpsertDefl('SPY','2026-09-03',[{sig:'a@765.00',tapBar:12,cont:null},{sig:'b@764.00',tapBar:30,cont:1},{tapBar:9}]);";
  (new Function('__g',code))(g);
  ok(puts.length===2,'4a two events with a sig are written; the one without a sig is refused',puts.length);
  ok(puts[0].id==='SPY|2026-09-03|a@765.00|12' && puts[0].sym==='SPY' && puts[0].date==='2026-09-03','4b id = sym|date|sig|tapBar, so the labelled pass OVERWRITES rather than duplicates',puts[0]);
  ok(puts[1].cont===1,'4c the forward label travels with the row',puts[1]);

  // IDB opens at version 3 and creates the 'defl' store on upgrade
  const created=[]; const g2={ REPO_DB_NAME:'gpts_repo_v1', REPO_DB:null };
  g2.indexedDB={ open:(name,ver)=>{ opened=ver; const req={};
      setTimeout(()=>{ req.onupgradeneeded({ target:{ result:{ objectStoreNames:{ contains:()=>false },
        createObjectStore:(n)=>{ created.push(n); return { createIndex:()=>{} }; } } } });
        req.onsuccess({ target:{ result:{} } }); },0); return req; } };
  code=Object.keys(g2).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('repoOpen')+'\nrepoOpen(function(){});';
  (new Function('__g',code))(g2);
  setTimeout(()=>{
    ok(opened===4,'4d the repo opens at IDB version 4 — v15.51 bumped it to 3 for the defl store, v15.66 to 4 for the tape store (a new store needs a bump or onupgradeneeded never fires)',opened);
    ok(created.indexOf('defl')>=0,'4e ...and the upgrade creates the defl store',created);
    ok(created.indexOf('feat')>=0 && created.indexOf('snaps')>=0,'4f ...without dropping the existing stores',created);
    finish();
  },5);
}

// ---- 4g · recordDeflections mirrors when it saved -------------------------------------------
{
  const g={}; const calls=[];
  g.RECORDER_SYMS=['SPY']; g.TODAY='2026-09-03';
  g.recorderBlind=()=>false;
  g.nodeMapModel=()=>({ ok:true, levels:[{k:765}] });
  g.STATE={ SPY:{ price:764.9, candles:new Array(12) } };
  const day={ date:'2026-09-03' };
  g.recorderLoad=()=>({}); g.recorderDay=()=>day;
  g.deflectionAt=()=>({ dir:'up', awayPts:0.3, bars:2 });
  g.classifyDeflection=()=>({ name:'pullback', chips:[] });
  g.deflSetupKey=()=>'pb.up';
  g.DEFL_FWD_BARS=10; g.RECORDER_MAX_EVENTS=500;
  g.labelDeflectionOutcomes=()=>false;
  g.recorderSave=()=>{};
  g.repoUpsertDefl=(s,d,a)=>calls.push([s,d,a.length]);
  const code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('recordDeflections')+'\nrecordDeflections("SPY");';
  (new Function('__g',code))(g);
  ok(calls.length===1 && calls[0][0]==='SPY' && calls[0][1]==='2026-09-03' && calls[0][2]===1,'4g a new deflection is mirrored to IDB in the same pass that saved it',calls);
}

// ---- 5 · one geometry --------------------------------------------------------------------------
{
  const mk=(cands, a, scaleUsed)=>{
    const g={ DEFL_NEAR:1.0 };
    g.closedCandles=()=>cands; g.atr=()=>a; g.emBand=()=>({ scaleUsed:scaleUsed });
    const code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('nodeBandDisp')+'\nreturn nodeBandDisp;';
    return (new Function('__g',code))(g);
  };
  let f=mk([{},{},{}], 0.5, 10);
  ok(f('SPY',10)===5,'5a band = atr(0.5) x undScale(10) x DEFL_NEAR(1.0) = 5 chart points on an ES-scale chart',f('SPY',10));
  ok(f('SPY',null)===5,'5b with no undScale it takes the band\'s own scaleUsed (10)',f('SPY',null));
  ok(f('SPY',1)===0.5,'5c on a SPY-scale chart the same ATR gives 0.5 points',f('SPY',1));
  f=mk([{}], 0.1, 10);
  ok(f('SPY',10)===null,'5d fewer than two closed bars -> null (atr() would have returned its 0.1 sentinel)',f('SPY',10));

  // levelMarkerOf with the band vs without it
  const LM=(band)=>{
    const g={ LVL_INPLAY_PTS:3, LVL_CLOSE_BARS:5 };
    g.nodeBandDisp=()=>band; g.reactDefence=()=>null; g.closedCandles=()=>[];
    const code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('levelMarkerOf')+'\nreturn levelMarkerOf;';
    return (new Function('__g',code))(g);
  };
  let lm=LM(5);
  ok(lm(7659,7655,'SPY',false,10)&&lm(7659,7655,'SPY',false,10).m==='IN PLAY','5e ES chart, band 5: a level 4 points away is IN PLAY (the old 3-point gate said no)');
  ok(lm(7661,7655,'SPY',false,10)===null,'5f ES chart, band 5: 6 points away is not');
  lm=LM(0.5);
  ok(lm(765.4,765.0,'SPY',false,1)&&lm(765.4,765.0,'SPY',false,1).m==='IN PLAY','5g SPY chart, band 0.5: 0.4 away is IN PLAY');
  const far=lm(767.9,765.0,'SPY',false,1);
  ok(far===null,'5h SPY chart, band 0.5: 2.9 away is NOT — the old fixed 3 marked this IN PLAY at ~5 ATR',far);
  const r=lm(765.4,765.0,'SPY',false,1);
  ok(r.band===0.5 && r.legacy===false,'5i the marker reports the band it used and that it was not the legacy constant',r);
  lm=LM(null);
  const lg=lm(767.9,765.0,'SPY',false,1);
  ok(lg && lg.m==='IN PLAY' && lg.legacy===true,'5j with no ATR yet it falls back to the legacy 3 AND SAYS SO',lg);

  // reactDefence gate
  const RD=(band)=>{
    const g={ LVL_INPLAY_PTS:3 };
    g.velOk=()=>true; g.nodeBandDisp=()=>band;
    g.tradeNodes=()=>[{ k:7660, es:7659, vel:{ d15:812000, d5:1000, d60:900000 }, velStale:false, velAge:3 }];
    g.velD=(v)=>({ txt:'x' });
    const code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('reactDefence')+'\nreturn reactDefence;';
    return (new Function('__g',code))(g);
  };
  let rd=RD(5)('SPY',7655);
  ok(rd && rd.verdict==='DEFENDING' && rd.band===5 && rd.dist===4,'5k reactDefence: node 4 points away inside a 5-point band -> DEFENDING, band and dist reported',rd);
  rd=RD(3)('SPY',7655);
  ok(rd===null,'5l ...and the same node is refused by a 3-point band — which is what the fixed gate did on every ES chart',rd);
}

function finish(){
  console.log('\n'+pass+' passed, '+fail+' failed');
  process.exit(fail?1:0);
}
