// (v14.0) THE DAY-PEAK TRACKER and the esTick trim rule — both RUN, not grepped.
//
// The gamma profile draws outline = day peak, fill = now; a wrong peak makes every % on it a lie,
// and the trim rule touches every ES price on the face. So: feed the tracker a day, watch the max;
// reload it; roll the day; replay-guard it. And assert the trim keeps REAL ticks (.50/.75/.25)
// while dropping only the .00 noise.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

// ---- esTick: tick-round, then trim ONLY a trailing .00 ----
eval(ex('esTick'));
eq(esTick(7709.00), '7709',    'a trailing .00 is dropped');
eq(esTick(7706.50), '7706.50', 'a real half-tick keeps its decimals');
eq(esTick(7706.49), '7706.50', 'still tick-rounded to quarters first');
eq(esTick(-22.75),  '-22.75',  'quarter ticks keep their decimals, sign intact');
eq(esTick(-42.001), '-42',     'negative .00 trims too');
eq(esTick(NaN),     null,      'non-numbers stay null');

// ---- the peak tracker ----
global.localStorage={_s:{},getItem(k){return this._s[k]??null},setItem(k,v){this._s[k]=String(v)}};
global.STATE={SPY:{lastClosedB:0}};
global.inReplay=()=>false;
global.velOk=()=>true;
let TODAY_STR='2026-08-25';
global.ctTodayStr=()=>TODAY_STR;
global.VEL={};
global.velAt=(k)=>{ const e=VEL[k]; return e?{v:{k:k,cur:e.cur},stale:false}:null; };
global.recorderLoad=()=>REC;
let REC={days:{}};
global.PEAK_KEY='gpts_peak_v1';
global.PEAK={ day:null, bar:0, m:{} };
eval(ex('peakSave')); eval(ex('peakLoad')); eval(ex('peakTick')); eval(ex('peakOf'));

// max accumulates across ticks, spike between bars included
VEL={ 7665:{k:7665,cur:80e6}, 7690:{k:7690,cur:30e6} };
STATE.SPY.lastClosedB=1; peakTick('SPY');
VEL={ 7665:{k:7665,cur:92e6}, 7690:{k:7690,cur:-49e6} };   // spike, and a negative node
peakTick('SPY');                                            // same bar — max still updates
VEL={ 7665:{k:7665,cur:88e6}, 7690:{k:7690,cur:20e6} };
STATE.SPY.lastClosedB=2; peakTick('SPY');
eq(peakOf(7665), 92e6, 'the peak is the day max, not the last value');
eq(peakOf(7690), 49e6, 'peaks are |absolute| — a -gamma node has a real size');
eq(peakOf(7000), null, 'unseen strike -> null');

// persisted per bar, rehydrated on load
{ const saved=JSON.parse(localStorage.getItem('gpts_peak_v1'));
  ok(saved && saved.day==='2026-08-25' && saved.m['7665']===92e6, 'saved at the bar gate');
  PEAK={ day:null, bar:0, m:{} };
  peakLoad();
  eq(peakOf(7665), 92e6, 'a reload rehydrates today\'s peaks'); }

// seeding from the recorder: vend rows [k, cur, ...] — max wins across stores
{ PEAK={ day:null, bar:0, m:{} };
  localStorage._s={};
  REC={ days:{ '2026-08-25':{ snaps:{ SPY:[ {vend:{rows:[[7665,95e6,0,0,0,0],[7650,12e6,0,0,0,0]]}},
                                            {vend:{rows:[[7665,60e6,0,0,0,0]]}} ] } } } };
  peakLoad();
  eq(peakOf(7665), 95e6, 'boot seeds the morning high-water from recorded bars');
  eq(peakOf(7650), 12e6, 'every recorded strike is seeded'); }

// a new day starts clean
{ TODAY_STR='2026-08-26'; REC={days:{}};
  VEL={ 7700:{k:7700,cur:5e6} }; STATE.SPY.lastClosedB=3; peakTick('SPY');
  eq(peakOf(7665), null, 'yesterday\'s peaks do not leak into today');
  eq(peakOf(7700), 5e6,  'today accumulates fresh'); }

// replay never writes
{ global.inReplay=()=>true;
  VEL={ 7700:{k:7700,cur:99e6} }; STATE.SPY.lastClosedB=4; peakTick('SPY');
  eq(peakOf(7700), 5e6, 'replay writes nothing');
  global.inReplay=()=>false; }

console.log(pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
