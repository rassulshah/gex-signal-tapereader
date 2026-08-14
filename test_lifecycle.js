// (v10.33) NODE LIFECYCLE tests — Skylit Academy (source of truth, node-lifecycle):
// Fresh (0 taps, ~80%) -> Tested (1, ~66%) -> Delivered (2+, ~33%, graveyard) -> Decaying
// (weakening, no interaction). A tap = touch within TAP_TOL, then leave by >=TAP_AWAY,
// then return = the NEXT tap (one long sit != many taps).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

// sync-guard the shipped constants
ok(src.indexOf('var TAP_TOL   = 0.20')>=0, 'shipped TAP_TOL=0.20 (sync-guard)');
ok(src.indexOf('var TAP_AWAY  = 0.60')>=0, 'shipped TAP_AWAY=0.60');
ok(src.indexOf('var TAP_PROB = { 0:80, 1:66, 2:33 }')>=0, 'shipped TAP_PROB 80/66/33 (Academy)');

// harness state + minimal deps
var TAP_TOL=0.20, TAP_AWAY=0.60, TAP_PROB={0:80,1:66,2:33};
var TAPS={SPY:{},QQQ:{}};
var STATE={SPY:{price:772, candles:[]}};
function todayKey(){ return '2026-8-13'; }
function tapeMap(sym){ return { pct:{ '770.00':80, '775.00':60 } }; }  // two tracked nodes

eval(ex('updateTaps'));
eval(ex('nodeTapCount'));
eval(ex('nodeLifecycle'));

function bar(l,h){ return {l:l,h:h}; }
function feed(px, l, h){ STATE.SPY.price=px; STATE.SPY.candles=[bar(l,h)]; updateTaps('SPY'); }

// ---- FRESH: never touched ----
feed(772, 771.5, 772.5);   // near neither 770 nor 775
ok(nodeTapCount('SPY',770)===0, '770 untouched -> 0 taps');
ok(nodeLifecycle('SPY',770,'Steady').stage==='Fresh', '0 taps + steady -> Fresh');
ok(nodeLifecycle('SPY',770,'Steady').prob===80, 'Fresh -> ~80% 1st-tap prob');

// ---- TESTED: one tap ----
feed(770.1, 769.9, 770.3);   // wick touches 770 (within 0.20)
ok(nodeTapCount('SPY',770)===1, 'wick into 770 -> 1 tap');
ok(nodeLifecycle('SPY',770,'Steady').stage==='Tested', '1 tap -> Tested');
ok(nodeLifecycle('SPY',770,'Steady').prob===66, 'Tested -> ~66% prob');

// ---- one long SIT should NOT inflate taps ----
feed(770.05, 769.95, 770.15);  // still sitting on 770, never left
ok(nodeTapCount('SPY',770)===1, 'still sitting on 770 (never left) -> still 1 tap (no inflation)');

// ---- price LEAVES (>= TAP_AWAY), then RETURNS = 2nd tap = DELIVERED ----
feed(771.2, 771.0, 771.4);    // left 770 by >=0.60 (re-arms)
feed(770.1, 769.9, 770.3);    // returns and touches -> 2nd tap
ok(nodeTapCount('SPY',770)===2, 'left then returned -> 2nd tap');
ok(nodeLifecycle('SPY',770,'Steady').stage==='Delivered', '2 taps -> Delivered (graveyard)');
ok(nodeLifecycle('SPY',770,'Steady').prob===33, 'Delivered -> ~33% prob');

// ---- DECAYING: untouched but fading ----
ok(nodeLifecycle('SPY',775,'Fading').stage==='Decaying', 'untouched + Fading -> Decaying');
ok(nodeLifecycle('SPY',775,'Fading').prob===null, 'Decaying -> no tap prob (spent/irrelevant)');
ok(nodeLifecycle('SPY',775,'Steady').stage==='Fresh', 'untouched + Steady (775) -> Fresh (never tapped)');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
