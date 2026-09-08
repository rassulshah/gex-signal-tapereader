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
eval(['irtRound','irtCsvRow','irtRatio','irtNqRatio','kingLatchTick','irtKingLatch','irtKingHeld','irtGLatch','irtGHeld','irtQqqKing','irtQqqTop','irtBuildCsv'].map(ex).join('\n'));   // (v15.80) irtQqqTop: G2..G5 on NQ
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
let IFL_ROWS=[{id:'CR0', k:7750, disp:7767.83, und:773.2},
               {id:'PS0', k:7650, disp:7667.60, und:763.2},
               {id:'FLIP', k:7700, disp:7717.71, und:768.2},
               {id:'Mag', k:7710, disp:7727.74, und:769.2}];
global.ifLadder=(sym)=>({ dispScale:1.0023, rows:IFL_ROWS, err:null,
                          srcSym:(sym==='QQQ'?'QQQ':'SPX') });   // srcSym is real: ifLadder returns it
let IF_CHAIN={ dte0:{ gf:{ flip:7695 } } };
global.ifChain=()=>IF_CHAIN;
// (v15.76) SEVEN strikes, not three: the G rows take Skylit's ranks 2-5 by |%King|, so the fixture
// needs a book with more than five nodes, a NEGATIVE node big enough to outrank a positive one
// (7700 at -63 must beat 7650 at +41 — size, not sign), and two nodes that must be LEFT OUT.
const SPXW_TAPE=()=>({ king:7710, pct:{ '7710.00':100, '7630.00':85, '7650.00':41, '7700.00':-63,
                                        '7680.00':30, '7600.00':-22, '7720.00':18 } });
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
// (v14.79) the 0DTE trio (CW0 · PW0 · FLIP0) rides EVERY configured target, like the kings do.
// (v15.76) so do G2..G5 — the rest of Skylit's top-5 (R-13).
// rows[] = 2 kings + 3 IF levels + 4 G = 9, x 2 target symbols = 18, + the NQ king + the ES projection.
// (v15.80) the ES projection of the QQQ King is gone — "qqq should only be converted for nq"; the NQ symbol now carries
// the QQQ book's G2..G5 too — this fixture's QQQ tape has ONE node beyond the King (648 at 44%), so one G row.
ok(b.n===11 && lines.length===1+9*2+2,
   '1b n=11 (2 kings + 3 IF levels + G2..G5 + NQ king + NQ G2); rows = 9x2 syms + 2 (v15.80: no ES projection, G rows on NQ)', [b.n,lines.length]);
const eRows=lines.filter(l=>l.startsWith('EPU26,'));
const qRows=lines.filter(l=>l.startsWith('ENQU26,'));
ok(eRows.length===9 && qRows.length===2,
   '1c EPU26 carries SPXW + SPY + CW0 + PW0 + FLIP + G2..G5 (v15.80: no projected QQQ); ENQU26 the native QQQ King + its G2', [eRows.length,qRows.length]);
