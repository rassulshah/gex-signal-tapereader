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
eval(v('IRT_QQQK_KEY')); eval(v('IRT_KINGS_KEY'));
eval(['irtRound','irtCsvRow','irtRatio','irtNqRatio','kingLatchTick','irtKingLatch','irtKingHeld','irtQqqKing','irtBuildCsv'].map(ex).join('\n'));
// (v14.75) the projection reads the RAIL's array — stub the two functions it comes from
let RAIL_QQQ={ at:7691.25, book:'QQQ', raw:650, kind:'proportional' };
global.emBand=()=>({ ok:true, now:7691.2, nowLive:7691.2, scaleUsed:10.0538 });
global.ladderKings=()=>[{at:7727.75,book:'SPXW'},{at:7691.25,book:'SPY'}].concat(RAIL_QQQ?[RAIL_QQQ]:[]);
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
let QQQ_LADDER=()=>null;                      // (v14.73) the face's array; null = fall through to the tape
global.ladderFor=(s)=>(s==='QQQ'?QQQ_LADDER():null);
global.LASTFEED={ SPY:{ ts:Date.now(), j:{ levels:[{ s:765 }] } }, QQQ:{ ts:Date.now(), j:{ levels:[{ s:650 }] } } };
global.extractWalls=()=>({ king:765, walls:[{k:765, pct:100, pos:false}] });

// ---------- 1. the whole file is three kings ----------
const b=irtBuildCsv();
ok(!!b, '0 builds');
const lines=b.csv.trim().split('\r\n');
ok(lines[0]==='SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH,PENSTYLE,bDRAWTEXT,bDRAWPRICE,LABELPOS,bCUSTPOS,CUSTPOSALLMARGIN,CUSTPOSLEFTRIGHT,CUSTPOSUNITS,CUSTPOSWIDTH,bBANDS,BANDPENCOLOR,BANDPENWIDTH,BANDPENSTYLE,BANDABOVEBEL,BANDUNITS,BANDPRICE,bBANDS2,BAND2PENCOLOR,BAND2ABOVEBEL,BAND2UNITS,BAND2PRICE,bBANDLABELS,bTRANSLUCENT', '1a header verbatim from the sample');
// (v14.75) 2 king rows × 2 configured symbols (EPU26 + ETF SPY) + the NQ king + the QQQ king
// PROJECTED onto EPU26. The projection is counted with the NQ side because it is the same King in a
// second coordinate system, not a fourth King.
ok(b.n===4 && lines.length===1+2*2+2, '1b n=4 (3 kings + the ES projection); rows = 2×2 syms + NQ + projection', [b.n,lines.length]);
const eRows=lines.filter(l=>l.startsWith('EPU26,'));
const qRows=lines.filter(l=>l.startsWith('ENQU26,'));
ok(eRows.length===3 && qRows.length===1,
   '1c EPU26 carries SPXW + SPY + the projected QQQ; ENQU26 carries the native QQQ', [eRows.length,qRows.length]);
ok(eRows.filter(l=>/QQQ KING/.test(l)).length===1 && /,2,1,/.test(eRows.find(l=>/QQQ KING/.test(l))||''),
   '1c2 ...and the projection is DASHED (style 1), so it cannot pass for a native level at a glance');
ok(lines.slice(1).every(l=>l.split(',').length===28), '1d every row keeps exactly 28 columns');

