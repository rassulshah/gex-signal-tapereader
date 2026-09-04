// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function ifChainRows(sym, which){
  try{
    var d=ifChain(sym); if(!d || d.err) return null;
    var w=d[which||'toFri']; if(!w || !w.lv) return null;
    var L=w.lv, out=[];
    if(L.cr!=null)  out.push({ id:'CR',  k:L.cr,  gex:null });
    if(L.mag!=null) out.push({ id:'Mag', k:L.mag, gex:null });
    if(L.ps!=null)  out.push({ id:'PS',  k:L.ps,  gex:null });
    if(L.maxPain!=null) out.push({ id:'MP*', k:L.maxPain, gex:null });
    out.sort(function(a,b){ return b.k-a.k; });
    return { rows:out, lv:L, exps:w.exps, which:(which||'toFri'),
             ageMin:d.ageMin, stale:d.stale, spot:d.spot };
  }catch(e){ return null; }
}