ok(eRows.filter(l=>/QQQ KING/.test(l)).length===0,
   '1c2 ...and no QQQ King wears the ES symbol (v15.80)');
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
  global.ifLadder=()=>({ dispScale:1.0023, rows:IFL_ROWS, err:null }); }
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
// ---------- 4i. THE 0DTE LEVELS FROM INSIDERFINANCE (v14.79) ----------
// Operator: "put the CW0 and PW0 and the Flip0 ... make the 0 dte levels dotted ... Put wall should
// be green, call wall should be red, flip can be purple." ⚠ Read from ifLadder's OWN rows — the
// array the rail draws — never re-derived here.
{
  const B=irtBuildCsv(); const L=B.csv.split('\r\n').filter(l=>/^EPU26/.test(l));
  const row=(name)=>L.find(l=>l.split(',')[2].indexOf(name)===0);
  const cw=row('CW0'), pw=row('PW0'), fl=L.find(l=>/FLIP/.test(l.split(',')[2]));
  ok(!!cw && !!pw, 'i1 CW0 and PW0 are exported', [!!cw,!!pw]);
  ok(cw && cw.split(',')[3]===String((240<<16)+(97<<8)+109), 'i2 the CALL wall is RED', cw&&cw.split(',')[3]);
  ok(pw && pw.split(',')[3]===String((46<<16)+(194<<8)+126), 'i3 the PUT wall is GREEN', pw&&pw.split(',')[3]);
  ok(fl && fl.split(',')[3]===String((163<<16)+(113<<8)+247), 'i4 the FLIP is PURPLE', fl&&fl.split(',')[3]);
  // (v15.80) "i dont want dashed or dotted lines, make all solid lines" — every row writes PENSTYLE 0
  ok(cw && cw.split(',')[5]==='0' && pw.split(',')[5]==='0' && fl.split(',')[5]==='0',
     'i5 all three 0DTE lines are SOLID (PENSTYLE 0) — v15.80, his call', [cw&&cw.split(',')[5], pw&&pw.split(',')[5], fl&&fl.split(',')[5]]);
  ok(irtBuildCsv().csv.split('\r\n').filter(l=>l && !/^SYMBOL/.test(l)).every(l=>l.split(',')[5]==='0'), 'i5b …and so does every other line in the file');
  // ⚠ THE KINGS ARE SOLID — "Make the king lines solid". A style that says "king" must not also say
  // "0DTE wall"; the two families are told apart by the line, not only by the label.
  const kings=L.filter(l=>/KING/.test(l.split(',')[2]) && !/QQQ/.test(l.split(',')[2]));
  ok(kings.length>=2 && kings.every(l=>l.split(',')[5]==='0'),
     'i6 the SPXW and SPY Kings are SOLID (PENSTYLE 0)', kings.map(l=>l.split(',')[2]+':'+l.split(',')[5]));
  // the price comes from ifLadder's own `disp`, converted back through the SAME ratio the loop applies
  const R=irtRatio().r;
  ok(cw && Math.abs(parseFloat(cw.split(',')[1]) - Math.round((7767.83/R*R)/0.25)*0.25) <= 0.25,
     'i7 CW0 lands on ifLadder\'s own disp price, on the 0.25 tick', cw&&cw.split(',')[1]);
  // ⚠⚠ THE FLIP ROW IS THE 0DTE FLIP, AND ONLY THAT (v14.80). Operator, 2026-08-28: "the flip 0dte
  // is displayed when 0dte is selected just like how you get the walls for 0 dte." The companion
  // fetches their page with NO expiry filter, so `pub.zeroGamma` is their ALL-EXPIRY view; the 0DTE
  // flip is `dte0.gf.flip` — their contracts, front expiry only, exactly like CW0/PW0. Plain FLIP0,
  // no asterisk, because CR0/PS0 carry none and the provenance is identical.
  ok(fl && fl.split(',')[2].split(' ')[0]==='FLIP0',
     'i8 the flip is labelled FLIP0 — same provenance as CW0/PW0, no asterisk', fl&&fl.split(',')[2]);
  // 7695 (dte0.gf.flip) x 1.0023 = 7712.70 -> 7712.75 on the tick. The ladder's ALL-EXPIRY FLIP row
  // sits at disp 7717.71, five points away — so this assertion tells the two sources apart by PRICE,
  // not merely by label, which is the failure the operator caught.
  ok(fl && parseFloat(fl.split(',')[1])===7712.75,
     'i8b ...and its price is dte0.gf.flip x dispScale (7712.75), NOT the ladder\'s all-expiry 7717.71',
     fl&&fl.split(',')[1]);
  // ⚠⚠ NO ALL-EXPIRY FALLBACK — his call, asked and answered 2026-08-28: "draw nothing". A purple
  // dotted line answering a DIFFERENT question cannot be told from this one at a glance.
  { const keep=IF_CHAIN; IF_CHAIN={};                       // no 0DTE flip available
    const B2=irtBuildCsv();
    ok(!/FLIP/.test(B2.csv),
       'i9 no 0DTE flip -> NO flip row at all; their all-expiry zero gamma is never substituted',
       (B2.csv.split('\r\n').find(l=>/FLIP/.test(l))||'(absent)'));
    ok(/no dte0 gamma flip/.test(IRT_LAST.ifWhy||''), 'i9b ...and the export says so', IRT_LAST.ifWhy);
    IF_CHAIN=keep; }
  { const keep=IFL_ROWS; IFL_ROWS=[];
    const B3=irtBuildCsv();
    ok(!/CW0|PW0/.test(B3.csv), 'i10 no IF ladder -> no 0DTE rows invented');
    ok(/no IF ladder|0DTE from/.test(IRT_LAST.ifWhy||''), 'i11 ...and the export says why', IRT_LAST.ifWhy);
    IFL_ROWS=keep; }
}

