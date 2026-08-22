// (companion v1.6) EXPECTED MOVE, CHARM, GAMMA FLIP — all computed from their per-contract payload,
// because every metric their page renders comes back null in the data.
const fs=require('fs'); const src=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// eval() inside a forEach callback declares into the CALLBACK's scope, not this one. Join first.
eval(['npdf','yearsTo','d1Of','expectedMove','gammaFlip'].map(ex).join('\n'));
const all=()=>true;
const O=(k,cp,o)=>Object.assign({strike:k,cp:cp,expireYear:2026,expireMonth:8,expireDay:28,openInterest:100,impliedVol:0.12,delta:0.5,gamma:0.001},o||{});

// ---- expected move ----
{
  const r=expectedMove([O(7700,'C',{bid:38,ask:42}),O(7700,'P',{bid:36,ask:40})],7700,all);
  eq(r.em,78,'the straddle is the two mids added — 40 + 38');
  eq(r.k,7700,'and it names the strike it used');
  ok(Math.abs(r.pct-1.01)<0.02,'expressed as a percent of spot too',r.pct);
}
{
  const r=expectedMove([O(7700,'C',{bid:38,ask:42}),O(7650,'P',{bid:36,ask:40})],7700,all);
  eq(r,null,'legs at DIFFERENT strikes are not a straddle');
}
{
  const r=expectedMove([O(7700,'C',{bid:38,ask:42})],7700,all);
  eq(r,null,'one leg is not a straddle');
}
{
  const r=expectedMove([O(7700,'C',{bid:0,ask:0}),O(7700,'P',{bid:0,ask:0})],7700,all);
  eq(r,null,'a zero-priced straddle is refused rather than reported as no expected move');
}
{
  // the nearest pair is 3% away — too far to be "at the money"
  const r=expectedMove([O(7930,'C',{bid:5,ask:6}),O(7930,'P',{bid:5,ask:6})],7700,all);
  eq(r,null,'a pair far from spot is not an ATM straddle');
}
{
  const r=expectedMove([O(7700,'C',{bid:38,ask:42}),O(7700,'P',{bid:36,ask:40}),
                        O(7705,'C',{bid:35,ask:39}),O(7705,'P',{bid:39,ask:43})],7702,all);
  eq(r.k,7700,'with two candidates it takes the strike NEAREST spot');
}
// ---- charm was REMOVED ----
// It was computed and never displayed. Asked what decision a "CHEX -$1.2B/day" cell changes, the honest
// answer was none — the consequence that matters (pins weaken into an expiry close) is already carried
// by the session phase tag. Carrying an unused computation is the same accumulation problem as carrying
// unused text, so it went rather than sat.
ok(!/function charmExposure/.test(src),'charm is gone from the companion, not merely unwired');
ok(!/\bch:charmExposure/.test(src),'and nothing still calls it');

// ---- gamma flip ----
{
  // calls above, puts below: the book flips somewhere between them
  const opts=[];
  for(let k=7600;k<=7700;k+=10) opts.push(O(k,'P',{openInterest:200}));
  for(let k=7710;k<=7810;k+=10) opts.push(O(k,'C',{openInterest:200}));
  const r=gammaFlip(opts,7700,all,20260821);
  ok(!!r,'a two-sided book produces a flip level');
  ok(r.flip>7650 && r.flip<7780,'and it lands between the put mass and the call mass',r&&r.flip);
  ok(/FALLBACK ONLY/.test(src),'and the source states it is a FALLBACK — their published level wins when the payload carries one');
}
{
  const opts=[]; for(let k=7600;k<=7800;k+=10) opts.push(O(k,'C',{openInterest:200}));
  const r=gammaFlip(opts,7700,all,20260821);
  eq(r,null,'an all-call book never crosses zero, so there is no flip to report');
}
{
  const opts=[O(7700,'C'),O(7700,'P')];
  eq(gammaFlip(opts,7700,all,20260821),null,'too few contracts to trust — reported as nothing rather than a number from two quotes');
}
// ---- the maths helpers ----
ok(Math.abs(npdf(0)-0.3989)<0.001,'the normal density at zero is 1/sqrt(2pi)');
ok(yearsTo(O(7700,'C',{expireDay:28}),20260821)>0,'days to expiry are positive for a future expiry');
eq(yearsTo(O(7700,'C',{expireDay:20}),20260821),null,'an expiry in the past is null, not negative time');
ok(d1Of(7700,7700,0.12,0.02)!==null,'d1 computes at the money');
eq(d1Of(7700,7700,0,0.02),null,'zero vol has no d1 rather than an infinity');
console.log('\n'+pass+' pass / '+fail+' fail');
