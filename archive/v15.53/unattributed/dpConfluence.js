// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function dpConfluence(sym, disp, scale){
  try{
    var D=darkPoolLevels(sym);
    if(!D || !(scale>0) || typeof disp!=='number') return null;
    // (v14.44) the S&R clause speaks the STATE too, because "on the dark pool" and "on a dark pool
    // that already broke" are opposite pieces of advice. GM-DP-003: a broken level only becomes
    // resistance after a rejection — which is exactly what FLIPPED means and BROKEN does not.
    var LC=null; try{ LC=dpLifecycle(sym); }catch(e0){}
    var best=null, bd=SESS_CONFL_PTS;
    D.prints.forEach(function(pr, i){
      var at=pr.px*scale, d=Math.abs(at-disp);
      if(d<=bd){ bd=d;
        var st=(LC&&LC[i]&&LC[i].st&&LC[i].st!=='UNKNOWN')?LC[i].st:null;
        best={ name:'the dark pool '+frameNum(at)+(st?(' ('+st.toLowerCase()+')'):''), at:at, d:d, st:st }; }
    });
    return best;
  }catch(e){ return null; }
}