// ---------- 4p. THE QQQ KING IS NOT ON ES ANY MORE (v15.80) ----------
// v14.75 wrote the rail's `~ QQQ` bearing onto the ES symbol as a dashed row. Operator, 2026-09-08: "currently the
// irt export includes qqq converted for ES, remove this. qqq should only be converted for nq."
{ const B=irtBuildCsv();
  const proj=B.csv.split('\r\n').filter(l=>/^EPU26/.test(l) && /QQQ KING/.test(l));
  ok(proj.length===0, '4p the ES symbol carries NO QQQ King row (v15.80: "qqq should only be converted for nq")', proj);
  ok(/off — QQQ converts for NQ only/.test(IRT_LAST.xqWhy||''), '4p2 ...and the export says it is a decision, not an absence', IRT_LAST.xqWhy);
  const keep=RAIL_QQQ; RAIL_QQQ={ at:7745.00, book:'QQQ', raw:655, kind:'proportional' };
  ok(!irtBuildCsv().csv.split('\r\n').some(l=>/^EPU26/.test(l) && /QQQ KING/.test(l)), '4p3 ...whatever the rail\'s bearing says');
  RAIL_QQQ=keep;
  const nq=B.csv.split('\r\n').filter(l=>/^ENQU26/.test(l) && /QQQ KING/.test(l));
  ok(nq.length===1, '4p4 the QQQ King still writes on the NQ symbol, once', nq.length); }

