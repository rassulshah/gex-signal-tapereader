// (v11.37 / companion v1.5) DEX AND A REAL 25-DELTA SKEW.
//
// Confirmed live 2026-08-22 via __gptsDebug.optKeys(): their payload carries
//   strike,expireYear,expireMonth,expireDay,cp,gamma,delta,openInterest,impliedVol,bid,ask
// while every one of their PUBLISHED metrics comes back null — their page computes them client-side.
// So we compute: DEX = sum(delta * OI * 100 * spot), and skew = 25-delta put IV minus 25-delta call IV,
// which is the metric their page prints as "25Δ Skew".
const fs=require('fs'); const src=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(ex('dexSkewFor'));
const all=()=>true;
const C=(k,delta,oi,iv)=>({strike:k,cp:'C',delta:delta,openInterest:oi,impliedVol:iv,gamma:0.001});
const P=(k,delta,oi,iv)=>({strike:k,cp:'P',delta:delta,openInterest:oi,impliedVol:iv,gamma:0.001});

// ---- DEX ----
{
  const r=dexSkewFor([C(7700,0.50,100,0.12)], 7700, all);
  eq(r.netDex, 0.50*100*100*7700, 'DEX is delta x OI x 100 x spot');
}
{
  // puts already carry negative delta in their payload — no sign flip is applied
  const r=dexSkewFor([C(7700,0.50,100,0.12), P(7700,-0.50,100,0.12)], 7700, all);
  eq(r.netDex, 0, 'a matched call and put cancel — put delta is already negative, so we must not flip it again');
}
{
  const r=dexSkewFor([P(7700,-0.40,500,0.14)], 7700, all);
  ok(r.netDex<0,'a put-heavy book is NEGATIVE net delta',r.netDex);
}
{
  const r=dexSkewFor([C(7700,0.5,100,0.12),C(7705,0.4,50,0.12),P(7695,-0.3,80,0.13)], 7700, all);
  eq(r.strikes,3,'DEX is aggregated per strike');
  ok(r.dexProf.length===3,'and a near-spot profile is emitted for drawing',r.dexProf);
}
{
  const r=dexSkewFor([C(7700,0.5,100,0.12), C(9000,0.02,100,0.30)], 7700, all);
  ok(r.dexProf.length===1,'the profile is trimmed to near-spot so one far strike cannot flatten the chart',r.dexProf);
}
ok(dexSkewFor([], 7700, all)===null,'an empty selection is null, not a zero reading');
{
  const r=dexSkewFor([{strike:7700,cp:'C',gamma:0.001}], 7700, all);
  ok(r===null,'a contract with no delta and no OI contributes nothing and yields null');
}

// ---- 25-DELTA SKEW ----
{
  const r=dexSkewFor([C(7750,0.25,10,0.107), P(7600,-0.25,10,0.140)], 7700, all);
  eq(r.skew25, 3.30, 'skew is 25d put IV minus 25d call IV, in IV points — 14.0 vs 10.7 gives +3.3, matching their published figure');
  eq(r.skewPutIV,14.00,'the put leg is reported'); eq(r.skewCallIV,10.70,'and the call leg');
  eq(r.skewPutK,7600,'with the strikes used, so the number can be checked');
}
{
  // nearest-delta match, not interpolation
  const r=dexSkewFor([C(7750,0.24,10,0.11), C(7760,0.31,10,0.09), P(7600,-0.26,10,0.15)], 7700, all);
  eq(r.skewCallIV,11.00,'the CLOSEST contract to 25 delta is used, not the one nearest in strike');
}
{
  const r=dexSkewFor([C(7900,0.05,10,0.30), P(7500,-0.04,10,0.40)], 7700, all);
  eq(r.skew25,null,'when nothing sits near 25 delta the skew is null rather than measured off far wings');
}
{
  const r=dexSkewFor([C(7750,0.25,10,0.107)], 7700, all);
  eq(r.skew25,null,'one side alone cannot produce a skew');
}
{
  const r=dexSkewFor([C(7750,0.25,10,0.10), P(7600,-0.25,10,0.10)], 7700, all);
  eq(r.skew25,0,'equal IVs give a flat skew, not a null');
}
{
  const r=dexSkewFor([C(7750,0.25,10,0.16), P(7600,-0.25,10,0.10)], 7700, all);
  ok(r.skew25<0,'calls richer than puts is a NEGATIVE skew — upside bid',r.skew25);
}
// ---- ATM IV ----
{
  const r=dexSkewFor([C(7700,0.50,10,0.12), P(7700,-0.50,10,0.14)], 7700, all);
  eq(r.atmIV,13.00,'ATM IV averages the two 50-delta legs');
}
// ---- the filter is honoured ----
{
  const opts=[C(7700,0.5,100,0.12), C(7700,0.5,100,0.12)];
  const r=dexSkewFor(opts, 7700, (o)=>o===opts[0]);
  eq(r.netDex, 0.5*100*100*7700, 'only contracts passing the window filter are counted');
}
console.log('\n'+pass+' pass / '+fail+' fail');
