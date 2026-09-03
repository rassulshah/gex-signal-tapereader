// ============================================================================================
// test_v1557.js — (v15.57) THE LEVELS HE ASKED FOR, AND THE TWO-LINE RULE.
//   EM edges · VWAP + bands · today's developing profile · London range · HVL / magnet — in the read, in
//   the corpus (study-sweeps.py) and in the book study; the READ shows the two biggest levels run and names
//   the rest. Every function is EXTRACTED AND RUN; GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs'), cp=require('child_process');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));
const TIER=new Function(src.slice(src.indexOf('var LEVEL_TIER='), src.indexOf('function levelTier('))+' return LEVEL_TIER;')();
const RATE_MIN_N=15;
const PAL={ ink:'#e6edf3', sub:'#8b98a9', line:'#1e2530', card:'#12161f', gold:'#e3c341', blue:'#7cc7ff', amber:'#f2b45a', longAccent:'#2ec27e', shortAccent:'#f0616d' };
const esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
const W=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS.json','utf8'));
const Wslim={ corpus:W.corpus, lookup:W.lookup, ledger:W.ledger, cells:W.cells };
const WB=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS-BOOK.json','utf8'));
const bareP=s=>{ const out=[]; const re=/(\d+)%/g; let m; const txt=String(s).replace(/<[^>]+>/g,' '); while((m=re.exec(txt))){ const tail=txt.slice(m.index, m.index+60); if(!/n=|\(n |n \d|sessions\)|n\s*\d+\/\d+/.test(tail)) out.push(tail.slice(0,40)); } return out; };

// ---- 1 · tiers: his list is tier 1 ----------------------------------------------------------------
{
  const g={ LEVEL_TIER:TIER };
  const t=build(g,['levelTier'],'return levelTier;')(g);
  ok(['PDH','PDL','ONH','ONL','VAH','VAL','POC-','POC+','KING-','KING+','EMH','EML'].every(k=>t(k)===1),'1a PDH PDL ONH ONL VAH VAL POC KING EM edges are tier 1 (his list)');
  ok(t('PDC-')===2 && t('CW0+')===2 && t('IBL')===2 && t('LDNL')===2 && t('HVL-')===2 && t('MAG+')===2,'1b PDC, the walls, IB, London, HVL and the magnet are tier 2');
  ok(t('VWAP-')===3 && t('VW1L')===3 && t('DPOC+')===3 && t('PML')===3 && t('OR15H')===3,'1c the dynamic and minor levels are tier 3');
  ok(t('XYZ')===4,'1d an unknown level ranks last, never first');
}

