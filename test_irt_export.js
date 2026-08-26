// (v11.4, rewritten v14.9, v14.12) IRT FLEXLEVELS EXPORT — format pinned to the user's own sample;
// CONTENT pinned to the operator's locked spec of 2026-08-26:
//   ONE FILE, ALL MARKETS (FlexLevels routes by SYMBOL) — EPU26 (ES) + ENQU26 (NQ), CQG symbology.
//   LABEL GRAMMAR: SOURCE + ROLE + STRENGTH — "SPXW KING 100%" / "SPY 51%" / "IF CW0" / "QQQ 34%".
//   ORDER: SPXW block, SPY block, IF block, then NQ.
//   COLOURS: SPXW/QQQ yellow(+γ)/purple(−γ); SPY the same, LIGHTER; IF CW0 red, PW0 green,
//   MAG0/MP0 neutral. Removed things (NextStop, PBentry, FLIP, SPY-book roles, D-* lanes) are
//   asserted ABSENT so they cannot creep back silently.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }
global.window={__gptsDebug:{}};
eval(v('IRT_HEADER')); eval(v('IRT_LAST'));
eval(['irtColor'].map(ex).join('\n')); eval(v('IRT_COLORS'));
eval(v('IRT_RATIO_KEY')); eval(v('IRT_NQRATIO_KEY'));
eval(['irtRound','irtCsvRow','irtRatio','irtNqRatio','irtBuildCsv'].map(ex).join('\n'));
global.ES_RATIO=10.05; global.NQ_RATIO=41.36; global.FEED_STALE_MS=12000;
var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
global.CFG={ nodeThresh:20, irt:{ on:true, secs:180, futSym:'EPU26', etfSym:'SPY', file:'FlexLevelsExport.csv',
                                  nqOn:true, nqSym:'ENQU26', nqRatio:41.9 } };
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
global.SUCC_CHART_PCT=60; (function(){ const m=src.match(/var SUCC_CHART_PCT\s*=\s*(\d+)/); if(m) global.SUCC_CHART_PCT=+m[1]; })();

// ---- sources, stubbed ----
let IF_WHICH=null;
const IF_ROWS=()=>({ rows:[{id:'CR',k:772},{id:'PS',k:768},{id:'Mag',k:770},{id:'MP*',k:769}] });
global.ifChainRows=(sym,which)=>{ IF_WHICH=which; return IF_ROWS(); };
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
// derived diamonds (SPY source) on the SPXW feed — HOST-scale strikes (verified live 2026-08-26)
const DRV=()=>({ ts:Date.now(), j:{ derived:[{ source:'SPY', ratio:10.02,
  levels:[{ l:[ {k:7660.3, v:900e6, net:1}, {k:7675.3, v:400e6, net:-1}, {k:7620.0, v:50e6, net:1} ] }] }] } });
global.LASTSPXW=DRV();
// the QQQ book (self-fetched feed) for the NQ block
const QQQ=()=>({ ts:Date.now(), feed:'gamma', j:{ levels:[{ l:[
  {k:650, v:900e6, net:1}, {k:648, v:400e6, net:-1}, {k:640, v:50e6, net:1} ] }] } });
global.LASTFEED={ SPY:null, QQQ:QQQ() };

