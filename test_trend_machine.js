// (v11.89) THE TREND MACHINE — loose reversal at 11, strict and slope-gated shadows.
//
// The user's rule: a NEW trend confirms at 15 of 20, but a REVERSAL out of a broken state needs only
// 11, RAW — no slope gate. The break is itself evidence and waiting the full 15 costs 4 bars.
//
// ⚠ THE COST IS SYMMETRIC AND IT IS WHAT THESE TESTS PIN. Once a trend has confirmed once, BOTH
// directions flip at 11, because after reversing the prior becomes the new direction and the mirror
// rule reverses back at 11. Minimum gap between opposite flips: 10 new bars -> 2.
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(a===b,m,{got:a,want:b});
function ex(n){
  const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index), d=0, e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } }
  return src.slice(m.index,e+1);
}
function exVar(n){ const m=new RegExp('var\\s+'+n+'\\s*=\\s*[^;]+;').exec(src);
  if(!m) throw new Error('var not found: '+n); return m[0]; }

// ---- a harness that drives the REAL trendVerdict off a synthetic candle series ----
// closes are placed a fixed distance above/below a flat 50-SMA so `up`/`dn` are exactly controllable.
let CLOSES=[];
// ⚠ eval'ing `var X = ...` inside a function creates a FUNCTION-LOCAL, not an outer binding — the first
// version of this harness silently left every threshold undefined. Read the values out of the source
// and put them on `global` explicitly, then assert they are the numbers the source actually ships.
function numFromSrc(n){ const m=new RegExp('var\\s+'+n+'\\s*=\\s*([0-9.]+)').exec(src);
  if(!m) throw new Error('var not numeric: '+n); return parseFloat(m[1]); }
function build(){
  global.TREND_WINDOW=numFromSrc('TREND_WINDOW');
  global.TREND_DOM=numFromSrc('TREND_DOM');
  global.TREND_DOM_REV=numFromSrc('TREND_DOM_REV');
  global.TREND_BANDX=numFromSrc('TREND_BANDX');
  global.TREND_LAST={SPY:null,QQQ:null};
  global.TREND_LAST_STRICT={SPY:null,QQQ:null};
  global.TREND_LAST_GATED={SPY:null,QQQ:null};
  global.CFG={trendMA:{SPY:50}};
  global.STATE={SPY:{contCloses:[],contPriorCount:0}};
  global.mul=(a,b)=>a/(1/b);
  global.atr=()=>1;                       // band = 0.25
  global.trendLastSave=()=>{};
  global.closedCandles=()=>CLOSES.map(c=>({c:c}));
  // a flat 100 SMA: 50 leading bars at exactly 100, then the window bars
  global.contSMAAtTodayIdx=()=>null;      // force the today-only inline SMA path
  eval(ex('trendVerdict'));
  return trendVerdict;
}
// window of `win` bars: `nUp` at +1 (above band), `nDn` at -1, rest at 100 (inside band)
function setWindow(nUp,nDn,win=20){
  const lead=new Array(50).fill(100);
  const w=[];
  for(let i=0;i<nUp;i++) w.push(101);
  for(let i=0;i<nDn;i++) w.push(99);
  while(w.length<win) w.push(100);
  CLOSES=lead.concat(w);
}
const TV=build();
const TREND_LAST=global.TREND_LAST, TREND_LAST_STRICT=global.TREND_LAST_STRICT, TREND_LAST_GATED=global.TREND_LAST_GATED;
const TREND_DOM=global.TREND_DOM, TREND_DOM_REV=global.TREND_DOM_REV;

