const fs=require('fs');
const {classifyTouch}=require('./outcome-classify.js');
const files=['data/2026-08-17.json','data/2026-08-18.json','data/2026-08-19.json','data/2026-08-20.json'];
const LOOK=5, MINABS=2e6, K=8, BIGPCT=60;

function tally(){ return {DEFLECT:0,PIN:0,BREAK:0,GRAZE:0,n:0}; }
function add(t,k){ t[k]++; t.n++; }
function fmt(t){
  if(!t.n) return 'n=0';
  const w=t.DEFLECT+t.PIN;
  return `n=${String(t.n).padStart(3)}  works ${String(Math.round(w/t.n*100)).padStart(3)}%  ` +
         `[defl ${String(Math.round(t.DEFLECT/t.n*100)).padStart(2)}%  pin ${String(Math.round(t.PIN/t.n*100)).padStart(2)}%  ` +
         `BREAK ${String(Math.round(t.BREAK/t.n*100)).padStart(2)}%  graze ${Math.round(t.GRAZE/t.n*100)}%]`;
}
const bySize={big:tally(),small:tally()}, byGrow={up:tally(),flat:tally(),dn:tally()},
      byNearBig={near:tally(),big:tally()}, cross={};

for(const f of files){
  let j; try{ j=JSON.parse(fs.readFileSync(f,'utf8')); }catch(e){ continue; }
  for(const sym of Object.keys(j.snaps||{})){
    const A=j.snaps[sym].filter(s=>typeof s.px==='number'&&Array.isArray(s.nodes)&&s.nodes.length);
    for(let i=LOOK;i<A.length-K;i++){
      const now=A[i], then=A[i-LOOK];
      const mT={}; then.nodes.forEach(n=>{ if(typeof n.abs==='number') mT[n.k]=n; });
      const fwd=A.slice(i+1,i+1+K);
      const below=now.nodes.filter(n=>n.side==='below'), above=now.nodes.filter(n=>n.side==='above');
      const nearF=below.length?below.reduce((a,b)=>b.k>a.k?b:a):null;
      const bigF =below.length?below.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null;
      const nearC=above.length?above.reduce((a,b)=>b.k<a.k?b:a):null;
      const bigC =above.length?above.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null;
      [[nearF,'near'],[bigF,'big'],[nearC,'near'],[bigC,'big']].forEach(([n,b])=>{
        if(!n) return; const c=classifyTouch(n.k,n.side,fwd); if(c) add(byNearBig[b],c.kind);
      });
      for(const n of now.nodes){
        if(typeof n.k!=='number'||typeof n.abs!=='number'||!n.side) continue;
        const prev=mT[n.k]; if(!prev) continue;
        const d=n.abs-prev.abs;
        const g=d>MINABS?'up':d<-MINABS?'dn':'flat';
        const big=(n.pct||0)>=BIGPCT;
        const c=classifyTouch(n.k,n.side,fwd); if(!c) continue;
        add(bySize[big?'big':'small'],c.kind); add(byGrow[g],c.kind);
        const kk=(big?'BIG  ':'small')+' x '+g.padEnd(4);
        (cross[kk]=cross[kk]||tally()); add(cross[kk],c.kind);
      }
    }
  }
}
console.log('THREE-WAY OUTCOMES — "works" = DEFLECT + PIN (both are the level functioning)\n');
console.log('BY ROLE');
console.log('  nearest (gatekeeper):', fmt(byNearBig.near));
console.log('  biggest (structural):', fmt(byNearBig.big));
console.log('\nBY MAGNITUDE');
console.log('  big  (>='+BIGPCT+'% King):', fmt(bySize.big));
console.log('  small            :', fmt(bySize.small));
console.log('\nBY DIRECTION OF CHANGE');
console.log('  growing:', fmt(byGrow.up));
console.log('  flat   :', fmt(byGrow.flat));
console.log('  fading :', fmt(byGrow.dn));
console.log('\nCROSSED');
Object.keys(cross).sort().forEach(k=>{ if(cross[k].n>=12) console.log('  '+k+':', fmt(cross[k])); });