// ---------- 1. format machinery ----------
const b=irtBuildCsv();
ok(!!b, '0 builds');
const lines=b.csv.trim().split('\r\n');
ok(lines[0]==='SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH,PENSTYLE,bDRAWTEXT,bDRAWPRICE,LABELPOS,bCUSTPOS,CUSTPOSALLMARGIN,CUSTPOSLEFTRIGHT,CUSTPOSUNITS,CUSTPOSWIDTH,bBANDS,BANDPENCOLOR,BANDPENWIDTH,BANDPENSTYLE,BANDABOVEBEL,BANDUNITS,BANDPRICE,bBANDS2,BAND2PENCOLOR,BAND2ABOVEBEL,BAND2UNITS,BAND2PRICE,bBANDLABELS,bTRANSLUCENT', '1a header verbatim from the sample');
ok(IF_WHICH==='dte0', '1b the IF window is 0DTE — never to-Friday (operator-directed)', IF_WHICH);
// ES rows: 4 SPXW (3 nodes + SUCC) + 2 SPY diamonds + 4 IF = 10, on both configured symbols;
// NQ rows: 2 (the 6% strike is floored). n = 10 + 2.
ok(b.n===12 && lines.length===1+10*2+2, '1c 12 levels: (SPXW 4 + SPY 2 + IF 4) ×2 syms + NQ 2', [b.n,lines.length]);
const eRows=lines.filter(l=>l.startsWith('EPU26,')); const sRows=lines.filter(l=>l.startsWith('SPY,'));
const qRows=lines.filter(l=>l.startsWith('ENQU26,'));
ok(eRows.length===10 && sRows.length===10 && qRows.length===2, '1d all three symbols written', [eRows.length,sRows.length,qRows.length]);
ok(lines.slice(1).every(l=>l.split(',').length===28), '1e every row keeps exactly 28 columns');
// the operator's row order: SPXW block, SPY block, IF block
const order=eRows.map(l=>l.split(',')[2].split(' ')[0]);
ok(order.join(',')==='SPXW,SPXW,SPXW,SPXW,SPY,SPY,IF,IF,IF,IF', '1f row order = SPXW → SPY → IF', order.join(','));

// ---------- 2. the rail nodes: labels, prices, colours ----------
const king=eRows.find(l=>/SPXW KING 100%/.test(l));
ok(!!king, '2a the King exports as SPXW KING + %King — source-prefixed, no strike text');
ok(king && /^EPU26,7727\.750000,/.test(king), '2b SPX 7710 lands at ES 7727.75 — the RAIL\'s price on the 0.25 tick', king);
ok(king && king.split(',')[4]==='3', '2c the King is drawn heaviest');
const gk=eRows.find(l=>/SPXW GK 79%/.test(l));
ok(gk && gk.split(',')[3]===String((163<<16)+(113<<8)+247), '2d a -gamma GK wears the accelerator purple (COLORREF)', gk&&gk.split(',')[3]);
const brk=eRows.find(l=>/SPXW BRK 41%/.test(l));
ok(brk && brk.split(',')[3]===String((227<<16)+(195<<8)+65), '2e a +gamma node wears the brake yellow', brk&&brk.split(',')[3]);
const succ=eRows.find(l=>/SPXW SUCC/.test(l));
ok(!!succ && !/SUCC \d/.test(b.csv), '2f the successor rides along as SPXW SUCC — NO % (meaning is "next King", not size)');
ok(succ && /^EPU26,7647\.500000,/.test(succ), '2g ...at its converted tick price', succ);

// ---------- 3. the SPY diamonds: labels, colours ----------
const spyK=eRows.find(l=>/SPY KING 100%/.test(l));
ok(!!spyK, '3a SPY\'s King is NAMED — the 100% row is arithmetic, not a guess');
ok(/SPY 44%/.test(b.csv), '3b every other SPY row is % only (roles would be guesses — the rail machine is SPXW-only)');
ok(spyK && spyK.split(',')[3]===String((240<<16)+(222<<8)+140), '3c +gamma SPY wears the LIGHT yellow', spyK&&spyK.split(',')[3]);
const spy44=eRows.find(l=>/SPY 44%/.test(l));
ok(spy44 && spy44.split(',')[3]===String((205<<16)+(180<<8)+250), '3d -gamma SPY wears the LIGHT purple', spy44&&spy44.split(',')[3]);
ok(spyK && Math.abs(parseFloat(spyK.split(',')[1]) - 7678.00) <= 0.25, '3e host-scale strike -> chart -> ES, on the tick', spyK&&spyK.split(',')[1]);
ok(spyK && spyK.split(',')[5]==='1', '3f dotted, like Skylit draws the diamonds');

// ---------- 4. the IF 0DTE walls: renamed to the panel's vocabulary + window suffix ----------
ok(/IF CW0/.test(b.csv) && /IF PW0/.test(b.csv) && /IF MAG0/.test(b.csv) && /IF MP0\*/.test(b.csv),
   '4a CR->CW0, PS->PW0, Mag->MAG0, MP->MP0 (companion\'s * survives)');
