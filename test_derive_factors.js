// (v11.7) deriveFactors — three defects, each of which silently produced a wrong structural read:
//   1. SPXW-derived lanes were pooled with native nodes on a %King basis. Each lane is normalised to its
//      OWN King, so the percentages are not comparable; one foreign lane at 100% outranked a real native
//      wall and the wall disappeared from ranks/hhi/imb/zg alike.
//   2. Every factor was computed on %King — a ratio to a MOVING denominator. A King that grows shrinks
//      every other node's percentage even when their dollar exposure never changed.
//   3. The call wall required pos===true; the put wall required nothing. A negative-gamma ceiling was
//      therefore unreportable, and cw came back null for whole sessions.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(ex('deriveFactors'));

const N=(k,pct,abs,pos,extra)=>Object.assign({k:k,pct:pct,abs:abs,pos:pos,derived:false},extra||{});

// ---------- 1. derived lanes are excluded, not mixed ----------
{
  const nodes=[ N(765,60,600000,false), N(770,40,400000,true),
                Object.assign(N(999,100,null,true),{derived:true,src:'SPXW'}) ];
  const d=deriveFactors(nodes,767,765);
  ok(d.nNat===2,'native count excludes derived lanes',d&&d.nNat);
  ok(d.nSkipped===1,'derived lanes are counted, not discarded silently',d&&d.nSkipped);
  ok(d.ranks.every(r=>r.k!==999),'a derived lane never enters the GEX ranks',d&&d.ranks);
  ok(d.cw===770,'a foreign 100% lane above spot does not become the call wall',d&&d.cw);
}
// ---------- 2. absolute basis is preferred when the tape carries it ----------
{
  const nodes=[ N(765,60,900000,false), N(770,100,100000,true) ];
  const d=deriveFactors(nodes,767,770);
  ok(d.basis==='abs','abs basis chosen when every native node has abs',d&&d.basis);
  ok(d.cw===770&&d.pw===765,'walls still land on the right side of spot',d&&[d.cw,d.pw]);
  // on %King the 770 node (100) would outrank 765 (60); on dollars 765 is 9x bigger.
  ok(d.ranks[0].k===765,'rank 1 follows dollars, not %King',d&&d.ranks);
}
{
  const nodes=[ N(765,60,null,false), N(770,100,null,true) ];
  const d=deriveFactors(nodes,767,770);
  ok(d.basis==='pct','falls back to %King when abs is absent',d&&d.basis);
  ok(d.ranks[0].k===770,'on the pct basis the bigger percentage ranks first',d&&d.ranks);
}
{
  const nodes=[ N(765,60,900000,false), N(770,100,null,true) ];
  const d=deriveFactors(nodes,767,770);
  ok(d.basis==='pct','a partial abs series is NOT half-used — one missing node forces pct',d&&d.basis);
}
// ---------- 3. walls are symmetric ----------
{
  // ceiling above spot is NEGATIVE gamma. Old code required pos===true and returned cw:null.
  const nodes=[ N(770,80,800000,false), N(765,40,400000,false) ];
  const d=deriveFactors(nodes,767,770);
  ok(d.cw===770,'a negative-gamma ceiling is still reported as the call wall',d&&d.cw);
  ok(d.cwPos===false,'its polarity is recorded rather than required',d&&d.cwPos);
  ok(d.pw===765&&d.pwPos===false,'put side unchanged',d&&[d.pw,d.pwPos]);
  ok(d.cwM===800000&&d.pwM===400000,'wall masses are reported on the chosen basis',d&&[d.cwM,d.pwM]);
}
{
  const nodes=[ N(770,80,800000,true), N(772,90,900000,true), N(765,40,400000,false) ];
  const d=deriveFactors(nodes,767,772);
  ok(d.cw===772,'heaviest mass above spot wins, not the nearest',d&&d.cw);
}
// ---------- structural invariants ----------
{
  const nodes=[ N(765,60,600000,false), N(770,40,400000,true) ];
  const d=deriveFactors(nodes,767,765);
  ok(d.ns===-1,'net sign follows signed mass (600k short vs 400k long)',d&&d.ns);
  ok(d.reg==='negGamma'||d.reg==='posGamma','a regime is always emitted',d&&d.reg);
  ok(d.imb!==null&&d.imb<0,'imbalance is negative when mass sits below spot',d&&d.imb);
  ok(d.hhi>0&&d.hhi<=1,'hhi is a proper concentration ratio',d&&d.hhi);
}
{
  const d=deriveFactors([Object.assign(N(999,100,1,true),{derived:true,src:'SPXW'})],767,null);
  ok(d===null,'an all-derived tape yields no factors rather than a fabricated read',d);
  ok(deriveFactors([],767,null)===null,'empty node set returns null');
  ok(deriveFactors([N(765,60,1,false)],null,null)===null,'no price returns null');
}
{
  // Cumulative signed mass, ascending: 760 -500k, 765 -1.0M, 770 -200k, 775 +600k.
  // The sign change is 770 -> 775, NOT 765 -> 770: two put strikes below still outweigh the first call
  // strike. zg interpolates 200/800 of the way = 771.25. Getting this bracket wrong is how a zero-gamma
  // line ends up drawn a full strike early, on the wrong side of price.
  const nodes=[ N(760,50,500000,false), N(765,50,500000,false), N(770,80,800000,true), N(775,80,800000,true) ];
  const d=deriveFactors(nodes,767,770);
  ok(d.zg===771.25,'zero gamma interpolates inside the true crossing bracket',d&&d.zg);
  ok(d.reg==='negGamma','spot 767 below zg 771.25 is negative gamma',d&&d.reg);
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
