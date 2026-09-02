// ============================================================================================
// test_replay_scale.js — (v15.40) THE REPLAYED LADDER WAS DRAWN IN THE WRONG PRICE SPACE.
//
// Operator, 2026-09-02: "i rewinded and it looks like you are still not showing the state during
// the replay. as you can see there are no nodes etc. you still haven't fixed this.. you are suppose
// to capture the state so i can rewind and replay the day."
//
// ⚠⚠ THE CAPTURE WAS NEVER THE PROBLEM. Measured on his panel, replaying 2026-09-01 14:21, the
// recorded frame held a PERFECT SPXW book — 36 strikes, seven clearing 20% of King, ALL SEVEN
// inside the band: 7630 -100, 7625 +77, 7610 +52, 7635 -48, 7620 -44, 7615 +39, 7650 +21.
// `replayLadder` returned dispScale 0.099775 === undScale 0.099775 — the CASH scale in both slots —
// so SPXW 7630 was plotted at 761.28 on a ladder framed 7615..7680, ~6,880 points below it.
// inFrame() refused every node and the ladder drew ZERO strike rows.
// ⚠ A READ FAULT AND A WRITE FAULT LOOK IDENTICAL FROM THE FACE. He reported it three times as a
// capture failure, and he was right about the symptom every time.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }

// ---- harness: his real frame, and a futures mode we control ---------------------------------
let FRAME={ px:761.6649, xm:{ SPXW:{ px:7634.0 } } };     // SPY 761.66, SPX 7634 -> basis 0.099773
let FUT={ on:true, r:10.036236692680397 };
global.replayFrame=()=>FRAME;
global.dispIsFut=()=>FUT.on;
global.dispR=()=>FUT.r;
eval(ex('replayLadder'));

// ===== 1 · HIS CASE, EXECUTED ==================================================================
let L=replayLadder('SPY');
ok(!!L, 'r1 EXECUTED: the frame yields a ladder');
ok(Math.abs(L.undScale-0.099773)<1e-5,
   'r1b the UNDERLYING scale is the frame’s own two prices, unchanged', L.undScale);
// ⚠⚠ THE WHOLE BUG IN ONE ASSERTION.
ok(L.dispScale!==L.undScale,
   'r2 THE CHART SCALE IS NOT THE CASH SCALE — they were identical, and that equality WAS the bug',
   {disp:L.dispScale, und:L.undScale});
ok(Math.abs(L.dispScale-1.001355)<2e-3,
   'r2b ...it is SPX→ES, about 1.0014, not SPX→SPY at 0.0998', L.dispScale);

// ⚠ EXECUTED IN PRICES, because a ratio is abstract and a misplaced node is not.
const K=7630, framedLo=7615, framedHi=7680;
const drawn=K*L.dispScale, wrong=K*L.undScale;
ok(drawn>=framedLo && drawn<=framedHi,
   'r3 EXECUTED: the King now lands INSIDE the drawn frame', {drawn:+drawn.toFixed(2), framedLo, framedHi});
ok(!(wrong>=framedLo && wrong<=framedHi),
   'r3b ...and the retired scale put it 6,880 points below it', +wrong.toFixed(2));
// every one of the seven nodes his frame actually held
[[7630,-100],[7625,77],[7610,52],[7635,-48],[7620,-44],[7615,39],[7650,21]].forEach(function(n){
  const d=n[0]*L.dispScale;
  ok(d>=framedLo-1 && d<=framedHi+1,
     'r3·'+n[0]+' node '+n[0]+' ('+n[1]+'% of King) is drawable', +d.toFixed(1));
});

// ===== 2 · THE SCALE IS DISCLOSED, NOT SILENT ==================================================
// ⚠ The live branch's own rule, v15.06: "a silent fallback is what made this invisible for four
// builds." The replayed basis uses TODAY's ES/SPY ratio because no frame records an ES print.
ok(/ratio-today/.test(L.scaleSrc), 'r4 the source says the ratio is TODAY’s, applied to a past day', L.scaleSrc);
FUT={on:false, r:1};
L=replayLadder('SPY');
ok(L.dispScale===L.undScale, 'r5 on a CASH chart the two scales are the same — correctly', [L.dispScale,L.undScale]);
ok(/cash/.test(L.scaleSrc), 'r5b ...and it says so', L.scaleSrc);
FUT={on:true, r:10.036236692680397};