const ifcw=eRows.find(l=>/IF CW0/.test(l));
ok(ifcw && /^EPU26,7761\.500000,/.test(ifcw), '4b IF CW0 772 × 10.0538 = 7761.53 → 7761.50 on the tick', ifcw);
ok(ifcw && ifcw.split(',')[3]===String((240<<16)+(97<<8)+109), '4c CW0 is red (resistance)', ifcw&&ifcw.split(',')[3]);
const ifpw=eRows.find(l=>/IF PW0/.test(l));
ok(ifpw && ifpw.split(',')[3]===String((46<<16)+(194<<8)+126), '4d PW0 is green (support)', ifpw&&ifpw.split(',')[3]);
const ifmag=eRows.find(l=>/IF MAG0/.test(l));
ok(ifmag && ifmag.split(',')[3]===String((120<<16)+(140<<8)+160), '4e MAG0 is NEUTRAL — a magnet is not a wall', ifmag&&ifmag.split(',')[3]);
ok(!/IF CR/.test(b.csv) && !/IF PS/.test(b.csv) && !/IF Mag[^0]/.test(b.csv), '4f the old CR/PS/Mag names are gone');

// ---------- 5. the NQ block: QQQ book -> ENQU26, same file ----------
const nqK=qRows.find(l=>/QQQ KING 100%/.test(l));
ok(!!nqK, '5a QQQ\'s King is named on the NQ chart');
ok(nqK && /^ENQU26,27235\.000000,/.test(nqK), '5b QQQ 650 × 41.9 = 27235.00 on the 0.25 tick', nqK);
ok(qRows.every(l=>/ ~/.test(l.split(',')[2])), '5c EVERY NQ label wears ~ — the ratio is manual by construction (no NQ price in Skylit)');
ok(nqK && nqK.split(',')[3]===String((227<<16)+(195<<8)+65), '5d +gamma QQQ wears the full brake yellow', nqK&&nqK.split(',')[3]);
const nq44=qRows.find(l=>/QQQ 44%/.test(l));
ok(nq44 && nq44.split(',')[3]===String((163<<16)+(113<<8)+247), '5e -gamma QQQ wears the full accelerator purple', nq44&&nq44.split(',')[3]);
ok(!/QQQ 6%/.test(b.csv), '5f the node threshold floors NQ rows too');
{ global.LASTFEED={ SPY:null, QQQ:{ ts:Date.now()-999999, feed:'gamma', j:QQQ().j } };
  ok(!/ENQU26/.test(irtBuildCsv().csv), '5g a stale QQQ feed exports NO NQ rows — absent, never old');
  global.LASTFEED={ SPY:null, QQQ:QQQ() }; }
{ global.CFG.irt.nqOn=false;
  ok(!/ENQU26/.test(irtBuildCsv().csv), '5h NQ off in settings → no ENQU26 rows');
  global.CFG.irt.nqOn=true; }
{ global.CFG.irt.nqRatio=42.5;
  const Bq=irtBuildCsv(); const k2=Bq.csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(k2 && /^ENQU26,27625\.000000,/.test(k2), '5i the settings ratio drives the NQ price (650×42.5)', k2);
  global.CFG.irt.nqRatio=41.9; }
// (v14.13) the ratio SELF-MEASURES on an NQ chart — the same chain the ES side runs
{ global.FUTMODE={ fam:'NQ', r:41.191, live:true };
  const Bl=irtBuildCsv(); const kL=Bl.csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(kL && !/ ~/.test(kL.split(',')[2]), '5j on a live NQ chart the labels drop the ~ — the ratio is MEASURED', kL&&kL.split(',')[2]);
  ok(kL && /^ENQU26,26774\.250000/.test(kL), '5k ...and the measured basis drives the price (650×41.191)', kL);
  global.FUTMODE={ chart:'SPY', fam:null, r:1, live:true };
  const Bg=irtBuildCsv(); const kG=Bg.csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(kG && / ~/.test(kG.split(',')[2]) && /^ENQU26,26774\.250000/.test(kG), '5l off the NQ chart the persisted last-good carries the SAME number, marked ~', kG);
  LS={}; // clear the persisted store
  const Bm=irtBuildCsv(); const kM=Bm.csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(kM && /^ENQU26,27235\.000000,/.test(kM), '5m no measurement anywhere → the settings manual number (650×41.9)', kM);
  global.CFG.irt.nqRatio=0;
  const Bc=irtBuildCsv(); const kC=Bc.csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(kC && /^ENQU26,26884\.000000,/.test(kC), '5n and with no manual either, the NQ_RATIO const (650×41.36)', kC);
  global.CFG.irt.nqRatio=41.9; global.FUTMODE={ fam:'ES', r:10.0538, live:true }; }