// ---------- 2. the SPXW king ----------
const kx=eRows.find(l=>/SPXW KING/.test(l));
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
const ky=eRows.find(l=>/SPY KING/.test(l));
ok(!!ky, '3a SPY KING present — "i must always have the spy and spxw king"');
ok(ky && Math.abs(parseFloat(ky.split(',')[1]) - 765*10.0538) <= 0.25, '3b SPY 765 × the ES ratio, on the tick', ky&&ky.split(',')[1]);
ok(ky && ky.split(',')[3]===String((205<<16)+(180<<8)+250), '3c a negative SPY crown wears the LIGHT purple', ky&&ky.split(',')[3]);
// ⚠⚠ (v14.74) THIS ASSERTION IS REVERSED, and the operator's own file is the reason.
// It used to demand that a stale SPY feed export NO SPY king — "absent, never old". Measured on his
// machine 2026-08-28: FlexLevelsExport.csv held ONE row (the QQQ King) while SPXW and SPY were gone,
// because one degraded tick wrote a file without them. Over the polling HTTP server that DELETES the
// levels from his chart mid-session. "Absent, never old" is right for a READING; it is wrong for a
// LEVEL, which does not stop existing because a feed blinked. The King is now HELD for the session
// day and the hold is reported in IRT_LAST.spyWhy.
{ delete LS[IRT_KINGS_KEY];
  global.LASTFEED={ SPY:{ ts:Date.now()-999999, j:{} } };
  const Bs0=irtBuildCsv();
  ok(!/SPY KING/.test(Bs0.csv), '3d a stale SPY feed with NOTHING latched exports no SPY king');
  global.LASTFEED={ SPY:{ ts:Date.now(), j:{} } };
  irtBuildCsv();                                      // one good tick seeds the latch
  global.LASTFEED={ SPY:{ ts:Date.now()-999999, j:{} } };
  const Bs1=irtBuildCsv();
  ok(/SPY KING/.test(Bs1.csv),
     '3d2 ...but once seen today it is HELD through a stale tick — a level does not vanish because a feed blinked');
  ok(/held/.test(IRT_LAST.spyWhy||''), '3d3 ...and the export says it is held, not live', IRT_LAST.spyWhy);
  global.LASTFEED={ SPY:{ ts:Date.now(), j:{} } }; }

// ---------- 4. the QQQ king ----------
const kq=qRows[0];
ok(/QQQ KING/.test(kq) && !/100%/.test(kq),
   '4a QQQ KING present on ENQU26, and the redundant 100% is gone (operator, 2026-08-28)');
ok(/^ENQU26,27235\.000000,/.test(kq), '4b QQQ 650 × 41.9 (manual-chain ratio) on the 0.25 tick', kq);
ok(/ ~/.test(kq.split(',')[2]), '4c ...wearing ~ (ratio not measured live)', kq.split(',')[2]);
ok(kq.split(',')[3]===String((163<<16)+(113<<8)+247), '4d a negative QQQ crown wears the full purple', kq.split(',')[3]);
{ global.FUTMODE={ fam:'NQ', r:41.191, live:true };
  const Bq=irtBuildCsv(); const k2=Bq.csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(k2 && !/ ~/.test(k2.split(',')[2]) && /^ENQU26,26774\.250000,/.test(k2), '4e on a live NQ chart the ratio is MEASURED — no ~ (v14.13 chain intact)', k2);
  global.FUTMODE={ fam:'ES', r:10.0538, live:true }; }
// ⚠⚠ (v14.73) THIS ASSERTION IS REVERSED, DELIBERATELY, AND THE REASON IS RECORDED.
// v14.15 refused a feed-sourced QQQ King ("Atlas is the source of truth, feed fallback rejected").
// On 2026-08-28 that rule cost the operator the level for two hours while the panel was DISPLAYING
// a healthy QQQ book — he caught it: "you show the qqq king in the tapereader app so you know the
// level." Refusing bought a MISSING level, not a safer one. The feed King is Skylit's own gamma
// book; it is now written and TAGGED, and the source is reported in IRT_LAST.nqWhy.
{ const keep=QQQ_TAPE; QQQ_TAPE=()=>({ king:650, count:20, fromFeed:true, pct:{'650.00':-100} });
  const csvF=irtBuildCsv().csv;
  ok(/ENQU26/.test(csvF), '4f a feed-sourced QQQ King is now WRITTEN, not refused (v14.15 reversed)');
  ok(/via feed/.test(IRT_LAST.nqWhy||''), '4f2 ...and the export records that it came from the feed', IRT_LAST.nqWhy);
  QQQ_TAPE=keep; }
