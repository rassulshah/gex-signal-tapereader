// (v13.9) THE ROLL LATCH — the doctrine counting rule (1 noise / 2 signal / 3 confirmation),
// applied to rollScan sightings per closed bar, with persistence while the destination HOLDS.
//
// Why this exists: rollScan re-derives from a sliding velocity window, so a roll was visible only
// while mass was mid-flight. On 2026-08-25 the 10:00–10:23 migration into 7665 built the shelf price
// later bounced from, and the face showed nothing by the time it mattered. The latch is the fix, and
// this test RUNS it — feeds sighting sequences bar by bar and asserts what the face would draw.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// the CONSTANTS come out of the source, so a silent rename or retune fails here, not in production
function exVar(n){ const m=src.match(new RegExp('var '+n+'\\s*=\\s*([0-9.]+)')); if(!m) throw new Error('const '+n+' not found'); return parseFloat(m[1]); }

global.window={__gptsDebug:{}};
global.localStorage={_s:{},getItem(k){return this._s[k]??null},setItem(k,v){this._s[k]=String(v)},removeItem(k){delete this._s[k]}};
global.STATE={SPY:{lastClosedB:0}};
global.inReplay=()=>false;
// (v14.55) the recorder guards call recorderBlind() now — ONE gate covering replay AND the
// close-of-session book, so a latched book can never be written as live. Stub both: the guard
// deliberately fails toward recording, so an unstubbed recorderBlind makes this test pass for the
// wrong reason.
global.recorderBlind=()=>(global.inReplay()||false);
global.velOk=()=>true;
global.ctTodayStr=()=>'2026-08-25';
global.ROLL_MIN_ABS=exVar('ROLL_MIN_ABS'); global.ROLL_MAX_DIST=exVar('ROLL_MAX_DIST'); global.ROLL_MIN_RATIO=exVar('ROLL_MIN_RATIO');
global.ROLL_SIG_N=exVar('ROLL_SIG_N'); global.ROLL_CONF_N=exVar('ROLL_CONF_N');
global.ROLL_MISS_DROP=exVar('ROLL_MISS_DROP'); global.ROLL_GONE_BARS=exVar('ROLL_GONE_BARS');
global.ROLL_HOLD_FRAC=exVar('ROLL_HOLD_FRAC');
global.ROLL_LATCH_KEY='gpts_rolllatch_v1';
global.ROLL_LATCH={ day:null, per:{} };

// a controllable book: VEL[k] = {d15, cur}; velAt returns the rollScan/latch shape
let VEL={};
global.velAt=(k)=>{ const e=VEL[k]; return e?{ v:{ k:k, d15:e.d15, cur:e.cur }, stale:false }:null; };
global.tradeNodes=()=>Object.keys(VEL).map(k=>({k:parseFloat(k)}));

eval(ex('rollScan')); eval(ex('rollLatchKey')); eval(ex('rollLatchSave')); eval(ex('rollLatchLoad'));
eval(ex('rollLatchTick')); eval(ex('rollLatched'));

// convenience: advance one closed bar with a given book and tick the latch
let BAR=0;
function bar(book){ VEL=book; BAR++; STATE.SPY.lastClosedB=BAR; rollLatchTick('SPY'); }
const ROLLING={ 7690:{d15:-2000000,cur:30e6}, 7665:{d15:1500000,cur:80e6} };   // 7690 -> 7665, dn
const QUIET  ={ 7690:{d15:0,cur:28e6},        7665:{d15:0,cur:80e6} };
const BLED   ={ 7690:{d15:0,cur:28e6},        7665:{d15:0,cur:30e6} };         // dest gave back (30/80 < 60%)

// ---- 1 sighting = noise: nothing is drawn ----
bar(ROLLING);
eq(rollLatched('SPY').length, 0, 'one sighting is noise and is never drawn');

// ---- 2 consecutive = SIGNAL: drawn, live, not yet confirmed ----
bar(ROLLING);
{ const L=rollLatched('SPY');
  eq(L.length, 1, 'two consecutive sightings draw');
  ok(L[0].from===7690 && L[0].to===7665 && L[0].dir==='dn', 'from/to/dir carried', L[0]);
  ok(L[0].live===true && L[0].conf===false, 'SIGNAL: live, unconfirmed', L[0]); }

// ---- same bar, second tick: the count must NOT inflate (bar gate) ----
rollLatchTick('SPY');
{ const L=rollLatched('SPY'); eq(L[0].count, 2, 'a second tick on the same bar does not double-count'); }

// ---- 3rd sighting = CONFIRMED, and the destination reference is taken ----
bar(ROLLING);
{ const L=rollLatched('SPY'); ok(L[0].conf===true && L[0].count===3, 'three sightings confirm', L[0]); }

// ---- the window slides past (quiet book) but the destination HOLDS: the roll STAYS, aged, not live ----
bar(QUIET); bar(QUIET); bar(QUIET); bar(QUIET);
{ const L=rollLatched('SPY');
  eq(L.length, 1, 'a confirmed roll outlives the sliding window while the destination holds');
  ok(L[0].live===false && L[0].conf===true && !L[0].gone, 'STUCK: latched, no longer in flight', L[0]); }

// ---- the destination bleeds below the hold fraction: GAVE BACK, then retires ----
bar(BLED);
{ const L=rollLatched('SPY'); ok(L.length===1 && L[0].gone===true, 'destination gave back -> flagged, still shown', L[0]); }
bar(BLED); bar(BLED); bar(BLED);
eq(rollLatched('SPY').length, 0, 'GAVE BACK retires after ROLL_GONE_BARS');
eq(Object.keys(ROLL_LATCH.per.SPY.m).length, 0, 'the entry is actually deleted, not hidden');

// ---- a single flicker is forgotten without ever being drawn ----
bar(ROLLING);                      // seen once
bar(QUIET); bar(QUIET);            // absent ROLL_MISS_DROP bars
eq(rollLatched('SPY').length, 0, 'a flicker never draws');
eq(Object.keys(ROLL_LATCH.per.SPY.m).length, 0, 'and is deleted from the register');

// ---- persistence: today's latch survives a reload; yesterday's does not ----
bar(ROLLING); bar(ROLLING); bar(ROLLING);
{ const saved=JSON.parse(localStorage.getItem('gpts_rolllatch_v1'));
  ok(saved && saved.day==='2026-08-25' && saved.per.SPY, 'the latch is saved per bar');
  ROLL_LATCH={ day:null, per:{} };
  rollLatchLoad();
  eq(rollLatched('SPY').length, 1, 'a reload rehydrates today\'s latch');
  ROLL_LATCH={ day:null, per:{} };
  const stale=Object.assign({}, saved, { day:'2026-08-24' });
  localStorage.setItem('gpts_rolllatch_v1', JSON.stringify(stale));
  rollLatchLoad();
  eq(rollLatched('SPY').length, 0, 'yesterday\'s latch is refused'); }

// ---- a replay must never write the latch ----
{ global.inReplay=()=>true; global.recorderBlind=()=>true;
  ROLL_LATCH={ day:null, per:{} }; BAR++; STATE.SPY.lastClosedB=BAR; VEL=ROLLING;
  rollLatchTick('SPY');
  eq(Object.keys(ROLL_LATCH.per).length, 0, 'replay writes nothing');
  global.inReplay=()=>false; global.recorderBlind=()=>false; }

console.log(pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
