// ============================================================================================
// test_measure.js — MEASURE THE INSTRUMENT HE TRADES.
//
// Operator, 2026-08-30: "its the es that i am trading but using spxw nodes" · "we are using other
// markets to get things like their kings because ES doesn't have its own book, so we use other
// tapes".
//
//     STRUCTURE   nodes / kings / walls / flip   <- SPXW, SPY, QQQ.  ES has no book. Correct.
//     MEASUREMENT HOD, LOD, candle, EFF, GD/RD   <- must be ES's OWN bars, not SPY x 10.04.
//
// ⚠⚠ MEASURED ON HIS PANEL: SPX and ES are 10.74 points apart — natively compatible, which is why
// reading SPXW nodes on an ES chart works. SPY is 10.0377x away, and EVERY scale failure of
// 2026-08-30 had SPY in the display path. The ratio is an EMA: never exactly right, and it drifts.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
// (v15.54) the four hot readers are memoised per frame in the panel; the harness runs them straight through
global.rmemo=function(k,f){ return f(); }; global.rmemoNext=function(){};
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const grab=(n)=>{ const i=src.indexOf('function '+n+'('); if(i<0) return ''; let d=0,st=false;
  for(let j=i;j<src.length;j++){ const c=src[j];
    if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0) return src.slice(i,j+1);} } return ''; };

const MB=grab('measureBarsRaw').replace(/\/\/[^\n]*/g,'');
ok(!!MB, 'x1 measureBars exists');
ok(/dispIsFut\(\)/.test(MB), 'x2 it only prefers ES when the chart IS a future');
ok(/futBarsLoad\(\)/.test(MB), 'x3 ...and reads the true ES 1-minute bars');
ok(/scale:1, src:'ES'/.test(MB), 'x4 ES bars are already chart-scale, so scale is 1 — no conversion');
ok(/src:'SPY'/.test(MB), 'x5 the SPY proxy remains a fallback');
// ⚠ a measurement whose instrument is unknown is not a measurement
ok(/src:'ES'/.test(MB) && /src:'SPY'/.test(MB), 'x6 ...and it always SAYS which instrument it used');
ok(/length>=30/.test(MB), 'x7 a stub session is refused rather than measured');

// every consumer of the measurement path must use it
[['hodLod','var MB=measureBars(sym)'],
 ['gdRead','var MBg=measureBars(sym)'],
 ['gdActual','measureBars(sym).bars'],
 ['hlEff','var MBe=measureBars(sym)']].forEach(([fn,frag],i)=>{
   const b=grab(fn);
   ok(b.indexOf(frag)>=0, 'x'+(8+i)+' '+fn+' measures via measureBars');
 });

// ⚠⚠ THE TRAP: reading dispR() after ES bars would multiply ES prices by ~10 — the exact failure
// this change removes. hodLod's scale must come from measureBars, never from the ratio.
const HL=grab('hodLod').replace(/\/\/[^\n]*/g,'');
ok(/rr=MB\.scale; out\.scale=rr;/.test(HL),
   'x12 hodLod takes its scale FROM measureBars, never from dispR()');
ok(!/out\.scale=rr; out\.isFut=\(rr!==1\);/.test(HL),
   'x13 ...and the old ratio-derived scale is gone');
const HE=grab('hlEff').replace(/\/\/[^\n]*/g,'');
ok(/rr=MBe\.scale/.test(HE), 'x14 EFF divides a range and a walk measured on the SAME instrument');

// the structure path is UNCHANGED — ES has no book, so the other tapes stay
ok(/IFSYM=\{ SPY:'SPX', QQQ:'QQQ' \}/.test(src), 'x15 the ladder still reads the SPX book');
ok(/function deflKings/.test(src) && /'SPXW'/.test(src), 'x16 the kings still come from the other tapes');

console.log('test_measure: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
