// test_v1594.js — (v15.94) THE CANDLE: TOOK UNDER EACH EXTREME · PFH/PFL · THE LIQUIDITY MAP (SWEPT + TARGET + NEXT DRAW)
//
// Operator, 2026-09-10. (1) The number under each extreme was the leg AFTER it (LOD showed 4h30 = 10:27→close); "the
// difference between the open and 10:27 is not 4h30" → it now shows the TOOK from the open (HOD 1h33, LOD 1h57).
// (2) "the overnight low, prior day low, the prior full low, the weekly low … all of their corresponding highs … indicate
// which levels were swept and targetted." PFH/PFL = the prior FULL Globex session (overnight in + RTH), distinct from
// PDH/PDL (prior RTH). (3) "targeted is the other extremity … a green bar that swept overnight low and then went to
// (prior full high) the PFHI, which is the target." The second extreme's tagged level = the TARGET (cyan ring), the
// nearest level beyond it = the NEXT DRAW (grey ring); swept levels keep their tick — all in ONE anti-overlap stack.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

ok(/@version\s+15\.9[4-9]/.test(src) && /var GPTS_VERSION='15\.9[4-9]';/.test(src), '0a v15.94 or later in both spots');

// ---------- 1. candleDraws — the target and the next draw ----------
{
  global.FUTMODE={fam:'ES'}; global.irtRatio=()=>({r:1});
  global.levelTier=(n)=>{const b=String(n).replace(/[-+]$/,'');return ({PDH:1,PDL:1,ONH:1,ONL:1,VAH:1,VAL:1,POC:1,PWH:1,PWL:1,WPOC:1,AHI:1,ALO:1,LHI:1,LLO:1,PFH:1,PFL:1}[b])||4;};
  let LV=[]; global.sweepLevelsToday=()=>LV;
  let SWEPT=[]; global.sweepEventsShown=()=>SWEPT;
  eval(ex('candleDraws'));
  // his green day: LOD first swept ONL/PDL, HOD second tagged PFH; PDH just above is the nearest next draw
  LV=[{name:'ONL',px:7642,low:true},{name:'PDL',px:7646,low:true},{name:'PFH',px:7686,low:false},{name:'PWH',px:7694,low:false},{name:'PDH',px:7690,low:false}];
  SWEPT=[{level:'ONL'},{level:'PDL'}];
  const D={ok:true,hod:7686,lod:7635,second:'HOD',first:'LOD'};
  const r=candleDraws('SPY',D,null);
  ok(r.target && r.target.name==='PFH' && r.target.px===7686, '1a the target is PFH — the un-swept level the HOD (the second extreme) tagged', r.target);
  ok(r.nextDraw && r.nextDraw.name==='PDH' && r.nextDraw.px===7690, '1b the next draw is the NEAREST level beyond the HOD (PDH 7690, nearer than PWH 7694)', r.nextDraw);
  // a swept level is never the target: mark PFH swept too → the next un-swept tagged level (none within tol) → no target
  SWEPT=[{level:'ONL'},{level:'PDL'},{level:'PFH'}];
  const r2=candleDraws('SPY',D,null);
  ok(!r2.target, '1c a level that was swept is excluded from the target (PFH swept → no target within tol)', r2.target);
  // nothing within tol of the extreme → no target, but still a next draw beyond
  SWEPT=[{level:'ONL'},{level:'PDL'}];
  const D2={ok:true,hod:7676,lod:7635,second:'HOD',first:'LOD'};   // HOD 7676, PFH 7686 is 10 pts above (beyond tol) → not tagged
  const r3=candleDraws('SPY',D2,null);
  ok(!r3.target && r3.nextDraw && r3.nextDraw.name==='PFH', '1d HOD 10 pts short of PFH → no target; PFH is the nearest next draw', [r3.target, r3.nextDraw]);
  // a down day: LOD second, target below
  LV=[{name:'ONH',px:7700,low:false},{name:'PFL',px:7640,low:true},{name:'PDL',px:7632,low:true}];
  SWEPT=[{level:'ONH'}];
  const D3={ok:true,hod:7705,lod:7640,second:'LOD',first:'HOD'};
  const r4=candleDraws('SPY',D3,null);
  ok(r4.target && r4.target.name==='PFL' && r4.nextDraw && r4.nextDraw.name==='PDL', '1e a down day: LOD tagged PFL (target), PDL below is the next draw', [r4.target, r4.nextDraw]);
  ok(candleDraws('SPY',{ok:false},null).target===null && candleDraws('SPY',null,null).nextDraw===null, '1f no D / not ok → empty, no throw');
}