// ---------- 4q. G2..G5 ON NQ — the QQQ book's next four, like the ES rows (v15.80, R-20) ----------
// "Can you add additional levels for NQ as well, similar to how you have it for ES, so NQ would include G2-G5 also."
{ const keepQ=QQQ_TAPE;
  QQQ_TAPE=()=>({ king:650, count:20, fromFeed:false, pct:{ '650.00':-100, '648.00':44, '655.00':-71, '640.00':30, '660.00':52, '645.00':-9, '652.00':12 } });
  const B=irtBuildCsv(); const g=B.csv.split('\r\n').filter(l=>/^ENQU26/.test(l) && /,G[2-5]/.test(l));
  const nqR=irtNqRatio(CFG.irt).r;   // the ratio in force (last-good by now — 4e stored one); the King row may be a held King
  ok(g.length===4 && g.map(l=>l.split(',')[2].replace(/ ~$/,'')).join(' ')==='G2 G3 G4 G5', '4q four G rows on ENQU26, G2..G5 (the ~ rides with the King\'s when the ratio is not live)', g.map(l=>l.split(',')[2]));
  ok(Math.abs(parseFloat(g[0].split(',')[1])-655*nqR)<=0.13 && Math.abs(parseFloat(g[1].split(',')[1])-660*nqR)<=0.13 && Math.abs(parseFloat(g[2].split(',')[1])-648*nqR)<=0.13 && Math.abs(parseFloat(g[3].split(',')[1])-640*nqR)<=0.13,
     '4q2 ranked by |%King| with the King dropped: 655 (-71) · 660 (52) · 648 (44) · 640 (30) — size, not sign; 645 and 652 left out; each × the SAME NQ ratio as the King row, on the 0.25 tick', g.map(l=>l.split(',')[1]));
  ok(g.every(l=>l.split(',')[3]===String(IRT_COLORS.gate) && l.split(',')[4]==='1' && l.split(',')[5]==='0'), '4q3 white, width 1, solid — the hierarchy reads at a glance');
  ok(/live via tape \(G2 655 -71%/.test(IRT_LAST.gqWhy||''), '4q4 IRT_LAST.gqWhy names the source and the rows', IRT_LAST.gqWhy);
  ok(!B.csv.split('\r\n').some(l=>/^ENQU26/.test(l) && /,G1,/.test(l)), '4q5 no G1 — the King\'s slot is the QQQ KING line');
  // the QQQ tape goes blind: the four are HELD (day-scoped, under GQ), never deleted
  QQQ_TAPE=()=>({ king:null, count:0, pct:{} });
  const B2=irtBuildCsv(); const g2=B2.csv.split('\r\n').filter(l=>/^ENQU26/.test(l) && /,G[2-5]/.test(l));
  ok(g2.length===4 && /^held \d+m/.test(IRT_LAST.gqWhy||''), '4q6 a blind tick keeps the four rows from the day\'s latch', [g2.length, IRT_LAST.gqWhy]);
  QQQ_TAPE=keepQ; LS[IRT_KINGS_KEY]=JSON.stringify(Object.assign(JSON.parse(LS[IRT_KINGS_KEY]||'{}'),{GQ:null})); }

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

// ---------- 4t. G2..G5 — THE REST OF SKYLIT'S TOP-5, WHITE (v15.76, R-13) ----------
// Operator, 2026-09-07: "i want to update the irt export so it exports the top 5 levels for the spx
// also. G1 - G5. all should be white. in addition to this, it already exports the kings, cw0, pw0 and
// the flip." Asked which five, he chose MIRROR SKYLIT: NODES=5 draws the five largest nodes by |%King|
// and the King is always #1 of them — so the King's slot is the gold SPXW KING line and the four that
// follow are G2..G5. ⚠ No G1 row: one level, one line.
{
  const G=eRows.filter(l=>/,G\d,/.test(l));
  ok(G.length===4 && G.map(l=>l.split(',')[2]).join(' ')==='G2 G3 G4 G5',
     '4t1 EPU26 carries exactly G2 G3 G4 G5, in rank order', G.map(l=>l.split(',')[2]));
  ok(!/,G1[ ,]/.test(b.csv) && !/,G6[ ,]/.test(b.csv), '4t2 no G1 (the King\'s slot is the gold line) and nothing past G5');
  // ranks 2-5 of {7710:100, 7630:85, 7700:-63, 7650:41, 7680:30, 7600:-22, 7720:18} by |%King|:
  // 7630 → 7700 → 7650 → 7680; 7600 and 7720 stay out. Prices go SPX x dispScale 1.0023 on the 0.25 tick.
  const px=G.map(l=>l.split(',')[1]);
  ok(px.join(' ')==='7647.500000 7717.750000 7667.500000 7697.750000',
     '4t3 G2=7630 G3=7700 G4=7650 G5=7680 — the -63% put node OUTRANKS the +41% call node (size, not sign)', px);
  ok(!/,7613\.500000,|,7737\.750000,/.test(G.join('\n')), '4t3b the 6th and 7th nodes (7600 at -22 → 7613.50, 7720 at 18 → 7737.75) are left out');
  ok(G.every(l=>l.split(',')[3]===String((255<<16)+(255<<8)+255)), '4t4 all four are WHITE (RGB 16777215) — his call', G.map(l=>l.split(',')[3]));
  ok(G.every(l=>l.split(',')[4]==='1' && l.split(',')[5]==='0'), '4t4b width 1, SOLID (by column index, not substring) — under the King\'s 3');
  // the ETF target carries the same four in SPY space, cents, no tick
  const gSpy=lines.filter(l=>l.startsWith('SPY,') && /,G\d,/.test(l));
  ok(gSpy.length===4 && Math.abs(parseFloat(gSpy[0].split(',')[1]) - 7630*1.0023/10.0538) < 0.006,
     '4t5 SPY rides the same four: G2 7630 x 1.0023 / 10.0538 = 760.66', gSpy.map(l=>l.split(',')[1]));
  ok(G.every(l=>!/ ~/.test(l.split(',')[2])), '4t6 a LIVE ratio leaves the G labels untagged');
  { global.FUTMODE={ fam:'ES', r:10.0538, live:false };
    const Bt=irtBuildCsv(); const Gt=Bt.csv.split('\r\n').filter(l=>l.startsWith('EPU26,') && /,G\d ~,/.test(l));
    ok(Gt.length===4, '4t6b ...and a last-known ratio marks every G label with ~ like every other EPU26 row', Gt.length);
    global.FUTMODE={ fam:'ES', r:10.0538, live:true }; }
  irtBuildCsv();
  ok(IRT_LAST.gWhy==='live (G2 7630 85%, G3 7700 -63%, G4 7650 41%, G5 7680 30%)',
     '4t7 IRT_LAST.gWhy names the four strikes and their sizes', IRT_LAST.gWhy);
}
// ⚠ THE KING'S SLOT IS THE EXPORTED KING'S STRIKE — the LATCHED crown (v14.19), not the tape's 100%.
// During a flap the new 100% node is not the exported King, so it must show up as G2 rather than
// vanish: a level under a lesser label beats a level absent from the chart.
{ irtBuildCsv();                                                       // latches the 7710 crown
  const T2=()=>({ king:7700, pct:{ '7700.00':-100, '7710.00':96, '7650.00':40 } });
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():T2());
  const Bf=irtBuildCsv(); const L=Bf.csv.split('\r\n').filter(l=>l.startsWith('EPU26,'));
  const kf=L.find(l=>/SPXW KING/.test(l)); const g2=L.find(l=>/,G2,/.test(l)); const g3=L.find(l=>/,G3,/.test(l));
  ok(kf && /^EPU26,7727\.750000,/.test(kf), '4t8 the exported King is still the latched 7710', kf);
  ok(g2 && /^EPU26,7717\.750000,/.test(g2), '4t8b ...and the tape\'s new 100% crown (7700) is G2, not dropped and not doubled', g2);
  ok(g3 && /^EPU26,7667\.500000,/.test(g3) && !L.some(l=>/,G[2-5],/.test(l) && /,7727\.750000,/.test(l)),
     '4t8c 7710 is never ALSO a G row — one level, one line', L.filter(l=>/,G\d,/.test(l)));
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE()); delete LS[KING_LATCH_KEY]; }
// ⚠ HELD like the Kings (v14.74): a blind tick must not delete four lines from his chart.
{ irtBuildCsv();                                                       // seeds today's G hold
  global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():null);                     // the SPXW tape goes blind
  const Bh=irtBuildCsv(); const Gh=Bh.csv.split('\r\n').filter(l=>l.startsWith('EPU26,') && /,G\d,/.test(l));
  ok(Gh.length===4 && Gh.map(l=>l.split(',')[1]).join(' ')==='7647.500000 7717.750000 7667.500000 7697.750000',
     '4t9 a blind tick HOLDS the four G rows at their last good prices', Gh.map(l=>l.split(',')[1]));
  ok(/^held \d+m/.test(IRT_LAST.gWhy||''), '4t9b ...and says they are held, not fresh', IRT_LAST.gWhy);
  ok(Gh.every(l=>l.split(',')[4]==='1' && l.split(',')[5]==='0' && l.split(',')[3]===String(16777215)), '4t9c held rows keep the same white, width 1, solid');
  const keepDay=global.ctTodayStr; global.ctTodayStr=()=>'2026-08-28';   // the next session
  const Bd=irtBuildCsv();
  ok(!(Bd && /,G\d,/.test(Bd.csv)), '4t9d ...and NEVER across days — yesterday\'s nodes are a different book');
  ok(/nothing latched today/.test(IRT_LAST.gWhy||''), '4t9e ...which the export says in words', IRT_LAST.gWhy);
  global.ctTodayStr=keepDay; global.tapeMap=(s)=>(s==='QQQ'?QQQ_TAPE():SPXW_TAPE()); }