// ---------- 4p. THE QQQ KING ON ES — READ FROM THE RAIL, NOT RECOMPUTED (v14.75) ----------
// The operator pointed at his own panel: "my tapereader app shows the qqq king" — `~7721 QQQ` in the
// price chute. ⚠ The export must write THAT number. A second computation, however well argued, puts
// a line on the chart that disagrees with the pill on the rail (DECISIONS v13.2).
{ const B=irtBuildCsv();
  const proj=B.csv.split('\r\n').filter(l=>/^EPU26/.test(l) && /QQQ KING/.test(l));
  ok(proj.length===1, '4p the ES chart gets exactly ONE projected QQQ King row', proj.length);
  ok(proj[0] && Math.abs(parseFloat(proj[0].split(',')[1]) - 7691.25) <= 0.001,
     '4p2 ...at the RAIL\'s own bearing, unrounded and unrecomputed', proj[0]&&proj[0].split(',')[1]);
  ok(/rail bearing/.test(IRT_LAST.xqWhy||''), '4p3 ...and the export says where it came from', IRT_LAST.xqWhy);
  // move the rail's bearing: the export must follow it, because it is not its own number
  const keep=RAIL_QQQ; RAIL_QQQ={ at:7745.00, book:'QQQ', raw:655, kind:'proportional' };
  const B2=irtBuildCsv();
  const p2=B2.csv.split('\r\n').filter(l=>/^EPU26/.test(l) && /QQQ KING/.test(l))[0];
  ok(p2 && Math.abs(parseFloat(p2.split(',')[1]) - 7745.00) <= 0.001,
     '4p4 ...and it FOLLOWS the rail when the rail moves — one quantity, one source', p2&&p2.split(',')[1]);
  // the rail has no bearing (QQQ feed stale): the export writes none rather than inventing one
  RAIL_QQQ=null;
  const B3=irtBuildCsv();
  ok(!B3.csv.split('\r\n').some(l=>/^EPU26/.test(l) && /QQQ KING/.test(l)),
     '4p5 no rail bearing -> no projected row; the export never computes its own');
  ok(/no QQQ bearing/.test(IRT_LAST.xqWhy||''), '4p6 ...and names the absence', IRT_LAST.xqWhy);
  RAIL_QQQ=keep; }
// ⚠ IT IS A BEARING, NOT A LEVEL — and the FILE has to say so, because the chart is the only surface
// the operator sees. Dashed (style 1) and tilde-tagged, per ladderKings' own warning that a
// proportional mapping assumes a correlation of one and is false on exactly the days it matters.
{ const row=irtBuildCsv().csv.split('\r\n').find(l=>/^EPU26/.test(l) && /QQQ KING/.test(l));
  // ⚠ BY COLUMN INDEX, NOT BY SUBSTRING. The first version tested /,2,1,/ against the whole row and
  // passed with the style mutated to SOLID — ",2,1," also occurs in the band fields near the end.
  // A mutation caught it. PENWIDTH is column 4, PENSTYLE column 5.
  const c=row?row.split(','):[];
  ok(c[4]==='2' && c[5]==='1',
     '4p7 the projection is DASHED (PENSTYLE=1), so it cannot pass for a native level', [c[4],c[5]]);
  ok(row && / ~/.test(c[2]), '4p8 ...and wears the tilde', c[2]);
  // ⚠ THE PANEL ROUNDS TO WHOLE POINTS, THE EXPORT MUST NOT. Operator, 2026-08-28: "on my tapereader
  // app, i have it rounded to the nearest whole number; on the export it should be based on the ES
  // chart which is in .25 increments." The rail's DISPLAY uses frameNum (whole points, D-9); the
  // export reads the RAW bearing off the same array and applies its own 0.25 rounding. Same
  // quantity, two precisions — and nobody may "fix" one to match the other.
  const keep=RAIL_QQQ; RAIL_QQQ={ at:7691.63, book:'QQQ', raw:650, kind:'proportional' };
  const r2=irtBuildCsv().csv.split('\r\n').find(l=>/^EPU26/.test(l) && /QQQ KING/.test(l));
  const px=parseFloat(r2.split(',')[1]);
  ok(Math.abs(px-7691.75)<=0.001,
     '4p9 a raw 7691.63 bearing exports as 7691.75 — the ES tick, NOT the panel\'s whole point (7692)', px);
  RAIL_QQQ=keep; }