// ---------- 2. priorFullHL — the prior full Globex session ----------
{
  // stub futSessionBars (prior RTH day) and overnightHL (that day's night)
  let CALL=[];
  const rth60=(hi,lo)=>{ const a=[]; for(let i=0;i<60;i++) a.push([i,7655,(i===3?hi:7665),(i===7?lo:7652),7658]); return a; };
  global.futSessionBars=(off,day)=>{ CALL.push([off,day]); if(off===1) return { rth:rth60(7670,7648), on:[], key:'2026-9-9' }; return null; };
  global.overnightHL=(k)=>{ return (k==='2026-9-9')?{ onh:7675, onl:7642, full:true }:null; };
  eval(ex('priorFullHL'));
  const P=priorFullHL('2026-9-10');
  ok(P && P.pfh===7675 && P.pfl===7642 && P.onIncluded===true, '2a PFH/PFL = max(prior RTH high, night high) / min(prior RTH low, night low) = 7675 / 7642', P);
  ok(CALL.some(c=>c[0]===1 && c[1]==='2026-9-10'), '2b it read the prior RTH day for the shown day, then that day\'s own overnight (via its key)');
  // no overnight → the RTH extremes alone
  global.overnightHL=()=>null;
  const P2=priorFullHL('2026-9-10');
  ok(P2 && P2.pfh===7670 && P2.pfl===7648 && P2.onIncluded===false, '2c night missing → the prior RTH high/low alone, said via onIncluded=false', P2);
  // too few prior bars → null
  global.futSessionBars=(off)=>(off===1?{ rth:[[0,1,1,1,1]], on:[], key:'x' }:null);   // < 60 bars
  ok(priorFullHL('2026-9-10')===null, '2d a prior day under 60 bars is not a session → null');
}

// ---------- 3. the source is wired ----------
{
  const slt=ex('sweepLevelsToday');
  ok(/var PF=priorFullHL\(dayStr\); if\(PF\)\{ add\('PFL', PF\.pfl, true, 0\); add\('PFH', PF\.pfh, false, 0\); \}/.test(slt), '3a sweepLevelsToday adds PFL / PFH');
  ok(/PFH:1, PFL:1/.test(src), '3b LEVEL_TIER ranks PFH / PFL tier 1 (drawn on the candle)');
  ok(/byDay\[k\]\.key=k;/.test(ex('futSessionBars')), '3c futSessionBars returns the resolved day key (priorFullHL anchors on it)');
  const cdl=ex('dayCandleSvg');
  ok(/if\(!\(levelTier\(m\.level\)===1 \|\| b1==='CW0' \|\| b1==='PW0'\)\) return false;/.test(cdl), '3d the candle draws tier 1 PLUS CW0 / PW0 (he asked for the 0DTE walls)');
  ok(/var tookFirst=\(typeof D\.took==='number'\)\?D\.took:null;/.test(cdl) && /var afterHod = firstUp \? tookFirst : tookSecond;/.test(cdl) && /var afterLod = firstUp \? tookSecond : tookFirst;/.test(cdl), '3e the number under each extreme is its TOOK from the open (first = D.took, second = D.took + D.gap)');
  // the unified stack: swept + target + draw, one rows[] through the STEP anti-overlap, rings for target/draw
  ok(/var items=SW\.map\(function\(m\)\{ return \{ kind:'swept'/.test(cdl) && /var DR=candleDraws\(sym, D, _dd\);/.test(cdl) && /kind:'target'[^}]*ring:true/.test(cdl) && /kind:'draw'[^}]*ring:true/.test(cdl), '3f the candle builds swept + target + next-draw into one items[] and asks candleDraws for them');
  ok(/items\.forEach\(function\(it\)\{ var yp=y\(it\.px\); if\(!\(yp>=y\(H\)-2 && yp<=y\(L\)\+2\)\) return; if\(seen\[it\.lbl\]\) return;/.test(cdl) && /rows\[si\]\.yl<rows\[si-1\]\.yl\+STEP/.test(cdl), '3g every item (swept + target + draw) goes through the SAME STEP anti-overlap stacking — nothing overlaps');
  ok(/if\(it\.ring\) h\+='<circle/.test(cdl) && /SWEPT \/ TARGET/.test(cdl), '3h target / next-draw draw as rings; the header reads SWEPT / TARGET');
  ok(/a level\s*\n?\s*\/\/ already swept is never re-drawn as a target/.test(cdl) || /candleDraws excludes them/.test(cdl), '3i the comment records the dedupe rule (swept wins over target/draw)');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
