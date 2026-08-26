// (v14.34) GARMA PHASE 1 — day classification + Trinity conviction + gatekeeper target-block.
// Source: garma/claude_package (42 evidence-weighted rules, 11 videos). Academy stays
// authoritative; these are observed-practice rules adopted with no conflicts found.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

// ---- dayTypeOf, executed against shaped stubs ----
global.eventTagLabel=()=>null;
global.isOpexDay=()=>false;
let TV={state:'na'};
global.trendVerdict=()=>TV;
eval(ex('dayTypeOf'));
const B=(up,dn)=>({ok:true, em:20, upExc:up, dnExc:dn});
ok(dayTypeOf('SPY', B(2,3)).t==='RANGE',        'quiet two-sided day = RANGE');
ok(dayTypeOf('SPY', B(9,9)).t==='WHIPSAW',      'both sides ran >35% of EM = WHIPSAW (fade edges)');
TV={state:'up'};
ok(dayTypeOf('SPY', B(10,2)).t==='TREND UP',    'one-sided EM use + SMA side = TREND UP');
TV={state:'dn'};
ok(dayTypeOf('SPY', B(2,10)).t==='TREND DOWN',  '...and the mirror');
global.eventTagLabel=()=>'FOMC';
ok(dayTypeOf('SPY', B(2,3)).t==='EVENT',        'an event tag outranks everything (Garma r41: downgraded confidence)');
global.eventTagLabel=()=>null; global.isOpexDay=()=>true;
ok(dayTypeOf('SPY', B(2,3)).t==='OPEX',         'OPEX flagged as its own day type');
global.isOpexDay=()=>false;

// ---- trinityRead, executed ----
let TAPES={ SPXW:{king:7650}, SPY:{king:760}, QQQ:{king:700} };
global.tapeMap=(s)=>TAPES[s]||null;
global.ifLadder=()=>({dispScale:1.002});
global.emBand=()=>({ok:true, now:7690});
global.STATE={ SPY:{price:765}, QQQ:{price:710} };
global.LASTFEED={};
eval(ex('trinityRead'));
let T=trinityRead();
ok(T.of===3 && T.n===3 && T.side==='below' && T.dissent.length===0, 'all three Kings below their spots = 3-of-3', T);
TAPES.QQQ={king:720};
T=trinityRead();
ok(T.n===2 && T.dissent.join()==='QQQ', 'QQQ crown above its spot = 2-of-3 with QQQ NAMED as the dissent', T);
TAPES.SPY={king:770};
T=trinityRead();
ok(T.of===3 && T.n<=2, 'a split field never reports 3-of-3', T);

// ---- the read carries the lead clause and the gatekeeper block (token-level) ----
ok(/GARMA PHASE 1/.test(src), 'the phase is documented at the definition site');
ok(/Trinity '\+frTri\.n\+'-of-'\+frTri\.of/.test(src), 'the read leads with the Trinity count');
ok(/WAIT per doctrine/.test(src), '...and can say WAIT (Garma r12)');
ok(/target BLOCKED by uncleared gatekeeper/.test(src), 'a braked path through a stalling GK names the block (Garma r5)');
ok(/out\.day=dt\.t/.test(src) && /out\.trinity=tr\.n/.test(src), 'day type + Trinity ride the levelstate record for nightly conditioning');

// ---------- (v14.35) item 1: session levels, executed ----------
(function(){
  global.mul=(a,b)=>a*b;
  global.ctTodayStr=()=>'2026-08-26';
  global.ctNowSecOfDay=()=>40000;                       // 10:36 CT — IB long since set
  const bar=(day,so,h,l,c)=>({day:day,so:so,h:h,l:l,c:c});
  global.closedCandles=()=>[
    bar('2026-08-25',30600,765.0,764.0,764.5), bar('2026-08-25',52000,766.5,763.2,766.0), // prior day
    bar('2026-08-26',30600,765.8,764.9,765.5), bar('2026-08-26',31500,766.2,765.1,765.9), // IB window
    bar('2026-08-26',33000,767.0,765.8,766.8) ];                                          // after IB
  eval(src.match(/var IB_MIN_S=\d+/)[0]+';');
  eval(src.match(/var SESS_CONFL_PTS=[\d.]+/)[0]+';');
  eval(ex('sessionLevels')); eval(ex('sessConfluence'));
  const SL=sessionLevels('SPY', 10);                    // ×10 into the chart frame
  ok(SL.ibSet===true, 's1 the IB is SET after 30 minutes (Garma waits for it)');
  ok(SL.ibH===7662 && SL.ibL===7649, 's2 IB high/low come from the FIRST 30 MINUTES only (post-IB bars excluded)', [SL.ibH,SL.ibL]);
  ok(SL.pdh===7665 && SL.pdl===7632 && SL.pdc===7660, 's3 prior-day H/L/C from the prior session bars', [SL.pdh,SL.pdl,SL.pdc]);
  const cf=sessConfluence(SL, 7661);
  ok(cf && /IB high/.test(cf.name), 's4 a level within 2 pts of the IB high names the confluence (Garma r22)', cf);
  ok(sessConfluence(SL, 7620)==null, 's5 nothing within reach = no confluence claim');
  global.ctNowSecOfDay=()=>31000;                        // 8:36 CT — inside the IB window
  const SL2=sessionLevels('SPY', 10);
  ok(SL2.ibSet===false, 's6 before the 30 minutes are up the IB is NOT set — Garma waits, so do we');
})();

console.log('test_garma_p1: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