// ⚠ a futures chart with NO usable ratio must NOT silently use the cash scale and pretend
FUT={on:true, r:0};
L=replayLadder('SPY');
ok(L.scaleSrc==='replay:cash',
   'r6 futures mode with no ratio falls back to cash AND SAYS SO — never a silent guess', L.scaleSrc);
FUT={on:true, r:1};
L=replayLadder('SPY');
ok(L.scaleSrc==='replay:cash', 'r6b ...and a ratio of exactly 1 is treated the same way', L.scaleSrc);
FUT={on:true, r:10.036236692680397};

// ===== 3 · IT STILL REFUSES TO INVENT LEVELS ===================================================
// ⚠ v15.18's other half was RIGHT and must survive: today's CW0/PW0/FLIP over a past book is a lie
// nothing downstream can detect. The scale is recoverable from the frame; the levels are not.
L=replayLadder('SPY');
ok(Array.isArray(L.rows) && L.rows.length===0,
   'r7 a replayed ladder still returns NO level rows — refuse, never fall through', L.rows);
ok(L.replay===true && L.src==='replay', 'r7b ...and is labelled as replay everywhere');

// ===== 4 · A FRAME THAT CANNOT ANSWER GETS NO LADDER ===========================================
FRAME={ px:761.66, xm:{} };
ok(replayLadder('SPY')===null, 'r8 no SPXW price in the frame -> null, not a guessed scale');
FRAME={ px:0, xm:{ SPXW:{px:7634} } };
ok(replayLadder('SPY')===null, 'r8b no underlying price -> null');
FRAME=null;
ok(replayLadder('SPY')===null, 'r8c no frame -> null');
FRAME={ px:761.6649, xm:{ SPXW:{ px:7634.0 } } };

// ===== 5 · THE TURNING POINTS NAME THEMSELVES ==================================================
// Operator, 2026-09-02: "it doesn't mention HOD and LOD ... is it a Hod turn or an LOD turn."
// ⚠ The old headings said "1ST HOD" until v15.33; the rename to "1ST TP" silently dropped the fact.
ok(/dcol\('1ST TP'\+t1/.test(src), 't1 the 1ST TP heading appends which extreme it was');
ok(/dcol\('2ND TP'\+t2/.test(src), 't1b ...and so does 2ND TP');
ok(/var t1=\(!NOREAD && D\.first\)/.test(src), 't2 it reads D.first, the value the column already had');
// ⚠ AND THE SECOND ONE ONLY WHEN IT HAS ACTUALLY PRINTED. Naming a turn that has not happened is a
// forecast wearing a label — and `D.secondT > D.clock` was three dead clauses for eight builds
// (v15.23), so this condition is written the way that bug taught: secondT != null AND <= clock.
// ⚠ ANCHORED ON THE `t2` ASSIGNMENT, not on the source at large. My first cut grepped the whole
// file for `D.secondT<=D.clock` — a condition that ALSO lives in `hlNodeAt` — so deleting the guard
// from this heading still passed on the other function's copy. Same fault as v15.38's CPE/HGE.
const t2line=(src.match(/var t2=\(!NOREAD[^\n]*/)||[''])[0];
ok(/D\.secondT!=null && D\.secondT<=D\.clock/.test(t2line),
   't3 the SECOND turn is named only once it has printed', t2line.slice(0,110));
ok(/IT HAS NOT PRINTED YET/.test(src), 't3b ...and the hover says so plainly when it has not');
ok(/at '\+hlClock\(D\.firstT\)/.test(src), 't4 the hover gains the CLOCK TIME of the turn');

console.log('test_replay_scale: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
