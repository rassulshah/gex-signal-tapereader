// (v11.4, rewritten v14.9) IRT FLEXLEVELS EXPORT — format pinned to the user's own sample; CONTENT
// pinned to the operator's brief: "the ES levels from the SPXW conversion, and the 0dte IF levels.
// I think that should be all." Everything else (SPY-book roles, derived lanes, our CR/PS/FLIP set,
// NextStop, PBentry) is asserted ABSENT, so it cannot creep back silently.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }
global.window={__gptsDebug:{}};
eval(v('IRT_HEADER')); eval(v('IRT_LAST'));
eval(['irtColor'].map(ex).join('\n')); eval(v('IRT_COLORS'));
eval(v('IRT_RATIO_KEY'));
eval(['irtRound','irtCsvRow','irtRatio','irtBuildCsv'].map(ex).join('\n'));
global.ES_RATIO=10.05; global.LASTFEED={SPY:null};
var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
global.CFG={ nodeThresh:20, irt:{ on:true, secs:180, futSym:'EPU26', etfSym:'SPY', file:'FlexLevelsExport.csv' } };
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
global.SUCC_CHART_PCT=60; (function(){ const m=src.match(/var SUCC_CHART_PCT\s*=\s*(\d+)/); if(m) global.SUCC_CHART_PCT=+m[1]; })();

// ---- the two remaining sources, stubbed ----
let IF_WHICH=null;
global.ifChainRows=(sym,which)=>{ IF_WHICH=which;
  return { rows:[{id:'CR',k:772},{id:'PS',k:768},{id:'Mag',k:770},{id:'MP',k:769}] }; };
global.emBand=()=>({ ok:true, now:7690 });
global.ifLadder=()=>({ dispScale:1.0023 });
const PILES=[
  {k:7710, pct:100, role:'KING', accel:false, disp:7727.73},
  {k:7700, pct:79,  role:'GK',   accel:true,  disp:7717.71},
  {k:7650, pct:41,  role:null,   accel:false, disp:7667.59},
];
global.emPiles=function(){ return PILES; };
global.emPiles.lastSrc='skylit';
global.tapeMap=()=>({ king:7710, pct:{ '7710':100, '7630':85, '7650':41 } });

// ---------- 1. format machinery ----------
const b=irtBuildCsv();
ok(!!b, '0 builds');
const lines=b.csv.trim().split('\r\n');
ok(lines[0]==='SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH,PENSTYLE,bDRAWTEXT,bDRAWPRICE,LABELPOS,bCUSTPOS,CUSTPOSALLMARGIN,CUSTPOSLEFTRIGHT,CUSTPOSUNITS,CUSTPOSWIDTH,bBANDS,BANDPENCOLOR,BANDPENWIDTH,BANDPENSTYLE,BANDABOVEBEL,BANDUNITS,BANDPRICE,bBANDS2,BAND2PENCOLOR,BAND2ABOVEBEL,BAND2UNITS,BAND2PRICE,bBANDLABELS,bTRANSLUCENT', '1a header verbatim from the sample');
ok(IF_WHICH==='dte0', '1b the IF window is 0DTE — never to-Friday (operator-directed)', IF_WHICH);
// rows: 4 IF + 3 rail nodes + 1 successor = 8, on both configured symbols
ok(b.n===8 && lines.length===1+8*2, '1c 8 levels (IF CR/PS/Mag/MP + KING/GK/BRK + SUCC) × 2 symbols', [b.n,lines.length]);
const eRows=lines.filter(l=>l.startsWith('EPU26,')); const sRows=lines.filter(l=>l.startsWith('SPY,'));
ok(eRows.length===8 && sRows.length===8, '1d both symbols written', [eRows.length,sRows.length]);
ok(eRows.every(l=>l.split(',').length===28), '1e every row keeps exactly 28 columns');