// the hold only ever returns G2..G5 rows — a corrupt or foreign entry cannot draw a stray line
{ LS[IRT_KINGS_KEY]=JSON.stringify({ day:ctTodayStr(), G:{ rows:[{k:760, lbl:'G1'}, {k:'x', lbl:'G2'}, {k:761.5, lbl:'G3'}], t:Date.now() } });
  const H=irtGHeld();
  ok(H && H.rows.length===1 && H.rows[0].lbl==='G3', '4t10 irtGHeld filters to well-formed G2..G5 rows', H&&H.rows);
  delete LS[IRT_KINGS_KEY]; }

// ⚠ ONE CONVERSION FOR THE WHOLE SPXW BOOK (v15.76): the King and the G rows go through the same
// closure, and on a LIVE ES chart that closure prefers the chart's own basis (dispScale / R) over the
// IF ladder's undScale (v14.14, chart-frame independence). A fixture whose ladder carries BOTH is the
// only way to tell the two apart — the mutation run found the earlier fixture could not.
{ global.ifLadder=(sym)=>({ dispScale:1.0023, undScale:0.0995, rows:IFL_ROWS, err:null, srcSym:'SPX' });
  const Bb=irtBuildCsv(); const L=Bb.csv.split('\r\n').filter(l=>l.startsWith('EPU26,'));
  const kk=L.find(l=>/SPXW KING/.test(l)); const g2=L.find(l=>/,G2,/.test(l));
  ok(kk && /^EPU26,7727\.750000,/.test(kk), '4t11 on a live ES chart the King follows the chart basis (7727.75), not undScale (7713.25)', kk);
  ok(g2 && /^EPU26,7647\.500000,/.test(g2), '4t11b ...and so does G2 — same closure, same scale', g2);
  global.ifLadder=(sym)=>({ dispScale:1.0023, rows:IFL_ROWS, err:null, srcSym:(sym==='QQQ'?'QQQ':'SPX') }); }

