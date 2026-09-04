// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function A_accumEdge(day, sym, mode){
  sym=sym||'SPY'; var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var B={ sup:{n:0,dh:0,sh:0,mfe:0,mae:0}, res:{n:0,dh:0,sh:0,mfe:0,mae:0} };
  var baseN=0, baseUp=0;
  snaps.forEach(function(s){ var o=s.out10; if(!o) return; var px=A_num(s.px); if(px==null) return; baseN++; if(o.net>0) baseUp++;
    (s.nodes||[]).forEach(function(nd){ var side=A_sideOf(nd,px); if(!side) return; var net=A_num(nd.net); if(net==null) return;
      var want=(mode==='accum')?(net>0):(net<0); if(!want) return;
      var expUp=(mode==='accum')?(side==='below'):(side==='above');
      var b=(side==='below')?B.sup:B.res; b.n++;
      var hit=expUp?(o.net>0):(o.net<0); if(hit) b.dh++;
      var mfe=A_num(o.mfe)||0, mae=A_num(o.mae)||0; b.mfe+=mfe; b.mae+=mae;
      var sw=expUp?(mfe> -mae):(-mae>mfe); if(sw) b.sh++;
    }); });
  function pk(b){ return { n:b.n, dirHit:A_pct(b.dh,b.n), swingHit:A_pct(b.sh,b.n), avgMFE:b.n?+(b.mfe/b.n).toFixed(2):null, avgMAE:b.n?+(b.mae/b.n).toFixed(2):null }; }
  return { mode:mode, baseline:A_pct(baseUp,baseN), baseN:baseN, support:pk(B.sup), resistance:pk(B.res) };
}