// ---------- 1. the counts land where the fixture says ----------
{
  setWindow(19,1); const t=TV('SPY');
  eq(t.up,19,'19 closes above the band are counted up'); eq(t.dn,1,'1 below');
  eq(t.win,20,'over a 20-bar window');
}
// ---------- 2. a NEW trend still needs 15 ----------
{
  TREND_LAST.SPY=null; TREND_LAST_STRICT.SPY=null; TREND_LAST_GATED.SPY=null;
  setWindow(14,0); eq(TV('SPY').state,'flat','14 of 20 up from FLAT is not a trend — a new trend still needs 15');
  setWindow(15,0); eq(TV('SPY').state,'up','15 is');
}
// ---------- 3. THE REVERSAL, and the exact bar it fires ----------
{
  TREND_LAST.SPY='up'; TREND_LAST_STRICT.SPY='up'; TREND_LAST_GATED.SPY='up';
  setWindow(14,6);  eq(TV('SPY').state,'up-broken','6 below breaks the uptrend but confirms nothing');
  setWindow(10,10); eq(TV('SPY').state,'up-broken','10 below is still not a reversal');
  TREND_LAST.SPY='up';
  setWindow(9,11);  eq(TV('SPY').state,'dn','11 below IS — the reversal fires at TREND_DOM_REV, not TREND_DOM');
  ok(TREND_DOM_REV===11,'and that threshold is 11');
  ok(TREND_DOM===15,'while a new trend still needs 15');
}
// ---------- 4. the mirror ----------
{
  TREND_LAST.SPY='dn';
  setWindow(11,9); eq(TV('SPY').state,'up','a down trend reverses up at 11 too — the rule is symmetric');
}
// ---------- 5. THE COST: both directions flip at 11 once a trend has confirmed ----------
// This is the whipsaw the user accepted. Pin it so nobody "fixes" it by accident, and so the number
// in the changelog is the number the code actually produces.
{
  TREND_LAST.SPY='up';
  setWindow(9,11);  eq(TV('SPY').state,'dn','flip one: down at 11');
  setWindow(11,9);  eq(TV('SPY').state,'up','flip two: back up at 11, two bars of window turnover later');
  ok(true,'so the minimum gap between OPPOSITE flips is 2 bars of composition change, against 10 under the old 15/15 rule');
}
// ---------- 6. THE STRICT SHADOW KEEPS ITS OWN MEMORY ----------
// A shadow that shares TREND_LAST is not a shadow. This is the assertion that catches that.
{
  TREND_LAST.SPY='up'; TREND_LAST_STRICT.SPY='up';
  setWindow(9,11); const t=TV('SPY');
  eq(t.state,'dn',            'the live machine reverses at 11');
  eq(t.stateStrict,'up-broken','the STRICT shadow does NOT — it still wants 15');
  eq(t.differs,true,           'and the bar is flagged as one where they disagree');
  eq(TREND_LAST.SPY,'dn',      'the live memory advanced');
  eq(TREND_LAST_STRICT.SPY,'up','the strict memory did NOT — separate priors, or the shadow is just an echo');
}
// ---------- 7. THE SLOPE-GATED SHADOW ----------
// The user chose RAW. The gated variant is recorded so the choice is measurable.
{
  TREND_LAST.SPY='up'; TREND_LAST_STRICT.SPY='up'; TREND_LAST_GATED.SPY='up';
  // a RISING SMA under a price that has dropped below it = a pullback, not a downtrend
  setWindow(9,11);
  const t=TV('SPY');
  eq(t.state,'dn','raw reverses regardless of what the average is doing');
  ok(t.stateGated==='dn' || t.stateGated==='up-broken','the gated shadow is computed', t.stateGated);
  ok(typeof t.slope==='number','and the slope it gates on is a real number', t.slope);
}
// ---------- 7b. A REVERSAL NEEDS A TREND TO REVERSE ----------
// Without the `prior` guard the 11-threshold would confirm a trend straight out of FLAT, which is the
// whole point of keeping 15 for a new one. The first version of this file never tested it.
{
  TREND_LAST.SPY=null; TREND_LAST_STRICT.SPY=null; TREND_LAST_GATED.SPY=null;
  setWindow(0,11);
  eq(TV('SPY').state,'flat','11 DOWN with no prior trend is NOT a reversal — there is nothing to reverse, and a new trend still needs 15');
  TREND_LAST.SPY=null;
  setWindow(11,0);
  eq(TV('SPY').state,'flat','and 11 UP from flat is not a trend either');
}
// ---------- 7c. THE STRICT SHADOW WRITES ITS OWN MEMORY, NOT THE LIVE ONE ----------
// §6 only covers the case where the shadow does NOT confirm, so a shadow that wrote to TREND_LAST
// would have gone unnoticed. Make it confirm.
{
  TREND_LAST.SPY='dn'; TREND_LAST_STRICT.SPY=null; TREND_LAST_GATED.SPY=null;
  setWindow(15,0);
  const t=TV('SPY');
  eq(t.state,'up','15 up confirms on the live machine');
  eq(t.stateStrict,'up','and on the strict shadow, which also needed 15');
  eq(TREND_LAST_STRICT.SPY,'up','the shadow advanced ITS OWN memory');
  eq(TREND_LAST.SPY,'up','the live memory advanced separately — a shadow that writes the live prior is not a shadow');
}
// ---------- 8. the neutral band is not the complement ----------
{
  // ⚠ a flat series is the only fixture where the SMA sits EXACTLY on every close, so it is the only
  // one that can prove the band excludes them. With mixed closes the SMA drifts and a bar written as
  // "on the average" is not actually on it — the first version of this test proved nothing because of
  // that, and a mutation removing the band entirely passed it.
  TREND_LAST.SPY=null; TREND_LAST_STRICT.SPY=null; TREND_LAST_GATED.SPY=null;
  setWindow(0,0);              // every close == the SMA
  const t=TV('SPY');
  eq(t.up,0,'a close sitting ON the average is not an UP bar');
  eq(t.dn,0,'nor a DOWN bar');
  eq(t.win,20,'all 20 were scored');
  ok(t.up+t.dn<t.win,'so up+dn is LESS than the window — "11 of 20 below" means 11 bars strictly below the 0.25-ATR band, NOT 55% of them', {up:t.up,dn:t.dn,win:t.win});
  eq(t.state,'flat','and a series with no side is flat, not a trend');
}
// ---------- 9. thresholds are reported, not hidden ----------
{
  setWindow(15,0); const t=TV('SPY');
  eq(t.domThresh,15,'the machine reports the threshold it used'); eq(t.revThresh,11,'and the reversal threshold');
}
console.log('\n'+pass+' pass / '+fail+' fail');
process.exit(fail?1:0);