// ---------- 2. the rail nodes: labels, prices, colours ----------
const king=eRows.find(l=>/KING 100%/.test(l));
ok(!!king, '2a the King exports as NODETYPE + %King — no strike text');
ok(king && /^EPU26,7727\.750000,/.test(king), '2b SPX 7710 lands at ES 7727.75 — the RAIL\'s price on the 0.25 tick', king);
ok(king && king.split(',')[4]==='3', '2c the King is drawn heaviest');
const gk=eRows.find(l=>/GK 79%/.test(l));
ok(gk && gk.split(',')[3]===String((247<<16)+(113<<8)+163), '2d a -gamma GK wears the accelerator purple (COLORREF)', gk&&gk.split(',')[3]);
const brk=eRows.find(l=>/BRK 41%/.test(l));
ok(brk && brk.split(',')[3]===String((65<<16)+(195<<8)+227), '2e a +gamma node wears the brake yellow', brk&&brk.split(',')[3]);
const succ=eRows.find(l=>/SUCC 85%/.test(l));
ok(!!succ, '2f the successor (>=60% of King) rides along — the rail\'s crown-roll warning');
ok(succ && /^EPU26,7647\.500000,/.test(succ), '2g ...at its converted tick price', succ);

// ---------- 3. the IF 0DTE levels ----------
ok(/IF CR/.test(b.csv) && /IF PS/.test(b.csv) && /IF Mag/.test(b.csv) && /IF MP/.test(b.csv),
   '3a all four IF 0DTE levels reach the file, tagged IF, never bare');
const ifcr=eRows.find(l=>/IF CR/.test(l));
ok(ifcr && /^EPU26,7761\.500000,/.test(ifcr), '3b IF CR 772 × 10.0538 = 7761.53 → 7761.50 on the tick', ifcr);

// ---------- 4. everything the operator removed stays ABSENT ----------
ok(!/NextStop/.test(b.csv), '4a NextStop is gone');
ok(!/PBentry/.test(b.csv), '4b PBentry is gone');
ok(!/FLIP/.test(b.csv), '4c our FLIP set is gone');
ok(!/,K 100%/.test(b.csv) && !/,Mag \d+%/.test(b.csv), '4d the SPY-book role rows are gone', null);
ok(!/SPXW \d+%/.test(b.csv), '4e the derived lanes are gone');

// ---------- 5. one source down never kills the other ----------
{ global.ifChainRows=()=>null;
  const B=irtBuildCsv();
  ok(!!B && /KING 100%/.test(B.csv) && !/IF /.test(B.csv), '5a no companion → rail levels still export, none of theirs');
  global.ifChainRows=(s2,w2)=>({ rows:[{id:'CR',k:772},{id:'PS',k:768}] }); }
{ global.emPiles.lastSrc='if-fallback';
  const B=irtBuildCsv();
  ok(!!B && /IF CR/.test(B.csv) && !/KING/.test(B.csv), '5b no SKYLIT tape → the IF levels still export alone');
  global.emPiles.lastSrc='skylit'; }
{ global.ifChainRows=()=>null; global.emPiles.lastSrc='if-fallback';
  ok(irtBuildCsv()==null, '5c both sources down → nothing is written, never an empty confident file');
  global.ifChainRows=(s2,w2)=>({ rows:[{id:'CR',k:772},{id:'PS',k:768},{id:'Mag',k:770},{id:'MP',k:769}] });
  global.emPiles.lastSrc='skylit'; }

