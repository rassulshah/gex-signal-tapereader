// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function A_combinedEdge(day, sym){
  sym=sym||'SPY'; var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var trap={n:0,hit:0}, lift={n:0,hit:0}, comp={n:0,range:0}, flow={n:0,hit:0}; var baseN=0,baseUp=0;
  snaps.forEach(function(s){ var o=s.out10; if(!o) return; var px=A_num(s.px); if(px==null) return; baseN++; if(o.net>0) baseUp++;
    var rB=false,rF=false,sB=false,sF=false,pol=0;
    (s.nodes||[]).forEach(function(nd){ var side=A_sideOf(nd,px); var net=A_num(nd.net); if(!side||net==null) return;
      if(side==='above'){ if(net>0){rB=true;pol-=Math.abs(net);} else if(net<0){rF=true;pol+=Math.abs(net);} }
      else { if(net>0){sB=true;pol+=Math.abs(net);} else if(net<0){sF=true;pol-=Math.abs(net);} } });
    if(rB&&sF){ trap.n++; if(o.net<0) trap.hit++; }
    if(sB&&rF){ lift.n++; if(o.net>0) lift.hit++; }
    if(sB&&rB){ comp.n++; if(Math.abs(o.net)<0.15||(o.revUp&&o.revDn)) comp.range++; }
    if(Math.abs(pol)>0.001){ flow.n++; var pu=pol>0; if((pu&&o.net>0)||(!pu&&o.net<0)) flow.hit++; } });
  return { baseline:A_pct(baseUp,baseN), baseN:baseN,
    trapdoor:{n:trap.n,hit:A_pct(trap.hit,trap.n)}, liftoff:{n:lift.n,hit:A_pct(lift.hit,lift.n)},
    compression:{n:comp.n,rangeRate:A_pct(comp.range,comp.n)},
    dualVsSingle:{dualN:trap.n+lift.n,dualHit:A_pct(trap.hit+lift.hit,trap.n+lift.n)},
    netFlow:{n:flow.n,dirHit:A_pct(flow.hit,flow.n)} };
}
