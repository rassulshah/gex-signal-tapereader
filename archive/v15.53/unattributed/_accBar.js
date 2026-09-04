// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function _accBar(pct,label,note){
  var col = pct==null?PAL.sub : (pct>=70?PAL.longAccent:(pct>=55?PAL.amber:PAL.shortAccent));
  var w = pct==null?0:pct;
  return '<div style="display:flex;align-items:center;gap:8px;padding:4px 0;border-bottom:1px solid #161b26">'+
    '<div style="flex:0 0 92px;font-size:11px;font-weight:700">'+label+'</div>'+
    '<div style="flex:1;height:8px;background:#0b0e14;border-radius:6px;overflow:hidden"><div style="width:'+w+'%;height:100%;background:'+col+'"></div></div>'+
    '<div style="flex:0 0 34px;text-align:right;font-size:11px;font-weight:800;color:'+col+'">'+(pct==null?'\u2013':pct+'%')+'</div>'+
    '<div style="flex:0 0 74px;font-size:9px;color:'+PAL.sub+';text-align:right">'+(note||'')+'</div>'+
  '</div>';
}