// ---- 2 · the series: VWAP, bands, the developing profile, the dynamic scan ---------------------------
{
  const g={ PROF_VA:0.70, SWEEP_RECLAIM_MAX:30 };
  const F=build(g,['vwapSeries','devProfileSeries','sweepScan','sweepScanDynamic'],'return { vwapSeries, devProfileSeries, sweepScanDynamic };')(g);
  const bar=(o,h,l,c,v)=>[0,o,h,l,c,v];
  // two bars whose typical price differs from the close (h/l asymmetric), volumes 1 and 3
  const b1=bar(100,104,98,100,1), b2=bar(104,105,103,104,3);
  const tp=b=>(b[2]+b[3]+b[4])/3; const evw=(tp(b1)*1+tp(b2)*3)/4; const e2=(tp(b1)*tp(b1)*1+tp(b2)*tp(b2)*3)/4; const esd=Math.sqrt(e2-evw*evw);
  const VS=F.vwapSeries([b1,b2]);
  ok(Math.abs(VS[1].vwap-evw)<1e-9 && Math.abs(VS[1].sd-esd)<1e-9 && Math.abs(VS[0].vwap-tp(b1))<1e-9 && Math.abs(evw-103)>0.05,'2a VWAP is TYPICAL price × volume, cumulative (not the close); sd is the volume-weighted sd about it',{vs:VS,evw,esd});
  // developing profile: a volume spike at 110 makes it the POC; the VA is the 70% band grown from it
  const DP=F.devProfileSeries([bar(100,102,98,100,1), bar(110,110,110,110,100), bar(120,122,118,120,1)]);
  ok(DP[1].poc===110 && DP[2].poc===110 && DP[0].poc>=98 && DP[0].poc<=102,'2b the POC follows the volume; the VA grows from it',DP);
  ok(DP[2].val<=110 && DP[2].vah>=110,'2c the value area contains the POC');
  // dynamic scan: price above the level then dips through it -> a low-side event, level frozen at the sweep bar
  const rth=[bar(105,106,104,105,1), bar(105,106,104,105,1), bar(105,105,99,100,1), bar(100,103,99,102,1), bar(102,106,101,105,1)];
  const series=[100,100,101,102,103];   // the level rises bar to bar
  const ev=F.sweepScanDynamic(rth, series, 1, null);
  ok(ev && ev.low===true && ev.sweep===2 && ev.px===101 && ev.reclaim===3 && ev.ext===99,'2d the side is the side price came from (above -> low-side); the level is frozen at 101, the sweep bar’s value',ev);
  const ev2=F.sweepScanDynamic(rth, series, 1, false);
  ok(ev2===null || ev2.low===false,'2e forcing the high side never produces a low-side event');
  const flat=[bar(100,101,99,100,1), bar(100,101,99,100,1), bar(100,101,99,100,1)];
  ok(F.sweepScanDynamic(flat,[105,105,105],1,null)===null,'2f price never through the level -> no event');
}