// ---------- 4x. ⚠ ES AND NQ TRADE IN QUARTER POINTS — INCLUDING A HELD KING ----------
// The operator, 2026-08-28: "remember that es is in 1/4 pt". The live path has rounded to the tick
// since v14.x; the HOLD path is new, and a latched King re-enters through the same conversion, so it
// must land on the same grid. A level at 7726.63 is not a price anyone can trade against.
{ delete LS[IRT_KINGS_KEY];
  const Bgood=irtBuildCsv();                                   // one good tick seeds every King
  // ⚠ FUTURES ROWS ONLY. The ETF target (cash SPY/QQQ) trades in cents and is written with tick 0 —
  // asserting a 0.25 grid on it would be asserting the wrong instrument's rules, which is this
  // project's oldest defect in miniature. ES and NQ are the quarter-point instruments.
  const isFut=(l)=>/^E(PU|NQU)/.test(l);
  ok(Bgood.csv.trim().split('\r\n').slice(1).filter(isFut).every(function(l){
       const px=parseFloat(l.split(',')[1]);
       return Math.abs(px/0.25 - Math.round(px/0.25)) < 1e-9;
     }), '4x every LIVE ES/NQ row lands on the 0.25 tick');
  const keepT=global.tapeMap, keepF=global.LASTFEED, keepQ=QQQ_TAPE, keepL=QQQ_LADDER;
  global.tapeMap=()=>null; global.LASTFEED={ SPY:{ ts:Date.now()-999999, j:{} } };
  QQQ_TAPE=()=>null; QQQ_LADDER=()=>null;                      // every reader blind: all three HELD
  const Bheld=irtBuildCsv();
  const heldRows=Bheld.csv.trim().split('\r\n').slice(1);
  // ⚠ NAME EVERY KING. "at least three rows" passed with the SPXW hold deleted — a mutation proved
  // it. An assertion that counts rows is not an assertion about WHICH rows.
  ok(/SPXW KING/.test(Bheld.csv) && /SPY KING/.test(Bheld.csv) && /QQQ KING/.test(Bheld.csv),
     '4x2 ...and a fully blind tick still writes ALL THREE Kings it saw today',
     heldRows.map(l=>l.split(',')[2]));
  // ⚠ AND NEVER ACROSS DAYS. A held King from this session is a level minutes old; one from
  // yesterday is a different book. Same rule as the QQQ latch (q4).
  { const stash=LS[IRT_KINGS_KEY];
    const y=JSON.parse(stash); y.day='2026-08-26';
    LS[IRT_KINGS_KEY]=JSON.stringify(y);
    const Bstale=irtBuildCsv();
    ok(!Bstale || (!/SPXW KING/.test(Bstale.csv) && !/SPY KING/.test(Bstale.csv)),
       '4x2b yesterday\'s Kings are NOT held into today', Bstale && Bstale.csv.split('\r\n').length);
    LS[IRT_KINGS_KEY]=stash; }
  ok(heldRows.filter(isFut).every(function(l){
       const px=parseFloat(l.split(',')[1]);
       return Math.abs(px/0.25 - Math.round(px/0.25)) < 1e-9;
     }), '4x3 ...with every HELD ES/NQ row on the 0.25 tick too — a held King is still a tradeable price');
  global.tapeMap=keepT; global.LASTFEED=keepF; QQQ_TAPE=keepQ; QQQ_LADDER=keepL; }

// ---------- 4g. the face's ladder OUTRANKS the tape (v13.2: one quantity, one source) ----------
{ const keepL=QQQ_LADDER, keepT=QQQ_TAPE;
  QQQ_LADDER=()=>({ king:660, count:100, src:'trinity', pct:{'660.00':100} });
  QQQ_TAPE=()=>({ king:650, count:20, fromFeed:false, pct:{'650.00':-100} });
  const row=irtBuildCsv().csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  // ⚠ the ratio in force here is whatever irtNqRatio resolves (test 4e cached a live one), so the
  // expectation is derived from it rather than hardcoded — a hardcoded 41.9 tested the RATIO, not
  // the KING, and would have passed on the wrong strike.
  const RNQ=irtNqRatio(CFG.irt).r;
  ok(row && Math.abs(parseFloat(row.split(',')[1]) - 660*RNQ) <= 0.25,
     '4g the export writes the King the FACE shows (660), not the tape\'s (650)', row&&row.split(',')[1]);
  ok(/via trinity/.test(IRT_LAST.nqWhy||''), '4g2 ...and says which array it read', IRT_LAST.nqWhy);
  QQQ_LADDER=keepL; QQQ_TAPE=keepT; }
