// test_v1581.js — (v15.81) THE IRT EXPORT ON SKYLIT'S OWN FUTURES PRICES (R-23). Operator, 2026-09-08: "yes, i want them
// to match skylits own ES1 prices". The ES1 / NQ1 gex/levels feeds are all derived — every SPY / SPXW / QQQ strike already
// at the futures price by Skylit's live ratio. The observer keeps them (LASTFUTDER), ensureFeeds self-fetches the one the
// app is not asking for, skylitFutPx answers "what price does Skylit draw for this strike", and the export writes it —
// the panel's own basis only as the fallback, tilde-tagged. Fixture numbers are the 10:25 CT reading of 2026-09-08
// (SKYLIT-FEEDS § THE ES1 BOOK): SPXW ratio 1.000578, SPY ratio 10.026042, 7700 → 7704.45, 770 → 7720.05.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }
function exVar(n){ const m=src.match(new RegExp('var\\s+'+n+'\\s*=\\s*([\\s\\S]*?);\\n')); return m?m[1]:null; }

// ---------- 0. the version ----------
ok(/@version\s+15\.(8[1-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[1-9]|9\d)';/.test(src), '0a v15.81 or later in both spots');

// ---------- 1. the state and the capture ----------
{
  ok(/^var LASTFUTDER = \{\};/m.test(src) && /^var FUTDER_STALE_MS = 300000;/m.test(src) && /^var SKY_FUT = \{ ES:'ES1', NQ:'NQ1' \};/m.test(src), '1a LASTFUTDER, FUTDER_STALE_MS (5 min) and SKY_FUT declared bare (exVar-readable)');
  const onf=ex('onFeed');
  ok(/if\(\(sym==='ES1' \|\| sym==='NQ1'\) && feed==='gamma' && j && j\.derived && j\.derived\.length\)\{/.test(onf) && /if\(!\(heldFT!=null && newFT!=null && newFT < heldFT-90\)\) LASTFUTDER\[sym\]=\{ j:j, ts:Date\.now\(\) \};/.test(onf), '1b onFeed keeps an ES1 / NQ1 GAMMA payload with derived[], never history over live (the 90 s rule on the derived series)');
  const iSeen=onf.indexOf('SYM_SEEN[sym]'), iFut=onf.indexOf("sym==='ES1'"), iRet=onf.indexOf('return;', iFut);
  ok(iSeen>0 && iFut>iSeen && iRet>iFut && iRet<onf.indexOf("if(!j || !j.levels || !j.levels.length) return;"), '1c …inside the non-SPY/QQQ branch: still counted in SYM_SEEN, still never reaches the read/record pipeline');
  // run it
  global.LASTSPXW=null; global.LASTFUTDER={}; global.SYM_SEEN={}; global.LASTDISP={}; global.LASTVEX={}; global.LASTFEED={}; global.FEED_REJECTS={SPY:{n:0},QQQ:{n:0}};
  eval(['feedNewestT','futDerNewestT','onFeed'].map(ex).join('\n'));
  const der=(t)=>({ success:true, levels:[], level_count:0, derived:[
    { source:'SPY',  ratio:10.026042, levels:[{ t:t, s:7699.95, l:[{k:7720.05,v:127712000,d:-1,net:-127712000},{k:7689.97,v:125000000,d:1,net:125000000},{k:7710.03,v:54056000,d:1,net:54056000}] }] },
    { source:'SPXW', ratio:1.000578,  levels:[{ t:t, s:7699.94, l:[{k:7704.45,v:125468000,d:1,net:125468000},{k:7709.46,v:118000000,d:1,net:118000000},{k:7684.44,v:32600000,d:-1,net:-32600000}] }] },
    { source:'SPX',  ratio:1.000578,  levels:[{ t:t, s:7699.94, l:[{k:7754.48,v:21959000,d:1,net:21959000}] }] } ] });
  onFeed('ES1','gamma',der(1788881100));
  ok(LASTFUTDER.ES1 && LASTFUTDER.ES1.j.derived.length===3 && typeof LASTFUTDER.ES1.ts==='number', '1d a live ES1 gamma payload is kept');
  onFeed('ES1','gamma',der(1788870000));
  ok(LASTFUTDER.ES1.j.derived[0].levels[0].t===1788881100, '1e a HISTORICAL ES1 payload (newest snapshot 3 h older) does not replace it');
  onFeed('ES1','gamma',der(1788881160));
  ok(LASTFUTDER.ES1.j.derived[0].levels[0].t===1788881160, '1f a fresher one does');
  onFeed('ES1','vanna',der(1788881220));
  ok(LASTFUTDER.ES1.j.derived[0].levels[0].t===1788881160, '1g vanna is not kept');
  onFeed('NQ1','gamma',{ success:true, levels:[], derived:[{ source:'QQQ', ratio:41.196, levels:[{ t:1788881160, s:29600, l:[{k:29750.7,v:60000000,d:1,net:60000000}] }] }] });
  ok(LASTFUTDER.NQ1 && LASTFUTDER.NQ1.j.derived[0].source==='QQQ' && SYM_SEEN.ES1.n===4 && SYM_SEEN.NQ1.n===1, '1h NQ1 kept beside ES1; SYM_SEEN still counts every futures payload');
  onFeed('USO','gamma',der(1788881300));
  ok(!LASTFUTDER.USO, '1i any other symbol is still dropped');
}

// ---------- 2. skylitFutPx ----------
{
  eval(ex('skylitFutPx'));
  global.FUTDER_STALE_MS=300000;
  const P=skylitFutPx('ES1','SPXW',7700);
  ok(P && P.px===7704.45 && P.src==='row' && Math.abs(P.ratio-1.000578)<1e-9, '2a SPXW 7700 → the derived row\'s k, 7704.45 (Skylit\'s own price), src row', P);
  const Q=skylitFutPx('ES1','SPY',770);
  ok(Q && Q.px===7720.05 && Q.src==='row', '2b SPY 770 → 7720.05', Q);
  const R=skylitFutPx('ES1','SPXW',7695);
  ok(R && R.src==='ratio' && Math.abs(R.px-7695*1.000578)<1e-6, '2c a strike the window does not carry → strike × the SAME ratio (7699.45)', R);
  const F=skylitFutPx('ES1','SPXW',7695.3);
  ok(F && F.src==='ratio' && Math.abs(F.px-7695.3*1.000578)<1e-6, '2d a level that is not a strike (the IF flip) → the ratio path too');
  ok(skylitFutPx('ES1','QQQ',770)===null && skylitFutPx('NQ1','SPY',770)===null, '2e a book the payload does not carry → null (no invention)');
  ok(skylitFutPx('ES1','SPY',NaN)===null && skylitFutPx('ES1','SPY','770')===null, '2f a bad strike → null');
  const keep=LASTFUTDER.ES1.ts; LASTFUTDER.ES1.ts=Date.now()-301000;
  ok(skylitFutPx('ES1','SPXW',7700)===null, '2g a payload older than five minutes → null (the caller falls back and says ~)');
  LASTFUTDER.ES1.ts=keep;
  ok(skylitFutPx('ZZ1','SPXW',7700)===null, '2h no payload for the symbol → null');
  ok(Math.abs(7704.45/1.000578-7700)<0.02 && Math.abs(7720.05/10.026042-770)<0.02, '2i the match is k / ratio ≈ strike (both fixtures land within 0.02)');
}

// ---------- 3. the self-fetch of the other futures book ----------
{
  const sff=ex('selfFetchFut'), ef=ex('ensureFeeds');
  ok(/url=url\.replace\(\/symbol=\[\^&\]\*\/, 'symbol='\+futSym\)/.test(sff) && /'data_type=gamma'/.test(sff) && /'exp_mode=current'/.test(sff) && /'exp_count=1'/.test(sff) && /include_derived=true/.test(sff), '3a selfFetchFut: the app\'s own last URL with symbol / gamma / the 0DTE window / include_derived=true');
  ok(/if\(FUTDER_SELF_LAST\[futSym\] && \(now-FUTDER_SELF_LAST\[futSym\]\)<60000\) return;/.test(sff) && /onFeed\(futSym, 'gamma', j, true\)/.test(sff) && /if\(res && res\.status===401\) return;/.test(sff), '3b …once a minute per symbol, lands in onFeed as a self-fetch, a 401 is not retried');
  ok(/var irtOn=false; try\{ irtOn=!!\(CFG && CFG\.irt && CFG\.irt\.on\); \}catch\(eCI\)\{\}/.test(ef) && /if\(irtOn\)\{/.test(ef) && /var futs=\['ES1','NQ1'\];/.test(ef) && /if\(!LASTFUTDER\[fs\] \|\| \(now-LASTFUTDER\[fs\]\.ts\)>60000\) selfFetchFut\(fs\);/.test(ef), '3c ensureFeeds asks for ES1 and NQ1 only while the IRT export is on, only when the held payload is over a minute old');
  // run ensureFeeds: the export off → nothing; on → the stale one fetched
  let fetched=[]; global.LASTFEEDURL='https://app.skylit.ai/tv/api/gex/levels?symbol=ES1&data_type=vanna&nodes=60&exp_mode=next_n&exp_count=4&extended=false&with_snapshot=true&with_slices=true&include_derived=true&dates=2026-09-08&v=1778049696';
  global.LASTAUTH=null; global.SELF_LAST={}; global.SELF_MIN_MS=4000; global.FEED_STALE_MS=12000;
  global.fetch=(u)=>{ fetched.push(u); return { then:()=>({ catch:()=>{} }) }; };
  global.selfFetch=()=>{}; global.panelVisible=()=>true; global.document={ visibilityState:'visible' };
  global.FUTDER_SELF_LAST={};
  eval(['selfFetchFut','ensureFeeds'].map(ex).join('\n'));
  global.CFG={ irt:{ on:false } }; ensureFeeds();
  ok(fetched.length===0, '3d the export off → no futures fetch');
  global.CFG={ irt:{ on:true } }; LASTFUTDER.ES1.ts=Date.now(); delete LASTFUTDER.NQ1; ensureFeeds();
  ok(fetched.length===1 && /symbol=NQ1/.test(fetched[0]) && /data_type=gamma/.test(fetched[0]) && /exp_mode=current&exp_count=1/.test(fetched[0]) && /include_derived=true/.test(fetched[0]) && !/symbol=ES1/.test(fetched[0]), '3e the export on, ES1 fresh, NQ1 missing → ONE fetch, NQ1, gamma, 0DTE, derived', fetched);
  ensureFeeds();
  ok(fetched.length===1, '3f …and not again within the minute');
  LASTFUTDER.ES1.ts=Date.now()-61000; FUTDER_SELF_LAST={}; ensureFeeds();
  ok(fetched.length===3 && fetched.some(u=>/symbol=ES1/.test(u)), '3g ES1 over a minute old → fetched too');
}

// ---------- 4. the export writes Skylit's prices ----------
{
  global.window={__gptsDebug:{}};
  eval(v('IRT_HEADER')); eval(v('IRT_LAST'));
  eval(['irtColor'].map(ex).join('\n')); eval(v('IRT_COLORS'));
  eval(v('IRT_RATIO_KEY')); eval(v('IRT_NQRATIO_KEY')); eval(v('KING_LATCH_KEY'));
  eval(src.match(/var KING_LATCH_MS=\d+/)[0]+';');
  eval(v('IRT_QQQK_KEY')); eval(v('IRT_KINGS_KEY'));
  eval(['irtRound','irtCsvRow','irtRatio','irtNqRatio','kingLatchTick','irtKingLatch','irtKingHeld','irtGLatch','irtGHeld','irtQqqKing','irtQqqTop','irtBuildCsv'].map(ex).join('\n'));
  global.SKY_FUT={ ES:'ES1', NQ:'NQ1' };
  global.emBand=()=>({ ok:true, now:7700, nowLive:7700, scaleUsed:10.0268 });
  global.ladderKings=()=>[];
  global.ES_RATIO=10.05; global.NQ_RATIO=41.36; global.FEED_STALE_MS=12000;
  var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
  global.CFG={ nodeThresh:20, irt:{ on:true, secs:180, futSym:'EPU26', etfSym:'SPY', file:'FlexLevelsExport.csv', nqOn:true, nqSym:'ENQU26', nqRatio:41.9, lines:'all' } };   // (v15.92) every family — Kings + walls is test_v1592
  global.FUTMODE={ fam:'ES', r:10.0268, live:true };
  global.ctTodayStr=()=>'2026-09-08';
  // the panel's own basis: dispScale 1.00065 (the IF spot, +5.0 at 7700) — Skylit's is 1.000578 (+4.45)
  const IFL_ROWS=[{id:'CR0', k:7750, disp:7755.04, und:773.2},{id:'PS0', k:7650, disp:7654.97, und:763.2}];
  global.ifLadder=(sym)=>({ dispScale:1.00065, undScale:0.0998, rows:IFL_ROWS, err:null, srcSym:(sym==='QQQ'?'QQQ':'SPX') });
  global.ifChain=()=>({ dte0:{ gf:{ flip:7695.3 } } });
  const SPXW_TAPE=()=>({ king:7700, pct:{ '7700.00':100, '7705.00':94, '7680.00':-26, '7695.00':18, '7675.00':-15 } });
  global.tapeMap=(s)=>(s==='QQQ'?({ king:722, count:20, fromFeed:false, pct:{ '722.00':100, '721.00':44 } }):SPXW_TAPE());
  global.ladderFor=()=>null;
  global.LASTFEED={ SPY:{ ts:Date.now(), j:{ levels:[{ s:768 }] } }, QQQ:{ ts:Date.now(), j:{ levels:[{ s:722 }] } } };
  global.extractWalls=()=>({ king:770, walls:[{k:770, pct:100, pos:false}] });
  LASTFUTDER.ES1.ts=Date.now();
  LASTFUTDER.NQ1={ ts:Date.now(), j:{ levels:[], derived:[{ source:'QQQ', ratio:41.196, levels:[{ t:1788881160, s:29600, l:[{k:29743.512,v:60000000,d:1,net:60000000}] }] }] } };
  const price=(lines,sym,lbl)=>{ const l=lines.find(x=>x.startsWith(sym+',') && x.split(',')[2]===lbl); return l?+l.split(',')[1]:null; };
  const label=(lines,sym,re)=>lines.filter(x=>x.startsWith(sym+',') && re.test(x.split(',')[2])).map(x=>x.split(',')[2]);

  const b=irtBuildCsv(); const L=b.csv.trim().split('\r\n');
  ok(price(L,'EPU26','SPXW KING')===7704.5, '4a SPXW KING on EPU26 = Skylit\'s 7704.45 → 7704.50 at the tick (the basis would have said 7705.00)', price(L,'EPU26','SPXW KING'));
  ok(price(L,'EPU26','XG2')===7709.5 && price(L,'EPU26','XG3')===7684.5, '4b XG2 (7705) 7709.46 → 7709.50 and XG3 (7680) 7684.44 → 7684.50 — derived rows (v15.85 labels)', [price(L,'EPU26','XG2'),price(L,'EPU26','XG3')]);
  ok(price(L,'EPU26','XG4')===7699.5 && price(L,'EPU26','XG5')===7679.5, '4c XG4 (7695) and XG5 (7675) are not in the 0DTE window → strike × Skylit\'s ratio (7699.45 → 7699.50, 7679.44 → 7679.50)', [price(L,'EPU26','XG4'),price(L,'EPU26','XG5')]);
  ok(price(L,'EPU26','SPY KING')===7720.0, '4d SPY KING = Skylit\'s 7720.05 → 7720.00 (the panel\'s EMA would have said 7720.50)', price(L,'EPU26','SPY KING'));
  ok(price(L,'EPU26','CW0')===7754.5 && price(L,'EPU26','PW0')===7654.5, '4e the IF 0DTE walls (SPX 7750 / 7650) on the SAME SPXW ratio: 7754.48 → 7754.50, 7654.42 → 7654.50 — one ruler', [price(L,'EPU26','CW0'),price(L,'EPU26','PW0')]);
  ok(price(L,'EPU26','FLIP0')===7699.75, '4f FLIP0 (SPX 7695.3, not a strike) by the ratio path: 7699.75', price(L,'EPU26','FLIP0'));
  ok(label(L,'EPU26',/~/).length===0, '4g no ES row wears the ~ — every one is Skylit\'s own price');
  ok(price(L,'SPY','SPXW KING')===+(7700*1.00065/10.0268).toFixed(2) && price(L,'SPY','SPY KING')===770, '4h the ETF symbol is untouched: SPY row-space as before', [price(L,'SPY','SPXW KING'),price(L,'SPY','SPY KING')]);
  ok(price(L,'ENQU26','QQQ KING')===29743.5 && label(L,'ENQU26',/QQQ KING/)[0]==='QQQ KING', '4i the NQ King on NQ1\'s derived QQQ row 29743.51 → 29743.50, no ~', price(L,'ENQU26','QQQ KING'));
  ok(price(L,'ENQU26','G2')===irtRound(721*41.196,0.25) && label(L,'ENQU26',/^G2/)[0]==='G2', '4j the NQ G2 (721, not in the window) = 721 × Skylit\'s QQQ ratio, no ~', price(L,'ENQU26','G2'));
  ok(/9 of 9 ES rows on Skylit’s ES1 prices \(\d+ s old · SPXW ratio 1\.000578 · SPY ratio 10\.026042\)/.test(IRT_LAST.skyWhy||''), '4k IRT_LAST.skyWhy names the count (9 of 9: 2 Kings, 4 G, 3 IF), the age and both ratios', IRT_LAST.skyWhy);
  ok(/2 of 2 NQ rows on Skylit’s NQ1 prices \(\d+ s old · QQQ ratio 41\.196000\)/.test(IRT_LAST.nqSkyWhy||''), '4l IRT_LAST.nqSkyWhy likewise', IRT_LAST.nqSkyWhy);
  // the latch carries the source strike, so a HELD King still takes Skylit's price
  const held=JSON.parse(LS[IRT_KINGS_KEY]);
  ok(held.SPXW && held.SPXW.src===7700 && held.SPY && held.SPY.src===770, '4m irtKingLatch stores the strike in its own book (SPX 7700, SPY 770) beside the SPY-row-space k', held.SPXW);
  global.tapeMap=(s)=>(s==='QQQ'?({ king:722, count:20, fromFeed:false, pct:{ '722.00':100, '721.00':44 } }):null);
  const b2=irtBuildCsv(); const L2=b2.csv.trim().split('\r\n');
  ok(price(L2,'EPU26','SPXW KING')===7704.5 && price(L2,'EPU26','XG2')===7709.5 && /^held/.test(IRT_LAST.spxWhy) && /^held/.test(IRT_LAST.gWhy), '4n with the tape gone the HELD King and G rows still print at Skylit\'s prices (the hold carries the strikes)', [price(L2,'EPU26','SPXW KING'), IRT_LAST.spxWhy]);
  global.tapeMap=(s)=>(s==='QQQ'?({ king:722, count:20, fromFeed:false, pct:{ '722.00':100, '721.00':44 } }):SPXW_TAPE());

  // ---- the fallback: no ES1 payload → the panel's own basis, tilde-tagged when the ratio is not live ----
  const keepES=LASTFUTDER.ES1; delete LASTFUTDER.ES1; delete LASTFUTDER.NQ1;
  const b3=irtBuildCsv(); const L3=b3.csv.trim().split('\r\n');
  ok(price(L3,'EPU26','SPXW KING')===7705.0 && price(L3,'EPU26','SPY KING')===irtRound(770*10.0268,0.25), '4o no ES1 payload → the old conversion (dispScale 1.00065 → 7705.00; the EMA ratio for SPY)', [price(L3,'EPU26','SPXW KING'), price(L3,'EPU26','SPY KING')]);
  ok(/^no ES1 payload yet/.test(IRT_LAST.skyWhy) && /^no NQ1 payload yet/.test(IRT_LAST.nqSkyWhy), '4p …and skyWhy / nqSkyWhy say so', [IRT_LAST.skyWhy, IRT_LAST.nqSkyWhy]);
  ok(label(L3,'EPU26',/~/).length===0, '4q the ratio is live (FUTMODE.live) → still no ~ on the fallback (the tag rule is unchanged)');
  global.FUTMODE={ fam:'ES', r:10.0268, live:false }; LS[IRT_RATIO_KEY]=JSON.stringify({ r:10.0268, t:Date.now() });
  const b4=irtBuildCsv(); const L4=b4.csv.trim().split('\r\n');
  ok(label(L4,'EPU26',/~/).length>0, '4r …the ratio not live → the fallback rows wear the ~');
  LASTFUTDER.ES1=keepES; LASTFUTDER.ES1.ts=Date.now();
  const b5=irtBuildCsv(); const L5=b5.csv.trim().split('\r\n');
  ok(label(L5,'EPU26',/~/).length===0 && price(L5,'EPU26','SPXW KING')===7704.5, '4s …and with the ES1 payload back, Skylit\'s price and no ~ even though the panel\'s ratio is not live — the row does not need it');
  // stale payload → fallback (the ratio live again, so the fallback is the dispScale path: 7705.00)
  global.FUTMODE={ fam:'ES', r:10.0268, live:true };
  LASTFUTDER.ES1.ts=Date.now()-301000;
  const b6=irtBuildCsv(); const L6=b6.csv.trim().split('\r\n');
  ok(price(L6,'EPU26','SPXW KING')===7705.0 && /0 of 9 ES rows on Skylit/.test(IRT_LAST.skyWhy) && /the rest on the basis/.test(IRT_LAST.skyWhy), '4t a payload older than five minutes → the basis, and skyWhy says 0 of 9', IRT_LAST.skyWhy);
  LASTFUTDER.ES1.ts=Date.now();
  // a QQQ-sourced IF ladder never takes the SPXW ratio
  global.ifLadder=(sym)=>({ dispScale:1.00065, undScale:0.0998, rows:IFL_ROWS, err:null, srcSym:'QQQ' });
  const b7=irtBuildCsv(); const L7=b7.csv.trim().split('\r\n');
  ok(price(L7,'EPU26','CW0')===irtRound(7755.04/10.0268*10.0268,0.25) && /6 of 9 ES rows/.test(IRT_LAST.skyWhy), '4u an IF ladder off the QQQ chain keeps its own conversion for its three rows (6 of 9 on Skylit)', [price(L7,'EPU26','CW0'), IRT_LAST.skyWhy]);
  global.ifLadder=(sym)=>({ dispScale:1.00065, undScale:0.0998, rows:IFL_ROWS, err:null, srcSym:'SPX' });
  ok(/window\.__gptsDebug\.futDer=function\(\)/.test(src), '4v __gptsDebug.futDer() exposes the books (age, sources, ratios)');
  ok(/\+\(IRT_LAST\.skyWhy\?\(' \\u00b7 '\+g3esc\(IRT_LAST\.skyWhy\)\):''\)\+\(IRT_LAST\.nqSkyWhy\?\(' \\u00b7 '\+g3esc\(IRT_LAST\.nqSkyWhy\)\):''\)/.test(src), '4w the gear\'s IRT status line says which ruler the file is on (skyWhy · nqSkyWhy)');
}

// ---------- 5. the record ----------
{
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r23=R.rows.find(r=>r.id==='R-23');
  ok(r23 && r23.status==='implemented' && r23.version==='15.81' && /ES1/.test(r23.text), '5a R-23 on Rec, implemented in v15.81', r23&&[r23.status,r23.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '5b REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.81/.test(cl) && cl.indexOf('## v15.81')<cl.indexOf('## v15.80'), '5c the CHANGELOG has the v15.81 entry on top');
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const r81=P.roadmap.find(r=>r.v==='15.81'), r82=P.roadmap.find(r=>r.v==='15.82');
  const seas=P.roadmap.find(r=>/SEASONALITY TRACKED/.test(r.title));
  ok(r81 && /SKYLIT'S OWN FUTURES PRICES/.test(r81.title) && (r81.status==='next' || r81.status==='shipped') && seas && +seas.v>15.81 && P.roadmap.filter(r=>r.status==='next').length===1, '5d the plan: v15.81 is this build (the seasonality moved past it; exactly one "next")', r81&&[r81.status, seas&&seas.v]);
  const seedP=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedP)===JSON.stringify(P), '5d2 PLAN_SEED equals the file');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.81/.test(ls.slice(logAt>=0?logAt:0)), '5e the lesson log carries the v15.81 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(8[1-9]|9\d)/.test(rn.slice(0,600)) && /ES1/.test(rn), '5f the resume note is at v15.81 or later and names the ES1 book');
  const fd=fs.readFileSync('session-state/SKYLIT-FEEDS.md','utf8'); ok(/## THE ES1 BOOK AND THE PROJECTION FEATURE/.test(fd), '5g SKYLIT-FEEDS carries the ES1 book and the projection');
  const dc=fs.readFileSync('session-state/DECISIONS.md','utf8'); ok(/skylits own ES1 prices/.test(dc), '5h DECISIONS records his words');
}

console.log('test_v1581: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