// ---- 3 · the read: EM edges, London, HVL/magnet join the levels; two lines by tier --------------------
{
  const rows=[{ id:'CR0', disp:7700 },{ id:'PS0', disp:7650 },{ id:'FLIP', disp:7672 },{ id:'Mag', disp:7690 }];
  const nodes=[{ k:7660, es:7660, pct:-100, isKing:true }];
  const on=[]; // a full night: 17:00 -> 08:29 CT, with the London part (02:00-08:29) lower
  const day0=Date.UTC(2026,8,3,0,0)/1000; // any day; the reader uses UTC fields after -5h
  for(let m=17*60; m<24*60; m++) on.push([day0-5*3600+ (m-24*60)*60 + 24*3600 + 5*3600 - 24*3600, 7700, 7710, 7690, 7700, 1]);
  // simpler: build epochs so that (epoch-5*3600) maps to the wanted CT minute on the same date
  const ct=(h,mm)=>Math.floor(Date.UTC(2026,8,3,h,mm)/1000)+5*3600;
  const on2=[]; for(let m=0;m<8*60+30;m++){ const h=Math.floor(m/60), mm=m%60; const ldn=(m>=120); on2.push([ct(h,mm), 7700, ldn?7705:7710, ldn?7680:7690, 7700, 1]); }
  for(let m=17*60;m<24*60;m++){ on2.push([ct(Math.floor(m/60),m%60)-86400, 7700, 7710, 7690, 7700, 1]); }
  const rth=[[ct(8,30),7695,7697,7693,7696,1]];
  const g={ FUTMODE:{ fam:'ES', r:10 }, irtRatio:()=>({ r:10 }), ifLadder:()=>({ rows:rows }), tradeNodes:()=>nodes, SWEEP_IB_BARS:60,
            overnightHL:()=>({ onh:7710, onl:7680, n:on2.length, full:true }), futSessionBars:(o)=>(o===0?{ rth:rth, on:on2 }:null), priorProfile:()=>null,
            emBand:()=>({ ok:true, high:7740, low:7650 }) };
  const LV=build(g,['dispToEs','bookLevelsNow','sweepLevelsToday'],'return sweepLevelsToday("SPY");')(g);
  const by={}; LV.forEach(l=>{ by[l.name]=l; });
  ok(by.EMH && by.EMH.px===7740 && !by.EMH.low && by.EML && by.EML.px===7650 && by.EML.low,'3a the expected-move edges join the levels (EMH high-side, EML low-side)',Object.keys(by));
  ok(by.LDNL && by.LDNL.px===7680 && by.LDNH && by.LDNH.px===7705,'3b the London range is the 02:00-08:29 part of a full night (7680 / 7705, not the night’s 7690 / 7710)',{l:by.LDNL,h:by.LDNH});
  ok(by['HVL-'] && by['HVL-'].px===7672 && by['MAG-'] && by['MAG-'].px===7690,'3c HVL (the FLIP row) and the magnet (Mag row) join the book levels, side by position against the open 7695',{h:by['HVL-'],m:by['MAG-']});
  // two lines by tier: a deep tier-3 poke, a tier-1 PDL flush, a tier-2 IBL, a tier-1 ONH on the other side -> PDL and ONH shown, the rest named
  const mk=(evs)=>{ const g2={ PAL, RATE_MIN_N, g3esc:esc, LEVEL_TIER:TIER, sweepsLoad:()=>Wslim, sweepsBookLoad:()=>null, sweepEventsToday:()=>evs, bookLevelsNow:()=>({ walls:[], king:null, top5:[] }), tapZoneEs:()=>5,
      recorderLoad:()=>({}), recorderDay:()=>({ defl:{ SPY:[] } }), ANALYSIS_NIGHTLY:null };
    return build(g2,['levelTier','rateTxt','pctOf','ctrlTxt','statsRead'],'return statsRead("SPY");')(g2); };
  const E=(level,side,ext,depth,atBar,status)=>({ level, side, px:ext+depth*(side==='LOD'?1:-1), at:('0'+Math.floor(8+atBar/60)).slice(-2)+':'+('0'+((30+atBar)%60)).slice(-2), atBar, epoch:1700000000+atBar*60, bucket:atBar<30?'08:30-09:00':'09:00-10:00', ext, depth, speed:(status==='accepted'?null:6), status:status||'reclaimed' });
  const R=mk([ E('VW2L','LOD',7640,20,40), E('PDL','LOD',7650,8,12), E('IBL','LOD',7655,6,70), E('ONH','HOD',7720,4,50) ]);
  const sweeps=R.lines.filter(l=>l.kind==='sweep');
  ok(sweeps.length===2 && /^PDL swept/.test(sweeps[0].head) && /^ONH swept/.test(sweeps[1].head),'3d two lines, the biggest levels first: PDL and ONH, not the deeper VWAP −2σ poke nor the IBL',sweeps.map(l=>l.head));
  const rest=R.lines.find(l=>l.kind==='rest');
  ok(rest && /also swept today, lesser levels: /.test(rest.txt) && /VW2L/.test(rest.txt) && /IBL/.test(rest.txt) && !/PDL/.test(rest.txt),'3e the rest are named in one trailer, the shown ones are not repeated',rest&&rest.txt);
  ok(R.of===6,'3f the alignment count covers the two shown lines only (3 conditions each)',R.of);
  // a broken tier-1 level outranks a reclaimed tier-2 one
  const R2=mk([ E('PDH','HOD',7712,12,20,'accepted'), E('IBH','HOD',7705,5,80), E('CW0+','HOD',7708,3,90) ]);
  const s2=R2.lines.filter(l=>l.kind==='sweep');
  const r2=R2.lines.find(l=>l.kind==='rest');
  ok(s2.length===2 && s2.some(l=>/^IBH swept/.test(l.head)) && s2.some(l=>/^CW0\+ swept/.test(l.head)) && r2 && /PDH 08:50 \(broke\)/.test(r2.txt),'3g (v15.58) the two RECLAIMED excursions are read; the broken PDH is named in the trailer as (broke)',{shown:s2.map(l=>l.head),rest:r2&&r2.txt});
  // (v15.58) reclaimed excursions come first; a level the session OPENED beyond is not a sweep and ranks last
  const R3=mk([ E('PDH','HOD',7712,12,0,'accepted'), E('VAH','HOD',7705,40,0,'accepted'), E('IBL','LOD',7655,6,70), E('VW2L','LOD',7640,20,40) ]);
  const s3=R3.lines.filter(l=>l.kind==='sweep');
  ok(s3.length===2 && s3.some(l=>/^IBL swept/.test(l.head)) && s3.some(l=>/^VW2L swept/.test(l.head)),'3i two reclaimed excursions outrank two gap-open breaks, whatever the tier',s3.map(l=>l.head));
  const r3=R3.lines.find(l=>l.kind==='rest');
  ok(r3 && /PDH 08:30 \(opened beyond\)/.test(r3.txt) && /VAH 08:30 \(opened beyond\)/.test(r3.txt),'3j a level the session opened beyond is named "(opened beyond)", never "broke"',r3&&r3.txt);
  const R4=mk([ E('PDH','HOD',7712,12,0,'accepted') ]);
  ok(/the session OPENED 12\.00 pts beyond it and never came back: not a sweep/.test(R4.lines[0].head),'3k …and when it is all there is, the line says so instead of "swept 08:30"',R4.lines[0].head);
  ok(bareP(R.lines.map(l=>(l.head||'')+' '+l.txt+' '+(l.node||'')).join(' ')).length===0,'3h no bare % in any line');
}

