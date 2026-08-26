// (v11.4; rewritten v14.9, v14.12, v14.20) IRT FLEXLEVELS EXPORT — format pinned to the user's own
// sample; CONTENT pinned to the operator's 2026-08-27 step-back: "too many levels — only the kings."
//   THE FILE IS THREE LINES: SPXW KING + SPY KING as EPU26, QQQ KING as ENQU26.
//   Everything the export used to carry — rail nodes, SUCC, IF walls, percentage rows — is
//   asserted ABSENT so it cannot creep back silently. The panel keeps all of it on screen.
// Kept machinery (each operator-verified): grammar (v14.12), RGB colours (v14.14), chart-frame-
// independent SPXW conversion (v14.14), latched crown (v14.19), SPY King from the self-fetched
// book (v14.19), QQQ King from the 0DTE ladder w/ feed-fallback rejection (v14.15), NQ ratio
// chain (v14.13), ES ratio chain (v11.4.1).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }
global.window={__gptsDebug:{}};
eval(v('IRT_HEADER')); eval(v('IRT_LAST'));
eval(['irtColor'].map(ex).join('\n')); eval(v('IRT_COLORS'));
eval(v('IRT_RATIO_KEY')); eval(v('IRT_NQRATIO_KEY'));
eval(v('KING_LATCH_KEY'));
eval(src.match(/var KING_LATCH_MS=\d+/)[0]+';');   // trailing comment defeats the v() grab
eval(['irtRound','irtCsvRow','irtRatio','irtNqRatio','kingLatchTick','irtBuildCsv'].map(ex).join('\n'));
global.ES_RATIO=10.05; global.NQ_RATIO=41.36; global.FEED_STALE_MS=12000;
var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
global.CFG={ nodeThresh:20, irt:{ on:true, secs:180, futSym:'EPU26', etfSym:'SPY', file:'FlexLevelsExport.csv',
                                  nqOn:true, nqSym:'ENQU26', nqRatio:41.9 } };
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
global.ctTodayStr=()=>'2026-08-27';

// ---- sources, stubbed ----
global.ifLadder=()=>({ dispScale:1.0023 });
const SPXW_TAPE=()=>({ king:7710, pct:{ '7710.00':100, '7630.00':85, '7650.00':41 } });
let QQQ_TAPE=()=>({ king:650, count:20, fromFeed:false, pct:{ '650.00':-100, '648.00':44 } });
global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE());
global.LASTFEED={ SPY:{ ts:Date.now(), j:{} } };
global.extractWalls=()=>({ king:765, walls:[{k:765, pct:100, pos:false}] });

// ---------- 1. the whole file is three kings ----------
const b=irtBuildCsv();
ok(!!b, '0 builds');
const lines=b.csv.trim().split('\r\n');
ok(lines[0]==='SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH,PENSTYLE,bDRAWTEXT,bDRAWPRICE,LABELPOS,bCUSTPOS,CUSTPOSALLMARGIN,CUSTPOSLEFTRIGHT,CUSTPOSUNITS,CUSTPOSWIDTH,bBANDS,BANDPENCOLOR,BANDPENWIDTH,BANDPENSTYLE,BANDABOVEBEL,BANDUNITS,BANDPRICE,bBANDS2,BAND2PENCOLOR,BAND2ABOVEBEL,BAND2UNITS,BAND2PRICE,bBANDLABELS,bTRANSLUCENT', '1a header verbatim from the sample');
// 2 king rows × 2 configured symbols (EPU26 + ETF SPY) + 1 NQ king
ok(b.n===3 && lines.length===1+2*2+1, '1b n=3 kings; rows = 2×2 syms + 1 NQ', [b.n,lines.length]);
const eRows=lines.filter(l=>l.startsWith('EPU26,'));
const qRows=lines.filter(l=>l.startsWith('ENQU26,'));
ok(eRows.length===2 && qRows.length===1, '1c EPU26 carries exactly TWO kings; ENQU26 exactly ONE', [eRows.length,qRows.length]);
ok(lines.slice(1).every(l=>l.split(',').length===28), '1d every row keeps exactly 28 columns');

