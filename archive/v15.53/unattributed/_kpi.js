// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function _kpi(label,val,sub,pct){
  var col = pct==null?PAL.ink:(pct>=70?PAL.longAccent:(pct>=55?PAL.amber:PAL.shortAccent));
  return '<div style="background:#12161f;border:1px solid '+PAL.line+';border-radius:8px;padding:6px 8px"><div style="font-size:9px;color:'+PAL.sub+'">'+label+'</div><div style="font-size:15px;font-weight:800;color:'+col+'">'+val+'</div><div style="font-size:8px;color:'+PAL.sub+'">'+sub+'</div></div>';
}