// ---------- 4h. THE LATCH — an unreadable tick must never DELETE the operator's level ----------
{ const keepL=QQQ_LADDER, keepT=QQQ_TAPE;
  QQQ_LADDER=()=>({ king:661, count:100, src:'trinity', pct:{'661.00':100} });
  irtBuildCsv();                                   // seeds today's latch
  QQQ_LADDER=()=>null; QQQ_TAPE=()=>null;          // both readers go blind
  const row2=irtBuildCsv().csv.split('\r\n').find(l=>/QQQ KING/.test(l));
  ok(row2 && Math.abs(parseFloat(row2.split(',')[1]) - 661*irtNqRatio(CFG.irt).r) <= 0.25,
     '4h a blind tick HOLDS the last good King — the line stays on his chart', row2&&row2.split(',')[1]);
  ok(/latched/.test(IRT_LAST.nqWhy||''), '4h2 ...and the export says it is held, not fresh', IRT_LAST.nqWhy);
  QQQ_LADDER=keepL; QQQ_TAPE=keepT; }

// ---------- 5. everything else stays OUT ----------
ok(!/IF /.test(b.csv), '5a the IF walls are out of the file');
ok(!/SUCC/.test(b.csv), '5b SUCC is out');
ok(!/SPXW (BRK|ACC|GK|RRUG|RUG|BAL)/.test(b.csv), '5c the rail percentage rows are out');
ok(!/SPY \d+%/.test(b.csv) && !/QQQ \d+%/.test(b.csv), '5d the SPY/QQQ percentage rows are out');
ok(!/NextStop|PBentry|FLIP|D-SPY/.test(b.csv), '5e and none of the long-dead lanes returned');

// ---------- 6. resilience: any king alone still writes; all dark writes nothing ----------
// ⚠ (v14.74) EVERY "a dark source writes nothing" CASE MUST CLEAR THE LATCH FIRST, or it silently
// tests the latch instead of the darkness — the same trap that hid inside 6b/6c when the QQQ latch
// landed. The latch is the FEATURE here; these assertions are about what happens with nothing held.
{ delete LS[IRT_KINGS_KEY];
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():null); // SPXW tape dead
  const B1=irtBuildCsv();
  ok(!!B1 && !/SPXW KING/.test(B1.csv) && /SPY KING/.test(B1.csv) && /QQQ KING/.test(B1.csv), '6a SPXW dark → SPY + QQQ kings still write');
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE()); }
{ global.tapeMap=(s)=>(s==='QQQ'?null:SPXW_TAPE()); global.LASTFEED={ SPY:null };
  // ⚠ (v14.73) THE LATCH MUST BE CLEARED FOR THIS TEST TO MEAN ANYTHING. With a King latched from
  // earlier in the session the export SHOULD still write it — that is the whole point of the latch —
  // so "every source dark" now has to mean dark AND nothing held. Leaving the latch in place made
  // this assertion silently test the latch instead of the darkness.
  delete LS[IRT_QQQK_KEY]; delete LS[IRT_KINGS_KEY];
  const B2=irtBuildCsv();
  ok(!!B2 && /SPXW KING/.test(B2.csv) && !/SPY KING/.test(B2.csv) && !/ENQU26/.test(B2.csv), '6b only the SPXW king alive → it writes alone');
  const keep6=QQQ_TAPE;
  // ⚠ (v14.75) the RAIL's bearing is a source too — "every source dark" now has to include it, or
  // this assertion silently tests everything except the newest row.
  global.tapeMap=()=>null; QQQ_TAPE=()=>null; global.ladderFor=()=>null;
  delete LS[IRT_QQQK_KEY]; delete LS[IRT_KINGS_KEY];
  const keepRail=RAIL_QQQ; RAIL_QQQ=null;
  ok(irtBuildCsv()==null, '6c every source dark AND nothing latched → nothing is written, never an empty confident file');
  RAIL_QQQ=keepRail;
  QQQ_TAPE=keep6; global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE());