// ---------- 2. the SPXW king ----------
const kx=eRows.find(l=>/SPXW KING 100%/.test(l));
ok(!!kx, '2a SPXW KING present');
ok(kx && /^EPU26,7727\.750000,/.test(kx), '2b SPX 7710 → ES 7727.75 (dispScale path, 0.25 tick)', kx);
ok(kx && kx.split(',')[3]===String((227<<16)+(195<<8)+65), '2c +gamma crown wears the full yellow (RGB)', kx&&kx.split(',')[3]);
ok(kx && kx.split(',')[4]==='3', '2d drawn heaviest');
{ // chart-frame independence survives the trim (the v14.14 lesson)
  global.FUTMODE={ chart:'SPXW', fam:'SPX', r:1, live:true };
  global.ifLadder=()=>({ dispScale:1.0, undScale:0.099773 });
  const Bc=irtBuildCsv(); const kc=Bc.csv.split('\r\n').find(l=>/SPXW KING/.test(l)&&l.startsWith('EPU26,'));
  ok(kc && Math.abs(parseFloat(kc.split(',')[1]) - 7733.9) <= 0.25, '2e on the SPXW CASH chart the king goes SPX→SPY→ES, never raw SPX', kc&&kc.split(',')[1]);
  global.FUTMODE={ fam:'ES', r:10.0538, live:true };
  global.ifLadder=()=>({ dispScale:1.0023 }); }
{ // the LATCHED crown exports, not a mid-flap blip (the v14.19 lesson)
  const T2=()=>({ king:7700, pct:{ '7700.00':-100, '7710.00':96 } });   // crown just flipped
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():T2());
  const Bf=irtBuildCsv(); const kf=Bf.csv.split('\r\n').find(l=>/SPXW KING/.test(l)&&l.startsWith('EPU26,'));
  ok(kf && /^EPU26,7727\.750000,/.test(kf), '2f a fresh flap does NOT move the exported king — the latch holds 7710', kf);
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE()); LS[KING_LATCH_KEY]=undefined; delete LS[KING_LATCH_KEY]; }

// ---------- 3. the SPY king ----------
const ky=eRows.find(l=>/SPY KING 100%/.test(l));
ok(!!ky, '3a SPY KING present — "i must always have the spy and spxw king"');
ok(ky && Math.abs(parseFloat(ky.split(',')[1]) - 765*10.0538) <= 0.25, '3b SPY 765 × the ES ratio, on the tick', ky&&ky.split(',')[1]);
ok(ky && ky.split(',')[3]===String((205<<16)+(180<<8)+250), '3c a negative SPY crown wears the LIGHT purple', ky&&ky.split(',')[3]);
{ global.LASTFEED={ SPY:{ ts:Date.now()-999999, j:{} } };
  const Bs=irtBuildCsv();
  ok(!/SPY KING/.test(Bs.csv), '3d a stale SPY feed exports NO SPY king — absent, never old');
  global.LASTFEED={ SPY:{ ts:Date.now(), j:{} } }; }

// ---------- 4. the QQQ king ----------
const kq=qRows[0];
ok(/QQQ KING 100%/.test(kq), '4a QQQ KING present on ENQU26');
ok(/^ENQU26,27235\.000000,/.test(kq), '4b QQQ 650 × 41.9 (manual-chain ratio) on the 0.25 tick', kq);
ok(/ ~/.test(kq.split(',')[2]), '4c ...wearing ~ (ratio not measured live)', kq.split(',')[2]);
ok(kq.split(',')[3]===String((163<<16)+(113<<8)+247), '4d a negative QQQ crown wears the full purple', kq.split(',')[3]);
{ global.FUTMODE={ fam:'NQ', r:41.191, live:true };
  const Bq=irtBuildCsv(); const k2=Bq.csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(k2 && !/ ~/.test(k2.split(',')[2]) && /^ENQU26,26774\.250000,/.test(k2), '4e on a live NQ chart the ratio is MEASURED — no ~ (v14.13 chain intact)', k2);
  global.FUTMODE={ fam:'ES', r:10.0538, live:true }; }
{ const keep=QQQ_TAPE; QQQ_TAPE=()=>({ king:650, count:20, fromFeed:true, pct:{'650.00':-100} });
  ok(!/ENQU26/.test(irtBuildCsv().csv), '4f the weekly feed fallback stays REJECTED (v14.15) — ladder or nothing');
  QQQ_TAPE=keep; }

