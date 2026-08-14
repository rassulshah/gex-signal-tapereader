// (v10.34) DEFLECTION tests — Skylit Academy (execution-doctrine + core-concepts):
// a DETECTED reversal off a node. Price taps within DEFLECT_ZONE, reverses away by
// >=DEFLECT_AWAY, sustained >=DEFLECT_CONFIRM bars. EVENT REPORT, never a prediction.
// Distinct from broke/held/false: a deflection is a clean bounce/rejection.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

// sync-guard constants
ok(src.indexOf('var DEFLECT_ZONE    = 0.50')>=0, 'shipped DEFLECT_ZONE=0.50 (docs +-0.50 SPY/QQQ)');
ok(src.indexOf('var DEFLECT_AWAY    = 0.45')>=0, 'shipped DEFLECT_AWAY=0.45');
ok(src.indexOf('var DEFLECT_CONFIRM = 2')>=0, 'shipped DEFLECT_CONFIRM=2 (multi-bar, anti one-bar misread)');

var DEFLECT_ZONE=0.50, DEFLECT_AWAY=0.45, DEFLECT_CONFIRM=2, DEFLECT_WINDOW=8;
var STATE={SPY:{}};
eval(ex('deflectionAt'));
function C(l,h,c){ return {l:l,h:h,c:c}; }

// ---- DEFLECTION UP off a floor at 770: tap, then reverse up and hold ----
STATE.SPY={ price:771.0, walls:[{k:770,pos:true}], candles:[
  C(771.0,771.4,771.2), C(770.4,771.1,770.6),  // wick taps 770 (within 0.50)
  C(770.6,771.3,771.1), C(771.0,771.6,771.5)    // reverses UP and holds above
]};
var d1=deflectionAt('SPY',770);
ok(d1 && d1.dir===1, 'tap 770 then reverse UP + hold -> deflection UP (dir=+1)');
ok(d1 && d1.awayPts>=0.45, 'away distance recorded (>=DEFLECT_AWAY)');
ok(d1 && d1.pos===true, 'polarity flavor captured (+gamma floor)');

// ---- DEFLECTION DOWN off a ceiling at 780: tap, then reverse down and hold ----
// tap on bar1; bars 2 & 3 close clearly BELOW 780 and out of the zone (<779.5).
STATE.SPY={ price:778.8, walls:[{k:780,pos:true}], candles:[
  C(779.0,779.3,779.2), C(779.6,780.2,779.7),  // wick taps 780 (bar 1)
  C(778.9,779.2,779.0), C(778.6,779.0,778.8)    // reverses DOWN, closes below & out of zone
]};
var d2=deflectionAt('SPY',780);
ok(d2 && d2.dir===-1, 'tap 780 then reverse DOWN + hold -> deflection DOWN (dir=-1)');

// ---- NO deflection: price tapped but is STILL sitting on the node (no clean turn) ----
STATE.SPY={ price:770.1, walls:[{k:770,pos:true}], candles:[
  C(770.0,770.4,770.2), C(769.9,770.3,770.1), C(770.0,770.2,770.1), C(769.95,770.25,770.1)
]};
ok(!deflectionAt('SPY',770), 'still sitting on the node (away < DEFLECT_AWAY) -> NOT a deflection');

// ---- NO deflection: never entered the zone ----
STATE.SPY={ price:775.0, walls:[{k:770,pos:true}], candles:[
  C(774.5,775.5,775.0), C(774.6,775.4,775.1), C(774.7,775.3,775.0)
]};
ok(!deflectionAt('SPY',770), 'never tapped the node zone -> no deflection');

// ---- multi-bar confirmation: a one-bar poke-and-back does NOT count ----
STATE.SPY={ price:771.0, walls:[{k:770,pos:true}], candles:[
  C(771.0,771.4,771.2), C(770.4,771.1,771.0)   // tapped on the LAST bar; barsSince < CONFIRM
]};
ok(!deflectionAt('SPY',770), 'tap on the most recent bar (barsSince<CONFIRM) -> not yet confirmed');

// ---- -gamma flavor is captured (counter-character deflection) ----
STATE.SPY={ price:771.0, walls:[{k:770,pos:false}], candles:[
  C(771.0,771.4,771.2), C(770.4,771.1,770.6), C(770.6,771.3,771.1), C(771.0,771.6,771.5)
]};
var d3=deflectionAt('SPY',770);
ok(d3 && d3.pos===false, '-gamma node deflection captures pos=false (flavor: counter-character)');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