// ---------- 5. everything else stays OUT ----------
ok(!/IF /.test(b.csv), '5a the IF walls are out of the file');
ok(!/SUCC/.test(b.csv), '5b SUCC is out');
ok(!/SPXW (BRK|ACC|GK|RRUG|RUG|BAL)/.test(b.csv), '5c the rail percentage rows are out');
ok(!/SPY \d+%/.test(b.csv) && !/QQQ \d+%/.test(b.csv), '5d the SPY/QQQ percentage rows are out');
// ⚠ (v14.79) FLIP LEFT THIS BAN LIST DELIBERATELY. The operator asked for it: "put the CW0 and PW0
// and the Flip0 that you are getting from inside finance in the irt export". The ban existed because
// the export was Kings-only and a stray lane meant a leak; a level he asked for is not a leak.
// The others stay banned — they were removed for cause and nothing has asked for them back.
ok(!/NextStop|PBentry|D-SPY/.test(b.csv), '5e and none of the long-dead lanes returned');
ok(/FLIP/.test(b.csv), '5e2 ...while the FLIP is now IN, by request');

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
  // ⚠ (v14.79) the IF ladder is a source too — "every source dark" must include it, or this
  // assertion quietly tests everything except the newest rows. Third time this pattern has bitten.
  global.tapeMap=()=>null; QQQ_TAPE=()=>null; global.ladderFor=()=>null;
  const keepIFL=IFL_ROWS; IFL_ROWS=[];
  delete LS[IRT_QQQK_KEY]; delete LS[IRT_KINGS_KEY];
  const keepRail=RAIL_QQQ; RAIL_QQQ=null;
  ok(irtBuildCsv()==null, '6c every source dark AND nothing latched → nothing is written, never an empty confident file');
  RAIL_QQQ=keepRail; IFL_ROWS=keepIFL;
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

