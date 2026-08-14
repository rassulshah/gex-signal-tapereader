// Issue D regression: S/R Imbalance = rate divergence + bar-close-committed dom (no flicker).
const fs = require('fs');
const src = fs.readFileSync('./v10.js','utf8');

function mul(a,b){ return a/(1/b); }
global.mul = mul;
global.MIN_STRENGTH = 5;
global.PROX_BARS = 8; global.PROX_MOVE_BARS = 6; global.PROX_MIN_STRIKES = 3; global.PROX_MAX_STRIKES = 12;
global.CANDLE_S = 180;
global.RENDER_SEQ = 0;
global.SRB_PREV = { SPY:null };
global.SRB_STATE = { SPY:{dom:'balanced', bucket:-1, cand:null, candBuckets:0} };
global.SRB_BAND = 0.015; global.SRB_FLIP_MULT = 1.8; global.SRB_HOLD_BARS = 2;
global.SRB_CACHE = { SPY:{seq:-1,val:null} };

// node history store the fns read via nodeHistory/nodeBuildRate
const HIST = {};                       // key strike -> [pctKing samples]
global.nodeHistory = function(sym,k){ return HIST[k.toFixed(2)]||null; };

// clock we can advance for bar-close tests
let NOW_SEC = 34200;                    // 9:30-ish
global.ctNowSecOfDay = function(){ return NOW_SEC; };

function extract(name){
  const re = new RegExp('function\\s+'+name+'\\s*\\(', 'g');
  const m = re.exec(src); if(!m) throw new Error('not found: '+name);
  let i = src.indexOf('{', m.index), depth=0, end=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')depth++; else if(src[k]==='}'){depth--; if(depth===0){end=k;break;}} }
  return src.slice(m.index, end+1);
}
// stub accumulationStateFor (srBattle only reads its .label for a fade penalty)
global.accumulationStateFor = function(sym,k){
  var h=HIST[k.toFixed(2)]; if(!h||h.length<2) return {label:'Steady'};
  var d=h[h.length-1]-h[0];
  return {label: d>3?'Building':(d<-3?'Fading':'Steady')};
};
const code = ['nodeBuildRate','adaptiveProxStrikes','res_nearest','imbalanceMetric','srBattle']
  .map(function(n){ try{return extract(n);}catch(e){return '';} }).join('\n');
eval(code);

// ---- scenario: BEARISH imbalance. price 771.9; resistance 772/773 BUILDING, support 769 FADING ----
function setBearish(){
  HIST['772.00']=[70,74,80];   // resistance building (rate +10)
  HIST['773.00']=[60,66,72];   // resistance building (rate +12)
  HIST['769.00']=[80,72,60];   // support fading (rate -20)
  HIST['767.00']=[40,40,41];   // support flat
}
setBearish();
global.STATE = { SPY:{ price:771.9, candles:[{h:772.4,l:771.2},{h:772.1,l:771.0},{h:772.6,l:771.3}],
  walls:[ {k:773,pct:72}, {k:772,pct:88}, {k:769,pct:54}, {k:767,pct:40} ] } };

let pass=0, fail=0;
function ok(name, cond, got){ if(cond){pass++; console.log('PASS '+name+(got!==undefined?' -> '+got:''));} else {fail++; console.log('FAIL '+name+(got!==undefined?' -> got '+got:''));} }

// 1) imbalance metric is negative (resistance gaining => bearish)
const im = imbalanceMetric('SPY', 771.9);
ok('imbalance is bearish (val<0)', im.val < 0, im.val.toFixed(3));
ok('imbalance not forming (>=3 samples)', im.forming===false, im.forming);
ok('gaining strike is a resistance (>=772)', im.gain>=772, im.gain);
ok('fading strike is the support 769', im.fade===769, im.fade);

// 2) first srBattle commits bearish (resistance dom) on this bar close
NOW_SEC = 34200; global.RENDER_SEQ++; global.SRB_CACHE.SPY.seq=-1;
const o1 = srBattle('SPY');
ok('srBattle dom committed = resistance (bearish)', o1.dom==='resistance', o1.dom);
ok('srBattle exposes mechanism strikes', o1.gain>=772 && o1.fade===769, 'gain='+o1.gain+' fade='+o1.fade);

// 3) FLICKER TEST: within the SAME bar, flip the rates hard to bullish -> dom must NOT change intrabar
HIST['772.00']=[80,74,70];   // now fading
HIST['769.00']=[60,72,80];   // now building
global.STATE.SPY.walls=[ {k:773,pct:72}, {k:772,pct:88}, {k:769,pct:54}, {k:767,pct:40} ];
global.RENDER_SEQ++; global.SRB_CACHE.SPY.seq=-1;
const o2 = srBattle('SPY');   // same NOW_SEC bucket
ok('dom HOLDS across intrabar rate flip (no flicker)', o2.dom==='resistance', o2.dom);

// 4) sustained bullish across HOLD_BARS closes -> eventually flips to support
let flipped=null;
for(let b=1;b<=SRB_HOLD_BARS+1;b++){
  NOW_SEC = 34200 + b*180;                 // new bar closes
  global.RENDER_SEQ++; global.SRB_CACHE.SPY.seq=-1;
  const o = srBattle('SPY');
  flipped = o.dom;
}
ok('sustained bullish rate eventually flips dom to support', flipped==='support', flipped);

console.log(fail===0 ? '\nALL PASS' : '\n'+fail+' FAILED');
process.exit(fail===0?0:1);
