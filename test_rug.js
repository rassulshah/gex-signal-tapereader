// (v10.30) Rug / Reverse-Rug — polarity-gated (yellow-over-purple, no floor) + HARDENED:
// anchors must be STRONG (>=RUG_ANCHOR_PCT) and the yellow ceiling must sit DIRECTLY over
// the purple node (within RUG_ADJ strikes), not an arbitrary far pairing.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function mul(a,b){return a/(1/b);} global.mul=mul;
global.RUG_SIG_PCT=20; global.RUG_ANCHOR_PCT=40; global.RUG_ADJ=3.0;
global.fmtNum=function(n){return (Math.round(n*100)/100).toString();};
let RATES={}; global.nodeBuildRate=function(sym,k){return RATES[k]||0;};
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let RUG_POLARITY_VERIFIED=true;   // verified live 2026-08-12
eval(['finalizeRug','rugDetect'].map(ex).join('\n'));
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n+(g!==undefined?' -> '+g:''));}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+g:''));}};
function W(k,pct,pos){return {k,pct,pos,net:0};}

// --- SYNC GUARD on the hardened thresholds ---
ok('shipped RUG_ANCHOR_PCT=40', src.indexOf('var RUG_ANCHOR_PCT = 40')>=0, true);
ok('shipped RUG_ADJ=3.0', src.indexOf('var RUG_ADJ        = 3.0')>=0, true);

// --- RUG: strong yellow ceiling DIRECTLY over strong purple (within 3), neg stack, no pos floor ---
// price 610; 611 +gamma(yellow) ceiling; 609 -gamma(purple, 2 below); 607/605 -gamma floors; no + floor.
RATES={609:-8,607:-6,605:-5,611:0};
global.STATE={SPY:{price:610, walls:[ W(611,60,true), W(609,80,false), W(607,55,false), W(605,45,false) ]}};
let r=rugDetect('SPY');
ok('RUG detected (strong, adjacent anchors)', r.ok===true && r.type==='Rug', r.type);
ok('RUG ceiling = yellow 611', r.ceilK===611, r.ceilK);
ok('RUG floor = purple 609', r.floorK===609, r.floorK);
ok('RUG targets are the neg-gamma floors', r.targets.length>=2, r.targets.join(','));
ok('RUG confirm downside growing', r.confirm.downGrowing===true, r.confirm.downGrowing);
ok('RUG SHOWN (polarity verified)', r.shown===true, r.shown);

// --- HARDENING #1: a FAR purple (yellow 611 over purple 604 = 7 strikes) is NOT a rug ---
RATES={604:-8,602:-6};
global.STATE={SPY:{price:610, walls:[ W(611,60,true), W(604,80,false), W(602,55,false) ]}};
r=rugDetect('SPY');
ok('no RUG when purple is far below the yellow (>RUG_ADJ)', !(r.ok&&r.type==='Rug'), r.type);

// --- HARDENING #2: WEAK anchors (yellow 30, purple 30 < 40) are NOT a rug ---
global.STATE={SPY:{price:610, walls:[ W(611,30,true), W(609,30,false), W(607,25,false) ]}};
r=rugDetect('SPY');
ok('no RUG when anchors are weak (<RUG_ANCHOR_PCT)', !(r.ok&&r.type==='Rug'), r.type);

// --- NO rug when a strong positive floor exists beneath ---
RATES={};
global.STATE={SPY:{price:610, walls:[ W(611,60,true), W(609,80,false), W(600,90,true) ]}};
r=rugDetect('SPY');
ok('no RUG when a strong positive floor sits beneath', !(r.ok&&r.type==='Rug'), r.type);

// --- REVERSE-RUG (mirror): strong purple ceiling over strong yellow floor, no neg ceiling above ---
global.STATE={SPY:{price:610, walls:[ W(612,70,false), W(609,75,true), W(607,50,true) ]}};
r=rugDetect('SPY');
ok('REVERSE-RUG detected (bullish mirror)', r.ok===true && r.type==='Reverse-Rug', r.type);

// --- REVERSE-RUG rejected when the yellow floor is weak ---
global.STATE={SPY:{price:610, walls:[ W(612,70,false), W(609,30,true) ]}};
r=rugDetect('SPY');
ok('no REVERSE-RUG when yellow floor is weak', !(r.ok&&r.type==='Reverse-Rug'), r.type);

console.log(f===0?'\nALL PASS':'\n'+f+' FAILED'); process.exit(f===0?0:1);
