// Were the ARROWS (latched rolls) helpful? Measured over the pushed day files.
// T1 DIRECTION: on bars with live rolls, does the 10-bar forward path lean the roll's way?
//    Baseline: the same stat on ALL feature bars (netGamma key, same cadence, same days).
// T2 DESTINATION: how often does price REACH the roll's destination within the same 10 bars,
//    vs reaching a same-distance point on the opposite side (the honest control).
const days=['2026-08-21','2026-08-24','2026-08-25','2026-08-26'];
const U=0.0998;               // SPXW strike -> SPY scale (close enough for distances)
let dirN=0, dirWin=0, base=[], reach=0, reachN=0, ctrl=0, ctrlN=0, upN=0, dnN=0;
let perDay={};
for(const day of days){
  let d; try{ d=require('../data/'+day+'.json'); }catch(e){ continue; }
  const F=(d.feat&&d.feat.SPY)||[];
  const rolls=F.filter(r=>r.key==='rolllatch' && r.resolved && r.rec && Array.isArray(r.rec.rolls) && r.rec.rolls.length);
  const bases=F.filter(r=>r.key==='netGamma' && r.resolved && typeof r.mfe==='number' && typeof r.mae==='number');
  bases.forEach(b=>base.push(Math.abs(b.mfe)-Math.abs(b.mae)));   // up-lean magnitude, sign-free baseline
  let dW=0,dN=0,dR=0,dRN=0;
  for(const r of rolls){
    if(typeof r.mfe!=='number'||typeof r.mae!=='number') continue;
    // net roll direction, amt-weighted, LIVE rolls only (the moving arrows)
    let net=0; for(const ro of r.rec.rolls){ if(!ro.live||ro.gone) continue; net+=(ro.dir==='up'?1:-1)*(Math.abs(ro.amt)||1); }
    if(net===0) continue;
    const up=net>0; if(up) upN++; else dnN++;
    // T1: did the forward path lean the roll's way? (bigger excursion on the roll side)
    const lean=Math.abs(r.mfe)-Math.abs(r.mae);        // >0 = upside excursion dominated
    dirN++; dN++;
    if((up&&lean>0)||(!up&&lean<0)){ dirWin++; dW++; }
    // T2: reach the nearest live destination on the roll side vs same-distance control
    let bestD=null;
    for(const ro of r.rec.rolls){ if(!ro.live||ro.gone) continue;
      const dd=ro.t*U - r.px; if((up&&dd>0.05)||(!up&&dd<-0.05)){ if(bestD==null||Math.abs(dd)<Math.abs(bestD)) bestD=dd; } }
    if(bestD!=null && Math.abs(bestD)<=3){       // within a reachable band (~30 ES pts)
      reachN++; dRN++;
      const got = bestD>0 ? (r.mfe>=bestD) : (r.mae<=bestD);
      if(got){ reach++; dR++; }
      ctrlN++;
      const gotC = bestD>0 ? (r.mae<=-bestD) : (r.mfe>=-bestD);   // same distance, opposite side
      if(gotC) ctrl++;
    }
  }
  perDay[day]={rollBars:dN, dirWin:dW, reach:dR+'/'+dRN};
}
const baseUp=base.filter(x=>x>0).length, baseN=base.length;
console.log('days:', JSON.stringify(perDay));
console.log('T1 DIRECTION: forward path leaned the roll\'s way on '+dirWin+'/'+dirN+' bars ('+(100*dirWin/dirN).toFixed(1)+'%)  [up-roll bars '+upN+', down '+dnN+']');
console.log('   baseline (all bars, upside-dominant): '+baseUp+'/'+baseN+' ('+(100*baseUp/baseN).toFixed(1)+'%) — a fair coin sits near 50%');
console.log('T2 DESTINATION REACHED within 10 bars: '+reach+'/'+reachN+' ('+(100*reach/reachN).toFixed(1)+'%)');
console.log('   same-distance OPPOSITE side reached: '+ctrl+'/'+ctrlN+' ('+(100*ctrl/ctrlN).toFixed(1)+'%) — the control');
process.exit(0);