// ---------- 5.5 the Derived diamonds (v14.10) ----------
{
  global.FEED_STALE_MS=12000;
  // ⚠ VERIFIED live 2026-08-26: derived rows arrive PRE-CONVERTED to the host scale; ratio is
  // informational. Primary stub is host-scale; the ratio-multiply fallback is asserted separately.
  global.LASTSPXW={ ts:Date.now(), j:{ derived:[{ source:'SPY', ratio:10.02,
    levels:[{ l:[ {k:7660.3, v:900e6, net:1}, {k:7675.3, v:400e6, net:-1}, {k:7620.0, v:50e6, net:1} ] }] }] } };
  const B=irtBuildCsv();
  const dRows=B.csv.split('\r\n').filter(l=>/D-SPY/.test(l)&&l.startsWith('EPU26,'));
  ok(dRows.length===2, '5d two diamonds clear the threshold (50e6/900e6=6% is floored out)', dRows.length);
  ok(/D-SPY 100%/.test(B.csv) && /D-SPY 44%/.test(B.csv), '5e labelled by SOURCE + own-King % — never the native %King');
  // host 7660.3 x 1.0023 dispScale -> ES on the 0.25 tick
  const d0=dRows.find(l=>/D-SPY 100%/.test(l));
  ok(d0 && Math.abs(parseFloat(d0.split(',')[1]) - 7678.00) <= 0.25, '5f host-scale strike -> chart -> ES, on the tick', d0&&d0.split(',')[1]);
  { // the fallback: a payload on the SOURCE grid still lands on the same price via ratio
    global.LASTSPXW={ ts:Date.now(), j:{ derived:[{ source:'SPY', ratio:10.02,
      levels:[{ l:[ {k:764.5, v:900e6, net:1} ] }] }] } };
    const Bf=irtBuildCsv();
    const df=Bf.csv.split('\r\n').find(l=>/D-SPY 100%/.test(l)&&l.startsWith('EPU26,'));
    ok(df && Math.abs(parseFloat(df.split(',')[1]) - 7678.00) <= 0.5, '5f2 source-grid payload falls back through ratio to the same price', df&&df.split(',')[1]);
    global.LASTSPXW={ ts:Date.now(), j:{ derived:[{ source:'SPY', ratio:10.02,
      levels:[{ l:[ {k:7660.3, v:900e6, net:1}, {k:7675.3, v:400e6, net:-1}, {k:7620.0, v:50e6, net:1} ] }] }] } };
  }
  ok(d0.split(',')[4]==='1' && d0.split(',')[5]==='1', '5g thin and dotted, like Skylit draws the diamonds');
  ok(d0.split(',')[3]===String((130<<16)+(110<<8)+90), '5h slate — the diamonds make no polarity claim');
  global.LASTSPXW={ ts:Date.now()-999999, j:global.LASTSPXW.j };
  ok(!/D-SPY/.test(irtBuildCsv().csv), '5i a stale SPXW feed exports NO diamonds — absent, never old');
  global.LASTSPXW=null;
  ok(!/D-SPY/.test(irtBuildCsv().csv), '5j and no store at all is fine');
}

// ---------- 6. ratio machinery (unchanged contract) ----------
global.FUTMODE={ fam:'ES', r:10.0538, live:false };
ok(/ ~/.test(irtBuildCsv().csv), '6a last-known ratio marks labels with ~');
global.CFG.irt.futSym=''; global.CFG.irt.etfSym='';
ok(irtBuildCsv()==null, '6b no symbols set → nothing written (never a wrong-symbol file)');
global.CFG.irt.futSym='EPU26'; global.CFG.irt.etfSym='';
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
irtBuildCsv();                                              // persists the live ratio
global.FUTMODE={ chart:'SPY', fam:null, r:1, live:true };   // back on the cash chart
const b4=irtBuildCsv();
ok(!!b4 && b4.ratio.src==='last-good' && Math.abs(b4.ratio.r-10.0538)<0.001, '6c on a CASH chart the export still writes, using the persisted ES ratio', b4&&b4.ratio);
ok(/ ~/.test(b4.csv), '6d ...marked ~ because the ratio is not live');
LS={};
global.LASTFEED={ SPY:{ j:{ derived:[{source:'SPXW', ratio:0.09974500868055555}] } } };
const b5=irtBuildCsv();
ok(b5 && b5.ratio.src==='spxw-derived' && Math.abs(b5.ratio.r-10.0256)<0.01, '6e no persisted ratio → the feed\'s own SPXW→SPY ratio', b5&&b5.ratio);
global.LASTFEED={SPY:null};
const b6=irtBuildCsv();
ok(b6 && b6.ratio.src==='const' && b6.ratio.r===10.05, '6f last resort: the ES constant', b6&&b6.ratio);
// comma sanitation still holds on the surviving label paths
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
const b7=irtBuildCsv();
ok(b7.csv.split('\r\n').every(l=>l==='' || l.split(',').length===28), '6g every emitted row keeps 28 columns');

console.log('test_irt_export: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