let QQQ_LADDER=()=>null;                      // (v14.73) the face's array; null = fall through to the tape
global.ladderFor=(s)=>(s==='QQQ'?QQQ_LADDER():null);
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


// ---- (v14.52) THE CSV IS WRITTEN IN PLACE, NOT REPLACED --------------------------------------
// Operator-reported: "it has problems reading from a local file unless i refresh — only after i
// refresh will the lines be displayed." Cause: createWritable() defaults to keepExistingData:false,
// which Chromium implements as an ATOMIC REPLACE — a swap file renamed over the original on close.
// The contents were always right, but the file IDENTITY changed on every export, so IRT (which opens
// the file once and polls it every minute) kept polling an orphaned file until a manual refresh made
// it re-open by path. Writing in place keeps the identity and the poll just works.
{
  const EX = ex('irtExportNow');
  ok(/createWritable\(\{keepExistingData:true\}\)/.test(EX),
     'ip1 the writable keeps the existing file instead of creating a swap to rename over it');
  ok(/type:'write', position:0/.test(EX),
     'ip2 ...and writes from position 0 of that same file');
  ok(/w\.truncate\(bytes\)/.test(EX),
     'ip3 TRUNCATE IS NOT OPTIONAL — without it a shorter export leaves the previous tail behind');
  ok(/new Blob\(\[built\.csv\]\)\.size/.test(EX),
     'ip4 truncate takes BYTES, so the length is measured as bytes and not characters');
  // the fallback must exist: no levels at all is worse than levels that need a refresh
  ok(/in-place write refused/.test(EX) && /fell back to replace/.test(EX),
     'ip5 a browser that refuses the in-place path falls back to the replacing write');
  ok(/inPlace:true/.test(EX) && /inPlace:false/.test(EX),
     'ip6 ...and IRT_LAST records WHICH path ran, so the two can be told apart');
  // ordering: truncate must follow the write, or it would clip the data just written
  ok(EX.indexOf("position:0") < EX.indexOf("w.truncate(bytes)"),
     'ip7 the truncate happens AFTER the write, not before it');
}