// ---- 4 · the corpus and the book carry the new levels -------------------------------------------------
{
  const L=W.lookup.level;
  ok(['LDNL','LDNH','VWAP-','VWAP+','VW1L','VW1H','VW2L','VW2H','DPOC-','DPOC+','DVAL','DVAH'].every(k=>L[k] && L[k].n>=15),'4a SWEEPS.json carries London, the VWAP and its bands, and today’s profile, each with n ≥ 15',Object.keys(L).length);
  ok(L['VW1L'].rate<0.15 && L['VW1L'].fresh>0.2 && L['DPOC-'].rate<0.10,'4b the interior levels are NOT the extreme: VWAP −1σ '+Math.round(100*L['VW1L'].rate)+'% (n='+L['VW1L'].n+') vs '+Math.round(100*L['VW1L'].fresh)+'% control; today’s POC '+Math.round(100*L['DPOC-'].rate)+'% (n='+L['DPOC-'].n+')');
  ok(WB.lookup.level['HVL-'] && WB.lookup.level['MAG+'] && WB.lookup.level['HVL-'].n>0,'4c SWEEPS-BOOK.json carries HVL and the magnet as book levels');
  const r=cp.spawnSync('python3',['tools/study-sweeps-book.py','--selftest'],{encoding:'utf8'});
  ok(r.status===0,'4d the book study self-test still passes with the two new levels');
  const S=JSON.parse(fs.readFileSync('learning/studies.json','utf8'));
  const flat=[]; S.subjects.forEach(sj=>sj.subsections.forEach(ss=>ss.studies.forEach(x=>flat.push(x))));
  ok(['H2.10h','H2.10i','H2.10j','H2.10k','H2.10l'].every(id=>flat.some(x=>x.id===id)) && /PULLBACK candidate/.test(flat.find(x=>x.id==='H2.10h').result),'4e the registry carries the five level families, and says a VWAP-band sweep is a pullback candidate, not the extreme');
  // the H2 table renders the new rows
  const g={ PAL, RATE_MIN_N, g3esc:esc, tabEmpty:t=>'<div>'+t+'</div>', sweepsBookLoad:()=>({ corpus:WB.corpus, lookup:WB.lookup }) };
  const h=build(g,['rateTxt','sweepTableHtml','panSection','panNote','panRow'],'return sweepTableHtml(__g.W);')(Object.assign({W:Wslim},g));
  ok(/LDNL → LOD/.test(h) && /VWAP −1σ → LOD/.test(h) && /today’s POC ↓ → LOD/.test(h) && /HVL \(zero γ\)/.test(h) && /magnet/.test(h),'4f the H2 table shows London, the VWAP bands, today’s profile, HVL and the magnet');
  ok(bareP(h).length===0,'4g no bare % in the table',bareP(h));
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