// ---------- 6. everything the operator removed stays ABSENT ----------
ok(!/NextStop/.test(b.csv), '6a NextStop is gone');
ok(!/PBentry/.test(b.csv), '6b PBentry is gone');
ok(!/FLIP/.test(b.csv), '6c our FLIP set is gone');
ok(!/,K 100%/.test(b.csv), '6d the SPY-book role rows are gone');
ok(!/D-SPY/.test(b.csv) && !/D-QQQ/.test(b.csv), '6e the D-* lane labels are gone (grammar is SPY %)');

// ---------- 7. one source down never kills the others ----------
{ global.ifChainRows=()=>null;
  const B=irtBuildCsv();
  ok(!!B && /SPXW KING 100%/.test(B.csv) && !/IF /.test(B.csv), '7a no companion → rail levels still export, none of theirs');
  global.ifChainRows=(s2,w2)=>IF_ROWS(); }
{ global.emPiles.lastSrc='if-fallback';
  const B=irtBuildCsv();
  ok(!!B && /IF CW0/.test(B.csv) && !/KING 100%.*EPU26|EPU26.*SPXW KING/.test(B.csv.split('\r\n').filter(l=>l.startsWith('EPU26')).filter(l=>/SPXW/.test(l)).join('')), '7b no SKYLIT tape → the IF levels still export');
  global.emPiles.lastSrc='skylit'; }
{ global.ifChainRows=()=>null; global.emPiles.lastSrc='if-fallback'; global.LASTSPXW=null;
  const B=irtBuildCsv();
  ok(!!B && B.csv.split('\r\n').filter(l=>l&&!/^SYMBOL/.test(l)).every(l=>l.startsWith('ENQU26,')), '7c ES sources all down → the NQ block still writes alone', B&&B.n);
  global.LASTFEED={ SPY:null, QQQ:null };
  ok(irtBuildCsv()==null, '7d EVERY source down → nothing is written, never an empty confident file');
  global.ifChainRows=(s2,w2)=>IF_ROWS(); global.emPiles.lastSrc='skylit';
  global.LASTSPXW=DRV(); global.LASTFEED={ SPY:null, QQQ:QQQ() }; }
{ // diamonds: only the SPY source projects onto the ES chart
  global.LASTSPXW={ ts:Date.now(), j:{ derived:[
    { source:'QQQ', ratio:0.09, levels:[{ l:[ {k:7660.0, v:900e6, net:1} ] }] } ] } };
  ok(!/SPY /.test(irtBuildCsv().csv.split('\r\n').filter(l=>l.startsWith('EPU26')).join('')), '7e a non-SPY derived source is IGNORED on the ES chart (QQQ gets its own book on NQ)');
  global.LASTSPXW=DRV(); }
{ // source-grid fallback: a payload on the SOURCE grid still lands on the same price via ratio
  global.LASTSPXW={ ts:Date.now(), j:{ derived:[{ source:'SPY', ratio:10.02,
    levels:[{ l:[ {k:764.5, v:900e6, net:1} ] }] }] } };
  const Bf=irtBuildCsv();
  const df=Bf.csv.split('\r\n').find(l=>/SPY KING 100%/.test(l)&&l.startsWith('EPU26,'));
  ok(df && Math.abs(parseFloat(df.split(',')[1]) - 7678.00) <= 0.5, '7f source-grid payload falls back through ratio to the same price', df&&df.split(',')[1]);
  global.LASTSPXW=DRV(); }

// ---------- 8. ratio machinery (unchanged contract) + config persistence ----------
global.FUTMODE={ fam:'ES', r:10.0538, live:false };
ok(/ ~/.test(irtBuildCsv().csv), '8a last-known ratio marks labels with ~');
global.CFG.irt.futSym=''; global.CFG.irt.etfSym='';
{ const B=irtBuildCsv();
  ok(!!B && B.csv.split('\r\n').filter(l=>l&&!/^SYMBOL/.test(l)).every(l=>l.startsWith('ENQU26,')), '8b no ES symbol set → no wrong-symbol ES rows; NQ unaffected');
  global.LASTFEED={ SPY:null, QQQ:null };
  ok(irtBuildCsv()==null, '8c ...and with NQ dark too, nothing is written');
  global.LASTFEED={ SPY:null, QQQ:QQQ() }; }
