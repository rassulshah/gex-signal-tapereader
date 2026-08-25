const fs=require('fs');
const files=['data/2026-08-17.json','data/2026-08-18.json','data/2026-08-19.json','data/2026-08-20.json'];
const DAYS=files.map(f=>{try{return[f,JSON.parse(fs.readFileSync(f,'utf8'))]}catch(e){return null}}).filter(Boolean);

function run(TOL,K,HOLD,BREAK){
  const tally={nearF:{t:0,h:0},bigF:{t:0,h:0},nearC:{t:0,h:0},bigC:{t:0,h:0}};
  const seen={nearF:new Set(),bigF:new Set(),nearC:new Set(),bigC:new Set()};
  for(const [f,j] of DAYS){
    for(const sym of Object.keys(j.snaps||{})){
      const A=j.snaps[sym].filter(s=>typeof s.px==='number'&&Array.isArray(s.nodes)&&s.nodes.length);
      for(let i=0;i<A.length-K;i++){
        const s=A[i];
        const bl=s.nodes.filter(n=>n.side==='below'&&typeof n.k==='number');
        const ab=s.nodes.filter(n=>n.side==='above'&&typeof n.k==='number');
        const C={nearF:bl.length?bl.reduce((a,b)=>b.k>a.k?b:a):null,
                 bigF:bl.length?bl.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null,
                 nearC:ab.length?ab.reduce((a,b)=>b.k<a.k?b:a):null,
                 bigC:ab.length?ab.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null};
        const fwd=A.slice(i+1,i+1+K);
        for(const key of Object.keys(C)){
          const c=C[key]; if(!c) continue;
          const isF=key.endsWith('F');
          let hit=-1;
          for(let q=0;q<fwd.length;q++){ if(isF?(fwd[q].l<=c.k+TOL):(fwd[q].h>=c.k-TOL)){hit=q;break;} }
          if(hit<0) continue;
          const id=f+sym+c.k.toFixed(2)+(i+1+hit); if(seen[key].has(id))continue; seen[key].add(id);
          const r=fwd.slice(hit);
          const held=isF?((Math.max(...r.map(x=>x.h))-c.k>=HOLD)&&(Math.min(...r.map(x=>x.l))>c.k-BREAK))
                        :((c.k-Math.min(...r.map(x=>x.l))>=HOLD)&&(Math.max(...r.map(x=>x.h))<c.k+BREAK));
          tally[key].t++; if(held)tally[key].h++;
        }
      }
    }
  }
  const p=o=>o.t?Math.round(o.h/o.t*100):null;
  return {F:[p(tally.nearF),p(tally.bigF),tally.nearF.t,tally.bigF.t],
          C:[p(tally.nearC),p(tally.bigC),tally.nearC.t,tally.bigC.t]};
}
console.log('TOL  K  HOLD BREAK |  FLOOR near/big (n)   |  CEIL near/big (n)');
for(const [TOL,K,H,B] of [[0.5,8,0.25,0.30],[0.5,5,0.25,0.30],[0.5,12,0.25,0.30],
                          [0.3,8,0.25,0.30],[0.7,8,0.25,0.30],
                          [0.5,8,0.15,0.20],[0.5,8,0.40,0.50],[0.5,10,0.30,0.40]]){
  const r=run(TOL,K,H,B);
  console.log(`${TOL}  ${String(K).padStart(2)}  ${H}  ${B}  |  ${String(r.F[0]).padStart(3)}% / ${String(r.F[1]).padStart(3)}%  (${r.F[2]}/${r.F[3]})  |  ${String(r.C[0]).padStart(3)}% / ${String(r.C[1]).padStart(3)}%  (${r.C[2]}/${r.C[3]})`);
}