// ============================================================================================
// (v14.76) THE ONE-CLICK PERMANENT GRANT.
// Chrome resets the File System Access permission on EVERY page load, and requestPermission()
// cannot succeed without user activation — which is why the export went silent at 10:04 on
// 2026-08-28 and again after each reload. Chrome 122+ offers "Allow on every visit"; all the panel
// has to do is ASK FROM A CLICK.
// ⚠ Every assertion below EXECUTES irtGrantFolder against a stubbed handle. A grep would not tell
// a granted path from a denied one, and the difference is the whole feature.
// ============================================================================================
{
  eval(ex('irtGrantFolder'));
  // ⚠ A SYNCHRONOUS THENABLE, not a real Promise. The first version stubbed with Promise.resolve()
  // and scheduled its assertions as microtasks — which run AFTER the summary and process.exit, so
  // six assertions silently never executed and the suite went green. Second time today. If a test
  // needs the event loop to finish before it asserts, it is not asserting.
  const sync=(v)=>({ then:(f)=>{ let r; try{ r=f(v); }catch(e){ return sync(v); } return sync(r); },
                     catch:()=>sync(v) });
  let asked=null, exported=0;
  global.renderCfg=()=>{};
  global.irtExportNow=()=>{ exported++; };
  const mk=(state, handle)=>{ asked=null; global.IRT_DIR_H=null;   // (v14.78) a fresh load has no cached handle
    global.repoKvGet=(k,cb)=>cb(handle===undefined
      ? { requestPermission:(o)=>{ asked=o&&o.mode; return sync(state); } }
      : handle); };

  // 1 · a granted click writes the file immediately — proof, not a promise
  mk('granted'); irtGrantFolder();
  ok(asked==='readwrite', 'g1 the grant asks for READWRITE, the mode the export needs', asked);
  ok(IRT_LAST.how==='permission granted' && IRT_LAST.err===null, 'g2 a granted click is recorded as granted', IRT_LAST.how);
  ok(exported===1, 'g3 ...and the file is written straight away, so the operator SEES it work', exported);

  // 2 · a denial says so in words he can act on — never a silent no-op
  mk('denied'); irtGrantFolder();
  ok(/permission denied/.test(IRT_LAST.err||''), 'g4 a denial is REPORTED, not swallowed', IRT_LAST.err);
  ok(exported===1, 'g5 ...and nothing is written on a denial', exported);

  // 3 · no folder picked yet -> it names the missing step
  mk('granted', null); irtGrantFolder();
  ok(/no folder picked/.test(IRT_LAST.err||''), 'g6 with no handle it names the missing step', IRT_LAST.err);

  // 4 · the button exists, is wired to a CLICK, and its hover names the Chrome option
  const CFGSRC = src.slice(src.indexOf('gpts-irt-grant')-200, src.indexOf('gpts-irt-grant')+900);
  ok(/Allow on every visit/.test(CFGSRC),
     'g7 the hover tells him the exact Chrome option that makes the grant permanent');
  // ⚠ g8 USED TO ASSERT A LITERAL BINDING and passed while the button did nothing, twice. What
  // matters is that a CLICK reaches irtGrantFolder — by delegation — not how it is spelled.
  ok(/gpts-irt-grant[\s\S]{0,80}irtGrantFolder\(\)/.test(src),
     'g8 a click on the grant reaches irtGrantFolder');
  // ⚠⚠ WIRED TO THE RIGHT ROOT. v14.76 queried elBody while the config panel renders into
  // `.gpts-cfg` (elCfg), so the lookup returned null, `if(irtG)` swallowed it, and the button did
  // NOTHING with no error. Every other control in that block uses elCfg. Assert the ROOT, not just
  // the listener — "it is wired" was true and useless.
  // ⚠⚠ (v14.78) DELEGATED, NOT BOUND — and this assertion replaces one that was WRONG TWICE.
  // v14.76 bound to elBody (null lookup, silent). v14.77 bound to elCfg and STILL did nothing: the
  // panel re-renders, and the node the listener was attached to is replaced. Asserting "it is bound
  // to the right container" was asserting the wrong property entirely — the requirement is that a
  // click ANYWHERE on that class reaches the handler, forever.
  ok(/document\.addEventListener\('click'[\s\S]{0,240}gpts-irt-grant/.test(src),
     'g8b the grant is DELEGATED on document, so a re-render cannot detach it');
  ok(/closest\('\.gpts-irt-grant'\)/.test(src),
     'g8c ...matching by closest(), so a click on the label inside the span still counts');
  // ⚠ USER ACTIVATION: requestPermission must run INSIDE the gesture. Reading the handle from
  // IndexedDB first spends the activation and Chrome rejects — the v14.53 lesson in a new costume.
  // ⚠ EXECUTED. The first version compared string POSITIONS in the source, so disabling the cached
  // branch with `if(false)` left it green — it was testing the order of two words, not behaviour.
  {
    let kvCalls=0;
    global.repoKvGet=(k,cb)=>{ kvCalls++; cb(null); };
    global.IRT_DIR_H={ requestPermission:(o)=>{ asked=o&&o.mode; return sync('granted'); } };
    const before=exported;
    irtGrantFolder();
    ok(kvCalls===0,
       'g10 with a cached handle the permission is asked SYNCHRONOUSLY — IndexedDB is never touched, so the click keeps its user activation', kvCalls);
    ok(asked==='readwrite' && exported===before+1, 'g10b ...and the granted path still writes the file');
    global.IRT_DIR_H=null;
  }
  ok(/var IRT_DIR_H=null;[\s\S]{0,200}repoKvGet\('irtDir'/.test(src),
     'g11 ...and the handle is cached at boot, not fetched when the button is pressed');
  ok(!/setInterval[\s\S]{0,200}irtGrantFolder/.test(src),
     'g9 ...and is never called from a timer, where it would reject and be swallowed (the v14.53 lesson)');
}

console.log('test_irt_export: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);


