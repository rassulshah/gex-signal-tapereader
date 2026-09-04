// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function dpLifecycle(sym){
  try{
    var D=darkPoolLevels(sym); if(!D) return null;
    var bars=null; try{ bars=futRawCandles(sym); }catch(e0){}
    var spot=null; try{ spot=(STATE[sym]||{}).price; }catch(e1){}
    return D.prints.map(function(pr){
      var lc=dpLifecycleOne(pr.px, pr.at, bars, spot);
      return { px:pr.px, size:pr.size, notional:pr.notional, at:pr.at,
               st:lc.st, why:lc.why, touches:lc.touches, obsFrom:lc.obsFrom };
    });
  }catch(e){ return null; }
}
