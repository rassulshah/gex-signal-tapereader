// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function kingPathSigMoves(pts, now){
  var out=[];
  for(var v=1; v<pts.length-0; v++){
    if(v===pts.length-1) continue;                        // last = gutter's job
    var step=Math.abs(pts[v].k-pts[v-1].k);
    var dwell=((v+1<pts.length)?pts[v+1].t:now)-pts[v].t;
    if(step>=2 || dwell>=15*60000) out.push({i:v, k:pts[v].k, up:pts[v].k>pts[v-1].k, step:step, dwellM:Math.round(dwell/60000)});
  }
  if(out.length>5){                                        // keep the 5 most significant
    out.sort(function(a,b){ return (b.step*30+b.dwellM)-(a.step*30+a.dwellM); });
    out=out.slice(0,5);
    out.sort(function(a,b){ return a.i-b.i; });
  }
  return out;
}