global.CFG.irt.futSym='EPU26'; global.CFG.irt.etfSym='';
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
irtBuildCsv();                                              // persists the live ratio
global.FUTMODE={ chart:'SPY', fam:null, r:1, live:true };   // back on the cash chart
const b4=irtBuildCsv();
ok(!!b4 && b4.ratio.src==='last-good' && Math.abs(b4.ratio.r-10.0538)<0.001, '8d on a CASH chart the export still writes, using the persisted ES ratio', b4&&b4.ratio);
ok(/ ~/.test(b4.csv), '8e ...marked ~ because the ratio is not live');
LS={};
global.LASTFEED={ SPY:{ j:{ derived:[{source:'SPXW', ratio:0.09974500868055555}] } }, QQQ:QQQ() };
const b5=irtBuildCsv();
ok(b5 && b5.ratio.src==='spxw-derived' && Math.abs(b5.ratio.r-10.0256)<0.01, '8f no persisted ratio → the feed\'s own SPXW→SPY ratio', b5&&b5.ratio);
global.LASTFEED={SPY:null, QQQ:QQQ()};
const b6=irtBuildCsv();
ok(b6 && b6.ratio.src==='const' && b6.ratio.r===10.05, '8g last resort: the ES constant', b6&&b6.ratio);
// comma sanitation still holds on the surviving label paths
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
const b7=irtBuildCsv();
ok(b7.csv.split('\r\n').every(l=>l==='' || l.split(',').length===28), '8h every emitted row keeps 28 columns');
// (v14.12) THE RELOAD BUG: loadCfg must merge o.irt back — the toggle silently reset to OFF on
// every reload from the v8 rewrite until 2026-08-26. Token-assert the merge lines exist.
ok(/if\(typeof o\.irt\.on==='boolean'\) CFG\.irt\.on=o\.irt\.on;/.test(src), '8i loadCfg merges irt.on back (the every-reload OFF bug)');
ok(/CFG\.irt\.nqRatio=o\.irt\.nqRatio/.test(src), '8j ...and the NQ fields persist too');

// (v14.14) RGB, not COLORREF — the King rendered BLUE on the operator's chart under BGR
ok(/function irtColor\(r,g,b\)\{ return \(r<<16\)\+\(g<<8\)\+b; \}/.test(src), '9a irtColor is RGB 0xRRGGBB (empirical: yellow rendered blue as BGR)');
// (v14.14) the export price must not depend on which chart Atlas shows
{ global.FUTMODE={ chart:'SPXW', fam:'SPX', r:1, live:true };            // the SPXW CASH chart
  global.ifLadder=()=>({ dispScale:1.0, undScale:0.099773 });            // dispScale ~1 there
  const Bc2=irtBuildCsv(); const kC2=Bc2.csv.split('\r\n').find(l=>/SPXW KING/.test(l)&&l.startsWith('EPU26,'));
  ok(kC2 && Math.abs(parseFloat(kC2.split(',')[1]) - 7733.9) <= 0.25,
     '9b on the SPXW CASH chart the rail rows go SPX->SPY->ES (undScale x r), never raw SPX (the 7655-on-ES bug)', kC2&&kC2.split(',')[1]);
  ok(kC2 && parseFloat(kC2.split(',')[1]) !== 7710.00, '9c ...so the raw strike price never reaches the futures chart');
  global.FUTMODE={ fam:'ES', r:10.0538, live:true };
  global.ifLadder=()=>({ dispScale:1.0023 });
  const Bc3=irtBuildCsv(); const kC3=Bc3.csv.split('\r\n').find(l=>/SPXW KING/.test(l)&&l.startsWith('EPU26,'));
  ok(kC3 && /^EPU26,7727\.750000,/.test(kC3), '9d and on an ES chart the preferred dispScale path still lands on the rail\'s own price', kC3); }

console.log('test_irt_export: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
