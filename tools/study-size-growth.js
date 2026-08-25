// Does magnitude add anything ON TOP of growth? 2x2: big/small x growing/flat.
const fs=require('fs');
const files=['data/2026-08-17.json','data/2026-08-18.json','data/2026-08-19.json','data/2026-08-20.json'];
const LOOK=5, MINABS=2e6, TOL=0.50, K=8, HOLD=0.25, BREAK=0.30;
const BIGPCT=60;   // "big" = >= this % of King on its side

const cell={};
const key=(b,g)=>(b?'BIG ':'small')+' x '+(g==='up'?'GROWING':g==='dn'?'FADING ':'flat   ');
for(const f of files){
  let j; try{ j=JSON.parse(fs.readFileSync(f,'utf8')); }catch(e){ continue; }
  for(const sym of Object.keys(j.snaps||{})){
    const A=j.snaps[sym].filter(s=>typeof s.px==='number'&&Array.isArray(s.nodes)&&s.nodes.length);
    for(let i=LOOK;i<A.length-K;i++){
      const now=A[i], then=A[i-LOOK];
      const mT={}; then.nodes.forEach(n=>{ if(typeof n.abs==='number') mT[n.k]=n; });
      const fwd=A.slice(i+1,i+1+K);
      for(const n of now.nodes){
        if(typeof n.k!=='number'||typeof n.abs!=='number'||!n.side) continue;
        const prev=mT[n.k]; if(!prev) continue;
        const d=n.abs-prev.abs;
        const g = d>MINABS?'up' : d<-MINABS?'dn' : 'flat';
        const big=(n.pct||0)>=BIGPCT;
        const isFloor=n.side==='below';
        let hit=-1;
        for(let q=0;q<fwd.length;q++){ if(isFloor?(fwd[q].l<=n.k+TOL):(fwd[q].h>=n.k-TOL)){hit=q;break;} }
        if(hit<0) continue;
        const r=fwd.slice(hit);
        const held=isFloor?((Math.max(...r.map(x=>x.h))-n.k>=HOLD)&&(Math.min(...r.map(x=>x.l))>n.k-BREAK))
                          :((n.k-Math.min(...r.map(x=>x.l))>=HOLD)&&(Math.max(...r.map(x=>x.h))<n.k+BREAK));
        const kk=key(big,g);
        (cell[kk]=cell[kk]||{t:0,h:0}); cell[kk].t++; if(held) cell[kk].h++;
      }
    }
  }
}
console.log('HOLD RATE — magnitude x direction of change   (big = >='+BIGPCT+'% of King)\n');
Object.keys(cell).sort().forEach(k=>{
  const o=cell[k];
  if(o.t<12){ console.log('  '+k+' :  n='+o.t+'  (too few)'); return; }
  console.log('  '+k+' :  '+(o.h/o.t*100).toFixed(0)+'%   ('+o.h+'/'+o.t+')');
});
