const fs=require('fs');
const files=['data/2026-08-17.json','data/2026-08-18.json','data/2026-08-19.json','data/2026-08-20.json'];
const TOL=0.50, K=8, HOLD=0.25, BREAK=0.30;

// For each candidate definition: when price REACHED it (within TOL) over the next K bars,
// did it hold (bounce >= HOLD from the touch) or break (trade BREAK beyond)?
const tally={ nearF:{t:0,h:0}, bigF:{t:0,h:0}, nearC:{t:0,h:0}, bigC:{t:0,h:0} };
const seen={ nearF:new Set(), bigF:new Set(), nearC:new Set(), bigC:new Set() };

for(const f of files){
  let j; try{ j=JSON.parse(fs.readFileSync(f,'utf8')); }catch(e){ continue; }
  for(const sym of Object.keys(j.snaps||{})){
    const A=j.snaps[sym].filter(s=>typeof s.px==='number'&&Array.isArray(s.nodes)&&s.nodes.length);
    for(let i=0;i<A.length-K;i++){
      const s=A[i];
      const below=s.nodes.filter(n=>n.side==='below'&&typeof n.k==='number');
      const above=s.nodes.filter(n=>n.side==='above'&&typeof n.k==='number');
      const cands={
        nearF: below.length? below.reduce((a,b)=>b.k>a.k?b:a):null,
        bigF : below.length? below.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null,
        nearC: above.length? above.reduce((a,b)=>b.k<a.k?b:a):null,
        bigC : above.length? above.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null
      };
      const fwd=A.slice(i+1,i+1+K);
      for(const key of Object.keys(cands)){
        const c=cands[key]; if(!c) continue;
        const isFloor=key.endsWith('F');
        // first bar that reaches the level
        let hit=-1;
        for(let q=0;q<fwd.length;q++){
          if(isFloor ? (fwd[q].l<=c.k+TOL) : (fwd[q].h>=c.k-TOL)){ hit=q; break; }
        }
        if(hit<0) continue;
        // dedupe: one verdict per (day,sym,level,touch-bar)
        const id=f+sym+c.k.toFixed(2)+(i+1+hit);
        if(seen[key].has(id)) continue; seen[key].add(id);
        const rest=fwd.slice(hit);
        const held = isFloor
          ? (Math.max(...rest.map(x=>x.h)) - c.k >= HOLD) && (Math.min(...rest.map(x=>x.l)) > c.k - BREAK)
          : (c.k - Math.min(...rest.map(x=>x.l)) >= HOLD) && (Math.max(...rest.map(x=>x.h)) < c.k + BREAK);
        tally[key].t++; if(held) tally[key].h++;
      }
    }
  }
}
function pc(o){ return o.t? (o.h/o.t*100).toFixed(0)+'%  ('+o.h+'/'+o.t+')' : 'n=0'; }
console.log('HOLD RATE when price actually reached the level (±'+TOL+'), forward '+K+' bars\n');
console.log('FLOOR   nearest below :', pc(tally.nearF));
console.log('        biggest below :', pc(tally.bigF));
console.log('CEILING nearest above :', pc(tally.nearC));
console.log('        biggest above :', pc(tally.bigC));
