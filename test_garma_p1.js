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

// ---------- (v14.36) item 1: session levels, executed against the REAL candle shapes ----------
// (the v14.35 stubs modelled a `day` field the live candles never carry — the ticks never drew;
// operator-caught. IB reads the today-only closed store; prior-day reads the raw fiber window.)
(function(){
  global.mul=(a,b)=>a*b;
  global.ctTodayStr=()=>'2026-08-26';
  global.ctNowSecOfDay=()=>40000;
  global.naiveDayStr=(t)=>new Date(t*1000).toISOString().slice(0,10);
  global.naiveSecOfDay=(t)=>{ const d=new Date(t*1000); return d.getUTCHours()*3600+d.getUTCMinutes()*60; };
  // closed store: TODAY only, so+h/l/c, NO day field (the real shape)
  global.closedCandles=()=>[
    {so:30600,h:765.8,l:764.9,c:765.5}, {so:31500,h:766.2,l:765.1,c:765.9},   // IB window
    {so:33000,h:767.0,l:765.8,c:766.8} ];                                     // after IB
  // raw fiber window: spans days, carries .time (epoch s, UTC used as naive here)
  const T=(day,hh,mm)=>Math.floor(Date.parse(day+'T'+String(hh).padStart(2,'0')+':'+String(mm).padStart(2,'0')+':00Z')/1000);
  global.futRawCandles=()=>[
    {time:T('2026-08-25',9,0),  open:764.2,high:765.0,low:764.0,close:764.5},
    {time:T('2026-08-25',14,0), open:765.5,high:766.5,low:763.2,close:766.0},   // prior-day extremes+close
    {time:T('2026-08-25',7,0),  open:760.0,high:770.0,low:750.0,close:760.0},   // PRE-RTH: must be ignored
    {time:T('2026-08-26',9,0),  open:765.0,high:765.8,low:764.9,close:765.5} ];
  eval(src.match(/var IB_MIN_S=\d+/)[0]+';');
  eval(src.match(/var SESS_CONFL_PTS=[\d.]+/)[0]+';');
  eval(ex('sessionLevels')); eval(ex('sessConfluence'));
  const SL=sessionLevels('SPY', 10);
  ok(SL.ibSet===true, 's1 the IB is SET after 30 minutes (Garma waits for it)');
  ok(SL.ibH===7662 && SL.ibL===7649, 's2 IB from the first-30-min bars of the today-only store', [SL.ibH,SL.ibL]);
  ok(SL.pdh===7665 && SL.pdl===7632 && SL.pdc===7660, 's3 prior-day H/L/C from the raw window, RTH bars only (the 770/750 pre-RTH bar ignored)', [SL.pdh,SL.pdl,SL.pdc]);
  const cf=sessConfluence(SL, 7661);
  ok(cf && /IB high/.test(cf.name), 's4 a level within 2 pts of the IB high names the confluence (Garma r22)', cf);
  ok(sessConfluence(SL, 7620)==null, 's5 nothing within reach = no confluence claim');
  global.ctNowSecOfDay=()=>31000;
  const SL2=sessionLevels('SPY', 10);
  ok(SL2.ibSet===false, 's6 before the 30 minutes are up the IB is NOT set — Garma waits, so do we');
})();

console.log('test_garma_p1: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
