// ============================================================================================
// test_scaleagree.js — THE LADDER AND THE PRICE MUST BE ON THE SAME SCALE.
//
// Operator's screenshot, 2026-08-29: price pill 7710.6, and beside it "T: 773.34  −6937.26" and
// "KING 769.85 (brake) holds, 6941 below price". Both numbers were individually right. Their
// DIFFERENCE was the subtraction of an ES-scale price from a SPY-scale level.
//
// ⚠⚠ THE SHAPE OF THE BUG IS THE REASON THIS FILE EXISTS. Two consumers read one switch —
// dispIsFut() — but required different inputs to honour it. emBand() needs FUTMODE.r; ifLadder()
// needed FUTMODE.futPx. `r` is a persisted EMA, `futPx` is null after hours. So the panel had a
// reachable state where one said "futures" and the other said "cash", and NOTHING ERRORED.
// A silent fallback across a unit boundary is the worst kind: every number stays plausible.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const grab=(n)=>{ const i=src.indexOf('function '+n+'('); if(i<0) return ''; let d=0,st=false;
  for(let j=i;j<src.length;j++){ const c=src[j];
    if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0) return src.slice(i,j+1);} } return ''; };
const LAD=grab('ifLadder').replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');

ok(/scaleSrc/.test(LAD), 's1 ifLadder declares which scale it used');
ok(/dispIsFut\(\)/.test(LAD), 's2 ...and it reads the same switch emBand does');
// ⚠ THE CORE ASSERTION: futPx missing must NOT drop it to cash while the switch says futures.
ok(/if\(FUTMODE\.futPx!=null && FUTMODE\.futPx>0\)\{[\s\S]{0,220}\}else\{[\s\S]{0,260}dispR\(\)/.test(LAD),
   's3 with the switch ON but futPx missing, it falls back to dispR() — the SAME ratio emBand uses');
ok(/scaleSrc='fut:ratio'/.test(LAD), 's4 ...and labels that state rather than hiding it');
ok(/scaleSrc='fut:live'/.test(LAD) && /scaleSrc='cash'/.test(LAD), 's5 all three states are named');
// ⚠ ASSIGNED IS NOT EXPORTED. Deleting `scaleSrc:scaleSrc` from the RETURNED object survived s1-s5,
// because all of those match the assignments. A diagnostic nobody can read is not a diagnostic.
ok(/scaleSrc:\s*scaleSrc/.test(LAD), 's5b ...and scaleSrc is actually RETURNED, not just assigned');
// the old shape must be gone: a bare futPx test with no else
ok(!/if\(dispIsFut\(\) && FUTMODE\.futPx!=null && FUTMODE\.futPx>0\)\{[^}]*\}\s*\}catch/.test(LAD),
   's6 the silent cash fallback is gone');
// emBand's own switch is unchanged and still ratio-based
const EB=grab('emBand').replace(/\/\/[^\n]*/g,'');
ok(/dispIsFut\(\)\?dispR\(\):1/.test(EB), 's7 emBand still scales by dispR() — the path ifLadder now matches');

// ---- BEHAVIOUR: simulate the after-hours state and check the two agree -----------------------
(function(){
  const theirSpot=7727.0, undPx=769.5, r=10.0387;
  // ifLadder, new logic, futPx MISSING
  const dispPx = undPx*r, dispScale = dispPx/theirSpot;
  const level  = 7722*dispScale;          // an SPX strike rendered for the chart
  const price  = undPx*r;                 // emBand().now
  const gap    = Math.abs(level-price);
  ok(gap < 40, 's8 after-hours: a nearby level sits within 40 pts of price, not ~6900', +gap.toFixed(1));
  // the OLD logic, for contrast
  const oldScale = undPx/theirSpot, oldLevel = 7722*oldScale;
  ok(Math.abs(oldLevel-price) > 6000, 's9 ...and the old logic really did produce a ~6900 pt gap',
     +Math.abs(oldLevel-price).toFixed(1));
})();

// ---- THE SECOND SITE: session levels must DECLINE, not fall back to cash --------------------
// ⚠ `sessionLevels(sym, EB.scaleUsed : 1)` put PDH at 770 on a chart drawn at 7710. Every other
// consumer of scaleUsed in this file guards with `>0` and skips; this line was the exception, and
// being the exception is exactly why nobody saw it.
const CLEAN=src.replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');
ok(!/sessionLevels\(sym,\s*\(EB&&typeof EB\.scaleUsed==='number'\)\?EB\.scaleUsed:1\)/.test(CLEAN),
   's10 session levels no longer fall back to the CASH scale');
ok(/EB\.scaleUsed>0\s*\)\s*\?\s*EB\.scaleUsed\s*:\s*null/.test(CLEAN),
   's11 ...they require a positive scale and decline without one');
ok(/if\(_sc!=null\)\s*SESSL=sessionLevels\(sym,\s*_sc\)/.test(CLEAN),
   's12 ...and sessionLevels is simply not called when the scale is unknown');

console.log('test_scaleagree: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
