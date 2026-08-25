// Do rolls into NON-LARGEST nodes provide support? Reconstructed from recorded snapshots.
const fs=require('fs');
const files=['data/2026-08-17.json','data/2026-08-18.json','data/2026-08-19.json','data/2026-08-20.json'];
const LOOK=5;        // bars over which a migration is measured (~15 min)
const NEAR=3.0;      // max strike distance source->destination (SPY dollars)
const MINABS=2e6;    // dollars shed / gained to count as material
const RATIO=0.40;    // destination must take >= this share of what source shed
const TOL=0.50, K=8, HOLD=0.25, BREAK=0.30;

const out={ big:{t:0,h:0}, small:{t:0,h:0} };
const rolls=[];

for(const f of files){
  let j; try{ j=JSON.parse(fs.readFileSync(f,'utf8')); }catch(e){ continue; }
  for(const sym of Object.keys(j.snaps||{})){
    const A=j.snaps[sym].filter(s=>typeof s.px==='number'&&Array.isArray(s.nodes)&&s.nodes.length);
    for(let i=LOOK;i<A.length-K;i++){
      const now=A[i], then=A[i-LOOK];
      const mapNow={}, mapThen={};
      now.nodes.forEach(n=>{ if(typeof n.k==='number'&&typeof n.abs==='number') mapNow[n.k]=n; });
      then.nodes.forEach(n=>{ if(typeof n.k==='number'&&typeof n.abs==='number') mapThen[n.k]=n; });
      const delta={};
      for(const k of Object.keys(mapNow)) if(mapThen[k]) delta[k]=mapNow[k].abs-mapThen[k].abs;
      const ks=Object.keys(delta).map(Number);
      // largest node on each side, right now
      const below=now.nodes.filter(n=>n.side==='below'), above=now.nodes.filter(n=>n.side==='above');
      const bigBelow=below.length?below.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null;
      const bigAbove=above.length?above.reduce((a,b)=>(b.abs||0)>(a.abs||0)?b:a):null;
      for(const src of ks){
        if(!(delta[src] < -MINABS)) continue;                 // shedding materially
        let best=null;
        for(const dst of ks){
          if(dst===src) continue;
          if(Math.abs(dst-src)>NEAR) continue;
          if(!(delta[dst] > MINABS)) continue;                 // receiving materially
          if(delta[dst]/Math.abs(delta[src]) < RATIO) continue;
          if(!best || delta[dst]>delta[best]) best=dst;
        }
        if(best===null) continue;
        const dnode=mapNow[best];
        const isBig = (bigBelow&&Math.abs(bigBelow.k-best)<1e-9) || (bigAbove&&Math.abs(bigAbove.k-best)<1e-9);
        // forward: did price reach the DESTINATION and hold?
        const fwd=A.slice(i+1,i+1+K);
        const isFloor = dnode.side==='below';
        let hit=-1;
        for(let q=0;q<fwd.length;q++){ if(isFloor?(fwd[q].l<=best+TOL):(fwd[q].h>=best-TOL)){hit=q;break;} }
        if(hit<0) continue;
        const r=fwd.slice(hit);
        const held=isFloor?((Math.max(...r.map(x=>x.h))-best>=HOLD)&&(Math.min(...r.map(x=>x.l))>best-BREAK))
                          :((best-Math.min(...r.map(x=>x.l))>=HOLD)&&(Math.max(...r.map(x=>x.h))<best+BREAK));
        const bucket=isBig?'big':'small';
        out[bucket].t++; if(held) out[bucket].h++;
        rolls.push({f:f.slice(5,15),sym,src,dst:best,dir:best>src?'up':'dn',
                    pctOfBiggest: dnode.pct, isBig, held, side:dnode.side});
      }
    }
  }
}
const pc=o=>o.t?(o.h/o.t*100).toFixed(0)+'%  ('+o.h+'/'+o.t+')':'n=0';
console.log('ROLL DESTINATION held when price reached it (±'+TOL+', '+K+' bars fwd)\n');
console.log('  destination IS the largest node on its side :', pc(out.big));
console.log('  destination is NOT the largest (secondary)  :', pc(out.small));
console.log('\ntotal rolls detected:', rolls.length);
const sm=rolls.filter(r=>!r.isBig);
console.log('secondary-destination rolls:', sm.length,
            '| median %King of destination:', sm.length? sm.map(r=>r.pctOfBiggest).sort((a,b)=>a-b)[Math.floor(sm.length/2)] : '-');
console.log('\nsample secondary rolls that HELD:');
sm.filter(r=>r.held).slice(0,6).forEach(r=>console.log('  ',r.f,r.sym,r.src+' -> '+r.dst,'('+r.dir+', '+r.side+', '+r.pctOfBiggest+'% of King)'));

// ---- CONTROL: is "roll destination" just a proxy for "node that grew"? ----
const ctl={ rollDst:{t:0,h:0}, grewOnly:{t:0,h:0}, flat:{t:0,h:0} };
for(const f of files){
  let j; try{ j=JSON.parse(fs.readFileSync(f,'utf8')); }catch(e){ continue; }
  for(const sym of Object.keys(j.snaps||{})){
    const A=j.snaps[sym].filter(s=>typeof s.px==='number'&&Array.isArray(s.nodes)&&s.nodes.length);
    for(let i=LOOK;i<A.length-K;i++){
      const now=A[i], then=A[i-LOOK];
      const mN={}, mT={}; now.nodes.forEach(n=>mN[n.k]=n); then.nodes.forEach(n=>mT[n.k]=n);
      const d={}; for(const k of Object.keys(mN)) if(mT[k]&&typeof mN[k].abs==='number'&&typeof mT[k].abs==='number') d[k]=mN[k].abs-mT[k].abs;
      const ks=Object.keys(d).map(Number);
      const isDst={};
      for(const src of ks){ if(!(d[src]<-MINABS)) continue;
        for(const dst of ks){ if(dst!==src&&Math.abs(dst-src)<=NEAR&&d[dst]>MINABS&&d[dst]/Math.abs(d[src])>=RATIO) isDst[dst]=1; } }
      const fwd=A.slice(i+1,i+1+K);
      for(const k of ks){
        const n=mN[k]; if(!n||typeof n.side!=='string') continue;
        const grew=d[k]>MINABS;
        const bucket = isDst[k] ? 'rollDst' : (grew ? 'grewOnly' : 'flat');
        if(bucket==='flat' && Math.abs(d[k])>MINABS) continue;   // keep 'flat' actually flat
        const isFloor=n.side==='below';
        let hit=-1;
        for(let q=0;q<fwd.length;q++){ if(isFloor?(fwd[q].l<=k+TOL):(fwd[q].h>=k-TOL)){hit=q;break;} }
        if(hit<0) continue;
        const r=fwd.slice(hit);
        const held=isFloor?((Math.max(...r.map(x=>x.h))-k>=HOLD)&&(Math.min(...r.map(x=>x.l))>k-BREAK))
                          :((k-Math.min(...r.map(x=>x.l))>=HOLD)&&(Math.max(...r.map(x=>x.h))<k+BREAK));
        ctl[bucket].t++; if(held) ctl[bucket].h++;
      }
    }
  }
}
console.log('\nCONTROL — hold rate by what the node was doing');
console.log('  roll DESTINATION (paired: something shed into it):', pc(ctl.rollDst));
console.log('  GREW but with no shedding source nearby         :', pc(ctl.grewOnly));
console.log('  flat (no material change)                       :', pc(ctl.flat));
