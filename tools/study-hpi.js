// (v14.5 study) HEDGING PRESSURE — does the dealer's OBLIGATION lead, confirm, or diverge?
//
// HPI per bar: F_t = -Gamma_local(t-1) * dPx_t.  Sign>0 = dealers forced BUYERS during the bar.
// Regime = sign of Gamma_local. Claims tested SEPARATELY, each against an honest null:
//   T1 CONFIRM: sign(F_t) -> sign(px[t+H]-px[t]), split by regime; null = sign(dPx_t) (momentum).
//   T2 LEAD:    sign(-Gamma_local(t)) alone -> forward drift (the regime as a directional bias).
//   T3 DIVERGE: 10-bar window where price direction and cumulative F disagree -> reversal rate
//               vs windows where they agree.
// Old-schema days (08-17..20) carry no dollars; Gamma_local is the SIGNED RELATIVE book:
// sum over nodes within a band of spot of (pos ? +1 : -1) * pct(%King). Sign and shape are what
// the tests need; dollar scaling cancels in every sign-based statistic.
const fs=require('fs');
const H=5;                      // forward horizon, bars (15m)
const W=10;                     // divergence window
const BAND=3.0;                 // +-SPY pts around spot for "local"
function pct(n,d){ return d? (100*n/d).toFixed(1)+'% ('+n+'/'+d+')' : '-'; }

const agg={ t1:{}, t2:{}, t3:{agree:{n:0,rev:0}, diverge:{n:0,rev:0}}, nullm:{} };
function bump(o,k,hit){ (o[k]=o[k]||{n:0,hit:0}); o[k].n++; if(hit) o[k].hit++; }

const days=process.argv.slice(2);
let barsTotal=0;
for(const f of days){
  const d=JSON.parse(fs.readFileSync(f,'utf8'));
  const sn=(d.snaps&&d.snaps.SPY)||[];
  if(sn.length<W+H+2) continue;
  // per-bar series: px and Gamma_local (signed relative)
  const px=[], G=[];
  for(const s of sn){
    px.push(s.px);
    let g=0;
    for(const nd of (s.nodes||[])){
      if(typeof nd.k!=='number'||typeof nd.pct!=='number') continue;
      if(Math.abs(nd.k-s.px)>BAND) continue;
      g += (nd.pos?1:-1)*nd.pct;
    }
    G.push(g);
  }
  barsTotal+=sn.length;
  const F=[]; F.push(0);
  for(let t=1;t<px.length;t++) F.push(-G[t-1]*(px[t]-px[t-1]));
  for(let t=1;t<px.length-H;t++){
    const fwd=px[t+H]-px[t]; if(Math.abs(fwd)<0.02) continue;      // flat forward = no verdict
    const reg=(G[t]>0)?'+G':(G[t]<0?'-G':null); if(!reg) continue;
    if(Math.abs(F[t])>0){
      bump(agg.t1, reg, (F[t]>0)===(fwd>0));
      bump(agg.t1, 'all', (F[t]>0)===(fwd>0));
    }
    const dp=px[t]-px[t-1];
    if(Math.abs(dp)>0){ bump(agg.nullm, reg, (dp>0)===(fwd>0)); bump(agg.nullm,'all',(dp>0)===(fwd>0)); }
    bump(agg.t2, reg, ((-G[t])>0)===(fwd>0));
  }
  // T3: non-overlapping windows
  for(let t=W;t<px.length-H;t+=W){
    const move=px[t]-px[t-W]; if(Math.abs(move)<0.10) continue;
    let cf=0; for(let j=t-W+1;j<=t;j++) cf+=F[j];
    if(Math.abs(cf)===0) continue;
    const agree=(move>0)===(cf>0);
    const rev=((px[t+H]-px[t])>0)!==(move>0);                       // forward move flips = reversal
    const cell=agree?agg.t3.agree:agg.t3.diverge;
    cell.n++; if(rev) cell.rev++;
  }
}
console.log('days:',days.length,' bars:',barsTotal,' (effective n ~ bars/'+H+' for T1/T2; T3 windows are non-overlapping)');
console.log('\nT1 CONFIRM  sign(F_t) -> sign of next-'+H+'-bar move   [null: momentum sign(dPx_t)]');
for(const k of ['+G','-G','all'])
  console.log('  '+k.padEnd(4), 'HPI:', pct(agg.t1[k]?agg.t1[k].hit:0, agg.t1[k]?agg.t1[k].n:0).padEnd(18),
              'null:', pct(agg.nullm[k]?agg.nullm[k].hit:0, agg.nullm[k]?agg.nullm[k].n:0));
console.log('\nT2 LEAD  sign(-Gamma_local) alone -> next-'+H+'-bar move');
for(const k of ['+G','-G'])
  console.log('  '+k.padEnd(4), pct(agg.t2[k]?agg.t2[k].hit:0, agg.t2[k]?agg.t2[k].n:0));
console.log('\nT3 DIVERGENCE  '+W+'-bar price move vs cumulative F  ->  reversal within '+H+' bars');
console.log('  agree   :', pct(agg.t3.agree.rev, agg.t3.agree.n));
console.log('  diverge :', pct(agg.t3.diverge.rev, agg.t3.diverge.n));
