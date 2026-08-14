// (v10.27) Regression: BREAKOUT QUALITY GATE — a BO only fires if the breakout bar
// also prints a new 14-bar EXTREME (14-bar HIGH for longs / 14-bar LOW for breakdowns),
// window INCLUSIVE of the breakout bar. Tests the pure isNBarExtreme() helper + a
// sync-guard that fails if BO_HL_LOOKBACK drifts from 14.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(ex('isNBarExtreme'));

// --- SYNC GUARD: the constant must stay 14 (drift = silent behavior change) ---
const mC = src.match(/var\s+BO_HL_LOOKBACK\s*=\s*(\d+)/);
ok(mC && parseInt(mC[1],10)===14, 'BO_HL_LOOKBACK constant is 14 (sync-guard) -> '+(mC?mC[1]:'MISSING'));

// helper: build N bars; caller sets the last bar's h/l to test the extreme
function bars(highs, lows){ return highs.map((h,i)=>({h:h, l:lows[i], t:i, b:i})); }

// --- LONG: last bar makes a NEW 14-bar high -> qualifies ---
let H=[10,11,12,11,10,11,12,13,12,11,12,13,12, 14];   // last=14 is highest
let L=[ 9,10,11,10, 9,10,11,12,11,10,11,12,11, 13];
ok(isNBarExtreme(bars(H,L),'long',14)===true, 'long: breakout bar IS the 14-bar high -> BO allowed');

// --- LONG: last bar does NOT make a new high (an earlier bar was higher) -> blocked ---
H=[10,11,12,11,10,11,12,15,12,11,12,13,12, 14];        // bar #7 (15) > last (14)
ok(isNBarExtreme(bars(H,L),'long',14)===false, 'long: weaker poke (not a 14-bar high) -> BO blocked');

// --- LONG: TIE at the high (last equals prior max) still qualifies (>=) ---
H=[10,11,12,11,10,11,12,14,12,11,12,13,12, 14];        // prior max 14 == last 14
ok(isNBarExtreme(bars(H,L),'long',14)===true, 'long: tie at the high (last==max) -> BO allowed (>=)');

// --- SHORT: last bar makes a NEW 14-bar low -> qualifies ---
let H2=[20,19,18,19,20,19,18,17,18,19,18,17,18, 16];
let L2=[19,18,17,18,19,18,17,16,17,18,17,16,17, 15];   // last=15 is lowest
ok(isNBarExtreme(bars(H2,L2),'short',14)===true, 'short: breakdown bar IS the 14-bar low -> BO allowed');

// --- SHORT: last bar not a new low (an earlier bar was lower) -> blocked ---
L2=[19,18,17,18,19,18,17,12,17,18,17,16,17, 15];       // bar #7 (12) < last (15)
ok(isNBarExtreme(bars(H2,L2),'short',14)===false, 'short: shallow poke (not a 14-bar low) -> BO blocked');

// --- INSUFFICIENT bars (<14) -> cannot confirm expansion -> false ---
ok(isNBarExtreme(bars([1,2,3],[0,1,2]),'long',14)===false, 'fewer than 14 bars -> BO blocked (no confirmation)');

// --- window is INCLUSIVE and only the LAST 14 count (older lower/higher bars ignored) ---
// 15 bars; bar#0 is a huge high but falls OUTSIDE the last-14 window -> last still qualifies
let H3=[99, 10,11,12,11,10,11,12,13,12,11,12,13,12, 14];
let L3=[98,  9,10,11,10, 9,10,11,12,11,10,11,12,11, 13];
ok(isNBarExtreme(bars(H3,L3),'long',14)===true, 'window inclusive: old out-of-window high (99) ignored -> last 14-bar high qualifies');

console.log(fail===0?'\nALL PASS':'\n'+fail+' FAILED'); process.exit(fail===0?0:1);
