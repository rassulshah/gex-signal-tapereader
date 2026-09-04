// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeHistRow(m){
  var out={ px:null, ceil:null, flr:null };
  try{
    if(!m || m.px==null) return out;
    var px=m.px; out.px=+px.toFixed(2);
    (m.levels||[]).forEach(function(L){
      if(!L || typeof L.k!=='number') return;
      var pct=(typeof L.pct==='number')?Math.abs(L.pct):(L.isKing?100:0);
      if(!(L.isKing || pct>=PB_MIN_PCT)) return;
      if(L.k>px+0.001){ if(!out.ceil || L.k<out.ceil.k) out.ceil={ k:L.k, pct:pct, state:L.state||null }; }
      else if(L.k<px-0.001){ if(!out.flr || L.k>out.flr.k) out.flr={ k:L.k, pct:pct, state:L.state||null }; }
    });
  }catch(e){}
  return out;
}