// ---- (v14.53) THE PERMISSION PATH — the export was silently dead for 54 minutes -----------------
// Measured on the live panel 2026-08-27 10:27 CT: handle SET, queryPermission "prompt", IRT_LAST
// frozen 54 minutes stale while irtTick fired every 180s. Chrome resets File System Access
// permission to "prompt" on EVERY page load, and requestPermission() REQUIRES A USER GESTURE — so
// from a timer it REJECTS, and the old code had no .catch on that inner promise. The rejection
// vanished and IRT_LAST was never written, so the face showed an unrelated stale error.
// v14.52's in-place write had therefore never executed even once.
{
  const fs=require('fs');
  const src=fs.readFileSync('./v10.js','utf8');
  const i=src.indexOf('function irtExportNow'); const j=src.indexOf('function irtTick');
  const body=(i>=0&&j>i)?src.slice(i,j):'';
  ok(body.length>0, 'irtExportNow is findable');
  ok(/navigator\.userActivation/.test(body),
     'it checks navigator.userActivation before ever calling requestPermission');
  ok(/needsGesture\s*:\s*true/.test(body),
     'it flags needsGesture so the FACE can say a click is required');
  // ⚠ the specific regression: an inner promise with no .catch
  const reqIdx=body.indexOf('requestPermission');
  const after=reqIdx>=0?body.slice(reqIdx):'';
  ok(reqIdx>=0 && /\.catch\(/.test(after),
     'the requestPermission promise has a .catch — an unhandled rejection is what froze IRT_LAST');
  ok(/permission request refused/.test(body),
     'a refused permission request reports itself instead of disappearing');
  // and it must NOT blindly retry requestPermission from a timer
  ok(/if\(!active\)/.test(body.replace(/\s/g,'')) || /if\s*\(\s*!active\s*\)/.test(body),
     'with no user activation it reports rather than attempting a call that cannot succeed');
}
// the face must surface it — the config drawer is not enough, nobody opens it while trading
{
  const fs=require('fs');
  const src=fs.readFileSync('./v10.js','utf8');
  const i=src.indexOf('function feedStatusHtml');
  const body=i>=0?src.slice(i,i+6000):'';
  ok(/IRT_LAST\s*&&\s*IRT_LAST\.needsGesture/.test(body),
     'feedStatusHtml reads the needsGesture flag');
  ok(/IRT needs a click/.test(body),
     'and prints a footer warning naming the fix');
}

// ============================================================================================
// (v14.73) THE QQQ KING — face first, tape second, LATCH last.
// The operator, 2026-08-28: "this is so strange because you show the qqq king in the tapereader
// app so you know the level." He was right: the face read one array, the export re-derived from
// another with a stricter rule, and the level silently vanished from his chart for two hours.
// ⚠ Every assertion below EXECUTES irtQqqKing under a stubbed world — a grep cannot tell which
// source won, and "which source won" is the entire behaviour.
// ============================================================================================
{
  const mk = (o) => {
    const store = {};
    global.localStorage = { getItem:k=>store[k]||null, setItem:(k,v)=>{store[k]=v;}, _s:store };
    global.ctTodayStr = () => '2026-08-28';
    global.ladderFor = () => o.ladder || null;
    global.tapeMap   = () => o.tape || null;
    return store;
  };
  eval(ex('irtQqqKing'));

  // 1 · the face's ladder wins
  mk({ ladder:{ king:717, count:100, src:'trinity', pct:{'717.00':100} } });
  let r = irtQqqKing();
  ok(r.k===717 && r.src==='trinity', 'q1 the export takes the King the FACE is showing', [r.k, r.src]);

  // 2 · a feed-sourced tape is ACCEPTED and TAGGED, where v14.15 refused it outright
  mk({ ladder:null, tape:{ king:718, count:60, fromFeed:true, pct:{'718.00':100} } });
  r = irtQqqKing();
  ok(r.k===718 && r.src==='feed',
     'q2 a feed King is written and TAGGED, not refused — refusing bought a missing level, not a safer one', [r.k, r.src]);

  // 3 · THE LATCH — nothing readable, but today's King was seen earlier
  const st = mk({ ladder:{ king:717, count:100, src:'trinity', pct:{'717.00':100} } });
  irtQqqKing();                                   // seeds the latch
  global.ladderFor = () => null; global.tapeMap = () => null;
  r = irtQqqKing();
  ok(r.k===717 && r.src==='latched' && /held from/.test(r.why),
     'q3 an unreadable tick HOLDS the last good King instead of deleting the level', [r.k, r.src, r.why]);

  // 4 · ...but never across days
  const st2 = mk({ ladder:null, tape:null });
  st2['gpts_irt_qqqking_v1'] = JSON.stringify({k:700, pct:100, day:'2026-08-27', t:Date.now()});
  r = irtQqqKing();
  ok(r.k===null && /nothing latched today/.test(r.why),
     'q4 yesterday\'s King is NOT latched into today — a stale level is worse than none', r.why);

  // 5 · a thin ladder is not a ladder
  mk({ ladder:{ king:717, count:2, src:'trinity', pct:{} }, tape:null });
  r = irtQqqKing();
  ok(r.k===null, 'q5 a 2-strike ladder is refused, not drawn', [r.k, r.src]);

  // 6 · the label no longer claims 100% — a King is 100% by definition (operator, 2026-08-28)
  const B = ex('irtBuildCsv');
  ok(!/KING 100%/.test(B) && /'SPXW KING'/.test(B) && /'QQQ KING'/.test(B),
     'q6 King labels drop the redundant 100%');
  ok(/IRT_LAST\.nqWhy/.test(B),
     'q7 the export RECORDS why a QQQ row was skipped — this cost an afternoon of inference');
}

console.log('test_irt_export: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);

