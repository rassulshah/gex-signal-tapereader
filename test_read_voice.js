// (v10.50) READ VOICE — the locked 3-beat sentence (WHERE · STATE+LEAN · POTENTIAL),
// one per state: bounce / reject / cont / split. read3Beat is PURE (primitives in,
// sentence out), so the voice is pinned here without the full spine.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0){ console.error('MISSING '+name); process.exit(1); }
  var d=0,st=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0){j++;break;}}} return src.slice(i,j); }
let p=0,f=0; function ok(c,m,g){ if(c){p++;} else {f++; console.log('  FAIL: '+m+(g!==undefined?' -> '+g:''));} }

function fmtNum(x){ return (Math.round(x*100)/100).toString(); }
// (v10.54) the head now names the REAL range zone (near Flr / mid / near Ceil) via
// rangePosOf, instead of saying "Mid-range" whatever price was doing.
eval(['_nmIsAcc','_nmIsDec','zoneRole','rangePosOf','read3Beat'].map(grab).join('\n'));

function L(o){ return Object.assign({state:'Steady',chg:0,taps:0,pos:true},o); }

// ---- BOUNCE: dir UP, in-play King below price, floor building, drift up ----
var flrB=L({k:772,isFlr:true,state:'Building'}), ceilB=L({k:776,isCeil:true});
var kingB=L({k:773,isKing:true});
var vB=read3Beat('UP', kingB, 773.4, flrB, ceilB, {dir:1,verdict:'AGREE-UP'}, 776, 'Ceil');
console.log('BOUNCE:', vB.kind, '|', vB.sentence);
ok(vB.kind==='bounce','bounce kind', vB.kind);
ok(vB.sentence==='At King 773. Support building with GEX and VEX leaning up. Potential bounce to 776.','bounce sentence', vB.sentence);
ok(vB.verdict==='BULLISH' && vB.arrow==='↑','bounce verdict word');

// ---- REJECT: dir DN, in-play Ceil above price, ceil building, drift down ----
var flrR=L({k:772,isFlr:true}), ceilR=L({k:776,isCeil:true,state:'Building'});
var vR=read3Beat('DN', ceilR, 774, flrR, ceilR, {dir:-1,verdict:'AGREE-DN'}, 772, 'Flr');
console.log('REJECT:', vR.kind, '|', vR.sentence);
ok(vR.kind==='reject','reject kind', vR.kind);
ok(vR.sentence==='At Ceil 776. Resistance building with GEX and VEX leaning down. Potential drop to 772.','reject sentence', vR.sentence);
ok(vR.verdict==='BEARISH' && vR.arrow==='↓','reject verdict word');

// ---- CONT: dir UP, price BELOW the in-play Gate, drift up, target King ----
// (v10.54, audit 7) 'cont' means price is travelling THROUGH the node it is engaging,
// i.e. dirNum === -holdDir. `L.isGatekeeper` used to be ORed in, so a clean bounce OFF a
// gatekeeper was narrated as a break through it. Price is now placed below the gate,
// which is what "through" actually requires.
var flrC=L({k:772,isFlr:true}), ceilC=L({k:776,isCeil:true});
var gate=L({k:774,isGatekeeper:true});
var vC=read3Beat('UP', gate, 773.8, flrC, ceilC, {dir:1,verdict:'AGREE-UP'}, 776, 'King');
console.log('CONT:', vC.kind, '|', vC.sentence);
ok(vC.kind==='cont','cont kind', vC.kind);
ok(vC.sentence==='Through Gate 774. GEX and VEX leaning up. Potential run to King 776.','cont sentence', vC.sentence);
// a gatekeeper price is sitting ON TOP of, with the read pointing up, is a BOUNCE off it
var vCb=read3Beat('UP', gate, 774.4, flrC, ceilC, {dir:1,verdict:'AGREE-UP'}, 776, 'King');
ok(vCb.kind==='bounce','a gatekeeper BELOW price with an up read is a bounce, not a break through it', vCb.kind);

// ---- SPLIT: dir SIDE (no lean), mid-range ----
var flrS=L({k:772,isFlr:true}), ceilS=L({k:776,isCeil:true});
var kingS=L({k:774,isKing:true});
var vS=read3Beat('SIDE', kingS, 774, flrS, ceilS, {dir:0,verdict:'SPLIT'}, null, '');
console.log('SPLIT:', vS.kind, '|', vS.sentence);
ok(vS.kind==='split','split kind', vS.kind);
ok(vS.sentence==='Mid-range 772–776. GEX and VEX split — no clean lean, rotation likely.','split sentence', vS.sentence);
ok(vS.verdict==='SIDEWAYS' && vS.arrow==='→','split verdict word');

// ---- SPLIT via drift SPLIT even when a direction exists ----
var vS2=read3Beat('UP', kingS, 774, flrS, ceilS, {dir:1,verdict:'SPLIT'}, 776, 'Ceil');
ok(vS2.kind==='split','drift SPLIT forces split kind', vS2.kind);

// ---- no target: POTENTIAL beat is omitted, never blank ----
var vNo=read3Beat('UP', kingB, 773.4, flrB, ceilB, {dir:1,verdict:'AGREE-UP'}, null, '');
ok(!/Potential/.test(vNo.sentence) && /^At King 773\./.test(vNo.sentence),'no target -> potential beat dropped cleanly', vNo.sentence);

// ---- never emits order words ----
ok(!/\b(buy|sell|long|short|stop|size)\b/i.test(vB.sentence+vR.sentence+vC.sentence+vS.sentence),'descriptive only, no order words');

console.log('test_read_voice: '+p+' passed, '+f+' failed');
process.exit(f?1:0);