// ---------- 5. everything else stays OUT ----------
ok(!/IF /.test(b.csv), '5a the IF walls are out of the file');
ok(!/SUCC/.test(b.csv), '5b SUCC is out');
ok(!/SPXW (BRK|ACC|GK|RRUG|RUG|BAL)/.test(b.csv), '5c the rail percentage rows are out');
ok(!/SPY \d+%/.test(b.csv) && !/QQQ \d+%/.test(b.csv), '5d the SPY/QQQ percentage rows are out');
ok(!/NextStop|PBentry|FLIP|D-SPY/.test(b.csv), '5e and none of the long-dead lanes returned');

// ---------- 6. resilience: any king alone still writes; all dark writes nothing ----------
{ global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():null); // SPXW tape dead
  const B1=irtBuildCsv();
  ok(!!B1 && !/SPXW KING/.test(B1.csv) && /SPY KING/.test(B1.csv) && /QQQ KING/.test(B1.csv), '6a SPXW dark → SPY + QQQ kings still write');
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE()); }
{ global.tapeMap=(s)=>(s==='QQQ'?null:SPXW_TAPE()); global.LASTFEED={ SPY:null };
  const B2=irtBuildCsv();
  ok(!!B2 && /SPXW KING/.test(B2.csv) && !/SPY KING/.test(B2.csv) && !/ENQU26/.test(B2.csv), '6b only the SPXW king alive → it writes alone');
  const keep6=QQQ_TAPE;
  global.tapeMap=()=>null; QQQ_TAPE=()=>null;
  ok(irtBuildCsv()==null, '6c every source dark → nothing is written, never an empty confident file');
  QQQ_TAPE=keep6; global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE());
  global.LASTFEED={ SPY:{ ts:Date.now(), j:{} } }; }

// ---------- 7. ratio machinery (unchanged contract) ----------
global.FUTMODE={ fam:'ES', r:10.0538, live:false };
ok(/ ~/.test(irtBuildCsv().csv), '7a last-known ES ratio marks the EPU26 labels with ~');
global.CFG.irt.futSym=''; global.CFG.irt.etfSym='';
{ const B=irtBuildCsv();
  ok(!!B && B.csv.split('\r\n').filter(l=>l&&!/^SYMBOL/.test(l)).every(l=>l.startsWith('ENQU26,')), '7b no ES symbol set → only the NQ king writes'); }
global.CFG.irt.futSym='EPU26';
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
irtBuildCsv();                                              // persists the live ratio
global.FUTMODE={ chart:'SPY', fam:null, r:1, live:true };
const b4=irtBuildCsv();
ok(!!b4 && b4.ratio.src==='last-good' && Math.abs(b4.ratio.r-10.0538)<0.001, '7c on a CASH chart the export still writes via the persisted ES ratio', b4&&b4.ratio);
LS={};
global.LASTFEED={ SPY:{ ts:Date.now(), j:{ derived:[{source:'SPXW', ratio:0.09974500868055555}] } } };
const b5=irtBuildCsv();
ok(b5 && b5.ratio.src==='spxw-derived' && Math.abs(b5.ratio.r-10.0256)<0.01, '7d no persisted ratio → the feed\'s own SPXW→SPY ratio', b5&&b5.ratio);
global.LASTFEED={ SPY:{ ts:Date.now(), j:{} } };
const b6=irtBuildCsv();
ok(b6 && b6.ratio.src==='const' && b6.ratio.r===10.05, '7e last resort: the ES constant', b6&&b6.ratio);
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
const b7=irtBuildCsv();
ok(b7.csv.split('\r\n').every(l=>l==='' || l.split(',').length===28), '7f every emitted row keeps 28 columns');
// the config-persistence guards stay token-asserted (the every-reload OFF bug, v14.12)
ok(/if\(typeof o\.irt\.on==='boolean'\) CFG\.irt\.on=o\.irt\.on;/.test(src), '7g loadCfg still merges irt.on back');
ok(/CFG\.irt\.nqRatio=o\.irt\.nqRatio/.test(src), '7h ...and the NQ fields persist');

console.log('test_irt_export: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
